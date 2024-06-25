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
 * Design requires unique submodule IDs and unique parameter indexes.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <pnet_api.h>

#define APP_GSDML_API 0

#define APP_GSDML_DEFAULT_STATION_NAME "lan9662-dev"

/* GSDML tag: VendorID */
#define APP_GSDML_VENDOR_ID 0x0493

/* GSDML tag: DeviceID */
#define APP_GSDML_DEVICE_ID 0x9662

/* Used in DCP communication */
#define APP_GSDML_OEM_VENDOR_ID 0xcafe
#define APP_GSDML_OEM_DEVICE_ID 0xee02

/* Used in I&M0 */
#define APP_GSDML_IM_HARDWARE_REVISION 3
#define APP_GSDML_IM_VERSION_MAJOR     1
#define APP_GSDML_IM_VERSION_MINOR     2

/* Allowed: 'V', 'R', 'P', 'U', 'T' */
#define APP_GSDML_SW_REV_PREFIX       'V'
#define APP_GSDML_PROFILE_ID          0x1234
#define APP_GSDML_PROFILE_SPEC_TYPE   0x5678
#define APP_GSDML_IM_REVISION_COUNTER 0 /* Typically 0 */

/* Note: You need to read out the actual hardware serial number instead */
#define APP_GSDML_EXAMPLE_SERIAL_NUMBER "007"

/* Initial values. Can be overwritten by PLC */
#define APP_GSDML_TAG_FUNCTION "my function"
#define APP_GSDML_TAG_LOCATION "my location"
#define APP_GSDML_IM_DATE      "2022-03-01 10:03"
#define APP_GSDML_DESCRIPTOR   "my descriptor"
#define APP_GSDML_SIGNATURE    ""

/* GSDML tag: Writeable_IM_Records */
#define APP_GSDML_IM_SUPPORTED                                                 \
   (PNET_SUPPORTED_IM1 | PNET_SUPPORTED_IM2 | PNET_SUPPORTED_IM3)

/* GSDML tag: OrderNumber */
#define APP_GSDML_ORDER_ID "12345 Abcdefghijk"
/* GSDML tag: ModuleInfo / Name */
#define APP_GSDML_PRODUCT_NAME "P-Net LAN9662 Sample"

/* GSDML tag: MinDeviceInterval */
#define APP_GSDML_MIN_DEVICE_INTERVAL 32 /* 1ms */

#define APP_GSDML_DIAG_CUSTOM_USI 0x1234

/* See "Specification for GSDML" 8.26 LogBookEntryItem for allowed values */
#define APP_GSDML_LOGBOOK_ERROR_CODE   0x20 /* Manufacturer specific */
#define APP_GSDML_LOGBOOK_ERROR_DECODE 0x82 /* Manufacturer specific */
#define APP_GSDML_LOGBOOK_ERROR_CODE_1 PNET_ERROR_CODE_1_FSPM
#define APP_GSDML_LOGBOOK_ERROR_CODE_2 0x00       /* Manufacturer specific */
#define APP_GSDML_LOGBOOK_ENTRY_DETAIL 0xFEE1DEAD /* Manufacturer specific */

#define APP_GSDML_MAX_SUBMODULES 20

#define APP_GSDML_DEFAULT_MAUTYPE 0x10 /* Copper 100 Mbit/s Full duplex */

#define APP_GSDML_ALARM_PAYLOAD_SIZE 1 /* bytes */

#define APP_GSDML_MOD_ID_DIGITAL_IN_1x8     0x00001001
#define APP_GSDML_MOD_ID_DIGITAL_OUT_1x8    0x00001002
#define APP_GSDML_MOD_ID_DIGITAL_IN_1x64    0x00001003
#define APP_GSDML_MOD_ID_DIGITAL_IN_2x32a   0x00001004
#define APP_GSDML_MOD_ID_DIGITAL_IN_2x32b   0x00001005
#define APP_GSDML_MOD_ID_DIGITAL_IN_1x800   0x00001006
#define APP_GSDML_MOD_ID_DIGITAL_OUT_1x64   0x00001007
#define APP_GSDML_MOD_ID_DIGITAL_OUT_2x32a  0x00001008
#define APP_GSDML_MOD_ID_DIGITAL_OUT_2x32b  0x00001009
#define APP_GSDML_MOD_ID_DIGITAL_OUT_1x800  0x0000100A
#define APP_GSDML_MOD_ID_DIGITAL_IN_PORT_A  0x0000100B
#define APP_GSDML_MOD_ID_DIGITAL_OUT_PORT_A 0x0000100C

#define APP_GSDML_SUBMOD_ID_DIGITAL_IN_1x8     0x00002001
#define APP_GSDML_SUBMOD_ID_DIGITAL_OUT_1x8    0x00002002
#define APP_GSDML_SUBMOD_ID_DIGITAL_IN_1x64    0x00002003
#define APP_GSDML_SUBMOD_ID_DIGITAL_IN_2x32a   0x00002004
#define APP_GSDML_SUBMOD_ID_DIGITAL_IN_2x32b   0x00002005
#define APP_GSDML_SUBMOD_ID_DIGITAL_IN_1x800   0x00002006
#define APP_GSDML_SUBMOD_ID_DIGITAL_OUT_1x64   0x00002007
#define APP_GSDML_SUBMOD_ID_DIGITAL_OUT_2x32a  0x00002008
#define APP_GSDML_SUBMOD_ID_DIGITAL_OUT_2x32b  0x00002009
#define APP_GSDML_SUBMOD_ID_DIGITAL_OUT_1x800  0x0000200A
#define APP_GSDML_SUBMOD_ID_DIGITAL_IN_PORT_A  0x0000200B
#define APP_GSDML_SUBMOD_ID_DIGITAL_OUT_PORT_A 0x0000200C

#define APP_GSDML_SIZE_DIGITAL_IN_1x8     1
#define APP_GSDML_SIZE_DIGITAL_OUT_1x8    1
#define APP_GSDML_SIZE_DIGITAL_IN_1x64    8
#define APP_GSDML_SIZE_DIGITAL_IN_2x32a   8
#define APP_GSDML_SIZE_DIGITAL_IN_2x32b   8
#define APP_GSDML_SIZE_DIGITAL_IN_1x800   100
#define APP_GSDML_SIZE_DIGITAL_OUT_1x64   8
#define APP_GSDML_SIZE_DIGITAL_OUT_2x32a  8
#define APP_GSDML_SIZE_DIGITAL_OUT_2x32b  8
#define APP_GSDML_SIZE_DIGITAL_OUT_1x800  100
#define APP_GSDML_SIZE_DIGITAL_IN_PORT_A  4
#define APP_GSDML_SIZE_DIGITAL_OUT_PORT_A 4

#define APP_GSDML_MAX_SUBMODULE_DATA_SIZE 100

/* TODO support virtual and pluggable submodules. */
typedef struct cfg_module
{
   uint32_t id;
   const char * name;
   uint32_t fixed_slot;   /* Set to UINT32_MAX if not fixed */
   uint32_t submodules[]; /* Variable length, ends with 0*/
} app_gsdml_module_t;

typedef struct app_gsdml_submodule
{
   uint32_t id;
   const char * name;
   uint32_t api;
   uint32_t fixed_subslot; /* Set to UINT32_MAX if not fixed */
   pnet_submodule_dir_t data_dir;
   uint16_t insize;
   uint16_t outsize;
   uint16_t parameters[]; /* Variable length, ends with 0 */
} app_gsdml_submodule_t;

typedef struct app_gsdml_param
{
   uint32_t index;
   const char * name;
   uint16_t length;
} app_gsdml_param_t;

/**
 * Get array of supported modules
 * @param array_len  Out: Length of array
 * @return Modules array
 */
const app_gsdml_module_t ** app_gsdml_get_modules (uint32_t * array_len);

/**
 * Get array of supported submodules
 * @param array_len  Out: Length of array
 * @return Submodules array
 */
const app_gsdml_submodule_t ** app_gsdml_get_submodules (uint32_t * array_len);

/**
 * Get array of supported parameters
 * @param array_len  Out: Length of array
 * @return Parameters array
 */
const app_gsdml_param_t ** app_gsdml_get_parameters (uint32_t * array_len);

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
