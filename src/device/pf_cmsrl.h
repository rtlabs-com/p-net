/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2025 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifndef PF_CMSRL_H
#define PF_CMSRL_H

#ifdef __cplusplus
extern "C" {
#endif

void pf_cmsrl_cmdev_state_ind (pnet_t *, pf_ar_t *, pnet_event_values_t);
void pf_cmsrl_cpm_new_data_ind (pnet_t *, pf_ar_t *, uint16_t, bool);
bool pf_cmsrl_al_issue_alarm_req (
   pnet_t *,
   pf_ar_set_t *,
   int16_t,
   pf_alarm_data_t *);
void pf_cmsrl_al_forward_alarm_cnf (
   pf_ar_set_t *,
   bool,
   pnet_pnio_status_t const *);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMSRL_H */
