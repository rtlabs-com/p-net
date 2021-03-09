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
#define pf_file_load                 mock_pf_file_load
#define pf_file_save_if_modified     mock_pf_file_save_if_modified
#define pf_fspm_get_im_location      mock_pf_fspm_get_im_location
#define pf_fspm_save_im_location     mock_pf_fspm_save_im_location
#endif

#include "pf_includes.h"
#include <string.h>

#define STRINGIFY(s)   STRINGIFIED (s)
#define STRINGIFIED(s) #s

/**
 * Log result of variable load operation
 *
 * @param error            In:    Error code returned from pf_file_load().
 * @param variable_name    In:    Name of variable loaded from file.
 * @param variable_value   In:    Value of variable loaded from file, in
 *                                case no error was detected.
 */
static void pf_snmp_log_loaded_variable (
   int error,
   const char * variable_name,
   const char * variable_value)
{
   if (error)
   {
      LOG_INFO (
         PF_SNMP_LOG,
         "SNMP(%d): Could not yet read %s from nvm. Using '%s'\n",
         __LINE__,
         variable_name,
         variable_value);
   }
   else
   {
      LOG_INFO (
         PF_SNMP_LOG,
         "SNMP(%d): Did read %s from nvm. %s: %s\n",
         __LINE__,
         variable_name,
         variable_name,
         variable_value);
   }
}

/**
 * Log result of variable save operation
 *
 * @param result           In:    Error code returned from
 *                                pf_file_save_if_modified().
 * @param variable_name    In:    Name of variable stored to file.
 * @param variable_value   In:    Value of variable save to file.
 */
static void pf_snmp_log_saved_variable (
   int result,
   const char * variable_name,
   const char * variable_value)
{
   switch (result)
   {
   case 2:
      LOG_INFO (
         PF_SNMP_LOG,
         "SNMP(%d): First nvm saving of %s. %s: %s\n",
         __LINE__,
         variable_name,
         variable_name,
         variable_value);
      break;
   case 1:
      LOG_INFO (
         PF_SNMP_LOG,
         "SNMP(%d): Updating nvm stored %s. %s: %s\n",
         __LINE__,
         variable_name,
         variable_name,
         variable_value);
      break;
   case 0:
      LOG_INFO (
         PF_SNMP_LOG,
         "SNMP(%d): No storing of nvm %s (no changes). %s: %s\n",
         __LINE__,
         variable_name,
         variable_name,
         variable_value);
      break;
   case -1:
      /* Fall-through */
   default:
      LOG_ERROR (
         PF_SNMP_LOG,
         "SNMP(%d): Failed to store nvm %s.\n",
         __LINE__,
         variable_name);
      break;
   }
}

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
   const char * directory = pf_cmina_get_file_directory (net);
   int error;
   int result;

   CC_STATIC_ASSERT (sizeof (name->string) >= PNAL_HOSTNAME_MAX_SIZE);

   error = pf_file_load (directory, PF_FILENAME_SYSNAME, name, sizeof (*name));
   if (error)
   {
      result = pnal_get_hostname (name->string);
      if (result != 0)
      {
         name->string[0] = '\0';
      }
   }
   name->string[sizeof (name->string) - 1] = '\0';
   pf_snmp_log_loaded_variable (error, "sysName", name->string);
}

int pf_snmp_set_system_name (pnet_t * net, const pf_snmp_system_name_t * name)
{
   const char * directory = pf_cmina_get_file_directory (net);
   pf_snmp_system_name_t temporary_buffer;
   int res;

   /* Ideally, we should set the Fully Qualified Domain Name here. However, not
    * all systems are expected to support doing that, so we don't.
    * Instead, we just save the sysName to file as to ensure it is persistent
    * across restarts.
    */
   res = pf_file_save_if_modified (
      directory,
      PF_FILENAME_SYSNAME,
      name,
      &temporary_buffer,
      sizeof (*name));
   pf_snmp_log_saved_variable (res, "sysName", name->string);

   return res == -1 ? -1 : 0;
}

void pf_snmp_get_system_contact (
   pnet_t * net,
   pf_snmp_system_contact_t * contact)
{
   const char * directory = pf_cmina_get_file_directory (net);
   int error;

   error = pf_file_load (
      directory,
      PF_FILENAME_SYSCONTACT,
      contact,
      sizeof (*contact));
   if (error)
   {
      contact->string[0] = '\0';
   }
   contact->string[sizeof (contact->string) - 1] = '\0';
   pf_snmp_log_loaded_variable (error, "sysContact", contact->string);
}

int pf_snmp_set_system_contact (
   pnet_t * net,
   const pf_snmp_system_contact_t * contact)
{
   const char * directory = pf_cmina_get_file_directory (net);
   pf_snmp_system_contact_t temporary_buffer;
   int res;

   res = pf_file_save_if_modified (
      directory,
      PF_FILENAME_SYSCONTACT,
      contact,
      &temporary_buffer,
      sizeof (*contact));
   pf_snmp_log_saved_variable (res, "sysContact", contact->string);

   return res == -1 ? -1 : 0;
}

void pf_snmp_get_system_location (
   pnet_t * net,
   pf_snmp_system_location_t * location)
{
   const char * directory = pf_cmina_get_file_directory (net);
   int error;

   error = pf_file_load (
      directory,
      PF_FILENAME_SYSLOCATION,
      location,
      sizeof (*location));
   if (error)
   {
      /* Use "IM_Tag_Location" from I&M1 */
      pf_fspm_get_im_location (net, location->string);
   }
   location->string[sizeof (location->string) - 1] = '\0';

   pf_snmp_log_loaded_variable (error, "sysLocation", location->string);
}

int pf_snmp_set_system_location (
   pnet_t * net,
   const pf_snmp_system_location_t * location)
{
   const char * directory = pf_cmina_get_file_directory (net);
   pf_snmp_system_location_t temporary_buffer;
   int res;

   res = pf_file_save_if_modified (
      directory,
      PF_FILENAME_SYSLOCATION,
      location,
      &temporary_buffer,
      sizeof (*location));
   pf_snmp_log_saved_variable (res, "sysLocation", location->string);

   /* Update 22 byte "IM_Tag_Location" in I&M1 */
   pf_fspm_save_im_location (net, location->string);

   return res == -1 ? -1 : 0;
}

void pf_snmp_get_system_description (
   pnet_t * net,
   pf_snmp_system_description_t * description)
{
   (void)pf_lldp_get_system_description (
      net,
      description->string,
      sizeof (description->string));
}

void pf_snmp_get_port_list (pnet_t * net, pf_lldp_port_list_t * p_list)
{
   pf_port_get_list_of_ports (net, p_list);
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
   char station_name[PNET_STATION_NAME_MAX_SIZE]; /** Terminated */

   /* FIXME: Use of pf_cmina_get_station_name() is not thread-safe.
    * Fix this, e.g. using a mutex.
    */
   pf_cmina_get_station_name (net, station_name);

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
