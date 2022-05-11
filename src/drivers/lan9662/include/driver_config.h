/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2022 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

/**
 * @file
 * @brief LAN9662-specific configuration
 *
 * This file contains definitions of configuration settings for the
 * LAN9662 driver.
 */

#ifndef DRIVER_CONFIG_H
#define DRIVER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pnet_driver_options.h"

typedef struct pnet_driver_config
{
   struct
   {
      uint16_t vcam_base_id;
      uint16_t rtp_base_id;
      uint16_t wal_base_id;
      uint16_t ral_base_id;
   } mera;

} pnet_driver_config_t;

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_CONFIG_H */