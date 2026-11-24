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

#ifndef PF_CMDEV_H
#define PF_CMDEV_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Do work on a device instance.
 * @param dev              In:    The device instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
typedef int (*pf_ftn_device_t) (pf_device_t * dev);

/**
 * Do work on an API instance.
 * @param api              In:    The API instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
typedef int (*pf_ftn_api_t) (pf_api_t * api);

/**
 * Do work on a slot instance.
 * @param slot             In:    The slot instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
typedef int (*pf_ftn_slot_t) (pf_slot_t * slot);

/**
 * Do work on a submodule instance.
 * @param submodule        In:    The submodule instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
typedef int (*pf_ftn_submodule_t) (pf_submodule_t * submodule);

/**
 * Initialize the cmdev component.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmdev_init (pnet_t * net);

/**
 * Un-initialize the cmdev component.
 * Delete all modules and sub-modules.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmdev_exit (pnet_t * net);

/**
 * Traverse the device tree.
 * @param net              InOut: The p-net stack instance
 * @param p_ftn_dev        In:    Call this function for the device.
 * @param p_ftn_api        In:    Call this function for each api.
 * @param p_ftn_slot       In:    Call this function for each slot.
 * @param p_ftn_sub        In:    Call this function for each submodule.
 * @return  0  If all provided functions return 0.
 *          -1 If any function called returns an error (!= 0)..
 */
int pf_cmdev_cfg_traverse (
   pnet_t * net,
   pf_ftn_device_t p_ftn_dev,
   pf_ftn_api_t p_ftn_api,
   pf_ftn_slot_t p_ftn_slot,
   pf_ftn_submodule_t p_ftn_sub);

/**
 * Call a function for each submodule of a p-net instance.
 * @param net              InOut: The p-net stack instance
 * @param func             In: The callback function
 * @param user_data        In: User data passed to the callback function.
 * @returns 0 if the function returns 0 for all submodules,
 *          otherwise the first nonzero return value of \a func,
 *          in which case the iteration is broken.
 */
int pf_cmdev_for_each_submodule (
   pnet_t * net,
   int (*func) (pnet_t *, pf_submodule_t *, void * user_data),
   void * user_data);

/**
 * Plug a module into a slot.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param module_ident_nbr In:    The module ident number.
 * @param slot_out         Out:   If not NULL, used to return the slot.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_plug_module (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint32_t module_ident_nbr,
   pf_slot_t ** slot_out);

/**
 * Check whether a plugged submodule is compatible with the expected
 * submodule.
 *
 * The submodule is compatible if the ident numbers are equal, or if the
 * application accepts it as a substitute via the \a pnet_is_substitute_cb()
 * user callback.
 *
 * @param net              InOut: The p-net stack instance
 * @param expected_ident   In:    The expected submodule ident number.
 * @param submodule_ident  In:    The plugged submodule ident number.
 * @return  true  if the submodule ident numbers are equal or the
 *                submodule is a substitute.
 *          false otherwise.
 */
bool pf_cmdev_submodule_is_compatible (
   pnet_t * net,
   uint32_t expected_ident,
   uint32_t submodule_ident);

/**
 * Plug a sub-module into a sub-slot.
 * Sends an alarm describing the result.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param subslot_nbr      In:    The sub-slot number.
 * @param module_ident_nbr In:    The module ident number.
 * @param submod_ident_nbr In:    The sub-module ident number.
 * @param direction        In:    Data direction(s).
 * @param length_input     In:    Number of input bytes.
 * @param length_output    In:    Number of output bytes.
 * @param flags            In:    Submodule flags.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_plug_submodule (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t module_ident_nbr,
   uint32_t submod_ident_nbr,
   pnet_submodule_dir_t direction,
   uint16_t length_input,
   uint16_t length_output,
   uint8_t flags);

/**
 * Pull a sub-module from a sub-slot.
 * Sends an alarm to the current owner, if there is one.
 * @param net                InOut: The p-net stack instance
 * @param api_id             In:    The API identifier.
 * @param slot_nbr           In:    The slot number.
 * @param subslot_nbr        In:    The subslot number.
 * @param pull_empty_module  In:    Whether to pull the module if it becomes
 *                                  empty.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_pull_submodule (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   bool pull_empty_module);

/**
 * Pull a module from a slot.
 * First pulls all submodules (sends alarms).
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_pull_module (pnet_t * net, uint32_t api_id, uint16_t slot_nbr);

/**
 * Abort request from the application or from RPC.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance (or NULL).
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_cm_abort (pnet_t * net, pf_ar_t * p_ar);

/**
 * Get a pointer to the device configuration.
 * @param net              InOut: The p-net stack instance
 * @param device_out       Out:   The device configuration.
 * @return  0  Always.
 */
int pf_cmdev_get_device (pnet_t * net, pf_device_t ** device_out);

/**
 * Get an API instance by its API id.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:    The API id.
 * @param api_out          Out:   The API instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_get_api (pnet_t * net, uint32_t api_id, pf_api_t ** api_out);

/**
 * Get a slot instance of an API instance by slot number.
 * @param api              In:  The API instance.
 * @param slot_nbr         In:  The slot number.
 * @param slot_out         Out: The slot instance.
 * @return  0 if operation succeeded.
 *         -1 if an error occurred.
 */
int pf_cmdev_get_slot (
   pf_api_t const * api,
   uint16_t slot_nbr,
   pf_slot_t ** slot_out);

/**
 * Get a submodule instance of a slot instance by subslot number.
 * @param slot             In:  The slot instance.
 * @param subslot_nbr      In:  The subslot number.
 * @param submodule_out    Out: The submodule instance.
 * @return  0 if operation succeeded.
 *         -1 if an error occurred.
 */
int pf_cmdev_get_submodule (
   pf_slot_t const * slot,
   uint16_t subslot_nbr,
   pf_submodule_t ** submodule_out);

/**
 * Get a submodule instance from the api ID, the slot number and the subslot
 * number.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param subslot_nbr      In:    The subslot number.
 * @param submodule_out    Out:   The submodule instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_get_submodule_full (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   pf_submodule_t ** submodule_out);

/* Not used */
/**
 * Get a slot instance from the api ID and the slot number.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param slot_out         Out:   The slot instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_get_slot_full (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   pf_slot_t ** slot_out);

/**
 * Get identity for plugged module from the api ID and slot number.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param p_module_ident   Out:   Identity for plugged module.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_get_module_ident (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint32_t * p_module_ident);

/**
 * Get identity for plugged submodule from
 * the api ID, slot and subslot number.
 * @param net               InOut: The p-net stack instance
 * @param api_id            In:    The API identifier.
 * @param slot_nbr          In:    The slot number.
 * @param subslot_nbr       In:    The subslot number.
 * @param p_submodule_ident Out:   Identity for plugged submodule.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_get_submodule_ident (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t * p_submodule_ident);

/**
 * Get a diag item, from the array of items (by using index).
 *
 * @param net              InOut: The p-net stack instance
 * @param item_ix          In:    The diag item index.
 * @param pp_item          Out:   The diag item.
 * @return  0  If item_ix is valid.
 *          -1 If item_ix is invalid.
 */
int pf_cmdev_get_diag_item (
   pnet_t * net,
   uint16_t item_ix,
   pf_diag_item_t ** pp_item);

/**
 * Allocate a new diag item entry from the free list.
 * The entry is also cleared.
 * @param net              InOut: The p-net stack instance
 * @param p_item_ix        Out:   Index of the allocated item.
 * @return  0  If an item was allocated.
 *          -1 If out of diag items.
 */
int pf_cmdev_new_diag (pnet_t * net, uint16_t * p_item_ix);

/**
 * Put a diag item back to the free list.
 * @param net              InOut: The p-net stack instance
 * @param item_ix          In:    Index of the item to return.
 */
void pf_cmdev_free_diag (pnet_t * net, uint16_t item_ix);

/**
 * Find next diagnosis USI value (sorted) for a subslot
 *
 * Finds the smallest USI value that is greater than \a low_usi_limit
 *
 * @param net              InOut: The p-net stack instance
 * @param list_head        In:    Index of first item of diagnoses for
 *                                this subslot
 * @param low_usi_limit    In:    Previous USI value
 * @param p_next_usi       Out:   Next USI value
 * @return 0 if an USI value was found, -1 otherwise.
 */
int pf_cmdev_get_next_diagnosis_usi (
   pnet_t * net,
   uint16_t list_head,
   uint16_t low_usi_limit,
   uint16_t * p_next_usi);

/**
 * Find a specific expected API instance in the AR.
 * @param ar               In:    The AR instance.
 * @param api              In:    The API to find.
 * @param exp_api          Out:   The expected API instance.
 * @return  0  if an API instance was found.
 *          -1 if the API instance was not found.
 */
int pf_cmdev_get_exp_api (
   pf_ar_t const * ar,
   uint32_t api,
   pf_exp_api_t ** exp_api);

/**
 * Find the expected module of an AR, addressed by the API and the
 * slot number.
 * @param ar               In:    The AR instance.
 * @param api              In:    The API.
 * @param slot_number      In:    The slot number.
 * @param exp_module       Out:   The expected module instance.
 * @return  0  if a module instance was found.
 *          -1 if the module instance was not found.
 */
int pf_cmdev_get_exp_mod (
   pf_ar_t const * ar,
   uint32_t api,
   uint16_t slot_number,
   pf_exp_module_t ** exp_module);

/**
 * Find the expected submodule of an AR, addressed by the API and the
 * slot and subslot numbers.
 * @param ar               In:    The AR instance.
 * @param api              In:    The API.
 * @param slot_number      In:    The slot number.
 * @param subslot_number   In:    The subslot number.
 * @param exp_submodule    Out:   The expected submodule instance.
 * @return  0  if a submodule instance was found.
 *          -1 if the submodule instance was not found.
 */
int pf_cmdev_get_exp_sub (
   pf_ar_t const * ar,
   uint32_t api,
   uint16_t slot_number,
   uint16_t subslot_number,
   pf_exp_submodule_t ** exp_submodule);

/**
 * Generate module diff, when needed, for the specified AR.
 * The AR should have a defined IO-AR.
 * @param net              InOut: The p-net stack instance
 * @param ar               InOut: The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_generate_module_diff (pnet_t * net, pf_ar_t * ar);

/**
 * Generate module diff focused on a specific submodule, when needed, for the
 * specified AR.
 * @param net              InOut: The p-net stack instance
 * @param ar               InOut: The AR instance.
 * @param api              In:    The API containing the submodule.
 * @param slot_number      In:    The slot containing the submodule.
 * @param subslot_number   In:    The subslot containing the submodule.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_generate_submodule_diff (
   pnet_t * net,
   pf_ar_t * ar,
   uint32_t api,
   uint16_t slot_number,
   uint16_t subslot_number);

/**
 * Check the IOCR param of an AR for errors.
 * @param p_ar             InOut: The AR instance.
 * @param p_stat           Out:   Detailed error information if return != 0.
 * @return  0  if no error was found.
 *          -1 if an error was found.
 */
int pf_cmdev_check_iocr_param (pf_ar_t * p_ar, pnet_result_t * p_stat);

/**
 * Return a string representation of the specified CMDEV state.
 * @param state            In:    The CMDEV state.
 * @return  A string representation of the CMDEV state.
 */
const char * pf_cmdev_state_to_string (pf_cmdev_state_values_t state);

/**
 * Return a string describing the submodule direction.
 * @param direction            In:    submodule direction
 * @return  A string representation
 */
const char * pf_cmdev_submod_dir_to_string (pnet_submodule_dir_t direction);

/**
 * Return a string representation of the specified CMDEV event.
 * @param event            In:    The CMDEV event.
 * @return  A string representation of the CMDEV event.
 */
const char * pf_cmdev_event_to_string (pnet_event_values_t event);

/**
 * Show CMDEV information of the AR.
 * @param p_ar             In:    The AR instance.
 */
void pf_cmdev_ar_show (const pf_ar_t * p_ar);

/**
 * Show the plugged modules and sub-modules.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmdev_device_show (pnet_t * net);

/**
 * Show diagnosis items.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmdev_diag_show (const pnet_t * net);

/**
 * Send a new state transition event of the CMDEV component.
 *
 * Among other actions, it calls the \a pnet_state_ind() user callback.
 *
 * If the event is PNET_EVENT_ABORT, it might send an alarm frame. Make sure
 * to set p_ar->err_cls and p_ar->err_code, in order to have the correct
 * error values in the alarm frame.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param event            In:    The CMDEV state transition event. Use
 *                                PNET_EVENT_xxx, not PF_CMDEV_STATE_xxx
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_state_ind (pnet_t * net, pf_ar_t * p_ar, pnet_event_values_t event);

/**
 * Get the CMDEV state of the specified AR.
 * @param p_ar             In:    The AR instance.
 * @param p_state          Out:   The CMDEV state.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_get_state (const pf_ar_t * p_ar, pf_cmdev_state_values_t * p_state);

/**
 * Handle CMIO "data_possibile" indications.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param data_possible    In:    true if data exchange is possible.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_cmio_info_ind (pnet_t * net, pf_ar_t * p_ar, bool data_possible);

/**
 * Get the prm_flag of a submodule with respect to a given AR.
 *
 * @param ar               In: The AR instance.
 * @param api_id           In: The API.
 * @param slot_nbr         In: The slot number.
 * @param subslot_nbr      In: The subslot number.
 * @return true if the submodule is being parameterized through the AR,
 *         false if not.
 */
bool pf_cmdev_prm_flag (
   pf_ar_t const * ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr);

/* ======================================================================
 *       Remote primitives
 */

/**
 * Handle an RPC connect request.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_connect_result Out:   Detailed error information if return != 0;
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_rm_connect_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_result_t * p_connect_result);

/**
 * Handle an RPC release request.
 *
 * This triggers the user callbacks \a pnet_release_ind() and
 * \a pnet_state_ind() with PNET_EVENT_ABORT.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_release_result Out:   Detailed error information if return != 0;
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_rm_release_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_result_t * p_release_result);

/**
 * Handle an RPC dcontrol request.
 *
 * This triggers the user callbacks \a pnet_dcontrol_ind() and
 * \a pnet_state_ind() with PNET_EVENT_PRMEND.
 *
 * @param net                  InOut: The p-net stack instance
 * @param p_ar                 InOut: The AR instance.
 * @param p_control_io         In:    The control block.
 * @param p_sm_list            InOut: The SubmoduleList block.
 * @param p_release_result     Out:   Detailed result of the connect operation.
 * @param p_set_state_prmend   Out:   Trigger PNET_EVENT_PRMEND after the
 *                                    response have been sent.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_rm_dcontrol_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_control_block_t * p_control_io,
   pf_submodule_list_block_t * p_sm_list,
   pnet_result_t * p_release_result,
   bool * p_set_state_prmend);

/**
 * Convey the APPLRDY indication from the application to the controller.
 * Ccontrol is a general mechanism, but it currently only has one purpose in the
 * device. For this reason alone it does not have an opcode parameter. If this
 * function does not see all PPM data areas are set (by the app) then it returns
 * error and the application must try again - otherwise the startup will hang.
 *
 * Triggers the \a pnet_state_ind() user callback with PNET_EVENT_APPLRDY.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @return  0  if a ccontrol request was sent.
 *          -1 if a ccontrol request was not sent.
 */
int pf_cmdev_cm_ccontrol_req (pnet_t * net, pf_ar_t * p_ar);

/**
 * Handle the confirmation of the ccontrol request.
 *
 * Triggers these user callbacks:
 * * \a pnet_state_ind() with PNET_EVENT_DATA.
 * * \a pnet_ccontrol_cnf()
 *
 * @param net                 InOut: The p-net stack instance
 * @param p_ar                InOut: The AR instance.
 * @param p_control_io        In:    The control block.
 * @param p_ccontrol_result   InOut: Detailed error information if return != 0.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_rm_ccontrol_cnf (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_control_block_t * p_control_io,
   pnet_result_t * p_ccontrol_result);

/************ Internal functions, made available for unit testing ************/

int pf_cmdev_calculate_exp_sub_data_descriptor_direction (
   pnet_submodule_dir_t submodule_dir,
   pf_data_direction_values_t data_dir,
   pf_dev_status_type_t status_type,
   pf_data_direction_values_t * resulting_data_dir);

int pf_cmdev_check_zero (uint8_t * p_start, uint16_t len);

int pf_cmdev_check_visible_string (const char * s);

int pf_cmdev_check_no_straddle (
   uint16_t start_1,
   uint16_t length_1,
   uint16_t start_2,
   uint16_t length_2);

int pf_cmdev_check_ar_type (uint16_t ar_type, bool device_access);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMDEV_H */
