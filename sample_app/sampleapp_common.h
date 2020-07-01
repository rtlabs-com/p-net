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

#include "osal.h"
#include <pnet_api.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define IP_INVALID              0

/********************** Settings **********************************************/

#define APP_PROFINET_SIGNAL_LED_ID     0
#define APP_DATA_LED_ID                1
#define EVENT_READY_FOR_DATA           BIT(0)
#define EVENT_TIMER                    BIT(1)
#define EVENT_ALARM                    BIT(2)
#define EVENT_ABORT                    BIT(15)

#define TICK_INTERVAL_US               1000        /* 1 ms */
#define APP_ALARM_USI                  1

/**************** From the GSDML file ****************************************/

#define APP_DEFAULT_STATION_NAME "rt-labs-dev"
#define APP_PARAM_IDX_1          123
#define APP_PARAM_IDX_2          124
#define APP_API                  0

/*
 * Module and submodule ident number for the DAP module.
 * The DAP module and submodules must be plugged by the application after the call to pnet_init.
 */
#define PNET_SLOT_DAP_IDENT                        0x00000000
#define PNET_MOD_DAP_IDENT                         0x00000001     /* For use in slot 0 */
#define PNET_SUBMOD_DAP_IDENT                      0x00000001     /* For use in subslot 1 */
#define PNET_SUBMOD_DAP_INTERFACE_1_IDENT          0x00008000     /* For use in subslot 0x8000 */
#define PNET_SUBMOD_DAP_INTERFACE_1_PORT_0_IDENT   0x00008001     /* For use in subslot 0x8001 */

/*
 * I/O Modules. These modules and their sub-modules must be plugged by the
 * application after the call to pnet_init.
 *
 * Assume that all modules only have a single submodule, with same number.
 */
#define PNET_MOD_8_0_IDENT          0x00000030     /* 8 bit input */
#define PNET_MOD_0_8_IDENT          0x00000031     /* 8 bit output */
#define PNET_MOD_8_8_IDENT          0x00000032     /* 8 bit input, 8 bit output */
#define PNET_SUBMOD_CUSTOM_IDENT    0x00000001

#define APP_DATASIZE_INPUT       1     /* bytes, for digital inputs data */
#define APP_DATASIZE_OUTPUT      1     /* bytes, for digital outputs data */
#define APP_ALARM_PAYLOAD_SIZE   1     /* bytes */


/*** Example on how to keep lists of supported modules and submodules ********/

static const uint32_t            cfg_available_module_types[] =
{
   PNET_MOD_DAP_IDENT,
   PNET_MOD_8_0_IDENT,
   PNET_MOD_0_8_IDENT,
   PNET_MOD_8_8_IDENT
};

static const struct
{
   uint32_t                api;
   uint32_t                module_ident_nbr;
   uint32_t                submodule_ident_nbr;
   pnet_submodule_dir_t    data_dir;
   uint16_t                insize;     /* bytes */
   uint16_t                outsize;    /* bytes */
} cfg_available_submodule_types[] =
{
   {APP_API, PNET_MOD_DAP_IDENT, PNET_SUBMOD_DAP_IDENT,                    PNET_DIR_NO_IO,  0,                  0},
   {APP_API, PNET_MOD_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_IDENT,        PNET_DIR_NO_IO,  0,                  0},
   {APP_API, PNET_MOD_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_PORT_0_IDENT, PNET_DIR_NO_IO,  0,                  0},
   {APP_API, PNET_MOD_8_0_IDENT, PNET_SUBMOD_CUSTOM_IDENT,                 PNET_DIR_INPUT,  APP_DATASIZE_INPUT, 0},
   {APP_API, PNET_MOD_0_8_IDENT, PNET_SUBMOD_CUSTOM_IDENT,                 PNET_DIR_OUTPUT, 0,                  APP_DATASIZE_OUTPUT},
   {APP_API, PNET_MOD_8_8_IDENT, PNET_SUBMOD_CUSTOM_IDENT,                 PNET_DIR_IO,     APP_DATASIZE_INPUT, APP_DATASIZE_OUTPUT},
};

/************************ App data storage ***********************************/

struct cmd_args {
   char path_button1[256];
   char path_button2[256];
   char station_name[64];
   char eth_interface[64];
   int  verbosity;
};

typedef struct app_data_obj
{
   os_timer_t                *main_timer;
   os_event_t                *main_events;
   uint32_t                  main_arep;
   bool                      alarm_allowed;
   struct cmd_args           arguments;
   uint32_t                  app_param_1;
   uint32_t                  app_param_2;
   uint8_t                   inputdata[APP_DATASIZE_INPUT];
   uint8_t                   custom_input_slots[PNET_MAX_MODULES];
   uint8_t                   custom_output_slots[PNET_MAX_MODULES];
   bool                      init_done;
} app_data_t;


typedef struct app_data_and_stack_obj
{
   app_data_t           *appdata;
   pnet_t               *net;
} app_data_and_stack_t;


/********************* Helper function declarations ***************************/

/**
 * Adjust some members of the p-net configuration object.
 *
 * @param stack_config     Out: Configuration for use by p-net
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
*/
int app_adjust_stack_configuration(
   pnet_cfg_t              *stack_config);

/**
 * Plug DAP (sub-)modules. This operation shall be called after p-net
 * stack initialization
 *
 * @param net     In: p-net stack instance
 * @param arg     In: user data for callbacks
 * @return  None
*/
void app_plug_dap(pnet_t *net, void *arg);

/**
 * Return a string representation of the given event.
 * @param event            In:   The event.
 * @return  A string representing the event.
 */
const char* event_value_to_string(
   pnet_event_values_t event);

/**
 * Return a string representation of the submodule data direction.
 * @param direction        In:   The direction.
 * @return  A string representing the direction.
 */
const char* submodule_direction_to_string(
   pnet_submodule_dir_t direction);


/********************** Hardware interaction **********************************/

/**
 * Control a LED
 *
 * This is hardware dependent, so the implementation of this function should be
 * located in the corresponding main file.
 *
 * @param id               In: LED number, starting from 0
 * @param led_state        In: LED state. Use true for on and false for off.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
*/
int app_set_led(
   uint16_t                id,         /* Starting from 0 */
   bool                    led_state);


#ifdef __cplusplus
}
#endif

#endif /* SAMPLEAPP_COMMON_H */
