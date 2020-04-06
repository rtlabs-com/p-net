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

#include <kern.h>
#include <cc.h>
#include <sys/socket.h>
#include <lwip/pbuf.h>
#include <dev.h>

#define OS_THREAD
#define OS_MUTEX
#define OS_CHANNEL
#define OS_SEM
#define OS_EVENT
#define OS_MBOX
#define OS_TIMER

#define OS_BUF_MAX_SIZE PBUF_POOL_BUFSIZE

typedef struct
{
   int handle;
   void (*callback) (void * arg);
   void * arg;
} os_channel_t;

typedef mtx_t os_mutex_t;
typedef task_t os_thread_t;
typedef sem_t os_sem_t;
typedef flags_t os_event_t;
typedef mbox_t os_mbox_t;
typedef tmr_t os_timer_t;


/* Re-use lwIP pbuf for rt-kernel */
typedef struct pbuf os_buf_t;

/* TODO: Integrate in standard drivers?
 * Local handling to enable the NIC drivers to support a RX hook
 */
#define IOCTL_NET_SET_RX_HOOK      0x601

int eth_ioctl (drv_t * drv, void * arg, int req, void * param);

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
   os_eth_callback_t * callback;
   void * arg;
   int if_id;
} os_eth_handle_t;

#ifdef __cplusplus
}
#endif

#endif /* OSAL_SYS_H */
