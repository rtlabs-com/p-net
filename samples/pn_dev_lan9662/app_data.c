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

#include "app_data.h"
#include "app_shm.h"
#include "app_utils.h"
#include "app_gsdml.h"
#include "app_log.h"
#include "sampleapp_common.h"
#include "osal.h"
#include "pnal.h"
#include <pnet_api.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * IO FPGA address mapping.
 */
#define APP_DATA_FPGA_START_ADDRESS 0x100
#define ALIGN32(x)                  ((((x)) + 3) & ~3)

#define APP_FPGA_ADDRESS_DIGITAL_IN_1x8 (APP_DATA_FPGA_START_ADDRESS)
#define APP_FPGA_ADDRESS_DIGITAL_OUT_1x8                                       \
   (APP_FPGA_ADDRESS_DIGITAL_IN_1x8 + ALIGN32 (APP_GSDML_SIZE_DIGITAL_IN_1x8))
#define APP_FPGA_ADDRESS_DIGITAL_IN_1x64                                       \
   (APP_FPGA_ADDRESS_DIGITAL_OUT_1x8 + ALIGN32 (APP_GSDML_SIZE_DIGITAL_OUT_1x8))
#define APP_FPGA_ADDRESS_DIGITAL_IN_2x32a                                      \
   (APP_FPGA_ADDRESS_DIGITAL_IN_1x64 + ALIGN32 (APP_GSDML_SIZE_DIGITAL_IN_1x64))
#define APP_FPGA_ADDRESS_DIGITAL_IN_2x32b                                      \
   (APP_FPGA_ADDRESS_DIGITAL_IN_2x32a +                                        \
    ALIGN32 (APP_GSDML_SIZE_DIGITAL_IN_2x32a))
#define APP_FPGA_ADDRESS_DIGITAL_IN_1x800                                      \
   (APP_FPGA_ADDRESS_DIGITAL_IN_2x32b +                                        \
    ALIGN32 (APP_GSDML_SIZE_DIGITAL_IN_2x32b))
#define APP_FPGA_ADDRESS_DIGITAL_OUT_1x64                                      \
   (APP_FPGA_ADDRESS_DIGITAL_IN_1x800 +                                        \
    ALIGN32 (APP_GSDML_SIZE_DIGITAL_IN_1x800))
#define APP_FPGA_ADDRESS_DIGITAL_OUT_2x32a                                     \
   (APP_FPGA_ADDRESS_DIGITAL_OUT_1x64 +                                        \
    ALIGN32 (APP_GSDML_SIZE_DIGITAL_OUT_1x64))
#define APP_FPGA_ADDRESS_DIGITAL_OUT_2x32b                                     \
   (APP_FPGA_ADDRESS_DIGITAL_OUT_2x32a +                                       \
    ALIGN32 (APP_GSDML_SIZE_DIGITAL_OUT_2x32a))
#define APP_FPGA_ADDRESS_DIGITAL_OUT_1x800                                     \
   (APP_FPGA_ADDRESS_DIGITAL_OUT_2x32b +                                       \
    ALIGN32 (APP_GSDML_SIZE_DIGITAL_OUT_2x32b))

#define APP_FPGA_ADDRESS_DIGITAL_IN_PORT_A  0x200
#define APP_FPGA_ADDRESS_DIGITAL_OUT_PORT_A 0x10

typedef struct submodule_fpga_config
{
   uint16_t address;
   const uint8_t * default_data;
} submodule_fpga_config_t;

typedef struct submodule
{
   const app_gsdml_submodule_t * info;

   /* shm -> pnet*/
   app_shm_t * indata_shm;
   uint8_t * in_data;

   /* pnet -> shm */
   app_shm_t * outdata_shm;
   uint8_t * out_data;

   /* FPGA address for full offload mode */
   const submodule_fpga_config_t * fpga_config;

} submodule_t;

typedef struct fpga_map
{
   uint32_t submodule_id;
   submodule_fpga_config_t fpga_config;
} fpga_map_t;

static const uint8_t app_data_default[APP_GSDML_MAX_SUBMODULE_DATA_SIZE] = {0};

/**
 * Configuration of FPGA addresses for the different
 * submodules when running full offload.
 */
static const fpga_map_t fpga[] = {
   {.submodule_id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_1x8,
    .fpga_config =
       {.address = APP_FPGA_ADDRESS_DIGITAL_IN_1x8,
        .default_data = app_data_default}},
   {.submodule_id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT_1x8,
    .fpga_config =
       {.address = APP_FPGA_ADDRESS_DIGITAL_OUT_1x8,
        .default_data = app_data_default}},
   {.submodule_id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_1x64,
    .fpga_config =
       {.address = APP_FPGA_ADDRESS_DIGITAL_IN_1x64,
        .default_data = app_data_default}},
   {.submodule_id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_2x32a,
    .fpga_config =
       {.address = APP_FPGA_ADDRESS_DIGITAL_IN_2x32a,
        .default_data = app_data_default}},
   {.submodule_id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_2x32b,
    .fpga_config =
       {.address = APP_FPGA_ADDRESS_DIGITAL_IN_2x32b,
        .default_data = app_data_default}},
   {.submodule_id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_1x800,
    .fpga_config =
       {.address = APP_FPGA_ADDRESS_DIGITAL_IN_1x800,
        .default_data = app_data_default}},
   {.submodule_id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT_1x64,
    .fpga_config =
       {.address = APP_FPGA_ADDRESS_DIGITAL_OUT_1x64,
        .default_data = app_data_default}},
   {.submodule_id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT_2x32a,
    .fpga_config =
       {.address = APP_FPGA_ADDRESS_DIGITAL_OUT_2x32a,
        .default_data = app_data_default}},
   {.submodule_id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT_2x32b,
    .fpga_config =
       {.address = APP_FPGA_ADDRESS_DIGITAL_OUT_2x32b,
        .default_data = app_data_default}},
   {.submodule_id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT_1x800,
    .fpga_config =
       {.address = APP_FPGA_ADDRESS_DIGITAL_OUT_1x800,
        .default_data = app_data_default}},

   {.submodule_id = APP_GSDML_SUBMOD_ID_DIGITAL_IN_PORT_A,
    .fpga_config =
       {.address = APP_FPGA_ADDRESS_DIGITAL_IN_PORT_A,
        .default_data = app_data_default}},

   {.submodule_id = APP_GSDML_SUBMOD_ID_DIGITAL_OUT_PORT_A,
    .fpga_config =
       {.address = APP_FPGA_ADDRESS_DIGITAL_OUT_PORT_A,
        .default_data = app_data_default}},
};

submodule_t submodules[APP_GSDML_MAX_SUBMODULES] = {0};

/**
 * Get a submodule using its submodule id
 * @param id            In:  Submodule id
 * @return Reference to submodule. NULL if id is not found.
 **/
static submodule_t * get_submodule_by_id (uint32_t id)
{
   uint16_t i;

   for (i = 0; i < NELEMENTS (submodules); i++)
   {
      if (submodules[i].info != NULL && submodules[i].info->id == id)
      {
         return &submodules[i];
      }
   }

   return NULL;
}

/**
 * Get submodule fpga config using its id.
 * @param id            In:  Submodule id
 * @return Reference to fgpa mapping. NULL if id is not found.
 **/
static const submodule_fpga_config_t * get_fpga_config_by_id (uint32_t id)
{
   uint32_t i = 0;

   while (i < NELEMENTS (fpga))
   {
      if (fpga[i].submodule_id == id)
      {
         return &fpga[i].fpga_config;
      }
      i++;
   }
   return NULL;
}

int app_data_get_fpga_info_by_id (
   uint32_t id,
   uint16_t * address,
   const uint8_t ** default_data)
{
   const submodule_fpga_config_t * config = get_fpga_config_by_id (id);

   if (config != NULL)
   {
      *address = config->address;
      *default_data = config->default_data;
      return 0;
   }
   return -1;
}

/**
 * Allocate a submodule from submodules array.
 * The returned submodule is uninitialized.
 * @return Reference to submodule. NULL if allocation fails.
 **/
static submodule_t * alloc_submodule (void)
{
   uint32_t i = 0;

   while (i < NELEMENTS (submodules))
   {
      if (submodules[i].info == NULL)
      {
         return &submodules[i];
      }
      i++;
   }
   return NULL;
}

/**
 * Initialize submodule
 * Set module and submodule configurations.
 * Configure shared memory regions.
 * Allocate buffers.
 * Will generate assert on failure.
 * @param module_cfg       In:  Module configuration
 * @param submodule_cfg    In:  Submodule configuration
 * @param app_mode         In:  Application mode
 **/
static void app_data_init_submodule (
   const app_gsdml_module_t * module_cfg,
   const app_gsdml_submodule_t * submodule_cfg,
   app_mode_t app_mode)
{
   submodule_t * submodule;

   CC_ASSERT (module_cfg != NULL);
   CC_ASSERT (submodule_cfg != NULL);

   submodule = alloc_submodule();
   CC_ASSERT (submodule != NULL);

   submodule->info = submodule_cfg;
   if (app_mode == MODE_HW_OFFLOAD_FULL)
   {
      submodule->fpga_config = get_fpga_config_by_id (submodule_cfg->id);
      CC_ASSERT (submodule->fpga_config != NULL);
   }
   else
   {
      submodule->fpga_config = NULL;
   }

   if (submodule->info->insize > 0)
   {
      submodule->in_data = malloc (submodule->info->insize);
      CC_ASSERT (submodule->in_data != NULL);

      if (app_mode != MODE_HW_OFFLOAD_FULL)
      {
         submodule->indata_shm = app_shm_create_input (
            submodule->info->name,
            module_cfg->fixed_slot,
            submodule_cfg->fixed_subslot,
            submodule->info->insize);
         CC_ASSERT (submodule->indata_shm != NULL);
      }
      else
      {
         APP_LOG_INFO (
            "[%u,%u,\"%s\"] mapped to FPGA address 0x%x\n",
            module_cfg->fixed_slot,
            submodule_cfg->fixed_subslot,
            submodule->info->name,
            submodule->fpga_config->address);
      }
   }

   if (submodule->info->outsize > 0)
   {
      submodule->out_data = malloc (submodule->info->outsize);
      CC_ASSERT (submodule->out_data != NULL);

      if (app_mode != MODE_HW_OFFLOAD_FULL)
      {
         submodule->outdata_shm = app_shm_create_output (
            submodule->info->name,
            module_cfg->fixed_slot,
            submodule_cfg->fixed_subslot,
            submodule->info->outsize);
         CC_ASSERT (submodule->outdata_shm != NULL);
      }
      else
      {
         APP_LOG_INFO (
            "[%u,%u,\"%s\"] mapped to FPGA address 0x%x\n",
            module_cfg->fixed_slot,
            submodule_cfg->fixed_subslot,
            submodule->info->name,
            submodule->fpga_config->address);
      }
   }
}

int app_data_init (app_mode_t app_mode)
{
   uint16_t i, j;
   const app_gsdml_module_t * module_cfg;
   const app_gsdml_submodule_t * submodule_cfg;
   uint32_t modules_array_len;
   const app_gsdml_module_t ** app_gsdml_modules;

   app_gsdml_modules = app_gsdml_get_modules (&modules_array_len);

   for (i = 0; i < modules_array_len; i++)
   {
      module_cfg = app_gsdml_modules[i];

      j = 0;
      while (module_cfg->submodules[j] != 0)
      {
         submodule_cfg =
            app_gsdml_get_submodule_cfg (module_cfg->submodules[j]);
         app_data_init_submodule (module_cfg, submodule_cfg, app_mode);
         j++;
      }
   }
   return 0;
}

uint8_t * app_data_get_input_data (
   uint32_t submodule_id,
   uint16_t * size,
   uint8_t * iops)
{
   int error = -1;
   submodule_t * submodule;

   if (size == NULL || iops == NULL)
   {
      return NULL;
   }

   submodule = get_submodule_by_id (submodule_id);

   if (submodule == NULL)
   {
      /* Handle unsupported submodules part of process data */
      *iops = PNET_IOXS_BAD;
      return NULL;
   }

   *size = submodule->info->insize;
   if (submodule->fpga_config == NULL)
   {
      error = app_shm_read (submodule->indata_shm, submodule->in_data, *size);
      CC_ASSERT (error == 0);
   }
   else
   {
      /* Input is mapped to FPGA. Data is not available to
       * application. Use default value set status.
       * P-Net needs an initial value during initialization.
       */
      memcpy (submodule->in_data, submodule->fpga_config->default_data, *size);
   }
   *iops = PNET_IOXS_GOOD;

   return submodule->in_data;
}

uint8_t * app_data_get_output_data_buffer (
   uint32_t submodule_id,
   uint16_t * size)
{
   submodule_t * submodule = get_submodule_by_id (submodule_id);

   CC_ASSERT (size != NULL);
   if (submodule == NULL)
   {
      *size = 0;
      return NULL;
   }

   *size = submodule->info->outsize;
   return submodule->out_data;
}

int app_data_set_output_data (
   uint32_t submodule_id,
   uint8_t * data,
   uint16_t size)
{
   int error = 0;
   submodule_t * submodule = get_submodule_by_id (submodule_id);

   if (data == NULL || submodule == NULL || size == 0)
   {
      return -1;
   }

   if (submodule->fpga_config == NULL)
   {
      /* Only write shared memory if output is not mapped
       * to FPGA.
       */
      error = app_shm_write (submodule->outdata_shm, data, size);
   }

   return error;
}

int app_data_set_default_outputs (void)
{
   uint16_t i;

   for (i = 0; i < NELEMENTS (submodules); i++)
   {
      if (submodules[i].outdata_shm != NULL)
      {
         if (submodules[i].fpga_config == NULL)
         {
            /* Only write shared memory if output is not mapped
             * to FPGA.
             */
            app_shm_reset (submodules[i].outdata_shm);
         }
      }
   }
   return 0;
}
