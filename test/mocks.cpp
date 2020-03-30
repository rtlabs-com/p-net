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

uint8_t     mock_os_eth_send_copy[1500];
uint16_t    mock_os_eth_send_len;
uint16_t    mock_os_eth_send_count;

uint16_t    mock_os_udp_sendto_len;
uint16_t    mock_os_udp_sendto_count;

uint16_t    mock_os_set_led_count;
bool        mock_os_set_led_on;

uint8_t     mock_os_udp_recvfrom_buffer[1500];
uint16_t    mock_os_udp_recvfrom_length;
uint16_t    mock_os_udp_recvfrom_count;

os_mutex_t  *mock_mutex;

void mock_clear(void)
{
   memset(mock_os_eth_send_copy, 0, sizeof(mock_os_eth_send_copy));
   mock_os_eth_send_len = 0;
   mock_os_eth_send_count = 0;

   mock_os_udp_sendto_len = 0;
   mock_os_udp_sendto_count = 0;

   mock_os_set_led_count = 0;
   mock_os_set_led_on = false;

   memset(mock_os_udp_recvfrom_buffer, 0, sizeof(mock_os_udp_recvfrom_buffer));
   mock_os_udp_recvfrom_length = 0;
   mock_os_udp_recvfrom_count = 0;
}

void mock_init(void)
{
   mock_mutex = os_mutex_create();
   mock_clear();
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
   os_ipaddr_t             *p_ipaddr,
   os_ipaddr_t             *p_netmask,
   os_ipaddr_t             *p_gw,
   const char              **p_device_name)
{
   return 0;
}

int mock_os_set_ip_suite(
   os_ipaddr_t             *p_ipaddr,
   os_ipaddr_t             *p_netmask,
   os_ipaddr_t             *p_gw,
   const char              *hostname)
{
   return 0;
}

int mock_os_eth_send(
   os_eth_handle_t         *handle,
   os_buf_t                *p_buf)
{
   memcpy(mock_os_eth_send_copy, p_buf->payload, p_buf->len);
   mock_os_eth_send_len = p_buf->len;
   mock_os_eth_send_count++;
   return p_buf->len;
}

int mock_os_udp_socket(void)
{
   int ret = 1;
   return ret;
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
   int                     len = size;
   mock_os_udp_sendto_len = len;
   mock_os_udp_sendto_count++;
   return len;
}

void mock_set_os_udp_recvfrom_buffer(
   uint8_t                 *p_src,
   uint16_t                len)
{
   os_mutex_lock(mock_mutex);
   memcpy(mock_os_udp_recvfrom_buffer, p_src, len);
   mock_os_udp_recvfrom_length = len;
   mock_os_udp_recvfrom_count++;
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
   memcpy(data, mock_os_udp_recvfrom_buffer, mock_os_udp_recvfrom_length);
   len = mock_os_udp_recvfrom_length;
   mock_os_udp_recvfrom_length = 0;
   os_mutex_unlock(mock_mutex);

   return len;
}

void mock_os_udp_close(
   uint32_t                id)
{
}

void mock_os_get_button(
   uint16_t                id,
   bool                    *p_pressed)
{
   *p_pressed = 0;
}

void mock_os_set_led(
   uint16_t                id,
   bool                    on)
{
   mock_os_set_led_count++;
   mock_os_set_led_on = on;
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
