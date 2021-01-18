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

#ifndef PNAL_SYS_H
#define PNAL_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "osal.h"

#define PNAL_BUF_MAX_SIZE 1522

typedef struct os_buf
{
   uint8_t * payload;
   uint16_t len;
} pnal_buf_t;

/**
 * The prototype of raw Ethernet reception call-back functions.
 * *
 * @param arg              In:   User-defined (may be NULL).
 * @param p_buf            In:   The incoming Ethernet frame
 *
 * @return  0  If the frame was NOT handled by this function.
 *          1  If the frame was handled and the buffer freed.
 */
typedef int (pnal_eth_callback_t) (void * arg, pnal_buf_t * p_buf);

typedef struct os_eth_handle
{
   pnal_eth_callback_t * callback;
   void * arg;
   int socket;
   os_thread_t * thread;
} pnal_eth_handle_t;

#ifdef __cplusplus
}
#endif

#endif /* PNAL_SYS_H */
