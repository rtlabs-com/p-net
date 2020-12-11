/*
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

p-net Profinet device stack
===========================

Copyright 2018 rt-labs AB

This instance of p-net is licensed for use in a specific product, as
follows:

Company: Bender GmbH & Co. KG
Product: COMTRAXX
Product description: COMTRAXX products provided by Bender GmbH & Co. KG

Use of this instance of p-net in other products is not allowed.

You are hereby given a non-exclusive, non-transferable license to use
the SOFTWARE and documentation according to the SOFTWARE LICENSE
AGREEMENT document shipped together with this instance of the
SOFTWARE. The SOFTWARE LICENSE AGREEMENT prohibits copying and
redistribution of the SOFTWARE.  It prohibits reverse-engineering
activities.  It limits warranty and liability on behalf of rt-labs.
The details of these terms are explained in subsequent chapters of the
SOFTWARE LICENSE AGREEMENT.
-----BEGIN PGP SIGNATURE-----

iQEzBAEBCgAdFiEE1e4U+gPgGJXLsxxLZlG0bopmiXIFAl/TMvQACgkQZlG0bopm
iXJE1QgAlEKblKkIrLF+hVRm6I1IfbGV6VWnBx+9IPwaHZmgQZAVTOlp4RHiNAre
Il1l1xheENWpuihbVlvz5hMCVto4fkT13IUXxOo9Va9MK8lm6JOu2lYpl1HgVIwV
R6I6dsWLeNBSBaA38DDBHIfjP4LBZC/HVYO2ISc3/OKutqasz/WIp+AjAURiKRsf
yaWGay88FITHymmqJpGaWzB+jAOvRFs8hW/GsOyX/r17AK5F5N51O+UqUTKAWNPT
f6M8QF0b07Hw+eOuB7A/0D5nvSl/rOd9LvYkiUhIUmQdHqERNkF9EJdroIjxTMXi
lmoOkPEqyGV58fh7JjMYlI60sPlGBw==
=TaN+
-----END PGP SIGNATURE-----
*/

#ifndef UTILS_FOR_TESTING_H
#define UTILS_FOR_TESTING_H

#include <gtest/gtest.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "pf_includes.h"
#include "mocks.h"

#if defined(TEST_DEBUG)
#define TEST_TRACE(...) printf (__VA_ARGS__)
#else
#define TEST_TRACE(...)
#endif

#define TEST_UDP_DELAY     (500 * 1000)      /* us */
#define TEST_DATA_DELAY    (2 * 1000)        /* us */
#define TEST_TIMEOUT_DELAY (3 * 1000 * 1000) /* us */
#define TICK_INTERVAL_US   1000              /* us */

#define TEST_API_IDENT                            0
#define TEST_API_NONEXIST_IDENT                   2
#define TEST_SLOT_IDENT                           1
#define TEST_SLOT_NONEXIST_IDENT                  2
#define TEST_SUBSLOT_IDENT                        1
#define TEST_SUBSLOT_NONEXIST_IDENT               2
#define TEST_CHANNEL_IDENT                        0
#define TEST_CHANNEL_NONEXIST_IDENT               1
#define TEST_CHANNEL_ILLEGAL                      0x8001
#define TEST_CHANNEL_DIRECTION                    PNET_DIAG_CH_PROP_DIR_OUTPUT
#define TEST_CHANNEL_NUMBER_OF_BITS               PNET_DIAG_CH_PROP_TYPE_8_BIT
#define TEST_CHANNEL_ERRORTYPE                    0x0001
#define TEST_CHANNEL_ERRORTYPE_B                  0x0002
#define TEST_CHANNEL_ERRORTYPE_C                  0x0003
#define TEST_CHANNEL_ERRORTYPE_D                  0x0004
#define TEST_CHANNEL_ERRORTYPE_NONEXIST           0x0007
#define TEST_DIAG_USI_CUSTOM                      0x1234
#define TEST_DIAG_USI_NONEXIST                    0x1235
#define TEST_DIAG_USI_INVALID                     0x8888
#define TEST_DIAG_EXT_ERRTYPE                     0x0002
#define TEST_DIAG_EXT_ERRTYPE_NONEXIST            0x0009
#define TEST_DIAG_EXT_ADDVALUE                    0x00030004
#define TEST_DIAG_EXT_ADDVALUE_B                  0x00030005
#define TEST_DIAG_QUALIFIER                       0x00010000
#define TEST_DIAG_QUALIFIER_NOTSET                0x00000000
#define TEST_INTERFACE_NAME                       "en1"
#define TEST_MAX_NUMBER_AVAILABLE_MODULE_TYPES    20
#define TEST_MAX_NUMBER_AVAILABLE_SUBMODULE_TYPES 20

/*
 * I/O Modules. These modules and their sub-modules must be plugged by the
 * application after the call to pnet_init.
 *
 * Assume that all modules only have a single submodule, with same number.
 */
#define TEST_MOD_8_0_IDENT       0x00000030 /* 8 bit input */
#define TEST_MOD_0_8_IDENT       0x00000031 /* 8 bit output */
#define TEST_MOD_8_8_IDENT       0x00000032 /* 8 bit input, 8 bit output */
#define TEST_SUBMOD_CUSTOM_IDENT 0x00000001
#define TEST_DATASIZE_INPUT      1 /* bytes, for digital inputs data */
#define TEST_DATASIZE_OUTPUT     1 /* bytes, for digital outputs data */

typedef struct cfg_submodules
{
   uint32_t module_ident_number;
   uint32_t submodule_ident_number;
   pnet_submodule_dir_t direction;
   uint16_t input_length;
   uint16_t output_length;
} cfg_submodules_t;

typedef struct call_counters_obj
{
   uint16_t state_calls;
   uint16_t connect_calls;
   uint16_t release_calls;
   uint16_t dcontrol_calls;
   uint16_t ccontrol_calls;
   uint16_t read_calls;
   uint16_t write_calls;
   uint16_t led_on_calls;
   uint16_t led_off_calls;
} call_counters_t;

typedef struct app_data_for_testing_obj
{
   uint32_t tick_ctr;
   os_timer_t * periodic_timer;
   pnet_event_values_t cmdev_state;
   uint16_t data_cycle_ctr;
   uint32_t data_ctr;
   os_event_t * main_events;
   uint32_t main_arep;
   bool alarm_allowed;
   uint32_t app_param_1;
   uint32_t app_param_2;
   uint8_t inputdata[TEST_DATASIZE_INPUT];
   uint8_t custom_input_slots[PNET_MAX_SLOTS];
   uint8_t custom_output_slots[PNET_MAX_SLOTS];
   uint32_t available_module_types[TEST_MAX_NUMBER_AVAILABLE_MODULE_TYPES];
   cfg_submodules_t
      available_submodule_types[TEST_MAX_NUMBER_AVAILABLE_SUBMODULE_TYPES];
   bool init_done;
   uint16_t read_fails;
   call_counters_t call_counters;
} app_data_for_testing_t;

/******************** Callbacks defined by p-net *****************************/

int my_connect_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result);

int my_release_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result);

int my_dcontrol_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_control_command_t control_command,
   pnet_result_t * p_result);

int my_ccontrol_cnf (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result);

int my_read_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint16_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint8_t ** pp_read_data,   /* Out: A pointer to the data */
   uint16_t * p_read_length,  /* Out: Size of data */
   pnet_result_t * p_result); /* Error status if returning != 0 */

int my_write_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint16_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint16_t write_length,
   uint8_t * p_write_data,
   pnet_result_t * p_result);

int my_new_data_status_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t crep,
   uint8_t changes,
   uint8_t data_status);

int my_alarm_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   const pnet_alarm_argument_t * p_alarm_arg,
   uint16_t data_len,
   uint16_t data_usi,
   uint8_t * p_data);

int my_alarm_cnf (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_pnio_status_t * p_pnio_status);

int my_state_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_event_values_t state);

int my_exp_module_ind (
   pnet_t * net,
   void * arg,
   uint16_t api,
   uint16_t slot,
   uint32_t module_ident);

int my_exp_submodule_ind (
   pnet_t * net,
   void * arg,
   uint16_t api,
   uint16_t slot,
   uint16_t subslot,
   uint32_t module_ident,
   uint32_t submodule_ident);

#ifdef __cplusplus
}
#endif

/*********************** Base classes for tests *****************************/

class PnetUnitTest : public ::testing::Test
{
 protected:
   virtual void SetUp(){};
};

class PnetIntegrationTestBase : public ::testing::Test
{
 protected:
   pnet_cfg_t pnet_default_cfg;
   app_data_for_testing_t appdata;
   pnet_t the_net;
   pnet_t * net = &the_net;

   /** Initialize appdata, including clearing available modules etc. */
   virtual void appdata_init();

   virtual void callcounter_reset();

   virtual void available_modules_and_submodules_init();

   virtual void cfg_init();

   /** Simulate sleep
    *
    * This function updates the app state and calls the periodic
    * maintenance functions while simulating sleeping.
    *
    * @param us         In: time to sleep (in us)
    */
   virtual void run_stack (int us);

   /** Send raw Ethernet test data
    *
    * @param data_packet    In: Data packet
    * @param len            In: Length of data packet
    */
   virtual void send_data (uint8_t * data_packet, uint16_t len);
};

class PnetIntegrationTest : public PnetIntegrationTestBase
{
 protected:
   virtual void SetUp() override
   {
      mock_init();
      cfg_init();
      appdata_init();
      available_modules_and_submodules_init();

      callcounter_reset();

      pnet_init_only (
         net,
         TEST_INTERFACE_NAME,
         TICK_INTERVAL_US,
         &pnet_default_cfg);

      mock_clear(); /* lldp sends a frame at init */
   };
};

/*************************** Assertion helpers ******************************/

template <typename T, size_t size>
::testing::AssertionResult ArraysMatch (
   const T (&expected)[size],
   const T (&actual)[size])
{
   for (size_t i (0); i < size; ++i)
   {
      if (expected[i] != actual[i])
      {
         return ::testing::AssertionFailure()
                << std::hex << std::showbase << "array[" << i << "] ("
                << static_cast<int> (actual[i]) << ") != expected[" << i
                << "] (" << static_cast<int> (expected[i]) << ")";
      }
   }

   return ::testing::AssertionSuccess();
}

#endif /* UTILS_FOR_TESTING_H */
