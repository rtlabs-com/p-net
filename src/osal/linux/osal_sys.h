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

#include <cc.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in.h>

#define OS_THREAD
#define OS_MUTEX
#define OS_CHANNEL
#define OS_SEM
#define OS_EVENT
#define OS_MBOX
#define OS_TIMER

#define OS_BUF_MAX_SIZE 1522

typedef struct
{
   int handle;
   void (*callback) (void * arg);
   void * arg;
} os_channel_t;

struct os_sem
{
   pthread_cond_t cond;
   pthread_mutex_t mutex;
   size_t count;
};

struct os_event
{
   pthread_cond_t cond;
   pthread_mutex_t mutex;
   uint32_t flags;
};

struct os_mbox
{
   pthread_cond_t cond;
   pthread_mutex_t mutex;
   size_t r;
   size_t w;
   size_t count;
   size_t size;
   void * msg[];
};

struct os_timer
{
   timer_t timerid;
   void(*fn) (struct os_timer *, void * arg);
   void * arg;
   uint32_t us;
   bool oneshot;
};

struct os_buf
{
   void * payload;
   uint16_t len;
};

typedef pthread_mutex_t os_mutex_t;
typedef pthread_t os_thread_t;
typedef struct os_sem os_sem_t;
typedef struct os_event os_event_t;
typedef struct os_mbox os_mbox_t;
typedef struct os_timer os_timer_t;
typedef struct os_buf os_buf_t;

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SYS_H */
