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
 * Some SNMP related data needs to be stored to file between invocations.
 * We read from file only at startup, but writing to file might happen any time.
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
#define pnal_get_hostname            mock_pnal_get_hostname
#define pf_bg_worker_start_job       mock_pf_bg_worker_start_job
#define pnal_snmp_configure          mock_pnal_snmp_configure
#endif

#include "pf_includes.h"
#include "pf_block_reader.h"
#include "pf_block_writer.h"
#include <string.h>

#define STRINGIFY(s)   STRINGIFIED (s)
#define STRINGIFIED(s) #s

const pf_snmp_config_t snmp_config_default = {
   .control_mode = PNAL_SNMP_CONTROL_MODE_READWRITE,
   .ro_name = {.string = "public"},
   .rw_name = {.string = "private"},
   .snmp_adjust_active = false};

/* The configurable constant PNET_MAX_FILENAME_SIZE should be at least
 * as large as the longest filename used, including termination.
 */
CC_STATIC_ASSERT (
   PNET_MAX_FILENAME_SIZE >= sizeof (PF_FILENAME_SNMP_SYSCONTACT));
CC_STATIC_ASSERT (PNET_MAX_FILENAME_SIZE >= sizeof (PF_FILENAME_SNMP_SYSNAME));
CC_STATIC_ASSERT (
   PNET_MAX_FILENAME_SIZE >= sizeof (PF_FILENAME_SNMP_SYSLOCATION));

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
         "SNMP(%d): Could not yet read %s from nvm. Using \"%s\"\n",
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

void pf_snmp_data_init (pnet_t * net)
{
   const char * directory = pf_cmina_get_file_directory (net);
   pf_snmp_data_t * snmp = &net->snmp_data;
   int error;
   int result;

   CC_STATIC_ASSERT (
      sizeof (snmp->system_name.string) >= PNAL_HOSTNAME_MAX_SIZE);

   net->snmp_mutex = os_mutex_create();

   /* sysContact */
   error = pf_file_load (
      directory,
      PF_FILENAME_SNMP_SYSCONTACT,
      &snmp->system_contact,
      sizeof (snmp->system_contact));
   if (error)
   {
      snmp->system_contact.string[0] = '\0';
   }
   snmp->system_contact.string[sizeof (snmp->system_contact.string) - 1] = '\0';
   pf_snmp_log_loaded_variable (
      error,
      "sysContact",
      snmp->system_contact.string);

   /* sysName */
   error = pf_file_load (
      directory,
      PF_FILENAME_SNMP_SYSNAME,
      &snmp->system_name,
      sizeof (snmp->system_name));
   if (error)
   {
      result = pnal_get_hostname (snmp->system_name.string);
      if (result != 0)
      {
         snmp->system_name.string[0] = '\0';
      }
   }
   snmp->system_name.string[sizeof (snmp->system_name.string) - 1] = '\0';
   pf_snmp_log_loaded_variable (error, "sysName", snmp->system_name.string);

   /* sysLocation */
   error = pf_file_load (
      directory,
      PF_FILENAME_SNMP_SYSLOCATION,
      &snmp->system_location,
      sizeof (snmp->system_location));
   if (error)
   {
      snmp->system_location.string[0] = '\0';
   }
   snmp->system_location.string[sizeof (snmp->system_location.string) - 1] =
      '\0';
   pf_snmp_log_loaded_variable (
      error,
      "sysLocation",
      snmp->system_location.string);

#if PNET_OPTION_SECURITY_LEVEL_1
   error = pf_file_load (
      directory,
      PF_FILENAME_SNMP_CONFIG,
      &net->snmp_config,
      sizeof (net->snmp_config));
   if (error)
   {
      net->snmp_config = snmp_config_default;
   }
#endif /* PNET_OPTION_SECURITY_LEVEL_1 */
}

void pf_snmp_remove_data_files (const char * file_directory)
{
   pf_file_clear (file_directory, PF_FILENAME_SNMP_SYSCONTACT);
   pf_file_clear (file_directory, PF_FILENAME_SNMP_SYSNAME);
   pf_file_clear (file_directory, PF_FILENAME_SNMP_SYSLOCATION);
#if PNET_OPTION_SECURITY_LEVEL_1
   pf_file_clear (file_directory, PF_FILENAME_SNMP_CONFIG);
#endif /* PNET_OPTION_SECURITY_LEVEL_1 */
}

void pf_snmp_data_clear (pnet_t * net)
{
   const char * p_file_directory = pf_cmina_get_file_directory (net);
   pf_snmp_data_t * snmp = &net->snmp_data;

   LOG_DEBUG (PF_SNMP_LOG, "SNMP(%d): Clearing SNMP data.\n", __LINE__);
   pf_snmp_remove_data_files (p_file_directory);

   os_mutex_lock (net->snmp_mutex);

   memset (
      snmp->system_contact.string,
      '\0',
      sizeof (snmp->system_contact.string));
   memset (snmp->system_name.string, '\0', sizeof (snmp->system_name.string));
   memset (
      snmp->system_location.string,
      '\0',
      sizeof (snmp->system_location.string));

   os_mutex_unlock (net->snmp_mutex);
}

void pf_snmp_data_reset (pnet_t * net)
{
   pf_snmp_system_contact_t syscontact = {0};
   pf_snmp_system_name_t sysname = {0};
   pf_snmp_system_location_t syslocation = {0};

   LOG_DEBUG (PF_SNMP_LOG, "SNMP(%d): Resetting SNMP data.\n", __LINE__);
   pf_snmp_set_system_contact (net, &syscontact);
   pf_snmp_set_system_name (net, &sysname);
   pf_snmp_set_system_location (net, &syslocation);
}

void pf_snmp_get_system_name (pnet_t * net, pf_snmp_system_name_t * name)
{
   const pf_snmp_data_t * snmp = &net->snmp_data;

   os_mutex_lock (net->snmp_mutex);
   snprintf (
      name->string,
      sizeof (name->string),
      "%s",
      snmp->system_name.string);
   os_mutex_unlock (net->snmp_mutex);
}

int pf_snmp_set_system_name (pnet_t * net, const pf_snmp_system_name_t * name)
{
   pf_snmp_data_t * snmp = &net->snmp_data;

   /* Ideally, we should set the Fully Qualified Domain Name here. However, not
    * all systems are expected to support doing that, so we don't.
    * Instead, we just save the sysName to file as to ensure it is persistent
    * across restarts.
    */
   os_mutex_lock (net->snmp_mutex);
   memcpy (
      snmp->system_name.string,
      name->string,
      sizeof (snmp->system_name.string) - 1);
   snmp->system_name.string[sizeof (snmp->system_name.string) - 1] = '\0';
   os_mutex_unlock (net->snmp_mutex);

   pf_bg_worker_start_job (net, PF_BGJOB_SAVE_SNMP_SYSTEM_NAME_NVM_DATA);

   return 0;
}

void pf_snmp_save_system_name (pnet_t * net)
{
   int res;
   pf_snmp_system_name_t temporary_buffer;
   pf_snmp_system_name_t system_name;
   pf_snmp_data_t * snmp = &net->snmp_data;
   const char * directory = pf_cmina_get_file_directory (net);

   os_mutex_lock (net->snmp_mutex);
   system_name = snmp->system_name;
   os_mutex_unlock (net->snmp_mutex);

   res = pf_file_save_if_modified (
      directory,
      PF_FILENAME_SNMP_SYSNAME,
      &system_name,
      &temporary_buffer,
      sizeof (system_name));

   pf_snmp_log_saved_variable (res, "sysName", system_name.string);
}

void pf_snmp_get_system_contact (
   pnet_t * net,
   pf_snmp_system_contact_t * contact)
{
   const pf_snmp_data_t * snmp = &net->snmp_data;
   os_mutex_lock (net->snmp_mutex);
   snprintf (
      contact->string,
      sizeof (contact->string),
      "%s",
      snmp->system_contact.string);
   os_mutex_unlock (net->snmp_mutex);
}

int pf_snmp_set_system_contact (
   pnet_t * net,
   const pf_snmp_system_contact_t * contact)
{
   pf_snmp_data_t * snmp = &net->snmp_data;

   os_mutex_lock (net->snmp_mutex);
   snprintf (
      snmp->system_contact.string,
      sizeof (snmp->system_contact.string),
      "%s",
      contact->string);
   os_mutex_unlock (net->snmp_mutex);

   pf_bg_worker_start_job (net, PF_BGJOB_SAVE_SNMP_SYSTEM_CONTACT_NVM_DATA);

   return 0;
}

void pf_snmp_save_system_contact (pnet_t * net)
{
   int res;
   pf_snmp_system_contact_t temporary_buffer;
   pf_snmp_system_contact_t system_contact;
   pf_snmp_data_t * snmp = &net->snmp_data;
   const char * directory = pf_cmina_get_file_directory (net);

   os_mutex_lock (net->snmp_mutex);
   system_contact = snmp->system_contact;
   os_mutex_unlock (net->snmp_mutex);

   res = pf_file_save_if_modified (
      directory,
      PF_FILENAME_SNMP_SYSCONTACT,
      &system_contact,
      &temporary_buffer,
      sizeof (system_contact));

   pf_snmp_log_saved_variable (res, "sysContact", system_contact.string);
}

void pf_snmp_get_system_location (
   pnet_t * net,
   pf_snmp_system_location_t * location)
{
   const pf_snmp_data_t * snmp = &net->snmp_data;

   os_mutex_lock (net->snmp_mutex);
   snprintf (
      location->string,
      sizeof (location->string),
      "%s",
      snmp->system_location.string);
   os_mutex_unlock (net->snmp_mutex);
}

int pf_snmp_set_system_location (
   pnet_t * net,
   const pf_snmp_system_location_t * location)
{
   pf_snmp_data_t * snmp = &net->snmp_data;

   os_mutex_lock (net->snmp_mutex);
   snprintf (
      snmp->system_location.string,
      sizeof (snmp->system_location.string),
      "%s",
      location->string);
   os_mutex_unlock (net->snmp_mutex);

   pf_bg_worker_start_job (net, PF_BGJOB_SAVE_SNMP_SYSTEM_LOCATION_NVM_DATA);

   return 0;
}

void pf_snmp_save_system_location (pnet_t * net)
{
   int res;
   pf_snmp_system_location_t temporary_buffer;
   pf_snmp_system_location_t system_location;
   pf_snmp_data_t * snmp = &net->snmp_data;
   const char * directory = pf_cmina_get_file_directory (net);

   os_mutex_lock (net->snmp_mutex);
   system_location = snmp->system_location;
   os_mutex_unlock (net->snmp_mutex);

   res = pf_file_save_if_modified (
      directory,
      PF_FILENAME_SNMP_SYSLOCATION,
      &system_location,
      &temporary_buffer,
      sizeof (system_location));

   pf_snmp_log_saved_variable (res, "sysLocation", system_location.string);
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

/* TODO: Move this code to some other file.
 * See file header above for description of what belongs in this file.
 */
#if PNET_OPTION_SECURITY_LEVEL_1
void pf_snmp_save_config (pnet_t * net)
{
   int res;
   pf_snmp_config_t temporary_buffer;
   pf_snmp_config_t snmp_config;
   const char * directory = pf_cmina_get_file_directory (net);

   os_mutex_lock (net->snmp_mutex);
   snmp_config = net->snmp_config;
   os_mutex_unlock (net->snmp_mutex);

   res = pf_file_save_if_modified (
      directory,
      PF_FILENAME_SNMP_CONFIG,
      &snmp_config,
      &temporary_buffer,
      sizeof (snmp_config));

   pf_snmp_log_saved_variable (res, "SNMP Config", "N/A");
}

int pf_snmp_save_snmp_config_bg (pnet_t * net)
{
   LOG_DEBUG (
      PF_SNMP_LOG,
      "SNMP(%d): Requesting background job to save SNMP configuration.\n",
      __LINE__);
   pf_bg_worker_start_job (net, PF_BGJOB_SAVE_SNMP_CONFIG);
   return 0;
}

int pf_snmp_activate_snmp_config (pnet_t * net)
{
   return pnal_snmp_configure (
      net->snmp_config.control_mode,
      net->snmp_config.ro_name.string,
      net->snmp_config.rw_name.string);
}

int pf_snmp_activate_snmp_config_bg (pnet_t * net)
{
   LOG_DEBUG (
      PF_SNMP_LOG,
      "SNMP(%d): Requesting background job to apply SNMP configuration.\n",
      __LINE__);

   pf_bg_worker_start_job (net, PF_BGJOB_ACTIVATE_SNMP_CONFIG);
   return 0;
}

int pf_snmp_set_snmp_config (
   pnet_t * net,
   const pf_snmp_config_t * p_snmp_config,
   bool * configuration_changed)
{
   *configuration_changed = false;

   LOG_INFO (
      PF_SNMP_LOG,
      "SNMP(%d): Setting new SNMP config. mode: %d, ro: \"%s\" "
      "rw: \"%s\" adjust_active: %d\n",
      __LINE__,
      p_snmp_config->control_mode,
      p_snmp_config->ro_name.string,
      p_snmp_config->rw_name.string,
      p_snmp_config->snmp_adjust_active);

   os_mutex_lock (net->snmp_mutex);

   if (net->snmp_config.control_mode != p_snmp_config->control_mode)
   {
      net->snmp_config.control_mode = p_snmp_config->control_mode;
      *configuration_changed = true;
   }

   if (
      strnlen (
         p_snmp_config->ro_name.string,
         sizeof (p_snmp_config->ro_name.string)) != 0 ||
      strncmp (
         net->snmp_config.ro_name.string,
         p_snmp_config->ro_name.string,
         sizeof (net->snmp_config.ro_name.string)) != 0)
   {
      snprintf (
         net->snmp_config.ro_name.string,
         sizeof (net->snmp_config.ro_name.string),
         "%s",
         p_snmp_config->ro_name.string);
      *configuration_changed = true;
   }
   else
   {
      /* Keep existing ro community name*/
   }

   if (
      strnlen (
         p_snmp_config->rw_name.string,
         sizeof (p_snmp_config->rw_name.string)) != 0 ||
      strncmp (
         net->snmp_config.rw_name.string,
         p_snmp_config->rw_name.string,
         sizeof (net->snmp_config.rw_name.string)) != 0)
   {
      snprintf (
         net->snmp_config.rw_name.string,
         sizeof (net->snmp_config.rw_name.string),
         "%s",
         p_snmp_config->rw_name.string);

      *configuration_changed = true;
   }
   else
   {
      /* Keep existing rw community name*/
   }

   if (p_snmp_config->snmp_adjust_active != net->snmp_config.snmp_adjust_active)
   {
      net->snmp_config.snmp_adjust_active = p_snmp_config->snmp_adjust_active;
      *configuration_changed = true;
   }

   os_mutex_unlock (net->snmp_mutex);

   return 0;
}

/**
 * Validate community name. Rules:
 * Total length is 0 to 240 octets
 * CommunityName consist of [a-zA-Z0-9.-]
 * CommunityName do not start with [.-]
 * CommunityName do not end with [.-]
 *
 * @param p_name          In:    The community name to validate
 * @return  0 if valid, -1 if invalid
 */
int pf_snmp_validate_community_name (const char * p_name)
{
   size_t len;

   if (p_name == NULL)
   {
      return -1; /* Null pointer is invalid */
   }

   len = strnlen (p_name, sizeof (pf_snmp_community_name_t));

   if (len == 0)
   {
      return 0; /* Zero length is allowed (means keep existing name) */
   }

   if (len > 240)
   {
      return -1; /* Invalid length */
   }

   /* Check first character */
   if (p_name[0] == '.' || p_name[0] == '-')
   {
      return -1;
   }

   /* Check last character */
   if (p_name[len - 1] == '.' || p_name[len - 1] == '-')
   {
      return -1;
   }

   /* Check allowed characters */
   for (size_t i = 0; i < len; i++)
   {
      char c = p_name[i];
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '.' || c == '-'))
      {
         return -1; /* Invalid character */
      }
   }

   return 0;
}

/**
 * Validate SNMP configuration
 *
 * @param p_snmp_config  In:    The SNMP configuration to validate
 * @return  0 if valid, -1 if invalid
 */
int pf_snmp_validate_config(const pf_snmp_config_t * p_snmp_config)
{
   int status = 0;

   /* Check rules for community names */
   switch (p_snmp_config->control_mode)
   {
   case PNAL_SNMP_CONTROL_MODE_DISABLED:
      /* For PNAL_SNMP_CONTROL_MODE_DISABLED, the community name lengths shall
       * be set to 0 */
      if (
         strnlen (
            p_snmp_config->ro_name.string,
            sizeof (p_snmp_config->ro_name.string)) != 0 ||
         strnlen (
            p_snmp_config->rw_name.string,
            sizeof (p_snmp_config->rw_name.string)) != 0)
      {
         status = -1;
      }
      break;
   case PNAL_SNMP_CONTROL_MODE_READONLY:
      /* For PNAL_SNMP_CONTROL_MODE_READONLY, the rw community name shall not be
       * set
       */
      if (
         strnlen (
            p_snmp_config->rw_name.string,
            sizeof (p_snmp_config->rw_name.string)) != 0 ||
         pf_snmp_validate_community_name (p_snmp_config->ro_name.string) != 0)
      {
         status = -1;
      }
      break;
   case PNAL_SNMP_CONTROL_MODE_READWRITE:
      if (
         pf_snmp_validate_community_name (p_snmp_config->ro_name.string) != 0 ||
         pf_snmp_validate_community_name (p_snmp_config->rw_name.string) != 0)
      {
         status = -1;
      }
      break;
   default:
      status = -1;
      break;
   }

   return status;
}

/**
 * Handle a write request to CIM SNMP Adjust.
 * Get configuration from request, validate it, and then
 * configure SNMP accordingly.
 *
 * @param net            InOut: The p-net stack instance
 * @param p_write_req    In:    The write request details
 * @param p_bytes        In:    The write request data
 * @param data_length    In:    The length of the write request data
 * @param p_result       Out:   The result to return to the caller
 * @return  0 on success, -1 on error
 */
int pf_cim_snmp_adjust (
   pnet_t * net,
   const pf_iod_write_request_t * p_write_req,
   const uint8_t * p_bytes,
   uint16_t data_length,
   pnet_result_t * p_result)
{
   int status = 0;
   uint16_t pos = 0;
   pf_snmp_config_t snmp_config = {0};
   bool snmp_configuration_changed = false;

   pf_get_info_t get_info =
   {
      .result = PF_PARSE_OK,
      .p_frame = NULL,
      .p_array = p_bytes,
      .is_big_endian = true,
      .len = data_length,
   };

   /* block header already consumed at this point */

   pf_get_cim_snmp_adjust (&get_info, &pos, &snmp_config);

   if (get_info.result != PF_PARSE_OK)
   {
      LOG_WARNING (
         PF_SNMP_LOG,
         "SNMP(%d): Failed to parse CIM SNMP Adjust\n",
         __LINE__);
      return -1;
   }

   status = pf_snmp_validate_config(&snmp_config);

   if (status == 0)
   {
      LOG_INFO (
         PF_SNMP_LOG,
         "SNMP(%d): Write request CIM SNMP Adjust "
         "mode = %d, ro = \"%s\", rw = \"%s\"\n",
         __LINE__,
         snmp_config.control_mode,
         snmp_config.ro_name.string,
         snmp_config.rw_name.string);

      snmp_config.snmp_adjust_active = true;

      status = pf_snmp_set_snmp_config (
         net,
         &snmp_config,
         &snmp_configuration_changed);

      if (status == 0 && snmp_configuration_changed)
      {
         status = pf_snmp_activate_snmp_config (net);
      }
   }

   if (status == 0)
   {
      pf_snmp_save_snmp_config_bg (net);
   }
   else
   {
      LOG_WARNING (
         PF_SNMP_LOG,
         "SNMP(%d): Write request CIM SNMP Adjust rejected\n",
         __LINE__);
   }

   return status;
}

void pf_snmp_config_reset (pnet_t * net)
{
   bool snmp_configuration_changed;

   LOG_DEBUG (PF_SNMP_LOG, "SNMP(%d): Resetting SNMP config.\n", __LINE__);

   pf_snmp_set_snmp_config (
      net,
      &snmp_config_default,
      &snmp_configuration_changed);
   if (snmp_configuration_changed)
   {
      pf_snmp_save_snmp_config_bg (net);
      pf_snmp_activate_snmp_config_bg (net);
   }
}

const pf_snmp_config_t * pf_snmp_get_config (pnet_t * net)
{
   return &net->snmp_config;
}
#endif /* PNET_OPTION_SECURITY_LEVEL_1 */

#if PNET_OPTION_MRP
void pf_snmp_get_mrp_domain_uuid(pnet_t * net, mrp_domain_uuid_t *domain)
{
   memcpy (
      domain->data,
      net->mrp_data.config.domain.data,
      sizeof (mrp_domain_uuid_t));
}

void pf_snmp_get_mrp_domain_name(pnet_t * net, mrp_domain_name_t *domain_name)
{
   domain_name->len = net->mrp_data.config.domain_name.len;
   memcpy(domain_name->name, net->mrp_data.config.domain_name.name, domain_name->len);
}

void pf_snmp_get_mrp_domain_role(pnet_t * net, int *role)
{
   *role = (int)net->mrp_data.config.role;
}

int pf_snmp_set_mrp_domain_role(pnet_t * net, int role)
{
   if (role == MEDIA_REDUNDANCY_DISABLED)
   {
      pf_mrp_disable(net);
      return 0;
   }
   if (role == MEDIA_REDUNDANCY_CLIENT)
   {
      pf_mrp_enable(net);
      return 0;
   }
   return -1;
}

int pf_snmp_mrp_get_port_info(pnet_t *pnet, int ring_id, int *port_idx, int *ring_port_state)
{
   int port;
   pf_port_iterator_t port_iterator;
   int i = ring_id;

   /* only allow queries for ring port 1 and 2 */
   if ((ring_id < 1) || (ring_id > 2))
      return -1;

   pf_port_init_iterator_over_ports (pnet, &port_iterator);

   while (i--)
   {
      port = pf_port_get_next (&port_iterator);
   }

   if (port > 0)
   {
      if (port_idx)
      {
         *port_idx = port;
      }
      if (ring_port_state)
      {
         pf_port_t * p_port_data = pf_port_get_state (pnet, port);
         mrp_ring_port_state_t state;
         pnal_mrp_get_ring_port_status(p_port_data->netif.name, port, &state);
         *ring_port_state = state;
      }
   }

   return port;
}

int pf_snmp_mrp_get_vlan_id(pnet_t *pnet, int *vlan_id)
{
   /* return vlan id for first port */
   int port;
   pf_port_iterator_t port_iterator;
   pf_port_init_iterator_over_ports (pnet, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   if (port > 0)
   {
      pf_port_t * p_port_data = pf_port_get_state (pnet, port);
      return pnal_mrp_get_vlan_id(p_port_data->netif.name, port, vlan_id);
   }
   *vlan_id = 0;
   return -1;
}
#endif

void pf_snmp_show (pnet_t * net)
{
   pf_snmp_data_t * snmp = &net->snmp_data;

   printf ("\nSNMP\n");
   printf (" SysName     : \"%s\"\n", snmp->system_name.string);
   printf (" SysLocation : \"%s\"\n", snmp->system_location.string);
   printf (" SysContact  : \"%s\"\n", snmp->system_contact.string);
#if PNET_OPTION_SECURITY_LEVEL_1
   printf (" ControlMode : %d\n", net->snmp_config.control_mode);
   printf (" RO Name     : \"%s\"\n", net->snmp_config.ro_name.string);
   printf (" RW Name     : \"%s\"\n", net->snmp_config.rw_name.string);
#endif /* PNET_OPTION_SECURITY_LEVEL_1 */
}
