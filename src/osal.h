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

#ifndef OSAL_H
#define OSAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "osal_sys.h"
#include "cc.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b)) ? (a) : (b)
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#endif

#ifndef BIT
#define BIT(n) (1U << (n))
#endif

#ifndef NELEMENTS
#define NELEMENTS(a) (sizeof(a)/sizeof((a)[0]))
#endif

#ifndef OS_WAIT_FOREVER
#define OS_WAIT_FOREVER 0xFFFFFFFF
#endif

#ifndef OS_MUTEX
typedef void os_mutex_t;
#endif

#ifndef OS_SEM
typedef void os_sem_t;
#endif

#ifndef OS_THREAD
typedef void os_thread_t;
#endif

#ifndef OS_EVENT
typedef void os_event_t;
#endif

#ifndef OS_MBOX
typedef void os_mbox_t;
#endif

#ifndef OS_TIMER
typedef void os_timer_t;
#endif

#ifndef OS_CHANNEL
typedef void os_channel_t;
#endif

#define OS_MAKEU32(a,b,c,d) (((uint32_t)((a) & 0xff) << 24) | \
                             ((uint32_t)((b) & 0xff) << 16) | \
                             ((uint32_t)((c) & 0xff) << 8)  | \
                             (uint32_t)((d) & 0xff))

/** Set an IP address given by the four byte-parts */
#define OS_IP4_ADDR_TO_U32(ipaddr, a,b,c,d)  ipaddr = OS_MAKEU32(a,b,c,d)

enum os_eth_type {
  OS_ETHTYPE_IP        = 0x0800U,
  OS_ETHTYPE_ARP       = 0x0806U,
  OS_ETHTYPE_VLAN      = 0x8100U,
  OS_ETHTYPE_PROFINET  = 0x8892U,
  OS_ETHTYPE_ETHERCAT  = 0x88A4U,
  OS_ETHTYPE_LLDP      = 0x88CCU,
};

#define OS_PF_RPC_SERVER_PORT    0x8894 /* PROFInet Context Manager */
#define OS_PF_SNMP_SERVER_PORT   161 /* snmp */

/* 255.255.255.255 */
#ifndef OS_IPADDR_NONE
#define OS_IPADDR_NONE         ((uint32_t)0xffffffffUL)
#endif
/* 127.0.0.1 */
#ifndef OS_IPADDR_LOOPBACK
#define OS_IPADDR_LOOPBACK     ((uint32_t)0x7f000001UL)
#endif
/* 0.0.0.0 */
#ifndef OS_IPADDR_ANY
#define OS_IPADDR_ANY          ((uint32_t)0x00000000UL)
#endif
/* 255.255.255.255 */
#ifndef OS_IPADDR_BROADCAST
#define OS_IPADDR_BROADCAST    ((uint32_t)0xffffffffUL)
#endif

typedef uint32_t os_ipaddr_t;
typedef uint16_t os_ipport_t;

int os_snprintf (char * str, size_t size, const char * fmt, ...) CC_FORMAT (3,4);
void os_log (int type, const char * fmt, ...) CC_FORMAT (2,3);
void * os_malloc (size_t size);

void os_usleep (uint32_t us);
uint32_t os_get_current_time_us (void);

os_thread_t * os_thread_create (const char * name, int priority,
        int stacksize, void (*entry) (void * arg), void * arg);

os_mutex_t * os_mutex_create (void);
void os_mutex_lock (os_mutex_t * mutex);
void os_mutex_unlock (os_mutex_t * mutex);
void os_mutex_destroy (os_mutex_t * mutex);

os_sem_t * os_sem_create (size_t count);
int os_sem_wait (os_sem_t * sem, uint32_t time);
void os_sem_signal (os_sem_t * sem);
void os_sem_destroy (os_sem_t * sem);

os_event_t * os_event_create (void);
int os_event_wait (os_event_t * event, uint32_t mask, uint32_t * value, uint32_t time);
void os_event_set (os_event_t * event, uint32_t value);
void os_event_clr (os_event_t * event, uint32_t value);
void os_event_destroy (os_event_t * event);

os_mbox_t * os_mbox_create (size_t size);
int os_mbox_fetch (os_mbox_t * mbox, void ** msg, uint32_t time);
int os_mbox_post (os_mbox_t * mbox, void * msg, uint32_t time);
void os_mbox_destroy (os_mbox_t * mbox);

os_timer_t * os_timer_create (uint32_t us, void (*fn) (os_timer_t * timer, void * arg),
                              void * arg, bool oneshot);
void os_timer_set (os_timer_t * timer, uint32_t us);
void os_timer_start (os_timer_t * timer);
void os_timer_stop (os_timer_t * timer);
void os_timer_destroy (os_timer_t * timer);

os_buf_t * os_buf_alloc(uint16_t length);
void os_buf_free(os_buf_t *p);
uint8_t os_buf_header(os_buf_t *p, int16_t header_size_increment);

/**
 * Send raw Ethernet data
 *
 * @param handle        In: Ethernet handle
 * @param buf           In: Buffer with data to be sent
 * @return  The number of bytes sent, or -1 if an error occurred.
 */
int os_eth_send(
   os_eth_handle_t         *handle,
   os_buf_t                *buf);

/**
 * Initialize receiving of raw Ethernet frames (in separate thread)
 *
 * @param if_name       In: Ethernet interface name
 * @param callback      In: Callback for received raw Ethernet frames
 * @param arg           InOut: User argument passed to the callback
 *
 * @return  the Ethernet handle, or NULL if an error occurred.
 */
os_eth_handle_t* os_eth_init(
   const char              *if_name,
   os_eth_callback_t       *callback,
   void                    *arg);

int os_udp_socket(void);
int os_udp_open(os_ipaddr_t addr, os_ipport_t port);
int os_udp_sendto(uint32_t id,
      os_ipaddr_t dst_addr,
      os_ipport_t dst_port,
      const uint8_t * data,
      int size);
int os_udp_recvfrom(uint32_t id,
      os_ipaddr_t *dst_addr,
      os_ipport_t *dst_port,
      uint8_t * data,
      int size);
void os_udp_close(uint32_t id);

int os_get_ip_suite(
   os_ipaddr_t             *p_ipaddr,
   os_ipaddr_t             *p_netmask,
   os_ipaddr_t             *p_gw,
   const char              **p_device_name);

int os_set_ip_suite(
   os_ipaddr_t             *p_ipaddr,
   os_ipaddr_t             *p_netmask,
   os_ipaddr_t             *p_gw,
   const char              *hostname);

void os_set_led(
   uint16_t                id,         /* Starting from 0 */
   bool                    on);

void os_get_button(
   uint16_t                id,         /* Starting from 0 */
   bool                    *p_pressed);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_H */
