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
#include "sampleapp_gsdml.h"

#include "app_gsdml_api.h"
#include "app_data_api.h"
#include "app_utils.h"
#include "app_log.h"
#include "osal.h"
#include <pnet_api.h>

#include <stdio.h>
#include <string.h>

CC_PACKED_BEGIN typedef struct CC_PACKED app_echo_data
{
   /* Network endianness.
      Used as a float, but we model it as a 4-byte integer to easily
      do endianness conversion */
   uint32_t echo_float_bytes;

   /* Network endianness */
   uint32_t echo_int;
} app_echo_data_t;
CC_PACKED_END
CC_STATIC_ASSERT (sizeof (app_echo_data_t) == APP_GSDML_INPUT_DATA_ECHO_SIZE);
CC_STATIC_ASSERT (sizeof (app_echo_data_t) == APP_GSDML_OUTPUT_DATA_ECHO_SIZE);

/******************************************************************************/

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
 * Uses first 8-bit digital input module, if available.
 *
 * @param app             InOut:    Application handle
 */
static void app_handle_demo_pnet_api (app_data_t * app)
{
   app_userdata_t * udata = app->user_data;
   uint16_t slot = 0;
   bool found_inputsubslot = false;
   uint16_t subslot_ix = 0;
   const app_subslot_t * p_subslot = NULL;
   pnet_pnio_status_t pnio_status = {0};
   pnet_diag_source_t diag_source = {
      .api = PNET_API_NO_APPLICATION_PROFILE,
      .slot = 0,
      .subslot = 0,
      .ch = APP_DIAG_CHANNEL_NUMBER,
      .ch_grouping = PNET_DIAG_CH_INDIVIDUAL_CHANNEL,
      .ch_direction = APP_DIAG_CHANNEL_DIRECTION};

   /* Loop though all subslots to find first digital 8-bit input subslot */
   while (!found_inputsubslot && (slot < PNET_MAX_SLOTS))
   {
      for (subslot_ix = 0;
           !found_inputsubslot && (subslot_ix < PNET_MAX_SUBSLOTS);
           subslot_ix++)
      {
         p_subslot = &app->main_api.slots[slot].subslots[subslot_ix];
         if (
            app_utils_subslot_is_input (p_subslot) &&
            (p_subslot->submodule_id == APP_GSDML_SUBMOD_ID_DIGITAL_IN ||
             p_subslot->submodule_id == APP_GSDML_SUBMOD_ID_DIGITAL_IN_OUT))
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

   switch (udata->alarm_demo_state)
   {
   case APP_DEMO_STATE_ALARM_SEND:
      if (app->alarm_allowed == true && app_utils_is_connected_to_controller (app))
      {
         udata->alarm_payload[0]++;
         APP_LOG_INFO (
            "Sending process alarm from slot %u subslot %u USI %u to "
            "PLC. Payload: 0x%x\n",
            slot,
            p_subslot->subslot_nbr,
            APP_ALARM_USI,
            udata->alarm_payload[0]);
         pnet_alarm_send_process_alarm (
            app->net,
            app->main_api.arep,
            PNET_API_NO_APPLICATION_PROFILE,
            slot,
            p_subslot->subslot_nbr,
            APP_ALARM_USI,
            APP_ALARM_PAYLOAD_SIZE,
            udata->alarm_payload);
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
         PNET_API_NO_APPLICATION_PROFILE,
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
         PNET_API_NO_APPLICATION_PROFILE,
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
         PNET_API_NO_APPLICATION_PROFILE,
         slot,
         p_subslot->subslot_nbr,
         APP_GSDML_DIAG_CUSTOM_USI);
      break;

   case APP_DEMO_STATE_LOGBOOK_ENTRY:
      if (app_utils_is_connected_to_controller (app))
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
      if (app_utils_is_connected_to_controller (app))
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

   switch (udata->alarm_demo_state)
   {
   case APP_DEMO_STATE_ALARM_SEND:
      udata->alarm_demo_state = APP_DEMO_STATE_CYCLIC_REDUNDANT;
      break;
   case APP_DEMO_STATE_CYCLIC_REDUNDANT:
      udata->alarm_demo_state = APP_DEMO_STATE_CYCLIC_NORMAL;
      break;
   case APP_DEMO_STATE_CYCLIC_NORMAL:
      udata->alarm_demo_state = APP_DEMO_STATE_DIAG_STD_ADD;
      break;
   case APP_DEMO_STATE_DIAG_STD_ADD:
      udata->alarm_demo_state = APP_DEMO_STATE_DIAG_STD_UPDATE;
      break;
   case APP_DEMO_STATE_DIAG_STD_UPDATE:
      udata->alarm_demo_state = APP_DEMO_STATE_DIAG_USI_ADD;
      break;
   case APP_DEMO_STATE_DIAG_USI_ADD:
      udata->alarm_demo_state = APP_DEMO_STATE_DIAG_USI_UPDATE;
      break;
   case APP_DEMO_STATE_DIAG_USI_UPDATE:
      udata->alarm_demo_state = APP_DEMO_STATE_DIAG_USI_REMOVE;
      break;
   case APP_DEMO_STATE_DIAG_USI_REMOVE:
      udata->alarm_demo_state = APP_DEMO_STATE_DIAG_STD_REMOVE;
      break;
   case APP_DEMO_STATE_DIAG_STD_REMOVE:
      udata->alarm_demo_state = APP_DEMO_STATE_LOGBOOK_ENTRY;
      break;
   case APP_DEMO_STATE_LOGBOOK_ENTRY:
      udata->alarm_demo_state = APP_DEMO_STATE_ABORT_AR;
      break;
   default:
   case APP_DEMO_STATE_ABORT_AR:
      udata->alarm_demo_state = APP_DEMO_STATE_ALARM_SEND;
      break;
   }
}

/**
 * Set LED state.
 *
 * Compares new state with previous state, to minimize system calls.
 *
 * Uses the hardware specific app_set_led() function.
 *
 * @param led_state        In:    New LED state
 */
static void app_handle_data_led_state (bool led_state)
{
   static bool previous_led_state = false;

   if (led_state != previous_led_state)
   {
      app_set_led (APP_DATA_LED_ID, led_state);
   }
   previous_led_state = led_state;
}

uint8_t * app_data_inputdata_getbuffer (
   void * user_data,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   uint16_t * size,
   uint8_t * iops)
{
   app_userdata_t * udata = (app_userdata_t *)user_data;
   app_echo_data_t * p_echo_inputdata =
      (app_echo_data_t *)&udata->echo_inputdata;
   app_echo_data_t * p_echo_outputdata =
      (app_echo_data_t *)&udata->echo_outputdata;
   float inputfloat;
   float outputfloat;
   uint32_t hostorder_inputfloat_bytes;
   uint32_t hostorder_outputfloat_bytes;

   if (size == NULL || iops == NULL)
   {
      return NULL;
   }

   if (
      submodule_id == APP_GSDML_SUBMOD_ID_DIGITAL_IN ||
      submodule_id == APP_GSDML_SUBMOD_ID_DIGITAL_IN_OUT)
   {
      /* Prepare digital input data
       * Lowest 7 bits: Counter    Most significant bit: Button
       */

      udata->inputdata[0] = udata->count++;
      if (udata->button1_pressed)
      {
         udata->inputdata[0] |= 0x80;
      }
      else
      {
         udata->inputdata[0] &= 0x7F;
      }

      *size = APP_GSDML_INPUT_DATA_DIGITAL_SIZE;
      *iops = PNET_IOXS_GOOD;
      return udata->inputdata;
   }

   if (submodule_id == APP_GSDML_SUBMOD_ID_ECHO)
   {
      /* Calculate echodata input (to the PLC)
       * by multiplying the output (from the PLC) with a gain factor
       */

      /* Integer */
      p_echo_inputdata->echo_int = CC_TO_BE32 (
         CC_FROM_BE32 (p_echo_outputdata->echo_int) *
         CC_FROM_BE32 (udata->app_param_echo_gain));

      /* Float */
      /* Use memcopy to avoid strict-aliasing rule warnings */
      hostorder_outputfloat_bytes =
         CC_FROM_BE32 (p_echo_outputdata->echo_float_bytes);
      memcpy (&outputfloat, &hostorder_outputfloat_bytes, sizeof (outputfloat));
      inputfloat = outputfloat * CC_FROM_BE32 (udata->app_param_echo_gain);
      memcpy (&hostorder_inputfloat_bytes, &inputfloat, sizeof (outputfloat));
      p_echo_inputdata->echo_float_bytes =
         CC_TO_BE32 (hostorder_inputfloat_bytes);

      *size = APP_GSDML_INPUT_DATA_ECHO_SIZE;
      *iops = PNET_IOXS_GOOD;
      return udata->echo_inputdata;
   }

   /* Automated RT Tester scenario 2 - unsupported (sub)module */
   return NULL;
}

int app_data_set_output_data (
   void * user_data,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   uint8_t * data,
   uint16_t size)
{
   app_userdata_t * udata = (app_userdata_t *)user_data;
   app_echo_data_t * p_echo_outputdata =
      (app_echo_data_t *)&udata->echo_outputdata;
   bool led_state;

   if (data == NULL)
   {
      return -1;
   }

   if (
      submodule_id == APP_GSDML_SUBMOD_ID_DIGITAL_OUT ||
      submodule_id == APP_GSDML_SUBMOD_ID_DIGITAL_IN_OUT)
   {
      if (size == APP_GSDML_OUTPUT_DATA_DIGITAL_SIZE)
      {
         memcpy (udata->outputdata, data, size);

         /* Most significant bit: LED */
         led_state = (udata->outputdata[0] & 0x80) > 0;
         app_handle_data_led_state (led_state);

         return 0;
      }
   }
   else if (submodule_id == APP_GSDML_SUBMOD_ID_ECHO)
   {
      if (size == APP_GSDML_OUTPUT_DATA_ECHO_SIZE)
      {
         memcpy (p_echo_outputdata, data, size);

         return 0;
      }
   }

   return -1;
}

uint8_t * app_data_outputdata_getbuffer (
   void * user_data,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   uint16_t * size)
{
   app_userdata_t * udata = (app_userdata_t *)user_data;

   if (
      submodule_id == APP_GSDML_SUBMOD_ID_DIGITAL_OUT ||
      submodule_id == APP_GSDML_SUBMOD_ID_DIGITAL_IN_OUT)
   {
      *size = APP_GSDML_OUTPUT_DATA_DIGITAL_SIZE;
      return udata->outputdata;
   }

   if (submodule_id == APP_GSDML_SUBMOD_ID_ECHO)
   {
      *size = APP_GSDML_OUTPUT_DATA_ECHO_SIZE;
      return udata->echo_outputdata;
   }

   /* Automated RT Tester scenario 2 - unsupported (sub)module */
   return NULL;
}

void app_data_outputdata_finalize (
   void * user_data,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id)
{
   app_userdata_t * udata = (app_userdata_t *)user_data;
   bool led_state;

   if (
      submodule_id == APP_GSDML_SUBMOD_ID_DIGITAL_OUT ||
      submodule_id == APP_GSDML_SUBMOD_ID_DIGITAL_IN_OUT)
   {
      /* Most significant bit: LED */
      led_state = (udata->outputdata[0] & 0x80) > 0;
      app_handle_data_led_state (led_state);
   }
}

int app_data_set_default_outputs (void * user_data)
{
   app_userdata_t * udata = (app_userdata_t *)user_data;

   udata->outputdata[0] = APP_DATA_DEFAULT_OUTPUT_DATA;
   app_handle_data_led_state (false);

   return 0;
}

int app_data_reset (
   void * user_data,
   bool should_reset_application,
   uint16_t reset_mode)
{
   return 0;
}

int app_data_signal_led (void * user_data, bool led_state)
{
   app_set_led (APP_PROFINET_SIGNAL_LED_ID, led_state);
   return 0;
}

void app_data_init (void * user_data, app_data_t * app)
{
   app_userdata_t * udata = (app_userdata_t *)user_data;

   udata->app_param_echo_gain = 1;

   app_set_led (APP_DATA_LED_ID, false);
}

void app_data_pre (void * user_data, app_data_t * app)
{
   app_userdata_t * udata = (app_userdata_t *)user_data;

   udata->buttons_tick_counter++;
   if (udata->buttons_tick_counter > APP_TICKS_READ_BUTTONS)
   {
      udata->button1_pressed = app_get_button (0);
      udata->button2_pressed = app_get_button (1);
      udata->buttons_tick_counter = 0;
   }

   /* Run alarm demo function if button2 is pressed */
   if (
      (udata->button2_pressed == true) &&
      (udata->button2_pressed_previous == false))
   {
      app_handle_demo_pnet_api (app);
   }
   udata->button2_pressed_previous = udata->button2_pressed;
}

int app_data_write_parameter (
   void * user_data,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   uint32_t index,
   const uint8_t * data,
   uint16_t length)
{
   const app_gsdml_param_t * par_cfg;
   app_userdata_t * udata = (app_userdata_t *)user_data;

   par_cfg = app_gsdml_get_parameter_cfg (submodule_id, index);
   if (par_cfg == NULL)
   {
      APP_LOG_WARNING (
         "PLC write request unsupported submodule/parameter. "
         "Submodule id: %u Index: %u\n",
         (unsigned)submodule_id,
         (unsigned)index);
      return -1;
   }

   if (length != par_cfg->length)
   {
      APP_LOG_WARNING (
         "PLC write request unsupported length. "
         "Index: %u Length: %u Expected length: %u\n",
         (unsigned)index,
         (unsigned)length,
         par_cfg->length);
      return -1;
   }

   if (index == APP_GSDML_PARAMETER_1_IDX)
   {
      memcpy (&udata->app_param_1, data, length);
   }
   else if (index == APP_GSDML_PARAMETER_2_IDX)
   {
      memcpy (&udata->app_param_2, data, length);
   }
   else if (index == APP_GSDML_PARAMETER_ECHO_IDX)
   {
      memcpy (&udata->app_param_echo_gain, data, length);
   }

   APP_LOG_DEBUG ("  Writing parameter \"%s\"\n", par_cfg->name);
   app_log_print_bytes (APP_LOG_LEVEL_DEBUG, data, length);

   return 0;
}

int app_data_read_parameter (
   void * user_data,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   uint32_t index,
   uint8_t ** data,
   uint16_t * length)
{
   const app_gsdml_param_t * par_cfg;
   app_userdata_t * udata = (app_userdata_t *)user_data;

   par_cfg = app_gsdml_get_parameter_cfg (submodule_id, index);
   if (par_cfg == NULL)
   {
      APP_LOG_WARNING (
         "PLC read request unsupported submodule/parameter. "
         "Submodule id: %u Index: %u\n",
         (unsigned)submodule_id,
         (unsigned)index);
      return -1;
   }

   if (*length < par_cfg->length)
   {
      APP_LOG_WARNING (
         "PLC read request unsupported length. "
         "Index: %u Length: %u Expected length: %u\n",
         (unsigned)index,
         (unsigned)*length,
         par_cfg->length);
      return -1;
   }

   APP_LOG_DEBUG ("  Reading \"%s\"\n", par_cfg->name);
   if (index == APP_GSDML_PARAMETER_1_IDX)
   {
      *data = (uint8_t *)&udata->app_param_1;
      *length = sizeof (udata->app_param_1);
   }
   else if (index == APP_GSDML_PARAMETER_2_IDX)
   {
      *data = (uint8_t *)&udata->app_param_2;
      *length = sizeof (udata->app_param_2);
   }
   else if (index == APP_GSDML_PARAMETER_ECHO_IDX)
   {
      *data = (uint8_t *)&udata->app_param_echo_gain;
      *length = sizeof (udata->app_param_echo_gain);
   }

   app_log_print_bytes (APP_LOG_LEVEL_DEBUG, *data, *length);

   return 0;
}
