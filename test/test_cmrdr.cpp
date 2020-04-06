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
 * @brief Integration testing of CMRDR.
 *
 * Using for example:
 *  pnet_application_ready()
 *  pnet_output_get_data_and_iops()
 *  pnet_input_get_iocs()
 *  pnet_input_set_data_and_iops()
 *  pnet_output_set_iocs()
 *  pnet_create_log_book_entry()
 *  pnet_diag_add()
 *
 */

#include "pf_includes.h"

#include <gtest/gtest.h>

#include "mocks.h"
#include "test_util.h"

// Test fixture

#define TEST_UDP_DELAY     (500*1000)     /* us */
#define TEST_DATA_DELAY    (2*1000)       /* us */
#define TEST_TIMEOUT_DELAY (3*1000*1000)  /* us */
#define TICK_INTERVAL_US   1000           /* us */

static pnet_t              *g_pnet;

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
static uint16_t            data_cycle_ctr = 0x0;

static uint16_t            read_fails = 0;
static uint32_t            sub_ident_offset = 0;   /* Used to create module diff */
/**
 * This is just a simple example on how the application can maintain its list of supported APIs, modules and submodules.
 * If modules are supported in all slots > 0, then this is clearly overkill.
 * In that case the application must build a database conting information about which (sub-)modules are in where.
 */
static uint32_t            cfg_module_ident_numbers[] =
{
    /* Order is not important */
    0x00000001,
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

static cfg_submodules_t    cfg_submodules[4];

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

class CmrdrTest : public ::testing::Test
{
protected:
   virtual void SetUp()
   {
      mock_init();
      init_cfg();
      counter_reset();

      g_pnet = pnet_init("en1", TICK_INTERVAL_US, &pnet_default_cfg);
      mock_clear();        /* lldp send a frame at init */

      periodic_timer = os_timer_create(TICK_INTERVAL_US, test_periodic, NULL, false);
      os_timer_start(periodic_timer);
   };

   pnet_cfg_t     pnet_default_cfg;

   void counter_reset()
   {
      state_calls = 0;
      connect_calls = 0;
      release_calls = 0;
      dcontrol_calls = 0;
      ccontrol_calls = 0;
      read_calls = 0;
      write_calls = 0;

      read_fails = 0;
   }

   void init_cfg()
   {
      cfg_submodules[0].module_ident_number = 0x00000001;
      cfg_submodules[0].submodule_ident_number = 0x00000001;
      cfg_submodules[0].direction = PNET_DIR_NO_IO;
      cfg_submodules[0].input_length = 0;
      cfg_submodules[0].output_length = 0;

      cfg_submodules[1].module_ident_number = 0x00000001;
      cfg_submodules[1].submodule_ident_number = 0x00008000;
      cfg_submodules[1].direction = PNET_DIR_NO_IO;
      cfg_submodules[1].input_length = 0;
      cfg_submodules[1].output_length = 0;

      cfg_submodules[2].module_ident_number = 0x00000001;
      cfg_submodules[2].submodule_ident_number = 0x00008001;
      cfg_submodules[2].direction = PNET_DIR_NO_IO;
      cfg_submodules[2].input_length = 0;
      cfg_submodules[2].output_length = 0;

      cfg_submodules[3].module_ident_number = 0x00000032;
      cfg_submodules[3].submodule_ident_number = 0x00000001;
      cfg_submodules[3].direction = PNET_DIR_IO;
      cfg_submodules[3].input_length = 1;
      cfg_submodules[3].output_length = 1;

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
   int ret;
   uint16_t    err_cls = 0;
   uint16_t    err_code = 0;

   printf("New event: %u\n", state);
   state_calls++;
   main_arep = arep;
   cmdev_state = state;
   if (state == PNET_EVENT_PRMEND)
   {
      ret = pnet_input_set_data_and_iops(net, 0, 0, 1, NULL, 0, PNET_IOXS_GOOD);
      EXPECT_EQ(ret, 0);
      ret = pnet_input_set_data_and_iops(net, 0, 0, 0x8000, NULL, 0, PNET_IOXS_GOOD);
      EXPECT_EQ(ret, 0);
      ret = pnet_input_set_data_and_iops(net, 0, 0, 0x8001, NULL, 0, PNET_IOXS_GOOD);
      EXPECT_EQ(ret, 0);
      ret = pnet_input_set_data_and_iops(net, 0, 1, 1, data, sizeof(data), PNET_IOXS_GOOD);
      EXPECT_EQ(ret, 0);
      ret = pnet_output_set_iocs(net, 0, 1, 1, PNET_IOXS_GOOD);
      EXPECT_EQ(ret, 0);
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
      ret = pnet_plug_submodule(net, api, slot, subslot, module_ident, submodule_ident + sub_ident_offset,
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
   return 0;
}

// Tests

static uint8_t connect_req[] =
{
                                                             0x04, 0x00, 0x28, 0x00, 0x10, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
 0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 0xdf, 0x7d, 0xbb, 0xac, 0x97, 0xe2, 0x76, 0x54, 0x9f, 0x47, 0xa5, 0xbd, 0xa5, 0xe3, 0x7d, 0x98,
 0xe5, 0xda, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x86, 0x01, 0x00, 0x00, 0x00, 0x00, 0x24, 0x10, 0x00, 0x00, 0x72, 0x01,
 0x00, 0x00, 0x24, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0x01, 0x00, 0x00, 0x01, 0x01,
 0x00, 0x42, 0x01, 0x00, 0x00, 0x01, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x02, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, 0xde, 0xa0,
 0x00, 0x00, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0x01, 0xf0, 0x00, 0x00, 0x01, 0x40, 0x00,
 0x00, 0x11, 0x02, 0x58, 0x88, 0x92, 0x00, 0x0c, 0x72, 0x74, 0x2d, 0x6c, 0x61, 0x62, 0x73, 0x2d,
 0x64, 0x65, 0x6d, 0x6f, 0x01, 0x02, 0x00, 0x50, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x88, 0x92,
 0x00, 0x00, 0x00, 0x02, 0x00, 0x28, 0x80, 0x01, 0x00, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x00, 0x03, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
 0x80, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03,
 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x05, 0x01, 0x02, 0x00, 0x50, 0x01, 0x00, 0x00, 0x02,
 0x00, 0x02, 0x88, 0x92, 0x00, 0x00, 0x00, 0x02, 0x00, 0x28, 0x80, 0x00, 0x00, 0x20, 0x00, 0x01,
 0x00, 0x01, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x03, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x01,
 0x00, 0x00, 0x80, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03, 0x01, 0x04, 0x00, 0x3c,
 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01,
 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x80, 0x01,
 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x04, 0x00, 0x26,
 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01,
 0x00, 0x02, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x16, 0x01, 0x00, 0x00, 0x01, 0x88, 0x92,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x02, 0x00, 0xc8, 0xc0, 0x00, 0xa0, 0x00
};

static uint8_t release_req[] =
{
                                                             0x04, 0x00, 0x28, 0x00, 0x10, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
 0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 0xdf, 0x7d, 0xbb, 0xac, 0x97, 0xe2, 0x76, 0x54, 0x9f, 0x47, 0xa5, 0xbd, 0xa5, 0xe3, 0x7d, 0x98,
 0xe5, 0xda, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x20, 0x00,
 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x14,
 0x00, 0x1c, 0x01, 0x00, 0x00, 0x00, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00
};

static uint8_t prm_end_req[] =
{
                                                             0x04, 0x00, 0x28, 0x00, 0x10, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
 0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 0xdf, 0x7d, 0xbb, 0xac, 0x97, 0xe2, 0x76, 0x54, 0x9f, 0x47, 0xa5, 0xbd, 0xa5, 0xe3, 0x7d, 0x98,
 0xe5, 0xda, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x20, 0x00,
 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x10,
 0x00, 0x1c, 0x01, 0x00, 0x00, 0x00, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00
};

static uint8_t appl_rdy_rsp[] =
{
                                                             0x04, 0x02, 0x0a, 0x00, 0x10, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x00, 0xbe, 0xef,
 0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 0xdf, 0x7d, 0x79, 0x56, 0x34, 0x12, 0x34, 0x12, 0x78, 0x56, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
 0x07, 0x08, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00,
 0x00, 0x00, 0xdc, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x81, 0x12,
 0x00, 0x1c, 0x01, 0x00, 0x00, 0x00, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00
};

static uint8_t data_packet_good_iops_good_iocs[] =
{
 0x1e, 0x30, 0x6c, 0xa2, 0x45, 0x5e, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, 0x88, 0x92, 0x80, 0x00,
 0x80, 0x80, 0x80, 0x80, 0x23, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf6, 0x35, 0x00
};

static void send_data(
   pnet_t                  *net,
   uint8_t                 *data_packet,
   uint16_t                len)
{
   int                     ret;
   os_buf_t                *p_buf;
   uint8_t                 *p_ctr;

   p_buf = os_buf_alloc(1500);
   if (p_buf == NULL)
   {
      printf("(%d): Out of memory in send_data\n", __LINE__);
   }
   else
   {
      memcpy(p_buf->payload, data_packet, len);

      /* Insert frame time */
      data_cycle_ctr++;
      p_ctr = &((uint8_t*)(p_buf->payload))[len-4];
      /* Store in big-endian */
      *(p_ctr + 0) = (data_cycle_ctr >> 8) & 0xff;
      *(p_ctr + 1) = data_cycle_ctr & 0xff;

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

typedef struct test_reads
{
   uint16_t                idx;
} test_reads_t;

static test_reads_t        test_mod_diff[] =
{
    0xe002
};

static test_reads_t        test_reads[] =
{
    0x8000, 0x8001,
    0x800a, 0x800b, 0x800c,
    0x8010, 0x8011, 0x8012, 0x8013,
    0x801e,
    0x8020,
    0x8027, 0x8028, 0x8029,
    0x802a, 0x802b,
    0x802c, 0x802d,
    0x802f,
    0x8030, 0x8031,
    0x8050, 0x8051, 0x8052, 0x8053, 0x8054, 0x8055, 0x8056, 0x8057,
    0x8060, 0x8061, 0x8062,
    0x8070, 0x8071, 0x8072,
    0x8080,
    0x8090,
    0x80af,
    0x80b0,
    0x80cf,
    0xaff0, 0xaff1, 0xaff2, 0xaff3, 0xaff4, 0xaff5, 0xaff6, 0xaff7, 0xaff8, 0xaff9, 0xaffa, 0xaffb, 0xaffc, 0xaffd, 0xaffe, 0xafff,
    0xc000, 0xc001,
    0xc00a, 0xc00b, 0xc00c,
    0xc010, 0xc011, 0xc012, 0xc013,
    0xe000, 0xe001, 0xe002,
    0xe00a, 0xe00b, 0xe00c,
    0xe010, 0xe011, 0xe012, 0xe013,
    0xe030, 0xe031,
    0xe040,
    0xe050,
    0xe060, 0xe061,
    0xf000,
    0xf00a, 0xf00b, 0xf00c,
    0xf010, 0xf011, 0xf012, 0xf013,
    0xf020,
    0xf80c,
    0xf820, 0xf821,
    0xf830, 0xf831,
    0xf840, 0xf841, 0xf842,
    0xf850,
    0xf860,
    0xf870, 0xf871,
    0xf880, 0xf881, 0xf882, 0xf883, 0xf884, 0xf885, 0xf886, 0xf887, 0xf888, 0xf889,
    0xfbff,
};

static pf_iod_read_request_t     read_request;
static pnet_result_t             read_status;
static uint16_t                  seq_nbr = 0;

void test_read(test_reads_t *p_the_test)
{
   pf_ar_t                 *p_ar;
   uint8_t                 buffer[1500];
   uint16_t                pos = 0;
   uint16_t                idx = p_the_test->idx;

   /* Send data to prevent timeout */
   send_data(g_pnet, data_packet_good_iops_good_iocs, sizeof(data_packet_good_iops_good_iocs));

   memset(&read_status, 0, sizeof(read_status));
   memset(&read_request, 0, sizeof(read_request));
   pf_ar_find_by_arep(g_pnet, main_arep, &p_ar);

   read_request.sequence_number = seq_nbr++;
   /* read_request.ar_uuid = NIL */
   read_request.api = 0;
   read_request.slot_number = 1;
   read_request.subslot_number = 1;
   read_request.index = idx;
   read_request.record_data_length = 0;
   // read_request.target_ar_uuid;   /* Only used if implicit AR */

   pf_cmrdr_rm_read_ind(g_pnet, p_ar, &read_request, &read_status, sizeof(buffer), buffer, &pos);

   if (read_status.pnio_status.error_code != 0)
   {
      printf("Read failed for idx %#x\n", (unsigned)idx);
      read_fails++;
   }
}

TEST_F (CmrdrTest, CmrdrRunTest)
{
   int                     ret;
   pnet_pnio_status_t      pnio_status = {1, 2, 3, 4};
   bool                    new_flag = false;
   uint8_t                 in_data[10];
   uint16_t                in_len = sizeof(in_data);
   uint8_t                 out_data[] = {
         0x33,       /* Slot 1, subslot 1 Data */
   };
   uint8_t                 iops = PNET_IOXS_BAD;
   uint8_t                 iocs = PNET_IOXS_BAD;
   uint32_t                ix;
   uint16_t                ch_properties = 0;

   printf("Line %d\n", __LINE__);
   mock_set_os_udp_recvfrom_buffer(connect_req, sizeof(connect_req));
   os_usleep(TEST_UDP_DELAY);
   EXPECT_EQ(state_calls, 1);
   EXPECT_EQ(cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ(connect_calls, 1);
   EXPECT_GT(mock_os_eth_send_count, 0);

   EXPECT_EQ(state_calls, 1);
   EXPECT_EQ(cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ(connect_calls, 1);

   printf("Line %d\n", __LINE__);
   mock_set_os_udp_recvfrom_buffer(prm_end_req, sizeof(prm_end_req));
   os_usleep(TEST_UDP_DELAY);

   EXPECT_EQ(state_calls, 2);
   EXPECT_EQ(cmdev_state, PNET_EVENT_PRMEND);
   EXPECT_EQ(connect_calls, 1);

   /* Simulate application calling APPL_RDY */
   printf("Line %d\n", __LINE__);
   ret = pnet_application_ready(g_pnet, main_arep);
   EXPECT_EQ(ret, 0);
   EXPECT_EQ(state_calls, 3);
   EXPECT_EQ(cmdev_state, PNET_EVENT_APPLRDY);

   printf("Line %d\n", __LINE__);
   mock_set_os_udp_recvfrom_buffer(appl_rdy_rsp, sizeof(appl_rdy_rsp));
   os_usleep(TEST_UDP_DELAY);
   EXPECT_EQ(state_calls, 3);
   EXPECT_EQ(cmdev_state, PNET_EVENT_APPLRDY);

   /* Perform the API calls: */

   /* Send a couple of data packets and verify reception */
   printf("Line %d\n", __LINE__);
   for (ix = 0; ix < 100; ix++)
   {
      send_data(g_pnet, data_packet_good_iops_good_iocs, sizeof(data_packet_good_iops_good_iocs));
   }

   printf("Line %d\n", __LINE__);
   iops = 88;     /* Something non-valid */
   in_len = sizeof(in_data);
   ret = pnet_output_get_data_and_iops(g_pnet, 0, 1, 1, &new_flag, in_data, &in_len, &iops);
   EXPECT_EQ(ret, 0);
   EXPECT_EQ(new_flag, true);
   EXPECT_EQ(in_len, 1);
   EXPECT_EQ(in_data[0], 0x23);
   EXPECT_EQ(iops, PNET_IOXS_GOOD);
   iocs = 77;     /* Something non-valid */
   ret = pnet_input_get_iocs(g_pnet, 0, 1, 1, &iocs);
   EXPECT_EQ(ret, 0);
   EXPECT_EQ(new_flag, true);
   EXPECT_EQ(in_len, 1);
   EXPECT_EQ(iocs, PNET_IOXS_GOOD);

   EXPECT_EQ(state_calls, 4);
   EXPECT_EQ(cmdev_state, PNET_EVENT_DATA);

   /* Send some data to the controller */
   printf("Line %d\n", __LINE__);
   ret = pnet_input_set_data_and_iops(g_pnet, 0, 1, 1, out_data, sizeof(out_data), PNET_IOXS_GOOD);
   EXPECT_EQ(ret, 0);

   /* Acknowledge the reception of controller data */
   ret = pnet_output_set_iocs(g_pnet, 0, 1, 1, PNET_IOXS_GOOD);
   EXPECT_EQ(ret, 0);
   EXPECT_EQ(state_calls, 4);
   EXPECT_EQ(cmdev_state, PNET_EVENT_DATA);

   /* Setup some record for the reader */

   /* Send data to avoid timeout */
   send_data(g_pnet, data_packet_good_iops_good_iocs, sizeof(data_packet_good_iops_good_iocs));

   printf("Line %d\n", __LINE__);
   /* Create a logbook entry */
   pnet_create_log_book_entry(g_pnet, main_arep, &pnio_status, 0x13245768);

   /* Create a diag and an alarm. */
   PNET_DIAG_CH_PROP_TYPE_SET(ch_properties, PNET_DIAG_CH_PROP_TYPE_8_BIT);
   PNET_DIAG_CH_PROP_ACC_SET(ch_properties, 0);
   PNET_DIAG_CH_PROP_MAINT_SET(ch_properties, PNET_DIAG_CH_PROP_MAINT_FAULT);
   PNET_DIAG_CH_PROP_SPEC_SET(ch_properties, PNET_DIAG_CH_PROP_SPEC_APPEARS);
   PNET_DIAG_CH_PROP_DIR_SET(ch_properties, PNET_DIAG_CH_PROP_DIR_OUTPUT);
   pnet_diag_add(g_pnet, main_arep, 0, 1, 1, 0, ch_properties, 0x0001, 0x0002, 0x00030004, 0, PNET_DIAG_USI_STD, NULL);

   printf("Number of tests: %u\n", (unsigned)NELEMENTS(test_reads));

   /* Now read all the records */
   for (ix = 0; ix < NELEMENTS(test_reads); ix++)
   {
      test_read(&test_reads[ix]);
   }
   EXPECT_EQ(read_fails, 60); // Currently expected number of fails.

   printf("Line %d\n", __LINE__);
   mock_set_os_udp_recvfrom_buffer(release_req, sizeof(release_req));
   os_usleep(TEST_UDP_DELAY);

   printf("Line %d\n", __LINE__);
   EXPECT_EQ(release_calls, 1);
   EXPECT_EQ(state_calls, 5);
   EXPECT_EQ(cmdev_state, PNET_EVENT_ABORT);
   printf("Line %d\n", __LINE__);
}

TEST_F (CmrdrTest, CmrdrModDiffTest)
{
   int                     ret;
   pnet_pnio_status_t      pnio_status = {1, 2, 3, 4};
   bool                    new_flag = false;
   uint8_t                 in_data[10];
   uint16_t                in_len = sizeof(in_data);
   uint8_t                 out_data[] = {
         0x33,       /* Slot 1, subslot 1 Data */
   };
   uint8_t                 iops = PNET_IOXS_BAD;
   uint8_t                 iocs = PNET_IOXS_BAD;
   uint32_t                ix;

   sub_ident_offset = 1;      /* Not the correct sub-module ident */

   printf("Line %d\n", __LINE__);
   mock_set_os_udp_recvfrom_buffer(connect_req, sizeof(connect_req));
   os_usleep(TEST_UDP_DELAY);
   EXPECT_EQ(state_calls, 1);
   EXPECT_EQ(cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ(connect_calls, 1);
   EXPECT_GT(mock_os_eth_send_count, 0);

   EXPECT_EQ(state_calls, 1);
   EXPECT_EQ(cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ(connect_calls, 1);

   printf("Line %d\n", __LINE__);
   mock_set_os_udp_recvfrom_buffer(prm_end_req, sizeof(prm_end_req));
   os_usleep(TEST_UDP_DELAY);

   EXPECT_EQ(state_calls, 2);
   EXPECT_EQ(cmdev_state, PNET_EVENT_PRMEND);
   EXPECT_EQ(connect_calls, 1);

   /* Simulate application calling APPL_RDY */
   printf("Line %d\n", __LINE__);
   ret = pnet_application_ready(g_pnet, main_arep);
   EXPECT_EQ(ret, 0);
   EXPECT_EQ(state_calls, 3);
   EXPECT_EQ(cmdev_state, PNET_EVENT_APPLRDY);

   printf("Line %d\n", __LINE__);
   mock_set_os_udp_recvfrom_buffer(appl_rdy_rsp, sizeof(appl_rdy_rsp));
   os_usleep(TEST_UDP_DELAY);
   EXPECT_EQ(state_calls, 3);
   EXPECT_EQ(cmdev_state, PNET_EVENT_APPLRDY);

   /* Perform the API calls: */

   /* Send a couple of data packets and verify reception */
   printf("Line %d\n", __LINE__);
   for (ix = 0; ix < 100; ix++)
   {
      send_data(g_pnet, data_packet_good_iops_good_iocs, sizeof(data_packet_good_iops_good_iocs));
   }

   printf("Line %d\n", __LINE__);
   iops = 88;     /* Something non-valid */
   in_len = sizeof(in_data);
   ret = pnet_output_get_data_and_iops(g_pnet, 0, 1, 1, &new_flag, in_data, &in_len, &iops);
   EXPECT_EQ(ret, 0);
   EXPECT_EQ(new_flag, true);
   EXPECT_EQ(in_len, 1);
   EXPECT_EQ(in_data[0], 0x23);
   EXPECT_EQ(iops, PNET_IOXS_GOOD);
   iocs = 77;     /* Something non-valid */
   ret = pnet_input_get_iocs(g_pnet, 0, 1, 1, &iocs);
   EXPECT_EQ(ret, 0);
   EXPECT_EQ(new_flag, true);
   EXPECT_EQ(in_len, 1);
   EXPECT_EQ(iocs, PNET_IOXS_GOOD);

   EXPECT_EQ(state_calls, 4);
   EXPECT_EQ(cmdev_state, PNET_EVENT_DATA);

   /* Send some data to the controller */
   printf("Line %d\n", __LINE__);
   ret = pnet_input_set_data_and_iops(g_pnet, 0, 1, 1, out_data, sizeof(out_data), PNET_IOXS_GOOD);
   EXPECT_EQ(ret, 0);

   /* Acknowledge the reception of controller data */
   ret = pnet_output_set_iocs(g_pnet, 0, 1, 1, PNET_IOXS_GOOD);
   EXPECT_EQ(ret, 0);
   EXPECT_EQ(state_calls, 4);
   EXPECT_EQ(cmdev_state, PNET_EVENT_DATA);

   /* Setup some record for the reader */

   printf("Line %d\n", __LINE__);
   /* Create a logbook entry */
   pnet_create_log_book_entry(g_pnet, main_arep, &pnio_status, 0x13245768);

   printf("Number of tests: %u\n", (unsigned)NELEMENTS(test_mod_diff));

   /* Now read all the mod diff record */
   for (ix = 0; ix < NELEMENTS(test_mod_diff); ix++)
   {
      test_read(&test_mod_diff[ix]);
   }
   EXPECT_EQ(read_fails, 0);

   printf("Line %d\n", __LINE__);
   mock_set_os_udp_recvfrom_buffer(release_req, sizeof(release_req));
   os_usleep(TEST_UDP_DELAY);

   printf("Line %d\n", __LINE__);
   EXPECT_EQ(release_calls, 1);
   EXPECT_EQ(state_calls, 5);
   EXPECT_EQ(cmdev_state, PNET_EVENT_ABORT);
   printf("Line %d\n", __LINE__);
}
