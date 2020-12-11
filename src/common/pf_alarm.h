/*
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

p-net Profinet device stack
===========================

Copyright 2018 rt-labs AB

This instance of p-net is licensed for use in a specific product, as
follows:

Company: Bender GmbH & Co. KG
Product: COMTRAXX
Product description: COMTRAXX products provided by Bender GmbH & Co. KG

Use of this instance of p-net in other products is not allowed.

You are hereby given a non-exclusive, non-transferable license to use
the SOFTWARE and documentation according to the SOFTWARE LICENSE
AGREEMENT document shipped together with this instance of the
SOFTWARE. The SOFTWARE LICENSE AGREEMENT prohibits copying and
redistribution of the SOFTWARE.  It prohibits reverse-engineering
activities.  It limits warranty and liability on behalf of rt-labs.
The details of these terms are explained in subsequent chapters of the
SOFTWARE LICENSE AGREEMENT.
-----BEGIN PGP SIGNATURE-----

iQEzBAEBCgAdFiEE1e4U+gPgGJXLsxxLZlG0bopmiXIFAl/TMvQACgkQZlG0bopm
iXJE1QgAlEKblKkIrLF+hVRm6I1IfbGV6VWnBx+9IPwaHZmgQZAVTOlp4RHiNAre
Il1l1xheENWpuihbVlvz5hMCVto4fkT13IUXxOo9Va9MK8lm6JOu2lYpl1HgVIwV
R6I6dsWLeNBSBaA38DDBHIfjP4LBZC/HVYO2ISc3/OKutqasz/WIp+AjAURiKRsf
yaWGay88FITHymmqJpGaWzB+jAOvRFs8hW/GsOyX/r17AK5F5N51O+UqUTKAWNPT
f6M8QF0b07Hw+eOuB7A/0D5nvSl/rOd9LvYkiUhIUmQdHqERNkF9EJdroIjxTMXi
lmoOkPEqyGV58fh7JjMYlI60sPlGBw==
=TaN+
-----END PGP SIGNATURE-----
*/

#ifndef PF_ALARM_H
#define PF_ALARM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the alarm component.
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_alarm_init (pnet_t * net);

/**
 * Create and activate an alarm instance for the specified AR.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_alarm_activate (pnet_t * net, pf_ar_t * p_ar);

/**
 * Close an alarm instance for the specified AR.
 *
 * Sends a low prio alarm.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_alarm_close (pnet_t * net, pf_ar_t * p_ar);

/**
 * Enable sending of all alarms.
 *
 * This is the default setting.
 *
 * @param p_ar             InOut  The AR instance.
 */
void pf_alarm_enable (pf_ar_t * p_ar);

/**
 * Disable sending of all alarms.
 *
 * @param p_ar             InOut: The AR instance.
 */
void pf_alarm_disable (pf_ar_t * p_ar);

/**
 * Check if an alarm is pending.
 *
 * @param p_ar             In:    The AR instance.
 * @return  true if an alarm is pending.
 *          false if no alarms are pending.
 */
bool pf_alarm_pending (const pf_ar_t * p_ar);

/**
 * Perform periodic tasks in the alarm component.
 *
 * Check for controller inputs.
 *
 * @param net              InOut: The p-net stack instance
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_alarm_periodic (pnet_t * net);

/**
 * Send a PULL alarm.
 *
 * This functions sends a PULL alarm.
 * If subslot_nbr == 0 and PULL_MODULE alarms are allowed by the controller (in
 * the CONNECT) then a PULL_MODULE alarm is send. Otherwise a PULL alarm is
 * sent.
 *
 * Uses no alarm payload (not attached to diagnosis ASE.
 * Related to whole submodule, not related to channels).
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut  The AR instance.
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param subslot_nbr      In:    The sub-slot number.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try
 *                                  later).
 */
int pf_alarm_send_pull (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr);

/**
 * Send a PLUG OK alarm.
 *
 * Uses no alarm payload (not attached to diagnosis ASE.
 * Related to whole submodule, not related to channels).
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param subslot_nbr      In:    The sub-slot number.
 * @param module_ident     In:    The module ident number.
 * @param submodule_ident  In:    The sub-module ident number.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try
 *                                  later).
 */
int pf_alarm_send_plug (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t module_ident,
   uint32_t submodule_ident);

/**
 * Send a PLUG_WRONG alarm.
 *
 * Uses no alarm payload (not attached to diagnosis ASE.
 * Related to whole submodule, not related to channels).
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param subslot_nbr      In:    The sub-slot number.
 * @param module_ident     In:    The module ident number.
 * @param submodule_ident  In:    The sub-module ident number.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try
 *                                  later).
 */
int pf_alarm_send_plug_wrong (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t module_ident,
   uint32_t submodule_ident);

/**
 * Send process alarm.
 *
 * This function sends a process alarm to a controller.
 * These alarms are always sent as high priority alarms.
 *
 * User-supplied payload. Not attached to diagnosis ASE, but might have a
 * relation to channels.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param subslot_nbr      In:    The sub-slot number.
 * @param payload_usi      In:    The USI for the alarm. Use 0 if no payload.
 *                                Max 0x7fff
 * @param payload_len      In:    Length of diagnostics data (may be 0).
 * @param p_payload        In:    User supplied payload (may be NULL if
 *                                payload_usi == 0).
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try
 *                                  later).
 */
int pf_alarm_send_process (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t payload_usi,
   uint16_t payload_len,
   const uint8_t * p_payload);

/**
 * Send diagnosis alarm.
 *
 * This function sends a diagnosis alarm to a controller.
 * These alarms are always sent as low priority alarms.
 *
 * The alarm type is typically "Diagnosis alarm", but might be for example
 * "Port data change notification" (based on the channel error type in the
 * diag item).
 *
 * This is attached to diagnosis ASE (has a diagnosis object as payload).
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param subslot_nbr      In:    The sub-slot number.
 * @param p_diag_item      In:    The diag item.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try
 *                                  later).
 */
int pf_alarm_send_diagnosis (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   const pf_diag_item_t * p_diag_item);

/**
 * Send Alarm ACK (from the application)
 *
 * Always uses high prio, has no payload.
 *
 * This is the "Alarm notification" service.
 *
 * ALPMR: ALPMR_Alarm_Ack.req
 * ALPMR: ALPMR_Alarm_Ack.cnf   (Implements part of that signal via the return
 *                               value)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_alarm_argument In:    The alarm argument (with slot, subslot,
 *                                alarm_type etc)
 * @param p_pnio_status    In:    Detailed ACK information.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try
 *                                  later).
 */
int pf_alarm_alpmr_alarm_ack (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pnet_alarm_argument_t * p_alarm_argument,
   const pnet_pnio_status_t * p_pnio_status);

/**
 * Show all alarm information of the specified AR.
 *
 * @param p_ar             In:    The AR instance.
 */
void pf_alarm_show (const pf_ar_t * p_ar);

/************ Internal functions, made available for unit testing ************/

void pf_alarm_add_diag_item_to_summary (
   const pf_ar_t * p_ar,
   const pf_subslot_t * p_subslot,
   const pf_diag_item_t * p_diag_item,
   pnet_alarm_spec_t * p_alarm_spec,
   uint32_t * p_maint_status);

int pf_alarm_add_send_queue (
   pf_alarm_queue_t * q,
   const pf_alarm_data_t * p_alarm_data);

int pf_alarm_fetch_send_queue (
   pf_alarm_queue_t * q,
   pf_alarm_data_t * p_alarm_data);

#ifdef __cplusplus
}
#endif

#endif /* PF_ALARM_H */
