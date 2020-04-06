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
extern "C"
{
#endif

/**
 * Do work on a device instance.
 * @param p_dev            In:   The device instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
typedef int (*pf_ftn_device_t)(
      pf_device_t          *p_dev);

/**
 * Do work on an API instance.
 * @param p_api            In:   The API instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
typedef int (*pf_ftn_api_t)(
      pf_api_t             *p_api);

/**
 * Do work on a slot instance.
 * @param p_slot           In:   The slot instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
typedef int (*pf_ftn_slot_t)(
      pf_slot_t            *p_slot);

/**
 * Do work on a sub-slot instance.
 * @param p_subslot        In:   The sub-slot instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
typedef int (*pf_ftn_subslot_t)(
      pf_subslot_t         *p_subslot);

/**
 * Initialize the cmdev component. Plugs the DAP (sub-)modules.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmdev_init(
   pnet_t                  *net);

/**
 * Un-initialize the cmdev component.
 * Delete all modules and sub-modules.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmdev_exit(
   pnet_t                  *net);

/**
 * Traverse the device tree.
 * @param net              InOut: The p-net stack instance
 * @param p_ftn_dev        In:   Call this function for every device.
 * @param p_ftn_api        In:   Call this function for each api.
 * @param p_ftn_slot       In:   Call this function for each slot.
 * @param p_ftn_sub        In:   Call this function for each sub-slots.
 * @return  0  If all provided functions return 0.
 *          -1 If any function called returns an error (!= 0)..
 */
int pf_cmdev_cfg_traverse(
   pnet_t                  *net,
   pf_ftn_device_t         p_ftn_dev,
   pf_ftn_api_t            p_ftn_api,
   pf_ftn_slot_t           p_ftn_slot,
   pf_ftn_subslot_t        p_ftn_sub);

/**
 * Plug a module into a slot.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @param module_ident_nbr In:   The module ident number.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_plug_module(
   pnet_t                  *net,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint32_t                module_ident_nbr);

/**
 * Plug a sub-module into a sub-slot.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param module_ident_nbr In:   The module ident number.
 * @param submod_ident_nbr In:   The sub-module ident number.
 * @param direction        In:   Data direction(s).
 * @param length_input     In:   Number of input bytes.
 * @param length_output    In:   Number of output bytes.
 * @param update           In:   true for update of sub-module.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_plug_submodule(
   pnet_t                  *net,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint32_t                module_ident_nbr,
   uint32_t                submod_ident_nbr,
   pnet_submodule_dir_t    direction,
   uint16_t                length_input,
   uint16_t                length_output,
   bool                    update);

/**
 * Pull a sub-module from a sub-slot.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_pull_submodule(
   pnet_t                  *net,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr);

/**
 * Pull a module from a slot.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_pull_module(
   pnet_t                  *net,
   uint32_t                api_id,
   uint16_t                slot_nbr);

/**
 * Abort request from the application.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_cm_abort(
   pnet_t                  *net,
   pf_ar_t                 *p_ar);

/**
 * Get a pointer to the device configuration.
 * @param net              InOut: The p-net stack instance
 * @param pp_device        Out:  The device configuration.
 * @return  0  Always.
 */
int pf_cmdev_get_device(
   pnet_t                  *net,
   pf_device_t             **pp_device);

/**
 * Get an API instance by its API id.
 * @param net              InOut: The p-net stack instance
 * @param api_id        In:  The API id.
 * @param pp_api        Out: The API instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_get_api(
   pnet_t                  *net,
   uint32_t                api_id,
   pf_api_t                **pp_api);

/**
 * Get a sub-slot instance from the api ID, the slot number and the sub-slot number.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param pp_subslot       Out:  The sub-slot instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_get_subslot_full(
   pnet_t                  *net,
   uint16_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   pf_subslot_t            **pp_subslot);


/* Not used */
/**
 * Get a slot instance from the api ID and the slot number.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @param pp_slot          Out:  The slot instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_get_slot_full(
   pnet_t                  *net,
   uint16_t                api_id,
   uint16_t                slot_nbr,
   pf_slot_t               **pp_slot);

/**
 * Get a diag item.
 * @param net              InOut: The p-net stack instance
 * @param item_ix          In:   The diag item index.
 * @param pp_item          Out:  The diag item.
 * @return  0  If item_ix is valid.
 *          -1 If item_ix is invalid.
 */
int pf_cmdev_get_diag_item(
   pnet_t                  *net,
   uint16_t                item_ix,
   pf_diag_item_t          **pp_item);

/**
 * Allocate a new diag item entry from the free list.
 * @param net              InOut: The p-net stack instance
 * @param p_item_ix        Out:  Index of the allocated item.
 * @return  0  If an item was allocated.
 *          -1 If out of diag items.
 */
int pf_cmdev_new_diag(
   pnet_t                  *net,
   uint16_t                *p_item_ix);

/**
 * Return a diag item to the free list.
 * @param net              InOut: The p-net stack instance
 * @param item_ix          In:   Index of the item to return.
 */
void pf_cmdev_free_diag(
   pnet_t                  *net,
   uint16_t                item_ix);

/**
 * Return a string representation of the specified CMDEV state.
 * @param state            In:   The CMDEV state.
 * @return  A string representation of the CMDEV state.
 */
const char *pf_cmdev_state_to_string(
   pf_cmdev_state_values_t state);

/**
 * Return a string representation of the specified CMDEV event.
 * @param event            In:   The CMDEV event.
 * @return  A string representation of the CMDEV event.
 */
const char *pf_cmdev_event_to_string(
   pnet_event_values_t     event);

/**
 * Show CMDEV information of the AR.
 * @param p_ar             In:   The AR instance.
 */
void pf_cmdev_show(
   pf_ar_t                 *p_ar);

/**
 * Show the plugged modules and sub-modules.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmdev_show_device(
   pnet_t                  *net);

/**
 * Indicate a new state transition of the CMDEV component.
 *
 * Among other actions, it calls the \a pnet_state_ind() user callback.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param state            In:   The new CMDEV state.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_state_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_event_values_t     state);

/**
 * Get the CMDEV state of the specified AR.
 * @param p_ar             In:   The AR instance.
 * @param p_state          Out:  The CMDEV state.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_get_state(
   pf_ar_t                 *p_ar,
   pf_cmdev_state_values_t *p_state);

/**
 * Handle CMIO "data_possibile" indications.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param data_possible    In:   true if data exchange is possible.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_cmio_info_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   bool                    data_possible);

/* ======================================================================
 *       Remote primitives
 */

/**
 * Handle an RPC connect request.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param p_connect_result Out:  Detailed result of the connect operation.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_rm_connect_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_result_t           *p_connect_result);

/**
 * Handle an RPC release request.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param p_release_result Out:  Detailed result of the connect operation.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_rm_release_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_result_t           *p_release_result);

/**
 * Handle an RPC dcontrol request.
 *
 * This triggers the user callbacks \a pnet_dcontrol_ind() and
 * \a pnet_state_ind() with PNET_EVENT_PRMEND.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param p_control_io     In:   The control block.
 * @param p_release_result Out:  Detailed result of the connect operation.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_rm_dcontrol_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_control_block_t      *p_control_io,
   pnet_result_t           *p_release_result);

/**
 * Convey the APPLRDY indication from the application to the controller.
 * Ccontrol is a general mechanism, but it currently only has one purpose in the device.
 * For this reason alone it does not have an opcode parameter.
 * If this function does not see all PPM data areas are set (by the app) then it returns
 * error and the application must try again - otherwise the startup will hang.
 *
 * Triggers the \a pnet_state_ind() user callback with PNET_EVENT_APPLRDY.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @return  0  if a ccontrol request was sent.
 *          -1 if a ccontrol request was not sent.
 */
int pf_cmdev_cm_ccontrol_req(
   pnet_t                  *net,
   pf_ar_t                 *p_ar);

/**
 * Handle the confirmation of the ccontrol request.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param p_control_io     In:   The control block.
 * @param p_ccontrol_result Out:  Detailed result of the connect operation.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmdev_rm_ccontrol_cnf(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_control_block_t      *p_control_io,
   pnet_result_t           *p_ccontrol_result);



/************ Internal functions, made available for unit testing ************/

int pf_cmdev_calculate_exp_sub_data_descriptor_direction(
   pnet_submodule_dir_t       submodule_dir,
   pf_data_direction_values_t data_dir,
   pf_dev_status_type_t       status_type,
   pf_data_direction_values_t *resulting_data_dir);


#ifdef __cplusplus
}
#endif

#endif /* PF_CMDEV_H */
