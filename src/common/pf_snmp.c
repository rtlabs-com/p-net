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

   CC_STATIC_ASSERT (sizeof (name->string) >= OS_HOST_NAME_MAX);

   error = os_get_hostname (name->string);
   if (error)
   {
      name->string[0] = '\0';
   }
   name->string[sizeof (name->string) - 1] = '\0';
}

int pf_snmp_set_system_name (pnet_t * net, const pf_snmp_system_name_t * name)
{
   /* TODO: Set new hostname permanently. Perhaps call os_set_ip_suite()? */
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
      (void *)contact, /* TODO: Remove cast after adding 'const' to parameter
                        */
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
   /* TODO: Implement support for multiple ports */
   memset (p_list, 0, sizeof (*p_list));
   p_list->ports[0] = BIT (8 - 1);
}

int pf_snmp_get_first_port (const pf_lldp_port_list_t * p_list)
{
   /* TODO: Implement support for multiple ports */
   return 1;
}

int pf_snmp_get_next_port (const pf_lldp_port_list_t * p_list, int loc_port_num)
{
   /* TODO: Implement support for multiple ports */
   return 0;
}

int pf_snmp_get_peer_timestamp (
   pnet_t * net,
   int loc_port_num,
   uint32_t * timestamp_10ms)
{
   /* TODO: Implement support for multiple ports */
   return pf_lldp_is_peer_info_received (net, timestamp_10ms) ? 0 : -1;
}

void pf_snmp_get_chassis_id (pnet_t * net, pf_lldp_chassis_id_t * p_chassis_id)
{
   /* TODO: Implement this */
   memset (p_chassis_id, 0, sizeof (*p_chassis_id));
   p_chassis_id->subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
   snprintf (
      p_chassis_id->string,
      sizeof (p_chassis_id->string),
      "my chassis-id");
   p_chassis_id->len = strlen (p_chassis_id->string);
}

int pf_snmp_get_peer_chassis_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_chassis_id_t * p_chassis_id)
{
   uint32_t timestamp_10ms;

   /* TODO: Implement this */
   CC_ASSERT (loc_port_num == 1);
   memset (p_chassis_id, 0, sizeof (*p_chassis_id));
   p_chassis_id->subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
   snprintf (
      p_chassis_id->string,
      sizeof (p_chassis_id->string),
      "peer chassis-id");
   p_chassis_id->len = strlen (p_chassis_id->string);

   return pf_lldp_is_peer_info_received (net, &timestamp_10ms) ? 0 : -1;
}

void pf_snmp_get_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id)
{
   /* TODO: Implement this */
   CC_ASSERT (loc_port_num == 1);
   memset (p_port_id, 0, sizeof (*p_port_id));
   p_port_id->subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
   snprintf (p_port_id->string, sizeof (p_port_id->string), "my port-id");
   p_port_id->len = strlen (p_port_id->string);
}

int pf_snmp_get_peer_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id)
{
   uint32_t timestamp_10ms;

   /* TODO: Implement this */
   CC_ASSERT (loc_port_num == 1);
   memset (p_port_id, 0, sizeof (*p_port_id));
   p_port_id->subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
   snprintf (p_port_id->string, sizeof (p_port_id->string), "peer port-id");
   p_port_id->len = strlen (p_port_id->string);

   return pf_lldp_is_peer_info_received (net, &timestamp_10ms) ? 0 : -1;
}

void pf_snmp_get_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_descr)
{
   /* TODO: Implement support for multiple ports */
   CC_ASSERT (loc_port_num == 1);
   snprintf (
      p_port_descr->string,
      sizeof (p_port_descr->string),
      "%s",
      net->interface_name);
   p_port_descr->len = strlen (p_port_descr->string);
}

int pf_snmp_get_peer_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_desc)
{
   uint32_t timestamp_10ms;

   /* TODO: Implement this */
   CC_ASSERT (loc_port_num == 1);
   memset (p_port_desc, 0, sizeof (*p_port_desc));
   snprintf (
      p_port_desc->string,
      sizeof (p_port_desc->string),
      "peer port descr");
   p_port_desc->len = strlen (p_port_desc->string);

   return pf_lldp_is_peer_info_received (net, &timestamp_10ms) ? 0 : -1;
}

void pf_snmp_get_management_address (
   pnet_t * net,
   pf_lldp_management_address_t * p_man_address)
{
   /* TODO: Implement this */
   memset (p_man_address, 0, sizeof (*p_man_address));
   p_man_address->subtype = 1;
   memset (p_man_address, 42, 4);
   p_man_address->len = 4;
}

int pf_snmp_get_peer_management_address (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_management_address_t * p_man_address)
{
   uint32_t timestamp_10ms;

   /* TODO: Implement this */
   CC_ASSERT (loc_port_num == 1);
   memset (p_man_address, 0, sizeof (*p_man_address));
   p_man_address->subtype = 1;
   memset (p_man_address, 42, 4);
   p_man_address->len = 4;

   return pf_lldp_is_peer_info_received (net, &timestamp_10ms) ? 0 : -1;
}

void pf_snmp_get_management_port_index (
   pnet_t * net,
   pf_lldp_management_port_index_t * p_man_port_index)
{
   /* TODO: Implement this */
   memset (p_man_port_index, 0, sizeof (*p_man_port_index));
   p_man_port_index->subtype = 2;
   p_man_port_index->index = 1;
}

int pf_snmp_get_peer_management_port_index (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_management_port_index_t * p_man_port_index)
{
   uint32_t timestamp_10ms;

   /* TODO: Implement this */
   CC_ASSERT (loc_port_num == 1);
   memset (p_man_port_index, 0, sizeof (*p_man_port_index));
   p_man_port_index->subtype = 2;
   p_man_port_index->index = 1;

   return pf_lldp_is_peer_info_received (net, &timestamp_10ms) ? 0 : -1;
}

void pf_snmp_get_station_name (
   pnet_t * net,
   pf_lldp_station_name_t * p_station_name)
{
   /* TODO: Implement this */
   memset (p_station_name, 0, sizeof (*p_station_name));
   snprintf (
      p_station_name->string,
      sizeof (p_station_name->string),
      "my station-name");
   p_station_name->len = strlen (p_station_name->string);
}

int pf_snmp_get_peer_station_name (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_station_name_t * p_station_name)
{
   uint32_t timestamp_10ms;

   /* TODO: Implement this */
   CC_ASSERT (loc_port_num == 1);
   memset (p_station_name, 0, sizeof (*p_station_name));
   snprintf (
      p_station_name->string,
      sizeof (p_station_name->string),
      "peer station-name");
   p_station_name->len = strlen (p_station_name->string);

   return pf_lldp_is_peer_info_received (net, &timestamp_10ms) ? 0 : -1;
}

void pf_snmp_get_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_signal_delay_t * p_delays)
{
   /* TODO: Implement this */
   CC_ASSERT (loc_port_num == 1);
   memset (p_delays, 0, sizeof (*p_delays));
}

int pf_snmp_get_peer_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_signal_delay_t * p_delays)
{
   uint32_t timestamp_10ms;

   /* TODO: Implement this */
   CC_ASSERT (loc_port_num == 1);
   memset (p_delays, 0, sizeof (*p_delays));

   return pf_lldp_is_peer_info_received (net, &timestamp_10ms) ? 0 : -1;
}

void pf_snmp_get_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status)
{
   /* TODO: Implement this */
   CC_ASSERT (loc_port_num == 1);
   memset (p_link_status, 0, sizeof (*p_link_status));
}

int pf_snmp_get_peer_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status)
{
   uint32_t timestamp_10ms;

   /* TODO: Implement this */
   CC_ASSERT (loc_port_num == 1);
   memset (p_link_status, 0, sizeof (*p_link_status));

   return pf_lldp_is_peer_info_received (net, &timestamp_10ms) ? 0 : -1;
}
