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
#include "pnal.h"
#include <pnet_api.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHANNEL_ERRORTYPE_SHORT_CIRCUIT                       0x0001
#define CHANNEL_ERRORTYPE_LINE_BREAK                          0x0006
#define CHANNEL_ERRORTYPE_DATA_TRANSMISSION_IMPOSSIBLE        0x8000
#define CHANNEL_ERRORTYPE_NETWORK_COMPONENT_FUNCTION_MISMATCH 0x8008
#define EXTENDED_CHANNEL_ERRORTYPE_FRAME_DROPPED              0x8000
#define EXTENDED_CHANNEL_ERRORTYPE_MAUTYPE_MISMATCH           0x8001
#define EXTENDED_CHANNEL_ERRORTYPE_LINE_DELAY_MISMATCH        0x8002

/********************** Settings **********************************************/

#define APP_DATA_LED_ID            1
#define APP_PROFINET_SIGNAL_LED_ID 2
#define APP_EVENT_READY_FOR_DATA   BIT (0)
#define APP_EVENT_TIMER            BIT (1)
#define APP_EVENT_ALARM            BIT (2)
#define APP_EVENT_ABORT            BIT (15)

#define APP_TICK_INTERVAL_US                1000 /* 1 ms */
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

#define APP_TICKS_READ_BUTTONS 10
#define APP_TICKS_UPDATE_DATA  100

/**************** From the GSDML file ****************************************/

#define APP_DEFAULT_STATION_NAME "rt-labs-dev"
#define APP_PARAM_IDX_1          123
#define APP_PARAM_IDX_2          124
#define APP_API                  0

#define APP_DIAG_CUSTOM_USI 0x1234

/* See "Specification for GSDML" 8.26 LogBookEntryItem for allowed values */
#define APP_LOGBOOK_ERROR_CODE   0x20 /* Manufacturer specific */
#define APP_LOGBOOK_ERROR_DECODE 0x82 /* Manufacturer specific */
#define APP_LOGBOOK_ERROR_CODE_1 PNET_ERROR_CODE_1_FSPM
#define APP_LOGBOOK_ERROR_CODE_2 0x00       /* Manufacturer specific */
#define APP_LOGBOOK_ENTRY_DETAIL 0xFEE1DEAD /* Manufacturer specific */

/*
 * I/O Modules. These modules and their sub-modules must be plugged by the
 * application after the call to pnet_init.
 *
 * Assume that all modules only have a single submodule, with same number.
 */
#define APP_MOD_8_0_IDENT       0x00000030 /* 8 bit input */
#define APP_MOD_0_8_IDENT       0x00000031 /* 8 bit output */
#define APP_MOD_8_8_IDENT       0x00000032 /* 8 bit input, 8 bit output */
#define APP_SUBMOD_CUSTOM_IDENT 0x00000001

#define APP_SUBSLOT_CUSTOM 0x00000001

#define APP_DATASIZE_INPUT     1 /* bytes, for digital inputs data */
#define APP_DATASIZE_OUTPUT    1 /* bytes, for digital outputs data */
#define APP_ALARM_PAYLOAD_SIZE 1 /* bytes */

/*** Example on how to keep lists of supported modules and submodules ********/

static const uint32_t cfg_available_module_types[] = {
   PNET_MOD_DAP_IDENT,
   APP_MOD_8_0_IDENT,
   APP_MOD_0_8_IDENT,
   APP_MOD_8_8_IDENT};

typedef struct cfg_submodule_type
{
   const char * name;
   uint32_t api;
   uint32_t module_ident_nbr;
   uint32_t submodule_ident_nbr;
   pnet_submodule_dir_t data_dir;
   uint16_t insize;  /* bytes */
   uint16_t outsize; /* bytes */
} cfg_submodule_type_t;

static const cfg_submodule_type_t cfg_available_submodule_types[] = {
   {"DAP Identity 1",
    APP_API,
    PNET_MOD_DAP_IDENT,
    PNET_SUBMOD_DAP_IDENT,
    PNET_DIR_NO_IO,
    0,
    0},
   {"DAP Interface 1",
    APP_API,
    PNET_MOD_DAP_IDENT,
    PNET_SUBMOD_DAP_INTERFACE_1_IDENT,
    PNET_DIR_NO_IO,
    0,
    0},
   {"DAP Port 1",
    APP_API,
    PNET_MOD_DAP_IDENT,
    PNET_SUBMOD_DAP_INTERFACE_1_PORT_1_IDENT,
    PNET_DIR_NO_IO,
    0,
    0},
#if PNET_MAX_PHYSICAL_PORTS >= 2
   {"DAP Port 2",
    APP_API,
    PNET_MOD_DAP_IDENT,
    PNET_SUBMOD_DAP_INTERFACE_1_PORT_2_IDENT,
    PNET_DIR_NO_IO,
    0,
    0},
#endif
#if PNET_MAX_PHYSICAL_PORTS >= 3
   {"DAP Port 3",
    APP_API,
    PNET_MOD_DAP_IDENT,
    PNET_SUBMOD_DAP_INTERFACE_1_PORT_3_IDENT,
    PNET_DIR_NO_IO,
    0,
    0},
#endif
#if PNET_MAX_PHYSICAL_PORTS >= 4
   {"DAP Port 4",
    APP_API,
    PNET_MOD_DAP_IDENT,
    PNET_SUBMOD_DAP_INTERFACE_1_PORT_4_IDENT,
    PNET_DIR_NO_IO,
    0,
    0},
#endif
   {"Input 8 bits",
    APP_API,
    APP_MOD_8_0_IDENT,
    APP_SUBMOD_CUSTOM_IDENT,
    PNET_DIR_INPUT,
    APP_DATASIZE_INPUT,
    0},
   {"Output 8 bits",
    APP_API,
    APP_MOD_0_8_IDENT,
    APP_SUBMOD_CUSTOM_IDENT,
    PNET_DIR_OUTPUT,
    0,
    APP_DATASIZE_OUTPUT},
   {"Input 8 bits output 8 bits",
    APP_API,
    APP_MOD_8_8_IDENT,
    APP_SUBMOD_CUSTOM_IDENT,
    PNET_DIR_IO,
    APP_DATASIZE_INPUT,
    APP_DATASIZE_OUTPUT},
};

/************************ App data storage ***********************************/

struct cmd_args
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
};

typedef struct app_netif_name
{
   char name[PNET_INTERFACE_NAME_MAX_SIZE];
} app_netif_name_t;

typedef struct app_netif_namelist
{
   app_netif_name_t netif[PNET_MAX_PHYSICAL_PORTS + 1];
} app_netif_namelist_t;

typedef struct app_subslot
{
   bool used;
   bool plugged;
   uint16_t slot_nbr;
   uint16_t subslot_nbr;
   uint32_t submodule_id;
   const char * submodule_name;
   pnet_data_cfg_t data_cfg;
   uint8_t * p_in_data;
} app_subslot_t;

typedef struct app_slot
{
   bool plugged;
   uint32_t module_id;
   app_subslot_t subslots[PNET_MAX_SUBSLOTS];
} app_slot_t;

typedef struct app_api_t
{
   uint32_t api_id;
   uint32_t arep;
   app_slot_t slots[PNET_MAX_SLOTS];
} app_api_t;

typedef struct app_data_obj
{
   os_timer_t * main_timer;
   os_event_t * main_events;
   app_api_t main_api;
   bool alarm_allowed;
   pnet_alarm_argument_t alarm_arg;
   struct cmd_args arguments;
   app_netif_namelist_t if_list;
   uint32_t app_param_1;
   uint32_t app_param_2;
   uint8_t inputdata[APP_DATASIZE_INPUT];
   uint32_t arep_for_appl_ready;
} app_data_t;

typedef struct app_data_and_stack_obj
{
   app_data_t * appdata;
   pnet_t * net;
} app_data_and_stack_t;

typedef enum app_demo_state
{
   APP_DEMO_STATE_ALARM_SEND,
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

/********************* Helper function declarations ***************************/

/**
 * Convert MAC address to string
 * @param mac              In:    MAC address
 * @param outputstring     Out:   Resulting string buffer. Should have size
 *                                PNAL_ETH_ADDRSTR_SIZE.
 */
void app_mac_to_string (pnet_ethaddr_t mac, char * outputstring);

/**
 * Print out current IP address, netmask and default gateway.
 *
 * @param ip               In:    IP address
 * @param netmask          In:    Netmask
 * @param gateway          In:    Gateway
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
void app_print_network_details (
   pnal_ipaddr_t ip,
   pnal_ipaddr_t netmask,
   pnal_ipaddr_t gateway);

/**
 * Adjust some members of the p-net configuration object.
 *
 * @param stack_config     Out:   Configuration for use by p-net
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int app_pnet_cfg_init_default (pnet_cfg_t * stack_config);

/**
 * Initialize interface configuration from argument string.
 *
 * @param netif_list_str   In:    Comma separated list of network interfaces.
 *                                Terminated string.
 * @param if_list          Out:   Network interface string storage.
 * @param p_cfg            InOut: p-net configuration
 * @param verbosity        In:    Verbosity
 * @return  0  on success
 *         -1  on error
 */
int app_pnet_cfg_init_netifs (
   const char * netif_list_str,
   app_netif_namelist_t * if_list,
   uint16_t * p_number_of_ports,
   pnet_cfg_t * p_cfg,
   int verbosity);

/**
 * Parse a comma separated list of network interfaces and check
 * that the number of interfaces match the PNET_MAX_PHYSICAL_PORTS
 * configuration.
 *
 * For a single Ethernet interface, the \a arg_str should consist of
 * one name. For two Ethernet interfaces, the  \a arg_str should consist of
 * three names, as we also need a bridge interface.
 *
 * Does only consider the number of commaseparated names. No check of the
 * names themselves are done.
 *
 * Examples:
 * arg_str                 num_ports
 * "eth0"                  1
 * "eth0,eth1"             error (We need a bridge as well)
 * "br0,eth0,eth1"         2
 *
 * @param arg_str      In:   Network interface list as comma separated,
 *                           terminated string. For example "eth0" or
 *                           "br0,eth0,eth1".
 * @param max_port     In:   PNET_MAX_PHYSICAL_PORTS, passed as argument to
 *                           allow test.
 * @param p_if_list    Out:  List of network interfaces
 * @param p_num_ports  Out:  Resulting number of physical ports
 * @return  0  on success
 *         -1  on error
 */
int app_get_netif_namelist (
   const char * arg_str,
   uint16_t max_port,
   app_netif_namelist_t * p_if_list,
   uint16_t * p_num_ports);

/**
 * Plug DAP (sub-)modules. This operation shall be called after p-net
 * stack initialization
 *
 * @param net              InOut: p-net stack instance
 * @param arg              In:    user data for callbacks
 */
void app_plug_dap (pnet_t * net, void * arg);

/**
 * Copy an IP address (as an integer) to a struct
 *
 * @param destination_struct  Out:   Destination
 * @param ip                  In:    IP address
 */
void app_copy_ip_to_struct (
   pnet_cfg_ip_addr_t * destination_struct,
   pnal_ipaddr_t ip);

/**
 * Sample app main loop.
 *
 * @param net              InOut: p-net stack instance
 * @param p_appdata        In:    Appdata
 */
void app_loop_forever (pnet_t * net, app_data_t * p_appdata);

/********************** Hardware interaction **********************************/

/**
 * Control a LED
 *
 * This is hardware dependent, so the implementation of this function should be
 * located in the corresponding main file.
 *
 * @param id               In:    LED number, starting from 0.
 * @param led_state        In:    LED state. Use true for on and false for off.
 * @param verbosity        In:    Print new led_state, if verbosity > 0
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int app_set_led (uint16_t id, bool led_state, int verbosity);

/**
 * Read a button.
 *
 * This is hardware dependent, so the implementation of this function should be
 * located in the corresponding main file.
 *
 * @param p_appdata        In:    App data
 * @param id               In:    Button number, starting from 0.
 * @param p_pressed        Out:   Button state. Use true for pressed.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
void app_get_button (
   const app_data_t * p_appdata,
   uint16_t id,
   bool * p_pressed);

#ifdef __cplusplus
}
#endif

#endif /* SAMPLEAPP_COMMON_H */
