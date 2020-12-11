/*
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

p-net Profinet device stack
===========================

Copyright 2018 rt-labs AB

This instance of p-net is licensed for use in a specific product, as
follows:

Company: Bender GmbH & Co. KG
Product: COMTRAXX
Product description: COMTRAXX products provided by Bender GmbH & Co. KG

Use of this instance of p-net in other products is not allowed.

You are hereby given a non-exclusive, non-transferable license to use
the SOFTWARE and documentation according to the SOFTWARE LICENSE
AGREEMENT document shipped together with this instance of the
SOFTWARE. The SOFTWARE LICENSE AGREEMENT prohibits copying and
redistribution of the SOFTWARE.  It prohibits reverse-engineering
activities.  It limits warranty and liability on behalf of rt-labs.
The details of these terms are explained in subsequent chapters of the
SOFTWARE LICENSE AGREEMENT.
-----BEGIN PGP SIGNATURE-----

iQEzBAEBCgAdFiEE1e4U+gPgGJXLsxxLZlG0bopmiXIFAl/TMvQACgkQZlG0bopm
iXJE1QgAlEKblKkIrLF+hVRm6I1IfbGV6VWnBx+9IPwaHZmgQZAVTOlp4RHiNAre
Il1l1xheENWpuihbVlvz5hMCVto4fkT13IUXxOo9Va9MK8lm6JOu2lYpl1HgVIwV
R6I6dsWLeNBSBaA38DDBHIfjP4LBZC/HVYO2ISc3/OKutqasz/WIp+AjAURiKRsf
yaWGay88FITHymmqJpGaWzB+jAOvRFs8hW/GsOyX/r17AK5F5N51O+UqUTKAWNPT
f6M8QF0b07Hw+eOuB7A/0D5nvSl/rOd9LvYkiUhIUmQdHqERNkF9EJdroIjxTMXi
lmoOkPEqyGV58fh7JjMYlI60sPlGBw==
=TaN+
-----END PGP SIGNATURE-----
*/

#include "mocks.h"

#include <gtest/gtest.h>
#include <string.h>

uint8_t pnet_log_level;

os_mutex_t * mock_mutex;
mock_os_data_t mock_os_data;
mock_lldp_data_t mock_lldp_data;

void mock_clear (void)
{
   memset (&mock_os_data, 0, sizeof (mock_os_data));
   memset (&mock_lldp_data, 0, sizeof (mock_lldp_data));
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

pnal_eth_handle_t * mock_pnal_eth_init (
   const char * if_name,
   pnal_eth_callback_t * callback,
   void * arg)
{
   pnal_eth_handle_t * handle;

   handle = (pnal_eth_handle_t *)calloc (1, sizeof (pnal_eth_handle_t));

   return handle;
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

int mock_pnal_snmp_init (pnet_t * pnet)
{
   return 0;
}
