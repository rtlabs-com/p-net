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

#ifndef APP_DATA_H
#define APP_DATA_H

/**
 * @file
 * @brief Sample application data interface
 *
 * Functions for:
 * - Getting input data (Button 1 and counter value)
 * - Setting output data (LED 1)
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

/**
 * Get application specific PNIO input data (for sending to PLC)
 *
 * The main sample application keeps track
 * of button so it is a parameter to this function.
 *
 * This function is not called for the DAP submodules (slot_nbr==0).
 *
 * @param slot_nbr      In:  Slot number
 * @param subslot_nbr   In:  Subslot number
 * @param submodule_id  In:  Submodule id
 * @param button_state  In:  State of button 1
 * @param size          Out: Size of pnio data.
 *                           Not modified on error.
 * @param iops          Out: Provider status. If for example
 *                           a sensor is failing or a short
 *                           circuit is detected on digital
 *                           input this shall be set to BAD.
 *                           Not modified on error.
 * @return Reference to PNIO data, NULL on error
 */
uint8_t * app_data_get_input_data (
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   bool button_state,
   uint16_t * size,
   uint8_t * iops);

/**
 * Set application specific PNIO output data (received from PLC)
 *
 * This function is not called for the DAP submodules (slot_nbr==0).
 *
 * @param slot_nbr      In:  Slot number
 * @param subslot_nbr   In:  Subslot number
 * @param submodule_id  In:  Submodule id
 * @param data          In:  Reference to output data
 * @param size          In:  Length of output data
 * @return 0 on success, -1 on error
 */
int app_data_set_output_data (
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   uint8_t * data,
   uint16_t size);

/**
 * Set default outputs for all subslots.
 *
 * For the sample application this means that
 * LED 1 is turned off.
 *
 * @return 0 on success, -1 on error
 */
int app_data_set_default_outputs (void);

/**
 * Write parameter index for a subslot
 *
 * @param slot_nbr      In:  Slot number
 * @param subslot_nbr   In:  Subslot number
 * @param submodule_id  In:  Submodule id
 * @param index         In:  Parameter index
 * @param data          In:  New parameter value
 * @param write_length  In:  Length of parameter data
 * @return 0 on success, -1 on error
 */
int app_data_write_parameter (
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   uint32_t index,
   const uint8_t * data,
   uint16_t write_length);

/**
 * Read parameter index from a subslot
 *
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
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   uint32_t index,
   uint8_t ** data,
   uint16_t * length);

#ifdef __cplusplus
}
#endif

#endif /* APP_DATA_H */
