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

#ifndef APP_GSDML_API_H
#define APP_GSDML_API_H

/**
 * @file
 * @brief Application GSDML settings interface
 *
 * Functions for getting module, submodule and parameter
 * configurations using their IDs.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <pnet_api.h>

typedef struct app_gsdml_module
{
   uint32_t id;

   /** Module name */
   const char * name;

   /** Submodule IDs. Variable length, ends with 0. */
   uint32_t submodules[];
} app_gsdml_module_t;

typedef struct app_gsdml_submodule
{
   uint32_t id;

   /** Submodule name */
   const char * name;

   uint32_t api;
   pnet_submodule_dir_t data_dir;
   uint16_t insize;
   uint16_t outsize;

   /** Parameter indexes. See app_gsdml_parameters.
    * Variable length, ends with 0. */
   uint16_t parameters[];
} app_gsdml_submodule_t;

typedef struct
{
   uint32_t index;
   const char * name;
   uint16_t length;
} app_gsdml_param_t;

/**
 * Get module configuration from module ID
 * @param module_id  In: Module ID
 * @return Module configuration, NULL if not found
 */
const app_gsdml_module_t * app_gsdml_get_module_cfg (uint32_t module_id);

/**
 * Get submodule module configuration from submodule ID
 * @param submodule_id  In: Submodule ID
 * @return Submodule configuration, NULL if not found
 */
const app_gsdml_submodule_t * app_gsdml_get_submodule_cfg (
   uint32_t submodule_id);

/**
 * Get parameter configuration from parameter index
 * @param submodule_id  In: Submodule ID
 * @param index         In: Parameters index
 * @return Parameter configuration, NULL if not found
 */
const app_gsdml_param_t * app_gsdml_get_parameter_cfg (
   uint32_t submodule_id,
   uint32_t index);

/**
 * Update GSDML dependent values in the configuration.
 * @param cfg           InOut: Configuration to be updated
 * @return 0 on success, -1 on error
 */
int app_gsdml_update_config (pnet_cfg_t * cfg);

#ifdef __cplusplus
}
#endif

#endif /* APP_GSDML_API_H */
