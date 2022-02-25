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

/**
 * @file
 * @brief Internal header for FSPM
 */

#ifndef PF_FSPM_H
#define PF_FSPM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the FSPM component.
 *
 * Validate and store the application configuration.
 *
 * Turn off the signal LED.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_cfg            In:   The application configuration of the Profinet
 * stack.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_fspm_init (pnet_t * net, const pnet_cfg_t * p_cfg);

/**
 * Retrieve the minimum device interval, from the configuration.
 *
 * This is the smallest allowed data exchange interval, in units of 31.25 us.
 * Typically 32, which corresponds to 1 ms.
 *
 * @param net              InOut: The p-net stack instance
 * @return the minimum device interval.
 */
int16_t pf_fspm_get_min_device_interval (const pnet_t * net);

/**
 * Create a LogBook entry.
 *
 * Logbooks are described in Profinet 2.4 Services, section 7.3.6 "LogBook ASE"
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:   The AREP, identifying the AR.
 * @param p_pnio_status    In:   The PNIO status.
 * @param entry_detail     In:   Additional application information.
 */
void pf_fspm_create_log_book_entry (
   pnet_t * net,
   uint32_t arep,
   const pnet_pnio_status_t * p_pnio_status,
   uint32_t entry_detail);

/**
 * Process write record requests from the controller.
 * If index indicates I&M data records then handle here.
 *
 * If index is user-defined then call application
 * call-back \a pnet_write_ind() if defined.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param p_write_request  In:    The write request record.
 * @param write_length     In:    Length in bytes of write data.
 * @param p_write_data     In:    The data to write.
 * @param p_result         Out:   Detailed error information if returning != 0
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_fspm_cm_write_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_request,
   uint16_t write_length,
   const uint8_t * p_write_data,
   pnet_result_t * p_result);

/**
 * Process read record requests from the controller.
 * Triggers the \a pnet_read_ind() user callback for some values.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param p_read_request   In:    The read request record.
 * @param pp_read_data     Out:   A pointer to the source data.
 * @param p_read_length    InOut: The maximum (in) and actual (out) length in
 *                                bytes of the binary value.
 * @param p_result         Out:   The result information.
 * @return  0  if operation succeeded.
 *          -1 if not handled or an error occurred.
 */
int pf_fspm_cm_read_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_read_request_t * p_read_request,
   uint8_t ** pp_read_data,
   uint16_t * p_read_length,
   pnet_result_t * p_result);

/**
 * Response from controller to appl_rdy request.
 *
 * Triggers the \a pnet_ccontrol_cnf() user callback.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param p_result         Out:  The result information.
 */
void pf_fspm_ccontrol_cnf (
   pnet_t * net,
   const pf_ar_t * p_ar,
   pnet_result_t * p_result);

/**
 * Call user call-back \a pnet_connect_ind() when a new connection is requested.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param p_result         Out:   The result information if return != 0.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_fspm_cm_connect_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   pnet_result_t * p_result);

/**
 * Call user call-back \a pnet_release_ind() when a connection is released.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param p_result         Out:   Detailed error information if return != 0.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_fspm_cm_release_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   pnet_result_t * p_result);

/**
 * Call user call-back \a pnet_dcontrol_ind() to indicate a control request
 * from the ProfiNet controller.
 *
 * Create a logbook entry.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param control_command  In:   The control command.
 * @param p_result         Out:  The result information.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_fspm_cm_dcontrol_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   pnet_control_command_t control_command,
   pnet_result_t * p_result);

/**
 * Call user call-back \a pnet_state_ind() to indicate a new event
 * within the profinet stack.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param event            In:    The new CMDEV state. Use PNET_EVENT_xxx, not
 *                                PF_CMDEV_STATE_xxx
 */
void pf_fspm_state_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   pnet_event_values_t event);

/**
 * The remote side sends an alarm.
 *
 * Calls user call-back \a pnet_alarm_ind().
 * We have already sent the transport acknowledge ("TACK") frame.
 *
 * ALPMR: ALPMR_Alarm_Notification.ind
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param p_alarm_argument In:    The alarm argument (with slot, subslot,
 *                                alarm_type etc)
 * @param data_len         In:    Data length
 * @param data_usi         In:    USI for the alarm
 * @param p_data           InOut: Alarm data
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_fspm_alpmr_alarm_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pnet_alarm_argument_t * p_alarm_arg,
   uint16_t data_len,
   uint16_t data_usi,
   const uint8_t * p_data);

/**
 * The remote side acknowledges the alarm sent by this side, by sending an
 * AlarmAck. Calls user call-back \a pnet_alarm_cnf().
 *
 * ALPMI: ALPMI_Alarm_Notification.cnf(+/-)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param p_pnio_status    In:    Detailed ACK information.
 */
void pf_fspm_alpmi_alarm_cnf (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pnet_pnio_status_t * p_pnio_status);

/**
 * Local side has successfully sent the AlarmAck to the remote side.
 * Calls user call-back \a pnet_alarm_ack_cnf().
 *
 * ALPMR: ALPMR_Alarm_Ack.cnf(+/-)  (Implements part of the signal)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param res              In:    0  if ACK was received by the remote side.
 *                                   This is cnf(+)
 *                                -1 if ACK was not received.
 *                                   This is cnf(-)
 */
void pf_fspm_alpmr_alarm_ack_cnf (pnet_t * net, const pf_ar_t * p_ar, int res);

/**
 * Call user call-back \a pnet_exp_module_ind() to indicate to application
 * it needs a specific module in a specific slot.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API id.
 * @param slot_nbr         In:   The slot number.
 * @param module_ident     In:   The module ident number.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_fspm_exp_module_ind (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint32_t module_ident);

/**
 * Call user call-back \a pnet_exp_submodule_ind() to indicate to application
 * it needs a specific sub-module in a specific sub-slot.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API id.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param module_ident     In:   The module ident number.
 * @param submodule_ident  In:   The sub-module ident number.
 * @param p_exp_data       In:   The expected data configuration
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_fspm_exp_submodule_ind (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t module_ident,
   uint32_t submodule_ident,
   const pnet_data_cfg_t * exp_data);

/**
 * Notify application that the received data status has changed,
 * via the \a pnet_new_data_status_ind() user callback.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param p_iocr           In:    The IOCR instance.
 * @param changes          In:    The changed bits in the data status.
 * @param data_status      In:    Data status (after the changes)
 */
void pf_fspm_data_status_changed (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iocr_t * p_iocr,
   uint8_t changes,
   uint8_t data_status);

/**
 * Call user call-back when the controller requests a reset.
 *
 * This uses the \a pnet_reset_ind() callback.
 *
 * @param net                       InOut: The p-net stack instance
 * @param should_reset_application  In:    True if the user should reset the
 *                                         application data.
 * @param reset_mode                In:    Detailed reset information.
 */
void pf_fspm_reset_ind (
   pnet_t * net,
   bool should_reset_application,
   uint16_t reset_mode);

/**
 * Call user call-back when the Profinet signal LED should change state.
 *
 * This uses the \a pnet_signal_led_ind() callback.
 *
 * @param net                       InOut: The p-net stack instance
 * @param led_state                 In:    True if the signal LED should be on.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred. Will trigger a log message.
 */
int pf_fspm_signal_led_ind (pnet_t * net, bool led_state);

/**
 * Retrieve a pointer to the current configuration data.
 * @param net              InOut: The p-net stack instance
 * @param pp_cfg           Out: The current configuration (or NULL).
 */
void pf_fspm_get_cfg (pnet_t * net, pnet_cfg_t ** pp_cfg);

/**
 * Retrieve a pointer to the default configuration data.
 * @param net              InOut: The p-net stack instance
 * @param pp_cfg           Out: The default configuration (or NULL).
 */
void pf_fspm_get_default_cfg (pnet_t * net, const pnet_cfg_t ** pp_cfg);

/**
 * Get device location from I&M data record 1.
 *
 * Device location is always 22 bytes long, possibly padded with
 * space characters (' ') to the right. It is always null terminated.
 *
 * Note that the SNMP thread may call this at any time.
 *
 * @param net              InOut: The p-net stack instance
 * @param location         Out:   Current device location.
 *                                Buffer must at least be of size
 *                                PNET_LOCATION_MAX_SIZE.
 */
void pf_fspm_get_im_location (pnet_t * net, char * location);

/**
 * Set device location in I&M data record 1 and save it to persistent memory.
 *
 * Only the first 22 bytes will be saved.
 * If \a location is less than 22 bytes, it will be padded with spaces
 * until it is 22 bytes long. Null termination will also be added.
 *
 * Note that the SNMP thread may call this at any time.
 *
 * @param net              InOut: The p-net stack instance
 * @param location         In:    New device location.
 */
void pf_fspm_save_im_location (pnet_t * net, const char * location);

/**
 * Clear the I&M data records 1-4.
 * @param net              InOut: The p-net stack instance
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_fspm_clear_im_data (pnet_t * net);

/**
 * Save the I&M settings to nonvolatile memory, if necessary.
 * Blocking call, intended to be called from the background worker.
 *
 * Compares with the content of already stored settings (in order not to
 * wear out the flash chip)
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_fspm_save_im (pnet_t * net);

/**
 * Show identification & maintenance settings.
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_fspm_im_show (const pnet_t * net);

/**
 * Show compile time options, and memory usage.
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_fspm_option_show (const pnet_t * net);

/**
 * Show logbook.
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_fspm_logbook_show (const pnet_t * net);

/************ Internal functions, made available for unit testing ************/

int pf_fspm_validate_configuration (const pnet_cfg_t * p_cfg);

#ifdef __cplusplus
}
#endif

#endif /* PF_FSPM_H */
