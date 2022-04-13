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

#ifndef PF_CPM_DRIVER_SW_H
#define PF_CPM_DRIVER_SW_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize p-net default cpm driver.
 * @param net              InOut: The p-net stack instance
 */
void pf_cpm_driver_sw_init (pnet_t * net);

#ifdef __cplusplus
}
#endif

#endif /* PF_CPM_DRIVER_SW_H */
