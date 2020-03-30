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

#ifndef PF_ALARM_H
#define PF_ALARM_H

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * Initialize the alarm component.
 * @param net              InOut: The p-net stack instance
 */
void pf_alarm_init(
   pnet_t                  *net);

/**
 * Create and activate an alarm instance for the specified AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_alarm_activate(
   pnet_t                  *net,
   pf_ar_t                 *p_ar);

/**
 * Close an alarm instance for the specified AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_alarm_close(
   pnet_t                  *net,
   pf_ar_t                 *p_ar);

/**
 * Enable sending of all alarms.
 *
 * This is the default setting.
 * @param p_ar             In:   The AR instance.
 */
void pf_alarm_enable(
   pf_ar_t                 *p_ar);

/**
 * Disable sending of all alarms.
 * @param p_ar             In:   The AR instance.
 */
void pf_alarm_disable(
   pf_ar_t                 *p_ar);

/**
 * Check if an alarm is pending.
 * @param p_ar             In:   The AR instance.
 * @return  true if an alarm is pending.
 *          false if no alarms are pending.
 */
bool pf_alarm_pending(
   pf_ar_t                 *p_ar);

/**
 * Perform periodic tasks in the alarm component.
 * Check for controller inputs.
 * @param net              InOut: The p-net stack instance
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_alarm_periodic(
   pnet_t                  *net);

/**
 * Send a PULL alarm.
 *
 * This functions sends a PULL alarm.
 * If subslot_nbr == 0 and PULL_MODULE alarms are allowed by the controller (in the CONNECT)
 * then a PULL_MODULE alarm is send.
 * Otherwise a PULL alarm is sent.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try later).
 */
int pf_alarm_send_pull(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr);

/**
 * Send a PLUG OK alarm.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param module_ident     In:   The module ident number.
 * @param submodule_ident  In:   The sub-module ident number.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try later).
 */
int pf_alarm_send_plug(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint32_t                module_ident,
   uint32_t                submodule_ident);

/**
 * Send a PLUG_WRONG alarm.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param module_ident     In:   The module ident number.
 * @param submodule_ident  In:   The sub-module ident number.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try later).
 */
int pf_alarm_send_plug_wrong(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint32_t                module_ident,
   uint32_t                submodule_ident);

/**
 * Send process alarm.
 *
 * This function sends a process alarm to a controller.
 * These alarms are always sent as high priority alarms.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param payload_usi      In:   The USI for the alarm (may be 0).
 * @param payload_len      In:   Length of diagnostics data (may be 0).
 * @param p_payload        In:   Diagnostics data (may be NULL if payload_usi == 0).
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try later).
 */
int pf_alarm_send_process(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint16_t                payload_usi,
   uint16_t                payload_len,
   uint8_t                 *p_payload);

/**
 * Send diagnosis alarm.
 *
 * This function sends a diagnosis alarm to a controller.
 * These alarms are always sent as low priority alarms.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param p_diag_item      In:   The diag item.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try later).
 */
int pf_alarm_send_diagnosis(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   pf_diag_item_t          *p_diag_item);

/**
 * Send Alarm ACK.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param p_pnio_status    In:   Detailed ACK information.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try later).
 */
int pf_alarm_alpmr_alarm_ack(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_pnio_status_t      *p_pnio_status);

/**
 * Show all alarm information of the specified AR.
 *
 * @param p_ar             In:   The AR instance.
 */
void pf_alarm_show(
   pf_ar_t                 *p_ar);

#ifdef __cplusplus
}
#endif

#endif /* PF_ALARM_H */
