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

/**
 * @file
 * @brief Integration test that a LLDP frame is sent at init.
 *
 */

#include "pf_includes.h"

#include <gtest/gtest.h>

#include "mocks.h"
#include "test_util.h"

 // Test fixture

#define TEST_UDP_DELAY     (100*1000)     /* us */
#define TEST_TIMEOUT_DELAY (3*1000*1000)  /* us */
#define TICK_INTERVAL_US   1000           /* us */

static uint16_t            state_calls = 0;
static uint16_t            connect_calls = 0;
static uint16_t            release_calls = 0;
static uint16_t            dcontrol_calls = 0;
static uint16_t            ccontrol_calls = 0;
static uint16_t            read_calls = 0;
static uint16_t            write_calls = 0;

static uint32_t            main_arep = 0;
static uint32_t            tick_ctr = 0;
static uint8_t             data[1] = { 0 };
static uint32_t            data_ctr = 0;
static os_timer_t          *periodic_timer = NULL;
static pnet_event_values_t cmdev_state;

static pnet_t              *g_pnet;

/**
 * This is just a simple example on how the application can maintain its list of supported APIs, modules and submodules.
 * If modules are supported in all slots > 0, then this is clearly overkill.
 * In that case the application must build a database conting information about which (sub-)modules are in where.
 */
static uint32_t            cfg_module_ident_numbers[] =
{
    /* Order is not important */
    0x00000032,
};

typedef struct cfg_submodules
{
   uint32_t                module_ident_number;
   uint32_t                submodule_ident_number;
   pnet_submodule_dir_t    direction;
   uint16_t                input_length;
   uint16_t                output_length;
} cfg_submodules_t;

static cfg_submodules_t    cfg_submodules[1];

static int my_connect_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_result_t *p_result);
static int my_release_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_result_t *p_result);
static int my_dcontrol_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_control_command_t control_command,
   pnet_result_t *p_result);
static int my_ccontrol_cnf(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_result_t *p_result);
static int my_state_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_event_values_t state);
static int my_read_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   uint16_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint8_t **pp_read_data, /* Out: A pointer to the data */
   uint16_t *p_read_length, /* Out: Size of data */
   pnet_result_t *p_result); /* Error status if returning != 0 */
static int my_write_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   uint16_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint16_t write_length,
   uint8_t *p_write_data,
   pnet_result_t *p_result);
static int my_exp_module_ind(
   pnet_t *net,
   void *arg,
   uint16_t api,
   uint16_t slot,
   uint32_t module_ident);
static int my_exp_submodule_ind(
   pnet_t *net,
   void *arg,
   uint16_t api,
   uint16_t slot,
   uint16_t subslot,
   uint32_t module_ident,
   uint32_t submodule_ident);
static int my_new_data_status_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   uint32_t crep,
   uint8_t changes,
   uint8_t data_status);
static int my_alarm_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t data_len,
   uint16_t data_usi,
   uint8_t *p_data);
static int my_alarm_cnf(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_pnio_status_t *p_pnio_status);

static void test_periodic(os_timer_t *p_timer, void *p_arg)
{
   /* Set new output data every 10ms */
   tick_ctr++;
   if ((main_arep != 0) && (tick_ctr > 10))
   {
      tick_ctr = 0;
      data[0] = data_ctr++;
      pnet_input_set_data_and_iops(g_pnet, 0, 1, 1, data, sizeof(data), PNET_IOXS_GOOD);
   }

   pnet_handle_periodic(g_pnet);
}

class LldpTest : public ::testing::Test
{
protected:
   virtual void SetUp()
   {
      mock_init();
      init_cfg();
      counter_reset();

      g_pnet = pnet_init("en1", TICK_INTERVAL_US, &pnet_default_cfg);
      /* Do not clear counters here - we need to verify send at init from LLDP */

      periodic_timer = os_timer_create(TICK_INTERVAL_US, test_periodic, NULL, false);
      os_timer_start(periodic_timer);
   };

   pnet_cfg_t pnet_default_cfg;

   void counter_reset()
   {
      state_calls = 0;
      connect_calls = 0;
      release_calls = 0;
      dcontrol_calls = 0;
      ccontrol_calls = 0;
      read_calls = 0;
      write_calls = 0;
   }

   void init_cfg()
   {
      cfg_submodules[0].module_ident_number = 0x00000032;
      cfg_submodules[0].submodule_ident_number = 0x00000001;
      cfg_submodules[0].direction = PNET_DIR_IO;
      cfg_submodules[0].input_length = 1;
      cfg_submodules[0].output_length = 1;

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
      pnet_default_cfg.reset_cb = NULL;
      pnet_default_cfg.cb_arg = NULL;

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
};

static int my_connect_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_result_t *p_result)
{
   connect_calls++;
   return 0;
}

static int my_release_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_result_t *p_result)
{
   release_calls++;
   return 0;
}

static int my_dcontrol_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_control_command_t control_command,
   pnet_result_t *p_result)
{
   dcontrol_calls++;
   return 0;
}

static int my_ccontrol_cnf(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_result_t *p_result)
{
   ccontrol_calls++;
   return 0;
}

static int my_state_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_event_values_t state)
{
   state_calls++;
   main_arep = arep;
   cmdev_state = state;
   return 0;
}

static int my_read_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   uint16_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint8_t **pp_read_data, /* Out: A pointer to the data */
   uint16_t *p_read_length, /* Out: Size of data */
   pnet_result_t *p_result) /* Error status if returning != 0 */
{
   read_calls++;
   return 0;
}
static int my_write_ind(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   uint16_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint16_t write_length,
   uint8_t *p_write_data,
   pnet_result_t *p_result)
{
   write_calls++;
   return 0;
}

static int my_exp_module_ind(
   pnet_t *net,
   void *arg,
   uint16_t api,
   uint16_t slot,
   uint32_t module_ident)
{
   int                     ret = -1;   /* Not supported in specified slot */
   uint16_t                ix;

   if (slot > 0)
   {
      /* Find it in the list of supported modules */
      ix = 0;
      while ((ix < NELEMENTS(cfg_module_ident_numbers)) &&
             (cfg_module_ident_numbers[ix] != module_ident))
      {
         ix++;
      }

      if (ix < NELEMENTS(cfg_module_ident_numbers))
      {
         /* For now support any module in any slot */
         ret = pnet_plug_module(net, api, slot, module_ident);
      }
   }
   else
   {
      /* Just accept how the DAP looks. */
      ret = 0;
   }

   return ret;
}

static int my_exp_submodule_ind(
   pnet_t *net,
   void *arg,
   uint16_t api,
   uint16_t slot,
   uint16_t subslot,
   uint32_t module_ident,
   uint32_t submodule_ident)
{
   int                     ret = -1;
   uint16_t                ix = 0;

   /* Find it in the list of supported submodules */
   ix = 0;
   while ((ix < NELEMENTS(cfg_submodules)) &&
          ((cfg_submodules[ix].module_ident_number != module_ident) ||
           (cfg_submodules[ix].submodule_ident_number != submodule_ident)))
   {
      ix++;
   }
   if (ix < NELEMENTS(cfg_submodules))
   {
      ret = pnet_plug_submodule(net, api, slot, subslot, module_ident, submodule_ident,
         cfg_submodules[ix].direction,
         cfg_submodules[ix].input_length, cfg_submodules[ix].output_length);
   }

   return ret;
}

static int my_new_data_status_ind(
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

static int my_alarm_ind(
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

static int my_alarm_cnf(
   pnet_t *net,
   void *arg,
   uint32_t arep,
   pnet_pnio_status_t *p_pnio_status)
{
   printf("Callback on alarm confirmation\n");
   return 0;
}

// Tests

TEST_F(LldpTest, LldpInitTest)
{
   /* Verify that LLDP has sent a frame at init */
   EXPECT_EQ(mock_os_eth_send_count, 1);
   EXPECT_EQ(0, state_calls);
   EXPECT_EQ(0, connect_calls);
   EXPECT_EQ(0, release_calls);
   EXPECT_EQ(0, dcontrol_calls);
   EXPECT_EQ(0, ccontrol_calls);
   EXPECT_EQ(0, read_calls);
   EXPECT_EQ(0, write_calls);
}
