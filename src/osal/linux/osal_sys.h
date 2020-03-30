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

#ifndef OSAL_SYS_H
#define OSAL_SYS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>
#include <time.h>
#include <netinet/in.h>

#define OS_THREAD
#define OS_MUTEX
#define OS_SEM
#define OS_EVENT
#define OS_MBOX
#define OS_TIMER

#define OS_BUF_MAX_SIZE 1522

typedef pthread_t os_thread_t;
typedef pthread_mutex_t os_mutex_t;

typedef struct os_sem
{
   pthread_cond_t cond;
   pthread_mutex_t mutex;
   size_t count;
} os_sem_t;

typedef struct os_event
{
   pthread_cond_t cond;
   pthread_mutex_t mutex;
   uint32_t flags;
} os_event_t;

typedef struct os_mbox
{
   pthread_cond_t cond;
   pthread_mutex_t mutex;
   size_t r;
   size_t w;
   size_t count;
   size_t size;
   void * msg[];
} os_mbox_t;

typedef struct os_timer
{
   timer_t timerid;
   os_thread_t * thread;
   pid_t thread_id;
   bool exit;
   void(*fn) (struct os_timer *, void * arg);
   void * arg;
   uint32_t us;
   bool oneshot;
} os_timer_t;

typedef struct os_buf
{
   void * payload;
   uint16_t len;
} os_buf_t;

/**
 * The prototype of raw Ethernet reception call-back functions.
 * *
 * @param arg              In:   User-defined (may be NULL).
 * @param p_buf            In:   The incoming Ethernet frame
 *
 * @return  0  If the frame was NOT handled by this function.
 *          1  If the frame was handled and the buffer freed.
 */
typedef int (os_eth_callback_t)(
   void                    *arg,
   os_buf_t                *p_buf);

typedef struct os_eth_handle
{
   os_eth_callback_t       *callback;
   void                    *arg;
   int                     socket;
   os_thread_t             *thread;
} os_eth_handle_t;

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SYS_H */
