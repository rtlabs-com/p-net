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

#include "mocks.h"

#include <gtest/gtest.h>
#include <string.h>

uint8_t pnet_log_level;

os_mutex_t  *mock_mutex;
mock_os_data_t mock_os_data;

void mock_clear(void)
{
   memset(&mock_os_data, 0, sizeof(mock_os_data));
}

void mock_init(void)
{
   mock_mutex = os_mutex_create();
   mock_clear();
}

uint32_t mock_os_get_current_time_us(
   void)
{
   return mock_os_data.current_time_us;
}

os_eth_handle_t* mock_os_eth_init(
   const char *if_name,
   os_eth_callback_t *callback,
   void *arg)
{
   os_eth_handle_t         *handle;

   handle = (os_eth_handle_t*)calloc(1, sizeof(os_eth_handle_t));

   return handle;
}

int mock_os_get_ip_suite(
   const char              *interface_name,
   os_ipaddr_t             *p_ipaddr,
   os_ipaddr_t             *p_netmask,
   os_ipaddr_t             *p_gw,
   const char              **p_device_name)
{
   return 0;
}

int mock_os_set_ip_suite(
   const char              *interface_name,
   os_ipaddr_t             *p_ipaddr,
   os_ipaddr_t             *p_netmask,
   os_ipaddr_t             *p_gw,
   const char              *hostname,
   bool                    permanent)
{
   mock_os_data.set_ip_suite_count++;
   return 0;
}

int mock_os_eth_send(
   os_eth_handle_t         *handle,
   os_buf_t                *p_buf)
{
   memcpy(mock_os_data.eth_send_copy, p_buf->payload, p_buf->len);
   mock_os_data.eth_send_len = p_buf->len;
   mock_os_data.eth_send_count++;

   return p_buf->len;
}

int mock_os_udp_open(
   os_ipaddr_t             addr,
   os_ipport_t             port)
{
   int ret = 2;
   return ret;
}

int mock_os_udp_sendto(
   uint32_t                id,
   os_ipaddr_t             dst_addr,
   os_ipport_t             dst_port,
   const uint8_t           *data,
   int                     size)
{
   mock_os_data.udp_sendto_len = size;
   mock_os_data.udp_sendto_count++;

   return size;
}

void mock_set_os_udp_recvfrom_buffer(
   uint8_t                 *p_src,
   uint16_t                len)
{
   os_mutex_lock(mock_mutex);

   memcpy(mock_os_data.udp_recvfrom_buffer, p_src, len);
   mock_os_data.udp_recvfrom_length = len;
   mock_os_data.udp_recvfrom_count++;

   os_mutex_unlock(mock_mutex);
}

int mock_os_udp_recvfrom(
   uint32_t                id,
   os_ipaddr_t             *p_dst_addr,
   os_ipport_t             *p_dst_port,
   uint8_t                 *data,
   int                     size)
{
   int                     len;

   os_mutex_lock(mock_mutex);

   memcpy(data, mock_os_data.udp_recvfrom_buffer, mock_os_data.udp_recvfrom_length);
   len = mock_os_data.udp_recvfrom_length;
   mock_os_data.udp_recvfrom_length = 0;

   os_mutex_unlock(mock_mutex);

   return len;
}

void mock_os_udp_close(
   uint32_t                id)
{
}

int mock_os_save_file(
   const char              *fullpath,
   void                    *object_1,
   size_t                  size_1,
   void                    *object_2,
   size_t                  size_2
)
{
   int                     pos = 0;

   if (size_1 + size_2 > sizeof(mock_os_data.file_content))
   {
      return -1;
   }
   if (strlen(fullpath) > sizeof(mock_os_data.file_fullpath))
   {
      return -1;
   }

   strcpy(mock_os_data.file_fullpath, fullpath);
   mock_os_data.file_size = size_1 + size_2;

   if (size_1 > 0)
   {
      memcpy(&mock_os_data.file_content[pos], object_1, size_1);
      pos += size_1;
   }
   if (size_2 > 0)
   {
      memcpy(&mock_os_data.file_content[pos], object_2, size_2);
   }

   return 0;
}

void mock_os_clear_file(
   const char              *fullpath
)
{
   if(strcmp(fullpath, mock_os_data.file_fullpath) == 0)
   {
      strcpy(mock_os_data.file_fullpath, "");
      mock_os_data.file_size = 0;
   }
}

int mock_os_load_file(
   const char              *fullpath,
   void                    *object_1,
   size_t                  size_1,
   void                    *object_2,
   size_t                  size_2
)
{
   int                     pos = 0;

   if (size_1 + size_2 > mock_os_data.file_size)
   {
      return -1;
   }

   if(strcmp(fullpath, mock_os_data.file_fullpath) != 0)
   {
      return -1;
   }

   if (size_1 > 0)
   {
      memcpy(object_1, &mock_os_data.file_content[pos], size_1);
      pos += size_1;
   }
   if (size_2 > 0)
   {
      memcpy(object_2, &mock_os_data.file_content[pos], size_2);
   }

   return 0;
}

int mock_pf_alarm_send_diagnosis(
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   pf_diag_item_t          *p_item)
{
   return 0;
}

void mock_pf_generate_uuid(
   uint32_t                timestamp,
   uint32_t                session_number,
   pnet_ethaddr_t          mac_address,
   pf_uuid_t               *p_uuid)
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
