/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2018 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifdef UNIT_TEST
#define pnal_get_system_uptime_10ms mock_pnal_get_system_uptime_10ms
#define pnal_get_interface_index    mock_pnal_get_interface_index
#endif

#define STRINGIFY(s)   STRINGIFIED (s)
#define STRINGIFIED(s) #s

/**
 * @file
 * @brief Implements Link Layer Discovery Protocol (LLDP), for neighbourhood
 * detection.
 *
 * Builds and sends LLDP frames.
 * Receives and parses LLDP frames.
 *
 * This file should not have any knowledge of Profinet slots, subslots etc.
 *
 * ToDo: Differentiate between device and port MAC addresses.
 */

#include "pf_includes.h"
#include "pf_block_writer.h"
#include "pf_block_reader.h"
#include "pf_cmrpc.h"

#include <string.h>
#include <ctype.h>

#if PNET_MAX_PORT_DESCRIPTION_SIZE > 256
#error "Port description can't be larger than 255 bytes plus termination"
#endif
#if PNET_MAX_PORT_DESCRIPTION_SIZE < PNET_INTERFACE_NAME_MAX_SIZE
#error "Port description should be at least as large as interface name"
#endif

#define LLDP_TYPE_END                0
#define LLDP_TYPE_CHASSIS_ID         1
#define LLDP_TYPE_PORT_ID            2
#define LLDP_TYPE_TTL                3
#define LLDP_TYPE_PORT_DESCRIPTION   4
#define LLDP_TYPE_MANAGEMENT_ADDRESS 8
#define LLDP_TYPE_ORG_SPEC           127

#define LLDP_IEEE_SUBTYPE_MAC_PHY 1

#define LLDP_TYPE_MASK   0xFE00
#define LLDP_TYPE_SHIFT  0x9
#define LLDP_LENGTH_MASK 0x1FF
#define LLDP_MAX_TLV     512

#define LLDP_PEER_MAX_TTL_IN_SECS                                              \
   60 /* Profinet peers should have TTL = 20 seconds. Use some margin. */

#define LLDP_PNIO_SUBTYPE_MEAS_DELAY_VALUES_TLV_LEN 24
#define LLDP_PNIO_SUBTYPE_PORT_STATUS_TLV_LEN       8
#define LLDP_PNIO_SUBTYPE_CHASSIS_MAC_TLV_LEN       10
#define LLDP_IEEE_SUBTYPE_MAC_PHY_TLV_LEN           9

/* Autonegotiation status */
#define LLDP_AUTONEG_SUPPORTED BIT (0)
#define LLDP_AUTONEG_ENABLED   BIT (1)

typedef struct lldp_tlv
{
   uint8_t type;
   uint16_t len;
   const uint8_t * p_data;
} lldp_tlv_t;

/** Header for Organizationally Specific TLV */
typedef struct pf_lldp_org_header
{
   /** Organizationally unique identifier */
   uint8_t id[3];

   /** Organizationally defined subtype */
   uint8_t subtype;
} pf_lldp_org_header_t;

static const char org_id_pnio[] = {0x00, 0x0e, 0xcf};
static const char org_id_ieee_8023[] = {0x00, 0x12, 0x0f};

typedef enum lldp_pnio_subtype_values
{
   LLDP_PNIO_SUBTYPE_RESERVED = 0,
   LLDP_PNIO_SUBTYPE_MEAS_DELAY_VALUES = 1,
   LLDP_PNIO_SUBTYPE_PORT_STATUS = 2,
   LLDP_PNIO_SUBTYPE_PORT_ALIAS = 3,
   LLDP_PNIO_SUBTYPE_MRP_RING_PORT_STATUS = 4,
   LLDP_PNIO_SUBTYPE_CHASSIS_MAC = 5,
   LLDP_PNIO_SUBTYPE_PTCP_STATUS = 6,
   LLDP_PNIO_SUBTYPE_MAU_TYPE_EXTENSION = 7,
   LLDP_PNIO_SUBTYPE_MRP_INTERCONNECTION_PORT_STATUS = 8,
   /* 0x09..0xff reserved */
} lldp_pnio_subtype_values_t;

static const pnet_ethaddr_t lldp_dst_addr = {
   {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e} /* LLDP Multicast */
};

/******************* Insert data into buffer ********************************/

/**
 * @internal
 * Insert header of a TLV field into a buffer.
 * Each TLV is structured as follows:
 * - Type   = 7 bits
 * - Length = 9 bits
 * - data   = 0-511 bytes
 *
 * This is for the type and the payload length.
 *
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The buffer position.
 * @param typ              In:    The TLV header type.
 * @param len              In:    The TLV payload length.
 */
static inline void pf_lldp_add_tlv_header (
   uint8_t * p_buf,
   uint16_t * p_pos,
   uint8_t typ,
   uint8_t len)
{
   uint16_t tlv_header = ((typ) << LLDP_TYPE_SHIFT) + ((len)&LLDP_LENGTH_MASK);
   pf_put_uint16 (true, tlv_header, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
}

/**
 * @internal
 * Parse LLDP TLV header.
 * Each TLV is structured as follows:
 * - Type   = 7 bits
 * - Length = 9 bits
 * - data   = 0-511 bytes
 *
 * @param p_info           InOut: Parser state
 * @param p_pos            InOut: Offset in parsed buffer
 * @return parsed frame. If parsing fails, frame type is set to LLDP_TYPE_END
 *                       and len field set to 0.
 */
lldp_tlv_t pf_lldp_get_tlv_from_packet (pf_get_info_t * p_info, uint16_t * p_pos)
{
   lldp_tlv_t tlv;
   uint16_t tlv_head = pf_get_uint16 (p_info, p_pos);

   if (p_info->result == PF_PARSE_OK)
   {
      tlv.type = (tlv_head & LLDP_TYPE_MASK) >> LLDP_TYPE_SHIFT;
      tlv.len = (tlv_head & LLDP_LENGTH_MASK);
      tlv.p_data = &p_info->p_buf[*p_pos];
   }
   else
   {
      tlv.type = LLDP_TYPE_END;
      tlv.len = 0;
      tlv.p_data = NULL;
   }

   return tlv;
}

/**
 * Insert Ethernet header into a buffer.
 *
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The buffer position.
 * @param dst_addr         In:    Destination MAC address.
 * @param src_addr         In:    Source MAC address.
 */
static void pf_lldp_add_ethernet_header (
   uint8_t * p_buf,
   uint16_t * p_pos,
   pnet_ethaddr_t dst_addr,
   pnet_ethaddr_t src_addr)
{
   pf_put_mem (
      dst_addr.addr,
      sizeof (dst_addr.addr),
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);

   pf_put_mem (
      src_addr.addr,
      sizeof (src_addr.addr),
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);

   pf_put_uint16 (true, PNAL_ETHTYPE_LLDP, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
}

/**
 * @internal
 * Insert a Profinet-specific header for a TLV field into a buffer.
 *
 * This inserts a TLV header with type="organisation-specific", and
 * the Profinet organisation identifier as the first part of the TLV payload.
 *
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The buffer position.
 * @param len              In:    The TLV payload length (for the part after the
 *                                organisation identifier)
 */
static inline void pf_lldp_add_pnio_header (
   uint8_t * p_buf,
   uint16_t * p_pos,
   uint8_t len)
{
   pf_lldp_add_tlv_header (p_buf, p_pos, LLDP_TYPE_ORG_SPEC, (len) + 3);
   pf_put_byte (0x00, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_byte (0x0e, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_byte (0xcf, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
}

/**
 * @internal
 * Insert a IEEE 802.3-specific header for a TLV field into a buffer.
 *
 * This inserts a TLV header with type="organisation-specific", and
 * the IEEE 802.3 organisation identifier as the first part of the TLV payload.
 *
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The buffer position.
 * @param len              In:    The TLV payload length (for the part after the
 *                                organisation identifier)
 */
static inline void pf_lldp_add_ieee_header (
   uint8_t * p_buf,
   uint16_t * p_pos,
   uint8_t len)
{
   pf_lldp_add_tlv_header (p_buf, p_pos, LLDP_TYPE_ORG_SPEC, (len) + 3);
   pf_put_byte (0x00, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_byte (0x12, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_byte (0x0f, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
}

/**
 * @internal
 * Insert the mandatory chassis_id TLV into a buffer.
 *
 * @param p_chassis_id     In:    The Chassis ID.
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The position in the buffer.
 */
static void pf_lldp_add_chassis_id_tlv (
   const pf_lldp_chassis_id_t * p_chassis_id,
   uint8_t * p_buf,
   uint16_t * p_pos)
{
   pf_lldp_add_tlv_header (
      p_buf,
      p_pos,
      LLDP_TYPE_CHASSIS_ID,
      1 + p_chassis_id->len);
   pf_put_byte (p_chassis_id->subtype, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_mem (
      p_chassis_id->string,
      p_chassis_id->len,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
}

/**
 * @internal
 * Insert the mandatory port_id TLV into a buffer.
 * @param p_port_id        In:    Port Id
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The position in the buffer.
 */
static void pf_lldp_add_port_id_tlv (
   const pf_lldp_port_id_t * p_port_id,
   uint8_t * p_buf,
   uint16_t * p_pos)
{
   pf_lldp_add_tlv_header (p_buf, p_pos, LLDP_TYPE_PORT_ID, 1 + p_port_id->len);
   pf_put_byte (p_port_id->subtype, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_mem (
      p_port_id->string,
      p_port_id->len,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
}

/**
 * @internal
 * Insert the mandatory time-to-live (TTL) TLV into a buffer.
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The position in the buffer.
 */
static void pf_lldp_add_ttl_tlv (uint8_t * p_buf, uint16_t * p_pos)
{
   pf_lldp_add_tlv_header (p_buf, p_pos, LLDP_TYPE_TTL, 2);
   pf_put_uint16 (true, PF_LLDP_TTL, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
}

/**
 * @internal
 * Insert the optional Profinet port status TLV into a buffer.
 *
 * The port status TLV is mandatory for ProfiNet.
 * @param rtclass_2_status  In:    Use 0. Not supported.
 * @param rtclass_3_status  In:    Use 0. Not supported.
 * @param p_buf             InOut: The buffer.
 * @param p_pos             InOut: The position in the buffer.
 */
static void pf_lldp_add_port_status (
   uint16_t rtclass_2_status,
   uint16_t rtclass_3_status,
   uint8_t * p_buf,
   uint16_t * p_pos)
{
   pf_lldp_add_pnio_header (p_buf, p_pos, 5);

   pf_put_byte (
      LLDP_PNIO_SUBTYPE_PORT_STATUS,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
   pf_put_uint16 (true, rtclass_2_status, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_uint16 (true, rtclass_3_status, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
}

/**
 * @internal
 * Insert the optional Profinet chassis MAC TLV into a buffer.
 *
 * The chassis MAC TLV is mandatory for ProfiNet.
 * @param mac_address      In:    Device MAC address.
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The position in the buffer.
 */
static void pf_lldp_add_chassis_mac (
   const pnet_ethaddr_t * mac_address,
   uint8_t * p_buf,
   uint16_t * p_pos)
{
   pf_lldp_add_pnio_header (p_buf, p_pos, 1 + sizeof (pnet_ethaddr_t));

   pf_put_byte (
      LLDP_PNIO_SUBTYPE_CHASSIS_MAC,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
   memcpy (&p_buf[*p_pos], mac_address->addr, sizeof (pnet_ethaddr_t));
   (*p_pos) += sizeof (pnet_ethaddr_t);
}

/**
 * @internal
 * Insert the optional IEEE 802.3 MAC TLV into a buffer.
 *
 * This is the autonegotiation capabilities and available speeds, and cable MAU
 * type.
 *
 * The IEEE 802.3 MAC TLV is mandatory for ProfiNet on 803.2 interfaces.
 * @param p_status         In:    Link status for this port.
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The position in the buffer.
 */
static void pf_lldp_add_ieee_mac_phy (
   const pf_lldp_link_status_t * p_status,
   uint8_t * p_buf,
   uint16_t * p_pos)
{
   uint8_t autoneg_status;

   autoneg_status = 0x00;
   if (p_status->is_autonegotiation_supported)
   {
      autoneg_status |= LLDP_AUTONEG_SUPPORTED;
   }
   if (p_status->is_autonegotiation_enabled)
   {
      autoneg_status |= LLDP_AUTONEG_ENABLED;
   }

   pf_lldp_add_ieee_header (p_buf, p_pos, 6);

   pf_put_byte (LLDP_IEEE_SUBTYPE_MAC_PHY, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_byte (autoneg_status, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_uint16 (
      true,
      p_status->autonegotiation_advertised_capabilities,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
   pf_put_uint16 (
      true,
      p_status->operational_mau_type,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
}

/**
 * Insert the optional management data TLV into a buffer.
 * It is mandatory for ProfiNet.
 *
 * Contains the IP address as well as information about the
 * management interface.
 *
 * @param p_man_address    In:    Management address
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The position in the buffer.
 */
static void pf_lldp_add_management (
   const pf_lldp_management_address_t * p_man_address,
   uint8_t * p_buf,
   uint16_t * p_pos)
{
   pf_lldp_add_tlv_header (p_buf, p_pos, LLDP_TYPE_MANAGEMENT_ADDRESS, 12);

   /* Management address length, type and value */
   pf_put_byte (1 + p_man_address->len, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_byte (p_man_address->subtype, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_mem (
      p_man_address->value,
      p_man_address->len,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);

   /* Interface number for management interface */
   pf_put_byte (
      p_man_address->interface_number.subtype,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
   pf_put_uint32 (
      true,
      p_man_address->interface_number.value,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);

   /* Management OID. Not supported */
   pf_put_byte (0, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
}

void pf_lldp_invalidate_peer_info (pnet_t * net, int loc_port_num)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   os_mutex_lock (net->lldp_mutex);
   p_port_data->lldp.is_peer_info_received = false;
   p_port_data->lldp.peer_info.chassis_id.is_valid = false;
   p_port_data->lldp.peer_info.port_id.is_valid = false;
   p_port_data->lldp.peer_info.port_description.is_valid = false;
   p_port_data->lldp.peer_info.management_address.is_valid = false;
   p_port_data->lldp.peer_info.port_delay.is_valid = false;
   p_port_data->lldp.peer_info.phy_config.is_valid = false;
   p_port_data->lldp.peer_info.ttl = 0;
   os_mutex_unlock (net->lldp_mutex);
}

/********************* Initialize and send **********************************/

/**
 * @internal
 * Handle LLDP peer timeout. Timeout occurs if no peer LLDP data is received
 * on port within the LLPD TTL timeout interval.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    Port instance, p_port_data
 * @param current_time     In:    Not used
 */
static void pf_lldp_receive_timeout (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{
   pf_port_t * p_port_data = (pf_port_t *)arg;

   pf_scheduler_reset_handle (&p_port_data->lldp.rx_timeout);
   LOG_INFO (
      PF_LLDP_LOG,
      "LLDP(%d): Port %d, receive timeout expired\n",
      __LINE__,
      p_port_data->port_num);

   pf_lldp_invalidate_peer_info (net, p_port_data->port_num);
   pf_pdport_peer_indication (net, p_port_data->port_num);
}

void pf_lldp_restart_peer_timeout (
   pnet_t * net,
   int loc_port_num,
   uint16_t timeout_in_secs)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   pf_scheduler_remove_if_running (net, &p_port_data->lldp.rx_timeout);

   /*
    *  Profinet states that the time to live shall be 20 seconds,
    *  See IEC61158-6-10 6.3.13.2.2
    *
    *  To handle situations where ttl is longer than 20s and timeout gets
    *  longer than scheduler can handle, only start receive timeout if
    *  ttl is within configured max ttl.
    *  Timeout will always be started if a profinet enabled switch
    *  with 20s LLDP peer ttl is used.
    */
   if (timeout_in_secs > LLDP_PEER_MAX_TTL_IN_SECS)
   {
      return;
   }

   if (
      pf_scheduler_add (
         net,
         timeout_in_secs * 1000000,
         pf_lldp_receive_timeout,
         p_port_data,
         &p_port_data->lldp.rx_timeout) != 0)
   {
      LOG_ERROR (
         PF_LLDP_LOG,
         "LLDP(%d): Failed to add LLDP timeout\n",
         __LINE__);
   }
}

void pf_lldp_stop_peer_timeout (pnet_t * net, int loc_port_num)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   pf_scheduler_remove_if_running (net, &p_port_data->lldp.rx_timeout);
}

/**
 * @internal
 * Check if management address is of type IPv4
 *
 * @param address          In:    Management address.
 * @return true if address is a valid IPv4 address,
 *         false if not.
 */
static bool pf_lldp_management_address_is_ipv4 (
   const pf_lldp_management_address_t * address)
{
   return address->is_valid && address->subtype == 1 && address->len == 4;
}

int pf_lldp_get_peer_timestamp (
   pnet_t * net,
   int loc_port_num,
   uint32_t * timestamp_10ms)
{
   bool is_received;
   const pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   os_mutex_lock (net->lldp_mutex);
   *timestamp_10ms = p_port_data->lldp.timestamp_for_last_peer_change;
   is_received = p_port_data->lldp.is_peer_info_received;
   os_mutex_unlock (net->lldp_mutex);

   return is_received ? 0 : -1;
}

void pf_lldp_get_chassis_id (pnet_t * net, pf_lldp_chassis_id_t * p_chassis_id)
{
   const pnet_ethaddr_t * device_mac_address =
      pf_cmina_get_device_macaddr (net);
   char station_name[PNET_STATION_NAME_MAX_SIZE]; /** Terminated */

   if (
      net->pf_interface.name_of_device_mode.mode ==
      PF_LLDP_NAME_OF_DEVICE_MODE_LEGACY)
   {
      /*
       * FIXME: Use of pf_cmina_get_station_name() is not thread-safe.
       * Fix this, e.g. using a mutex.
       */
      pf_cmina_get_station_name (net, station_name);
      p_chassis_id->len = strlen (station_name);

      if (
         p_chassis_id->len == 0 ||
         p_chassis_id->len >= PNET_LLDP_CHASSIS_ID_MAX_SIZE)
      {
         /* Use the device MAC address */
         p_chassis_id->subtype = PF_LLDP_SUBTYPE_MAC;
         p_chassis_id->len = sizeof (pnet_ethaddr_t);
         memcpy (
            p_chassis_id->string,
            device_mac_address->addr,
            sizeof (pnet_ethaddr_t));
      }
      else
      {
         /* Use the station name (NameOfStation/NameOfInterface) */
         p_chassis_id->subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
         memcpy (p_chassis_id->string, station_name, p_chassis_id->len);
      }
   }
   else /* PF_LLDP_NAME_OF_DEVICE_MODE_STANDARD */
   {
      p_chassis_id->subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
      p_chassis_id->len = pf_lldp_get_system_description (
         net,
         p_chassis_id->string,
         sizeof (p_chassis_id->string));
   }
   p_chassis_id->string[p_chassis_id->len] = '\0';
   p_chassis_id->is_valid = true;
}

int pf_lldp_get_peer_chassis_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_chassis_id_t * p_chassis_id)
{
   const pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   os_mutex_lock (net->lldp_mutex);
   *p_chassis_id = p_port_data->lldp.peer_info.chassis_id;
   os_mutex_unlock (net->lldp_mutex);

   return p_chassis_id->is_valid ? 0 : -1;
}

void pf_lldp_get_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id)
{
   const pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   char station_name[PNET_STATION_NAME_MAX_SIZE]; /** Terminated */
   const pnet_ethaddr_t * device_mac_address =
      pf_cmina_get_device_macaddr (net);

   if (
      net->pf_interface.name_of_device_mode.mode ==
      PF_LLDP_NAME_OF_DEVICE_MODE_LEGACY)
   {
      snprintf (
         p_port_id->string,
         sizeof (p_port_id->string),
         "%s",
         p_port_data->port_name);
   }
   else /* PF_LLDP_NAME_OF_DEVICE_MODE_STANDARD */
   {
      pf_cmina_get_station_name (net, station_name);
      if (strlen (station_name) == 0)
      {
         /* Profinet 2.4 Protocol, table 335 "LLDP substitutions":
            Use chassis MAC address if no NameOfStation is assigned */
         pf_lldp_mac_address_to_string (
            device_mac_address->addr,
            station_name,
            sizeof (station_name));
      }

      snprintf (
         p_port_id->string,
         sizeof (p_port_id->string),
         "%s.%s",
         p_port_data->port_name,
         station_name);
   }

   p_port_id->subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
   p_port_id->len = strlen (p_port_id->string);
   p_port_id->is_valid = true;
}

int pf_lldp_get_peer_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id)
{
   const pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   os_mutex_lock (net->lldp_mutex);
   *p_port_id = p_port_data->lldp.peer_info.port_id;
   os_mutex_unlock (net->lldp_mutex);

   return p_port_id->is_valid ? 0 : -1;
}

void pf_lldp_get_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_descr)
{
   const pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   snprintf (
      p_port_descr->string,
      sizeof (p_port_descr->string),
      "%s",
      p_port_data->netif.name);
   p_port_descr->len = strlen (p_port_descr->string);
   p_port_descr->is_valid = true;
}

int pf_lldp_get_peer_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_desc)
{
   const pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   os_mutex_lock (net->lldp_mutex);
   *p_port_desc = p_port_data->lldp.peer_info.port_description;
   os_mutex_unlock (net->lldp_mutex);

   return p_port_desc->is_valid ? 0 : -1;
}

void pf_lldp_get_management_address (
   pnet_t * net,
   pf_lldp_management_address_t * p_man_address)
{
   pnal_ipaddr_t ipaddr = pf_cmina_get_ipaddr (net);

   p_man_address->subtype = 1; /* IPv4 */
   ipaddr = CC_TO_BE32 (ipaddr);
   memcpy (p_man_address->value, &ipaddr, sizeof (ipaddr));
   p_man_address->len = sizeof (ipaddr);

   p_man_address->interface_number.subtype = 2; /* ifIndex */
   p_man_address->interface_number.value =
      pnal_get_interface_index (net->pf_interface.main_port.name);

   p_man_address->is_valid = true;
}

int pf_lldp_get_peer_management_address (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_management_address_t * p_man_address)
{
   const pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   os_mutex_lock (net->lldp_mutex);
   *p_man_address = p_port_data->lldp.peer_info.management_address;
   os_mutex_unlock (net->lldp_mutex);

   return p_man_address->is_valid ? 0 : -1;
}

int pf_lldp_get_peer_station_name (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_station_name_t * p_station_name)
{
   pf_lldp_chassis_id_t chassis_id;
   pf_lldp_port_id_t port_id;
   char * p;
   bool found = false;

   const pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   memset (p_station_name, 0, sizeof (*p_station_name));

   os_mutex_lock (net->lldp_mutex);
   port_id = p_port_data->lldp.peer_info.port_id;
   chassis_id = p_port_data->lldp.peer_info.chassis_id;
   os_mutex_unlock (net->lldp_mutex);

   if (port_id.is_valid)
   {
      p = strchr (port_id.string, '.');
      if (p != NULL)
      {
         /* PF_LLDP_NAME_OF_DEVICE_MODE_STANDARD
          * Example: port-001.dut
          */
         snprintf (
            p_station_name->string,
            sizeof (p_station_name->string),
            "%s",
            p + 1);
         found = true;
      }
   }

   if (!found && chassis_id.is_valid)
   {
      /* PF_LLDP_NAME_OF_DEVICE_MODE_LEGACY */
      if (chassis_id.subtype != PF_LLDP_SUBTYPE_MAC)
      {
         snprintf (
            p_station_name->string,
            sizeof (p_station_name->string),
            "%s",
            chassis_id.string);
      }
      else
      {
         pf_lldp_mac_address_to_string (
            (uint8_t *)chassis_id.string,
            p_station_name->string,
            sizeof (p_station_name->string));
      }
   }

   /* Do not use peer mac address here since its status is unknown. */

   p_station_name->len = strlen (p_station_name->string);

   return p_station_name->len ? 0 : -1;
}

int pf_lldp_get_peer_port_name (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_name_t * p_port_name)
{
   pf_lldp_port_id_t port_id;
   size_t i;
   const pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   memset (p_port_name, 0, sizeof (*p_port_name));

   os_mutex_lock (net->lldp_mutex);
   port_id = p_port_data->lldp.peer_info.port_id;
   os_mutex_unlock (net->lldp_mutex);

   if (!port_id.is_valid)
   {
      return -1;
   }

   for (i = 0; (i < port_id.len) && (port_id.string[i] != '\0') &&
               (i < (sizeof (p_port_name->string) - 1));
        i++)
   {
      if (port_id.string[i] == '.')
      {
         /* PF_LLDP_NAME_OF_DEVICE_MODE_STANDARD */
         break;
      }
      p_port_name->string[i] = port_id.string[i];
   }

   p_port_name->len = strlen (p_port_name->string);

   return p_port_name->len ? 0 : -1;
}

void pf_lldp_get_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_signal_delay_t * p_delays)
{
   /* Line delay measurement is not supported */
   p_delays->cable_delay_local = 0; /* Not measured */
   p_delays->rx_delay_local = 0;    /* Not measured */
   p_delays->tx_delay_local = 0;    /* Not measured */
   p_delays->rx_delay_remote = 0;   /* Not measured */
   p_delays->tx_delay_remote = 0;   /* Not measured */
   p_delays->is_valid = true;
}

int pf_lldp_get_peer_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_signal_delay_t * p_delays)
{
   const pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   os_mutex_lock (net->lldp_mutex);
   *p_delays = p_port_data->lldp.peer_info.port_delay;
   os_mutex_unlock (net->lldp_mutex);

   return p_delays->is_valid ? 0 : -1;
}

void pf_lldp_get_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status)
{
   pnal_eth_status_t status =
      pf_pdport_get_eth_status_filtered_mau (net, loc_port_num);

   p_link_status->is_autonegotiation_supported =
      status.is_autonegotiation_supported;
   p_link_status->is_autonegotiation_enabled =
      status.is_autonegotiation_enabled;
   p_link_status->autonegotiation_advertised_capabilities =
      status.autonegotiation_advertised_capabilities;
   p_link_status->operational_mau_type = status.operational_mau_type;
   p_link_status->is_valid = true;
}

int pf_lldp_get_peer_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status)
{
   const pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   os_mutex_lock (net->lldp_mutex);
   *p_link_status = p_port_data->lldp.peer_info.phy_config;
   os_mutex_unlock (net->lldp_mutex);

   return p_link_status->is_valid ? 0 : -1;
}

size_t pf_lldp_get_system_description (
   const pnet_t * net,
   char * system_description,
   size_t system_description_max_len)
{
   return snprintf (
      system_description,
      system_description_max_len,
      /* clang-format off */
      "%-" STRINGIFY (PNET_PRODUCT_NAME_MAX_LEN) "s "
      "%-" STRINGIFY (PNET_ORDER_ID_MAX_LEN) "s "
      "%-" STRINGIFY (PNET_SERIAL_NUMBER_MAX_LEN) "s "
      "%5u "
      "%c%3u%3u%3u",
      /* clang-format on */
      net->fspm_cfg.product_name,
      net->fspm_cfg.im_0_data.im_order_id,
      net->fspm_cfg.im_0_data.im_serial_number,
      net->fspm_cfg.im_0_data.im_hardware_revision,
      net->fspm_cfg.im_0_data.im_sw_revision_prefix,
      net->fspm_cfg.im_0_data.im_sw_revision_functional_enhancement,
      net->fspm_cfg.im_0_data.im_sw_revision_bug_fix,
      net->fspm_cfg.im_0_data.im_sw_revision_internal_change);
}

void pf_lldp_mac_address_to_string (
   const uint8_t * mac,
   char * mac_str,
   size_t mac_str_max_len)
{
   snprintf (
      mac_str,
      mac_str_max_len,
      "%02X-%02X-%02X-%02X-%02X-%02X",
      mac[0],
      mac[1],
      mac[2],
      mac[3],
      mac[4],
      mac[5]);
}

/**
 * Construct Ethernet frame containing LLDP PDU as payload
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @param buf              Out:   Ethernet frame buffer of size
 *                                PF_FRAME_BUFFER_SIZE bytes.
 *
 * @return Size of constructed frame, in bytes.
 */
size_t pf_lldp_construct_frame (pnet_t * net, int loc_port_num, uint8_t buf[])
{
   uint16_t pos;
   const pnet_ethaddr_t * device_mac_address =
      pf_cmina_get_device_macaddr (net);
   pf_lldp_link_status_t link_status;
   pf_lldp_chassis_id_t chassis_id;
   pf_lldp_port_id_t port_id;
   pf_lldp_management_address_t man_address;
   const pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

#if LOG_DEBUG_ENABLED(PF_LLDP_LOG)
   pnal_ipaddr_t ipaddr = pf_cmina_get_ipaddr (net);
   char ip_string[PNAL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   const char * chassis_id_description = "<MAC address>";
#endif

   pf_lldp_get_chassis_id (net, &chassis_id);
   pf_lldp_get_port_id (net, loc_port_num, &port_id);
   pf_lldp_get_link_status (net, loc_port_num, &link_status);
   pf_lldp_get_management_address (net, &man_address);
#if LOG_DEBUG_ENABLED(PF_LLDP_LOG)
   pf_cmina_ip_to_string (ipaddr, ip_string);
   if (chassis_id.subtype == PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED)
   {
      chassis_id_description = chassis_id.string;
   }
   LOG_DEBUG (
      PF_LLDP_LOG,
      "LLDP(%d): Send port %u MAC: "
      "%02X:%02X:%02X:%02X:%02X:%02X "
      "IP: %s Chassis ID: \"%s\" Port ID: \"%s\"\n",
      __LINE__,
      loc_port_num,
      p_port_data->netif.mac_address.addr[0],
      p_port_data->netif.mac_address.addr[1],
      p_port_data->netif.mac_address.addr[2],
      p_port_data->netif.mac_address.addr[3],
      p_port_data->netif.mac_address.addr[4],
      p_port_data->netif.mac_address.addr[5],
      ip_string,
      chassis_id_description,
      port_id.string);
#endif

   pos = 0;

   pf_lldp_add_ethernet_header (
      buf,
      &pos,
      lldp_dst_addr,
      p_port_data->netif.mac_address);

   /* Add mandatory parts */
   pf_lldp_add_chassis_id_tlv (&chassis_id, buf, &pos);
   pf_lldp_add_port_id_tlv (&port_id, buf, &pos);
   pf_lldp_add_ttl_tlv (buf, &pos);

   /* Add optional parts */
   pf_lldp_add_port_status (0, 0, buf, &pos);
   pf_lldp_add_chassis_mac (device_mac_address, buf, &pos);
   pf_lldp_add_ieee_mac_phy (&link_status, buf, &pos);
   pf_lldp_add_management (&man_address, buf, &pos);

   /* Add end of LLDP-PDU marker */
   pf_lldp_add_tlv_header (buf, &pos, LLDP_TYPE_END, 0);

   return pos;
}

/**
 * Build and send a LLDP message on a specific port.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
static void pf_lldp_send (pnet_t * net, int loc_port_num)
{
   /* FIXME: Buffer size should include Ethernet header (14 bytes) */
   pnal_buf_t * p_buffer = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);

   if (p_buffer == NULL)
   {
      return;
   }

   if (p_buffer->payload != NULL)
   {
      p_buffer->len =
         pf_lldp_construct_frame (net, loc_port_num, p_buffer->payload);
      (void)pf_eth_send_on_physical_port (net, loc_port_num, p_buffer);
   }

   pnal_buf_free (p_buffer);
}

/**
 * @internal
 * Trigger sending a LLDP frame.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * Re-schedules itself after 5 s.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    Reference to port_data
 * @param current_time     In:    Not used.
 */
static void pf_lldp_trigger_sending (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{
   pf_port_t * p_port_data = (pf_port_t *)arg;

   pf_lldp_send (net, p_port_data->port_num);

   if (
      pf_scheduler_add (
         net,
         PF_LLDP_SEND_INTERVAL * 1000,
         pf_lldp_trigger_sending,
         p_port_data,
         &p_port_data->lldp.tx_timeout) != 0)
   {
      LOG_ERROR (
         PF_LLDP_LOG,
         "LLDP(%d): Failed to reschedule LLDP sending\n",
         __LINE__);
   }
}

/**
 * @internal
 * Restart LLDP transmission timer and optionally send LLDP frame.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @param send             In:    Send LLDP message
 */
static void pf_lldp_tx_restart (pnet_t * net, int loc_port_num, bool send)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   if (
      pf_scheduler_restart (
         net,
         PF_LLDP_SEND_INTERVAL * 1000,
         pf_lldp_trigger_sending,
         p_port_data,
         &p_port_data->lldp.tx_timeout) != 0)
   {
      LOG_ERROR (
         PF_ETH_LOG,
         "LLDP(%d): Failed to schedule LLDP sending for port %d\n",
         __LINE__,
         loc_port_num);
   }

   if (send)
   {
      pf_lldp_send (net, loc_port_num);
   }
}

void pf_lldp_init (pnet_t * net)
{
   int port;
   pf_port_iterator_t port_iterator;
   pf_port_t * p_port_data = NULL;

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);

      memset (&p_port_data->lldp, 0, sizeof (p_port_data->lldp));
      pf_scheduler_init_handle (&p_port_data->lldp.rx_timeout, "lldp_rx");
      pf_scheduler_init_handle (&p_port_data->lldp.tx_timeout, "lldp_tx");

      port = pf_port_get_next (&port_iterator);
   }

   net->lldp_mutex = os_mutex_create();
   CC_ASSERT (net->lldp_mutex != NULL);
}

void pf_lldp_send_enable (pnet_t * net, int loc_port_num)
{
   LOG_DEBUG (
      PF_LLDP_LOG,
      "LLDP(%d): Enabling LLDP transmission for port %d\n",
      __LINE__,
      loc_port_num);
   pf_lldp_tx_restart (net, loc_port_num, true);
}

void pf_lldp_send_disable (pnet_t * net, int loc_port_num)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   LOG_DEBUG (
      PF_LLDP_LOG,
      "LLDP(%d): Disabling LLDP transmission for port %d\n",
      __LINE__,
      loc_port_num);
   pf_scheduler_remove_if_running (net, &p_port_data->lldp.tx_timeout);
}

/**
 * Get organizationally specific header from received packet
 *
 * Layout of organizationally specific header:
 * 3     Organizationally unique identifier
 * 1     Organizationally defined subtype
 *
 * @param parse_info       InOut: Parser state.
 * @param offset           InOut: Offset in parsed buffer.
 * @param header           Out:   Parsed organizationally specific header.
 */
void pf_lldp_get_org_header_from_packet (
   pf_get_info_t * parse_info,
   uint16_t * offset,
   pf_lldp_org_header_t * header)
{
   pf_get_mem (parse_info, offset, sizeof (header->id), header->id);
   header->subtype = pf_get_byte (parse_info, offset);
}

/**
 * Get Chassis ID from received packet
 *
 * Layout of Chassis ID TLV:
 * 2     TLV header (already processed)
 * 1     Chassis ID subtype
 * 1-255 Chassis ID
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset            InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param chassis_id       Out:   Parsed Chassis ID.
 */
static void pf_lldp_get_chassis_id_from_packet (
   pf_get_info_t * parse_info,
   uint16_t * const offset,
   const size_t tlv_len,
   pf_lldp_chassis_id_t * chassis_id)
{
   size_t max_len = sizeof (chassis_id->string) - 1;

   if (tlv_len == 0)
   {
      LOG_WARNING (
         PF_LLDP_LOG,
         "LLDP(%d): Ignoring Chassis Id, tlv len=%d\n",
         __LINE__,
         (int)tlv_len);

      return;
   }

   chassis_id->len = tlv_len - 1;
   if (chassis_id->len > max_len)
   {
      *offset += tlv_len;
      LOG_ERROR (
         PF_LLDP_LOG,
         "LLDP(%d): Chassis Id too long (%d)\n",
         __LINE__,
         (int)chassis_id->len);
      /* Should not happen. Reconfigure PNET_LLDP_CHASSIS_ID_MAX_LEN
       * if it does*/

      return;
   }

   chassis_id->subtype = pf_get_byte (parse_info, offset);
   pf_get_mem (parse_info, offset, chassis_id->len, chassis_id->string);
   chassis_id->string[chassis_id->len] = '\0';
   chassis_id->is_valid = true;
}

/**
 * Get Port ID from received packet
 *
 * Layout of Port ID TLV:
 * 2     TLV header (already processed)
 * 1     Port ID subtype
 * 1-255 Port ID
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset            InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param port_id          Out:   Parsed Port ID.
 */
static void pf_lldp_get_port_id_from_packet (
   pf_get_info_t * parse_info,
   uint16_t * const offset,
   const size_t tlv_len,
   pf_lldp_port_id_t * port_id)
{
   size_t max_len = sizeof (port_id->string) - 1;

   if (tlv_len == 0)
   {
      LOG_WARNING (
         PF_LLDP_LOG,
         "LLDP(%d): Ignoring Port Id, tlv len=%d\n",
         __LINE__,
         (int)tlv_len);

      return;
   }

   port_id->len = tlv_len - 1;
   if (port_id->len > max_len)
   {
      *offset += tlv_len;
      LOG_ERROR (
         PF_LLDP_LOG,
         "LLDP(%d): Port Id too long (%d)\n",
         __LINE__,
         (int)port_id->len);
      /* Should not happen. Reconfigure PNET_LLDP_PORT_ID_MAX_LEN
       * if it does*/

      return;
   }

   port_id->subtype = pf_get_byte (parse_info, offset);
   pf_get_mem (parse_info, offset, port_id->len, port_id->string);
   port_id->string[port_id->len] = '\0';
   port_id->is_valid = true;
}

/**
 * Get TTL from received packet
 *
 * Layout of Time To Live TLV:
 * 2     TLV header (already processed)
 * 2     Time to live (TTL)
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset            InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param ttl              Out:   Parsed TTL.
 */
static void pf_lldp_get_ttl_from_packet (
   pf_get_info_t * parse_info,
   uint16_t * const offset,
   const size_t tlv_len,
   uint16_t * ttl)
{
   if (tlv_len != 2)
   {
      parse_info->result = PF_PARSE_ERROR;
      LOG_WARNING (
         PF_LLDP_LOG,
         "LLDP(%d): Ignoring TTL, unexpected tlv len %d(2)\n",
         __LINE__,
         (int)tlv_len);

      return;
   }

   *ttl = pf_get_uint16 (parse_info, offset);
}

/**
 * Get port description from received packet
 *
 * Layout of Port description TLV:
 * 2     TLV header (already processed)
 * 0-255 Port description
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset            InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param port_description Out:   Parsed port description.
 */
static void pf_lldp_get_port_description_from_packet (
   pf_get_info_t * parse_info,
   uint16_t * const offset,
   const size_t tlv_len,
   pf_lldp_port_description_t * port_description)
{
   size_t max_len = sizeof (port_description->string) - 1;

   if (tlv_len > max_len)
   {
      *offset += tlv_len;
      LOG_ERROR (
         PF_LLDP_LOG,
         "LLDP(%d): Port Description too long (%d)\n",
         __LINE__,
         (int)tlv_len);

      return;
   }

   port_description->len = tlv_len;
   pf_get_mem (parse_info, offset, tlv_len, port_description->string);
   port_description->string[tlv_len] = '\0';

   port_description->is_valid = true;
}

/**
 * Get management address from received packet
 *
 * Layout of Management address TLV:
 * 2     TLV header (already processed)
 * 1     Management address string length
 * 1     Management address subtype
 * 1-31  Management address
 * 1     Interface numbering subtype
 * 4     Interface number
 * 1     OID string length
 * 0-128 Object identifier
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset            InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param management_address Out: Parsed management address.
 */
static void pf_lldp_get_management_address_from_packet (
   pf_get_info_t * parse_info,
   uint16_t * const offset,
   const size_t tlv_len,
   pf_lldp_management_address_t * management_address)
{
   size_t oid_len;

   memset (management_address, 0, sizeof (*management_address));

   management_address->len = pf_get_byte (parse_info, offset) - 1;
   if (management_address->len < 1 || management_address->len > 31)
   {
      *offset += tlv_len - 1;
      LOG_ERROR (
         PF_LLDP_LOG,
         "LLDP(%d): Invalid management address length (%d)\n",
         __LINE__,
         (int)management_address->len);

      return;
   }

   management_address->subtype = pf_get_byte (parse_info, offset);
   pf_get_mem (
      parse_info,
      offset,
      management_address->len,
      management_address->value);

   management_address->interface_number.subtype =
      pf_get_byte (parse_info, offset);
   management_address->interface_number.value =
      pf_get_uint32 (parse_info, offset);

   /* We don't save the Object ID */
   oid_len = pf_get_byte (parse_info, offset);
   *offset += oid_len;

   management_address->is_valid = true;
}

/**
 * Get signal delay values from received packet
 *
 * Layout of LLDP_PNIO_DELAY TLV:
 * 2     TLV header (already processed)
 * 3     LLDP_PNIO_Header (already processed)
 * 1     LLDP_PNIO_SubType (already processed)
 * 4     PTCP_PortRxDelayLocal
 * 4     PTCP_PortRxDelayRemote
 * 4     PTCP_PortTxDelayLocal
 * 4     PTCP_PortTxDelayRemote
 * 4     CableDelayLocal
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset            InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param delay            Out:   Parsed delay values.
 */
static void pf_lldp_get_signal_delay_from_packet (
   pf_get_info_t * parse_info,
   uint16_t * const offset,
   const size_t tlv_len,
   pf_lldp_signal_delay_t * delay)
{
   if (tlv_len != LLDP_PNIO_SUBTYPE_MEAS_DELAY_VALUES_TLV_LEN)
   {
      parse_info->result = PF_PARSE_ERROR;

      return;
   }

   delay->rx_delay_local = pf_get_uint32 (parse_info, offset);
   delay->rx_delay_remote = pf_get_uint32 (parse_info, offset);
   delay->tx_delay_local = pf_get_uint32 (parse_info, offset);
   delay->tx_delay_remote = pf_get_uint32 (parse_info, offset);
   delay->cable_delay_local = pf_get_uint32 (parse_info, offset);
   delay->is_valid = true;
}

/**
 * Get port status from received packet
 *
 * Layout of LLDP_PNIO_PORTSTATUS TLV:
 * 2     TLV header (already processed)
 * 3     LLDP_PNIO_Header (already processed)
 * 1     LLDP_PNIO_SubType (already processed)
 * 2     RTClass2_PortStatus
 * 2     RTClass3_PortStatus
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset            InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param status           Out:   Parsed port status.
 */
static void pf_lldp_get_port_status_from_packet (
   pf_get_info_t * parse_info,
   uint16_t * const offset,
   const size_t tlv_len,
   uint8_t status[4])
{
   if (tlv_len != LLDP_PNIO_SUBTYPE_PORT_STATUS_TLV_LEN)
   {
      parse_info->result = PF_PARSE_ERROR;

      return;
   }

   /* FIXME: Should interpret as two 16-bit integers */
   pf_get_mem (parse_info, offset, 4, status);
}

/**
 * Get Chassis MAC address from received packet
 *
 * Layout of LLDP_PNIO_CHASSIS_MAC TLV:
 * 2     TLV header (already processed)
 * 3     LLDP_PNIO_Header (already processed)
 * 1     LLDP_PNIO_SubType (already processed)
 * 6     Chassis MAC address
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset            InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param chassis_mac      Out:   Parsed Chassis MAC address.
 */
static void pf_lldp_get_chassis_mac_from_packet (
   pf_get_info_t * parse_info,
   uint16_t * const offset,
   const size_t tlv_len,
   pnet_ethaddr_t * chassis_mac)
{
   if (tlv_len != LLDP_PNIO_SUBTYPE_CHASSIS_MAC_TLV_LEN)
   {
      parse_info->result = PF_PARSE_ERROR;

      return;
   }

   pf_get_mem (
      parse_info,
      offset,
      sizeof (chassis_mac->addr),
      chassis_mac->addr);
}

/**
 * Get link status from received packet
 *
 * Layout of MAC/PHY Configuration/Status TLV:
 * 2     TLV header (already processed)
 * 4     802.3 header (already processed)
 * 1     Auto-negotiation support/status:
 *       Bit 0: Auto-negotiation support
 *       Bit 1: Auto-negotiation status
 * 2     PMD auto-negotiation advertised capability
 * 2     Operational MAU typ
 *
 * @param parse_info       InOut: Parsing information.
 * @param offset            InOut: Byte offset in buffer.
 * @param tlv_len          In:    Length of TLV in bytes.
 * @param link_status      Out:   Parsed link status.
 */
static void pf_lldp_get_link_status_from_packet (
   pf_get_info_t * parse_info,
   uint16_t * const offset,
   const size_t tlv_len,
   pf_lldp_link_status_t * link_status)
{
   uint8_t aneg_support_status;

   memset (link_status, 0, sizeof (*link_status));

   if (tlv_len != LLDP_IEEE_SUBTYPE_MAC_PHY_TLV_LEN)
   {
      parse_info->result = PF_PARSE_ERROR;

      return;
   }

   aneg_support_status = pf_get_byte (parse_info, offset);
   link_status->is_autonegotiation_supported = aneg_support_status &
                                               LLDP_AUTONEG_SUPPORTED;
   link_status->is_autonegotiation_enabled = aneg_support_status &
                                             LLDP_AUTONEG_ENABLED;

   link_status->autonegotiation_advertised_capabilities =
      pf_get_uint16 (parse_info, offset);

   link_status->operational_mau_type = pf_get_uint16 (parse_info, offset);

   link_status->is_valid = true;
}

/**
 * @internal
 * Parse LLDP frame buffer
 *
 * @param buf              In:    LLDP data.
 * @param len              In:    Length of LLDP data.
 * @param lldp_peer_info   Out:   Record holding parsed LLDP information.
 *                                Nulled initially (also on failure).
 * @return 0 if parsing is successful, -1 on error.
 */
int pf_lldp_parse_packet (
   const uint8_t buf[],
   uint16_t len,
   pf_lldp_peer_info_t * lldp_peer_info)
{
   lldp_tlv_t tlv;
   pf_lldp_org_header_t org;
   pf_get_info_t parse_info;
   pf_lldp_management_address_t management_address;
   uint16_t offset = 0;

   memset (lldp_peer_info, 0, sizeof (*lldp_peer_info));

   parse_info.result = PF_PARSE_OK;
   parse_info.is_big_endian = true;
   parse_info.len = len;
   parse_info.p_buf = buf;

   tlv = pf_lldp_get_tlv_from_packet (&parse_info, &offset);

   while (tlv.type != LLDP_TYPE_END)
   {
      switch (tlv.type)
      {
      case LLDP_TYPE_CHASSIS_ID:
         pf_lldp_get_chassis_id_from_packet (
            &parse_info,
            &offset,
            tlv.len,
            &lldp_peer_info->chassis_id);
         break;
      case LLDP_TYPE_PORT_ID:
         pf_lldp_get_port_id_from_packet (
            &parse_info,
            &offset,
            tlv.len,
            &lldp_peer_info->port_id);
         break;
      case LLDP_TYPE_TTL:
         pf_lldp_get_ttl_from_packet (
            &parse_info,
            &offset,
            tlv.len,
            &lldp_peer_info->ttl);
         break;
      case LLDP_TYPE_PORT_DESCRIPTION:
         pf_lldp_get_port_description_from_packet (
            &parse_info,
            &offset,
            tlv.len,
            &lldp_peer_info->port_description);
         break;
      case LLDP_TYPE_MANAGEMENT_ADDRESS:
         pf_lldp_get_management_address_from_packet (
            &parse_info,
            &offset,
            tlv.len,
            &management_address);
         if (pf_lldp_management_address_is_ipv4 (&management_address))
         {
            lldp_peer_info->management_address = management_address;
         }
         break;
      case LLDP_TYPE_ORG_SPEC:
      {
         pf_lldp_get_org_header_from_packet (&parse_info, &offset, &org);
         if (memcmp (org_id_pnio, org.id, sizeof (org.id)) == 0)
         {
            switch (org.subtype)
            {
            case LLDP_PNIO_SUBTYPE_MEAS_DELAY_VALUES:
               pf_lldp_get_signal_delay_from_packet (
                  &parse_info,
                  &offset,
                  tlv.len,
                  &lldp_peer_info->port_delay);
               break;
            case LLDP_PNIO_SUBTYPE_PORT_STATUS:
               pf_lldp_get_port_status_from_packet (
                  &parse_info,
                  &offset,
                  tlv.len,
                  lldp_peer_info->port_status);
               break;
            case LLDP_PNIO_SUBTYPE_CHASSIS_MAC:
               pf_lldp_get_chassis_mac_from_packet (
                  &parse_info,
                  &offset,
                  tlv.len,
                  &lldp_peer_info->mac_address);
               break;
            default:
               offset += (tlv.len - 4);
               break;
            }
         }
         else if (memcmp (org_id_ieee_8023, org.id, sizeof (org.id)) == 0)
         {
            switch (org.subtype)
            {
            case LLDP_IEEE_SUBTYPE_MAC_PHY:
               pf_lldp_get_link_status_from_packet (
                  &parse_info,
                  &offset,
                  tlv.len,
                  &lldp_peer_info->phy_config);
               break;
            default:
               offset += (tlv.len - 4);
               break;
            }
         }
         else
         {
            offset += (tlv.len - 4);
         }
      }
      break;
      default:
         offset += tlv.len;
         break;
      }
      tlv = pf_lldp_get_tlv_from_packet (&parse_info, &offset);
   }

   if (parse_info.result != PF_PARSE_OK)
   {
      LOG_ERROR (
         PF_LLDP_LOG,
         "LLDP(%d): Received LLDP frame, but failed to parse it.\n",
         __LINE__);

      return -1;
   }

   return 0;
}

/**
 * @internal
 * Generate an alias name (for me) from the peer port_id and peer chassis_id
 *
 * See PN-Topology 6.4.2 and PN-Protocol 4.3.1.4.18
 *
 * @param port_id          In:    Peer port ID. Terminated string.
 * @param chassis_id       In:    Peer chassis ID. Terminated string.
 * @param alias            Out:   Alias name. Terminated string.
 * @param len              In:    Size of alias name output buffer
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_lldp_generate_alias_name (
   const char * port_id,
   const char * chassis_id,
   char * alias,
   uint16_t len)
{
   int res = 0;

   if (port_id == NULL || chassis_id == NULL || alias == NULL)
   {
      return -1;
   }

   if (strchr (port_id, '.') != NULL)
   {
      /* Assume PN v2.3+ device. PortId include NameOfStation.
       * and the PortId can can used as alias.
       */
      if (strlen (port_id) >= len)
      {
         return -1;
      }
      strncpy (alias, port_id, len);
   }
   else
   {
      /* Assume PN v2.2 device. ChassisId is same as NameOfStation
       * Alias includes PortId and ChassisId
       */
      if ((strlen (port_id) + strlen (chassis_id) + 1) >= len)
      {
         return -1;
      }

      res = snprintf (alias, len, "%s.%s", port_id, chassis_id);
      if ((res < 0) || (res >= (int)len))
      {
         return -1;
      }
   }

   return 0;
}

bool pf_lldp_is_alias_matching (pnet_t * net, const char * alias)
{
   char port_alias[PF_ALIAS_NAME_MAX_SIZE]; /** Terminated */
   int port;
   pf_port_iterator_t port_iterator;
   pf_port_t * p_port_data = NULL;

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);
      if (
         p_port_data->lldp.is_peer_info_received &&
         (pf_lldp_generate_alias_name (
             p_port_data->lldp.peer_info.port_id.string,
             p_port_data->lldp.peer_info.chassis_id.string,
             port_alias,
             sizeof (port_alias)) == 0))
      {
         if (strcmp (alias, port_alias) == 0)
         {
            return true;
         }
      }

      port = pf_port_get_next (&port_iterator);
   }

   return false;
}

/**
 * @internal
 * Store peer information
 *
 * Information is stored atomically (by means of the LLDP mutex).
 * This ensures that other threads (e.g. SNMP) will read consistent data.
 *
 * A timestamp is also generated and stored.
 *
 * @param net              InOut: p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @param new_info         In:    Peer data to be stored
 */
void pf_lldp_store_peer_info (
   pnet_t * net,
   int loc_port_num,
   const pf_lldp_peer_info_t * new_info)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   uint32_t timestamp = pnal_get_system_uptime_10ms();

   os_mutex_lock (net->lldp_mutex);

   p_port_data->lldp.is_peer_info_received = true;
   p_port_data->lldp.timestamp_for_last_peer_change = timestamp;
   p_port_data->lldp.peer_info = *new_info,

   os_mutex_unlock (net->lldp_mutex);
}

/**
 * Apply updated peer information
 *
 * Trigger a diagnosis if the peer information not is as expected.
 * Resets the peer watchdog timer.
 *
 * @param net              InOut: p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @param lldp_peer_info   In:    Peer data to be applied
 */
void pf_lldp_update_peer (
   pnet_t * net,
   int loc_port_num,
   const pf_lldp_peer_info_t * lldp_peer_info)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   pf_lldp_restart_peer_timeout (net, loc_port_num, lldp_peer_info->ttl);

   if (
      memcmp (
         lldp_peer_info,
         &p_port_data->lldp.peer_info,
         sizeof (pf_lldp_peer_info_t)) == 0)
   {
      /* No changes */
      return;
   }

   if (p_port_data->lldp.is_peer_info_received)
   {
      if (
         p_port_data->lldp.peer_info.chassis_id.subtype ==
            PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED &&
         p_port_data->lldp.peer_info.port_id.subtype ==
            PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED)
      {
         LOG_INFO (
            PF_LLDP_LOG,
            "LLDP(%d): Peer info changed on port %d. Old peer info - MAC: "
            "%02X:%02X:%02X:%02X:%02X:%02X "
            "Chassis ID: %s Port ID: %s\n",
            __LINE__,
            loc_port_num,
            p_port_data->lldp.peer_info.mac_address.addr[0],
            p_port_data->lldp.peer_info.mac_address.addr[1],
            p_port_data->lldp.peer_info.mac_address.addr[2],
            p_port_data->lldp.peer_info.mac_address.addr[3],
            p_port_data->lldp.peer_info.mac_address.addr[4],
            p_port_data->lldp.peer_info.mac_address.addr[5],
            p_port_data->lldp.peer_info.chassis_id.string,
            p_port_data->lldp.peer_info.port_id.string);
      }
      else
      {
         LOG_INFO (
            PF_LLDP_LOG,
            "LLDP(%d): Peer info changed on port %d.\n",
            __LINE__,
            loc_port_num);
      }
   }

   if (
      lldp_peer_info->chassis_id.subtype == PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED &&
      lldp_peer_info->port_id.subtype == PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED)
   {
      LOG_INFO (
         PF_LLDP_LOG,
         "LLDP(%d): New peer info on port %d - MAC: "
         "%02X:%02X:%02X:%02X:%02X:%02X "
         "Chassis ID: %s Port ID: %s\n",
         __LINE__,
         loc_port_num,
         lldp_peer_info->mac_address.addr[0],
         lldp_peer_info->mac_address.addr[1],
         lldp_peer_info->mac_address.addr[2],
         lldp_peer_info->mac_address.addr[3],
         lldp_peer_info->mac_address.addr[4],
         lldp_peer_info->mac_address.addr[5],
         lldp_peer_info->chassis_id.string,
         lldp_peer_info->port_id.string);
   }

   pf_lldp_store_peer_info (net, loc_port_num, lldp_peer_info);
   pf_pdport_peer_indication (net, loc_port_num);
}

int pf_lldp_recv (
   pnet_t * net,
   int loc_port_num,
   pnal_buf_t * p_frame_buf,
   uint16_t offset)
{
   uint8_t * buf = p_frame_buf->payload + offset;
   uint16_t buf_len = p_frame_buf->len - offset;
   pf_lldp_peer_info_t peer_data;
   int err = 0;

   if (net->lldp_mutex == NULL) {
      return -1;
   }
   
   err = pf_lldp_parse_packet (buf, buf_len, &peer_data);

   if (!err)
   {
      if (
         peer_data.chassis_id.subtype == PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED &&
         peer_data.port_id.subtype == PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED)
      {
         LOG_DEBUG (
            PF_LLDP_LOG,
            "LLDP(%d): Receive port %u MAC: "
            "%02X:%02X:%02X:%02X:%02X:%02X "
            "Len: %d Chassis ID: %s Port ID: %s\n",
            __LINE__,
            loc_port_num,
            peer_data.mac_address.addr[0],
            peer_data.mac_address.addr[1],
            peer_data.mac_address.addr[2],
            peer_data.mac_address.addr[3],
            peer_data.mac_address.addr[4],
            peer_data.mac_address.addr[5],
            p_frame_buf->len,
            peer_data.chassis_id.string,
            peer_data.port_id.string);
      }
      else if (peer_data.port_id.subtype == PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED)
      {
         LOG_DEBUG (
            PF_LLDP_LOG,
            "LLDP(%d): LLDP frame received on port %u Len: %d Port "
            "ID: %s\n",
            __LINE__,
            loc_port_num,
            p_frame_buf->len,
            peer_data.port_id.string);
      }
      else
      {
         LOG_DEBUG (
            PF_LLDP_LOG,
            "LLDP(%d): Non-profinet LLDP frame received on port %u "
            "Len: "
            "%d\n",
            __LINE__,
            loc_port_num,
            p_frame_buf->len);
      }

      if (pf_port_is_valid (net, loc_port_num))
      {
         pf_lldp_update_peer (net, loc_port_num, &peer_data);
      }
      else
      {
         LOG_ERROR (
            PF_LLDP_LOG,
            "LLDP(%d): Unexpected local port %u, frame discarded.\n",
            __LINE__,
            loc_port_num);
      }
   }

   pnal_buf_free (p_frame_buf);

   return 1; /* Means: handled */
}
