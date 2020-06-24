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

#include "sampleapp_common.h"

#include "log.h"
#include "osal.h"
#include <pnet_api.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Print contents of a buffer
 *
 * @param bytes      In: inputbuffer
 * @param len        In: Number of bytes to print
*/
static void print_bytes(uint8_t *bytes, int32_t len)
{
   printf("  Bytes: ");
   for (int i = 0; i < len; i++)
   {
      printf("%02X ", bytes[i]);
   }
   printf("\n");
}

void print_network_details(
   const char              *interface_name)
{
   os_ethaddr_t            macbuffer;
   char                    ip_string[OS_INET_ADDRSTRLEN];
   char                    netmask_string[OS_INET_ADDRSTRLEN];
   char                    gateway_string[OS_INET_ADDRSTRLEN];
   char                    mac_string[OS_ETH_ADDRSTRLEN];
   char                    hostname_string[OS_HOST_NAME_MAX];

   os_get_macaddress(interface_name, &macbuffer);
   os_mac_to_string(macbuffer, mac_string);
   os_ip_to_string(os_get_ip_address(interface_name), ip_string);
   os_ip_to_string(os_get_netmask(interface_name), netmask_string);
   os_ip_to_string(os_get_gateway(interface_name), gateway_string);
   os_get_hostname(hostname_string);

   printf("Ethernet interface:  %s\n", interface_name);
   printf("Current hostname:    %s\n", hostname_string);
   printf("MAC address:         %s\n", mac_string);
   printf("IP address:          %s\n", ip_string);
   printf("Netmask:             %s\n", netmask_string);
   printf("Gateway:             %s\n\n", gateway_string);
}

/*********************************** Callbacks ********************************/

static int app_connect_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Connect call-back. AREP: %u  Status codes: %d %d %d %d\n",
         arep,
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }
   /*
    *  Handle the request on an application level.
    *  This is a very simple application which does not need to handle anything.
    *  All the needed information is in the AR data structure.
    */

   return 0;
}

static int app_release_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Release (disconnect) call-back. AREP: %u  Status codes: %d %d %d %d\n",
         arep,
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }

   return 0;
}

static int app_dcontrol_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_control_command_t  control_command,
   pnet_result_t           *p_result)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Dcontrol call-back. AREP: %u  Command: %d  Status codes: %d %d %d %d\n",
         arep,
         control_command,
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }

   return 0;
}

static int app_ccontrol_cnf(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_result_t           *p_result)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Ccontrol confirmation call-back. AREP: %u  Status codes: %d %d %d %d\n",
         arep,
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }

   return 0;
}

static int app_write_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                idx,
   uint16_t                sequence_number,
   uint16_t                write_length,
   uint8_t                 *p_write_data,
   pnet_result_t           *p_result)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Parameter write call-back. AREP: %u API: %u Slot: %2u Subslot: %u Index: %u Sequence: %2u Length: %u\n",
         arep,
         api,
         slot,
         subslot,
         (unsigned)idx,
         sequence_number,
         write_length);
   }
   if (idx == APP_PARAM_IDX_1)
   {
      if (write_length == sizeof(p_appdata->app_param_1))
      {
         memcpy(&p_appdata->app_param_1, p_write_data, sizeof(p_appdata->app_param_1));
         if (p_appdata->arguments.verbosity > 0)
         {
            print_bytes(p_write_data, sizeof(p_appdata->app_param_1));
         }
      }
      else
      {
         printf("Wrong length in write call-back. Index: %u Length: %u Expected length: %u\n",
            (unsigned)idx,
            (unsigned)write_length,
            (unsigned)sizeof(p_appdata->app_param_1));
      }
   }
   else if (idx == APP_PARAM_IDX_2)
   {
      if (write_length == sizeof(p_appdata->app_param_2))
      {
         memcpy(&p_appdata->app_param_2, p_write_data, sizeof(p_appdata->app_param_2));
         if (p_appdata->arguments.verbosity > 0)
         {
            print_bytes(p_write_data, sizeof(p_appdata->app_param_2));
         }
      }
      else
      {
         printf("Wrong length in write call-back. Index: %u Length: %u Expected length: %u\n",
            (unsigned)idx,
            (unsigned)write_length,
            (unsigned)sizeof(p_appdata->app_param_2));
      }
   }
   else
   {
      printf("Wrong index in write call-back: %u\n", (unsigned)idx);
   }

   return 0;
}

static int app_read_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                idx,
   uint16_t                sequence_number,
   uint8_t                 **pp_read_data,
   uint16_t                *p_read_length,
   pnet_result_t           *p_result)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Parameter read call-back. AREP: %u API: %u Slot: %2u Subslot: %u Index: %u Sequence: %2u  Max length: %u\n",
         arep,
         api,
         slot,
         subslot,
         (unsigned)idx,
         sequence_number,
         (unsigned)*p_read_length);
   }
   if ((idx == APP_PARAM_IDX_1) &&
       (*p_read_length >= sizeof(p_appdata->app_param_1)))
   {
      *pp_read_data = (uint8_t*)&p_appdata->app_param_1;
      *p_read_length = sizeof(p_appdata->app_param_1);
      if (p_appdata->arguments.verbosity > 0)
      {
         print_bytes(*pp_read_data, *p_read_length);
      }
   }
   if ((idx == APP_PARAM_IDX_2) &&
       (*p_read_length >= sizeof(p_appdata->app_param_2)))
   {
      *pp_read_data = (uint8_t*)&p_appdata->app_param_2;
      *p_read_length = sizeof(p_appdata->app_param_2);
      if (p_appdata->arguments.verbosity > 0)
      {
         print_bytes(*pp_read_data, *p_read_length);
      }
   }

   return 0;
}

static int app_state_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_event_values_t     state)
{
   uint16_t                err_cls = 0;
   uint16_t                err_code = 0;
   uint16_t                slot = 0;
   const char              *error_description = "";

   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Callback on event %s   AREP: %u\n", event_value_to_string(state), arep);
   }

   if (state == PNET_EVENT_ABORT)
   {
      if (pnet_get_ar_error_codes(net, arep, &err_cls, &err_code) == 0)
      {
         if (p_appdata->arguments.verbosity > 0)
         {
               /* A few of the most common error codes */
               switch (err_cls)
               {
               case 0:
                  error_description = "Unknown error class";
                  break;
               case PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL:
                  switch (err_code)
                  {
                  case PNET_ERROR_CODE_2_ABORT_AR_CONSUMER_DHT_EXPIRED:
                     error_description = "AR_CONSUMER_DHT_EXPIRED";
                     break;
                  case PNET_ERROR_CODE_2_ABORT_AR_CMI_TIMEOUT:
                     error_description = "ABORT_AR_CMI_TIMEOUT";
                     break;
                  case PNET_ERROR_CODE_2_ABORT_AR_RELEASE_IND_RECEIVED:
                     error_description = "Controller sent release request.";
                     break;
                  }
                  break;
               }
               printf("    Error class: %u Error code: %u  %s\n",
                  (unsigned)err_cls, (unsigned)err_code, error_description);
         }
      }
      else
      {
         if (p_appdata->arguments.verbosity > 0)
         {
               printf("    No error status available\n");
         }
      }
      /* Only abort AR with correct session key */
      os_event_set(p_appdata->main_events, EVENT_ABORT);
   }
   else if (state == PNET_EVENT_PRMEND)
   {
      /* Save the arep for later use */
      p_appdata->main_arep = arep;
      os_event_set(p_appdata->main_events, EVENT_READY_FOR_DATA);

      /* Set IOPS for DAP slot (has same numbering as the module identifiers) */
      (void)pnet_input_set_data_and_iops(net, APP_API, PNET_SLOT_DAP_IDENT, PNET_SUBMOD_DAP_IDENT,                    NULL, 0, PNET_IOXS_GOOD);
      (void)pnet_input_set_data_and_iops(net, APP_API, PNET_SLOT_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_IDENT,        NULL, 0, PNET_IOXS_GOOD);
      (void)pnet_input_set_data_and_iops(net, APP_API, PNET_SLOT_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_PORT_0_IDENT, NULL, 0, PNET_IOXS_GOOD);

      /* Set initial data and IOPS for custom input modules, and IOCS for custom output modules */
      for (slot = 0; slot < PNET_MAX_MODULES; slot++)
      {
         if (p_appdata->custom_input_slots[slot] == true)
         {
            if (p_appdata->arguments.verbosity > 0)
            {
               printf("  Setting input data and IOPS for slot %2u subslot %u\n", slot, PNET_SUBMOD_CUSTOM_IDENT);
            }
            (void)pnet_input_set_data_and_iops(net, APP_API, slot, PNET_SUBMOD_CUSTOM_IDENT, p_appdata->inputdata,  sizeof(p_appdata->inputdata), PNET_IOXS_GOOD);
         }
         if (p_appdata->custom_output_slots[slot] == true)
         {
            if (p_appdata->arguments.verbosity > 0)
            {
               printf("  Setting output IOCS         for slot %2u subslot %u\n", slot, PNET_SUBMOD_CUSTOM_IDENT);
            }
            (void)pnet_output_set_iocs(net, APP_API, slot, PNET_SUBMOD_CUSTOM_IDENT, PNET_IOXS_GOOD);
         }
      }

      (void)pnet_set_provider_state(net, true);
   }

   return 0;
}

static int app_reset_ind(
   pnet_t                  *net,
   void                    *arg,
   bool                    should_reset_application,
   uint16_t                reset_mode)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Reset call-back. Application reset mandatory: %u  Reset mode: %d\n",
         should_reset_application,
         reset_mode);
   }

   return 0;
}

static int app_signal_led_ind(
   pnet_t                  *net,
   void                    *arg,
   bool                    led_state)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Profinet signal LED call-back. New state: %u\n",
         led_state);
   }
   return app_set_led(APP_PROFINET_SIGNAL_LED_ID, led_state);
}

static int app_exp_module_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                api,
   uint16_t                slot,
   uint32_t                module_ident)
{
   int                     ret = -1;   /* Not supported in specified slot */
   int                     result = 0;
   uint16_t                ix;
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Module plug call-back\n");
   }

   /* Find it in the list of supported modules */
   ix = 0;
   while ((ix < NELEMENTS(cfg_available_module_types)) &&
          (cfg_available_module_types[ix] != module_ident))
   {
      ix++;
   }

   if (ix < NELEMENTS(cfg_available_module_types))
   {
      if (p_appdata->arguments.verbosity > 0)
      {
         printf("  Pull old module.    API: %u Slot: %2u",
            api,
            slot
         );
      }
      result = pnet_pull_module(net, api, slot);
      if (p_appdata->arguments.verbosity > 0)
      {
         if (result != 0)
         {
            printf("    Slot was empty.");
         }
         printf("\n");
      }

      /* For now support any of the known modules in any slot */
      if (p_appdata->arguments.verbosity > 0)
      {
         printf("  Plug module.        API: %u Slot: %2u Module ID: 0x%x Index in supported modules: %u\n", api, slot, (unsigned)module_ident, ix);
      }
      ret = pnet_plug_module(net, api, slot, module_ident);
      if (ret != 0)
      {
         printf("Plug module failed. Ret: %u API: %u Slot: %2u Module ID: 0x%x Index in list of supported modules: %u\n", ret, api, slot, (unsigned)module_ident, ix);
      }
      else
      {
         /* Remember what is plugged in each slot */
         if (slot < PNET_MAX_MODULES)
         {
            if (module_ident == PNET_MOD_8_0_IDENT || module_ident == PNET_MOD_8_8_IDENT)
            {
               p_appdata->custom_input_slots[slot] = true;
            }
            if (module_ident == PNET_MOD_8_8_IDENT || module_ident == PNET_MOD_0_8_IDENT)
            {
               p_appdata->custom_output_slots[slot] = true;
            }
         }
         else
         {
            printf("Wrong slot number recieved: %u  It should be less than %u\n", slot, PNET_MAX_MODULES);
         }
      }

   }
   else
   {
      printf("  Module ID %08x not found. API: %u Slot: %2u\n",
         (unsigned)module_ident,
         api,
         slot);
   }

   return ret;
}

static int app_exp_submodule_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint32_t                module_ident,
   uint32_t                submodule_ident)
{
   int                     ret = -1;
   int                     result = 0;
   uint16_t                ix = 0;
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Submodule plug call-back.\n");
   }

   /* Find it in the list of supported submodules */
   ix = 0;
   while ((ix < NELEMENTS(cfg_available_submodule_types)) &&
          ((cfg_available_submodule_types[ix].module_ident_nbr != module_ident) ||
           (cfg_available_submodule_types[ix].submodule_ident_nbr != submodule_ident)))
   {
      ix++;
   }

   if (ix < NELEMENTS(cfg_available_submodule_types))
   {
      if (p_appdata->arguments.verbosity > 0)
      {
         printf("  Pull old submodule. API: %u Slot: %2u                   Subslot: %u ",
            api,
            slot,
            subslot
         );
      }
      result = pnet_pull_submodule(net, api, slot, subslot);
      if (p_appdata->arguments.verbosity > 0)
      {
         if (result != 0)
         {
            printf("     Subslot was empty.");
         }
         printf("\n");
      }

      if (p_appdata->arguments.verbosity > 0)
      {
         printf("  Plug submodule.     API: %u Slot: %2u Module ID: 0x%-4x Subslot: %u Submodule ID: 0x%x Index in supported submodules: %u Dir: %s In: %u Out: %u bytes\n",
            api,
            slot,
            (unsigned)module_ident,
            subslot,
            (unsigned)submodule_ident,
            ix,
            submodule_direction_to_string(cfg_available_submodule_types[ix].data_dir),
            cfg_available_submodule_types[ix].insize,
            cfg_available_submodule_types[ix].outsize
            );
      }
      ret = pnet_plug_submodule(net, api, slot, subslot,
         module_ident,
         submodule_ident,
         cfg_available_submodule_types[ix].data_dir,
         cfg_available_submodule_types[ix].insize,
         cfg_available_submodule_types[ix].outsize);
      if (ret != 0)
      {
         printf("  Plug submodule failed. Ret: %u API: %u Slot: %2u Subslot %u Module ID: 0x%x Submodule ID: 0x%x Index in list of supported modules: %u\n",
            ret,
            api,
            slot,
            subslot,
            (unsigned)module_ident,
            (unsigned)submodule_ident,
            ix);
      }
   }
   else
   {
      printf("  Submodule ID 0x%x in module ID 0x%x not found. API: %u Slot: %2u Subslot %u \n",
         (unsigned)submodule_ident,
         (unsigned)module_ident,
         api,
         slot,
         subslot);
   }

   return ret;
}

static int app_new_data_status_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   uint32_t                crep,
   uint8_t                 changes,
   uint8_t                 data_status)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("New data status callback. AREP: %u  Status changes: 0x%02x  Status: 0x%02x\n", arep, changes, data_status);
   }

   return 0;
}

static int app_alarm_ind(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                data_len,
   uint16_t                data_usi,
   uint8_t                 *p_data)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Alarm indicated callback. AREP: %u  API: %d  Slot: %d  Subslot: %d  Length: %d  USI: %d",
         arep,
         api,
         slot,
         subslot,
         data_len,
         data_usi);
   }
   os_event_set(p_appdata->main_events, EVENT_ALARM);

   return 0;
}

static int app_alarm_cnf(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_pnio_status_t      *p_pnio_status)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Alarm confirmed (by controller) callback. AREP: %u  Status code %u, %u, %u, %u\n",
         arep,
         p_pnio_status->error_code,
         p_pnio_status->error_decode,
         p_pnio_status->error_code_1,
         p_pnio_status->error_code_2);
   }
   p_appdata->alarm_allowed = true;

   return 0;
}

static int app_alarm_ack_cnf(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   int                     res)
{
   app_data_t              *p_appdata = (app_data_t*)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf("Alarm ACK confirmation (from controller) callback. AREP: %u  Result: %d\n", arep, res);
   }

   return 0;
}

void app_plug_dap(pnet_t *net)
{
   /* Use existing callback functions to plug the (sub-)modules */
   app_exp_module_ind(net, NULL, APP_API , PNET_SLOT_DAP_IDENT, PNET_MOD_DAP_IDENT);

   app_exp_submodule_ind(net, NULL, APP_API, PNET_SLOT_DAP_IDENT, PNET_SUBMOD_DAP_IDENT,
         PNET_MOD_DAP_IDENT, PNET_SUBMOD_DAP_IDENT);
   app_exp_submodule_ind(net, NULL, APP_API, PNET_SLOT_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_IDENT,
         PNET_MOD_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_IDENT);
   app_exp_submodule_ind(net, NULL, APP_API, PNET_SLOT_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_PORT_0_IDENT,
         PNET_MOD_DAP_IDENT, PNET_SUBMOD_DAP_INTERFACE_1_PORT_0_IDENT);
}

/************ Configuration of product ID, software version etc **************/

int app_adjust_stack_configuration(
   pnet_cfg_t              *stack_config)
{
   memset(stack_config, 0, sizeof(pnet_cfg_t));

   /* For clarity, some members are set to 0 even though the entire struct is cleared.
    *
    * Note that these members are set by the sample_app main:
    *    cb_arg
    *    im_0_data.order_id
    *    im_0_data.im_serial_number
    *    eth_addr.addr
    *    ip_addr
    *    ip_gateway
    *    ip_mask
    *    station_name
    */

   /* Call-backs */
   stack_config->state_cb = app_state_ind;
   stack_config->connect_cb = app_connect_ind;
   stack_config->release_cb = app_release_ind;
   stack_config->dcontrol_cb = app_dcontrol_ind;
   stack_config->ccontrol_cb = app_ccontrol_cnf;
   stack_config->read_cb = app_read_ind;
   stack_config->write_cb = app_write_ind;
   stack_config->exp_module_cb = app_exp_module_ind;
   stack_config->exp_submodule_cb = app_exp_submodule_ind;
   stack_config->new_data_status_cb = app_new_data_status_ind;
   stack_config->alarm_ind_cb = app_alarm_ind;
   stack_config->alarm_cnf_cb = app_alarm_cnf;
   stack_config->alarm_ack_cnf_cb = app_alarm_ack_cnf;
   stack_config->reset_cb = app_reset_ind;
   stack_config->signal_led_cb = app_signal_led_ind;

   /* Identification & Maintenance */
   stack_config->im_0_data.vendor_id_hi = 0xfe;
   stack_config->im_0_data.vendor_id_lo = 0xed;
   stack_config->im_0_data.im_hardware_revision = 1;
   stack_config->im_0_data.sw_revision_prefix = 'P'; /* 'V', 'R', 'P', 'U', or 'T' */
   stack_config->im_0_data.im_sw_revision_functional_enhancement = 0;
   stack_config->im_0_data.im_sw_revision_bug_fix = 0;
   stack_config->im_0_data.im_sw_revision_internal_change = 0;
   stack_config->im_0_data.im_revision_counter = 0;  /* Only 0 allowed according to standard */
   stack_config->im_0_data.im_profile_id = 0x1234;
   stack_config->im_0_data.im_profile_specific_type = 0x5678;
   stack_config->im_0_data.im_version_major = 1;
   stack_config->im_0_data.im_version_minor = 1;
   stack_config->im_0_data.im_supported = 0x001e;        /* Only I&M0..I&M4 supported */
   strcpy(stack_config->im_1_data.im_tag_function, "");
   strcpy(stack_config->im_1_data.im_tag_location, "");
   strcpy(stack_config->im_2_data.im_date, "");
   strcpy(stack_config->im_3_data.im_descriptor, "");
   strcpy(stack_config->im_4_data.im_signature, "");  /* For functional safety only */

   /* Device configuration */
   stack_config->device_id.vendor_id_hi = 0xfe;
   stack_config->device_id.vendor_id_lo = 0xed;
   stack_config->device_id.device_id_hi = 0xbe;
   stack_config->device_id.device_id_lo = 0xef;
   stack_config->oem_device_id.vendor_id_hi = 0xc0;
   stack_config->oem_device_id.vendor_id_lo = 0xff;
   stack_config->oem_device_id.device_id_hi = 0xee;
   stack_config->oem_device_id.device_id_lo = 0x01;
   strcpy(stack_config->device_vendor, "rt-labs");
   strcpy(stack_config->manufacturer_specific_string, "PNET demo");

   /* LLDP settings */
   strcpy(stack_config->lldp_cfg.chassis_id, "rt-labs1");
   strcpy(stack_config->lldp_cfg.port_id, "port-001");
   stack_config->lldp_cfg.ttl = 20;                /* seconds */
   stack_config->lldp_cfg.rtclass_2_status = 0;
   stack_config->lldp_cfg.rtclass_3_status = 0;
   stack_config->lldp_cfg.cap_aneg = PNET_LLDP_AUTONEG_SUPPORTED |
                                     PNET_LLDP_AUTONEG_ENABLED;
   stack_config->lldp_cfg.cap_phy = PNET_LLDP_AUTONEG_CAP_100BaseTX_HALF_DUPLEX |
                                    PNET_LLDP_AUTONEG_CAP_100BaseTX_FULL_DUPLEX;
   stack_config->lldp_cfg.mau_type = PNET_MAU_COPPER_100BaseTX_FULL_DUPLEX;

   /* Network configuration */
   stack_config->send_hello = 1;
   stack_config->dhcp_enable = 0;

   return 0;
}

/*************************** Helper functions ********************************/

const char* event_value_to_string(
   pnet_event_values_t event)
{
   const char *s = "<error>";

   switch (event)
   {
   case PNET_EVENT_ABORT:   s = "PNET_EVENT_ABORT"; break;
   case PNET_EVENT_STARTUP: s = "PNET_EVENT_STARTUP"; break;
   case PNET_EVENT_PRMEND:  s = "PNET_EVENT_PRMEND"; break;
   case PNET_EVENT_APPLRDY: s = "PNET_EVENT_APPLRDY"; break;
   case PNET_EVENT_DATA:    s = "PNET_EVENT_DATA"; break;
   }

   return s;
}

const char* submodule_direction_to_string(
   pnet_submodule_dir_t direction)
{
   const char *s = "<error>";

   switch (direction)
   {
   case PNET_DIR_NO_IO:  s = "NO_IO"; break;
   case PNET_DIR_INPUT:  s = "INPUT"; break;
   case PNET_DIR_OUTPUT: s = "OUTPUT"; break;
   case PNET_DIR_IO:     s = "INPUT_OUTPUT"; break;
   }

   return s;
}
