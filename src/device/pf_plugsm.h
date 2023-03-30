/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2023 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

/**
 * @file
 * @brief Partial implementation of the Plug State Machine (PLUGSM)
 *
 * These functions implement the subset of the PLUGSM state machine (see
 * Profinet 2.4 services, section 7.3.2.2.6.2.4) that deals with submodules
 * being released by an AR, when it is released. A released submodule may get
 * a new owning AR, the controller side of which is expected to go through a
 * parameterisation sequence, before the ownership is finalised.
 */

#ifndef PF_PLUGSM_H
#define PF_PLUGSM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Request a Released alarm to be sent for a submodule to a controller through
 * a specific AR.
 *
 * The request is queued up for the AR in question. If the queue is empty at
 * the time of calling this function, the request is acted upon immediately.
 *
 * @param net              InOut: The p-net stack instance.
 * @param ar               InOut: The AR instance.
 * @param api              In:    The API containing the submodule.
 * @param slot_number      In:    The slot containing the submodule.
 * @param subslot_number   In:    The subslot containing the submodule.
 * @param module_ident     In:    The identification of the module.
 * @param submodule_ident  In:    The identification of the submodule.
 */
void pf_plugsm_released_alarm_req (
   pnet_t * net,
   pf_ar_t * ar,
   uint32_t api,
   uint16_t slot_number,
   uint16_t subslot_number,
   uint32_t module_ident,
   uint32_t submodule_ident);

/**
 * Handle the PrmEnd request sent by a controller as a result of a submodule
 * being released.
 *
 * This will call the \a pnet_sm_released_ind() callback registered by the
 * application, and act according to its return value: either record that the
 * application is expected to call \a pnet_sm_released_cnf(), or activate the
 * next request for a Released alarm from the same AR.
 *
 * @param net              InOut: The p-net stack instance.
 * @param ar               InOut: The AR instance.
 * @param result           Out:   Detailed error information if return != 0;
 * @return 0 on success.
 *         -1 on failure.
 */
int pf_plugsm_prmend_ind (
   pnet_t * net,
   pf_ar_t * ar,
   pnet_result_t * result);

/**
 * Send an Application Ready request to the controller, after an AR to it has
 * become the new owner of a released submodule.
 *
 * This will be called indirectly by the application, when it calls \a
 * pnet_sm_released_cnf() to confirm that it has handled a released submodule.
 *
 * @param net              InOut: The p-net stack instance.
 * @param ar               InOut: The AR instance.
 */
void pf_plugsm_application_ready_req (pnet_t * net, pf_ar_t * ar);

/**
 * Handle the Application Ready confirmation sent by a controller, after an AR
 * to it has become the new owner of a released submodule.
 *
 * This will check the queue of Released alarm requests, and act on the first
 * request in the queue, if there is one.
 *
 * @param net              InOut: The p-net stack instance.
 * @param ar               InOut: The AR instance.
 * @return 0 on success.
 *         -1 on failure.
 */
int pf_plugsm_application_ready_cnf (pnet_t * net, pf_ar_t * ar);

#ifdef __cplusplus
}
#endif

#endif /* PF_PLUGSM_H */
