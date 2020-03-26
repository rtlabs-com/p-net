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

#include "osal.h"
#include "options.h"
#include "log.h"
#include <gpio.h>
#include <lwip/netif.h>
#include <drivers/net.h>


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GPIO_LED1       GPIO_P5_9
#define GPIO_LED2       GPIO_P5_8

#define GPIO_BUTTON1    GPIO_P15_13
#define GPIO_BUTTON2    GPIO_P15_12

/**
 * Set IP addr, mask and gateway.
 * Set hostname.
 * @param p_ipaddr
 * @param p_netmask
 * @param p_gw
 * @param hostname
 * @return
 */
int os_set_ip_suite(
   os_ipaddr_t             *p_ipaddr,
   os_ipaddr_t             *p_netmask,
   os_ipaddr_t             *p_gw,
   const char              *hostname)
{
   ip_addr_t      ip_addr;
   ip_addr_t      ip_mask;
   ip_addr_t      ip_gw;

   ip_addr.addr = htonl(*p_ipaddr);
   ip_mask.addr = htonl(*p_netmask);
   ip_gw.addr = htonl(*p_gw);
   net_configure(NULL, &ip_addr, &ip_mask, &ip_gw, NULL, hostname);

   /* ToDo: Save to flash?? */
   return 0;
}

int os_snprintf(char * str, size_t size, const char * fmt, ...)
{
   int ret;
   va_list list;

   va_start (list, fmt);
   ret = snprintf (str, size, fmt, list);
   va_end (list);
   return ret;
}

void os_log (int type, const char * fmt, ...)
{
   va_list list;

   switch(LOG_LEVEL_GET (type))
   {
   case LOG_LEVEL_DEBUG:   rprintf ("%10d [DEBUG] ", tick_get()); break;
   case LOG_LEVEL_INFO:    rprintf ("%10d [INFO ] ", tick_get()); break;
   case LOG_LEVEL_WARNING: rprintf ("%10d [WARN ] ", tick_get()); break;
   case LOG_LEVEL_ERROR:   rprintf ("%10d [ERROR] ", tick_get()); break;
   default: break;
   }

   va_start (list, fmt);
   vprintf (fmt, list);
   va_end (list);
}

void * os_malloc (size_t size)
{
   return malloc (size);
}

os_thread_t * os_thread_create (const char * name, int priority,
        int stacksize, void (*entry) (void * arg), void * arg)
{
   return task_spawn (name, entry, priority, stacksize, arg);
}

os_mutex_t * os_mutex_create (void)
{
   return mtx_create();
}

void os_mutex_lock (os_mutex_t * mutex)
{
   mtx_lock (mutex);
}

void os_mutex_unlock (os_mutex_t * mutex)
{
   mtx_unlock (mutex);
}

void os_mutex_destroy (os_mutex_t * mutex)
{
   mtx_destroy (mutex);
}

void os_usleep (uint32_t us)
{
   task_delay (tick_from_ms (us / 1000));
}

uint32_t os_get_current_time_us (void)
{
   return 1000 * tick_to_ms (tick_get());
}

os_sem_t * os_sem_create (size_t count)
{
   return sem_create (count);
}

int os_sem_wait (os_sem_t * sem, uint32_t time)
{
   int tmo = 0;

   if (time != OS_WAIT_FOREVER)
   {
      tmo = sem_wait_tmo (sem, tick_from_ms (time));
   }
   else
   {
      sem_wait (sem);
   }

   return tmo;
}

void os_sem_signal (os_sem_t * sem)
{
   sem_signal (sem);
}

void os_sem_destroy (os_sem_t * sem)
{
   sem_destroy (sem);
}

os_event_t * os_event_create (void)
{
   return flags_create (0);
}

int os_event_wait (os_event_t * event, uint32_t mask, uint32_t * value, uint32_t time)
{
   int tmo = 0;

   if (time != OS_WAIT_FOREVER)
   {
      tmo = flags_wait_any_tmo (event, mask, tick_from_ms (time), value);
   }
   else
   {
      flags_wait_any (event, mask, value);
   }

   *value = *value & mask;
   return tmo;
}

void os_event_set (os_event_t * event, uint32_t value)
{
   flags_set (event, value);
}

void os_event_clr (os_event_t * event, uint32_t value)
{
   flags_clr (event, value);
}

void os_event_destroy (os_event_t * event)
{
   flags_destroy (event);
}

os_mbox_t * os_mbox_create (size_t size)
{
   return mbox_create (size);
}

int os_mbox_fetch (os_mbox_t * mbox, void ** msg, uint32_t time)
{
   int tmo = 0;

   if (time != OS_WAIT_FOREVER)
   {
      tmo = mbox_fetch_tmo (mbox, msg, tick_from_ms (time));
   }
   else
   {
      mbox_fetch (mbox, msg);
   }

   return tmo;
}

int os_mbox_post (os_mbox_t * mbox, void * msg, uint32_t time)
{
   int tmo = 0;

   if (time != OS_WAIT_FOREVER)
   {
      tmo = mbox_post_tmo (mbox, msg, tick_from_ms (time));
   }
   else
   {
      mbox_post (mbox, msg);
   }

   return tmo;
}

void os_mbox_destroy (os_mbox_t * mbox)
{
   mbox_destroy (mbox);
}

os_timer_t * os_timer_create (uint32_t us, void (*fn) (os_timer_t *, void * arg),
                              void * arg, bool oneshot)
{
   return tmr_create (tick_from_ms (us / 1000), fn, arg,
                      (oneshot) ? TMR_ONCE : TMR_CYCL);
}

void os_timer_set (os_timer_t * timer, uint32_t us)
{
   tmr_set (timer, tick_from_ms (us / 1000));
}

void os_timer_start (os_timer_t * timer)
{
   tmr_start (timer);
}

void os_timer_stop (os_timer_t * timer)
{
   tmr_stop (timer);
}

void os_timer_destroy (os_timer_t * timer)
{
   tmr_destroy (timer);
}

os_buf_t * os_buf_alloc(uint16_t length)
{
   return pbuf_alloc(PBUF_RAW, length, PBUF_POOL);
}
void os_buf_free(os_buf_t *p)
{
   if (pbuf_free(p) != 1)
   {
      printf("\n\nPBUF not freed\n");
   }
}

uint8_t os_buf_header(os_buf_t *p, int16_t header_size_increment)
{
   return pbuf_header(p, header_size_increment);
}

void os_get_button(uint16_t id, bool *p_pressed)
{
   if (id == 0)
   {
      *p_pressed = (gpio_get(GPIO_BUTTON1) == 0);
   }
   else if (id == 1)
   {
      *p_pressed = (gpio_get(GPIO_BUTTON2) == 0);
   }
   else
   {
      *p_pressed = false;
   }
}

void os_set_led(uint16_t id, bool on)
{
   if (id == 0)
   {
      gpio_set(GPIO_LED1, on?1:0);
   }
   else if (id == 1)
   {
      gpio_set(GPIO_LED2, on?1:0);
   }
}
