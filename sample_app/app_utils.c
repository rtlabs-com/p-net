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

#define _GNU_SOURCE /* For asprintf() */

#include "app_utils.h"

#include "app_gsdml_api.h"
#include "app_log.h"

#include "osal.h"
#include "osal_log.h" /* For LOG_LEVEL */
#include "pnal.h"
#include <pnet_api.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void app_utils_ip_to_string (pnal_ipaddr_t ip, char * outputstring)
{
   snprintf (
      outputstring,
      PNAL_INET_ADDRSTR_SIZE,
      "%u.%u.%u.%u",
      (uint8_t)((ip >> 24) & 0xFF),
      (uint8_t)((ip >> 16) & 0xFF),
      (uint8_t)((ip >> 8) & 0xFF),
      (uint8_t)(ip & 0xFF));
}

void app_utils_mac_to_string (pnet_ethaddr_t mac, char * outputstring)
{
   snprintf (
      outputstring,
      PNAL_ETH_ADDRSTR_SIZE,
      "%02X:%02X:%02X:%02X:%02X:%02X",
      mac.addr[0],
      mac.addr[1],
      mac.addr[2],
      mac.addr[3],
      mac.addr[4],
      mac.addr[5]);
}

const char * app_utils_submod_dir_to_string (pnet_submodule_dir_t direction)
{
   const char * s = "<error>";

   switch (direction)
   {
   case PNET_DIR_NO_IO:
      s = "NO_IO";
      break;
   case PNET_DIR_INPUT:
      s = "INPUT";
      break;
   case PNET_DIR_OUTPUT:
      s = "OUTPUT";
      break;
   case PNET_DIR_IO:
      s = "INPUT_OUTPUT";
      break;
   }

   return s;
}

const char * app_utils_ioxs_to_string (pnet_ioxs_values_t ioxs)
{
   const char * s = "<error>";
   switch (ioxs)
   {
   case PNET_IOXS_BAD:
      s = "IOXS_BAD";
      break;
   case PNET_IOXS_GOOD:
      s = "IOXS_GOOD";
      break;
   }

   return s;
}

void app_utils_get_error_code_strings (
   uint16_t err_cls,
   uint16_t err_code,
   const char ** err_cls_str,
   const char ** err_code_str)
{
   if (err_cls_str == NULL || err_cls_str == NULL)
   {
      return;
   }
   *err_cls_str = "Not decoded";
   *err_code_str = "Not decoded";

   switch (err_cls)
   {
   case PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL:
      *err_cls_str = "Real-Time Acyclic Protocol";
      switch (err_code)
      {
      case PNET_ERROR_CODE_2_ABORT_AR_CONSUMER_DHT_EXPIRED:
         *err_code_str = "Device missed cyclic data "
                         "deadline, device terminated AR";
         break;
      case PNET_ERROR_CODE_2_ABORT_AR_CMI_TIMEOUT:
         *err_code_str = "Communication initialization "
                         "timeout, device terminated AR";
         break;
      case PNET_ERROR_CODE_2_ABORT_AR_RELEASE_IND_RECEIVED:
         *err_code_str = "AR release indication received";
         break;
      case PNET_ERROR_CODE_2_ABORT_DCP_STATION_NAME_CHANGED:
         *err_code_str = "DCP station name changed, "
                         "device terminated AR";
         break;
      case PNET_ERROR_CODE_2_ABORT_DCP_RESET_TO_FACTORY:
         *err_code_str = "DCP reset to factory or factory "
                         "reset, device terminated AR";
         break;
      }
      break;

   case PNET_ERROR_CODE_1_CTLDINA:
      *err_cls_str = "CTLDINA = Name and IP assignment from controller";
      switch (err_code)
      {
      case PNET_ERROR_CODE_2_CTLDINA_ARP_MULTIPLE_IP_ADDRESSES:
         *err_code_str = "Multiple users of same IP address";
         break;
      }
      break;
   }
}

void app_utils_copy_ip_to_struct (
   pnet_cfg_ip_addr_t * destination_struct,
   pnal_ipaddr_t ip)
{
   destination_struct->a = ((ip >> 24) & 0xFF);
   destination_struct->b = ((ip >> 16) & 0xFF);
   destination_struct->c = ((ip >> 8) & 0xFF);
   destination_struct->d = (ip & 0xFF);
}

const char * app_utils_dcontrol_cmd_to_string (
   pnet_control_command_t control_command)
{
   const char * s = NULL;

   switch (control_command)
   {
   case PNET_CONTROL_COMMAND_PRM_BEGIN:
      s = "PRM_BEGIN";
      break;
   case PNET_CONTROL_COMMAND_PRM_END:
      s = "PRM_END";
      break;
   case PNET_CONTROL_COMMAND_APP_RDY:
      s = "APP_RDY";
      break;
   case PNET_CONTROL_COMMAND_RELEASE:
      s = "RELEASE";
      break;
   default:
      s = "<error>";
      break;
   }

   return s;
}

const char * app_utils_event_to_string (pnet_event_values_t event)
{
   const char * s = "<error>";

   switch (event)
   {
   case PNET_EVENT_ABORT:
      s = "PNET_EVENT_ABORT";
      break;
   case PNET_EVENT_STARTUP:
      s = "PNET_EVENT_STARTUP";
      break;
   case PNET_EVENT_PRMEND:
      s = "PNET_EVENT_PRMEND";
      break;
   case PNET_EVENT_APPLRDY:
      s = "PNET_EVENT_APPLRDY";
      break;
   case PNET_EVENT_DATA:
      s = "PNET_EVENT_DATA";
      break;
   }

   return s;
}

int app_utils_pnet_cfg_init_default (
   pnet_cfg_t * cfg,
   uint32_t tick_interval_us,
   const char * serial_number,
   app_data_t * app)
{
   memset (cfg, 0, sizeof (pnet_cfg_t));

   app_gsdml_update_config (cfg);

   cfg->tick_us = tick_interval_us;

   snprintf (
      cfg->im_0_data.im_serial_number,
      sizeof (cfg->im_0_data.im_serial_number),
      "%s",
      serial_number);

   /* Will typically be overwritten by application, as part of network
    * configuration. */
   cfg->num_physical_ports = 1;

   /* Diagnosis mechanism */
   /* We prefer using "Extended channel diagnosis" instead of
    * "Qualified channel diagnosis" format on the wire,
    * as this is better supported by Wireshark.
    */
   cfg->use_qualified_diagnosis = false;

   cfg->state_cb = app_utils_state_ind;
   cfg->connect_cb = app_utils_connect_ind;
   cfg->release_cb = app_utils_release_ind;
   cfg->dcontrol_cb = app_utils_dcontrol_ind;
   cfg->ccontrol_cb = app_utils_ccontrol_cnf;
   cfg->read_cb = app_utils_read_ind;
   cfg->write_cb = app_utils_write_ind;
   cfg->exp_module_cb = app_utils_exp_module_ind;
   cfg->exp_submodule_cb = app_utils_exp_submodule_ind;
   cfg->new_data_status_cb = app_utils_new_data_status_ind;
   cfg->alarm_ind_cb = app_utils_alarm_ind;
   cfg->alarm_cnf_cb = app_utils_alarm_cnf;
   cfg->alarm_ack_cnf_cb = app_utils_alarm_ack_cnf;
   cfg->reset_cb = app_utils_reset_ind;
   cfg->signal_led_cb = app_utils_signal_led;
   cfg->cb_arg = (void *)app;

   return 0;
}

int app_utils_get_netif_namelist (
   const char * arg_str,
   uint16_t max_port,
   app_utils_netif_namelist_t * p_if_list,
   uint16_t * p_num_ports)
{
   int ret = 0;
   uint16_t i = 0;
   uint16_t j = 0;
   uint16_t if_index = 0;
   uint16_t number_of_given_names = 1;
   uint16_t if_list_size = max_port + 1;
   char c;

   if (max_port == 0)
   {
      printf ("Error: max_port is 0.\n");
      return -1;
   }

   memset (p_if_list, 0, sizeof (*p_if_list));
   c = arg_str[i++];
   while (c != '\0')
   {
      if (c != ',')
      {
         if (if_index < if_list_size)
         {
            p_if_list->netif[if_index].name[j++] = c;
         }
      }
      else
      {
         if (if_index < if_list_size)
         {
            p_if_list->netif[if_index].name[j++] = '\0';
            j = 0;
            if_index++;
         }
         number_of_given_names++;
      }

      c = arg_str[i++];
   }

   if (max_port == 1 && number_of_given_names > 1)
   {
      printf ("Error: Only 1 network interface expected as max_port is 1.\n");
      return -1;
   }
   if (number_of_given_names == 2)
   {
      printf ("Error: It is illegal to give 2 interface names. Use 1, or one "
              "more than the number of physical interfaces.\n");
      return -1;
   }
   if (number_of_given_names > max_port + 1)
   {
      printf (
         "Error: You have given %u interface names, but max is %u as "
         "PNET_MAX_PHYSICAL_PORTS is %u.\n",
         number_of_given_names,
         max_port + 1,
         max_port);
      return -1;
   }

   if (number_of_given_names == 1)
   {
      if (strlen (p_if_list->netif[0].name) == 0)
      {
         printf ("Error: Zero length network interface name.\n");
         return -1;
      }
      else
      {
         p_if_list->netif[1] = p_if_list->netif[0];
         *p_num_ports = 1;
      }
   }
   else
   {
      for (i = 0; i < number_of_given_names; i++)
      {
         if (strlen (p_if_list->netif[i].name) == 0)
         {
            printf ("Error: Zero length network interface name (%d).\n", i);
            return -1;
         }
      }

      *p_num_ports = number_of_given_names - 1;
   }

   return ret;
}

int app_utils_pnet_cfg_init_netifs (
   const char * netif_list_str,
   uint16_t default_mau_type,
   app_utils_netif_namelist_t * if_list,
   uint16_t * number_of_ports,
   pnet_if_cfg_t * if_cfg)
{
   int ret = 0;
   int i = 0;
   pnal_ipaddr_t ip;
   pnal_ipaddr_t netmask;
   pnal_ipaddr_t gateway;

   ret = app_utils_get_netif_namelist (
      netif_list_str,
      PNET_MAX_PHYSICAL_PORTS,
      if_list,
      number_of_ports);
   if (ret != 0)
   {
      return ret;
   }
   if_cfg->main_netif_name = if_list->netif[0].name;

   for (i = 1; i <= *number_of_ports; i++)
   {
      if_cfg->physical_ports[i - 1].netif_name = if_list->netif[i].name;
      if_cfg->physical_ports[i - 1].default_mau_type = default_mau_type;
   }

   /* Read IP, netmask, gateway from operating system */
   ip = pnal_get_ip_address (if_cfg->main_netif_name);
   netmask = pnal_get_netmask (if_cfg->main_netif_name);
   gateway = pnal_get_gateway (if_cfg->main_netif_name);

   app_utils_copy_ip_to_struct (&if_cfg->ip_cfg.ip_addr, ip);
   app_utils_copy_ip_to_struct (&if_cfg->ip_cfg.ip_gateway, gateway);
   app_utils_copy_ip_to_struct (&if_cfg->ip_cfg.ip_mask, netmask);

   return ret;
}

static void app_utils_print_mac_address (const char * netif_name)
{
   pnal_ethaddr_t pnal_mac_addr;
   if (pnal_get_macaddress (netif_name, &pnal_mac_addr) == 0)
   {
      APP_LOG_INFO (
         "%02X:%02X:%02X:%02X:%02X:%02X\n",
         pnal_mac_addr.addr[0],
         pnal_mac_addr.addr[1],
         pnal_mac_addr.addr[2],
         pnal_mac_addr.addr[3],
         pnal_mac_addr.addr[4],
         pnal_mac_addr.addr[5]);
   }
   else
   {
      APP_LOG_ERROR ("Failed read mac address\n");
   }
}

void app_utils_print_network_config (
   pnet_if_cfg_t * if_cfg,
   uint16_t number_of_ports)
{
   uint16_t i;
   char hostname_string[PNAL_HOSTNAME_MAX_SIZE]; /* Terminated string */

   APP_LOG_INFO ("Management port:      %s ", if_cfg->main_netif_name);
   app_utils_print_mac_address (if_cfg->main_netif_name);
   for (i = 1; i <= number_of_ports; i++)
   {
      APP_LOG_INFO (
         "Physical port [%u]:    %s ",
         i,
         if_cfg->physical_ports[i - 1].netif_name);

      app_utils_print_mac_address (if_cfg->physical_ports[i - 1].netif_name);
   }

   if (pnal_get_hostname (hostname_string) != 0)
   {
      hostname_string[0] = '\0';
   }

   APP_LOG_INFO ("Hostname:             %s\n", hostname_string);
   APP_LOG_INFO (
      "IP address:           %u.%u.%u.%u\n",
      if_cfg->ip_cfg.ip_addr.a,
      if_cfg->ip_cfg.ip_addr.b,
      if_cfg->ip_cfg.ip_addr.c,
      if_cfg->ip_cfg.ip_addr.d);
   APP_LOG_INFO (
      "Netmask:              %u.%u.%u.%u\n",
      if_cfg->ip_cfg.ip_mask.a,
      if_cfg->ip_cfg.ip_mask.b,
      if_cfg->ip_cfg.ip_mask.c,
      if_cfg->ip_cfg.ip_mask.d);
   APP_LOG_INFO (
      "Gateway:              %u.%u.%u.%u\n",
      if_cfg->ip_cfg.ip_gateway.a,
      if_cfg->ip_cfg.ip_gateway.b,
      if_cfg->ip_cfg.ip_gateway.c,
      if_cfg->ip_cfg.ip_gateway.d);
}

void app_utils_print_ioxs_change (
   const app_subslot_t * subslot,
   const char * ioxs_str,
   uint8_t iocs_current,
   uint8_t iocs_new)
{
   if (iocs_current != iocs_new)
   {
      if (iocs_new == PNET_IOXS_BAD)
      {
         APP_LOG_DEBUG (
            "PLC reports %s BAD for slot %u subslot %u \"%s\"\n",
            ioxs_str,
            subslot->slot_nbr,
            subslot->subslot_nbr,
            subslot->submodule_name);
      }
      else if (iocs_new == PNET_IOXS_GOOD)
      {
         APP_LOG_DEBUG (
            "PLC reports %s GOOD for slot %u subslot %u \"%s\".\n",
            ioxs_str,
            subslot->slot_nbr,
            subslot->subslot_nbr,
            subslot->submodule_name);
      }
      else if (iocs_new != PNET_IOXS_GOOD)
      {
         APP_LOG_DEBUG (
            "PLC reports %s %u for input slot %u subslot %u \"%s\".\n"
            "  Is the PLC in STOP mode?\n",
            ioxs_str,
            iocs_new,
            subslot->slot_nbr,
            subslot->subslot_nbr,
            subslot->submodule_name);
      }
   }
}

int app_utils_plug_module (
   app_api_t * p_api,
   uint16_t slot_nbr,
   uint32_t id,
   const char * name)
{
   if (slot_nbr >= PNET_MAX_SLOTS)
   {
      return -1;
   }

   p_api->slots[slot_nbr].module_id = id;
   p_api->slots[slot_nbr].plugged = true;
   p_api->slots[slot_nbr].name = name;

   return 0;
}

int app_utils_pull_module (app_api_t * p_api, uint16_t slot_nbr)
{
   if (slot_nbr >= PNET_MAX_SLOTS)
   {
      return -1;
   }

   p_api->slots[slot_nbr].plugged = false;

   return 0;
}

app_subslot_t * app_utils_plug_submodule (
   app_api_t * p_api,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_ident,
   const pnet_data_cfg_t * p_data_cfg,
   const char * submodule_name,
   app_utils_cyclic_callback cyclic_callback,
   void * tag)
{
   uint16_t subslot_ix;

   if (slot_nbr >= PNET_MAX_SLOTS || p_api == NULL || p_data_cfg == NULL)
   {
      return NULL;
   }

   /** Find a free subslot */
   for (subslot_ix = 0; subslot_ix < PNET_MAX_SUBSLOTS; subslot_ix++)
   {
      if (p_api->slots[slot_nbr].subslots[subslot_ix].used == false)
      {
         app_subslot_t * p_subslot =
            &p_api->slots[slot_nbr].subslots[subslot_ix];

         p_subslot->used = true;
         p_subslot->plugged = true;
         p_subslot->slot_nbr = slot_nbr;
         p_subslot->subslot_nbr = subslot_nbr;
         p_subslot->submodule_name = submodule_name;
         p_subslot->submodule_id = submodule_ident;
         p_subslot->data_cfg = *p_data_cfg;
         p_subslot->cyclic_callback = cyclic_callback;
         p_subslot->tag = tag;
         p_subslot->indata_iocs = PNET_IOXS_BAD;
         p_subslot->outdata_iops = PNET_IOXS_BAD;
         return p_subslot;
      }
   }

   return NULL;
}

int app_utils_pull_submodule (
   app_api_t * p_api,
   uint16_t slot_nbr,
   uint16_t subslot_nbr)
{
   app_subslot_t * p_subslot = NULL;

   if (slot_nbr >= PNET_MAX_SUBSLOTS || p_api == NULL)
   {
      return -1;
   }

   p_subslot = app_utils_subslot_get (p_api, slot_nbr, subslot_nbr);
   if (p_subslot == NULL)
   {
      return -1;
   }

   memset (p_subslot, 0, sizeof (app_subslot_t));
   p_subslot->used = false;

   return 0;
}

app_subslot_t * app_utils_subslot_get (
   app_api_t * p_api,
   uint16_t slot_nbr,
   uint16_t subslot_nbr)
{
   uint16_t subslot_ix;

   if (slot_nbr >= PNET_MAX_SLOTS || p_api == NULL)
   {
      return NULL;
   }

   for (subslot_ix = 0; subslot_ix < PNET_MAX_SUBSLOTS; subslot_ix++)
   {
      if (p_api->slots[slot_nbr].subslots[subslot_ix].subslot_nbr == subslot_nbr)
      {
         return &p_api->slots[slot_nbr].subslots[subslot_ix];
      }
   }

   return NULL;
}

bool app_utils_subslot_is_input (const app_subslot_t * p_subslot)
{
   if (p_subslot == NULL || p_subslot->used == false)
   {
      return false;
   }

   if (
      p_subslot->data_cfg.data_dir == PNET_DIR_INPUT ||
      p_subslot->data_cfg.data_dir == PNET_DIR_IO)
   {
      return true;
   }

   return false;
}

bool app_utils_subslot_is_no_io (const app_subslot_t * p_subslot)
{
   if (p_subslot == NULL || p_subslot->used == false)
   {
      return false;
   }

   return p_subslot->data_cfg.data_dir == PNET_DIR_NO_IO;
}

bool app_utils_subslot_is_output (const app_subslot_t * p_subslot)
{
   if (p_subslot == NULL || p_subslot->used == false)
   {
      return false;
   }

   if (
      p_subslot->data_cfg.data_dir == PNET_DIR_OUTPUT ||
      p_subslot->data_cfg.data_dir == PNET_DIR_IO)
   {
      return true;
   }

   return false;
}

void app_utils_cyclic_data_poll (app_api_t * p_api)
{
   uint16_t slot_nbr;
   uint16_t subslot_index;
   app_subslot_t * p_subslot;

   for (slot_nbr = 0; slot_nbr < PNET_MAX_SLOTS; slot_nbr++)
   {
      for (subslot_index = 0; subslot_index < PNET_MAX_SUBSLOTS;
           subslot_index++)
      {
         p_subslot = &p_api->slots[slot_nbr].subslots[subslot_index];
         if (p_subslot->plugged && p_subslot->cyclic_callback != NULL)
         {
            p_subslot->cyclic_callback (p_subslot, p_subslot->tag);
         }
      }
   }
}

pnet_t * app_utils_get_pnet_instance (app_data_t * app)
{
   if (app == NULL)
   {
      return NULL;
   }

   return app->net;
}

bool app_utils_is_connected_to_controller (app_data_t * app)
{
   return app->main_api.arep != UINT32_MAX;
}

void app_utils_main_timer_tick (os_timer_t * timer, void * arg)
{
   app_data_t * app = (app_data_t *)arg;

   os_event_set (app->main_events, APP_EVENT_TIMER);
}

void app_utils_set_outputs_default_value (void * user_data)
{
   APP_LOG_DEBUG ("Setting outputs to default values.\n");
   app_data_set_default_outputs (user_data);
}

/*********************************** Callbacks ********************************/

int app_utils_connect_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result)
{
   APP_LOG_DEBUG ("PLC connect indication. AREP: %u\n", arep);
   /*
    *  Handle the request on an application level.
    *  This is a very simple application which does not need to handle anything.
    *  All the needed information is in the AR data structure.
    */

   return 0;
}

int app_utils_release_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result)
{
   app_data_t * app = (app_data_t *)arg;

   APP_LOG_DEBUG ("PLC release (disconnect) indication. AREP: %u\n", arep);

   app_utils_set_outputs_default_value (app->user_data);

   return 0;
}

int app_utils_dcontrol_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_control_command_t control_command,
   pnet_result_t * p_result)
{
   APP_LOG_DEBUG (
      "PLC dcontrol message (The PLC is done with parameter writing). "
      "AREP: %u  Command: %s\n",
      arep,
      app_utils_dcontrol_cmd_to_string (control_command));

   return 0;
}

int app_utils_ccontrol_cnf (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result)
{
   APP_LOG_DEBUG (
      "PLC ccontrol message confirmation (The PLC has received our Application "
      "Ready message). AREP: %u  Status codes: %d %d %d %d\n",
      arep,
      p_result->pnio_status.error_code,
      p_result->pnio_status.error_decode,
      p_result->pnio_status.error_code_1,
      p_result->pnio_status.error_code_2);

   return 0;
}

int app_utils_write_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t api,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t idx,
   uint16_t sequence_number,
   uint16_t write_length,
   const uint8_t * p_write_data,
   pnet_result_t * p_result)
{
   int result = 0;
   app_data_t * app = (app_data_t *)arg;
   app_subslot_t * subslot;
   APP_LOG_DEBUG (
      "PLC write record indication.\n"
      "  AREP: %u API: %u Slot: %2u Subslot: %u Index: %u Sequence: %2u "
      "Length: %u\n",
      arep,
      api,
      slot_nbr,
      subslot_nbr,
      (unsigned)idx,
      sequence_number,
      write_length);

   subslot = app_utils_subslot_get (&app->main_api, slot_nbr, subslot_nbr);
   if (subslot == NULL)
   {
      APP_LOG_WARNING (
         "No submodule plugged in AREP: %u API: %u Slot: %2u Subslot: %u "
         "Index will not be written.\n",
         arep,
         api,
         slot_nbr,
         subslot_nbr);
      p_result->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
      p_result->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
      p_result->pnio_status.error_code_1 = PNET_ERROR_CODE_1_APP_WRITE_ERROR;
      p_result->pnio_status.error_code_2 = 0; /* User specific */

      return -1;
   }

   result = app_data_write_parameter (
      app->user_data,
      slot_nbr,
      subslot_nbr,
      subslot->submodule_id,
      idx,
      p_write_data,
      write_length);
   if (result != 0)
   {
      APP_LOG_WARNING (
         "Failed to write index for AREP: %u API: %u Slot: %2u Subslot: %u "
         "index %u.\n",
         arep,
         api,
         slot_nbr,
         subslot_nbr,
         idx);
      p_result->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
      p_result->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
      p_result->pnio_status.error_code_1 = PNET_ERROR_CODE_1_APP_WRITE_ERROR;
      p_result->pnio_status.error_code_2 = 0; /* User specific */
   }

   return result;
}

int app_utils_read_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t api,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t idx,
   uint16_t sequence_number,
   uint8_t ** pp_read_data,
   uint16_t * p_read_length,
   pnet_result_t * p_result)
{
   int result = 0;
   app_data_t * app = (app_data_t *)arg;
   app_subslot_t * subslot;

   APP_LOG_DEBUG (
      "PLC read record indication.\n"
      "  AREP: %u API: %u Slot: %2u Subslot: %u Index: %u Sequence: %2u Max "
      "length: %u\n",
      arep,
      api,
      slot_nbr,
      subslot_nbr,
      (unsigned)idx,
      sequence_number,
      (unsigned)*p_read_length);

   subslot = app_utils_subslot_get (&app->main_api, slot_nbr, subslot_nbr);
   if (subslot == NULL)
   {
      APP_LOG_WARNING (
         "No submodule plugged in AREP: %u API: %u Slot: %2u Subslot: %u "
         "Index will not be read.\n",
         arep,
         api,
         slot_nbr,
         subslot_nbr);
      p_result->pnio_status.error_code = PNET_ERROR_CODE_READ;
      p_result->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
      p_result->pnio_status.error_code_1 = PNET_ERROR_CODE_1_APP_READ_ERROR;
      p_result->pnio_status.error_code_2 = 0; /* User specific */
      return -1;
   }

   result = app_data_read_parameter (
      app->user_data,
      slot_nbr,
      subslot_nbr,
      subslot->submodule_id,
      idx,
      pp_read_data,
      p_read_length);

   if (result != 0)
   {
      APP_LOG_WARNING (
         "Failed to read index for AREP: %u API: %u Slot: %2u Subslot: %u "
         "index %u.\n",
         arep,
         api,
         slot_nbr,
         subslot_nbr,
         idx);
      p_result->pnio_status.error_code = PNET_ERROR_CODE_READ;
      p_result->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
      p_result->pnio_status.error_code_1 = PNET_ERROR_CODE_1_APP_READ_ERROR;
      p_result->pnio_status.error_code_2 = 0; /* User specific */
   }

   return result;
}

int app_utils_state_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_event_values_t event)
{
   uint16_t err_cls = 0;  /* Error code 1 */
   uint16_t err_code = 0; /* Error code 2 */
   const char * error_class_description = "";
   const char * error_code_description = "";

   app_data_t * app = (app_data_t *)arg;

   APP_LOG_DEBUG (
      "Event indication %s   AREP: %u\n",
      app_utils_event_to_string (event),
      arep);

   if (event == PNET_EVENT_ABORT)
   {
      if (pnet_get_ar_error_codes (net, arep, &err_cls, &err_code) == 0)
      {
         app_utils_get_error_code_strings (
            err_cls,
            err_code,
            &error_class_description,
            &error_code_description);
         APP_LOG_DEBUG (
            "    Error class: 0x%02x %s \n"
            "    Error code:  0x%02x %s \n",
            (unsigned)err_cls,
            error_class_description,
            (unsigned)err_code,
            error_code_description);
      }
      else
      {
         APP_LOG_DEBUG ("    No error status available\n");
      }
      /* Set output values */
      app_utils_set_outputs_default_value (app->user_data);

      /* Only abort AR with correct session key */
      os_event_set (app->main_events, APP_EVENT_ABORT);
   }
   else if (event == PNET_EVENT_PRMEND)
   {
      if (app_utils_is_connected_to_controller (app))
      {
         APP_LOG_WARNING ("Warning - AREP out of sync\n");
      }
      app->main_api.arep = arep;
      app_utils_set_initial_data_and_ioxs (app);

      (void)pnet_set_provider_state (net, true);

      /* Send application ready at next tick
         Do not call pnet_application_ready() here as it will affect
         the internal stack states */
      app->arep_for_appl_ready = arep;
      os_event_set (app->main_events, APP_EVENT_READY_FOR_DATA);
   }
   else if (event == PNET_EVENT_DATA)
   {
      APP_LOG_DEBUG ("Cyclic data transmission started\n\n");
   }

   return 0;
}

int app_utils_exp_module_ind (
   pnet_t * net,
   void * arg,
   uint32_t api,
   uint16_t slot,
   uint32_t module_ident)
{
   int ret = -1;
   int result = 0;
   app_data_t * app = (app_data_t *)arg;
   const char * module_name = "unknown";
   const app_gsdml_module_t * module_config;

   APP_LOG_DEBUG ("Module plug indication\n");

   if (slot >= PNET_MAX_SLOTS)
   {
      APP_LOG_ERROR (
         "Wrong slot number received: %u  It should be less than %u\n",
         slot,
         PNET_MAX_SLOTS);
      return -1;
   }

   module_config = app_gsdml_get_module_cfg (module_ident);
   if (module_config == NULL)
   {
      APP_LOG_ERROR ("  Module ID %08x not found.\n", (unsigned)module_ident);
      /*
       * Needed to pass Behavior scenario 2
       */
      APP_LOG_DEBUG ("  Plug expected module anyway\n");
   }
   else
   {
      module_name = module_config->name;
   }

   APP_LOG_DEBUG ("  Pull old module.    API: %u Slot: %2u\n", api, slot);
   result = pnet_pull_module (net, api, slot);

   if (result == 0)
   {
      (void)app_utils_pull_module (&app->main_api, slot);
   }

   APP_LOG_DEBUG (
      "  Plug module.        API: %u Slot: %2u Module ID: 0x%x \"%s\"\n",
      api,
      slot,
      (unsigned)module_ident,
      module_name);

   ret = pnet_plug_module (net, api, slot, module_ident);
   if (ret == 0)
   {
      (void)app_utils_plug_module (
         &app->main_api,
         slot,
         module_ident,
         module_name);
   }
   else
   {
      APP_LOG_ERROR (
         "Plug module failed. Ret: %u API: %u Slot: %2u Module ID: 0x%x\n",
         ret,
         api,
         slot,
         (unsigned)module_ident);
   }

   return ret;
}

int app_utils_exp_submodule_ind (
   pnet_t * net,
   void * arg,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint32_t module_id,
   uint32_t submodule_id,
   const pnet_data_cfg_t * p_exp_data)
{
   int ret = -1;
   int result = 0;
   pnet_data_cfg_t data_cfg = {0};
   app_data_t * app = (app_data_t *)arg;
   const app_gsdml_submodule_t * submod_cfg;
   const char * name = "Unsupported";
   app_utils_cyclic_callback cyclic_data_callback = NULL;

   APP_LOG_DEBUG ("Submodule plug indication.\n");

   submod_cfg = app_gsdml_get_submodule_cfg (submodule_id);
   if (submod_cfg != NULL)
   {
      data_cfg.data_dir = submod_cfg->data_dir;
      data_cfg.insize = submod_cfg->insize;
      data_cfg.outsize = submod_cfg->outsize;
      name = submod_cfg->name;

      if (data_cfg.insize > 0 || data_cfg.outsize > 0)
      {
         cyclic_data_callback = app_utils_cyclic_data_callback;
      }
   }
   else
   {
      APP_LOG_WARNING (
         "  Submodule ID 0x%x in module ID 0x%x not found. API: %u Slot: %2u "
         "Subslot %u \n",
         (unsigned)submodule_id,
         (unsigned)module_id,
         api,
         slot,
         subslot);

      /*
       * Needed for behavior scenario 2 to pass.
       * Iops will be set to bad for this subslot
       */
      APP_LOG_WARNING ("  Plug expected submodule anyway \n");

      data_cfg.data_dir = p_exp_data->data_dir;
      data_cfg.insize = p_exp_data->insize;
      data_cfg.outsize = p_exp_data->outsize;
   }

   APP_LOG_DEBUG (
      "  Pull old submodule. API: %u Slot: %2u Subslot: %u\n",
      api,
      slot,
      subslot);

   result = pnet_pull_submodule (net, api, slot, subslot);
   if (result == 0)
   {
      (void)app_utils_pull_submodule (&app->main_api, slot, subslot);
   }

   APP_LOG_DEBUG (
      "  Plug submodule.     API: %u Slot: %2u Module ID: 0x%-4x\n"
      "                      Subslot: %u Submodule ID: 0x%x \"%s\"\n",
      api,
      slot,
      (unsigned)module_id,
      subslot,
      (unsigned)submodule_id,
      name);

   APP_LOG_DEBUG (
      "                      Data Dir: %s  In: %u bytes  Out: %u bytes\n",
      app_utils_submod_dir_to_string (data_cfg.data_dir),
      data_cfg.insize,
      data_cfg.outsize);

   if (
      data_cfg.data_dir != p_exp_data->data_dir ||
      data_cfg.insize != p_exp_data->insize ||
      data_cfg.outsize != p_exp_data->outsize)
   {
      APP_LOG_WARNING (
         "    Warning expected  Data Dir: %s  In: %u bytes  Out: %u bytes\n",
         app_utils_submod_dir_to_string (p_exp_data->data_dir),
         p_exp_data->insize,
         p_exp_data->outsize);
   }
   ret = pnet_plug_submodule (
      net,
      api,
      slot,
      subslot,
      module_id,
      submodule_id,
      data_cfg.data_dir,
      data_cfg.insize,
      data_cfg.outsize);

   if (ret == 0)
   {
      (void)app_utils_plug_submodule (
         &app->main_api,
         slot,
         subslot,
         submodule_id,
         &data_cfg,
         name,
         cyclic_data_callback,
         app);
   }
   else
   {
      APP_LOG_ERROR (
         "  Plug submodule failed. Ret: %u API: %u Slot: %2u Subslot %u "
         "Module ID: 0x%x Submodule ID: 0x%x \n",
         ret,
         api,
         slot,
         subslot,
         (unsigned)module_id,
         (unsigned)submodule_id);
   }

   return ret;
}

int app_utils_new_data_status_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t crep,
   uint8_t changes,
   uint8_t data_status)
{
   app_data_t * app = (app_data_t *)arg;

   bool is_running = data_status & BIT (PNET_DATA_STATUS_BIT_PROVIDER_STATE);
   bool is_valid = data_status & BIT (PNET_DATA_STATUS_BIT_DATA_VALID);

   APP_LOG_DEBUG (
      "Data status indication. AREP: %u  Data status changes: 0x%02x  "
      "Data status: 0x%02x\n",
      arep,
      changes,
      data_status);
   APP_LOG_DEBUG (
      "   %s, %s, %s, %s, %s\n",
      is_running ? "Run" : "Stop",
      is_valid ? "Valid" : "Invalid",
      (data_status & BIT (PNET_DATA_STATUS_BIT_STATE)) ? "Primary" : "Backup",
      (data_status & BIT (PNET_DATA_STATUS_BIT_STATION_PROBLEM_INDICATOR))
         ? "Normal operation"
         : "Problem",
      (data_status & BIT (PNET_DATA_STATUS_BIT_IGNORE))
         ? "Ignore data status"
         : "Evaluate data status");

   if (is_running == false || is_valid == false)
   {
      app_utils_set_outputs_default_value (app->user_data);
   }

   return 0;
}

int app_utils_alarm_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   const pnet_alarm_argument_t * p_alarm_arg,
   uint16_t data_len,
   uint16_t data_usi,
   const uint8_t * p_data)
{
   app_data_t * app = (app_data_t *)arg;

   APP_LOG_DEBUG (
      "Alarm indication. AREP: %u API: %d Slot: %d Subslot: %d "
      "Type: %d Seq: %d Length: %d USI: %d\n",
      arep,
      p_alarm_arg->api_id,
      p_alarm_arg->slot_nbr,
      p_alarm_arg->subslot_nbr,
      p_alarm_arg->alarm_type,
      p_alarm_arg->sequence_number,
      data_len,
      data_usi);

   app->alarm_arg = *p_alarm_arg;
   os_event_set (app->main_events, APP_EVENT_ALARM);

   return 0;
}

int app_utils_alarm_cnf (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   const pnet_pnio_status_t * p_pnio_status)
{
   app_data_t * app = (app_data_t *)arg;

   APP_LOG_DEBUG (
      "PLC alarm confirmation. AREP: %u  Status code %u, "
      "%u, %u, %u\n",
      arep,
      p_pnio_status->error_code,
      p_pnio_status->error_decode,
      p_pnio_status->error_code_1,
      p_pnio_status->error_code_2);

   app->alarm_allowed = true;

   return 0;
}

int app_utils_alarm_ack_cnf (pnet_t * net, void * arg, uint32_t arep, int res)
{
   APP_LOG_DEBUG (
      "PLC alarm ACK confirmation. AREP: %u  Result: "
      "%d\n",
      arep,
      res);

   return 0;
}

int app_utils_reset_ind (
   pnet_t * net,
   void * arg,
   bool should_reset_application,
   uint16_t reset_mode)
{
   app_data_t * app = (app_data_t *)arg;

   APP_LOG_DEBUG (
      "PLC reset indication. Application reset mandatory: %u  Reset mode: %d\n",
      should_reset_application,
      reset_mode);

   return app_data_reset (app->user_data, should_reset_application, reset_mode);
}

int app_utils_signal_led (pnet_t * net, void * arg, bool led_state)
{
   app_data_t * app = (app_data_t *)arg;

   APP_LOG_INFO ("Profinet signal LED indication. New state: %u\n", led_state);

   return app_data_signal_led (app->user_data, led_state);
}

/**************************************************************************/

int app_utils_set_initial_data_and_ioxs (app_data_t * app)
{
   int ret;
   uint16_t slot;
   uint16_t subslot_index;
   const app_subslot_t * p_subslot;
   uint8_t * indata;
   uint16_t indata_size;
   uint8_t indata_iops;
   bool is_user_inputmodule = false;

   for (slot = 0; slot < PNET_MAX_SLOTS; slot++)
   {
      for (subslot_index = 0; subslot_index < PNET_MAX_SUBSLOTS;
           subslot_index++)
      {
         p_subslot = &app->main_api.slots[slot].subslots[subslot_index];
         if (p_subslot->plugged)
         {
            indata = NULL;
            indata_size = 0;
            indata_iops = PNET_IOXS_BAD;

            if (
               p_subslot->data_cfg.insize > 0 ||
               p_subslot->data_cfg.data_dir == PNET_DIR_NO_IO)
            {
               is_user_inputmodule =
                  (p_subslot->slot_nbr != PNET_SLOT_DAP_IDENT &&
                   p_subslot->data_cfg.insize > 0);

               /* Get input data for submodule
                *
                * For the sample application data includes
                * includes button state and a counter value
                */
               if (is_user_inputmodule)
               {
                  indata = app_data_inputdata_getbuffer (
                     app->user_data,
                     p_subslot->slot_nbr,
                     p_subslot->subslot_nbr,
                     p_subslot->submodule_id,
                     &indata_size,
                     &indata_iops);
               }
               else if (p_subslot->slot_nbr == PNET_SLOT_DAP_IDENT)
               {
                  indata_iops = PNET_IOXS_GOOD;
               }

               ret = pnet_input_set_data_and_iops (
                  app->net,
                  app->main_api.api_id,
                  p_subslot->slot_nbr,
                  p_subslot->subslot_nbr,
                  indata,
                  indata_size,
                  indata_iops);

               /*
                * If a submodule is still plugged but not used in current AR,
                * setting the data and IOPS will fail.
                * This is not a problem.
                * Log message below will only be printed for active
                * submodules.
                */
               if (ret == 0)
               {
                  APP_LOG_DEBUG (
                     "  Set initial input data and IOPS for slot %2u "
                     "subslot "
                     "%5u %9s size %3d \"%s\" \n",
                     p_subslot->slot_nbr,
                     p_subslot->subslot_nbr,
                     app_utils_ioxs_to_string (indata_iops),
                     p_subslot->data_cfg.insize,
                     p_subslot->submodule_name);
               }
            }

            if (p_subslot->data_cfg.outsize > 0)
            {
               ret = pnet_output_set_iocs (
                  app->net,
                  app->main_api.api_id,
                  p_subslot->slot_nbr,
                  p_subslot->subslot_nbr,
                  PNET_IOXS_GOOD);

               if (ret == 0)
               {
                  APP_LOG_DEBUG (
                     "  Set initial output         IOCS for slot %2u "
                     "subslot "
                     "%5u %9s          \"%s\"\n",
                     p_subslot->slot_nbr,
                     p_subslot->subslot_nbr,
                     app_utils_ioxs_to_string (PNET_IOXS_GOOD),
                     p_subslot->submodule_name);
               }
            }
         }
      }
   }
   return 0;
}

void app_utils_cyclic_data_callback (app_subslot_t * subslot, void * tag)
{
   app_data_t * app = (app_data_t *)tag;
   uint8_t * indata;
   uint16_t indata_size = 0;
   uint8_t indata_iops = PNET_IOXS_BAD;
   uint8_t indata_iocs = PNET_IOXS_BAD;
   uint8_t * outdata_buf;
   uint16_t outdata_size = 0;
   bool outdata_updated = false;
   uint8_t outdata_iops = PNET_IOXS_BAD;

   if (app == NULL)
   {
      APP_LOG_ERROR ("Application tag not set in subslot?\n");
      return;
   }

   if (subslot->slot_nbr != PNET_SLOT_DAP_IDENT && subslot->data_cfg.outsize > 0)
   {
      outdata_buf = app_data_outputdata_getbuffer (
         app->user_data,
         subslot->slot_nbr,
         subslot->subslot_nbr,
         subslot->submodule_id,
         &outdata_size);

      if (outdata_size != subslot->data_cfg.outsize)
      {
         APP_LOG_ERROR ("Wrong outputdata size implemented: %u\n", outdata_size);
         app_utils_set_outputs_default_value (app->user_data);
      }
      else
      {
         /* Get output data from the PLC */
         (void)pnet_output_get_data_and_iops (
            app->net,
            PNET_API_NO_APPLICATION_PROFILE,
            subslot->slot_nbr,
            subslot->subslot_nbr,
            &outdata_updated,
            outdata_buf,
            &outdata_size,
            &outdata_iops);

         app_utils_print_ioxs_change (
            subslot,
            "Provider Status (IOPS)",
            subslot->outdata_iops,
            outdata_iops);
         subslot->outdata_iops = outdata_iops;

         if (outdata_size != subslot->data_cfg.outsize)
         {
            APP_LOG_ERROR (
               "Wrong outputdata size written from PLC: %u\n",
               outdata_size);
            app_utils_set_outputs_default_value (app->user_data);
         }
         else if (outdata_iops == PNET_IOXS_GOOD)
         {
            app_data_outputdata_finalize (
               app->user_data,
               subslot->slot_nbr,
               subslot->subslot_nbr,
               subslot->submodule_id);
         }
         else
         {
            app_utils_set_outputs_default_value (app->user_data);
         }
      }
   }

   if (subslot->slot_nbr != PNET_SLOT_DAP_IDENT && subslot->data_cfg.insize > 0)
   {
      /* Get application specific input data from a submodule (not DAP)
       *
       * For the sample application, the data includes a button
       * state and a counter value. */
      indata = app_data_inputdata_getbuffer (
         app->user_data,
         subslot->slot_nbr,
         subslot->subslot_nbr,
         subslot->submodule_id,
         &indata_size,
         &indata_iops);

      if (indata_size != subslot->data_cfg.insize)
      {
         APP_LOG_ERROR ("Wrong inputdata size implemented: %u\n", outdata_size);
      }
      else
      {
         /* Send input data to the PLC */
         (void)pnet_input_set_data_and_iops (
            app->net,
            PNET_API_NO_APPLICATION_PROFILE,
            subslot->slot_nbr,
            subslot->subslot_nbr,
            indata,
            indata_size,
            indata_iops);

         (void)pnet_input_get_iocs (
            app->net,
            PNET_API_NO_APPLICATION_PROFILE,
            subslot->slot_nbr,
            subslot->subslot_nbr,
            &indata_iocs);

         app_utils_print_ioxs_change (
            subslot,
            "Consumer Status (IOCS)",
            subslot->indata_iocs,
            indata_iocs);
         subslot->indata_iocs = indata_iocs;
      }
   }
}

void app_utils_plug_dap (app_data_t * app, uint16_t number_of_ports)
{
   const pnet_data_cfg_t cfg_dap_data = {
      .data_dir = PNET_DIR_NO_IO,
      .insize = 0,
      .outsize = 0,
   };

   APP_LOG_DEBUG ("\nPlug DAP module and its submodules\n");

   app_utils_exp_module_ind (
      app->net,
      app,
      PNET_API_NO_APPLICATION_PROFILE,
      PNET_SLOT_DAP_IDENT,
      PNET_MOD_DAP_IDENT);

   app_utils_exp_submodule_ind (
      app->net,
      app,
      PNET_API_NO_APPLICATION_PROFILE,
      PNET_SLOT_DAP_IDENT,
      PNET_SUBSLOT_DAP_IDENT,
      PNET_MOD_DAP_IDENT,
      PNET_SUBMOD_DAP_IDENT,
      &cfg_dap_data);

   app_utils_exp_submodule_ind (
      app->net,
      app,
      PNET_API_NO_APPLICATION_PROFILE,
      PNET_SLOT_DAP_IDENT,
      PNET_SUBSLOT_DAP_INTERFACE_1_IDENT,
      PNET_MOD_DAP_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_IDENT,
      &cfg_dap_data);

   app_utils_exp_submodule_ind (
      app->net,
      app,
      PNET_API_NO_APPLICATION_PROFILE,
      PNET_SLOT_DAP_IDENT,
      PNET_SUBSLOT_DAP_INTERFACE_1_PORT_1_IDENT,
      PNET_MOD_DAP_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_PORT_1_IDENT,
      &cfg_dap_data);

   if (number_of_ports >= 2)
   {
      app_utils_exp_submodule_ind (
         app->net,
         app,
         PNET_API_NO_APPLICATION_PROFILE,
         PNET_SLOT_DAP_IDENT,
         PNET_SUBSLOT_DAP_INTERFACE_1_PORT_2_IDENT,
         PNET_MOD_DAP_IDENT,
         PNET_SUBMOD_DAP_INTERFACE_1_PORT_2_IDENT,
         &cfg_dap_data);
   }

   if (number_of_ports >= 3)
   {
      app_utils_exp_submodule_ind (
         app->net,
         app,
         PNET_API_NO_APPLICATION_PROFILE,
         PNET_SLOT_DAP_IDENT,
         PNET_SUBSLOT_DAP_INTERFACE_1_PORT_3_IDENT,
         PNET_MOD_DAP_IDENT,
         PNET_SUBMOD_DAP_INTERFACE_1_PORT_3_IDENT,
         &cfg_dap_data);
   }

   if (number_of_ports >= 4)
   {
      app_utils_exp_submodule_ind (
         app->net,
         app,
         PNET_API_NO_APPLICATION_PROFILE,
         PNET_SLOT_DAP_IDENT,
         PNET_SUBSLOT_DAP_INTERFACE_1_PORT_4_IDENT,
         PNET_MOD_DAP_IDENT,
         PNET_SUBMOD_DAP_INTERFACE_1_PORT_4_IDENT,
         &cfg_dap_data);
   }

   APP_LOG_DEBUG ("Done plugging DAP\n\n");
}

void app_utils_handle_send_application_ready (pnet_t * net, uint32_t arep)
{
   int ret = -1;

   APP_LOG_DEBUG (
      "Application will signal that it is ready for data, for "
      "AREP %u.\n",
      arep);

   ret = pnet_application_ready (net, arep);
   if (ret != 0)
   {
      APP_LOG_ERROR (
         "Error returned when application telling that it is ready for "
         "data. Have you set IOCS or IOPS for all subslots?\n");
   }

   /* When the PLC sends a confirmation to this message, the
      pnet_ccontrol_cnf() callback will be triggered.  */
}

void app_utils_handle_send_alarm_ack (
   pnet_t * net,
   uint32_t arep,
   const pnet_alarm_argument_t * p_alarm_arg)
{
   pnet_pnio_status_t pnio_status = {0, 0, 0, 0};
   int ret;

   ret = pnet_alarm_send_ack (net, arep, p_alarm_arg, &pnio_status);
   if (ret != 0)
   {
      APP_LOG_DEBUG ("Error when sending alarm ACK. Error: %d\n", ret);
   }
   else
   {
      APP_LOG_DEBUG ("Alarm ACK sent\n");
   }
}

void app_utils_handle_cyclic_data (app_data_t * app)
{
   /* For the application cyclic data is updated
    * with a period defined by app->ticks_between_polls
    */
   app->process_data_tick_counter++;
   if (app->process_data_tick_counter < app->ticks_between_polls)
   {
      return;
   }
   app->process_data_tick_counter = 0;

   app_utils_cyclic_data_poll (&app->main_api);
}

void app_utils_loop_forever (void * arg)
{
   app_data_t * app = (app_data_t *)arg;
   uint32_t mask = APP_EVENT_READY_FOR_DATA | APP_EVENT_TIMER |
                   APP_EVENT_ALARM | APP_EVENT_ABORT;
   uint32_t flags = 0;

   app->main_api.arep = UINT32_MAX;

   app_data_init (app->user_data, app);

   app_utils_plug_dap (app, app->pnet_cfg->num_physical_ports);
   APP_LOG_INFO ("Waiting for PLC connect request\n\n");

   /* Main event loop */
   for (;;)
   {
      os_event_wait (app->main_events, mask, &flags, OS_WAIT_FOREVER);
      if (flags & APP_EVENT_READY_FOR_DATA)
      {
         os_event_clr (app->main_events, APP_EVENT_READY_FOR_DATA);

         app_utils_handle_send_application_ready (
            app->net,
            app->arep_for_appl_ready);
      }
      else if (flags & APP_EVENT_ALARM)
      {
         os_event_clr (app->main_events, APP_EVENT_ALARM);

         app_utils_handle_send_alarm_ack (
            app->net,
            app->main_api.arep,
            &app->alarm_arg);
      }
      else if (flags & APP_EVENT_TIMER)
      {
         os_event_clr (app->main_events, APP_EVENT_TIMER);

         app_data_pre (app->user_data, app);

         if (app_utils_is_connected_to_controller (app))
         {
            app_utils_handle_cyclic_data (app);
         }

         /* Run p-net stack */
         pnet_handle_periodic (app->net);
      }
      else if (flags & APP_EVENT_ABORT)
      {
         os_event_clr (app->main_events, APP_EVENT_ABORT);

         app->main_api.arep = UINT32_MAX;
         app->alarm_allowed = true;
         APP_LOG_DEBUG ("Connection closed\n");
         APP_LOG_DEBUG ("Waiting for new PLC connect request\n\n");
      }
   }
}

int app_utils_init (
   app_data_t * app,
   const pnet_cfg_t * pnet_cfg,
   uint32_t ticks_between_polls,
   void * user_data)
{
   APP_LOG_INFO ("Initialise Profinet stack and application\n");
   if (app == NULL)
   {
      return -1;
   }

   app->alarm_allowed = true;
   app->main_api.arep = UINT32_MAX;
   app->pnet_cfg = pnet_cfg;
   app->ticks_between_polls = ticks_between_polls;
   app->user_data = user_data;
   app->net = pnet_init (app->pnet_cfg);

   if (app->net == NULL)
   {
      return -1;
   }

   return 0;
}

int app_utils_start (
   app_data_t * app,
   app_run_in_separate_task_t task_config,
   uint32_t priority,
   uint32_t stacksize,
   uint32_t tick_interval_us)
{
   APP_LOG_INFO ("Start Profinet application main loop\n");
   if (app == NULL)
   {
      return -1;
   }

   app->main_events = os_event_create();
   if (app->main_events == NULL)
   {
      return -1;
   }

   app->main_timer = os_timer_create (
      tick_interval_us,
      app_utils_main_timer_tick,
      (void *)app,
      false);

   if (app->main_timer == NULL)
   {
      os_event_destroy (app->main_events);
      return -1;
   }

   if (task_config == RUN_IN_SEPARATE_THREAD)
   {
      os_thread_create (
         "p-net__app",
         priority,
         stacksize,
         app_utils_loop_forever,
         (void *)app);
   }

   os_timer_start (app->main_timer);

   return 0;
}
