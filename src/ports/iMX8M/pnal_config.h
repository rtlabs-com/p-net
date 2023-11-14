/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

/**
 * @file
 * @brief PNAL-specific configuration
 *
 * This file contains definitions of configuration settings for the
 * PNAL layer.
 */

#ifndef PNAL_CONFIG_H
#define PNAL_CONFIG_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pnal_thread_cfg
{
   uint32_t prio;
   size_t stack_size;
} pnal_thread_cfg_t;

typedef struct pnal_cfg
{
   pnal_thread_cfg_t bg_worker_thread;
} pnal_cfg_t;

#ifdef __cplusplus
}
#endif

#endif /* PNAL_CONFIG_H */
