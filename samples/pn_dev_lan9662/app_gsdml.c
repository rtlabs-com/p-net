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

/**
 * @file
 * @brief Device properties defined by the GSDML device definition
 *
 * Functions for getting module, submodule and parameter
 * configurations using their IDs.
 *
 * Important:
 * Any change in this file may require an update of the GSDML file.
 * Note that when the GSDML file is updated it has to be reloaded
 * in your Profinet engineering tool. PLC applications may be affected.
 *
 * Design requires unique submodule IDs.
 */

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

/************************* Modules ***************************/

static const app_gsdml_module_t dap_1 = {
   .id = PNET_MOD_DAP_IDENT,
   .name = "DAP 1",
   .fixed_slot = 0,
   .submodules = {
      PNET_SUBMOD_DAP_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_PORT_1_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_PORT_2_IDENT,
      0}};

static const app_gsdml_module_t module_di_1x8 = {
   .id = APP_GSDML_MOD_ID_DIGITAL_IN_1x8,
   .name = "DI 1x8",
   .fixed_slot = 1,
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_IN_1x8, 0},
};

static const app_gsdml_module_t module_do_1x8 = {
   .id = APP_GSDML_MOD_ID_DIGITAL_OUT_1x8,
   .name = "DO 1x8",
   .fixed_slot = 2,
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_OUT_1x8, 0}};

static const app_gsdml_module_t module_di_1x64 = {
   .id = APP_GSDML_MOD_ID_DIGITAL_IN_1x64,
   .name = "DI 1x64",
   .fixed_slot = 3,
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_IN_1x64, 0},
};

static const app_gsdml_module_t module_di_2x32a = {
   .id = APP_GSDML_MOD_ID_DIGITAL_IN_2x32a,
   .name = "DI 2x32a",
   .fixed_slot = 4,
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_IN_2x32a, 0},
};

static const app_gsdml_module_t module_di_2x32b = {
   .id = APP_GSDML_MOD_ID_DIGITAL_IN_2x32b,
   .name = "DI 2x32b",
   .fixed_slot = 5,
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_IN_2x32b, 0},
};

static const app_gsdml_module_t module_di_1x800 = {
   .id = APP_GSDML_MOD_ID_DIGITAL_IN_1x800,
   .name = "DI 1x800",
   .fixed_slot = 6,
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_IN_1x800, 0},
};

static const app_gsdml_module_t module_do_1x64 = {
   .id = APP_GSDML_MOD_ID_DIGITAL_OUT_1x64,
   .name = "DO 1x64",
   .fixed_slot = 7,
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_OUT_1x64, 0},
};

static const app_gsdml_module_t module_do_2x32a = {
   .id = APP_GSDML_MOD_ID_DIGITAL_OUT_2x32a,
   .name = "DO 2x32a",
   .fixed_slot = 8,
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_OUT_2x32a, 0},
};

static const app_gsdml_module_t module_do_2x32b = {
   .id = APP_GSDML_MOD_ID_DIGITAL_OUT_2x32b,
   .name = "DO 2x32b",
   .fixed_slot = 9,
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_OUT_2x32b, 0},
};

static const app_gsdml_module_t module_do_1x800 = {
   .id = APP_GSDML_MOD_ID_DIGITAL_OUT_1x800,
   .name = "DO 1x800",
   .fixed_slot = 10,
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_OUT_1x800, 0},
};

static const app_gsdml_module_t module_di_port_a = {
   .id = APP_GSDML_MOD_ID_DIGITAL_IN_PORT_A,
   .name = "DI Port A",
   .fixed_slot = 11,
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_IN_PORT_A, 0},
};

static const app_gsdml_module_t module_do_port_a = {
   .id = APP_GSDML_MOD_ID_DIGITAL_OUT_PORT_A,
   .name = "DO Port A",
   .fixed_slot = 12,
   .submodules = {APP_GSDML_SUBMOD_ID_DIGITAL_OUT_PORT_A, 0},
};
/************************* Submodules ***************************/

static const app_gsdml_submodule_t dap_identity_1 = {
   .name = "DAP Identity 1",
   .api = APP_GSDML_API,
   .id = PNET_SUBMOD_DAP_IDENT,
   .fixed_subslot = PNET_SUBSLOT_DAP_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_interface_1 = {
   .name = "DAP Interface 1",
   .api = APP_GSDML_API,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_IDENT,
   .fixed_subslot = PNET_SUBSLOT_DAP_INTERFACE_1_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_port_1 = {
   .name = "DAP Port 1",
   .api = APP_GSDML_API,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_PORT_1_IDENT,
   .fixed_subslot = PNET_SUBSLOT_DAP_INTERFACE_1_PORT_1_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_port_2 = {
   .name = "DAP Port 2",
   .api = APP_GSDML_API,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_PORT_2_IDENT,
   .fixed_subslot = PNET_SUBSLOT_DAP_INTERFACE_1_PORT_2_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_port_3 = {
   .name = "DAP Port 3",
   .api = APP_GSDML_API,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_PORT_3_IDENT,
   .fixed_subslot = PNET_SUBSLOT_DAP_INTERFACE_1_PORT_3_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t dap_port_4 = {
   .name = "DAP Port 4",
   .api = APP_GSDML_API,
   .id = PNET_SUBMOD_DAP_INTERFACE_1_PORT_4_IDENT,
   .fixed_subslot = PNET_SUBSLOT_DAP_INTERFACE_1_PORT_4_IDENT,
   .data_dir = PNET_DIR_NO_IO,
   .insize = 0,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t submodule_di_1x8 = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_1x8,
   .fixed_subslot = 1,
   .name = "Digital Input 1x8",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_INPUT,
   .insize = APP_GSDML_SIZE_DIGITAL_IN_1x8,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t submodule_do_1x8 = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT_1x8,
   .fixed_subslot = 1,
   .name = "Digital Output 1x8",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_OUTPUT,
   .insize = 0,
   .outsize = APP_GSDML_SIZE_DIGITAL_OUT_1x8,
   .parameters = {0}};

static const app_gsdml_submodule_t submodule_di_1x64 = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_1x64,
   .fixed_subslot = 1,
   .name = "Digital Input 1x64",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_INPUT,
   .insize = APP_GSDML_SIZE_DIGITAL_IN_1x64,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t submodule_di_2x32a = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_2x32a,
   .fixed_subslot = 1,
   .name = "Digital Input 2x32 a",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_INPUT,
   .insize = 8,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t submodule_di_2x32b = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_2x32b,
   .fixed_subslot = 1,
   .name = "Digital Input 2x32 b",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_INPUT,
   .insize = APP_GSDML_SIZE_DIGITAL_IN_2x32b,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t submodule_di_1x800 = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_1x800,
   .fixed_subslot = 1,
   .name = "Digital Input 1x800",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_INPUT,
   .insize = APP_GSDML_SIZE_DIGITAL_IN_1x800,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t submodule_do_1x64 = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT_1x64,
   .fixed_subslot = 1,
   .name = "Digital Output 1x64",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_OUTPUT,
   .insize = 0,
   .outsize = APP_GSDML_SIZE_DIGITAL_OUT_1x64,
   .parameters = {0}};

static const app_gsdml_submodule_t submodule_do_2x32a = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT_2x32a,
   .fixed_subslot = 1,
   .name = "Digital Output 2x32 a",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_OUTPUT,
   .insize = 0,
   .outsize = APP_GSDML_SIZE_DIGITAL_OUT_2x32a,
   .parameters = {0}};

static const app_gsdml_submodule_t submodule_do_2x32b = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT_2x32b,
   .fixed_subslot = 1,
   .name = "Digital Output 2x32 b",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_OUTPUT,
   .insize = 0,
   .outsize = APP_GSDML_SIZE_DIGITAL_OUT_2x32b,
   .parameters = {0}};

static const app_gsdml_submodule_t submodule_do_1x800 = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT_1x800,
   .fixed_subslot = 1,
   .name = "Digital Output 1x800",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_OUTPUT,
   .insize = 0,
   .outsize = APP_GSDML_SIZE_DIGITAL_OUT_1x800,
   .parameters = {0}};

static const app_gsdml_submodule_t submodule_di_port_a = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_PORT_A,
   .fixed_subslot = 1,
   .name = "Digital Input Port A",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_INPUT,
   .insize = APP_GSDML_SIZE_DIGITAL_IN_PORT_A,
   .outsize = 0,
   .parameters = {0}};

static const app_gsdml_submodule_t submodule_do_port_a = {
   .id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT_PORT_A,
   .fixed_subslot = 1,
   .name = "Digital Output Port A",
   .api = APP_GSDML_API,
   .data_dir = PNET_DIR_OUTPUT,
   .insize = 0,
   .outsize = APP_GSDML_SIZE_DIGITAL_OUT_PORT_A,
   .parameters = {0}};

/* List of supported modules */
static const app_gsdml_module_t * app_gsdml_modules[] = {
   &dap_1,
   &module_di_1x8,
   &module_do_1x8,
   &module_di_1x64,
   &module_di_2x32a,
   &module_di_2x32b,
   &module_di_1x800,
   &module_do_1x64,
   &module_do_2x32a,
   &module_do_2x32b,
   &module_do_1x800,
   &module_di_port_a,
   &module_do_port_a};

/* List of supported submodules */
static const app_gsdml_submodule_t * app_gsdml_submodules[] = {
   &dap_identity_1,
   &dap_interface_1,
   &dap_port_1,
   &dap_port_2,
   &dap_port_3,
   &dap_port_4,

   &submodule_di_1x8,
   &submodule_do_1x8,
   &submodule_di_1x64,
   &submodule_di_2x32a,
   &submodule_di_2x32b,
   &submodule_di_1x800,
   &submodule_do_1x64,
   &submodule_do_2x32a,
   &submodule_do_2x32b,
   &submodule_do_1x800,
   &submodule_di_port_a,
   &submodule_do_port_a};

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

const app_gsdml_module_t ** app_gsdml_get_modules (uint32_t * array_len)
{
   *array_len = NELEMENTS (app_gsdml_modules);
   return app_gsdml_modules;
}

const app_gsdml_submodule_t ** app_gsdml_get_submodules (uint32_t * array_len)
{
   *array_len = NELEMENTS (app_gsdml_submodules);
   return app_gsdml_submodules;
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
