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
extern "C"
{
#endif

#include <stdint.h>

#include "pnet_api.h"
#include "pf_includes.h"

#include "osal.h"

extern uint8_t     mock_os_eth_send_copy[1500];
extern uint16_t    mock_os_eth_send_len;
extern uint16_t    mock_os_eth_send_count;

extern uint16_t    mock_os_udp_sendto_len;
extern uint16_t    mock_os_udp_sendto_count;

extern uint16_t    mock_os_set_led_count;
extern bool        mock_os_set_led_on;

void mock_init(void);
void mock_clear(void);
void mock_set_os_udp_recvfrom_buffer(uint8_t *p_src, uint16_t len);

os_eth_handle_t* mock_os_eth_init(
   const char *if_name,
   os_eth_callback_t *callback,
   void *arg);
int mock_os_eth_send(os_eth_handle_t *handle, os_buf_t * buf);
void mock_os_cpy_mac_addr(uint8_t * mac_addr);
int mock_os_udp_socket(void);
int mock_os_udp_open(os_ipaddr_t addr, os_ipport_t port);
int mock_os_udp_sendto(uint32_t id,
      os_ipaddr_t dst_addr,
      os_ipport_t dst_port,
      const uint8_t * data,
      int size);
int mock_os_udp_recvfrom(uint32_t id,
      os_ipaddr_t *dst_addr,
      os_ipport_t *dst_port,
      uint8_t * data,
      int size);
void mock_os_udp_close(uint32_t id);
int mock_os_set_ip_suite(
   os_ipaddr_t             *p_ipaddr,
   os_ipaddr_t             *p_netmask,
   os_ipaddr_t             *p_gw,
   const char              *hostname);
void mock_os_get_button(uint16_t id, bool *p_pressed);
void mock_os_set_led(uint16_t id, bool on);
int mock_pf_alarm_send_diagnosis(
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   pf_diag_item_t          *p_item);

#ifdef __cplusplus
}
#endif

#endif /* MOCKS_H */
