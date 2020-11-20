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

#ifdef UNIT_TEST
#endif

/**
 * @file
 * @brief Helper functions for use by platform-dependent SNMP server (agent).
 *
 */

#include "pf_includes.h"
#include <string.h>

#define STRINGIFY(s)   STRINGIFIED (s)
#define STRINGIFIED(s) #s

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
      PNET_FILENAME_SYSCONTACT,
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
      PNET_FILENAME_SYSCONTACT,
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
   snprintf (
      location->string,
      sizeof (location->string),
      "%s",
      net->fspm_cfg.im_1_data.im_tag_location);
}

int pf_snmp_set_system_location (
   pnet_t * net,
   const pf_snmp_system_location_t * location)
{
   /* TODO: Write to I&M1 */
   return -1;
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
   pf_lldp_get_port_list (net, p_list);
}

int pf_snmp_get_first_port (const pf_lldp_port_list_t * p_list)
{
   return pf_lldp_get_first_port (p_list);
}

int pf_snmp_get_next_port (const pf_lldp_port_list_t * p_list, int loc_port_num)
{
   return pf_lldp_get_next_port (p_list, loc_port_num);
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
   pf_lldp_management_address_t * p_man_address)
{
   pf_lldp_get_management_address (net, p_man_address);
}

int pf_snmp_get_peer_management_address (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_management_address_t * p_man_address)
{
   return pf_lldp_get_peer_management_address (net, loc_port_num, p_man_address);
}

void pf_snmp_get_management_port_index (
   pnet_t * net,
   pf_lldp_management_port_index_t * p_man_port_index)
{
   pf_lldp_get_management_port_index (net, p_man_port_index);
}

int pf_snmp_get_peer_management_port_index (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_management_port_index_t * p_man_port_index)
{
   return pf_lldp_get_peer_management_port_index (
      net,
      loc_port_num,
      p_man_port_index);
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
   pf_lldp_signal_delay_t * p_delays)
{
   pf_lldp_get_signal_delays (net, loc_port_num, p_delays);
}

int pf_snmp_get_peer_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_signal_delay_t * p_delays)
{
   return pf_lldp_get_peer_signal_delays (net, loc_port_num, p_delays);
}

void pf_snmp_get_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status)
{
   pf_lldp_get_link_status (net, loc_port_num, p_link_status);
}

int pf_snmp_get_peer_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status)
{
   return pf_lldp_get_peer_link_status (net, loc_port_num, p_link_status);
}
