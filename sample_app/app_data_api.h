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

#ifndef APP_DATA_API_H
#define APP_DATA_API_H

/**
 * @file
 * @brief Application data interface
 *
 * Functions for:
 * - Getting input data
 * - Setting output data
 * - Setting default output state. This should be
 *   part of all device implementations for setting
 *   defined state when device is not connected to PLC
 * - Reading and writing parameters
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct app_data_t app_data_t;

/**
 * Get application specific input data (for sending to PLC).
 *
 * This function is not called for the DAP submodules (slot_nbr 0).
 *
 * @param user_data     InOut: User data
 * @param slot_nbr      In:    Slot number
 * @param subslot_nbr   In:    Subslot number
 * @param submodule_id  In:    Submodule id
 * @param size          Out:   Size of pnio data.
 *                             Not modified on error.
 * @param iops          Out:   Provider status. If for example
 *                             a sensor is failing or a short
 *                             circuit is detected on digital
 *                             input this shall be set to BAD.
 *                             Not modified on error.
 * @return Reference to inputdata buffer, NULL on error
 */
uint8_t * app_data_inputdata_getbuffer (
   void * user_data,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   uint16_t * size,
   uint8_t * iops);

/**
 * Find address to data buffer, so the stack can write outputdata from PLC.
 *
 * This function is not called for the DAP submodules (slot_nbr 0).
 *
 * @param user_data     InOut: User data
 * @param slot_nbr      In:    Slot number
 * @param subslot_nbr   In:    Subslot number
 * @param submodule_id  In:    Submodule id
 * @param size          Out:   Number of bytes expected to be written
 * @return Reference to outputdata buffer, NULL on error
 */
uint8_t * app_data_outputdata_getbuffer (
   void * user_data,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   uint16_t * size);

/**
 * Run application specific code after the stack has written data from PLC.
 *
 * This function is not called for the DAP submodules (slot_nbr 0).
 * Will only be called if the IOPS is GOOD.
 *
 * @param user_data     InOut: User data
 * @param slot_nbr      In:    Slot number
 * @param subslot_nbr   In:    Subslot number
 * @param submodule_id  In:    Submodule id
 */
void app_data_outputdata_finalize (
   void * user_data,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id);

/**
 * Set default outputs for all subslots.
 *
 * For the sample application this means that
 * LED 1 is turned off.
 *
 * @param user_data                InOut: User data
 * @return 0 on success, -1 on error
 */
int app_data_set_default_outputs (void * user_data);

/**
 * Handle reset command from PLC
 *
 * See pnet_reset_ind() for description of parameters.
 *
 * @param user_data                InOut: User data
 * @param should_reset_application In:    True if the user should reset the
 *                                        application data.
 * @param reset_mode               In:    Detailed reset information.
 * @return 0 on success, -1 on error
 */
int app_data_reset (
   void * user_data,
   bool should_reset_application,
   uint16_t reset_mode);

/**
 * Handle Profinet LED signal command from PLC
 *
 * @param user_data        InOut: User data
 * @param led_state        In:    True if the signal LED should be on.
 * @return 0 on success, -1 on error
 */
int app_data_signal_led (void * user_data, bool led_state);

/**
 * Intitialise the data storage
 *
 * @param user_data        InOut: User data
 * @param app              InOut: Application handle
 */
void app_data_init (void * user_data, app_data_t * app);

/**
 * Prepare the data storage for next cyclic data loop
 *
 * @param user_data        InOut: User data
 * @param app              InOut: Application handle
 */
void app_data_pre (void * user_data, app_data_t * app);

/**
 * Write parameter index for a subslot
 *
 * @param user_data     InOut: User data
 * @param slot_nbr      In:    Slot number
 * @param subslot_nbr   In:    Subslot number
 * @param submodule_id  In:    Submodule id
 * @param index         In:    Parameter index
 * @param data          In:    New parameter value
 * @param write_length  In:    Length of parameter data
 * @return 0 on success, -1 on error
 */
int app_data_write_parameter (
   void * user_data,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   uint32_t index,
   const uint8_t * data,
   uint16_t write_length);

/**
 * Read parameter index from a subslot
 *
 * @param user_data     InOut: User data
 * @param slot_nbr      In:    Slot number
 * @param subslot_nbr   In:    Subslot number
 * @param submodule_id  In:    Submodule id
 * @param index         In:    Parameter index
 * @param data          In:    Reference to parameter data
 * @param length        InOut: The maximum (in) and actual (out) length in
 *                             bytes of the data.
 * @return 0 on success, -1 on error
 */
int app_data_read_parameter (
   void * user_data,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   uint32_t index,
   uint8_t ** data,
   uint16_t * length);

#ifdef __cplusplus
}
#endif

#endif /* APP_DATA_API_H */
