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

#ifndef MAIN_OSAL_H
#define MAIN_OSAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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

int os_snprintf (char * str, size_t size, const char * fmt, ...);
void os_log (int type, const char * fmt, ...);
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


void os_set_led(
   uint16_t                id,         /* Starting from 0 */
   bool                    on);

void os_get_button(
   uint16_t                id,         /* Starting from 0 */
   bool                    *p_pressed);


#ifdef __cplusplus
}
#endif

#endif /* MAIN_OSAL_H */
