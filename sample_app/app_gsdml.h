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
 * Design requires unique submodule IDs and unique parameter indexes.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <pnet_api.h>

#define APP_GSDML_API 0

#define APP_GSDML_DEFAULT_STATION_NAME "rt-labs-dev"

/* GSDML tag: VendorID */
#define APP_GSDML_VENDOR_ID 0x0493

/* GSDML tag: DeviceID */
#define APP_GSDML_DEVICE_ID 0x0002

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
#define APP_GSDML_PRODUCT_NAME "P-Net Sample Application"

/* GSDML tag: MinDeviceInterval */
#define APP_GSDML_MIN_DEVICE_INTERVAL 32 /* 1 ms */

#define APP_GSDML_DIAG_CUSTOM_USI 0x1234

/* See "Specification for GSDML" 8.26 LogBookEntryItem for allowed values */
#define APP_GSDML_LOGBOOK_ERROR_CODE   0x20 /* Manufacturer specific */
#define APP_GSDML_LOGBOOK_ERROR_DECODE 0x82 /* Manufacturer specific */
#define APP_GSDML_LOGBOOK_ERROR_CODE_1 PNET_ERROR_CODE_1_FSPM
#define APP_GSDML_LOGBOOK_ERROR_CODE_2 0x00       /* Manufacturer specific */
#define APP_GSDML_LOGBOOK_ENTRY_DETAIL 0xFEE1DEAD /* Manufacturer specific */

#define APP_GSDML_PARAMETER_1_IDX    123
#define APP_GSDML_PARAMETER_2_IDX    124
#define APP_GSDML_PARAMETER_ECHO_IDX 125

/* Use same size for all parameters in example */
#define APP_GSDML_PARAMETER_LENGTH 4

#define APP_GSDML_DEFAULT_MAUTYPE 0x10 /* Copper 100 Mbit/s Full duplex */

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

#define APP_GSDML_MOD_ID_8_0_DIGITAL_IN     0x00000030
#define APP_GSDML_MOD_ID_0_8_DIGITAL_OUT    0x00000031
#define APP_GSDML_MOD_ID_8_8_DIGITAL_IN_OUT 0x00000032
#define APP_GSDML_MOD_ID_ECHO               0x00000040
#define APP_GSDML_SUBMOD_ID_DIGITAL_IN      0x00000130
#define APP_GSDML_SUBMOD_ID_DIGITAL_OUT     0x00000131
#define APP_GSDML_SUBMOD_ID_DIGITAL_IN_OUT  0x00000132
#define APP_GSDML_SUBMOD_ID_ECHO            0x00000140
#define APP_GSDML_INPUT_DATA_DIGITAL_SIZE   1 /* bytes */
#define APP_GSDML_OUTPUT_DATA_DIGITAL_SIZE  1 /* bytes */
#define APP_GSDML_INPUT_DATA_ECHO_SIZE      8 /* bytes */
#define APP_GSDML_OUTPUT_DATA_ECHO_SIZE     APP_GSDML_INPUT_DATA_ECHO_SIZE
#define APP_GSDML_ALARM_PAYLOAD_SIZE        1 /* bytes */

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

#ifdef __cplusplus
}
#endif

#endif /* APP_GSDML_H */
