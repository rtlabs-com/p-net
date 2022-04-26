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

#ifndef SAMPLEAPP_COMMON_H
#define SAMPLEAPP_COMMON_H

#include "sampleapp_gsdml.h"
#include <pnet_api.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_TICK_INTERVAL_US 1000 /* 1 ms */

/* Thread configuration for targets where sample
 * event loop is run in a separate thread (not main).
 * This applies for linux sample app implementation.
 */
#define APP_MAIN_THREAD_PRIORITY  15
#define APP_MAIN_THREAD_STACKSIZE 4096 /* bytes */

#define APP_DATA_LED_ID            1
#define APP_PROFINET_SIGNAL_LED_ID 2

#define APP_TICKS_READ_BUTTONS 10
#define APP_TICKS_UPDATE_DATA  100

#define APP_ALARM_PAYLOAD_SIZE 1 /* bytes */

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

#define APP_DATA_DEFAULT_OUTPUT_DATA 0

/** Command line arguments for sample application */
typedef struct app_args
{
   char path_button1[PNET_MAX_FILE_FULLPATH_SIZE]; /** Terminated string */
   char path_button2[PNET_MAX_FILE_FULLPATH_SIZE]; /** Terminated string */
   char path_storage_directory[PNET_MAX_DIRECTORYPATH_SIZE]; /** Terminated */
   char station_name[PNET_STATION_NAME_MAX_SIZE]; /** Terminated string */
   char eth_interfaces
      [PNET_INTERFACE_NAME_MAX_SIZE * (PNET_MAX_PHYSICAL_PORTS + 1) +
       PNET_MAX_PHYSICAL_PORTS]; /** Terminated string */
   int verbosity;
   int show;
   bool factory_reset;
   bool remove_files;
} app_args_t;

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

typedef struct app_userdata
{
   bool button1_pressed;
   bool button2_pressed;
   bool button2_pressed_previous;

   /** Counter to control when buttons are read */
   uint32_t buttons_tick_counter;

   app_demo_state_t alarm_demo_state;
   uint8_t alarm_payload[APP_ALARM_PAYLOAD_SIZE];

   /** Parameter data for echo submodules
    * The stored value is shared between all echo submodules in this example.
    *
    * Todo: Data is always in pnio data format. Add conversion to uint32_t.
    */
   uint32_t app_param_echo_gain; /* Network endianness */

   /* Parameter data for digital submodules
    * The stored value is shared between all digital submodules in this example.
    *
    * Todo: Data is always in pnio data format. Add conversion to uint32_t.
    */
   uint32_t app_param_1; /* Network endianness */
   uint32_t app_param_2; /* Network endianness */

   /* Network endianness */
   uint8_t echo_inputdata[APP_GSDML_INPUT_DATA_ECHO_SIZE];
   uint8_t echo_outputdata[APP_GSDML_OUTPUT_DATA_ECHO_SIZE];

   /* Digital submodule process data
    * The stored value is shared between all digital submodules in this example.
    */
   uint8_t count;
   uint8_t inputdata[APP_GSDML_INPUT_DATA_DIGITAL_SIZE];
   uint8_t outputdata[APP_GSDML_OUTPUT_DATA_DIGITAL_SIZE];

} app_userdata_t;

/**
 * Set LED state
 * Hardware specific. Implemented in sample app main file for
 * each supported platform.
 *
 * @param id               In:    LED number, starting from 0.
 * @param led_state        In:    LED state. Use true for on and false for off.
 */
void app_set_led (uint16_t id, bool led_state);

/**
 * Read button state
 *
 * Hardware specific. Implemented in sample app main file for
 * each supported platform.
 *
 * @param id               In:    Button number, starting from 0.
 * @return  true if button is pressed, false if not
 */
bool app_get_button (uint16_t id);

#ifdef __cplusplus
}
#endif

#endif /* SAMPLEAPP_COMMON_H */
