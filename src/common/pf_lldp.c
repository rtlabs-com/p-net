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
#endif

/**
 * @file
 * @brief Implements Link Layer Discovery Protocol (LLDP), for neighbourhood
 * detection.
 *
 * Builds and sends LLDP frames.
 * Receives and parses LLDP frames.
 *
 * ToDo: Differentiate between device and port MAC addresses.
 * ToDo: Handle PNET_MAX_PORT ports.
 */

#include "pf_includes.h"
#include "pf_block_writer.h"
#include "pf_block_reader.h"
#include "pf_cmrpc.h"

#include <string.h>
#include <ctype.h>

#define LLDP_TYPE_END        0
#define LLDP_TYPE_CHASSIS_ID 1
#define LLDP_TYPE_PORT_ID    2
#define LLDP_TYPE_TTL        3
#define LLDP_TYPE_MANAGEMENT 8
#define LLDP_TYPE_ORG_SPEC   127

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

typedef struct lldp_tlv
{
   uint8_t type;
   uint16_t len;
   uint8_t * p_data;
} lldp_tlv_t;

static const char org_id_pnio[] = {0x00, 0x0e, 0xcf};
static const char org_id_ieee_8023[] = {0x00, 0x12, 0x0f};

static const char * shed_tag_tx = "lldp_tx";
static const char * shed_tag_rx = "lldp_rx";

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
lldp_tlv_t pf_lldp_get_tlv (pf_get_info_t * p_info, uint16_t * p_pos)
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
 * @param p_port_cfg       In:    LLDP configuration for this port
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The position in the buffer.
 */
static void pf_lldp_add_port_id_tlv (
   const pnet_lldp_port_cfg_t * p_port_cfg,
   uint8_t * p_buf,
   uint16_t * p_pos)
{
   uint16_t len;

   len = (uint16_t)strlen (p_port_cfg->port_id);

   pf_lldp_add_tlv_header (p_buf, p_pos, LLDP_TYPE_PORT_ID, 1 + len);

   pf_put_byte (
      PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
   pf_put_mem (p_port_cfg->port_id, len, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
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
   pf_put_uint16 (true, PNET_LLDP_TTL, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
}

/**
 * @internal
 * Insert the optional Profinet port status TLV into a buffer.
 *
 * The port status TLV is mandatory for ProfiNet.
 * @param p_port_cfg       In:    LLDP configuration for this port
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The position in the buffer.
 */
static void pf_lldp_add_port_status (
   const pnet_lldp_port_cfg_t * p_port_cfg,
   uint8_t * p_buf,
   uint16_t * p_pos)
{
   pf_lldp_add_pnio_header (p_buf, p_pos, 5);

   pf_put_byte (
      LLDP_PNIO_SUBTYPE_PORT_STATUS,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
   pf_put_uint16 (
      true,
      p_port_cfg->rtclass_2_status,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
   pf_put_uint16 (
      true,
      p_port_cfg->rtclass_3_status,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
}

/**
 * @internal
 * Insert the optional Profinet chassis MAC TLV into a buffer.
 *
 * The chassis MAC TLV is mandatory for ProfiNet.
 * @param p_mac_address    In:    Device MAC address.
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The position in the buffer.
 */
static void pf_lldp_add_chassis_mac (
   const pnet_ethaddr_t * p_mac_address,
   uint8_t * p_buf,
   uint16_t * p_pos)
{
   pf_lldp_add_pnio_header (p_buf, p_pos, 1 + sizeof (pnet_ethaddr_t));

   pf_put_byte (
      LLDP_PNIO_SUBTYPE_CHASSIS_MAC,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
   memcpy (&p_buf[*p_pos], p_mac_address->addr, sizeof (pnet_ethaddr_t));
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
 * @param p_port_cfg       In:    LLDP configuration for this port
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The position in the buffer.
 */
static void pf_lldp_add_ieee_mac_phy (
   const pnet_lldp_port_cfg_t * p_port_cfg,
   uint8_t * p_buf,
   uint16_t * p_pos)
{
   pf_lldp_add_ieee_header (p_buf, p_pos, 6);

   pf_put_byte (LLDP_IEEE_SUBTYPE_MAC_PHY, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_byte (p_port_cfg->cap_aneg, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_uint16 (true, p_port_cfg->cap_phy, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_uint16 (
      true,
      p_port_cfg->mau_type,
      PF_FRAME_BUFFER_SIZE,
      p_buf,
      p_pos);
}

/**
 * Insert the optional management data TLV into a buffer.
 * It is mandatory for ProfiNet.
 *
 * Contains the IP address.
 *
 * @param p_ipaddr         In:    IP address
 * @param p_buf            InOut: The buffer.
 * @param p_pos            InOut: The position in the buffer.
 */
static void pf_lldp_add_management (
   const os_ipaddr_t * p_ipaddr,
   uint8_t * p_buf,
   uint16_t * p_pos)
{
   /* TODO: Add more port info to the pnet_lldp_port_cfg_t configuration */
   pf_lldp_add_tlv_header (p_buf, p_pos, LLDP_TYPE_MANAGEMENT, 12);

   pf_put_byte (1 + 4, PF_FRAME_BUFFER_SIZE, p_buf, p_pos); /* Address string
                                                               length (incl
                                                               type) */
   pf_put_byte (1, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);     /* Type IPV4 */
   pf_put_uint32 (true, *p_ipaddr, PF_FRAME_BUFFER_SIZE, p_buf, p_pos);
   pf_put_byte (1, PF_FRAME_BUFFER_SIZE, p_buf, p_pos); /* Interface Subtype:
                                                           Unknown */
   pf_put_uint32 (true, 0, PF_FRAME_BUFFER_SIZE, p_buf, p_pos); /* Interface
                                                                   number:
                                                                   Unknown */
   pf_put_byte (0, PF_FRAME_BUFFER_SIZE, p_buf, p_pos); /* OID string length: 0
                                                           => Not supported */
}

/********************* Initialize and send **********************************/

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
 * @param arg              In:    Not used.
 * @param current_time     In:    Not used.
 */
static void pf_lldp_trigger_sending (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{
   pf_lldp_send (net);

   /* Reschedule */
   if (
      pf_scheduler_add (
         net,
         PF_LLDP_SEND_INTERVAL * 1000,
         shed_tag_tx,
         pf_lldp_trigger_sending,
         NULL,
         &net->lldp_timeout) != 0)
   {
      LOG_ERROR (
         PF_LLDP_LOG,
         "LLDP(%d): Failed to reschedule LLDP sending\n",
         __LINE__);
   }
}

/**
 * @internal
 * Handle LLDP peer timeout. Timeout occurs if no peer LLDP data is received
 * on port within the LLPD TTL timeout interval.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    Not used
 * @param current_time     In:    Not used
 */
static void pf_lldp_receive_timeout (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{
   net->lldp_rx_timeout = 0;
   LOG_WARNING (
      PF_LLDP_LOG,
      "LLDP(%d): Receive timeout expired - TODO trig alarm\n",
      __LINE__);
}

/**
 * @internal
 * Start or restart a timer that monitors reception of LLDP frames from peer.
 *
 * @param net              InOut: The p-net stack instance
 * @param timeout_in_secs  In:    TTL of the peer, typically 20 seconds.
 */
static void pf_lldp_reset_peer_timeout (pnet_t * net, uint16_t timeout_in_secs)
{
   if (net->lldp_rx_timeout != 0)
   {
      pf_scheduler_remove (net, shed_tag_rx, net->lldp_rx_timeout);
      net->lldp_rx_timeout = 0;
   }

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
   if (timeout_in_secs <= LLDP_PEER_MAX_TTL_IN_SECS)
   {
      if (
         pf_scheduler_add (
            net,
            timeout_in_secs * 1000000,
            shed_tag_rx,
            pf_lldp_receive_timeout,
            NULL,
            &net->lldp_rx_timeout) != 0)
      {
         LOG_ERROR (
            PF_LLDP_LOG,
            "LLDP(%d): Failed to add LLDP timeout\n",
            __LINE__);
      }
   }
}

void pf_lldp_get_port_config (
   pnet_t * net,
   int loc_port_num,
   const pnet_lldp_port_cfg_t ** pp_port_cfg)
{
   if (loc_port_num < 1 || loc_port_num > PNET_MAX_PORT)
   {
      *pp_port_cfg = NULL;
      return;
   }

   *pp_port_cfg = &net->fspm_cfg.lldp_cfg.ports[loc_port_num - 1];
}

void pf_lldp_get_port_list (pnet_t * net, pf_lldp_port_list_t * p_list)
{
   /* TODO: Implement support for multiple ports */
   /* TODO: Move this to some other source file, as the list of local ports is
    * not part of any LLDP TLV sent in LLDP frames.
    */
   memset (p_list, 0, sizeof (*p_list));
   p_list->ports[0] = BIT (8 - 1);
}

int pf_lldp_get_first_port (const pf_lldp_port_list_t * p_list)
{
   /* TODO: Implement support for multiple ports */
   /* TODO: Move this to some other source file, as the list of local ports is
    * not part of any LLDP TLV sent in LLDP frames.
    */
   return 1;
}

int pf_lldp_get_next_port (const pf_lldp_port_list_t * p_list, int loc_port_num)
{
   /* TODO: Implement support for multiple ports */
   /* TODO: Move this to some other source file, as the list of local ports is
    * not part of any LLDP TLV sent in LLDP frames.
    */
   return 0;
}

int pf_lldp_get_peer_timestamp (
   pnet_t * net,
   int loc_port_num,
   uint32_t * timestamp_10ms)
{
   bool is_received;

   /* TODO: Implement support for multiple ports */
   CC_ASSERT (loc_port_num > 0);
   os_mutex_lock (net->lldp_mutex);

   *timestamp_10ms = net->lldp_timestamp_for_last_peer_change;
   is_received = net->lldp_is_peer_info_received;

   os_mutex_unlock (net->lldp_mutex);
   return is_received ? 0 : -1;
}

void pf_lldp_get_chassis_id (pnet_t * net, pf_lldp_chassis_id_t * p_chassis_id)
{
   pnet_cfg_t * p_cfg = NULL;
   const char * station_name = NULL;

   /* Try to use NameOfStation as Chassis ID.
    *
    * FIXME: Use of pf_cmina_get_station_name() is not thread-safe, as the
    * returned pointer points to non-constant memory shared by multiple threads.
    * Fix this, e.g. using a mutex.
    *
    * TODO: Add option to use SystemIdentification as Chassis ID.
    * See IEC61158-6-10 table 361.
    */
   pf_cmina_get_station_name (net, &station_name);
   CC_ASSERT (station_name != NULL);

   p_chassis_id->len = strlen (station_name);
   if (p_chassis_id->len == 0 || p_chassis_id->len > PNET_LLDP_CHASSIS_ID_MAX_LEN)
   {
      pf_fspm_get_cfg (net, &p_cfg);
      CC_ASSERT (p_cfg != NULL);

      /* Use the MAC address */
      p_chassis_id->subtype = PF_LLDP_SUBTYPE_MAC;
      p_chassis_id->len = sizeof (pnet_ethaddr_t);
      memcpy (
         p_chassis_id->string,
         p_cfg->eth_addr.addr,
         sizeof (pnet_ethaddr_t));
   }
   else
   {
      /* Use the locally assigned chassis ID name */
      p_chassis_id->subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
      memcpy (p_chassis_id->string, station_name, p_chassis_id->len);
   }

   p_chassis_id->string[p_chassis_id->len] = '\0';
}

int pf_lldp_get_peer_chassis_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_chassis_id_t * p_chassis_id)
{
   bool is_received;

   CC_ASSERT (loc_port_num > 0);
   os_mutex_lock (net->lldp_mutex);

   /* TODO: Implement support for multiple ports */
   *p_chassis_id = net->lldp_peer_info.chassis_id;
   is_received = net->lldp_is_peer_info_received;

   /* TODO: Return error if received LLDP frame did not contain this TLV */
   os_mutex_unlock (net->lldp_mutex);
   return is_received ? 0 : -1;
}

void pf_lldp_get_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id)
{
   const pnet_lldp_port_cfg_t * p_port_cfg = NULL;

   pf_lldp_get_port_config (net, loc_port_num, &p_port_cfg);
   CC_ASSERT (p_port_cfg != NULL);

   /* TODO: Implement support for multiple ports */
   CC_ASSERT (loc_port_num > 0);
   snprintf (
      p_port_id->string,
      sizeof (p_port_id->string),
      "%s",
      p_port_cfg->port_id);
   p_port_id->subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
   p_port_id->len = strlen (p_port_id->string);
}

int pf_lldp_get_peer_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id)
{
   bool is_received;
   size_t len;

   CC_ASSERT (loc_port_num > 0);
   os_mutex_lock (net->lldp_mutex);

   /* TODO: Implement support for multiple ports */
   len = MIN (net->lldp_peer_info.port_id_len, sizeof (p_port_id->string) - 1);
   memcpy (p_port_id->string, net->lldp_peer_info.port_id, len);
   p_port_id->string[len] = '\0';
   p_port_id->subtype = net->lldp_peer_info.port_id_subtype;
   p_port_id->len = len;
   is_received = net->lldp_is_peer_info_received;

   /* TODO: Return error if received LLDP frame did not contain this TLV */
   os_mutex_unlock (net->lldp_mutex);
   return is_received ? 0 : -1;
}

void pf_lldp_get_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_descr)
{
   /* TODO: Implement support for multiple ports */
   CC_ASSERT (loc_port_num > 0);
   snprintf (
      p_port_descr->string,
      sizeof (p_port_descr->string),
      "%s",
      net->interface_name);
   p_port_descr->len = strlen (p_port_descr->string);
}

int pf_lldp_get_peer_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_desc)
{
   bool is_received;

   CC_ASSERT (loc_port_num > 0);
   os_mutex_lock (net->lldp_mutex);

   /* TODO: Copy data previously received in LLDP frame on this port */
   memset (p_port_desc, 0, sizeof (*p_port_desc));
   snprintf (
      p_port_desc->string,
      sizeof (p_port_desc->string),
      "%s",
      "peer port descr");
   p_port_desc->len = strlen (p_port_desc->string);
   is_received = net->lldp_is_peer_info_received;

   /* TODO: Return error if received LLDP frame did not contain this TLV */
   os_mutex_unlock (net->lldp_mutex);
   return is_received ? 0 : -1;
}

void pf_lldp_get_management_address (
   pnet_t * net,
   pf_lldp_management_address_t * p_man_address)
{
   os_ipaddr_t ipaddr;

   pf_cmina_get_ipaddr (net, &ipaddr);
   p_man_address->subtype = 1; /* IPv4 */
   ipaddr = CC_TO_BE32 (ipaddr);
   memcpy (p_man_address->value, &ipaddr, sizeof (ipaddr));
   p_man_address->len = sizeof (ipaddr);
}

int pf_lldp_get_peer_management_address (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_management_address_t * p_man_address)
{
   bool is_received;

   CC_ASSERT (loc_port_num > 0);
   os_mutex_lock (net->lldp_mutex);

   /* TODO: Copy data previously received in LLDP frame on this port */
   memset (p_man_address, 0, sizeof (*p_man_address));
   p_man_address->subtype = 1;
   p_man_address->value[0] = 10;
   p_man_address->value[1] = 11;
   p_man_address->value[2] = 12;
   p_man_address->value[3] = 13;
   p_man_address->len = 4;
   is_received = net->lldp_is_peer_info_received;

   /* TODO: Return error if received LLDP frame did not contain this TLV */
   os_mutex_unlock (net->lldp_mutex);
   return is_received ? 0 : -1;
}

void pf_lldp_get_management_port_index (
   pnet_t * net,
   pf_lldp_management_port_index_t * p_man_port_index)
{
   /* TODO: Get actual ifIndex value */
   memset (p_man_port_index, 0, sizeof (*p_man_port_index));
   p_man_port_index->subtype = 2; /* ifIndex */
   p_man_port_index->index = 1;
}

int pf_lldp_get_peer_management_port_index (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_management_port_index_t * p_man_port_index)
{
   bool is_received;

   CC_ASSERT (loc_port_num > 0);
   os_mutex_lock (net->lldp_mutex);

   /* TODO: Copy data previously received in LLDP frame on this port */
   memset (p_man_port_index, 0, sizeof (*p_man_port_index));
   p_man_port_index->subtype = 2; /* ifIndex */
   p_man_port_index->index = 1;
   is_received = net->lldp_is_peer_info_received;

   /* TODO: Return error if received LLDP frame did not contain this TLV */
   os_mutex_unlock (net->lldp_mutex);
   return is_received ? 0 : -1;
}

int pf_lldp_get_peer_station_name (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_station_name_t * p_station_name)
{
   bool is_received;

   CC_ASSERT (loc_port_num > 0);
   os_mutex_lock (net->lldp_mutex);

   /* TODO: Copy data previously received in LLDP frame on this port.
    * Value should be part of either received Chassis ID or Port ID.
    */
   memset (p_station_name, 0, sizeof (*p_station_name));
   snprintf (
      p_station_name->string,
      sizeof (p_station_name->string),
      "%s",
      "peer station-name");
   p_station_name->len = strlen (p_station_name->string);
   is_received = net->lldp_is_peer_info_received;

   /* TODO: Return error if received LLDP frame did not contain this TLV */
   os_mutex_unlock (net->lldp_mutex);
   return is_received ? 0 : -1;
}

void pf_lldp_get_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_signal_delay_t * p_delays)
{
   CC_ASSERT (loc_port_num > 0);

   /* Line delay measurement is not supported */
   p_delays->line_propagation_delay_ns = 0; /* Not measured */
   p_delays->port_rx_delay_ns = 0;          /* Not measured */
   p_delays->port_tx_delay_ns = 0;          /* Not measured */
}

int pf_lldp_get_peer_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_signal_delay_t * p_delays)
{
   bool is_received;
   const pnet_lldp_peer_info_t * peer_info;

   CC_ASSERT (loc_port_num > 0);
   os_mutex_lock (net->lldp_mutex);

   /* TODO: Implement support for multiple ports */
   peer_info = &net->lldp_peer_info;
   p_delays->line_propagation_delay_ns =
      peer_info->port_delay.cable_delay_local;
   p_delays->port_rx_delay_ns = peer_info->port_delay.rx_delay_local;
   p_delays->port_tx_delay_ns = peer_info->port_delay.tx_delay_local;
   is_received = net->lldp_is_peer_info_received;

   /* TODO: Return error if received LLDP frame did not contain this TLV */
   os_mutex_unlock (net->lldp_mutex);
   return is_received ? 0 : -1;
}

void pf_lldp_get_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status)
{
   const pnet_lldp_port_cfg_t * p_port_cfg = NULL;

   CC_ASSERT (loc_port_num > 0);

   pf_lldp_get_port_config (net, loc_port_num, &p_port_cfg);
   CC_ASSERT (p_port_cfg != NULL);

   /* TODO: Get current link status from Ethernet driver */
   /* TODO: Implement support for multiple ports */
   p_link_status->auto_neg_supported = p_port_cfg->cap_aneg &
                                       PNET_LLDP_AUTONEG_SUPPORTED;
   p_link_status->auto_neg_enabled = p_port_cfg->cap_aneg &
                                     PNET_LLDP_AUTONEG_ENABLED;
   p_link_status->auto_neg_advertised_cap = p_port_cfg->cap_phy;
   p_link_status->oper_mau_type = p_port_cfg->mau_type;
}

int pf_lldp_get_peer_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status)
{
   bool is_received;
   const pnet_lldp_peer_info_t * peer_info;

   CC_ASSERT (loc_port_num > 0);
   os_mutex_lock (net->lldp_mutex);

   /* TODO: Implement support for multiple ports */
   peer_info = &net->lldp_peer_info;
   p_link_status->auto_neg_supported =
      peer_info->phy_config.aneg_support_status & PNET_LLDP_AUTONEG_SUPPORTED;
   p_link_status->auto_neg_enabled = peer_info->phy_config.aneg_support_status &
                                     PNET_LLDP_AUTONEG_ENABLED;
   p_link_status->auto_neg_advertised_cap = peer_info->phy_config.aneg_cap;
   p_link_status->oper_mau_type = peer_info->phy_config.operational_mau_type;
   is_received = net->lldp_is_peer_info_received;

   /* TODO: Return error if received LLDP frame did not contain this TLV */
   os_mutex_unlock (net->lldp_mutex);
   return is_received ? 0 : -1;
}
/**
 * Build and send a LLDP message on a specific port.
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. N, where N is the total
 *                                number of local ports used by p-net stack.
 */
static void pf_lldp_send_port (pnet_t * net, int loc_port_num)
{
   os_buf_t * p_lldp_buffer = os_buf_alloc (PF_FRAME_BUFFER_SIZE);
   uint8_t * p_buf = NULL;
   uint16_t pos = 0;
   pnet_cfg_t * p_cfg = NULL;
   const pnet_lldp_port_cfg_t * p_port_cfg = NULL;
   os_ipaddr_t ipaddr = 0;
   char ip_string[OS_INET_ADDRSTRLEN] = {0};
   pf_lldp_chassis_id_t chassis_id;

   pf_fspm_get_cfg (net, &p_cfg);
   /*
    * LLDP-PDU ::=  LLDPChassis, LLDPPort, LLDPTTL, LLDP-PNIO-PDU, LLDPEnd
    *
    * LLDPChassis ::= LLDPChassisStationName ^
    *                 LLDPChassisMacAddress           (If no station name)
    * LLDPChassisStationName ::= LLDP_TLVHeader,      (According to IEEE
    * 802.1AB-2016) LLDP_ChassisIDSubType(7),       (According to IEEE
    * 802.1AB-2016) LLDP_ChassisID LLDPChassisMacAddress ::= LLDP_TLVHeader,
    * (According to IEEE 802.1AB-2016) LLDP_ChassisIDSubType(4), (According to
    * IEEE 802.1AB-2016)
    *
    * LLDP-PNIO-PDU ::= {
    *                [LLDP_PNIO_DELAY],               (If LineDelay measurement
    * is supported) LLDP_PNIO_PORTSTATUS, [LLDP_PNIO_ALIAS],
    *                [LLDP_PNIO_MRPPORTSTATUS],       (If MRP is activated for
    * this port) [LLDP_PNIO_MRPICPORTSTATUS],     (If MRP Interconnection is
    * activated for this port) LLDP_PNIO_CHASSIS_MAC, LLDP8023MACPHY, (If IEEE
    * 802.3 is used) LLDPManagement,                  (According to IEEE
    * 802.1AB-2016, 8.5.9) [LLDP_PNIO_PTCPSTATUS],          (If PTCP is
    * activated by means of the PDSyncData Record) [LLDP_PNIO_MAUTypeExtension],
    * (If a MAUType with MAUTypeExtension is used and may exist otherwise)
    *                [LLDPOption*],                   (Other LLDP options may be
    * used concurrently) [LLDP8021*], [LLDP8023*]
    *                }
    *
    * LLDP_PNIO_HEADER ::= LLDP_TLVHeader,            (According to IEEE
    * 802.1AB-2016) LLDP_OUI(00-0E-CF)
    *
    * LLDP_PNIO_PORTSTATUS ::= LLDP_PNIO_HEADER, LLDP_PNIO_SubType(0x02),
    * RTClass2_PortStatus, RTClass3_PortStatus
    *
    * LLDP_PNIO_CHASSIS_MAC ::= LLDP_PNIO_HEADER, LLDP_PNIO_SubType(0x05), (
    *                CMResponderMacAdd ^
    *                CMInitiatorMacAdd                (Shall be the interface
    * MAC address of the transmitting node)
    *                )
    *
    * LLDP8023MACPHY ::= LLDP_TLVHeader,              (According to IEEE
    * 802.1AB-2016) LLDP_OUI(00-12-0F),              (According to IEEE
    * 802.1AB-2016, Annex F) LLDP_8023_SubType(1),            (According to IEEE
    * 802.1AB-2016, Annex F) LLDP_8023_AUTONEG,               (According to IEEE
    * 802.1AB-2016, Annex F) LLDP_8023_PMDCAP,                (According to IEEE
    * 802.1AB-2016, Annex F) LLDP_8023_OPMAU                  (According to IEEE
    * 802.1AB-2016, Annex F)
    *
    * LLDPManagement ::= LLDP_TLVHeader,              (According to IEEE
    * 802.1AB-2016) LLDP_ManagementData              (Use PNIO MIB Enterprise
    * number = 24686 (dec))
    *
    * LLDP_ManagementData ::=
    */
   if (p_lldp_buffer != NULL)
   {
      p_buf = p_lldp_buffer->payload;
      if (p_buf != NULL)
      {
         pf_lldp_get_port_config (net, loc_port_num, &p_port_cfg);
         if (p_port_cfg != NULL)
         {
            pf_lldp_get_chassis_id (net, &chassis_id);
            pf_cmina_get_ipaddr (net, &ipaddr);
            pf_cmina_ip_to_string (ipaddr, ip_string);
            LOG_DEBUG (
               PF_LLDP_LOG,
               "LLDP(%d): Sending LLDP frame. MAC "
               "%02X:%02X:%02X:%02X:%02X:%02X "
               "IP: %s Chassis ID: %s Port number: %u Port ID: %s\n",
               __LINE__,
               p_cfg->eth_addr.addr[0],
               p_cfg->eth_addr.addr[1],
               p_cfg->eth_addr.addr[2],
               p_cfg->eth_addr.addr[3],
               p_cfg->eth_addr.addr[4],
               p_cfg->eth_addr.addr[5],
               ip_string,
               chassis_id.string,
               loc_port_num,
               p_port_cfg->port_id);

            pos = 0;

            /* Add destination MAC address */
            pf_put_mem (
               &lldp_dst_addr,
               sizeof (lldp_dst_addr),
               PF_FRAME_BUFFER_SIZE,
               p_buf,
               &pos);

            /* Add source MAC address.  */
            memcpy (
               &p_buf[pos],
               p_port_cfg->port_addr.addr,
               sizeof (pnet_ethaddr_t));
            pos += sizeof (pnet_ethaddr_t);

            /* Add Ethertype for LLDP */
            pf_put_uint16 (
               true,
               OS_ETHTYPE_LLDP,
               PF_FRAME_BUFFER_SIZE,
               p_buf,
               &pos);

            /* Add mandatory parts */
            pf_lldp_add_chassis_id_tlv (&chassis_id, p_buf, &pos);
            pf_lldp_add_port_id_tlv (p_port_cfg, p_buf, &pos);
            pf_lldp_add_ttl_tlv (p_buf, &pos);

            /* Add optional parts */
            pf_lldp_add_port_status (p_port_cfg, p_buf, &pos);
            pf_lldp_add_chassis_mac (&p_cfg->eth_addr, p_buf, &pos);
            pf_lldp_add_ieee_mac_phy (p_port_cfg, p_buf, &pos);
            pf_lldp_add_management (&ipaddr, p_buf, &pos);

            /* Add end of LLDP-PDU marker */
            pf_lldp_add_tlv_header (p_buf, &pos, LLDP_TYPE_END, 0);

            p_lldp_buffer->len = pos;

            (void)pf_eth_send (net, net->eth_handle, p_lldp_buffer);
         }
      }

      os_buf_free (p_lldp_buffer);
   }
}

void pf_lldp_send (pnet_t * net)
{
   pf_lldp_send_port (net, PNET_PORT_1);
}

void pf_lldp_tx_restart (pnet_t * net, bool send)
{
   if (net->lldp_timeout != 0)
   {
      pf_scheduler_remove (net, shed_tag_tx, net->lldp_timeout);
      net->lldp_timeout = 0;
   }

   if (
      pf_scheduler_add (
         net,
         PF_LLDP_SEND_INTERVAL * 1000,
         shed_tag_tx,
         pf_lldp_trigger_sending,
         NULL,
         &net->lldp_timeout) != 0)
   {
      LOG_ERROR (
         PF_ETH_LOG,
         "LLDP(%d): Failed to schedule LLDP sending\n",
         __LINE__);
   }

   if (send)
   {
      pf_lldp_send (net);
   }
}

void pf_lldp_init (pnet_t * net)
{
   memset (&net->lldp_peer_info, 0, sizeof (net->lldp_peer_info));

   net->lldp_mutex = os_mutex_create();
   CC_ASSERT (net->lldp_mutex != NULL);

   pf_lldp_tx_restart (net, true);
}

/**
 * @internal
 * Parse LLDP frame buffer
 *
 * @param buf              In:    LLDP data.
 * @param len              In:    Length of LLDP data.
 * @param lldp_peer_info   Out:   Record holding parsed LLDP information.
 * @return 0 if parsing is successful, -1 on error.
 */
int pf_lldp_parse_packet (
   uint8_t buf[],
   uint16_t len,
   pnet_lldp_peer_info_t * lldp_peer_info)
{
   lldp_tlv_t tlv;
   pf_get_info_t parse_info;
   uint16_t offset = 0;

   parse_info.result = PF_PARSE_OK;
   parse_info.is_big_endian = true;
   parse_info.len = len;
   parse_info.p_buf = buf;

   tlv = pf_lldp_get_tlv (&parse_info, &offset);

   while (tlv.type != LLDP_TYPE_END)
   {
      switch (tlv.type)
      {
      case LLDP_TYPE_CHASSIS_ID:
         if (
            (tlv.len > 0) && ((size_t) (tlv.len - 1) <
                              sizeof (lldp_peer_info->chassis_id.string)))
         {
            lldp_peer_info->chassis_id.len = tlv.len - 1;
            lldp_peer_info->chassis_id.subtype =
               pf_get_byte (&parse_info, &offset);
            pf_get_mem (
               &parse_info,
               &offset,
               lldp_peer_info->chassis_id.len,
               &lldp_peer_info->chassis_id.string);
            lldp_peer_info->chassis_id.string[lldp_peer_info->chassis_id.len] =
               '\0';
         }
         else
         {
            offset += tlv.len;
            if (tlv.len > 1)
            {
               LOG_ERROR (
                  PF_LLDP_LOG,
                  "LLDP(%d): Chassis Id too long (%d)\n",
                  __LINE__,
                  tlv.len - 1);
               /* Should not happen. Reconfigure PNET_LLDP_CHASSIS_ID_MAX_LEN
                * if it does*/
            }
            else
            {
               LOG_WARNING (
                  PF_LLDP_LOG,
                  "LLDP(%d): Ignoring Chassis Id, tlv len=%d\n",
                  __LINE__,
                  tlv.len);
            }
         }
         break;
      case LLDP_TYPE_PORT_ID:
         if (
            (tlv.len > 0) &&
            ((size_t) (tlv.len - 1) < sizeof (lldp_peer_info->port_id)))
         {
            lldp_peer_info->port_id_len = tlv.len - 1;
            lldp_peer_info->port_id_subtype =
               pf_get_byte (&parse_info, &offset);
            pf_get_mem (
               &parse_info,
               &offset,
               lldp_peer_info->port_id_len,
               &lldp_peer_info->port_id);
            lldp_peer_info->port_id[lldp_peer_info->port_id_len] = '\0';
         }
         else
         {
            offset += tlv.len;
            if (tlv.len > 1)
            {
               LOG_ERROR (
                  PF_LLDP_LOG,
                  "LLDP(%d): Port Id too long (%d)\n",
                  __LINE__,
                  tlv.len - 1);
               /* Should not happen. Reconfigure PNET_LLDP_PORT_ID_MAX_LEN if it
                * does */
            }
            else
            {
               LOG_WARNING (
                  PF_LLDP_LOG,
                  "LLDP(%d): Ignoring Port Id, tlv len=%d\n",
                  __LINE__,
                  tlv.len);
            }
         }
         break;
      case LLDP_TYPE_TTL:
         if (tlv.len == 2)
         {
            lldp_peer_info->ttl = pf_get_uint16 (&parse_info, &offset);
         }
         else
         {
            parse_info.result = PF_PARSE_ERROR;
            LOG_WARNING (
               PF_LLDP_LOG,
               "LLDP(%d): Ignoring TTL, unexpected tlv len %d(2)\n",
               __LINE__,
               tlv.len);
         }
         break;
      case LLDP_TYPE_ORG_SPEC:
      {
         uint8_t org_id[3];
         uint8_t org_subtype;
         pf_get_mem (&parse_info, &offset, sizeof (org_id), org_id);
         org_subtype = pf_get_byte (&parse_info, &offset);

         if (memcmp (org_id_pnio, org_id, sizeof (org_id)) == 0)
         {
            switch (org_subtype)
            {
            case LLDP_PNIO_SUBTYPE_MEAS_DELAY_VALUES:
               if (tlv.len == LLDP_PNIO_SUBTYPE_MEAS_DELAY_VALUES_TLV_LEN)
               {
                  lldp_peer_info->port_delay.rx_delay_local =
                     pf_get_uint32 (&parse_info, &offset);
                  lldp_peer_info->port_delay.rx_delay_remote =
                     pf_get_uint32 (&parse_info, &offset);
                  lldp_peer_info->port_delay.tx_delay_local =
                     pf_get_uint32 (&parse_info, &offset);
                  lldp_peer_info->port_delay.tx_delay_remote =
                     pf_get_uint32 (&parse_info, &offset);
                  lldp_peer_info->port_delay.cable_delay_local =
                     pf_get_uint32 (&parse_info, &offset);
               }
               else
               {
                  parse_info.result = PF_PARSE_ERROR;
               }
               break;
            case LLDP_PNIO_SUBTYPE_PORT_STATUS:
               if (tlv.len == LLDP_PNIO_SUBTYPE_PORT_STATUS_TLV_LEN)
               {
                  pf_get_mem (
                     &parse_info,
                     &offset,
                     4,
                     lldp_peer_info->port_status);
               }
               else
               {
                  parse_info.result = PF_PARSE_ERROR;
               }
               break;
            case LLDP_PNIO_SUBTYPE_CHASSIS_MAC:
               if (tlv.len == LLDP_PNIO_SUBTYPE_CHASSIS_MAC_TLV_LEN)
               {
                  pf_get_mem (
                     &parse_info,
                     &offset,
                     6,
                     &lldp_peer_info->mac_address.addr);
               }
               else
               {
                  parse_info.result = PF_PARSE_ERROR;
               }
               break;
            default:
               offset += (tlv.len - 4);
               break;
            }
         }
         else if (memcmp (org_id_ieee_8023, org_id, sizeof (org_id)) == 0)
         {
            switch (org_subtype)
            {
            case LLDP_IEEE_SUBTYPE_MAC_PHY:
               if (tlv.len == LLDP_IEEE_SUBTYPE_MAC_PHY_TLV_LEN)
               {
                  lldp_peer_info->phy_config.aneg_support_status =
                     pf_get_byte (&parse_info, &offset);
                  lldp_peer_info->phy_config.aneg_cap =
                     pf_get_uint16 (&parse_info, &offset);
                  lldp_peer_info->phy_config.operational_mau_type =
                     pf_get_uint16 (&parse_info, &offset);
               }
               else
               {
                  parse_info.result = PF_PARSE_ERROR;
               }
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
      tlv = pf_lldp_get_tlv (&parse_info, &offset);
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
 * Generate an alias name from port_id and chassis_id
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

/**
 * Apply updated peer information
 * @param net              InOut: p-net stack instance
 * @param lldp_peer_info   In:    Peer data to be applied
 */
void pf_lldp_update_peer (
   pnet_t * net,
   const pnet_lldp_peer_info_t * lldp_peer_info)
{
   int error = 0;
   char alias[sizeof (net->cmina_current_dcp_ase.alias_name)];
   pnet_lldp_peer_info_t * stored_peer_info = &net->lldp_peer_info;

   pf_lldp_reset_peer_timeout (net, lldp_peer_info->ttl);

   if (
      memcmp (
         lldp_peer_info,
         stored_peer_info,
         sizeof (pnet_lldp_peer_info_t)) == 0)
   {
      /* No changes */
      return;
   }

   error = pf_lldp_generate_alias_name (
      lldp_peer_info->port_id,
      lldp_peer_info->chassis_id.string,
      alias,
      sizeof (alias));
   if (!error && (strcmp (alias, net->cmina_current_dcp_ase.alias_name) != 0))
   {
      LOG_INFO (
         PF_LLDP_LOG,
         "LLDP(%d): Updating alias name: %s -> %s\n",
         __LINE__,
         net->cmina_current_dcp_ase.alias_name,
         alias);

      strncpy (
         net->cmina_current_dcp_ase.alias_name,
         alias,
         sizeof (net->cmina_current_dcp_ase.alias_name));

      /*
       * TODO - pf_lldp_send_remote_mismatch_alarm(net);
       */
   }

   /* Update stored peer information
    *
    * We lock a mutex as other threads (e.g. an SNMP agent) may read from it.
    */
   os_mutex_lock (net->lldp_mutex);
   net->lldp_is_peer_info_received = true;
   net->lldp_timestamp_for_last_peer_change = os_get_system_uptime_10ms();
   memcpy (stored_peer_info, lldp_peer_info, sizeof (pnet_lldp_peer_info_t));
   os_mutex_unlock (net->lldp_mutex);
}

int pf_lldp_recv (pnet_t * net, os_buf_t * p_frame_buf, uint16_t offset)
{
   uint8_t * buf = p_frame_buf->payload + offset;
   uint16_t buf_len = p_frame_buf->len - offset;
   pnet_lldp_peer_info_t peer_data = {0};
   int err = 0;

   err = pf_lldp_parse_packet (buf, buf_len, &peer_data);

   if (!err)
   {
      LOG_DEBUG (
         PF_LLDP_LOG,
         "LLDP(%d): Received LLDP packet from %02X:%02X:%02X:%02X:%02X:%02X "
         "Len: %d Chassis ID: %s Port ID: %s\n",
         __LINE__,
         peer_data.mac_address.addr[0],
         peer_data.mac_address.addr[1],
         peer_data.mac_address.addr[2],
         peer_data.mac_address.addr[3],
         peer_data.mac_address.addr[4],
         peer_data.mac_address.addr[5],
         p_frame_buf->len,
         peer_data.chassis_id.string,
         peer_data.port_id);
      pf_lldp_update_peer (net, &peer_data);
   }

   if (p_frame_buf != NULL)
   {
      os_buf_free (p_frame_buf);
   }

   return 1; /* Means: handled */
}
