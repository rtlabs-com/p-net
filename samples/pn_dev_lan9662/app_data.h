/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifndef APP_DATA_H
#define APP_DATA_H

#include "sampleapp_common.h"

/**
 * @file
 * @brief Sample application data interface
 *
 * Functions for:
 * - Getting input data
 * - Setting output data
 * - Setting default output state
 *
 * In this LAN9662 sample application the submodule data
 * is mapped to shared memory found in /dev/shm on Linux.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize data submodules.
 * During the initialization the shared memory areas
 * supported by the application are created and the
 * mapping between the shared memory and the submodules
 * is configured.
 * @param app_mode  In:  Application mode
 * @return 0 on success, -1 on error
 */
int app_data_init (app_mode_t app_mode);

/**
 * Get PNIO input data using submodule id.
 * When this function is called the mapped shared
 * memory is copied to the submodule input buffer.
 * A reference to the the input buffer is returned.
 * @param submodule_id  In:  Submodule id
 * @param size          Out: Size of pnio data
 * @param iops          Out: Provider status. If for example
 *                           a sensor is failing or a short
 *                           circuit detected on digital
 *                           input this shall be set to BAD.
 *                           Set to BAD for unsupported submodules
 *                           or data can not be provided.
 * @return Reference to PNIO data, NULL on error
 */
uint8_t * app_data_get_input_data (
   uint32_t submodule_id,
   uint16_t * size,
   uint8_t * iops);

/**
 * Get FPGA data start address for submodule data
 * @param id            In:  Submodule id
 * @param address       Out: FPGA start address
 * @param default_data  Out: Submodule default data
 * @return 0 on success, -1 on error
 */
int app_data_get_fpga_info_by_id (
   uint32_t id,
   uint16_t * address,
   const uint8_t ** default_data);

/**
 * Get PNIO output data buffer using submodule id.
 * @param submodule_id  In:  Submodule id
 * @param size          Out: Size of pnio data. If submodule
 *                           is not supported size is set to 0.
 * @return Reference to PNIO data, NULL on error
 */
uint8_t * app_data_get_output_data_buffer (
   uint32_t submodule_id,
   uint16_t * size);

/**
 * Set PNIO output data using module id.
 * When this operation is called the data is
 * written the mapped shared memory.
 * @param submodule_id  In:  Submodule id
 * @param data          In:  Reference to output data
 * @param size          In:  Length of output data
 * @return 0 on success, -1 on error
 */
int app_data_set_output_data (
   uint32_t submodule_id,
   uint8_t * data,
   uint16_t size);

/**
 * Set default values for all outputs.
 * When this operation is called the mapped
 * shared memory is set to 0.
 * @return 0 on success, -1 on error
 */
int app_data_set_default_outputs (void);

#ifdef __cplusplus
}
#endif

#endif /* APP_DATA_H */
