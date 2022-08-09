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
#include "app_data.h"
#include "app_log.h"
#include "osal.h"
#include "pnal.h"
#include <pnet_api.h>
#include "pnet_lan9662_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Events handled by main task */
#define APP_EVENT_READY_FOR_DATA BIT (0)
#define APP_EVENT_TIMER          BIT (1)
#define APP_EVENT_ALARM          BIT (2)
#define APP_EVENT_ABORT          BIT (15)

/* Defines used for alarm demo functionality */
#define CHANNEL_ERRORTYPE_SHORT_CIRCUIT                       0x0001
#define CHANNEL_ERRORTYPE_LINE_BREAK                          0x0006
#define CHANNEL_ERRORTYPE_DATA_TRANSMISSION_IMPOSSIBLE        0x8000
#define CHANNEL_ERRORTYPE_NETWORK_COMPONENT_FUNCTION_MISMATCH 0x8008
#define EXTENDED_CHANNEL_ERRORTYPE_FRAME_DROPPED              0x8000
#define EXTENDED_CHANNEL_ERRORTYPE_MAUTYPE_MISMATCH           0x8001
#define EXTENDED_CHANNEL_ERRORTYPE_LINE_DELAY_MISMATCH        0x8002

#define APP_ALARM_USI                       0x0010
#define APP_DIAG_CHANNEL_NUMBER             4
#define APP_DIAG_CHANNEL_DIRECTION          PNET_DIAG_CH_PROP_DIR_INPUT
#define APP_DIAG_CHANNEL_NUMBER_OF_BITS     PNET_DIAG_CH_PROP_TYPE_1_BIT
#define APP_DIAG_CHANNEL_SEVERITY           PNET_DIAG_CH_PROP_MAINT_FAULT
#define APP_DIAG_CHANNEL_ERRORTYPE          CHANNEL_ERRORTYPE_SHORT_CIRCUIT
#define APP_DIAG_CHANNEL_ADDVALUE_A         0
#define APP_DIAG_CHANNEL_ADDVALUE_B         1234
#define APP_DIAG_CHANNEL_EXTENDED_ERRORTYPE 0
#define APP_DIAG_CHANNEL_QUAL_SEVERITY      0 /* Not used (Max one bit set) */

typedef struct app_data_t
{
   pnet_t * net;

   /* P-Net configuration passed in app_init(). */
   pnet_cfg_t * pnet_cfg;

   /* Application api for administration of plugged
    *(sub)modules and connection state.
    */
   app_api_t main_api;

   os_timer_t * main_timer;
   os_event_t * main_events;

   bool alarm_allowed;
   pnet_alarm_argument_t alarm_arg;
   uint8_t alarm_payload[APP_GSDML_ALARM_PAYLOAD_SIZE];

   uint32_t arep_for_appl_ready;
   uint32_t appl_ready_delay_count;
   bool appl_ready_wait;

   uint32_t process_data_tick_counter;

   app_mode_t mode;

} app_data_t;

static void app_plug_dap (app_data_t * app, uint16_t number_of_ports);
static int app_set_initial_data_and_ioxs (app_data_t * app);
static void app_cyclic_data_callback (app_subslot_t * subslot, void * tag);

app_data_t app_state;

pnet_t * app_get_pnet_instance (app_data_t * app)
{
   if (app != NULL)
   {
      return app->net;
   }
   return NULL;
}

app_data_t * app_init (pnet_cfg_t * pnet_cfg, const app_args_t * app_args)
{
   app_data_t * app = &app_state;

   app->alarm_allowed = true;
   app->main_api.arep = UINT32_MAX;
   app->pnet_cfg = pnet_cfg;

#if PNET_OPTION_DRIVER_LAN9662
   app->mode = app_args->mode;
   switch (app->mode)
   {
   case MODE_HW_OFFLOAD_CPU:
      APP_LOG_INFO ("Application RTE mode \"cpu\"\n");
      pnet_cfg->driver_enable = true;
      break;
   case MODE_HW_OFFLOAD_FULL:
      APP_LOG_INFO ("Application RTE mode \"full\"\n");
      pnet_cfg->driver_enable = true;
      break;
   case MODE_HW_OFFLOAD_NONE:
      APP_LOG_INFO ("Application RTE mode \"none\"\n");
      pnet_cfg->driver_enable = false;
      break;
   default:
      APP_LOG_ERROR ("Application mode undefined\n");
      pnet_cfg->driver_enable = false;
      break;
   }
#else
   app->mode = MODE_HW_OFFLOAD_NONE;
#endif

   app_data_init (app->mode);

   app->net = pnet_init (app->pnet_cfg);

   if (app->net == NULL)
   {
      return NULL;
   }

   return app;
}

static void main_timer_tick (os_timer_t * timer, void * arg)
{
   app_data_t * app = (app_data_t *)arg;

   os_event_set (app->main_events, APP_EVENT_TIMER);
}

int app_start (app_data_t * app, app_run_in_separate_task_t task_config)
{
   if (app == NULL)
   {
      return -1;
   }

   app->main_events = os_event_create();
   if (app->main_events == NULL)
   {
      return -1;
   }

   app->main_timer = os_timer_create (
      APP_TICK_INTERVAL_US,
      main_timer_tick,
      (void *)app,
      false);

   if (app->main_timer == NULL)
   {
      os_event_destroy (app->main_events);
      return -1;
   }

   if (task_config == RUN_IN_SEPARATE_THREAD)
   {
      os_thread_create (
         "p-net_sample_app",
         APP_MAIN_THREAD_PRIORITY,
         APP_MAIN_THREAD_STACKSIZE,
         app_loop_forever,
         (void *)app);
   }

   os_timer_start (app->main_timer);

   return 0;
}

static void app_set_outputs_default_value (void)
{
   // APP_LOG_DEBUG ("Setting outputs to default values.\n");
   app_data_set_default_outputs();
}

/*********************************** Callbacks ********************************/

static int app_connect_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result)
{
   APP_LOG_DEBUG ("PLC connect indication. AREP: %u\n", arep);
   /*
    *  Handle the request on an application level.
    *  This is a very simple application which does not need to handle anything.
    *  All the needed information is in the AR data structure.
    */

   return 0;
}

static int app_release_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result)
{
   APP_LOG_DEBUG ("PLC release (disconnect) indication. AREP: %u\n", arep);

   app_set_outputs_default_value();

   return 0;
}

static int app_dcontrol_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_control_command_t control_command,
   pnet_result_t * p_result)
{
   APP_LOG_DEBUG (
      "PLC dcontrol message. AREP: %u  Command: %s\n",
      arep,
      app_utils_dcontrol_cmd_to_string (control_command));

   return 0;
}

static int app_ccontrol_cnf (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result)
{
   APP_LOG_DEBUG (
      "PLC ccontrol message confirmation. AREP: %u  Status codes: %d "
      "%d %d %d\n",
      arep,
      p_result->pnio_status.error_code,
      p_result->pnio_status.error_decode,
      p_result->pnio_status.error_code_1,
      p_result->pnio_status.error_code_2);

   return 0;
}

static int app_write_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t api,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t idx,
   uint16_t sequence_number,
   uint16_t write_length,
   const uint8_t * p_write_data,
   pnet_result_t * p_result)
{
   APP_LOG_DEBUG (
      "PLC write record indication.\n"
      "  AREP: %u API: %u Slot: %2u Subslot: %u Index: %u Sequence: %2u "
      "Length: %u\n",
      arep,
      api,
      slot_nbr,
      subslot_nbr,
      (unsigned)idx,
      sequence_number,
      write_length);

   return 0;
}

static int app_read_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t api,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t idx,
   uint16_t sequence_number,
   uint8_t ** pp_read_data,
   uint16_t * p_read_length,
   pnet_result_t * p_result)
{
   APP_LOG_DEBUG (
      "PLC read record indication.\n"
      "  AREP: %u API: %u Slot: %2u Subslot: %u Index: %u Sequence: %2u Max "
      "length: %u\n",
      arep,
      api,
      slot_nbr,
      subslot_nbr,
      (unsigned)idx,
      sequence_number,
      (unsigned)*p_read_length);

   return 0;
}

static int app_state_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_event_values_t state)
{
   uint16_t err_cls = 0;  /* Error code 1 */
   uint16_t err_code = 0; /* Error code 2 */
   const char * error_class_description = "";
   const char * error_code_description = "";

   app_data_t * app = (app_data_t *)arg;

   APP_LOG_DEBUG (
      "Event indication %s   AREP: %u\n",
      app_utils_event_to_string (state),
      arep);

   if (state == PNET_EVENT_ABORT)
   {
      if (pnet_get_ar_error_codes (net, arep, &err_cls, &err_code) == 0)
      {
         app_utils_get_error_code_strings (
            err_cls,
            err_code,
            &error_class_description,
            &error_code_description);
         APP_LOG_DEBUG (
            "    Error class: 0x%02x %s \n"
            "    Error code:  0x%02x %s \n",
            (unsigned)err_cls,
            error_class_description,
            (unsigned)err_code,
            error_code_description);
      }
      else
      {
         APP_LOG_DEBUG ("    No error status available\n");
      }
      /* Set output values */
      app_set_outputs_default_value();

      /* Only abort AR with correct session key */
      os_event_set (app->main_events, APP_EVENT_ABORT);
   }
   else if (state == PNET_EVENT_PRMEND)
   {
      if (app->main_api.arep != UINT32_MAX)
      {
         APP_LOG_WARNING ("Warning - AREP out of sync\n");
      }
      app->main_api.arep = arep;

      app_set_initial_data_and_ioxs (app);

      (void)pnet_set_provider_state (net, true);

      /* Send application ready at next tick
         Do not call pnet_application_ready() here as it will affect
         the internal stack states */
      app->arep_for_appl_ready = arep;
      os_event_set (app->main_events, APP_EVENT_READY_FOR_DATA);
   }
   else if (state == PNET_EVENT_DATA)
   {
      APP_LOG_DEBUG ("Cyclic data transmission started\n\n");
   }

   return 0;
}

static int app_reset_ind (
   pnet_t * net,
   void * arg,
   bool should_reset_application,
   uint16_t reset_mode)
{
   APP_LOG_DEBUG (
      "PLC reset indication. Application reset mandatory: %u  Reset mode: %d\n",
      should_reset_application,
      reset_mode);

   return 0;
}

static int app_signal_led_ind (pnet_t * net, void * arg, bool led_state)
{
   APP_LOG_INFO ("Profinet signal LED indication. New state: %u\n", led_state);

   app_set_led (APP_PROFINET_SIGNAL_LED_ID, led_state);
   return 0;
}

static int app_exp_module_ind (
   pnet_t * net,
   void * arg,
   uint32_t api,
   uint16_t slot,
   uint32_t module_ident)
{
   int ret = -1;
   int result = 0;
   app_data_t * app = (app_data_t *)arg;
   const char * module_name = "unknown";
   const app_gsdml_module_t * module_config;

   APP_LOG_DEBUG ("Module plug indication API %d\n");

   if (slot >= PNET_MAX_SLOTS)
   {
      APP_LOG_ERROR (
         "Unsupported slot number: %u  It should be less than %u\n",
         slot,
         PNET_MAX_SLOTS);
      return -1;
   }

   module_config = app_gsdml_get_module_cfg (module_ident);
   if (module_config == NULL)
   {
      APP_LOG_ERROR ("  Module ID %08x not found.\n", (unsigned)module_ident);
      /*
       * Needed to pass Behavior scenario 2
       */
      APP_LOG_DEBUG ("  Plug expected module anyway\n");
   }
   else
   {
      module_name = module_config->name;
   }

   APP_LOG_DEBUG ("  [%u] Pull old module\n", slot);
   result = pnet_pull_module (net, api, slot);

   if (result == 0)
   {
      app_utils_pull_module (&app->main_api, slot);
   }

   APP_LOG_DEBUG (
      "  [%u] Plug module. Module ID: 0x%x \"%s\"\n",
      slot,
      (unsigned)module_ident,
      module_name);

   ret = pnet_plug_module (net, api, slot, module_ident);
   if (ret == 0)
   {
      app_utils_plug_module (&app->main_api, slot, module_ident, module_name);
   }
   else
   {
      APP_LOG_ERROR (
         "  [%u] Plug module failed. Ret: %u Module ID: 0x%x \" \"\n",
         slot,
         ret,
         (unsigned)module_ident,
         module_name);
   }

   return ret;
}

static int app_exp_submodule_ind (
   pnet_t * net,
   void * arg,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint32_t module_id,
   uint32_t submodule_id,
   const pnet_data_cfg_t * p_exp_data)
{
   int ret = -1;
   int result = 0;
   pnet_data_cfg_t data_cfg = {0};
   app_data_t * app = (app_data_t *)arg;
   const app_gsdml_submodule_t * submod_cfg;
   const char * name = "Unsupported";
   app_utils_cyclic_callback cyclic_data_callback = NULL;
   app_subslot_t * p_subslot;

   APP_LOG_DEBUG ("Submodule plug indication API %u\n", api);

   submod_cfg = app_gsdml_get_submodule_cfg (submodule_id);
   if (submod_cfg != NULL)
   {
      data_cfg.data_dir = submod_cfg->data_dir;
      data_cfg.insize = submod_cfg->insize;
      data_cfg.outsize = submod_cfg->outsize;
      name = submod_cfg->name;

      if (data_cfg.insize > 0 || data_cfg.outsize > 0)
      {
         cyclic_data_callback = app_cyclic_data_callback;
      }
   }
   else
   {
      APP_LOG_WARNING (
         "  [%u,%u] Submodule ID 0x%x in Module ID 0x%x not found \n",
         slot,
         subslot,
         (unsigned)submodule_id,
         (unsigned)module_id);

      /*
       * Needed for behavior scenario 2 to pass.
       * Iops will be set to bad for this subslot
       */
      APP_LOG_WARNING (
         "  [%u,%u] Plug expected submodule anyway\n",
         slot,
         subslot);

      data_cfg.data_dir = p_exp_data->data_dir;
      data_cfg.insize = p_exp_data->insize;
      data_cfg.outsize = p_exp_data->outsize;
   }

   APP_LOG_DEBUG ("  [%u,%u] Pull old submodule.\n", slot, subslot);

   result = pnet_pull_submodule (net, api, slot, subslot);
   if (result == 0)
   {
      app_utils_pull_submodule (&app->main_api, slot, subslot);
   }

   APP_LOG_DEBUG (
      "  [%u,%u] Plug submodule. Submodule ID: 0x%x Data Dir: %s In: %u Out: "
      "%u \"%s\"\n",
      slot,
      subslot,
      (unsigned)submodule_id,
      app_utils_submod_dir_to_string (data_cfg.data_dir),
      data_cfg.insize,
      data_cfg.outsize,
      name);

   if (
      data_cfg.data_dir != p_exp_data->data_dir ||
      data_cfg.insize != p_exp_data->insize ||
      data_cfg.outsize != p_exp_data->outsize)
   {
      APP_LOG_WARNING (
         "  [%u,%u] Warning expected Data Dir: %s In: %u Out: %u\n",
         slot,
         subslot,
         app_utils_submod_dir_to_string (p_exp_data->data_dir),
         p_exp_data->insize,
         p_exp_data->outsize);
   }
   ret = pnet_plug_submodule (
      net,
      api,
      slot,
      subslot,
      module_id,
      submodule_id,
      data_cfg.data_dir,
      data_cfg.insize,
      data_cfg.outsize);

   if (ret == 0)
   {
      p_subslot = app_utils_plug_submodule (
         &app->main_api,
         slot,
         subslot,
         submodule_id,
         &data_cfg,
         name,
         cyclic_data_callback,
         app);

      /**
       * Setup the RTE configuration for submodules data provided by the FPGA.
       */

      if (app->mode == MODE_HW_OFFLOAD_FULL)
      {
         const uint8_t * default_data;
         uint16_t fpga_address;

         if (
            app_data_get_fpga_info_by_id (
               submodule_id,
               &fpga_address,
               &default_data) == 0)
         {
            pnet_mera_rte_data_cfg_t rte_cfg = {
               .type = PNET_MERA_DATA_TYPE_QSPI,
               .address = fpga_address,
               .default_data = default_data};

            APP_LOG_INFO (
               "  %-40s Set RTE QSPI address 0x%x\n",
               app_utils_get_subslot_string (p_subslot),
               rte_cfg.address);

            if (data_cfg.insize > 0)
            {
               if (
                  pnet_mera_input_set_rte_config (
                     app->net,
                     APP_GSDML_API,
                     slot,
                     subslot,
                     &rte_cfg) != 0)
               {
                  APP_LOG_ERROR (
                     "  %-40s RTE configuration failed\n",
                     app_utils_get_subslot_string (p_subslot));
               }
            }
            else
            {
               if (
                  pnet_mera_output_set_rte_config (
                     app->net,
                     APP_GSDML_API,
                     slot,
                     subslot,
                     &rte_cfg) != 0)
               {
                  APP_LOG_ERROR (
                     "  %-40s RTE configuration failed\n",
                     app_utils_get_subslot_string (p_subslot));
               }
            }
         }
      }
   }
   else
   {
      APP_LOG_ERROR (
         "  [%u,%u] Plug submodule failed. Ret: %u\n",
         slot,
         subslot,
         ret);
   }

   return ret;
}

static int app_new_data_status_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t crep,
   uint8_t changes,
   uint8_t data_status)
{
   bool is_running = data_status & BIT (PNET_DATA_STATUS_BIT_PROVIDER_STATE);
   bool is_valid = data_status & BIT (PNET_DATA_STATUS_BIT_DATA_VALID);

   APP_LOG_DEBUG (
      "Data status indication. AREP: %u  Data status changes: 0x%02x  "
      "Data status: 0x%02x\n",
      arep,
      changes,
      data_status);
   APP_LOG_DEBUG (
      "   %s, %s, %s, %s, %s\n",
      is_running ? "Run" : "Stop",
      is_valid ? "Valid" : "Invalid",
      (data_status & BIT (PNET_DATA_STATUS_BIT_STATE)) ? "Primary" : "Backup",
      (data_status & BIT (PNET_DATA_STATUS_BIT_STATION_PROBLEM_INDICATOR))
         ? "Normal operation"
         : "Problem",
      (data_status & BIT (PNET_DATA_STATUS_BIT_IGNORE))
         ? "Ignore data status"
         : "Evaluate data status");

   if (is_running == false || is_valid == false)
   {
      app_set_outputs_default_value();
   }

   return 0;
}

static int app_alarm_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   const pnet_alarm_argument_t * p_alarm_arg,
   uint16_t data_len,
   uint16_t data_usi,
   const uint8_t * p_data)
{
   app_data_t * app = (app_data_t *)arg;

   APP_LOG_DEBUG (
      "Alarm indication. AREP: %u API: %d Slot: %d Subslot: %d "
      "Type: %d Seq: %d Length: %d USI: %d\n",
      arep,
      p_alarm_arg->api_id,
      p_alarm_arg->slot_nbr,
      p_alarm_arg->subslot_nbr,
      p_alarm_arg->alarm_type,
      p_alarm_arg->sequence_number,
      data_len,
      data_usi);

   app->alarm_arg = *p_alarm_arg;
   os_event_set (app->main_events, APP_EVENT_ALARM);

   return 0;
}

static int app_alarm_cnf (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   const pnet_pnio_status_t * p_pnio_status)
{
   app_data_t * app = (app_data_t *)arg;

   APP_LOG_DEBUG (
      "PLC alarm confirmation. AREP: %u  Status code %u, "
      "%u, %u, %u\n",
      arep,
      p_pnio_status->error_code,
      p_pnio_status->error_decode,
      p_pnio_status->error_code_1,
      p_pnio_status->error_code_2);

   app->alarm_allowed = true;

   return 0;
}

static int app_alarm_ack_cnf (pnet_t * net, void * arg, uint32_t arep, int res)
{
   APP_LOG_DEBUG (
      "PLC alarm ACK confirmation. AREP: %u  Result: "
      "%d\n",
      arep,
      res);

   return 0;
}

/**
 * Plug all DAP (sub)modules
 * Use existing callback functions to plug the (sub-)modules
 * @param app              InOut:   Application handle
 * @param number_of_ports  In:      Number of active ports
 */
static void app_plug_dap (app_data_t * app, uint16_t number_of_ports)
{
   const pnet_data_cfg_t cfg_dap_data = {
      .data_dir = PNET_DIR_NO_IO,
      .insize = 0,
      .outsize = 0,
   };

   APP_LOG_DEBUG ("Plug DAP module and its submodules\n");

   app_exp_module_ind (
      app->net,
      app,
      APP_GSDML_API,
      PNET_SLOT_DAP_IDENT,
      PNET_MOD_DAP_IDENT);

   app_exp_submodule_ind (
      app->net,
      app,
      APP_GSDML_API,
      PNET_SLOT_DAP_IDENT,
      PNET_SUBSLOT_DAP_IDENT,
      PNET_MOD_DAP_IDENT,
      PNET_SUBMOD_DAP_IDENT,
      &cfg_dap_data);

   app_exp_submodule_ind (
      app->net,
      app,
      APP_GSDML_API,
      PNET_SLOT_DAP_IDENT,
      PNET_SUBSLOT_DAP_INTERFACE_1_IDENT,
      PNET_MOD_DAP_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_IDENT,
      &cfg_dap_data);

   app_exp_submodule_ind (
      app->net,
      app,
      APP_GSDML_API,
      PNET_SLOT_DAP_IDENT,
      PNET_SUBSLOT_DAP_INTERFACE_1_PORT_1_IDENT,
      PNET_MOD_DAP_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_PORT_1_IDENT,
      &cfg_dap_data);

   if (number_of_ports >= 2)
   {
      app_exp_submodule_ind (
         app->net,
         app,
         APP_GSDML_API,
         PNET_SLOT_DAP_IDENT,
         PNET_SUBSLOT_DAP_INTERFACE_1_PORT_2_IDENT,
         PNET_MOD_DAP_IDENT,
         PNET_SUBMOD_DAP_INTERFACE_1_PORT_2_IDENT,
         &cfg_dap_data);
   }

   if (number_of_ports >= 3)
   {
      app_exp_submodule_ind (
         app->net,
         app,
         APP_GSDML_API,
         PNET_SLOT_DAP_IDENT,
         PNET_SUBSLOT_DAP_INTERFACE_1_PORT_3_IDENT,
         PNET_MOD_DAP_IDENT,
         PNET_SUBMOD_DAP_INTERFACE_1_PORT_3_IDENT,
         &cfg_dap_data);
   }

   if (number_of_ports >= 4)
   {
      app_exp_submodule_ind (
         app->net,
         app,
         APP_GSDML_API,
         PNET_SLOT_DAP_IDENT,
         PNET_SUBSLOT_DAP_INTERFACE_1_PORT_4_IDENT,
         PNET_MOD_DAP_IDENT,
         PNET_SUBMOD_DAP_INTERFACE_1_PORT_4_IDENT,
         &cfg_dap_data);
   }
}

/**
 * Send application ready to the PLC
 * @param net              InOut: p-net stack instance
 * @param arep             In:    Arep
 */
static void app_handle_send_application_ready (pnet_t * net, uint32_t arep)
{
   int ret = -1;

   APP_LOG_DEBUG (
      "Application will signal that it is ready for data, for "
      "AREP %u.\n",
      arep);

   ret = pnet_application_ready (net, arep);
   if (ret != 0)
   {
      APP_LOG_ERROR (
         "Error returned when application telling that it is ready for "
         "data. Have you set IOCS or IOPS for all subslots?\n");
   }

   /* When the PLC sends a confirmation to this message, the
      pnet_ccontrol_cnf() callback will be triggered.  */
}

/**
 * Send alarm ACK to the PLC
 *
 * @param net              InOut: p-net stack instance
 * @param arep             In:    Arep
 * @param p_alarm_arg      In:    Alarm argument (slot, subslot etc)
 */
static void app_handle_send_alarm_ack (
   pnet_t * net,
   uint32_t arep,
   const pnet_alarm_argument_t * p_alarm_arg)
{
   pnet_pnio_status_t pnio_status = {0, 0, 0, 0};
   int ret;

   ret = pnet_alarm_send_ack (net, arep, p_alarm_arg, &pnio_status);
   if (ret != 0)
   {
      APP_LOG_DEBUG ("Error when sending alarm ACK. Error: %d\n", ret);
   }
   else
   {
      APP_LOG_DEBUG ("Alarm ACK sent\n");
   }
}

/**
 * Handle cyclic data for all plugged submodules
 * Data is read from / written to the app_data file
 * which handles the data and update the physical
 * input /outputs.
 *
 * For subslots with mapped shared memory inputs, the
 * shared memory is read and the data is passed to the
 * stack.
 *
 * For subslots with mapped shared memory outputs, the
 * current output data is fetched from the stack and written to
 * mapped shared memory.
 *
 * @param subslot    In:   Subslot reference
 * @param tag        In:   Application handle
 */
static void app_cyclic_data_callback (app_subslot_t * subslot, void * tag)
{
   app_data_t * app = (app_data_t *)tag;
   uint8_t iops = PNET_IOXS_BAD;
   uint8_t iocs = PNET_IOXS_BAD;
   uint8_t * indata;
   uint16_t indata_size;
   bool outdata_updated;
   uint16_t outdata_length;
   uint8_t outdata_iops;
   uint8_t * outdata;

   if (app == NULL)
   {
      APP_LOG_ERROR ("Application tag not set in subslot?\n");
      return;
   }

   if (subslot->data_cfg.insize > 0)
   {
      indata =
         app_data_get_input_data (subslot->submodule_id, &indata_size, &iops);

      if (indata != NULL)
      {
         iops = PNET_IOXS_GOOD;
      }

      (void)pnet_input_set_data_and_iops (
         app->net,
         APP_GSDML_API,
         subslot->slot_nbr,
         subslot->subslot_nbr,
         indata,
         indata_size,
         iops);

      (void)pnet_input_get_iocs (
         app->net,
         APP_GSDML_API,
         subslot->slot_nbr,
         subslot->subslot_nbr,
         &iocs);

      app_utils_print_ioxs_change (
         subslot,
         "Consumer Status (IOCS)",
         subslot->indata_iocs,
         iocs);
      subslot->indata_iocs = iocs;
   }

   if (subslot->data_cfg.outsize > 0)
   {
      outdata = app_data_get_output_data_buffer (
         subslot->submodule_id,
         &outdata_length);

      pnet_output_get_data_and_iops (
         app->net,
         APP_GSDML_API,
         subslot->slot_nbr,
         subslot->subslot_nbr,
         &outdata_updated,
         outdata,
         &outdata_length,
         &outdata_iops);

      app_utils_print_ioxs_change (
         subslot,
         "Provider Status (IOPS)",
         subslot->outdata_iops,
         outdata_iops);
      subslot->outdata_iops = outdata_iops;

      if (outdata_length != subslot->data_cfg.outsize)
      {
         APP_LOG_ERROR (
            "  %-40s Wrong outputdata length %u (%u)\n",
            app_utils_get_subslot_string (subslot),
            outdata_length,
            subslot->data_cfg.outsize);
         app_set_outputs_default_value();
      }
      else if (outdata_iops == PNET_IOXS_GOOD)
      {
         /* Set output data for submodule */
         app_data_set_output_data (
            subslot->submodule_id,
            outdata,
            outdata_length);
      }
      else
      {
         app_set_outputs_default_value();
      }
   }
}

/**
 * Set initial input data, provider and consumer status for a subslot.
 *
 * @param app              In:    Application handle
 */
static int app_set_initial_data_and_ioxs (app_data_t * app)
{
   int ret;
   uint8_t iops;
   uint16_t slot;
   uint16_t subslot;
   const app_subslot_t * p_subslot;
   uint8_t * indata;
   uint16_t indata_size;

   for (slot = 0; slot < PNET_MAX_SLOTS; slot++)
   {
      for (subslot = 0; subslot < PNET_MAX_SUBSLOTS; subslot++)
      {
         p_subslot = &app->main_api.slots[slot].subslots[subslot];
         if (p_subslot->plugged)
         {
            iops = PNET_IOXS_GOOD;
            if (
               p_subslot->data_cfg.insize > 0 ||
               p_subslot->data_cfg.data_dir == PNET_DIR_NO_IO)
            {
               indata = NULL;
               indata_size = p_subslot->data_cfg.insize;

               if (p_subslot->data_cfg.insize > 0)
               {
                  indata = app_data_get_input_data (
                     p_subslot->submodule_id,
                     &indata_size,
                     &iops);
               }

               ret = pnet_input_set_data_and_iops (
                  app->net,
                  APP_GSDML_API,
                  p_subslot->slot_nbr,
                  p_subslot->subslot_nbr,
                  indata,
                  indata_size,
                  iops);

               /*
                * If a submodule is still plugged but not used in current
                * AR setting the data and iops will fail. This is not a
                * problem. Log message below will only be printed for
                * active submodules.
                */
               if (ret == 0)
               {
                  APP_LOG_DEBUG (
                     "  %-40s Set input data and IOPS. Size: %d "
                     "IOPS: %s\n",
                     app_utils_get_subslot_string (p_subslot),
                     p_subslot->data_cfg.insize,
                     app_utils_ioxs_to_string (iops));
               }
            }

            if (p_subslot->data_cfg.outsize > 0)
            {
               ret = pnet_output_set_iocs (
                  app->net,
                  APP_GSDML_API,
                  p_subslot->slot_nbr,
                  p_subslot->subslot_nbr,
                  PNET_IOXS_GOOD);

               if (ret == 0)
               {
                  APP_LOG_DEBUG (
                     "  %-40s Set output IOCS: %s\n",
                     app_utils_get_subslot_string (p_subslot),
                     app_utils_ioxs_to_string (PNET_IOXS_GOOD));
               }
            }
         }
      }
   }
   return 0;
}

/**
 * Send and receive cyclic / process data
 *
 * Generate callback for all plugged submodules
 * the sample application handles all submodules
 * in the app_cyclic_data_callback() function.
 * A complex device may register separate callbacks
 * for each submodule.
 *
 * @param app        In:    Application handle
 */
static void app_handle_cyclic_data (app_data_t * app)
{
   /* For the sample application cyclic data is updated
    * with a period defined by APP_TICKS_UPDATE_DATA
    */
   app->process_data_tick_counter++;
   if (app->process_data_tick_counter < APP_TICKS_UPDATE_DATA)
   {
      return;
   }
   app->process_data_tick_counter = 0;

   app_utils_cyclic_data_poll (&app->main_api);
}

void app_pnet_cfg_init_default (pnet_cfg_t * pnet_cfg)
{
   app_utils_pnet_cfg_init_default (pnet_cfg);

   pnet_cfg->state_cb = app_state_ind;
   pnet_cfg->connect_cb = app_connect_ind;
   pnet_cfg->release_cb = app_release_ind;
   pnet_cfg->dcontrol_cb = app_dcontrol_ind;
   pnet_cfg->ccontrol_cb = app_ccontrol_cnf;
   pnet_cfg->read_cb = app_read_ind;
   pnet_cfg->write_cb = app_write_ind;
   pnet_cfg->exp_module_cb = app_exp_module_ind;
   pnet_cfg->exp_submodule_cb = app_exp_submodule_ind;
   pnet_cfg->new_data_status_cb = app_new_data_status_ind;
   pnet_cfg->alarm_ind_cb = app_alarm_ind;
   pnet_cfg->alarm_cnf_cb = app_alarm_cnf;
   pnet_cfg->alarm_ack_cnf_cb = app_alarm_ack_cnf;
   pnet_cfg->reset_cb = app_reset_ind;
   pnet_cfg->signal_led_cb = app_signal_led_ind;

   pnet_cfg->cb_arg = (void *)&app_state;
}

void app_loop_forever (void * arg)
{
   app_data_t * app = (app_data_t *)arg;
   uint32_t mask = APP_EVENT_READY_FOR_DATA | APP_EVENT_TIMER |
                   APP_EVENT_ALARM | APP_EVENT_ABORT;
   uint32_t flags = 0;

   app->main_api.arep = UINT32_MAX;

   app_set_led (APP_DATA_LED_ID, false);
   app_plug_dap (app, app->pnet_cfg->num_physical_ports);
   APP_LOG_INFO ("Waiting for PLC connect request\n\n");

   /* Main event loop */
   for (;;)
   {
      os_event_wait (app->main_events, mask, &flags, OS_WAIT_FOREVER);
      if (flags & APP_EVENT_READY_FOR_DATA)
      {
         /* Delay the application ready event to
          * allow the RTE to start sending frames.
          */
         app->appl_ready_delay_count = 256;
         app->appl_ready_wait = true;
         os_event_clr (app->main_events, APP_EVENT_READY_FOR_DATA);
      }
      else if (flags & APP_EVENT_ALARM)
      {
         os_event_clr (app->main_events, APP_EVENT_ALARM);

         app_handle_send_alarm_ack (
            app->net,
            app->main_api.arep,
            &app->alarm_arg);
      }
      else if (flags & APP_EVENT_TIMER)
      {
         os_event_clr (app->main_events, APP_EVENT_TIMER);

         if (app->appl_ready_wait)
         {
            /* Delayed application ready event */
            if (app->appl_ready_delay_count == 0)
            {
               app->appl_ready_wait = false;
               app_handle_send_application_ready (
                  app->net,
                  app->arep_for_appl_ready);
            }
            else
            {
               app->appl_ready_delay_count--;
            }
         }

         if (app->main_api.arep != UINT32_MAX)
         {
            /* Todo this operation should be split
             * and inputs handled before pnet_handle_periodic is
             * called and outputs after.*/
            app_handle_cyclic_data (app);
         }

         /* Run p-net stack */
         pnet_handle_periodic (app->net);
      }
      else if (flags & APP_EVENT_ABORT)
      {
         os_event_clr (app->main_events, APP_EVENT_ABORT);

         app->main_api.arep = UINT32_MAX;
         app->alarm_allowed = true;
         APP_LOG_DEBUG ("Connection closed\n");
         APP_LOG_DEBUG ("Waiting for PLC connect request\n\n");
      }
   }
}
