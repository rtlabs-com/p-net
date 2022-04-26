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

#include "app_gsdml_api.h"
#include "sampleapp_gsdml.h"

#include "app_log.h"
#include "osal.h"
#include <pnet_api.h>

#define GET_LOW_BYTE(id)  (id & 0xFF)
#define GET_HIGH_BYTE(id) ((id >> 8) & 0xFF)

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
   .api = PNET_API_NO_APPLICATION_PROFILE,
   .id = PNET_SUBMOD_DAP_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_interface_1 = {
   .name = "DAP Interface 1",
   .api = PNET_API_NO_APPLICATION_PROFILE,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_port_1 = {
   .name = "DAP Port 1",
   .api = PNET_API_NO_APPLICATION_PROFILE,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_PORT_1_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_port_2 = {
   .name = "DAP Port 2",
   .api = PNET_API_NO_APPLICATION_PROFILE,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_PORT_2_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_port_3 = {
   .name = "DAP Port 3",
   .api = PNET_API_NO_APPLICATION_PROFILE,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_PORT_3_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_port_4 = {
   .name = "DAP Port 4",
   .api = PNET_API_NO_APPLICATION_PROFILE,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_PORT_4_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t submod_digital_in = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_IN,
   .name = "Digital Input",
   .api = PNET_API_NO_APPLICATION_PROFILE,
   .data_dir = PNET_DIR_INPUT,
   .insize = APP_GSDML_INPUT_DATA_DIGITAL_SIZE,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t submod_digital_out = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT,
   .name = "Digital Output",
   .api = PNET_API_NO_APPLICATION_PROFILE,
   .data_dir = PNET_DIR_OUTPUT,
   .insize = 0,
   .outsize = APP_GSDML_OUTPUT_DATA_DIGITAL_SIZE,
   .parameters = {0}};

static const app_gsdml_submodule_t submod_digital_inout = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_OUT,
   .name = "Digital Input/Output",
   .api = PNET_API_NO_APPLICATION_PROFILE,
   .data_dir = PNET_DIR_IO,
   .insize = APP_GSDML_INPUT_DATA_DIGITAL_SIZE,
   .outsize = APP_GSDML_OUTPUT_DATA_DIGITAL_SIZE,
   .parameters = {APP_GSDML_PARAMETER_1_IDX, APP_GSDML_PARAMETER_2_IDX, 0}};

static const app_gsdml_submodule_t submod_echo = {
   .id = APP_GSDML_SUBMOD_ID_ECHO,
   .name = "Echo submodule",
   .api = PNET_API_NO_APPLICATION_PROFILE,
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

/******************** Public API ****************************/

int app_gsdml_update_config (pnet_cfg_t * cfg)
{

   /* Identification & Maintenance */

   cfg->im_0_data.im_vendor_id_hi = GET_HIGH_BYTE (APP_GSDML_VENDOR_ID);
   cfg->im_0_data.im_vendor_id_lo = GET_LOW_BYTE (APP_GSDML_VENDOR_ID);

   cfg->im_0_data.im_hardware_revision = APP_GSDML_IM_HARDWARE_REVISION;
   cfg->im_0_data.im_sw_revision_prefix = APP_GSDML_SW_REV_PREFIX;
   cfg->im_0_data.im_sw_revision_functional_enhancement =
      APP_GSDML_SW_REV_FUNCTIONAL_ENHANCEMENT;
   cfg->im_0_data.im_sw_revision_bug_fix = APP_GSDML_SW_REV_BUGFIX;
   cfg->im_0_data.im_sw_revision_internal_change =
      APP_GSDML_SW_REV_INTERNAL_CHANGE;
   cfg->im_0_data.im_revision_counter = APP_GSDML_IM_REVISION_COUNTER;
   cfg->im_0_data.im_profile_id = APP_GSDML_PROFILE_ID;
   cfg->im_0_data.im_profile_specific_type = APP_GSDML_PROFILE_SPEC_TYPE;
   cfg->im_0_data.im_version_major = 1; /** Always 1 */
   cfg->im_0_data.im_version_minor = 1; /** Always 1 */
   cfg->im_0_data.im_supported = APP_GSDML_IM_SUPPORTED;

   /* Serial number is handled elsewhere */
   snprintf (
      cfg->im_0_data.im_order_id,
      sizeof (cfg->im_0_data.im_order_id),
      "%s",
      APP_GSDML_ORDER_ID);
   snprintf (
      cfg->im_1_data.im_tag_function,
      sizeof (cfg->im_1_data.im_tag_function),
      "%s",
      APP_GSDML_TAG_FUNCTION);
   snprintf (
      cfg->im_1_data.im_tag_location,
      sizeof (cfg->im_1_data.im_tag_location),
      "%s",
      APP_GSDML_TAG_LOCATION);
   snprintf (
      cfg->im_2_data.im_date,
      sizeof (cfg->im_2_data.im_date),
      "%s",
      APP_GSDML_IM_DATE);
   snprintf (
      cfg->im_3_data.im_descriptor,
      sizeof (cfg->im_3_data.im_descriptor),
      "%s",
      APP_GSDML_DESCRIPTOR);
   snprintf (
      cfg->im_4_data.im_signature,
      sizeof (cfg->im_4_data.im_signature),
      "%s",
      APP_GSDML_SIGNATURE);

   /* Device configuration */
   cfg->device_id.vendor_id_hi = GET_HIGH_BYTE (APP_GSDML_VENDOR_ID);
   cfg->device_id.vendor_id_lo = GET_LOW_BYTE (APP_GSDML_VENDOR_ID);
   cfg->device_id.device_id_hi = GET_HIGH_BYTE (APP_GSDML_DEVICE_ID);
   cfg->device_id.device_id_lo = GET_LOW_BYTE (APP_GSDML_DEVICE_ID);
   cfg->oem_device_id.vendor_id_hi = GET_HIGH_BYTE (APP_GSDML_OEM_VENDOR_ID);
   cfg->oem_device_id.vendor_id_lo = GET_LOW_BYTE (APP_GSDML_OEM_VENDOR_ID);
   cfg->oem_device_id.device_id_hi = GET_HIGH_BYTE (APP_GSDML_OEM_DEVICE_ID);
   cfg->oem_device_id.device_id_lo = GET_LOW_BYTE (APP_GSDML_OEM_DEVICE_ID);

   snprintf (
      cfg->product_name,
      sizeof (cfg->product_name),
      "%s",
      APP_GSDML_PRODUCT_NAME);

   cfg->send_hello = true;

   /* Timing */
   cfg->min_device_interval = APP_GSDML_MIN_DEVICE_INTERVAL;

   snprintf (
      cfg->station_name,
      sizeof (cfg->station_name),
      "%s",
      APP_GSDML_DEFAULT_STATION_NAME);

   return 0;
}

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
