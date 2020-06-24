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


/************************** Utilities ****************************************/

/* Send raw Ethernet test data
 *
 * @param net     InOut: Stack handle
 * @param         InOut: Cycle counter
 * @param         In: Data packet
 * @param         In: Length of data packet
 */
void send_data(
   pnet_t                  *net,
   uint16_t                *cycle_counter,
   uint8_t                 *data_packet,
   uint16_t                len)
{
   int                     ret;
   os_buf_t                *p_buf;
   uint8_t                 *p_ctr;

   p_buf = os_buf_alloc(PF_FRAME_BUFFER_SIZE);
   if (p_buf == NULL)
   {
      printf("(%d): Out of memory in send_data\n", __LINE__);
   }
   else
   {
      memcpy(p_buf->payload, data_packet, len);

      /* Insert frame time, store in big-endian */
      (*cycle_counter)++;
      p_ctr = &((uint8_t*)(p_buf->payload))[len - 4];
      *(p_ctr + 0) = (*cycle_counter >> 8) & 0xff;
      *(p_ctr + 1) = *cycle_counter & 0xff;

      p_buf->len = len;
      ret = pf_eth_recv(net, p_buf);
      EXPECT_EQ(ret, 1);
      if (ret == 0)
      {
         printf("(%d): Unhandled p_buf\n", __LINE__);
         os_buf_free(p_buf);
      }

      os_usleep(TEST_DATA_DELAY);
   }
}

/*
 * This is a timer callback,
 * and the arguments should fulfill the requirements of os_timer_create()
 */
void run_periodic(os_timer_t *p_timer, void *p_arg)
{
   app_data_and_stack_for_testing_t       *appdata_and_stack = (app_data_and_stack_for_testing_t*)p_arg;
   uint16_t                               slot = 0;

   /* Set new output data every 10ms */
   appdata_and_stack->appdata->tick_ctr++;
   if ((appdata_and_stack->appdata->main_arep != 0) && (appdata_and_stack->appdata->tick_ctr > 10))
   {
      appdata_and_stack->appdata->tick_ctr = 0;
      appdata_and_stack->appdata->inputdata[0] = appdata_and_stack->appdata->data_ctr++;

      /* Set data for custom input modules, if any */
      for (slot = 0; slot < PNET_MAX_MODULES; slot++)
      {
         if (appdata_and_stack->appdata->custom_input_slots[slot] == true)
         {
            (void)pnet_input_set_data_and_iops(appdata_and_stack->net, TEST_API_IDENT, slot, TEST_SUBMOD_CUSTOM_IDENT, appdata_and_stack->appdata->inputdata, TEST_DATASIZE_INPUT, PNET_IOXS_GOOD);
         }
      }
   }

   pnet_handle_periodic(appdata_and_stack->net);
}


/******************** Callbacks defined by p-net *****************************/

int my_connect_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_result_t *p_result)
{
   app_data_for_testing_t     *p_appdata = (app_data_for_testing_t*)arg;

   p_appdata->call_counters.connect_calls++;
   return 0;
}

int my_release_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_result_t *p_result)
{
   app_data_for_testing_t     *p_appdata = (app_data_for_testing_t*)arg;

   p_appdata->call_counters.release_calls++;
   return 0;
}

int my_dcontrol_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_control_command_t control_command,
   pnet_result_t *p_result)
{
   app_data_for_testing_t     *p_appdata = (app_data_for_testing_t*)arg;

   p_appdata->call_counters.dcontrol_calls++;
   return 0;
}

int my_ccontrol_cnf(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_result_t *p_result)
{
   app_data_for_testing_t     *p_appdata = (app_data_for_testing_t*)arg;

   p_appdata->call_counters.ccontrol_calls++;
   return 0;
}

<<<<<<< HEAD
static int my_signal_led_ind(
   pnet_t *net,
   void *arg,
   bool led_state)
{
   app_data_for_testing_t     *p_appdata = (app_data_for_testing_t*)arg;

   printf("Callback on set LED state: %u\n", led_state);
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

=======
>>>>>>> 3e70de9d4b3b77af8bb756b147213ffd1c989e1f
int my_read_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
<<<<<<< HEAD
   uint32_t api,
=======
   uint16_t api,
>>>>>>> 3e70de9d4b3b77af8bb756b147213ffd1c989e1f
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint8_t **pp_read_data, /* Out: A pointer to the data */
   uint16_t *p_read_length, /* Out: Size of data */
   pnet_result_t *p_result) /* Error status if returning != 0 */
{
   app_data_for_testing_t     *p_appdata = (app_data_for_testing_t*)arg;

   printf("Callback on read\n");
   printf("  API: %u Slot: %u Subslot: %u Index: %u Sequence: %u\n", api, slot, subslot, idx, sequence_number);
   p_appdata->call_counters.read_calls++;
   return 0;
}

int my_write_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
<<<<<<< HEAD
   uint32_t api,
=======
   uint16_t api,
>>>>>>> 3e70de9d4b3b77af8bb756b147213ffd1c989e1f
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint16_t write_length,
   uint8_t *p_write_data,
   pnet_result_t *p_result)
{
   app_data_for_testing_t     *p_appdata = (app_data_for_testing_t*)arg;

   printf("Callback on write\n");
   printf("  API: %u Slot: %u Subslot: %u Index: %u Sequence: %u Len: %u\n", api, slot, subslot, idx, sequence_number, write_length);
   p_appdata->call_counters.write_calls++;
   return 0;
}

int my_new_data_status_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   uint32_t crep,
   uint8_t changes,
   uint8_t data_status)
{
   printf("Callback on new data\n");
   return 0;
}

int my_alarm_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t data_len,
   uint16_t data_usi,
   uint8_t *p_data)
{
   printf("Callback on alarm\n");
   return 0;
}

int my_alarm_cnf(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_pnio_status_t *p_pnio_status)
{
   printf("Callback on alarm confirmation\n");
   return 0;
}

int my_state_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_event_values_t state)
{
   app_data_for_testing_t     *p_appdata = (app_data_for_testing_t*)arg;
   int                        ret;
   uint16_t                   slot = 0;
   uint16_t                   err_cls = 0;
   uint16_t                   err_code = 0;

   p_appdata->call_counters.state_calls++;
   p_appdata->main_arep = arep;
   p_appdata->cmdev_state = state;

   if (state == PNET_EVENT_PRMEND)
   {
      /* Set IOPS for DAP slot (has same numbering as the module identifiers) */
      ret = pnet_input_set_data_and_iops(net, TEST_API_IDENT, TEST_SLOT_DAP_IDENT, TEST_SUBMOD_DAP_IDENT,                    NULL, 0, PNET_IOXS_GOOD);
      EXPECT_EQ(ret, 0);
      ret = pnet_input_set_data_and_iops(net, TEST_API_IDENT, TEST_SLOT_DAP_IDENT, TEST_SUBMOD_DAP_INTERFACE_1_IDENT,        NULL, 0, PNET_IOXS_GOOD);
      EXPECT_EQ(ret, 0);
      ret = pnet_input_set_data_and_iops(net, TEST_API_IDENT, TEST_SLOT_DAP_IDENT, TEST_SUBMOD_DAP_INTERFACE_1_PORT_0_IDENT, NULL, 0, PNET_IOXS_GOOD);
      EXPECT_EQ(ret, 0);

      /* Set initial data and IOPS for custom input modules, and IOCS for custom output modules */
      for (slot = 0; slot < PNET_MAX_MODULES; slot++)
      {
         if (p_appdata->custom_input_slots[slot] == true)
         {
            ret = pnet_input_set_data_and_iops(net, TEST_API_IDENT, slot, TEST_SUBMOD_CUSTOM_IDENT, p_appdata->inputdata, TEST_DATASIZE_INPUT, PNET_IOXS_GOOD);
            EXPECT_EQ(ret, 0);
         }
         if (p_appdata->custom_output_slots[slot] == true)
         {
            ret = pnet_output_set_iocs(net, TEST_API_IDENT, slot, TEST_SUBMOD_CUSTOM_IDENT, PNET_IOXS_GOOD);
            EXPECT_EQ(ret, 0);
         }
      }

      ret = pnet_set_provider_state(net, true);
      EXPECT_EQ(ret, 0);
   }
   else if (state == PNET_EVENT_ABORT)
   {
      ret = pnet_get_ar_error_codes(net, arep, &err_cls, &err_code);
      EXPECT_EQ(ret, 0);
      printf("ABORT err_cls 0x%02x  err_code 0x%02x\n", (unsigned)err_cls, (unsigned)err_code);
   }

   return 0;
}

int my_exp_module_ind(
   pnet_t *net,
   void *arg,
<<<<<<< HEAD
   uint32_t api,
=======
   uint16_t api,
>>>>>>> 3e70de9d4b3b77af8bb756b147213ffd1c989e1f
   uint16_t slot,
   uint32_t module_ident)
{
   app_data_for_testing_t     *p_appdata = (app_data_for_testing_t*)arg;
   int                        ret = -1;   /* Not supported in specified slot */
   bool                       found = false;
   uint16_t                   ix;

   printf("Callback on module\n");

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
      printf("  Plug module.    API: %u Slot: %u Module ID: %" PRIu32 " Index in list of supported modules: %u\n", api, slot, module_ident, ix);
      ret = pnet_plug_module(net, api, slot, module_ident);
      EXPECT_EQ(ret, 0);

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
      printf("  Module ident %08x not found\n", (unsigned)module_ident);
      EXPECT_TRUE(false);  // Fail the test
   }

   return ret;
}

int my_exp_submodule_ind(
   pnet_t *net,
   void *arg,
<<<<<<< HEAD
   uint32_t api,
=======
   uint16_t api,
>>>>>>> 3e70de9d4b3b77af8bb756b147213ffd1c989e1f
   uint16_t slot,
   uint16_t subslot,
   uint32_t module_ident,
   uint32_t submodule_ident)
{
   app_data_for_testing_t     *p_appdata = (app_data_for_testing_t*)arg;
   int                        ret = -1;
   bool                       found = false;
   uint16_t                   ix = 0;

   printf("Callback on submodule\n");

   /* Find it in the list of supported submodules */
   ix = 0;
   while (ix < TEST_MAX_NUMBER_AVAILABLE_SUBMODULE_TYPES)
   {
      if (p_appdata->available_submodule_types[ix].module_ident_number == module_ident &&
         p_appdata->available_submodule_types[ix].submodule_ident_number== submodule_ident)
      {
         found = true;
         break;
      }
      ix++;
   }

   if (found == true)
   {
      printf("  Plug submodule. API: %u Slot: %u Subslot: %u Module ID: %" PRIu32 " Submodule ID: %" PRIu32 " (Index in available submodules: %u) Direction: %u Len in: %u out: %u\n",
        api, slot, subslot, module_ident, submodule_ident, ix,
        p_appdata->available_submodule_types[ix].direction,
        p_appdata->available_submodule_types[ix].input_length,
        p_appdata->available_submodule_types[ix].output_length);
      ret = pnet_plug_submodule(net, api, slot, subslot, module_ident, submodule_ident,
         p_appdata->available_submodule_types[ix].direction,
         p_appdata->available_submodule_types[ix].input_length,
         p_appdata->available_submodule_types[ix].output_length);
      EXPECT_EQ(ret, 0);
   }
   else
   {
      printf("  Sub-module ident %08x not found\n", (unsigned)submodule_ident);
      EXPECT_TRUE(false);  // Fail the test
   }

   return ret;
}


/*********************** Base classes for tests *****************************/

void PnetIntegrationTestBase::appdata_init()
{
   memset(&appdata, 0, sizeof(appdata));
}

void PnetIntegrationTestBase::callcounter_reset()
{
   memset(&appdata.call_counters, 0, sizeof(call_counters_t));
}

void PnetIntegrationTestBase::available_modules_and_submodules_init()
{
   appdata.available_module_types[0] = TEST_MOD_DAP_IDENT;
   appdata.available_module_types[1] = TEST_MOD_8_8_IDENT;
   appdata.available_module_types[2] = TEST_MOD_8_0_IDENT;

   appdata.available_submodule_types[0].module_ident_number = TEST_MOD_DAP_IDENT;
   appdata.available_submodule_types[0].submodule_ident_number = TEST_SUBMOD_DAP_IDENT;
   appdata.available_submodule_types[0].direction = PNET_DIR_NO_IO;
   appdata.available_submodule_types[0].input_length = 0;
   appdata.available_submodule_types[0].output_length = 0;

   appdata.available_submodule_types[1].module_ident_number = TEST_MOD_DAP_IDENT;
   appdata.available_submodule_types[1].submodule_ident_number = TEST_SUBMOD_DAP_INTERFACE_1_IDENT;
   appdata.available_submodule_types[1].direction = PNET_DIR_NO_IO;
   appdata.available_submodule_types[1].input_length = 0;
   appdata.available_submodule_types[1].output_length = 0;

   appdata.available_submodule_types[2].module_ident_number = TEST_MOD_DAP_IDENT;
   appdata.available_submodule_types[2].submodule_ident_number = TEST_SUBMOD_DAP_INTERFACE_1_PORT_0_IDENT;
   appdata.available_submodule_types[2].direction = PNET_DIR_NO_IO;
   appdata.available_submodule_types[2].input_length = 0;
   appdata.available_submodule_types[2].output_length = 0;

   appdata.available_submodule_types[3].module_ident_number = TEST_MOD_8_8_IDENT;
   appdata.available_submodule_types[3].submodule_ident_number = TEST_SUBMOD_CUSTOM_IDENT;
   appdata.available_submodule_types[3].direction = PNET_DIR_IO;
   appdata.available_submodule_types[3].input_length = TEST_DATASIZE_INPUT;
   appdata.available_submodule_types[3].output_length = TEST_DATASIZE_OUTPUT;

   appdata.available_submodule_types[4].module_ident_number = TEST_MOD_8_0_IDENT;
   appdata.available_submodule_types[4].submodule_ident_number = TEST_SUBMOD_CUSTOM_IDENT;
   appdata.available_submodule_types[4].direction = PNET_DIR_OUTPUT;
   appdata.available_submodule_types[4].input_length = 0;
   appdata.available_submodule_types[4].output_length = TEST_DATASIZE_OUTPUT;
}

void PnetIntegrationTestBase::cfg_init()
{
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
<<<<<<< HEAD
   pnet_default_cfg.signal_led_cb = my_signal_led_ind;
=======
>>>>>>> 3e70de9d4b3b77af8bb756b147213ffd1c989e1f
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

   strcpy(pnet_default_cfg.station_name, "");
   strcpy(pnet_default_cfg.device_vendor, "rt-labs");
   strcpy(pnet_default_cfg.manufacturer_specific_string, "PNET demo");

   strcpy(pnet_default_cfg.lldp_cfg.chassis_id, "rt-labs demo system"); /* Is this a valid name? '-' allowed?*/
   strcpy(pnet_default_cfg.lldp_cfg.port_id, "port-001");
   pnet_default_cfg.lldp_cfg.ttl = 20; /* seconds */
   pnet_default_cfg.lldp_cfg.rtclass_2_status = 0;
   pnet_default_cfg.lldp_cfg.rtclass_3_status = 0;
   pnet_default_cfg.lldp_cfg.cap_aneg = 3; /* Supported (0x01) + enabled (0x02) */
   pnet_default_cfg.lldp_cfg.cap_phy = 0x8000; /* Unknown (0x8000) */
   pnet_default_cfg.lldp_cfg.mau_type = 0x0000; /* Unknown */

   /* Network configuration */
   pnet_default_cfg.send_hello = 1; /* Send HELLO */
   pnet_default_cfg.dhcp_enable = 0;
   pnet_default_cfg.ip_addr.a = 192;
   pnet_default_cfg.ip_addr.b = 168;
   pnet_default_cfg.ip_addr.c = 1;
   pnet_default_cfg.ip_addr.d = 171;
   pnet_default_cfg.ip_mask.a = 255;
   pnet_default_cfg.ip_mask.b = 255;
   pnet_default_cfg.ip_mask.c = 255;
   pnet_default_cfg.ip_mask.d = 255;
   pnet_default_cfg.ip_gateway.a = 192;
   pnet_default_cfg.ip_gateway.b = 168;
   pnet_default_cfg.ip_gateway.c = 1;
   pnet_default_cfg.ip_gateway.d = 1;

   pnet_default_cfg.im_0_data.vendor_id_hi = 0x00;
   pnet_default_cfg.im_0_data.vendor_id_lo = 0x01;
   strcpy(pnet_default_cfg.im_0_data.order_id, "<orderid>           ");
   strcpy(pnet_default_cfg.im_0_data.im_serial_number, "<serial nbr>    ");
   pnet_default_cfg.im_0_data.im_hardware_revision = 1;
   pnet_default_cfg.im_0_data.sw_revision_prefix = 'P'; /* 'V', 'R', 'P', 'U', or 'T' */
   pnet_default_cfg.im_0_data.im_sw_revision_functional_enhancement = 0;
   pnet_default_cfg.im_0_data.im_sw_revision_bug_fix = 0;
   pnet_default_cfg.im_0_data.im_sw_revision_internal_change = 0;
   pnet_default_cfg.im_0_data.im_revision_counter = 0;
   pnet_default_cfg.im_0_data.im_profile_id = 0x1234;
   pnet_default_cfg.im_0_data.im_profile_specific_type = 0x5678;
   pnet_default_cfg.im_0_data.im_version_major = 0;
   pnet_default_cfg.im_0_data.im_version_minor = 1;
   pnet_default_cfg.im_0_data.im_supported = 0x001e; /* Only I&M0..I&M4 supported */
   strcpy(pnet_default_cfg.im_1_data.im_tag_function, "");
   strcpy(pnet_default_cfg.im_1_data.im_tag_location, "");
   strcpy(pnet_default_cfg.im_2_data.im_date, "");
   strcpy(pnet_default_cfg.im_3_data.im_descriptor, "");
   strcpy(pnet_default_cfg.im_4_data.im_signature, "");
}
