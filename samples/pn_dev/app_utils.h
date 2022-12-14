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

#ifndef APP_UTILS_H
#define APP_UTILS_H

/**
 * @file
 * @brief Application utilities and helper functions
 *
 * Functions for getting string representation of
 * P-Net events, error codes and more.
 *
 * API, slot and subslot administration.
 *
 * Initialization of P-Net configuration from app_gsdml.h.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "osal.h"
#include "pnal.h"
#include <pnet_api.h>

typedef struct app_utils_netif_name
{
   char name[PNET_INTERFACE_NAME_MAX_SIZE];
} app_utils_netif_name_t;

typedef struct app_utils_netif_namelist
{
   app_utils_netif_name_t netif[PNET_MAX_PHYSICAL_PORTS + 1];
} app_utils_netif_namelist_t;

/* Forward declaration */
typedef struct app_subslot app_subslot_t;

/**
 * Callback for updated cyclic data
 *
 * @param subslot    InOut: Subslot structure
 * @param tag        InOut: Typically a handle to a submodule
 */
typedef void (*app_utils_cyclic_callback) (app_subslot_t * subslot, void * tag);

/**
 * Information of submodule plugged into a subslot.
 *
 * Note that submodule data is not stored here but must
 * be handled by the submodule implementation.
 *
 * All parameters are initialized by the app_utils_plug_submodule()
 * function.
 *
 * The cyclic_callback is used when app_utils_cyclic_data_poll()
 * is called. Typically on the tick event in the main task.
 * The \a tag parameter is passed with the cyclic_callback and
 * is typically a handle to a submodule on application.
 */
typedef struct app_subslot
{
   /** True when the position in the subslot array is occupied */
   bool used;

   /** True when the subslot is plugged */
   bool plugged;

   uint16_t slot_nbr;
   uint16_t subslot_nbr;
   uint32_t submodule_id;
   const char * submodule_name;
   pnet_data_cfg_t data_cfg;

   /** Status indicator from PLC */
   uint8_t indata_iocs;

   /** Status indicator from PLC */
   uint8_t outdata_iops;

   /** Callback for cyclic input- or output data, or NULL if not implemented */
   app_utils_cyclic_callback cyclic_callback;
   void * tag;
} app_subslot_t;

/**
 * Information of module plugged into a slot,
 * and array of subslots for admin of submodules.
 */
typedef struct app_slot
{
   bool plugged;
   uint32_t module_id;
   const char * name; /** Module name */

   /** Subslots. Use a separate index, as the subslot number might be large.
    *  For example the subslot for DAP port 1 has number 0x8001 */
   app_subslot_t subslots[PNET_MAX_SUBSLOTS];
} app_slot_t;

/**
 * Information relating to an application relation.
 */
typedef struct app_ar
{
   uint32_t arep;
   uint32_t events;
} app_ar_t;

/**
 * AR list iterator state.
 */
typedef struct app_ar_iterator
{
   app_ar_t *ar;
   int16_t index;
   bool modified;
} app_ar_iterator_t;

/**
 * Profinet API state for application
 *
 * Used to manage plugged modules into slots (and submodules into subslots).
 */
typedef struct app_api_t
{
   uint32_t api_id;

   /**
    * Active AR:s.
    * A list which is terminated by an entry with arep == UINT32_MAX.
    */
   app_ar_t ar[PNET_MAX_AR];

   /** Slots. Use slot number as index */
   app_slot_t slots[PNET_MAX_SLOTS];
} app_api_t;

/**
 * Add an arep to the AR list of an API.
 * @param api              InOut: The \a app_api_t instance.
 * @param arep             In:    The arep to add.
 * @param ar               Out:   The AR entry, if successful.
 *
 * @return 0 if the arep could not be added.
 *         1 if the arep was added.
 */
int app_ar_add_arep (app_api_t * api, uint32_t arep, app_ar_t ** ar);

/**
 * Get the arep of an AR.
 * @param ar               In: The AR.
 * @return the arep of the AR.
 */
uint32_t app_ar_arep (app_ar_t * ar);

/**
 * Clear an event from the events value of the AR.
 * @param ar               InOut: The AR to modify.
 * @param event            In:    The event(s) (bitmask) that should be cleared.
 * @return 0 if none of the set bits in \a event were set for the AR.
 *         1 if at least one of the set bits in \a event were set for the AR.
 */
int app_ar_event_clr (app_ar_t * ar, uint32_t event);

/**
 * Set an event from the events value of the AR.
 * @param ar               InOut: The AR to modify.
 * @param event            In:    The event(s) (bitmask) that should be set.
 */
void app_ar_event_set (app_ar_t * ar, uint32_t event);

/**
 * Initialize a list iterator for the AR list of an API.
 * @param iterator         InOut: The iterator to be initialized.
 * @param api              In:    The \a app_api_t instance.
 */
void app_ar_iterator_init (
   app_ar_iterator_t * iterator,
   app_api_t * api);

/**
 * Get the next AR from a list iterator.
 * @param iterator         InOut: The iterator to use.
 * @param ar               Out:   If the return value is 1, the next AR from
 *                                the list.
 * @return 0 if there were no more items in the list.
 *         1 if there was another item in the list.
 */
int app_ar_iterator_next (app_ar_iterator_t * iterator, app_ar_t ** ar);

/**
 * Check whether the iterator has run to the end of entries.
 * @param iterator         InOut: The iterator.
 * @return 0 if the iterator is finished.
 *         1 if the iterator is finished.
 */
int app_ar_iterator_done (app_ar_iterator_t * iterator);

/**
 * Delete the current AR entry of the list iterator.
 * @param ar               InOut: The AR.
 */
void app_ar_iterator_delete_current (app_ar_iterator_t * iterator);

/**
 * Convert IP address to string
 * @param ip               In:    IP address
 * @param outputstring     Out:   Resulting string buffer. Should have size
 *                                PNAL_INET_ADDRSTR_SIZE.
 */
void app_utils_ip_to_string (pnal_ipaddr_t ip, char * outputstring);

/**
 * Get string description of data direction
 * @param direction               In:    Submodule data direction
 * @return String representation of data direction
 */
const char * app_utils_submod_dir_to_string (pnet_submodule_dir_t direction);

/**
 * Get string description of PNIO producer or consumer status
 * @param ioxs               In:    Producer or consumer status (IOPS/IOCS)
 * @return String representation of ioxs (IOPS/IOCS)
 */
const char * app_utils_ioxs_to_string (pnet_ioxs_values_t ioxs);

/**
 * Convert MAC address to string
 * @param mac              In:    MAC address
 * @param outputstring     Out:   Resulting string buffer. Should have size
 *                                PNAL_ETH_ADDRSTR_SIZE.
 */
void app_utils_mac_to_string (pnet_ethaddr_t mac, char * outputstring);

/**
 * Convert error code to string format
 * Only common error codes supported.
 * Todo: Add rest of error codes.
 *
 * @param err_cls        In:   The error class. See PNET_ERROR_CODE_1_*
 * @param err_code       In:   The error code. See PNET_ERROR_CODE_2_*
 * @param err_cls_str    Out:   The error class string
 * @param err_code_str   Out:   The error code string
 */
void app_utils_get_error_code_strings (
   uint16_t err_cls,
   uint16_t err_code,
   const char ** err_cls_str,
   const char ** err_code_str);

/**
 * Copy an IP address (as an integer) to a struct
 * @param destination_struct  Out:   Destination
 * @param ip                  In:    IP address
 */
void app_utils_copy_ip_to_struct (
   pnet_cfg_ip_addr_t * destination_struct,
   pnal_ipaddr_t ip);

/**
 * Return a string representation of
 * the given dcontrol command.
 * @param event            In:    control_command
 * @return  A string representing the command
 */
const char * app_utils_dcontrol_cmd_to_string (
   pnet_control_command_t control_command);

/**
 * Return a string representation of the given event.
 * @param event            In:    event
 * @return  A string representing the event
 */
const char * app_utils_event_to_string (pnet_event_values_t event);

/**
 * Update network configuration from a string
 * defining a list of network interfaces examples:
 * "eth0" or "br0,eth0,eth1"
 *
 * Read IP, netmask etc from operating system.
 *
 * @param netif_list_str      In:  Comma separated string of network ifs
 * @param if_list             Out: Array of network ifs
 * @param number_of_ports     Out: Number of ports
 * @param if_cfg              Out: P-Net network configuration to be updated
 * @return 0 on success, -1 on error
 */
int app_utils_pnet_cfg_init_netifs (
   const char * netif_list_str,
   app_utils_netif_namelist_t * if_list,
   uint16_t * number_of_ports,
   pnet_if_cfg_t * if_cfg);

/**
 * Parse a comma separated list of network interfaces and check
 * that the number of interfaces match the PNET_MAX_PHYSICAL_PORTS
 * configuration.
 *
 * For a single Ethernet interface, the \a arg_str should consist of
 * one name. For two Ethernet interfaces, the  \a arg_str should consist of
 * three names, as we also need a bridge interface.
 *
 * Does only consider the number of comma separated names. No check of the
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
int app_utils_get_netif_namelist (
   const char * arg_str,
   uint16_t max_port,
   app_utils_netif_namelist_t * p_if_list,
   uint16_t * p_num_ports);

/**
 * Print network configuration using APP_LOG_INFO().
 *
 * @param if_cfg           In:   Network configuration
 * @param number_of_ports  In:   Number of used ports
 */
void app_utils_print_network_config (
   pnet_if_cfg_t * if_cfg,
   uint16_t number_of_ports);

/**
 * Print message if IOXS has changed.
 *
 * Uses APP_LOG_INFO()
 *
 * @param subslot          In: Subslot
 * @param ioxs_str         In: String description Producer or Consumer
 * @param ioxs_current     In: Current status
 * @param ioxs_new         In: New status
 */
void app_utils_print_ioxs_change (
   const app_subslot_t * subslot,
   const char * ioxs_str,
   uint8_t ioxs_current,
   uint8_t ioxs_new);

/**
 * Init the p-net configuration to default values.
 *
 * Most values are picked from app_gsdml.h
 *
 * Network configuration not initialized.
 * This means that \a '.if_cfg' must be set by application.
 *
 * Use this function to init P-Net configuration before
 * before passing config to app_init().
 *
 * @param pnet_cfg     Out:   Configuration for use by p-net
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int app_utils_pnet_cfg_init_default (pnet_cfg_t * pnet_cfg);

/**
 * Plug application module
 *
 * This is for the application to remember which slots are
 * populated in the p-net stack.
 *
 * @param p_api            InOut: API
 * @param slot_nbr         In:    Slot number
 * @param id               In:    Module identity
 * @param name             In:    Module name
 * @return 0 on success, -1 on error
 */
int app_utils_plug_module (
   app_api_t * p_api,
   uint16_t slot_nbr,
   uint32_t id,
   const char * name);

/**
 * Pull any application module in given slot.
 *
 * This is for the application to remember which slots are
 * populated in the p-net stack.
 *
 * @param p_api            InOut: API
 * @param slot_nbr         In:    Slot number
 * @return 0 on success, -1 on error
 */
int app_utils_pull_module (app_api_t * p_api, uint16_t slot_nbr);

/**
 * Plug application submodule.
 *
 * This is for the application to remember which subslots are
 * populated in the p-net stack.
 *
 * @param p_api            InOut: API
 * @param slot_nbr         In:    Slot number
 * @param subslot_nbr      In:    Subslot number
 * @param submodule_id     In:    Submodule identity
 * @param p_data_cfg       In:    Data configuration,
 *                                direction, in and out sizes
 * @param submodule_name   In:    Submodule name
 * @param cyclic_callback  In:    Submodule data callback
 * @param tag              In:    Tag passed in cyclic callback
 *                                Typically application or
 *                                submodule handle
 * @return Reference to allocated subslot,
 *         NULL if no free subslot is available. This should
 *         never happen if application is aligned with p-net state.
 */
app_subslot_t * app_utils_plug_submodule (
   app_api_t * p_api,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_id,
   const pnet_data_cfg_t * p_data_cfg,
   const char * submodule_name,
   app_utils_cyclic_callback cyclic_callback,
   void * tag);

/**
 * Unplug any application submodule from given subslot.
 *
 * This is for the application to remember which subslots are
 * populated in the p-net stack.
 *
 * @param p_api            InOut: API
 * @param slot_nbr         In:    Slot number
 * @param subslot_nbr      In:    Subslot number
 * @return 0 on success, -1 on error.
 */
int app_utils_pull_submodule (
   app_api_t * p_api,
   uint16_t slot_nbr,
   uint16_t subslot_nbr);

/**
 * Trigger data callback for all plugged submodules in all slots.
 *
 * The callbacks given in \a app_utils_plug_submodule() are used.
 *
 * @param p_api         In:   API
 */
void app_utils_cyclic_data_poll (app_api_t * p_api);

/**
 * Get subslot application information.
 *
 * @param p_appdata        InOut: Application state.
 * @param slot_nbr         In:    Slot number.
 * @param subslot_nbr      In:    Subslot number. Range 0 - 0x9FFF.
 * @return Reference to application subslot,
 *         NULL if subslot is not found/plugged.
 */
app_subslot_t * app_utils_subslot_get (
   app_api_t * p_api,
   uint16_t slot_nbr,
   uint16_t subslot_nbr);

/**
 * Return true if subslot is input.
 *
 * @param p_subslot        In:    Reference to subslot.
 * @return true if subslot is input or input/output.
 *         false if not.
 */
bool app_utils_subslot_is_input (const app_subslot_t * p_subslot);

/**
 * Return true if subslot is neither input or output.
 *
 * This is applies for DAP submodules/slots
 *
 * @param p_subslot     In: Reference to subslot.
 * @return true if subslot is input or input/output.
 *         false if not.
 */
bool app_utils_subslot_is_no_io (const app_subslot_t * p_subslot);

/**
 * Return true if subslot is output.
 *
 * @param p_subslot     In: Reference to subslot.
 * @return true if subslot is output or input/output,
 *         false if not.
 */
bool app_utils_subslot_is_output (const app_subslot_t * p_subslot);

#ifdef __cplusplus
}
#endif

#endif /* APP_UTILS_H */
