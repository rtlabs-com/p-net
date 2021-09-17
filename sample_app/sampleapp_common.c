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

#include "sampleapp_common.h"
#include "app_utils.h"
#include "app_gsdml.h"
#include "app_data.h"
#include "app_log.h"
#include "osal.h"
#include "pnal.h"
#include <pnet_api.h>

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

typedef enum app_demo_state
{
   APP_DEMO_STATE_ALARM_SEND = 0,
   APP_DEMO_STATE_LOGBOOK_ENTRY,
   APP_DEMO_STATE_ABORT_AR,
   APP_DEMO_STATE_CYCLIC_REDUNDANT,
   APP_DEMO_STATE_CYCLIC_NORMAL,
   APP_DEMO_STATE_DIAG_STD_ADD,
   APP_DEMO_STATE_DIAG_STD_UPDATE,
   APP_DEMO_STATE_DIAG_STD_REMOVE,
   APP_DEMO_STATE_DIAG_USI_ADD,
   APP_DEMO_STATE_DIAG_USI_UPDATE,
   APP_DEMO_STATE_DIAG_USI_REMOVE,
} app_demo_state_t;

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
   app_demo_state_t alarm_demo_state;
   uint8_t alarm_payload[APP_GSDM_ALARM_PAYLOAD_SIZE];

   uint32_t arep_for_appl_ready;

   bool button1_pressed;
   bool button2_pressed;
   bool button2_pressed_previous;

   /* Counters used to control when buttons are checked
    * and process data is updated
    */
   uint32_t buttons_tick_counter;
   uint32_t process_data_tick_counter;

   /* Counter value part of process data */
   uint8_t counter_data;
} app_data_t;

static void app_plug_dap (app_data_t * app, uint16_t number_of_ports);
static int app_set_initial_data_and_ioxs (app_data_t * app);
static void app_cyclic_data_callback (app_subslot_t * subslot, void * tag);

static app_data_t app_state;

pnet_t * app_get_pnet_instance (app_data_t * app)
{
   if (app != NULL)
   {
      return app->net;
   }
   return NULL;
}

app_data_t * app_init (pnet_cfg_t * pnet_cfg)
{
   app_data_t * app = &app_state;

   app->alarm_allowed = true;
   app->main_api.arep = UINT32_MAX;
   app->pnet_cfg = pnet_cfg;

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
   APP_LOG_DEBUG ("Setting outputs to default values.\n");
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
   app_data_t * app = (app_data_t *)arg;
   app_subslot_t * subslot;
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

   subslot = app_utils_subslot_get (&app->main_api, slot_nbr, subslot_nbr);
   if (subslot != NULL)
   {
      app_data_write_parameter (
         subslot->submodule_id,
         idx,
         p_write_data,
         write_length);
   }
   else
   {
      APP_LOG_WARNING (
         "No submodule plugged in AREP: %u API: %u Slot: %2u Subslot: %u "
         "Index will not be written.\n",
         arep,
         api,
         slot_nbr,
         subslot_nbr);
   }
   /* Todo: update p_result on error.
    * Check return code of app_data_write_parameter.
    */
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
   app_data_t * app = (app_data_t *)arg;
   app_subslot_t * subslot;

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

   subslot = app_utils_subslot_get (&app->main_api, slot_nbr, subslot_nbr);
   if (subslot != NULL)
   {
      app_data_read_parameter (
         subslot->submodule_id,
         idx,
         pp_read_data,
         p_read_length);
   }
   else
   {
      APP_LOG_WARNING (
         "No submodule plugged in AREP: %u API: %u Slot: %2u Subslot: %u "
         "Index will not be read.\n",
         arep,
         api,
         slot_nbr,
         subslot_nbr);
   }

   /* Todo: update p_result on error.
    * Check return code of app_data_write_parameter.
    */
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

   APP_LOG_DEBUG ("Module plug indication\n");

   if (slot >= PNET_MAX_SLOTS)
   {
      APP_LOG_ERROR (
         "Wrong slot number received: %u  It should be less than %u\n",
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

   APP_LOG_DEBUG ("  Pull old module.    API: %u Slot: %2u\n", api, slot);
   result = pnet_pull_module (net, api, slot);

   if (result == 0)
   {
      app_utils_pull_module (&app->main_api, slot);
   }

   APP_LOG_DEBUG (
      "  Plug module.        API: %u Slot: %2u Module ID: 0x%x %s\n",
      api,
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
         "Plug module failed. Ret: %u API: %u Slot: %2u Module ID: 0x%x\n",
         ret,
         api,
         slot,
         (unsigned)module_ident);
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

   APP_LOG_DEBUG ("Submodule plug indication.\n");

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
         "  Submodule ID 0x%x in module ID 0x%x not found. API: %u Slot: %2u "
         "Subslot %u \n",
         (unsigned)submodule_id,
         (unsigned)module_id,
         api,
         slot,
         subslot);

      /*
       * Needed for behavior scenario 2 to pass.
       * Iops will be set to bad for this subslot
       */
      APP_LOG_WARNING ("  Plug expected submodule anyway \n");

      data_cfg.data_dir = p_exp_data->data_dir;
      data_cfg.insize = p_exp_data->insize;
      data_cfg.outsize = p_exp_data->outsize;
   }

   APP_LOG_DEBUG (
      "  Pull old submodule. API: %u Slot: %2u Subslot: %u\n",
      api,
      slot,
      subslot);

   result = pnet_pull_submodule (net, api, slot, subslot);
   if (result == 0)
   {
      app_utils_pull_submodule (&app->main_api, slot, subslot);
   }

   APP_LOG_DEBUG (
      "  Plug submodule.     API: %u Slot: %2u Module ID: 0x%-4x\n"
      "                      Subslot: %u Submodule ID: 0x%x \"%s\"\n",
      api,
      slot,
      (unsigned)module_id,
      subslot,
      (unsigned)submodule_id,
      name);

   APP_LOG_DEBUG (
      "                      Data Dir: %s In: %u Out: %u\n",
      app_utils_submod_dir_to_string (data_cfg.data_dir),
      data_cfg.insize,
      data_cfg.outsize);

   if (
      data_cfg.data_dir != p_exp_data->data_dir ||
      data_cfg.insize != p_exp_data->insize ||
      data_cfg.outsize != p_exp_data->outsize)
   {
      APP_LOG_WARNING (
         "    Warning expected  Data Dir: %s In: %u Out: %u\n",
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
      app_utils_plug_submodule (
         &app->main_api,
         slot,
         subslot,
         submodule_id,
         &data_cfg,
         name,
         cyclic_data_callback,
         app);
   }
   else
   {
      APP_LOG_ERROR (
         "  Plug submodule failed. Ret: %u API: %u Slot: %2u Subslot %u "
         "Module ID: 0x%x Submodule ID: 0x%x \n",
         ret,
         api,
         slot,
         subslot,
         (unsigned)module_id,
         (unsigned)submodule_id);
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

   uint8_t outdata_buf[10]; /* Todo: Remove temporary buffer */

   if (app == NULL)
   {
      APP_LOG_ERROR ("Application tag not set in subslot?\n");
      return;
   }

   if (subslot->data_cfg.insize > 0)
   {
      /* Get input data for submodule
       * For the sample application data includes
       * includes button state and a counter value
       */
      indata = app_data_get_input_data (
         subslot->submodule_id,
         app->button1_pressed,
         app->counter_data,
         &indata_size,
         &iops);

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
         subslot->iocs,
         iocs);
      subslot->iocs = iocs;
   }

   if (subslot->data_cfg.outsize > 0)
   {
      outdata_length = subslot->data_cfg.outsize;
      CC_ASSERT (outdata_length < sizeof (outdata_buf));

      pnet_output_get_data_and_iops (
         app->net,
         APP_GSDML_API,
         subslot->slot_nbr,
         subslot->subslot_nbr,
         &outdata_updated,
         outdata_buf,
         &outdata_length,
         &outdata_iops);

      app_utils_print_ioxs_change (
         subslot,
         "Provider Status (IOPS)",
         subslot->iops,
         outdata_iops);
      subslot->iops = outdata_iops;

      if (outdata_length != subslot->data_cfg.outsize)
      {
         APP_LOG_ERROR ("Wrong outputdata length: %u\n", outdata_length);
         app_set_outputs_default_value();
      }
      else if (outdata_iops == PNET_IOXS_GOOD)
      {
         /* Set output data for submodule */
         app_data_set_output_data (
            subslot->submodule_id,
            outdata_buf,
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
               indata_size = 0;

               /* Get input data for submodule
                * For the sample application data includes
                * includes button state and a counter value
                */
               if (p_subslot->data_cfg.insize > 0)
               {
                  indata = app_data_get_input_data (
                     p_subslot->submodule_id,
                     app->button1_pressed,
                     app->counter_data,
                     &indata_size,
                     &iops);
               }

               ret = pnet_input_set_data_and_iops (
                  app->net,
                  app->main_api.api_id,
                  p_subslot->slot_nbr,
                  p_subslot->subslot_nbr,
                  indata,
                  indata_size,
                  iops);

               /*
                * If a submodule is still plugged but not used in current AR
                * setting the data and iops will fail.
                * This is not a problem.
                * Log message below will only be printed for active submodules.
                */
               if (ret == 0)
               {
                  APP_LOG_DEBUG (
                     "  Set input data and IOPS for slot %2u subslot %u "
                     "\"%s\"\n"
                     "    %s size %d \n",
                     p_subslot->slot_nbr,
                     p_subslot->subslot_nbr,
                     p_subslot->submodule_name,
                     app_utils_ioxs_to_string (iops),
                     p_subslot->data_cfg.insize);
               }
            }

            if (p_subslot->data_cfg.outsize > 0)
            {
               ret = pnet_output_set_iocs (
                  app->net,
                  app->main_api.api_id,
                  p_subslot->slot_nbr,
                  p_subslot->subslot_nbr,
                  PNET_IOXS_GOOD);

               if (ret == 0)
               {
                  APP_LOG_DEBUG (
                     "  Set output IOCS for slot %2u subslot %u \"%s\"\n"
                     "    %s\n",
                     p_subslot->slot_nbr,
                     p_subslot->subslot_nbr,
                     p_subslot->submodule_name,
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
   app->counter_data++;
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

/**
 * Set alarm, diagnostics and logbook entries.
 *
 * Alternates between these functions each time the button2 is pressed:
 *  - pnet_alarm_send_process_alarm()
 *  - pnet_diag_std_add()
 *  - pnet_set_redundancy_state()
 *  - pnet_set_state()
 *  - pnet_diag_std_update()
 *  - pnet_diag_usi_add()
 *  - pnet_diag_usi_update()
 *  - pnet_diag_usi_remove()
 *  - pnet_diag_std_remove()
 *  - pnet_create_log_book_entry()
 *  - pnet_ar_abort()
 *
 * Uses first input module, if available.
 *
 * @param app             InOut:    Application handle
 */
static void app_handle_demo_pnet_api (app_data_t * app)
{
   uint16_t slot = 0;
   bool found_inputsubslot = false;
   uint16_t subslot_ix = 0;
   const app_subslot_t * p_subslot = NULL;
   pnet_pnio_status_t pnio_status = {0};
   pnet_diag_source_t diag_source = {
      .api = APP_GSDML_API,
      .slot = 0,
      .subslot = 0,
      .ch = APP_DIAG_CHANNEL_NUMBER,
      .ch_grouping = PNET_DIAG_CH_INDIVIDUAL_CHANNEL,
      .ch_direction = APP_DIAG_CHANNEL_DIRECTION};

   /* Loop though slots and subslots to find first input subslot */
   while (!found_inputsubslot && (slot < PNET_MAX_SLOTS))
   {
      for (subslot_ix = 0;
           !found_inputsubslot && (subslot_ix < PNET_MAX_SUBSLOTS);
           subslot_ix++)
      {
         p_subslot = &app->main_api.slots[slot].subslots[subslot_ix];
         if (app_utils_subslot_is_input (p_subslot))
         {
            found_inputsubslot = true;
            break;
         }
      }
      if (!found_inputsubslot)
      {
         slot++;
      }
   }
   if (!found_inputsubslot)
   {
      APP_LOG_DEBUG ("Did not find any input module in the slots. Skipping.\n");
      return;
   }

   diag_source.slot = slot;
   diag_source.subslot = p_subslot->subslot_nbr;

   switch (app->alarm_demo_state)
   {
   case APP_DEMO_STATE_ALARM_SEND:
      if (app->alarm_allowed == true && app->main_api.arep != UINT32_MAX)
      {
         app->alarm_payload[0]++;
         APP_LOG_INFO (
            "Sending process alarm from slot %u subslot %u USI %u to "
            "PLC. Payload: 0x%x\n",
            slot,
            p_subslot->subslot_nbr,
            APP_ALARM_USI,
            app->alarm_payload[0]);
         pnet_alarm_send_process_alarm (
            app->net,
            app->main_api.arep,
            APP_GSDML_API,
            slot,
            p_subslot->subslot_nbr,
            APP_ALARM_USI,
            APP_GSDM_ALARM_PAYLOAD_SIZE,
            app->alarm_payload);
         app->alarm_allowed = false; /* Not allowed until ACK received */

         /* todo handle return code on pnet_alarm_send_process_alarm */
      }
      else
      {
         APP_LOG_WARNING (
            "Could not send process alarm, as alarm_allowed == false or "
            "no connection available\n");
      }
      break;

   case APP_DEMO_STATE_CYCLIC_REDUNDANT:
      APP_LOG_INFO (
         "Setting cyclic data to backup and to redundant. See Wireshark.\n");
      if (pnet_set_primary_state (app->net, false) != 0)
      {
         APP_LOG_WARNING ("   Could not set cyclic data state to backup.\n");
      }
      if (pnet_set_redundancy_state (app->net, true) != 0)
      {
         APP_LOG_WARNING ("   Could not set cyclic data state to reundant.\n");
      }
      break;

   case APP_DEMO_STATE_CYCLIC_NORMAL:
      APP_LOG_INFO (
         "Setting cyclic data back to primary and non-redundant. See "
         "Wireshark.\n");
      if (pnet_set_primary_state (app->net, true) != 0)
      {
         APP_LOG_ERROR ("   Could not set cyclic data state to primary.\n");
      }
      if (pnet_set_redundancy_state (app->net, false) != 0)
      {
         APP_LOG_ERROR (
            "   Could not set cyclic data state to non-reundant.\n");
      }
      break;

   case APP_DEMO_STATE_DIAG_STD_ADD:
      APP_LOG_INFO (
         "Adding standard diagnosis. Slot %u subslot %u channel %u Errortype "
         "%u\n",
         diag_source.slot,
         diag_source.subslot,
         diag_source.ch,
         APP_DIAG_CHANNEL_ERRORTYPE);
      (void)pnet_diag_std_add (
         app->net,
         &diag_source,
         APP_DIAG_CHANNEL_NUMBER_OF_BITS,
         APP_DIAG_CHANNEL_SEVERITY,
         APP_DIAG_CHANNEL_ERRORTYPE,
         APP_DIAG_CHANNEL_EXTENDED_ERRORTYPE,
         APP_DIAG_CHANNEL_ADDVALUE_A,
         APP_DIAG_CHANNEL_QUAL_SEVERITY);
      break;

   case APP_DEMO_STATE_DIAG_STD_UPDATE:
      APP_LOG_INFO (
         "Updating standard diagnosis. Slot %u subslot %u channel %u\n",
         diag_source.slot,
         diag_source.subslot,
         diag_source.ch);
      pnet_diag_std_update (
         app->net,
         &diag_source,
         APP_DIAG_CHANNEL_ERRORTYPE,
         APP_DIAG_CHANNEL_EXTENDED_ERRORTYPE,
         APP_DIAG_CHANNEL_ADDVALUE_B);
      break;

   case APP_DEMO_STATE_DIAG_STD_REMOVE:
      APP_LOG_INFO (
         "Removing standard diagnosis. Slot %u subslot %u channel %u\n",
         diag_source.slot,
         diag_source.subslot,
         diag_source.ch);
      pnet_diag_std_remove (
         app->net,
         &diag_source,
         APP_DIAG_CHANNEL_ERRORTYPE,
         APP_DIAG_CHANNEL_EXTENDED_ERRORTYPE);
      break;

   case APP_DEMO_STATE_DIAG_USI_ADD:
      APP_LOG_INFO (
         "Adding USI diagnosis. Slot %u subslot %u\n",
         slot,
         p_subslot->subslot_nbr);
      pnet_diag_usi_add (
         app->net,
         APP_GSDML_API,
         slot,
         p_subslot->subslot_nbr,
         APP_GSDML_DIAG_CUSTOM_USI,
         11,
         (uint8_t *)"diagdata_1");
      break;

   case APP_DEMO_STATE_DIAG_USI_UPDATE:
      APP_LOG_INFO (
         "Updating USI diagnosis. Slot %u subslot %u\n",
         slot,
         p_subslot->subslot_nbr);
      pnet_diag_usi_update (
         app->net,
         APP_GSDML_API,
         slot,
         p_subslot->subslot_nbr,
         APP_GSDML_DIAG_CUSTOM_USI,
         13,
         (uint8_t *)"diagdata_123");
      break;

   case APP_DEMO_STATE_DIAG_USI_REMOVE:
      APP_LOG_INFO (
         "Removing USI diagnosis. Slot %u subslot %u\n",
         slot,
         p_subslot->subslot_nbr);
      pnet_diag_usi_remove (
         app->net,
         APP_GSDML_API,
         slot,
         p_subslot->subslot_nbr,
         APP_GSDML_DIAG_CUSTOM_USI);
      break;

   case APP_DEMO_STATE_LOGBOOK_ENTRY:
      if (app->main_api.arep != UINT32_MAX)
      {
         APP_LOG_INFO (
            "Writing to logbook. Error_code1: %02X Error_code2: %02X  Entry "
            "detail: 0x%08X\n",
            APP_GSDML_LOGBOOK_ERROR_CODE_1,
            APP_GSDML_LOGBOOK_ERROR_CODE_2,
            APP_GSDML_LOGBOOK_ENTRY_DETAIL);
         pnio_status.error_code = APP_GSDML_LOGBOOK_ERROR_CODE;
         pnio_status.error_decode = APP_GSDML_LOGBOOK_ERROR_DECODE;
         pnio_status.error_code_1 = APP_GSDML_LOGBOOK_ERROR_CODE_1;
         pnio_status.error_code_2 = APP_GSDML_LOGBOOK_ERROR_CODE_2;
         pnet_create_log_book_entry (
            app->net,
            app->main_api.arep,
            &pnio_status,
            APP_GSDML_LOGBOOK_ENTRY_DETAIL);
      }
      else
      {
         APP_LOG_WARNING (
            "Could not add logbook entry as no connection is available\n");
      }
      break;

   case APP_DEMO_STATE_ABORT_AR:
      if (app->main_api.arep != UINT32_MAX)
      {
         APP_LOG_INFO (
            "Sample app will disconnect and reconnect. Executing "
            "pnet_ar_abort()  AREP: %u\n",
            app->main_api.arep);
         (void)pnet_ar_abort (app->net, app->main_api.arep);
      }
      else
      {
         APP_LOG_WARNING (
            "Could not execute pnet_ar_abort(), as no connection is "
            "available\n");
      }
      break;
   }

   switch (app->alarm_demo_state)
   {
   case APP_DEMO_STATE_ALARM_SEND:
      app->alarm_demo_state = APP_DEMO_STATE_CYCLIC_REDUNDANT;
      break;
   case APP_DEMO_STATE_CYCLIC_REDUNDANT:
      app->alarm_demo_state = APP_DEMO_STATE_CYCLIC_NORMAL;
      break;
   case APP_DEMO_STATE_CYCLIC_NORMAL:
      app->alarm_demo_state = APP_DEMO_STATE_DIAG_STD_ADD;
      break;
   case APP_DEMO_STATE_DIAG_STD_ADD:
      app->alarm_demo_state = APP_DEMO_STATE_DIAG_STD_UPDATE;
      break;
   case APP_DEMO_STATE_DIAG_STD_UPDATE:
      app->alarm_demo_state = APP_DEMO_STATE_DIAG_USI_ADD;
      break;
   case APP_DEMO_STATE_DIAG_USI_ADD:
      app->alarm_demo_state = APP_DEMO_STATE_DIAG_USI_UPDATE;
      break;
   case APP_DEMO_STATE_DIAG_USI_UPDATE:
      app->alarm_demo_state = APP_DEMO_STATE_DIAG_USI_REMOVE;
      break;
   case APP_DEMO_STATE_DIAG_USI_REMOVE:
      app->alarm_demo_state = APP_DEMO_STATE_DIAG_STD_REMOVE;
      break;
   case APP_DEMO_STATE_DIAG_STD_REMOVE:
      app->alarm_demo_state = APP_DEMO_STATE_LOGBOOK_ENTRY;
      break;
   case APP_DEMO_STATE_LOGBOOK_ENTRY:
      app->alarm_demo_state = APP_DEMO_STATE_ABORT_AR;
      break;
   default:
   case APP_DEMO_STATE_ABORT_AR:
      app->alarm_demo_state = APP_DEMO_STATE_ALARM_SEND;
      break;
   }
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

static void update_button_states (app_data_t * app)
{
   app->buttons_tick_counter++;
   if (app->buttons_tick_counter > APP_TICKS_READ_BUTTONS)
   {
      app->button1_pressed = app_get_button (0);
      app->button2_pressed = app_get_button (1);
      app->buttons_tick_counter = 0;
   }
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
         os_event_clr (app->main_events, APP_EVENT_READY_FOR_DATA);

         app_handle_send_application_ready (app->net, app->arep_for_appl_ready);
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

         update_button_states (app);
         if (app->main_api.arep != UINT32_MAX)
         {
            app_handle_cyclic_data (app);
         }

         /* Run alarm demo function if button2 is pressed */
         if (
            (app->button2_pressed == true) &&
            (app->button2_pressed_previous == false))
         {
            app_handle_demo_pnet_api (app);
         }
         app->button2_pressed_previous = app->button2_pressed;

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
