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

#ifndef MOCKS_H
#define MOCKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "options.h"
#include "pnet_api.h"
#include "pf_includes.h"
#include "osal.h"

typedef struct mock_os_data_obj
{
   uint8_t eth_send_copy[PF_FRAME_BUFFER_SIZE];
   uint16_t eth_send_len;
   uint16_t eth_send_count;

   uint16_t udp_sendto_len;
   uint16_t udp_sendto_count;

   uint8_t udp_recvfrom_buffer[PF_FRAME_BUFFER_SIZE];
   uint16_t udp_recvfrom_length;
   uint16_t udp_recvfrom_count;

   uint16_t set_ip_suite_count;

   uint32_t current_time_us;

   char file_fullpath[100]; /* Full file path at latest save operation */
   uint16_t file_size;
   uint8_t file_content[2000];

} mock_os_data_t;

extern mock_os_data_t mock_os_data;

uint32_t mock_os_get_current_time_us (void);

void mock_init (void);
void mock_clear (void);
void mock_set_os_udp_recvfrom_buffer (uint8_t * p_src, uint16_t len);

os_eth_handle_t * mock_os_eth_init (
   const char * if_name,
   os_eth_callback_t * callback,
   void * arg);
int mock_os_eth_send (os_eth_handle_t * handle, os_buf_t * buf);
void mock_os_cpy_mac_addr (uint8_t * mac_addr);
int mock_os_udp_open (os_ipaddr_t addr, os_ipport_t port);
int mock_os_udp_sendto (
   uint32_t id,
   os_ipaddr_t dst_addr,
   os_ipport_t dst_port,
   const uint8_t * data,
   int size);
int mock_os_udp_recvfrom (
   uint32_t id,
   os_ipaddr_t * dst_addr,
   os_ipport_t * dst_port,
   uint8_t * data,
   int size);
void mock_os_udp_close (uint32_t id);
int mock_os_set_ip_suite (
   const char * interface_name,
   os_ipaddr_t * p_ipaddr,
   os_ipaddr_t * p_netmask,
   os_ipaddr_t * p_gw,
   const char * hostname,
   bool permanent);

int mock_os_save_file (
   const char * fullpath,
   void * object_1,
   size_t size_1,
   void * object_2,
   size_t size_2);

void mock_os_clear_file (const char * fullpath);

int mock_os_load_file (
   const char * fullpath,
   void * object_1,
   size_t size_1,
   void * object_2,
   size_t size_2);

int mock_pf_alarm_send_diagnosis (
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   pf_diag_item_t * p_item);

void mock_pf_generate_uuid (
   uint32_t timestamp,
   uint32_t session_number,
   pnet_ethaddr_t mac_address,
   pf_uuid_t * p_uuid);

#ifdef __cplusplus
}
#endif

#endif /* MOCKS_H */
