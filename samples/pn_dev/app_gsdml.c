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

#include "sampleapp_common.h"
#include "app_utils.h"
#include "app_gsdml.h"
#include "app_log.h"
#include "osal.h"
#include "pnal.h"
#include <pnet_api.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************* Supported modules ***************************/

static const app_gsdml_module_t dap_1 = {
   .id = PNET_MOD_DAP_IDENT,
   .name = "DAP 1",
   .submodules = {
      PNET_SUBMOD_DAP_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_PORT_1_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_PORT_2_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_PORT_3_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_PORT_4_IDENT,
      0}};

static const app_gsdml_module_t module_digital_in = {
   .id = APP_GSDML_MOD_ID_8_0_DIGITAL_IN,
   .name = "DI 8xLogicLevel",
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_IN, 0},
};

static const app_gsdml_module_t module_digital_out = {
   .id = APP_GSDML_MOD_ID_0_8_DIGITAL_OUT,
   .name = "DO 8xLogicLevel",
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_OUT, 0}};

static const app_gsdml_module_t module_digital_in_out = {
   .id = APP_GSDML_MOD_ID_8_8_DIGITAL_IN_OUT,
   .name = "DIO 8xLogicLevel",
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_IN_OUT, 0}};

static const app_gsdml_module_t module_echo = {
   .id = APP_GSDML_MOD_ID_ECHO,
   .name = "Echo module",
   .submodules = {APP_GSDML_SUBMOD_ID_ECHO, 0}};

/******************* Supported submodules ************************/

static const app_gsdml_submodule_t dap_indentity_1 = {
   .name = "DAP Identity 1",
   .api = APP_GSDML_API,
   .id = PNET_SUBMOD_DAP_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_interface_1 = {
   .name = "DAP Interface 1",
   .api = APP_GSDML_API,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_port_1 = {
   .name = "DAP Port 1",
   .api = APP_GSDML_API,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_PORT_1_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_port_2 = {
   .name = "DAP Port 2",
   .api = APP_GSDML_API,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_PORT_2_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_port_3 = {
   .name = "DAP Port 3",
   .api = APP_GSDML_API,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_PORT_3_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_port_4 = {
   .name = "DAP Port 4",
   .api = APP_GSDML_API,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_PORT_4_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t submod_digital_in = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_IN,
   .name = "Digital Input",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_INPUT,
   .insize = APP_GSDML_INPUT_DATA_DIGITAL_SIZE,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t submod_digital_out = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT,
   .name = "Digital Output",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_OUTPUT,
   .insize = 0,
   .outsize = APP_GSDML_OUTPUT_DATA_DIGITAL_SIZE,
   .parameters = {0}};

static const app_gsdml_submodule_t submod_digital_inout = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_OUT,
   .name = "Digital Input/Output",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_IO,
   .insize = APP_GSDML_INPUT_DATA_DIGITAL_SIZE,
   .outsize = APP_GSDML_OUTPUT_DATA_DIGITAL_SIZE,
   .parameters = {APP_GSDML_PARAMETER_1_IDX, APP_GSDML_PARAMETER_2_IDX, 0}};

static const app_gsdml_submodule_t submod_echo = {
   .id = APP_GSDML_SUBMOD_ID_ECHO,
   .name = "Echo submodule",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_IO,
   .insize = APP_GSDML_INPUT_DATA_ECHO_SIZE,
   .outsize = APP_GSDML_OUTPUT_DATA_ECHO_SIZE,
   .parameters = {APP_GSDML_PARAMETER_ECHO_IDX, 0}};

/** List of supported modules */
static const app_gsdml_module_t * app_gsdml_modules[] = {
   &dap_1,
   &module_digital_in,
   &module_digital_out,
   &module_digital_in_out,
   &module_echo};

/** List of supported submodules */
static const app_gsdml_submodule_t * app_gsdml_submodules[] = {
   &dap_indentity_1,
   &dap_interface_1,
   &dap_port_1,
   &dap_port_2,
   &dap_port_3,
   &dap_port_4,

   &submod_digital_in,
   &submod_digital_out,
   &submod_digital_inout,

   &submod_echo,
};

/* List of supported parameters.
 * Note that parameters are submodule attributes.
 * This list contain all parameters while each
 * submodule list its supported parameters using
 * their indexes.
 */
static app_gsdml_param_t app_gsdml_parameters[] = {
   {
      .index = APP_GSDML_PARAMETER_1_IDX,
      .name = "Demo 1",
      .length = APP_GSDML_PARAMETER_LENGTH,
   },
   {
      .index = APP_GSDML_PARAMETER_2_IDX,
      .name = "Demo 2",
      .length = APP_GSDML_PARAMETER_LENGTH,
   },
   {
      .index = APP_GSDML_PARAMETER_ECHO_IDX,
      .name = "Echo gain setting",
      .length = APP_GSDML_PARAMETER_LENGTH,
   }};

const app_gsdml_module_t * app_gsdml_get_module_cfg (uint32_t id)
{
   uint32_t i;
   for (i = 0; i < NELEMENTS (app_gsdml_modules); i++)
   {
      if (app_gsdml_modules[i]->id == id)
      {
         return app_gsdml_modules[i];
      }
   }
   return NULL;
}

const app_gsdml_submodule_t * app_gsdml_get_submodule_cfg (uint32_t id)
{
   uint32_t i;
   for (i = 0; i < NELEMENTS (app_gsdml_submodules); i++)
   {
      if (app_gsdml_submodules[i]->id == id)
      {
         return app_gsdml_submodules[i];
      }
   }
   return NULL;
}

const app_gsdml_param_t * app_gsdml_get_parameter_cfg (
   uint32_t submodule_id,
   uint32_t index)
{
   uint16_t i;
   uint16_t j;

   const app_gsdml_submodule_t * submodule_cfg =
      app_gsdml_get_submodule_cfg (submodule_id);

   if (submodule_cfg == NULL)
   {
      /* Unsupported submodule id */
      return NULL;
   }

   /* Search for parameter index in submodule configuration */
   for (i = 0; submodule_cfg->parameters[i] != 0; i++)
   {
      if (submodule_cfg->parameters[i] == index)
      {
         /* Find parameter configuration */
         for (j = 0; j < NELEMENTS (app_gsdml_parameters); j++)
         {
            if (app_gsdml_parameters[j].index == index)
            {
               return &app_gsdml_parameters[j];
            }
         }
      }
   }

   return NULL;
}
