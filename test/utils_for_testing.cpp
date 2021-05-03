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

#include "utils_for_testing.h"

#include <gtest/gtest.h>

#include <inttypes.h>

/******************** Callbacks defined by p-net *****************************/

int my_connect_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result)
{
   app_data_for_testing_t * p_appdata = (app_data_for_testing_t *)arg;

   p_appdata->call_counters.connect_calls++;
   return 0;
}

int my_release_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result)
{
   app_data_for_testing_t * p_appdata = (app_data_for_testing_t *)arg;

   p_appdata->call_counters.release_calls++;
   return 0;
}

int my_dcontrol_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_control_command_t control_command,
   pnet_result_t * p_result)
{
   app_data_for_testing_t * p_appdata = (app_data_for_testing_t *)arg;

   p_appdata->call_counters.dcontrol_calls++;
   return 0;
}

int my_ccontrol_cnf (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result)
{
   app_data_for_testing_t * p_appdata = (app_data_for_testing_t *)arg;

   p_appdata->call_counters.ccontrol_calls++;
   return 0;
}

static int my_signal_led_ind (pnet_t * net, void * arg, bool led_state)
{
   app_data_for_testing_t * p_appdata = (app_data_for_testing_t *)arg;

   TEST_TRACE ("Callback on set LED state: %u\n", led_state);
   if (led_state == 1)
   {
      p_appdata->call_counters.led_on_calls++;
   }
   else
   {
      p_appdata->call_counters.led_off_calls++;
   }

   return 0;
}

int my_read_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint8_t ** pp_read_data,  /* Out: A pointer to the data */
   uint16_t * p_read_length, /* Out: Size of data */
   pnet_result_t * p_result) /* Error status if returning != 0 */
{
   app_data_for_testing_t * p_appdata = (app_data_for_testing_t *)arg;

   TEST_TRACE ("Callback on read\n");
   TEST_TRACE (
      "  API: %" PRIu32 " Slot: %" PRIu16 " Subslot: %" PRIu16
      " Index: %" PRIu16 " Sequence: %" PRIu16 "\n",
      api,
      slot,
      subslot,
      idx,
      sequence_number);
   p_appdata->call_counters.read_calls++;
   return 0;
}

int my_write_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint16_t write_length,
   const uint8_t * p_write_data,
   pnet_result_t * p_result)
{
   app_data_for_testing_t * p_appdata = (app_data_for_testing_t *)arg;

   TEST_TRACE ("Callback on write\n");
   TEST_TRACE (
      "  API: %" PRIu32 " Slot: %" PRIu16 " Subslot: %" PRIu16
      " Index: %" PRIu16 " Sequence: %" PRIu16 " Len: %" PRIu16 "\n",
      api,
      slot,
      subslot,
      idx,
      sequence_number,
      write_length);
   p_appdata->call_counters.write_calls++;
   return 0;
}

int my_new_data_status_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t crep,
   uint8_t changes,
   uint8_t data_status)
{
   TEST_TRACE ("Callback on new data\n");
   return 0;
}

int my_alarm_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   const pnet_alarm_argument_t * p_alarm_arg,
   uint16_t data_len,
   uint16_t data_usi,
   const uint8_t * p_data)
{
   TEST_TRACE ("Callback on alarm\n");
   return 0;
}

int my_alarm_cnf (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   const pnet_pnio_status_t * p_pnio_status)
{
   TEST_TRACE ("Callback on alarm confirmation\n");
   return 0;
}

int my_state_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_event_values_t state)
{
   app_data_for_testing_t * p_appdata = (app_data_for_testing_t *)arg;
   int ret;
   uint16_t slot = 0;
   uint16_t err_cls = 0;
   uint16_t err_code = 0;

   p_appdata->call_counters.state_calls++;
   p_appdata->main_arep = arep;
   p_appdata->cmdev_state = state;

   if (state == PNET_EVENT_PRMEND)
   {
      /* Set IOPS for DAP slot (has same numbering as the module identifiers) */
      ret = pnet_input_set_data_and_iops (
         net,
         TEST_API_IDENT,
         PNET_SLOT_DAP_IDENT,
         PNET_SUBSLOT_DAP_IDENT,
         NULL,
         0,
         PNET_IOXS_GOOD);
      EXPECT_EQ (ret, 0);
      ret = pnet_input_set_data_and_iops (
         net,
         TEST_API_IDENT,
         PNET_SLOT_DAP_IDENT,
         PNET_SUBSLOT_DAP_INTERFACE_1_IDENT,
         NULL,
         0,
         PNET_IOXS_GOOD);
      EXPECT_EQ (ret, 0);
      ret = pnet_input_set_data_and_iops (
         net,
         TEST_API_IDENT,
         PNET_SLOT_DAP_IDENT,
         PNET_SUBSLOT_DAP_INTERFACE_1_PORT_1_IDENT,
         NULL,
         0,
         PNET_IOXS_GOOD);
      EXPECT_EQ (ret, 0);

      /* Set initial data and IOPS for custom input modules, and IOCS for custom
       * output modules */
      for (slot = 0; slot < PNET_MAX_SLOTS; slot++)
      {
         if (p_appdata->custom_input_slots[slot] == true)
         {
            ret = pnet_input_set_data_and_iops (
               net,
               TEST_API_IDENT,
               slot,
               TEST_SUBMOD_CUSTOM_IDENT,
               p_appdata->inputdata,
               TEST_DATASIZE_INPUT,
               PNET_IOXS_GOOD);
            EXPECT_EQ (ret, 0);
         }
         if (p_appdata->custom_output_slots[slot] == true)
         {
            ret = pnet_output_set_iocs (
               net,
               TEST_API_IDENT,
               slot,
               TEST_SUBMOD_CUSTOM_IDENT,
               PNET_IOXS_GOOD);
            EXPECT_EQ (ret, 0);
         }
      }

      ret = pnet_set_provider_state (net, true);
      EXPECT_EQ (ret, 0);
   }
   else if (state == PNET_EVENT_ABORT)
   {
      ret = pnet_get_ar_error_codes (net, arep, &err_cls, &err_code);
      EXPECT_EQ (ret, 0);
      TEST_TRACE (
         "ABORT err_cls 0x%02" PRIx16 "  err_code 0x%02" PRIx16 "\n",
         err_cls,
         err_code);
   }

   return 0;
}

int my_exp_module_ind (
   pnet_t * net,
   void * arg,
   uint32_t api,
   uint16_t slot,
   uint32_t module_ident)
{
   app_data_for_testing_t * p_appdata = (app_data_for_testing_t *)arg;
   int ret = -1; /* Not supported in specified slot */
   bool found = false;
   uint16_t ix;

   TEST_TRACE ("Callback on module\n");

   /* Find it in the list of supported modules */
   ix = 0;
   while (ix < TEST_MAX_NUMBER_AVAILABLE_MODULE_TYPES)
   {
      if (p_appdata->available_module_types[ix] == module_ident)
      {
         found = true;
         break;
      }
      ix++;
   }

   if (found == true)
   {
      /* For now support any module in any slot */
      TEST_TRACE (
         "  Plug module.    API: %" PRIu32 " Slot: %" PRIu16 " Module ID: "
         "%" PRIu32 " Index in list of supported modules: %" PRIu16 "\n",
         api,
         slot,
         module_ident,
         ix);
      ret = pnet_plug_module (net, api, slot, module_ident);
      EXPECT_EQ (ret, 0);

      /* Remember what is plugged in each slot */
      if (module_ident == TEST_MOD_8_0_IDENT || module_ident == TEST_MOD_8_8_IDENT)
      {
         p_appdata->custom_input_slots[slot] = true;
      }
      if (module_ident == TEST_MOD_8_8_IDENT || module_ident == TEST_MOD_0_8_IDENT)
      {
         p_appdata->custom_output_slots[slot] = true;
      }
   }
   else
   {
      TEST_TRACE ("  Module ident %08" PRIx32 " not found\n", module_ident);
      EXPECT_TRUE (false); // Fail the test
   }

   return ret;
}

int my_exp_submodule_ind (
   pnet_t * net,
   void * arg,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint32_t module_ident,
   uint32_t submodule_ident,
   const pnet_data_cfg_t * p_exp_data)
{
   app_data_for_testing_t * p_appdata = (app_data_for_testing_t *)arg;
   int ret = -1;
   bool found = false;
   uint16_t ix = 0;

   TEST_TRACE ("Callback on submodule\n");

   /* Find it in the list of supported submodules */
   ix = 0;
   while (ix < TEST_MAX_NUMBER_AVAILABLE_SUBMODULE_TYPES)
   {
      if (
         p_appdata->available_submodule_types[ix].module_ident_number ==
            module_ident &&
         p_appdata->available_submodule_types[ix].submodule_ident_number ==
            submodule_ident)
      {
         found = true;
         break;
      }
      ix++;
   }

   if (found == true)
   {
      TEST_TRACE (
         "  Plug submodule. API: %" PRIu32 " Slot: %" PRIu16 " Subslot: "
         "%" PRIu16 " Module ID: %" PRIu32 " Submodule ID: %" PRIu32
         " (Index in "
         "available "
         "submodules: %" PRIu16 ") Direction: %u Len in: %" PRIu16
         " out: %" PRIu16 "\n",
         api,
         slot,
         subslot,
         module_ident,
         submodule_ident,
         ix,
         p_appdata->available_submodule_types[ix].direction,
         p_appdata->available_submodule_types[ix].input_length,
         p_appdata->available_submodule_types[ix].output_length);
      ret = pnet_plug_submodule (
         net,
         api,
         slot,
         subslot,
         module_ident,
         submodule_ident,
         p_appdata->available_submodule_types[ix].direction,
         p_appdata->available_submodule_types[ix].input_length,
         p_appdata->available_submodule_types[ix].output_length);
      EXPECT_EQ (ret, 0);
   }
   else
   {
      TEST_TRACE (
         "  Sub-module ident %08" PRIx32 " not found\n",
         submodule_ident);
      EXPECT_TRUE (false); // Fail the test
   }

   return ret;
}

/*********************** Base classes for tests *****************************/

void PnetIntegrationTestBase::appdata_init()
{
   memset (&appdata, 0, sizeof (appdata));
}

void PnetIntegrationTestBase::callcounter_reset()
{
   memset (&appdata.call_counters, 0, sizeof (call_counters_t));
}

void PnetIntegrationTestBase::available_modules_and_submodules_init()
{
   appdata.available_module_types[0] = PNET_MOD_DAP_IDENT;
   appdata.available_module_types[1] = TEST_MOD_8_8_IDENT;
   appdata.available_module_types[2] = TEST_MOD_8_0_IDENT;

   appdata.available_submodule_types[0].module_ident_number =
      PNET_MOD_DAP_IDENT;
   appdata.available_submodule_types[0].submodule_ident_number =
      PNET_SUBMOD_DAP_IDENT;
   appdata.available_submodule_types[0].direction = PNET_DIR_NO_IO;
   appdata.available_submodule_types[0].input_length = 0;
   appdata.available_submodule_types[0].output_length = 0;

   appdata.available_submodule_types[1].module_ident_number =
      PNET_MOD_DAP_IDENT;
   appdata.available_submodule_types[1].submodule_ident_number =
      PNET_SUBMOD_DAP_INTERFACE_1_IDENT;
   appdata.available_submodule_types[1].direction = PNET_DIR_NO_IO;
   appdata.available_submodule_types[1].input_length = 0;
   appdata.available_submodule_types[1].output_length = 0;

   appdata.available_submodule_types[2].module_ident_number =
      PNET_MOD_DAP_IDENT;
   appdata.available_submodule_types[2].submodule_ident_number =
      PNET_SUBMOD_DAP_INTERFACE_1_PORT_1_IDENT;
   appdata.available_submodule_types[2].direction = PNET_DIR_NO_IO;
   appdata.available_submodule_types[2].input_length = 0;
   appdata.available_submodule_types[2].output_length = 0;

   appdata.available_submodule_types[3].module_ident_number =
      TEST_MOD_8_8_IDENT;
   appdata.available_submodule_types[3].submodule_ident_number =
      TEST_SUBMOD_CUSTOM_IDENT;
   appdata.available_submodule_types[3].direction = PNET_DIR_IO;
   appdata.available_submodule_types[3].input_length = TEST_DATASIZE_INPUT;
   appdata.available_submodule_types[3].output_length = TEST_DATASIZE_OUTPUT;

   appdata.available_submodule_types[4].module_ident_number =
      TEST_MOD_8_0_IDENT;
   appdata.available_submodule_types[4].submodule_ident_number =
      TEST_SUBMOD_CUSTOM_IDENT;
   appdata.available_submodule_types[4].direction = PNET_DIR_OUTPUT;
   appdata.available_submodule_types[4].input_length = 0;
   appdata.available_submodule_types[4].output_length = TEST_DATASIZE_OUTPUT;
}

void PnetIntegrationTestBase::cfg_init()
{
   pnet_default_cfg.tick_us = TEST_TICK_INTERVAL_US;
   pnet_default_cfg.state_cb = my_state_ind;
   pnet_default_cfg.connect_cb = my_connect_ind;
   pnet_default_cfg.release_cb = my_release_ind;
   pnet_default_cfg.dcontrol_cb = my_dcontrol_ind;
   pnet_default_cfg.ccontrol_cb = my_ccontrol_cnf;
   pnet_default_cfg.read_cb = my_read_ind;
   pnet_default_cfg.write_cb = my_write_ind;
   pnet_default_cfg.exp_module_cb = my_exp_module_ind;
   pnet_default_cfg.exp_submodule_cb = my_exp_submodule_ind;
   pnet_default_cfg.new_data_status_cb = my_new_data_status_ind;
   pnet_default_cfg.alarm_ind_cb = my_alarm_ind;
   pnet_default_cfg.alarm_cnf_cb = my_alarm_cnf;
   pnet_default_cfg.signal_led_cb = my_signal_led_ind;
   pnet_default_cfg.reset_cb = NULL;
   pnet_default_cfg.cb_arg = &appdata;

   /* Device configuration */
   pnet_default_cfg.device_id.vendor_id_hi = 0xfe;
   pnet_default_cfg.device_id.vendor_id_lo = 0xed;
   pnet_default_cfg.device_id.device_id_hi = 0xbe;
   pnet_default_cfg.device_id.device_id_lo = 0xef;
   pnet_default_cfg.oem_device_id.vendor_id_hi = 0xfe;
   pnet_default_cfg.oem_device_id.vendor_id_lo = 0xed;
   pnet_default_cfg.oem_device_id.device_id_hi = 0xbe;
   pnet_default_cfg.oem_device_id.device_id_lo = 0xef;

   strcpy (pnet_default_cfg.station_name, "");
   strcpy (pnet_default_cfg.product_name, "PNET unit tests");

   pnet_default_cfg.if_cfg.physical_ports[0].netif_name = TEST_INTERFACE_NAME;

   /* Timing */
   pnet_default_cfg.min_device_interval = 32; /* Corresponds to 1 ms */

   /* Network configuration */
   pnet_default_cfg.send_hello = 1; /* Send HELLO */
   pnet_default_cfg.if_cfg.ip_cfg.dhcp_enable = 0;
   pnet_default_cfg.if_cfg.main_netif_name = TEST_INTERFACE_NAME;
   pnet_default_cfg.if_cfg.ip_cfg.ip_addr.a = 192;
   pnet_default_cfg.if_cfg.ip_cfg.ip_addr.b = 168;
   pnet_default_cfg.if_cfg.ip_cfg.ip_addr.c = 1;
   pnet_default_cfg.if_cfg.ip_cfg.ip_addr.d = 171;
   pnet_default_cfg.if_cfg.ip_cfg.ip_mask.a = 255;
   pnet_default_cfg.if_cfg.ip_cfg.ip_mask.b = 255;
   pnet_default_cfg.if_cfg.ip_cfg.ip_mask.c = 255;
   pnet_default_cfg.if_cfg.ip_cfg.ip_mask.d = 255;
   pnet_default_cfg.if_cfg.ip_cfg.ip_gateway.a = 192;
   pnet_default_cfg.if_cfg.ip_cfg.ip_gateway.b = 168;
   pnet_default_cfg.if_cfg.ip_cfg.ip_gateway.c = 1;
   pnet_default_cfg.if_cfg.ip_cfg.ip_gateway.d = 1;

   pnet_default_cfg.im_0_data.im_vendor_id_hi = 0x00;
   pnet_default_cfg.im_0_data.im_vendor_id_lo = 0x01;
   strcpy (pnet_default_cfg.im_0_data.im_order_id, "<orderid>           ");
   strcpy (pnet_default_cfg.im_0_data.im_serial_number, "<serial nbr>    ");
   pnet_default_cfg.im_0_data.im_hardware_revision = 1;
   pnet_default_cfg.im_0_data.im_sw_revision_prefix = 'P'; /* 'V', 'R', 'P',
                                                              'U', or 'T' */
   pnet_default_cfg.im_0_data.im_sw_revision_functional_enhancement = 0;
   pnet_default_cfg.im_0_data.im_sw_revision_bug_fix = 0;
   pnet_default_cfg.im_0_data.im_sw_revision_internal_change = 0;
   pnet_default_cfg.im_0_data.im_revision_counter = 0;
   pnet_default_cfg.im_0_data.im_profile_id = 0x1234;
   pnet_default_cfg.im_0_data.im_profile_specific_type = 0x5678;
   pnet_default_cfg.im_0_data.im_version_major = 0;
   pnet_default_cfg.im_0_data.im_version_minor = 1;
   pnet_default_cfg.im_0_data.im_supported =
      PNET_SUPPORTED_IM1 | PNET_SUPPORTED_IM2 | PNET_SUPPORTED_IM3 |
      PNET_SUPPORTED_IM4;
   strcpy (pnet_default_cfg.im_1_data.im_tag_function, "");
   strcpy (pnet_default_cfg.im_1_data.im_tag_location, "");
   strcpy (pnet_default_cfg.im_2_data.im_date, "");
   strcpy (pnet_default_cfg.im_3_data.im_descriptor, "");
   strcpy (pnet_default_cfg.im_4_data.im_signature, "");

   /* Storage */
   strcpy (pnet_default_cfg.file_directory, "/disk1");

   /* Diagnosis */
   pnet_default_cfg.use_qualified_diagnosis = true;

   pnet_default_cfg.num_physical_ports = PNET_MAX_PHYSICAL_PORTS;
}

void PnetIntegrationTestBase::run_stack (int us)
{
   uint16_t slot = 0;

   for (int tmr = 0; tmr < us / TEST_TICK_INTERVAL_US; tmr++)
   {
      /* Set new output data every 10 ticks */
      appdata.tick_ctr++;
      if ((appdata.main_arep != 0) && (appdata.tick_ctr > 10))
      {
         appdata.tick_ctr = 0;
         appdata.inputdata[0] = appdata.counter_data++;

         /* Set data for custom input modules, if any */
         for (slot = 0; slot < PNET_MAX_SLOTS; slot++)
         {
            if (appdata.custom_input_slots[slot] == true)
            {
               (void)pnet_input_set_data_and_iops (
                  net,
                  TEST_API_IDENT,
                  slot,
                  TEST_SUBMOD_CUSTOM_IDENT,
                  appdata.inputdata,
                  TEST_DATASIZE_INPUT,
                  PNET_IOXS_GOOD);
            }
         }
      }

      /* Run stack functionality every tick */
      pnet_handle_periodic (net);
      mock_os_data.current_time_us += TEST_TICK_INTERVAL_US;
   }
}

void PnetIntegrationTestBase::send_data (uint8_t * data_packet, uint16_t len)
{
   int ret;
   pnal_buf_t * p_buf;
   uint8_t * p_ctr;

   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   if (p_buf == NULL)
   {
      TEST_TRACE ("(%d): Out of memory in send_data\n", __LINE__);
   }
   else
   {
      memcpy (p_buf->payload, data_packet, len);

      /* Insert frame time, store in big-endian */
      appdata.data_cycle_ctr++;
      p_ctr = &((uint8_t *)(p_buf->payload))[len - 4];
      *(p_ctr + 0) = (appdata.data_cycle_ctr >> 8) & 0xff;
      *(p_ctr + 1) = appdata.data_cycle_ctr & 0xff;

      p_buf->len = len;
      ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
      EXPECT_EQ (ret, 1);
      if (ret == 0)
      {
         TEST_TRACE ("(%d): Unhandled p_buf\n", __LINE__);
         pnal_buf_free (p_buf);
      }
   }
}
