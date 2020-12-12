/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2020 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

/**
 * @file
 * @brief Helper functions for use by platform-dependent SNMP server (agent).
 *
 */

#ifdef UNIT_TEST
#define pf_lldp_get_management_address mock_pf_lldp_get_management_address
#define pf_lldp_get_peer_management_address                                    \
   mock_pf_lldp_get_peer_management_address
#define pf_lldp_get_link_status      mock_pf_lldp_get_link_status
#define pf_lldp_get_peer_link_status mock_pf_lldp_get_peer_link_status
#endif

#include "pf_includes.h"
#include <string.h>

#define STRINGIFY(s)   STRINGIFIED (s)
#define STRINGIFIED(s) #s

/**
 * Reverse bits in byte.
 *
 * @param byte             In:    Byte with bit pattern to reverse.
 * @return Bit pattern in reversed order.
 */
static uint8_t pf_snmp_bit_reversed_byte (const uint8_t byte)
{
   uint8_t reversed = 0x00;

   reversed |= (byte & BIT (7)) >> 7;
   reversed |= (byte & BIT (0)) << 7;

   reversed |= (byte & BIT (6)) >> 5;
   reversed |= (byte & BIT (1)) << 5;

   reversed |= (byte & BIT (5)) >> 3;
   reversed |= (byte & BIT (2)) << 3;

   reversed |= (byte & BIT (4)) >> 1;
   reversed |= (byte & BIT (3)) << 1;

   return reversed;
}

/**
 * Encode link status for use in SNMP response.
 *
 * @param encoded          Out:   Encoded link status.
 * @param plain            In:    Unencoded link status.
 */
static void pf_snmp_encode_link_status (
   pf_snmp_link_status_t * encoded,
   const pf_lldp_link_status_t * plain)
{
   /* See RFC 2579 "Textual Conventions for SMIv2": "TruthValue" */
   encoded->auto_neg_supported = plain->is_autonegotiation_supported ? 1 : 2;
   encoded->auto_neg_enabled = plain->is_autonegotiation_enabled ? 1 : 2;

   encoded->oper_mau_type = plain->operational_mau_type;

   /* See RFC 1906 "Transport Mappings for Version 2 of the
    *  Simple Network Management Protocol (SNMPv2)", ch. 8:
    * "Serialization using the Basic Encoding Rules", clause 3:
    * "When encoding an object whose syntax is described using the BITS
    *  construct, the value is encoded as an OCTET STRING, in which all
    *  the named bits in (the definition of) the bitstring, commencing
    *  with the first bit and proceeding to the last bit, are placed in
    *  bits 8 to 1 of the first octet, followed by bits 8 to 1 of each
    *  subsequent octet in turn, followed by as many bits as are needed of
    *  the final subsequent octet, commencing with bit 8.  Remaining bits,
    *  if any, of the final octet are set to zero on generation and
    *  ignored on receipt."
    */
   encoded->auto_neg_advertised_cap[0] = pf_snmp_bit_reversed_byte (
      (plain->autonegotiation_advertised_capabilities >> 0) & 0xff);
   encoded->auto_neg_advertised_cap[1] = pf_snmp_bit_reversed_byte (
      (plain->autonegotiation_advertised_capabilities >> 8) & 0xff);
}

/**
 * Encode management address for use in SNMP response.
 *
 * @param encoded          Out:   Encoded management address.
 * @param plain            In:    Unencoded management address.
 */
static void pf_snmp_encode_management_address (
   pf_snmp_management_address_t * encoded,
   const pf_lldp_management_address_t * plain)
{
   encoded->subtype = plain->subtype;

   /* See RFC 2578 "Structure of Management Information Version 2 (SMIv2)",
    * section 7.7, clause 3:
    * "string-valued, variable-length strings (not preceded by the IMPLIED
    * keyword):  `n+1' sub-identifiers, where `n' is the length of the
    * string (the first sub-identifier is `n' itself, following this,
    * each octet of the string is encoded in a separate sub-identifier);""
    */
   encoded->len = plain->len + 1;
   encoded->value[0] = plain->len;
   memcpy (&encoded->value[1], plain->value, plain->len);
}

/**
 * Encode signal delays for use in SNMP response.
 *
 * @param encoded          Out:   Encoded signal delays.
 * @param plain            In:    Unencoded signal delays.
 */
static void pf_snmp_encode_signal_delays (
   pf_snmp_signal_delay_t * encoded,
   const pf_lldp_signal_delay_t * plain)
{
   encoded->port_tx_delay_ns = plain->tx_delay_local;
   encoded->port_rx_delay_ns = plain->rx_delay_local;
   encoded->line_propagation_delay_ns = plain->cable_delay_local;
}

void pf_snmp_get_system_name (pnet_t * net, pf_snmp_system_name_t * name)
{
   int error;

   CC_STATIC_ASSERT (sizeof (name->string) >= PNAL_HOST_NAME_MAX);

   error = pnal_get_hostname (name->string);
   if (error)
   {
      name->string[0] = '\0';
   }
   name->string[sizeof (name->string) - 1] = '\0';
}

int pf_snmp_set_system_name (pnet_t * net, const pf_snmp_system_name_t * name)
{
   /* TODO: Set new hostname permanently. Perhaps call pnal_set_ip_suite()? */
   return -1;
}

void pf_snmp_get_system_contact (
   pnet_t * net,
   pf_snmp_system_contact_t * contact)
{
   const char * directory = net->fspm_cfg.file_directory;
   int error;

   error = pf_file_load (
      directory,
      PF_FILENAME_SYSCONTACT,
      contact,
      sizeof (*contact));
   if (error)
   {
      contact->string[0] = '\0';
      LOG_INFO (
         PF_SNMP_LOG,
         "SNMP(%d): Could not yet read sysContact from nvm.\n",
         __LINE__);
   }
   else
   {
      LOG_INFO (
         PF_SNMP_LOG,
         "SNMP(%d): Did read sysContact from nvm.\n",
         __LINE__);
   }
   contact->string[sizeof (contact->string) - 1] = '\0';
}

int pf_snmp_set_system_contact (
   pnet_t * net,
   const pf_snmp_system_contact_t * contact)
{
   const char * directory = net->fspm_cfg.file_directory;
   pf_snmp_system_contact_t temporary_buffer;
   int res;

   res = pf_file_save_if_modified (
      directory,
      PF_FILENAME_SYSCONTACT,
      contact,
      &temporary_buffer,
      sizeof (*contact));
   switch (res)
   {
   case 2:
      LOG_INFO (
         PF_SNMP_LOG,
         "SNMP(%d): First nvm saving of sysContact. "
         "sysContact: %s\n",
         __LINE__,
         contact->string);
      break;
   case 1:
      LOG_INFO (
         PF_SNMP_LOG,
         "SNMP(%d): Updating nvm stored sysContact. "
         "sysContact: %s\n",
         __LINE__,
         contact->string);
      break;
   case 0:
      LOG_INFO (
         PF_SNMP_LOG,
         "SNMP(%d): No storing of nvm sysContact (no changes). "
         "sysContact: %s\n",
         __LINE__,
         contact->string);
      break;
   default:
      /* Fall-through */
   case -1:
      LOG_ERROR (
         PF_SNMP_LOG,
         "SNMP(%d): Failed to store nvm sysContact.\n",
         __LINE__);
      break;
   }

   return res == -1 ? -1 : 0;
}

void pf_snmp_get_system_location (
   pnet_t * net,
   pf_snmp_system_location_t * location)
{
   pf_fspm_get_system_location (net, location);
}

int pf_snmp_set_system_location (
   pnet_t * net,
   const pf_snmp_system_location_t * location)
{
   pf_fspm_save_system_location (net, location);
   return 0;
}

void pf_snmp_get_system_description (
   pnet_t * net,
   pf_snmp_system_description_t * description)
{
   /* Encode as per Profinet specification:
    * DeviceType, Blank, OrderID, Blank, IM_Serial_Number, Blank, HWRevision,
    * Blank, SWRevisionPrefix, SWRevision.
    *
    * See IEC CDV 61158-6-10 (PN-AL-Protocol) ch. 5.1.2 "APDU abstract syntax",
    * field "SystemIdentification".
    */
   snprintf (
      description->string,
      sizeof (description->string),
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

void pf_snmp_get_port_list (pnet_t * net, pf_lldp_port_list_t * p_list)
{
   pf_port_get_list_of_ports (net, PNET_MAX_PORT, p_list);
}

void pf_snmp_init_port_iterator (pnet_t * net, pf_port_iterator_t * p_iterator)
{
   pf_port_init_iterator_over_ports (net, p_iterator);
}

int pf_snmp_get_next_port (pf_port_iterator_t * p_iterator)
{
   return pf_port_get_next (p_iterator);
}

int pf_snmp_get_peer_timestamp (
   pnet_t * net,
   int loc_port_num,
   uint32_t * timestamp_10ms)
{
   return pf_lldp_get_peer_timestamp (net, loc_port_num, timestamp_10ms);
}

void pf_snmp_get_chassis_id (pnet_t * net, pf_lldp_chassis_id_t * p_chassis_id)
{
   pf_lldp_get_chassis_id (net, p_chassis_id);
}

int pf_snmp_get_peer_chassis_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_chassis_id_t * p_chassis_id)
{
   return pf_lldp_get_peer_chassis_id (net, loc_port_num, p_chassis_id);
}

void pf_snmp_get_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id)
{
   pf_lldp_get_port_id (net, loc_port_num, p_port_id);
}

int pf_snmp_get_peer_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id)
{
   return pf_lldp_get_peer_port_id (net, loc_port_num, p_port_id);
}

void pf_snmp_get_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_descr)
{
   pf_lldp_get_port_description (net, loc_port_num, p_port_descr);
}

int pf_snmp_get_peer_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_desc)
{
   return pf_lldp_get_peer_port_description (net, loc_port_num, p_port_desc);
}

void pf_snmp_get_management_address (
   pnet_t * net,
   pf_snmp_management_address_t * p_man_address)
{
   pf_lldp_management_address_t man_address;

   pf_lldp_get_management_address (net, &man_address);
   pf_snmp_encode_management_address (p_man_address, &man_address);
}

int pf_snmp_get_peer_management_address (
   pnet_t * net,
   int loc_port_num,
   pf_snmp_management_address_t * p_man_address)
{
   pf_lldp_management_address_t man_address;
   int error;

   error =
      pf_lldp_get_peer_management_address (net, loc_port_num, &man_address);
   if (error == 0)
   {
      pf_snmp_encode_management_address (p_man_address, &man_address);
   }

   return error;
}

void pf_snmp_get_management_port_index (
   pnet_t * net,
   pf_lldp_interface_number_t * p_man_port_index)
{
   pf_lldp_management_address_t man_address;

   pf_lldp_get_management_address (net, &man_address);
   *p_man_port_index = man_address.interface_number;
}

int pf_snmp_get_peer_management_port_index (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_interface_number_t * p_man_port_index)
{
   int error;
   pf_lldp_management_address_t man_address;

   error =
      pf_lldp_get_peer_management_address (net, loc_port_num, &man_address);
   if (error == 0)
   {
      *p_man_port_index = man_address.interface_number;
   }

   return error;
}

void pf_snmp_get_station_name (
   pnet_t * net,
   pf_lldp_station_name_t * p_station_name)
{
   const char * station_name = NULL;

   /* FIXME: Use of pf_cmina_get_station_name() is not thread-safe, as the
    * returned pointer points to non-constant memory shared by multiple threads.
    * Fix this, e.g. using a mutex.
    */
   pf_cmina_get_station_name (net, &station_name);
   CC_ASSERT (station_name != NULL);

   snprintf (
      p_station_name->string,
      sizeof (p_station_name->string),
      "%s",
      station_name);
   p_station_name->len = strlen (p_station_name->string);
}

int pf_snmp_get_peer_station_name (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_station_name_t * p_station_name)
{
   return pf_lldp_get_peer_station_name (net, loc_port_num, p_station_name);
}

void pf_snmp_get_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_snmp_signal_delay_t * p_delays)
{
   pf_lldp_signal_delay_t delays;

   pf_lldp_get_signal_delays (net, loc_port_num, &delays);
   pf_snmp_encode_signal_delays (p_delays, &delays);
}

int pf_snmp_get_peer_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_snmp_signal_delay_t * p_delays)
{
   pf_lldp_signal_delay_t delays;
   int error;

   error = pf_lldp_get_peer_signal_delays (net, loc_port_num, &delays);
   if (error == 0)
   {
      pf_snmp_encode_signal_delays (p_delays, &delays);
   }

   return error;
}

void pf_snmp_get_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_snmp_link_status_t * p_link_status)
{
   pf_lldp_link_status_t link_status;

   pf_lldp_get_link_status (net, loc_port_num, &link_status);
   pf_snmp_encode_link_status (p_link_status, &link_status);
}

int pf_snmp_get_peer_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_snmp_link_status_t * p_link_status)
{
   pf_lldp_link_status_t link_status;
   int error;

   error = pf_lldp_get_peer_link_status (net, loc_port_num, &link_status);
   if (error == 0)
   {
      pf_snmp_encode_link_status (p_link_status, &link_status);
   }

   return error;
}
