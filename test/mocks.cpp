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

// TODO: Must be included first due to redefinition of atomics in
// pf_types.h
#include <gtest/gtest.h>

#include "mocks.h"

#include <string.h>

struct pnal_eth_handle
{
   const char * if_name;
   pnal_eth_callback_t * callback;
   void * arg;
};

uint8_t pnet_log_level;

os_mutex_t * mock_mutex;
mock_os_data_t mock_os_data;
mock_lldp_data_t mock_lldp_data;
mock_file_data_t mock_file_data;
mock_fspm_data_t mock_fspm_data;
pnal_eth_handle_t mock_eth_handle;

void mock_clear (void)
{
   memset (&mock_os_data, 0, sizeof (mock_os_data));
   memset (&mock_lldp_data, 0, sizeof (mock_lldp_data));
   memset (&mock_file_data, 0, sizeof (mock_file_data));
   memset (&mock_fspm_data, 0, sizeof (mock_fspm_data));
   mock_os_data.eth_status[1].operational_mau_type =
      PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX;
   mock_os_data.eth_status[1].running = true;
}

void mock_init (void)
{
   mock_mutex = os_mutex_create();
   mock_clear();
}

uint32_t mock_os_get_current_time_us (void)
{
   return mock_os_data.current_time_us;
}

uint32_t mock_pnal_get_system_uptime_10ms (void)
{
   return mock_os_data.system_uptime_10ms;
}

pnal_eth_handle_t * mock_pnal_eth_init (
   const char * if_name,
   const pnal_cfg_t * pnal_cfg,
   pnal_eth_callback_t * callback,
   void * arg)
{
   pnal_eth_handle_t * handle;

   handle = &mock_eth_handle;

   handle->if_name = if_name;
   handle->arg = arg;
   handle->callback = callback;

   mock_os_data.eth_if_handle = handle;

   return handle;
}

int mock_pnal_eth_get_status (
   const char * interface_name,
   pnal_eth_status_t * status)
{
   /* TODO get loc_port_num from interface_name */
   *status = mock_os_data.eth_status[1];

   return 0;
}

int mock_pnal_get_port_statistics (
   const char * interface_name,
   pnal_port_stats_t * port_stats)
{
   /* TODO get loc_port_num from interface_name */
   *port_stats = mock_os_data.port_statistics[1];

   return 0;
}

int mock_pnal_get_hostname (char * hostname)
{
   hostname[0] = 'A';
   hostname[1] = '\0';

   return 0;
}

int mock_pnal_get_ip_suite (
   const char * interface_name,
   pnal_ipaddr_t * p_ipaddr,
   pnal_ipaddr_t * p_netmask,
   pnal_ipaddr_t * p_gw,
   const char ** p_device_name)
{
   return 0;
}

int mock_pnal_set_ip_suite (
   const char * interface_name,
   const pnal_ipaddr_t * p_ipaddr,
   const pnal_ipaddr_t * p_netmask,
   const pnal_ipaddr_t * p_gw,
   const char * hostname,
   bool permanent)
{
   mock_os_data.set_ip_suite_count++;
   return 0;
}

int mock_pnal_eth_send (pnal_eth_handle_t * handle, pnal_buf_t * p_buf)
{
   memcpy (mock_os_data.eth_send_copy, p_buf->payload, p_buf->len);
   mock_os_data.eth_send_len = p_buf->len;
   mock_os_data.eth_send_count++;

   return p_buf->len;
}

int mock_pnal_get_macaddress (
   const char * interface_name,
   pnal_ethaddr_t * p_mac)
{
   p_mac->addr[0] = 0x12;
   p_mac->addr[1] = 0x34;
   p_mac->addr[2] = 0x00;
   p_mac->addr[3] = 0x78;
   p_mac->addr[4] = 0x90;
   p_mac->addr[5] = 0xab;
   return 0;
}

int mock_pnal_udp_open (pnal_ipaddr_t addr, pnal_ipport_t port)
{
   int ret = 2;
   return ret;
}

int mock_pnal_udp_sendto (
   uint32_t id,
   pnal_ipaddr_t dst_addr,
   pnal_ipport_t dst_port,
   const uint8_t * data,
   int size)
{
   mock_os_data.udp_sendto_len = size;
   mock_os_data.udp_sendto_count++;

   return size;
}

void mock_set_pnal_udp_recvfrom_buffer (uint8_t * p_src, uint16_t len)
{
   os_mutex_lock (mock_mutex);

   memcpy (mock_os_data.udp_recvfrom_buffer, p_src, len);
   mock_os_data.udp_recvfrom_length = len;
   mock_os_data.udp_recvfrom_count++;

   os_mutex_unlock (mock_mutex);
}

int mock_pnal_udp_recvfrom (
   uint32_t id,
   pnal_ipaddr_t * p_dst_addr,
   pnal_ipport_t * p_dst_port,
   uint8_t * data,
   int size)
{
   int len;

   os_mutex_lock (mock_mutex);

   memcpy (
      data,
      mock_os_data.udp_recvfrom_buffer,
      mock_os_data.udp_recvfrom_length);
   len = mock_os_data.udp_recvfrom_length;
   mock_os_data.udp_recvfrom_length = 0;

   os_mutex_unlock (mock_mutex);

   return len;
}

void mock_pnal_udp_close (uint32_t id)
{
}

int mock_pnal_get_interface_index (const char * interface_name)
{
   return mock_os_data.interface_index;
}

int mock_pnal_save_file (
   const char * fullpath,
   const void * object_1,
   size_t size_1,
   const void * object_2,
   size_t size_2)
{
   int pos = 0;

   if (size_1 + size_2 > sizeof (mock_os_data.file_content))
   {
      return -1;
   }
   if (strlen (fullpath) > sizeof (mock_os_data.file_fullpath))
   {
      return -1;
   }

   strcpy (mock_os_data.file_fullpath, fullpath);
   mock_os_data.file_size = size_1 + size_2;

   if (size_1 > 0)
   {
      memcpy (&mock_os_data.file_content[pos], object_1, size_1);
      pos += size_1;
   }
   if (size_2 > 0)
   {
      memcpy (&mock_os_data.file_content[pos], object_2, size_2);
   }

   return 0;
}

void mock_pnal_clear_file (const char * fullpath)
{
   if (strcmp (fullpath, mock_os_data.file_fullpath) == 0)
   {
      strcpy (mock_os_data.file_fullpath, "");
      mock_os_data.file_size = 0;
   }
}

int mock_pnal_load_file (
   const char * fullpath,
   void * object_1,
   size_t size_1,
   void * object_2,
   size_t size_2)
{
   int pos = 0;

   if (size_1 + size_2 > mock_os_data.file_size)
   {
      return -1;
   }

   if (strcmp (fullpath, mock_os_data.file_fullpath) != 0)
   {
      return -1;
   }

   if (size_1 > 0)
   {
      memcpy (object_1, &mock_os_data.file_content[pos], size_1);
      pos += size_1;
   }
   if (size_2 > 0)
   {
      memcpy (object_2, &mock_os_data.file_content[pos], size_2);
   }

   return 0;
}

int mock_pf_alarm_send_diagnosis (
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   pf_diag_item_t * p_item)
{
   return 0;
}

void mock_pf_generate_uuid (
   uint32_t timestamp,
   uint32_t session_number,
   pnet_ethaddr_t mac_address,
   pf_uuid_t * p_uuid)
{
   p_uuid->data1 = session_number;
   p_uuid->data2 = 0x1234;
   p_uuid->data3 = 0x5678;
   p_uuid->data4[0] = 0x01;
   p_uuid->data4[1] = 0x02;
   p_uuid->data4[2] = 0x03;
   p_uuid->data4[3] = 0x04;
   p_uuid->data4[4] = 0x05;
   p_uuid->data4[5] = 0x06;
   p_uuid->data4[6] = 0x07;
   p_uuid->data4[7] = 0x08;
}

void mock_pf_lldp_get_management_address (
   pnet_t * net,
   pf_lldp_management_address_t * p_man_address)
{
   *p_man_address = mock_lldp_data.management_address;
}

int mock_pf_lldp_get_peer_management_address (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_management_address_t * p_man_address)
{
   *p_man_address = mock_lldp_data.peer_management_address;
   return mock_lldp_data.error;
}

void mock_pf_lldp_get_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status)
{
   *p_link_status = mock_lldp_data.link_status;
}

int mock_pf_lldp_get_peer_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status)
{
   *p_link_status = mock_lldp_data.peer_link_status;
   return mock_lldp_data.error;
}

int mock_pnal_snmp_init (pnet_t * pnet, const pnal_cfg_t * pnal_cfg)
{
   return 0;
}

int mock_pf_file_save_if_modified (
   const char * directory,
   const char * filename,
   const void * p_object,
   void * p_tempobject,
   size_t size)
{
   int result;

   /* Exit if any argument is invalid */
   if (size > sizeof (mock_file_data.object))
   {
      return -1;
   }
   if (filename == NULL || p_object == NULL || p_tempobject == NULL)
   {
      return -1;
   }
   if (strlen (filename) >= sizeof (mock_file_data.filename))
   {
      return -1;
   }

   /* Inject error? */
   if (mock_file_data.is_save_failing)
   {
      return -1;
   }

   /* Calculate return value */
   if (strcmp (filename, mock_file_data.filename) != 0)
   {
      result = 2; /* First saving of file */
   }
   else if (
      size == mock_file_data.size &&
      memcmp (p_object, mock_file_data.object, size) == 0)
   {
      result = 0; /* No update required */
   }
   else
   {
      result = 1; /* Updating file */
   }

   /* Store to fake file */
   strcpy (mock_file_data.filename, filename);
   mock_file_data.size = size;
   memcpy (mock_file_data.object, p_object, size);
   return result;
}

int mock_pf_file_save (
   const char * directory,
   const char * filename,
   const void * p_object,
   size_t size)
{
   /* Exit if any argument is invalid */
   if (size > sizeof (mock_file_data.object))
   {
      return -1;
   }
   if (filename == NULL || p_object == NULL)
   {
      return -1;
   }
   if (strlen (filename) >= sizeof (mock_file_data.filename))
   {
      return -1;
   }

   /* Inject error? */
   if (mock_file_data.is_save_failing)
   {
      return -1;
   }

   /* Store to fake file */
   strcpy (mock_file_data.filename, filename);
   mock_file_data.size = size;
   memcpy (mock_file_data.object, p_object, size);
   return 0;
}

void mock_pf_file_clear (const char * directory, const char * filename)
{
   if (strcmp (filename, mock_file_data.filename) == 0)
   {
      strcpy (mock_file_data.filename, "");
      mock_file_data.size = 0;
   }
}

int mock_pf_file_load (
   const char * directory,
   const char * filename,
   void * p_object,
   size_t size)
{
   /* Exit if any argument is invalid */
   if (size != mock_file_data.size)
   {
      return -1;
   }
   if (filename == NULL || p_object == NULL)
   {
      return -1;
   }
   if (strcmp (filename, mock_file_data.filename) != 0)
   {
      return -1;
   }

   /* Inject error? */
   if (mock_file_data.is_load_failing)
   {
      return -1;
   }

   memcpy (p_object, mock_file_data.object, size);
   return 0;
}

void mock_pf_fspm_get_im_location (pnet_t * net, char * location)
{
   snprintf (
      location,
      PNET_LOCATION_MAX_SIZE,
      "%-22s",
      mock_fspm_data.im_location);
}

void mock_pf_fspm_save_im_location (pnet_t * net, const char * location)
{
   snprintf (
      mock_fspm_data.im_location,
      sizeof (mock_fspm_data.im_location),
      "%-22s",
      location);
}

void mock_pf_bg_worker_init (pnet_t * net)
{
   return;
}

int mock_pf_bg_worker_start_job (pnet_t * net, pf_bg_job_t job_id)
{
   return 0;
}
