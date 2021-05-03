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

#ifndef APP_GSDML_H
#define APP_GSDML_H

/**
 * @file
 * @brief Device properties defined by the GSDML device definition
 *
 * Functions for getting module, submodule and parameter
 * configurations using their ids.
 *
 * Important:
 * Any change in this file may require an update of the GSDML file.
 * Note that when the GSDML file is updated it has to be reloaded
 * in your Profinet engineering tool. PLC applications may be affected.
 *
 * Design requires unique submodule ids.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <pnet_api.h>

#define APP_GSDML_API 0

#define APP_GSDML_DEFAULT_STATION_NAME "rt-labs-dev"

/* GSDML tag: VendorID */
#define APP_GSDML_VENDOR_ID 0xfeed

/* GSDML tag: DeviceID */
#define APP_GSDML_DEVICE_ID 0xbeef

/* Allowed: 'V', 'R', 'P', 'U', 'T' */
#define APP_GSDML_SW_REV_PREFIX     'V'
#define APP_GSDML_PROFILE_ID        0x1234
#define APP_GSDML_PROFILE_SPEC_TYPE 0x5678

/* GSDML tag: Writeable_IM_Records */
#define APP_GSDML_IM_SUPPORTED                                                 \
   (PNET_SUPPORTED_IM1 | PNET_SUPPORTED_IM2 | PNET_SUPPORTED_IM3)

/* GSDML tag: OrderNumber */
#define APP_GSDML_ORDER_ID "12345 Abcdefghijk"
/* GSDML tag: ModuleInfo / Name */
#define APP_GSDML_PRODUCT_NAME "P-Net Sample Application"

/* GSDML tag: MinDeviceInterval */
#define APP_GSDML_MIN_DEVICE_INTERVAL 32 /* 1ms */

#define APP_GSDML_DIAG_CUSTOM_USI 0x1234

/* See "Specification for GSDML" 8.26 LogBookEntryItem for allowed values */
#define APP_GSDML_LOGBOOK_ERROR_CODE   0x20 /* Manufacturer specific */
#define APP_GSDML_LOGBOOK_ERROR_DECODE 0x82 /* Manufacturer specific */
#define APP_GSDML_LOGBOOK_ERROR_CODE_1 PNET_ERROR_CODE_1_FSPM
#define APP_GSDML_LOGBOOK_ERROR_CODE_2 0x00       /* Manufacturer specific */
#define APP_GSDML_LOGBOOK_ENTRY_DETAIL 0xFEE1DEAD /* Manufacturer specific */

#define APP_GSDM_PARAMETER_1_IDX  123
#define APP_GSDM_PARAMETER_2_IDX  124
#define APP_GSDM_PARAMETER_LENGTH 4

typedef struct cfg_module
{
   uint32_t id;
   const char * name;
   uint32_t submodules[]; /* Variable length, ends with 0*/
} app_gsdml_module_t;

typedef struct app_gsdml_submodule
{
   uint32_t id;
   const char * name;
   uint32_t api;
   pnet_submodule_dir_t data_dir;
   uint16_t insize;
   uint16_t outsize;
   uint16_t parameters[]; /* Variable length, ends with 0 */
} app_gsdml_submodule_t;

typedef struct
{
   uint32_t index;
   const char * name;
   uint16_t length;
} app_gsdml_param_t;

#define APP_GSDML_MOD_ID_8_0_DIGITAL_IN     0x00000030
#define APP_GSDML_MOD_ID_0_8_DIGITAL_OUT    0x00000031
#define APP_GSDML_MOD_ID_8_8_DIGITAL_IN_OUT 0x00000032
#define APP_GSDML_SUBMOD_ID_DIGITAL_IN      0x00000130
#define APP_GSDML_SUBMOD_ID_DIGITAL_OUT     0x00000131
#define APP_GSDML_SUBMOD_ID_DIGITAL_IN_OUT  0x00000132

#define APP_GSDML_INPUT_DATA_SIZE   1 /* bytes, for digital inputs data */
#define APP_GSDML_OUTPUT_DATA_SIZE  1 /* bytes, for digital outputs data */
#define APP_GSDM_ALARM_PAYLOAD_SIZE 1 /* bytes */

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
   .insize = APP_GSDML_INPUT_DATA_SIZE,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t submod_digital_out = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT,
   .name = "Digital Output",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_OUTPUT,
   .insize = 0,
   .outsize = APP_GSDML_OUTPUT_DATA_SIZE,
   .parameters = {0}};

static const app_gsdml_submodule_t submod_digital_inout = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_OUT,
   .name = "Digital Input/Output",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_IO,
   .insize = APP_GSDML_INPUT_DATA_SIZE,
   .outsize = APP_GSDML_OUTPUT_DATA_SIZE,
   .parameters = {APP_GSDM_PARAMETER_1_IDX, APP_GSDM_PARAMETER_2_IDX, 0}};

/* List of supported modules */
static const app_gsdml_module_t * app_gsdml_modules[] =
   {&dap_1, &module_digital_in, &module_digital_out, &module_digital_in_out};

/* List of supported submodules */
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
};

/* List of supported parameters.
 * Note that paramerers are submodule attribute.
 * This list contain all parameters while each
 * submodule list its supported parameters using
 * their indexes.
 */
static app_gsdml_param_t app_gsdml_parameters[] = {
   {
      .index = APP_GSDM_PARAMETER_1_IDX,
      .name = "Demo 1",
      .length = APP_GSDM_PARAMETER_LENGTH,
   },
   {
      .index = APP_GSDM_PARAMETER_2_IDX,
      .name = "Demo 2",
      .length = APP_GSDM_PARAMETER_LENGTH,
   }};

/**
 * Get module configuration from module id
 * @param module_id  In: Module id
 * @return Module configuration, NULL if not found
 */
const app_gsdml_module_t * app_gsdml_get_module_cfg (uint32_t module_id);

/**
 * Get submodule module configuration from submodule id
 * @param submodule_id  In: Submodule id
 * @return Submodule configuration, NULL if not found
 */
const app_gsdml_submodule_t * app_gsdml_get_submodule_cfg (
   uint32_t submodule_id);

/**
 * Get parameter configuration from parameter index
 * @param submodule_id  In: Submodule id
 * @param index         In: Parameters index
 * @return Parameter configuration, NULL if not found
 */
const app_gsdml_param_t * app_gsdml_get_parameter_cfg (
   uint32_t submodule_id,
   uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* APP_GSDML_H */
