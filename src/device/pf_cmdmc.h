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

#ifndef PF_CMDMC_H
#define PF_CMDMC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ?
 * @param p_ar             InOut: The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdmc_activate_req (pf_ar_t * p_ar);

/**
 * ?
 * @param p_ar             InOut: The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdmc_close_req (pf_ar_t * p_ar);

/**
 * ?
 * @param p_ar             InOut: The AR instance.
 * @param ix               In:    ?
 * @param start            In:    ?
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdmc_cpm_state_ind (pf_ar_t * p_ar, uint16_t ix, bool start);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMDMC_H */
