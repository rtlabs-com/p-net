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

#include "pnet_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************ Diagnostic printing ********************************/

/**
 * Print contents of a buffer
 *
 * @param bytes      In: inputbuffer
 * @param len        In: Number of bytes to print
 */
static void print_bytes (const uint8_t * bytes, int32_t len)
{
   printf ("  Bytes: ");
   for (int i = 0; i < len; i++)
   {
      printf ("%02X ", bytes[i]);
   }
   printf ("\n");
}

/**
 * Convert IPv4 address to string
 * @param ip               In:    IP address
 * @param outputstring     Out:   Resulting string. Should have length
 *                                PNAL_INET_ADDRSTRLEN.
 */
static void ip_to_string (pnal_ipaddr_t ip, char * outputstring)
{
   snprintf (
      outputstring,
      PNAL_INET_ADDRSTRLEN,
      "%u.%u.%u.%u",
      (uint8_t) ((ip >> 24) & 0xFF),
      (uint8_t) ((ip >> 16) & 0xFF),
      (uint8_t) ((ip >> 8) & 0xFF),
      (uint8_t) (ip & 0xFF));
}

/**
 * Convert MAC address to string
 * @param mac              In:    MAC address
 * @param outputstring     Out:   Resulting string. Should have length
 *                                PNAL_ETH_ADDRSTRLEN.
 */
static void mac_to_string (pnal_ethaddr_t mac, char * outputstring)
{
   snprintf (
      outputstring,
      PNAL_ETH_ADDRSTRLEN,
      "%02X:%02X:%02X:%02X:%02X:%02X",
      mac.addr[0],
      mac.addr[1],
      mac.addr[2],
      mac.addr[3],
      mac.addr[4],
      mac.addr[5]);
}

/**
 * Return a string representation of the dcontrol command.
 * @param control_command  In:    Command
 * @return  A string representing the command.
 */
static const char * dcontrol_command_to_string (
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

/**
 * Return a string representation of the given event.
 * @param event            In:    The event.
 * @return  A string representing the event.
 */
static const char * event_value_to_string (pnet_event_values_t event)
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

/**
 * Return a string representation of the submodule data direction.
 * @param direction        In:    The direction.
 * @return  A string representing the direction.
 */
static const char * submodule_direction_to_string (
   pnet_submodule_dir_t direction)
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

static const char * ioxs_to_string (pnet_ioxs_values_t ioxs)
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

void app_print_network_details (
   pnal_ethaddr_t * p_macbuffer,
   pnal_ipaddr_t ip,
   pnal_ipaddr_t netmask,
   pnal_ipaddr_t gateway)
{
   char ip_string[PNAL_INET_ADDRSTRLEN];
   char netmask_string[PNAL_INET_ADDRSTRLEN];
   char gateway_string[PNAL_INET_ADDRSTRLEN];
   char mac_string[PNAL_ETH_ADDRSTRLEN];
   char hostname_string[PNAL_HOST_NAME_MAX];

   mac_to_string (*p_macbuffer, mac_string);
   ip_to_string (ip, ip_string);
   ip_to_string (netmask, netmask_string);
   ip_to_string (gateway, gateway_string);
   pnal_get_hostname (hostname_string);

   printf ("MAC address:          %s\n", mac_string);
   printf ("Current hostname:     %s\n", hostname_string);
   printf ("Current IP address:   %s\n", ip_string);
   printf ("Current Netmask:      %s\n", netmask_string);
   printf ("Current Gateway:      %s\n", gateway_string);
}

/**
 * Get subslot application information.
 * @param p_appdata     InOut: Application state.
 * @param slot_nbr      In: Slot number.
 * @param subslot_nbr   In: Subslot number.
 * @return Reference to application subslot,
 *         NULL is subslot is not found/plugged.
 */
static app_subslot_t * app_subslot_get (
   app_data_t * p_appdata,
   uint16_t slot_nbr,
   uint16_t subslot_nbr)
{
   uint16_t subslot;
   for (subslot = 0; subslot < PNET_MAX_SUBSLOTS; subslot++)
   {
      if (
         p_appdata->main_api.slots[slot_nbr].subslots[subslot].subslot_nbr ==
         subslot_nbr)
      {
         return &p_appdata->main_api.slots[slot_nbr].subslots[subslot];
      }
   }
   return NULL;
}

/**
 * Alloc/reserve application subslot.
 * @param p_appdata        InOut: Application state.
 * @param slot_nbr         In: Slot number.
 * @param subslot_nbr      In: Subslot number.
 * @param submodule_ident  In: Submodule identity.
 * @param p_data_cfg       In: Data configuration,
 *                             direction, in and out sizes.
 * @param submodule_name   In: Submodule name
 * @return Reference to allocated subslot,
 *         NULL if no free subslot is available.
 */
static app_subslot_t * app_subslot_alloc (
   app_data_t * p_appdata,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t submodule_ident,
   pnet_data_cfg_t * p_data_cfg,
   const char * submodule_name)
{
   uint16_t subslot;

   if (slot_nbr >= PNET_MAX_SLOTS || p_appdata == NULL || p_data_cfg == NULL)
   {
      return NULL;
   }

   for (subslot = 0; subslot < PNET_MAX_SUBSLOTS; subslot++)
   {
      if (p_appdata->main_api.slots[slot_nbr].subslots[subslot].used == false)
      {
         app_subslot_t * p_subslot =
            &p_appdata->main_api.slots[slot_nbr].subslots[subslot];

         p_subslot->used = true;
         p_subslot->plugged = true;
         p_subslot->slot_nbr = slot_nbr;
         p_subslot->subslot_nbr = subslot_nbr;
         p_subslot->submodule_name = submodule_name;
         p_subslot->submodule_id = submodule_ident;
         p_subslot->p_in_data = NULL;
         p_subslot->data_cfg = *p_data_cfg;
         return p_subslot;
      }
   }
   return NULL;
}

/**
 * Free application subslot.
 * @param p_appdata     InOut: Application state.
 * @param slot_nbr      In: Slot number.
 * @param subslot_nbr   In: Subslot number.
 * @return 0 on success, -1 on error.
 */
static int app_subslot_free (
   app_data_t * p_appdata,
   uint16_t slot_nbr,
   uint16_t subslot_nbr)
{
   app_subslot_t * p_subslot = NULL;

   if (slot_nbr >= PNET_MAX_SUBSLOTS || p_appdata == NULL)
   {
      return -1;
   }

   p_subslot = app_subslot_get (p_appdata, slot_nbr, subslot_nbr);
   if (p_subslot != NULL)
   {
      memset (p_subslot, 0, sizeof (app_subslot_t));
      p_subslot->used = false;
      return 0;
   }

   return -1;
}

/**
 * Return subslot input configuration.
 * @param p_subslot     In: Reference to subslot.
 * @return true if subslot is input or input/output.
 *         false if not.
 */
static bool app_subslot_is_input (app_subslot_t * p_subslot)
{
   if (
      p_subslot != NULL && (p_subslot->data_cfg.data_dir == PNET_DIR_INPUT ||
                            p_subslot->data_cfg.data_dir == PNET_DIR_IO))
   {
      return true;
   }
   else
   {
      return false;
   }
}

/**
 * Return true if subslot is neither input or output.
 * This is applies for DAP submodules/slots
 * @param p_subslot     In: Reference to subslot.
 * @return true if subslot is input or input/output.
 *         false if not.
 */
static bool app_subslot_is_no_io (app_subslot_t * p_subslot)
{
   if (p_subslot != NULL && p_subslot->data_cfg.data_dir == PNET_DIR_NO_IO)
   {
      return true;
   }
   else
   {
      return false;
   }
}

/**
 * Return subslot output configuration.
 * @param p_subslot     In: Reference to subslot.
 * @return true if subslot is output or input/output,
 *         false if not.
 */
static bool app_subslot_is_output (app_subslot_t * p_subslot)
{
   if (
      p_subslot != NULL && (p_subslot->data_cfg.data_dir == PNET_DIR_OUTPUT ||
                            p_subslot->data_cfg.data_dir == PNET_DIR_IO))
   {
      return true;
   }
   else
   {
      return false;
   }
}

/*********************************** Callbacks ********************************/

static int app_connect_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "Connect call-back. AREP: %u  Status codes: %d %d %d %d\n",
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

static int app_release_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "Release (disconnect) call-back. AREP: %u  Status codes: %d %d %d "
         "%d\n",
         arep,
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }

   return 0;
}

static int app_dcontrol_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_control_command_t control_command,
   pnet_result_t * p_result)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "Dcontrol call-back. AREP: %u  Command: %s\n",
         arep,
         dcontrol_command_to_string (control_command));
   }

   return 0;
}

static int app_ccontrol_cnf (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "Ccontrol confirmation call-back. AREP: %u  Status codes: %d %d %d "
         "%d\n",
         arep,
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2);
   }

   return 0;
}

static int app_write_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint16_t write_length,
   const uint8_t * p_write_data,
   pnet_result_t * p_result)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "Parameter write call-back. AREP: %u API: %u Slot: %2u Subslot: %u "
         "Index: %u Sequence: %2u Length: %u\n",
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
      if (write_length == sizeof (p_appdata->app_param_1))
      {
         memcpy (
            &p_appdata->app_param_1,
            p_write_data,
            sizeof (p_appdata->app_param_1));
         if (p_appdata->arguments.verbosity > 0)
         {
            print_bytes (p_write_data, sizeof (p_appdata->app_param_1));
         }
      }
      else
      {
         printf (
            "Wrong length in write call-back. Index: %u Length: %u Expected "
            "length: %u\n",
            (unsigned)idx,
            (unsigned)write_length,
            (unsigned)sizeof (p_appdata->app_param_1));
      }
   }
   else if (idx == APP_PARAM_IDX_2)
   {
      if (write_length == sizeof (p_appdata->app_param_2))
      {
         memcpy (
            &p_appdata->app_param_2,
            p_write_data,
            sizeof (p_appdata->app_param_2));
         if (p_appdata->arguments.verbosity > 0)
         {
            print_bytes (p_write_data, sizeof (p_appdata->app_param_2));
         }
      }
      else
      {
         printf (
            "Wrong length in write call-back. Index: %u Length: %u Expected "
            "length: %u\n",
            (unsigned)idx,
            (unsigned)write_length,
            (unsigned)sizeof (p_appdata->app_param_2));
      }
   }
   else
   {
      printf ("Wrong index in write call-back: %u\n", (unsigned)idx);
   }

   return 0;
}

static int app_read_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint8_t ** pp_read_data,
   uint16_t * p_read_length,
   pnet_result_t * p_result)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "Parameter read call-back. AREP: %u API: %u Slot: %2u Subslot: %u "
         "Index: %u Sequence: %2u  Max length: %u\n",
         arep,
         api,
         slot,
         subslot,
         (unsigned)idx,
         sequence_number,
         (unsigned)*p_read_length);
   }
   if (
      (idx == APP_PARAM_IDX_1) &&
      (*p_read_length >= sizeof (p_appdata->app_param_1)))
   {
      *pp_read_data = (uint8_t *)&p_appdata->app_param_1;
      *p_read_length = sizeof (p_appdata->app_param_1);
      if (p_appdata->arguments.verbosity > 0)
      {
         print_bytes (*pp_read_data, *p_read_length);
      }
   }
   if (
      (idx == APP_PARAM_IDX_2) &&
      (*p_read_length >= sizeof (p_appdata->app_param_2)))
   {
      *pp_read_data = (uint8_t *)&p_appdata->app_param_2;
      *p_read_length = sizeof (p_appdata->app_param_2);
      if (p_appdata->arguments.verbosity > 0)
      {
         print_bytes (*pp_read_data, *p_read_length);
      }
   }

   return 0;
}

/**
 * Set input data, provider and consumer status
 * for a subslot.
 * @param net           InOut: The p-net stack instance.
 * @param p_appdata     In: Application state.
 * @param p_subslot     In: Subslot.
 * @return 0 on success, -1 on error
 */
static int app_set_data_and_ioxs (
   pnet_t * net,
   app_data_t * p_appdata,
   app_subslot_t * p_subslot)
{
   int ret;
   uint8_t iops = PNET_IOXS_GOOD;

   if (p_subslot != NULL)
   {
      if (app_subslot_is_input (p_subslot) || app_subslot_is_no_io (p_subslot))
      {
         if (app_subslot_is_input (p_subslot) && p_subslot->p_in_data == NULL)
         {
            iops = PNET_IOXS_BAD;
         }

         ret = pnet_input_set_data_and_iops (
            net,
            p_appdata->main_api.api_id,
            p_subslot->slot_nbr,
            p_subslot->subslot_nbr,
            p_subslot->p_in_data,
            p_subslot->data_cfg.insize,
            iops);

         /*
          * If a submodule is still plugged but not used in current AR
          * setting the data and iops will fail.
          * This is not a problem.
          * Log message below will only be printed for active submodules.
          */
         if (ret == 0 && p_appdata->arguments.verbosity > 0)
         {
            printf (
               "  Set input data and IOPS for slot %2u subslot %u \"%s\""
               "  size %d %s\n",
               p_subslot->slot_nbr,
               p_subslot->subslot_nbr,
               p_subslot->submodule_name,
               p_subslot->data_cfg.insize,
               ioxs_to_string (iops));
         }
      }

      if (app_subslot_is_output (p_subslot))
      {
         ret = pnet_output_set_iocs (
            net,
            APP_API,
            p_subslot->slot_nbr,
            p_subslot->subslot_nbr,
            PNET_IOXS_GOOD);

         if (ret == 0 && p_appdata->arguments.verbosity > 0)
         {
            printf (
               "  Set output IOCS         for slot %2u subslot %u \"%s\"\n",
               p_subslot->slot_nbr,
               p_subslot->subslot_nbr,
               p_subslot->submodule_name);
         }
      }
   }
   return 0;
}

static int app_state_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_event_values_t state)
{
   uint16_t err_cls = 0;
   uint16_t err_code = 0;
   uint16_t slot = 0;
   uint16_t subslot = 0;
   const char * error_class_description = "";
   const char * error_code_description = "";

   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "Callback on event %s   AREP: %u\n",
         event_value_to_string (state),
         arep);
   }

   if (state == PNET_EVENT_ABORT)
   {
      if (pnet_get_ar_error_codes (net, arep, &err_cls, &err_code) == 0)
      {
         if (p_appdata->arguments.verbosity > 0)
         {
            /* A few of the most common error codes */
            switch (err_cls)
            {
            case PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL:
               error_class_description = "Real-Time Acyclic Protocol";
               switch (err_code)
               {
               case PNET_ERROR_CODE_2_ABORT_AR_CONSUMER_DHT_EXPIRED:
                  error_code_description = "Device missed cyclic data "
                                           "deadline, device terminated AR";
                  break;
               case PNET_ERROR_CODE_2_ABORT_AR_CMI_TIMEOUT:
                  error_code_description = "Communication initialization "
                                           "timeout, device terminated AR";
                  break;
               case PNET_ERROR_CODE_2_ABORT_AR_RELEASE_IND_RECEIVED:
                  error_code_description = "AR release indication received";
                  break;
               case PNET_ERROR_CODE_2_ABORT_DCP_STATION_NAME_CHANGED:
                  error_code_description = "DCP station name changed, "
                                           "device terminated AR";
                  break;
               case PNET_ERROR_CODE_2_ABORT_DCP_RESET_TO_FACTORY:
                  error_code_description = "DCP reset to factory or factory "
                                           "reset, device terminated AR";
                  break;
               }
               break;
            }
            printf (
               "    Error class: 0x%02x %s \n"
               "    Error code:  0x%02x %s \n",
               (unsigned)err_cls,
               error_class_description,
               (unsigned)err_code,
               error_code_description);
         }
      }
      else
      {
         if (p_appdata->arguments.verbosity > 0)
         {
            printf ("    No error status available\n");
         }
      }
      /* Only abort AR with correct session key */
      os_event_set (p_appdata->main_events, EVENT_ABORT);
   }
   else if (state == PNET_EVENT_PRMEND)
   {
      /* Save the arep for later use */
      p_appdata->main_api.arep = arep;
      os_event_set (p_appdata->main_events, EVENT_READY_FOR_DATA);

      /* Set initial data and IOPS for input modules, and IOCS for
       * output modules
       */
      for (slot = 0; slot < PNET_MAX_SLOTS; slot++)
      {
         for (subslot = 0; subslot < PNET_MAX_SUBSLOTS; subslot++)
         {
            if (
               p_appdata->main_api.slots[slot].plugged &&
               p_appdata->main_api.slots[slot].subslots[subslot].plugged)
            {
               app_set_data_and_ioxs (
                  net,
                  p_appdata,
                  &p_appdata->main_api.slots[slot].subslots[subslot]);
            }
         }
      }

      (void)pnet_set_provider_state (net, true);
   }

   return 0;
}

static int app_reset_ind (
   pnet_t * net,
   void * arg,
   bool should_reset_application,
   uint16_t reset_mode)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "Reset call-back. Application reset mandatory: %u  Reset mode: %d\n",
         should_reset_application,
         reset_mode);
   }

   return 0;
}

static int app_signal_led_ind (pnet_t * net, void * arg, bool led_state)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf ("Profinet signal LED call-back. New state: %u\n", led_state);
   }
   return app_set_led (APP_PROFINET_SIGNAL_LED_ID, led_state);
}

static int app_exp_module_ind (
   pnet_t * net,
   void * arg,
   uint32_t api,
   uint16_t slot,
   uint32_t module_ident)
{
   int ret = -1; /* Not supported in specified slot */
   int result = 0;
   uint16_t ix;
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf ("Module plug call-back\n");
   }

   if (slot >= PNET_MAX_SLOTS)
   {
      printf (
         "Wrong slot number recieved: %u  It should be less than %u\n",
         slot,
         PNET_MAX_SLOTS);
      return -1;
   }

   /* Find it in the list of supported modules */
   ix = 0;
   while ((ix < NELEMENTS (cfg_available_module_types)) &&
          (cfg_available_module_types[ix] != module_ident))
   {
      ix++;
   }
   if (ix >= NELEMENTS (cfg_available_module_types))
   {
      printf (
         "  Module ID %08x not found. API: %u Slot: %2u\n",
         (unsigned)module_ident,
         api,
         slot);
      /*
       * Needed to pass Behavior scenario 2
       */
      printf ("Plug expected module anyway\n");
   }

   if (p_appdata->arguments.verbosity > 0)
   {
      printf ("  Pull old module.    API: %u Slot: %2u", api, slot);
   }
   result = pnet_pull_module (net, api, slot);
   if (p_appdata->arguments.verbosity > 0)
   {
      if (result == 0)
      {
         p_appdata->main_api.slots[slot].module_id = 0;
         p_appdata->main_api.slots[slot].plugged = false;
      }
      else
      {
         printf ("    Slot was empty.");
      }
      printf ("\n");
   }

   /* For now support any of the known modules in any slot */
   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "  Plug module.        API: %u Slot: %2u Module ID: 0x%x\n",
         api,
         slot,
         (unsigned)module_ident);
   }
   ret = pnet_plug_module (net, api, slot, module_ident);
   if (ret == 0)
   {
      p_appdata->main_api.slots[slot].module_id = module_ident;
      p_appdata->main_api.slots[slot].plugged = true;
   }
   else
   {
      printf (
         "Plug module failed. Ret: %u API: %u Slot: %2u Module ID: 0x%x\n",
         ret,
         api,
         slot,
         (unsigned)module_ident);
   }

   return ret;
}

static int app_exp_submodule_ind (
   pnet_t * net,
   void * arg,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint32_t module_ident,
   uint32_t submodule_ident,
   const pnet_data_cfg_t * p_exp_data)
{
   int ret = -1;
   int result = 0;
   uint16_t ix = 0;
   pnet_data_cfg_t data_cfg = {0};
   app_data_t * p_appdata = (app_data_t *)arg;
   uint8_t * p_app_data = NULL;
   app_subslot_t * p_subslot = NULL;
   const char * name = "Unsupported";

   if (p_appdata->arguments.verbosity > 0)
   {
      printf ("Submodule plug call-back.\n");
   }

   /* Find it in the list of supported submodules */
   ix = 0;
   while (
      (ix < NELEMENTS (cfg_available_submodule_types)) &&
      ((cfg_available_submodule_types[ix].module_ident_nbr != module_ident) ||
       (cfg_available_submodule_types[ix].submodule_ident_nbr !=
        submodule_ident)))
   {
      ix++;
   }

   if (ix < NELEMENTS (cfg_available_submodule_types))
   {
      data_cfg.data_dir = cfg_available_submodule_types[ix].data_dir;
      data_cfg.insize = cfg_available_submodule_types[ix].insize;
      data_cfg.outsize = cfg_available_submodule_types[ix].outsize;
      if (data_cfg.insize > 0)
      {
         p_app_data = p_appdata->inputdata;
      }
      name = cfg_available_submodule_types[ix].name;
   }
   else
   {
      printf (
         "  Submodule ID 0x%x in module ID 0x%x not found. API: %u Slot: %2u "
         "Subslot %u \n",
         (unsigned)submodule_ident,
         (unsigned)module_ident,
         api,
         slot,
         subslot);

      /*
       * Needed for behavior scenario 2 to pass.
       * Iops will be set to bad for this subslot
       */
      printf ("  Plug expected submodule anyway \n");

      data_cfg.data_dir = p_exp_data->data_dir;
      data_cfg.insize = p_exp_data->insize;
      data_cfg.outsize = p_exp_data->outsize;
   }

   {
      if (p_appdata->arguments.verbosity > 0)
      {
         printf (
            "  Pull old submodule. API: %u Slot: %2u                   "
            "Subslot: %u ",
            api,
            slot,
            subslot);
      }
      result = pnet_pull_submodule (net, api, slot, subslot);
      if (result == 0)
      {
         app_subslot_free (p_appdata, slot, subslot);
      }
      if (p_appdata->arguments.verbosity > 0)
      {
         if (result != 0)
         {
            printf ("     Subslot was empty.");
         }
         printf ("\n");
      }

      if (p_appdata->arguments.verbosity > 0)
      {
         printf (
            "  Plug submodule.     API: %u Slot: %2u Module ID: 0x%-4x "
            "Subslot: %u Submodule ID: 0x%x \"%s\"\n"
            "",
            api,
            slot,
            (unsigned)module_ident,
            subslot,
            (unsigned)submodule_ident,
            name);
         printf (
            "                      Data Dir: %s In: %u Out: %u "
            "(Exp Data Dir: %s In: %u Out: %u)\n",
            submodule_direction_to_string (data_cfg.data_dir),
            data_cfg.insize,
            data_cfg.outsize,
            submodule_direction_to_string (p_exp_data->data_dir),
            p_exp_data->insize,
            p_exp_data->outsize);
      }
      ret = pnet_plug_submodule (
         net,
         api,
         slot,
         subslot,
         module_ident,
         submodule_ident,
         data_cfg.data_dir,
         data_cfg.insize,
         data_cfg.outsize);
      if (ret == 0)
      {
         p_subslot = app_subslot_alloc (
            p_appdata,
            slot,
            subslot,
            submodule_ident,
            &data_cfg,
            name);
         if (p_subslot)
         {
            p_subslot->p_in_data = p_app_data;
         }
      }
      else
      {
         printf (
            "  Plug submodule failed. Ret: %u API: %u Slot: %2u Subslot %u "
            "Module ID: 0x%x Submodule ID: 0x%x \n",
            ret,
            api,
            slot,
            subslot,
            (unsigned)module_ident,
            (unsigned)submodule_ident);
      }
   }

   return ret;
}

static int app_new_data_status_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t crep,
   uint8_t changes,
   uint8_t data_status)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "New data status callback. AREP: %u  Data status changes: 0x%02x  "
         "Data status: 0x%02x\n",
         arep,
         changes,
         data_status);
   }

   return 0;
}

static int app_alarm_ind (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   const pnet_alarm_argument_t * p_alarm_arg,
   uint16_t data_len,
   uint16_t data_usi,
   const uint8_t * p_data)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "Alarm indicated callback. AREP: %u API: %d Slot: %d Subslot: %d "
         "Type: %d Seq: %d Length: %d USI: %d\n",
         arep,
         p_alarm_arg->api_id,
         p_alarm_arg->slot_nbr,
         p_alarm_arg->subslot_nbr,
         p_alarm_arg->alarm_type,
         p_alarm_arg->sequence_number,
         data_len,
         data_usi);
   }
   p_appdata->alarm_arg = *p_alarm_arg;
   os_event_set (p_appdata->main_events, EVENT_ALARM);

   return 0;
}

static int app_alarm_cnf (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   const pnet_pnio_status_t * p_pnio_status)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "Alarm confirmed (by controller) callback. AREP: %u  Status code %u, "
         "%u, %u, %u\n",
         arep,
         p_pnio_status->error_code,
         p_pnio_status->error_decode,
         p_pnio_status->error_code_1,
         p_pnio_status->error_code_2);
   }
   p_appdata->alarm_allowed = true;

   return 0;
}

static int app_alarm_ack_cnf (pnet_t * net, void * arg, uint32_t arep, int res)
{
   app_data_t * p_appdata = (app_data_t *)arg;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf (
         "Alarm ACK confirmation (from controller) callback. AREP: %u  Result: "
         "%d\n",
         arep,
         res);
   }

   return 0;
}

void app_plug_dap (pnet_t * net, void * arg)
{
   const pnet_data_cfg_t cfg_dap_data = {
      .data_dir = PNET_DIR_NO_IO,
      .insize = 0,
      .outsize = 0,
   };

   /*
    * Use existing callback functions to plug the (sub-)modules */
   /* Submodule callback requires an expected data configuration.
    * cfg_dap_data defines the expected configuration for DAP
    * submodules
    */
   app_exp_module_ind (
      net,
      arg,
      APP_API,
      PNET_SLOT_DAP_IDENT,
      PNET_MOD_DAP_IDENT);

   app_exp_submodule_ind (
      net,
      arg,
      APP_API,
      PNET_SLOT_DAP_IDENT,
      PNET_SUBSLOT_DAP_IDENT,
      PNET_MOD_DAP_IDENT,
      PNET_SUBMOD_DAP_IDENT,
      &cfg_dap_data);
   app_exp_submodule_ind (
      net,
      arg,
      APP_API,
      PNET_SLOT_DAP_IDENT,
      PNET_SUBSLOT_DAP_INTERFACE_1_IDENT,
      PNET_MOD_DAP_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_IDENT,
      &cfg_dap_data);
   app_exp_submodule_ind (
      net,
      arg,
      APP_API,
      PNET_SLOT_DAP_IDENT,
      PNET_SUBSLOT_DAP_INTERFACE_1_PORT_1_IDENT,
      PNET_MOD_DAP_IDENT,
      PNET_SUBMOD_DAP_INTERFACE_1_PORT_1_IDENT,
      &cfg_dap_data);
}

/************ Configuration of product ID, software version etc **************/

int app_adjust_stack_configuration (pnet_cfg_t * stack_config)
{
   memset (stack_config, 0, sizeof (pnet_cfg_t));

   /* For clarity, some members are set to 0 even though the entire struct is
    * cleared.
    *
    * Note that these members are set by the sample_app main:
    *    cb_arg
    *    eth_addr.addr
    *    ip_addr
    *    ip_gateway
    *    ip_mask
    *    station_name
    *    file_directory
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
   stack_config->im_0_data.im_vendor_id_hi = 0xfe;
   stack_config->im_0_data.im_vendor_id_lo = 0xed;
   stack_config->im_0_data.im_hardware_revision = 1;
   stack_config->im_0_data.im_sw_revision_prefix = 'V'; /* 'V', 'R', 'P', 'U',
                                                           or 'T' */
   stack_config->im_0_data.im_sw_revision_functional_enhancement = 0;
   stack_config->im_0_data.im_sw_revision_bug_fix = 0;
   stack_config->im_0_data.im_sw_revision_internal_change = 0;
   stack_config->im_0_data.im_revision_counter = 0; /* Only 0 allowed according
                                                       to standard */
   stack_config->im_0_data.im_profile_id = 0x1234;
   stack_config->im_0_data.im_profile_specific_type = 0x5678;
   stack_config->im_0_data.im_version_major = 1;
   stack_config->im_0_data.im_version_minor = 1;
   stack_config->im_0_data.im_supported =
      PNET_SUPPORTED_IM1 | PNET_SUPPORTED_IM2 | PNET_SUPPORTED_IM3;
   strcpy (stack_config->im_0_data.im_order_id, "12345");
   strcpy (stack_config->im_0_data.im_serial_number, "00001");
   strcpy (stack_config->im_1_data.im_tag_function, "my function");
   strcpy (stack_config->im_1_data.im_tag_location, "my location");
   strcpy (stack_config->im_2_data.im_date, "2020-09-03 13:53");
   strcpy (stack_config->im_3_data.im_descriptor, "my descriptor");
   strcpy (stack_config->im_4_data.im_signature, ""); /* Functional safety */

   /* Device configuration */
   stack_config->device_id.vendor_id_hi = 0xfe;
   stack_config->device_id.vendor_id_lo = 0xed;
   stack_config->device_id.device_id_hi = 0xbe;
   stack_config->device_id.device_id_lo = 0xef;
   stack_config->oem_device_id.vendor_id_hi = 0xc0;
   stack_config->oem_device_id.vendor_id_lo = 0xff;
   stack_config->oem_device_id.device_id_hi = 0xee;
   stack_config->oem_device_id.device_id_lo = 0x01;
   strcpy (stack_config->manufacturer_specific_string, "PNET demo");
   strcpy (stack_config->product_name, "rt-labs PNET demo");

   /* Timing */
   stack_config->min_device_interval = 32; /* Corresponds to 1 ms */

   /* LLDP settings */
   strcpy (stack_config->lldp_cfg.ports[0].port_id, "port-001");
   stack_config->lldp_cfg.ports[0].rtclass_2_status = 0;
   stack_config->lldp_cfg.ports[0].rtclass_3_status = 0;
   stack_config->lldp_cfg.ports[0].cap_aneg = PNET_LLDP_AUTONEG_SUPPORTED |
                                              PNET_LLDP_AUTONEG_ENABLED;
   stack_config->lldp_cfg.ports[0].cap_phy =
      PNET_LLDP_AUTONEG_CAP_100BaseTX_HALF_DUPLEX |
      PNET_LLDP_AUTONEG_CAP_100BaseTX_FULL_DUPLEX;
   stack_config->lldp_cfg.ports[0].mau_type =
      PNET_MAU_COPPER_100BaseTX_FULL_DUPLEX;

   /* Network configuration */
   stack_config->send_hello = true;
   stack_config->dhcp_enable = false;

   return 0;
}

/*************************** Helper functions ********************************/

void app_copy_ip_to_struct (
   pnet_cfg_ip_addr_t * destination_struct,
   pnal_ipaddr_t ip)
{
   destination_struct->a = ((ip >> 24) & 0xFF);
   destination_struct->b = ((ip >> 16) & 0xFF);
   destination_struct->c = ((ip >> 8) & 0xFF);
   destination_struct->d = (ip & 0xFF);
}

/**
 * Send application ready to the controller
 *
 * @param net              InOut: p-net stack instance
 * @param arep             In:    Arep
 * @param verbosity        In:    Verbosity
 */
static void app_handle_send_application_ready (
   pnet_t * net,
   uint32_t arep,
   int verbosity)
{
   int ret = -1;

   if (verbosity > 0)
   {
      printf ("Application will signal that it is ready for data.\n");
   }

   ret = pnet_application_ready (net, arep);
   if (ret != 0)
   {
      printf ("Error returned when application telling that it is ready for "
              "data. Have you set IOCS or IOPS for all subslots?\n");
   }

   /*
    * cm_ccontrol_cnf(+/-) is indicated later (app_state_ind(DATA)), when the
    * confirmation arrives from the controller.
    */
}

/**
 * Send alarm ACK to the controller
 *
 * @param net              InOut: p-net stack instance
 * @param arep             In:    Arep
 * @param verbosity        In:    Verbosity
 * @param p_alarm_arg      In:    Alarm argument (slot, subslot etc)
 */
static void app_handle_send_alarm_ack (
   pnet_t * net,
   uint32_t arep,
   int verbosity,
   pnet_alarm_argument_t * p_alarm_arg)
{
   pnet_pnio_status_t pnio_status = {0, 0, 0, 0};
   int ret = -1;

   ret = pnet_alarm_send_ack (net, arep, p_alarm_arg, &pnio_status);
   if (ret != 0)
   {
      printf ("Error when sending alarm ACK. Error: %d\n", ret);
   }
   else if (verbosity > 0)
   {
      printf ("Alarm ACK sent\n");
   }
}

/**
 * Send and receive cyclic data
 *
 * The payload is built from the binary value of a button input,
 * and the value in a counter.
 *
 * @param net              InOut: p-net stack instance
 * @param verbosity        In:    Verbosity
 * @param button_pressed   In:    True if button is pressed.
 * @param data_ctr         In:    Data counter.
 * @param p_input_slots    In:    Array describing if there is a plugged input
 *                                module in corresponding slot.
 * @param p_output_slots   In:    Array describing if there is a plugged output
 *                                module in corresponding slot.
 * @param p_inputdata      InOut: Inputdata for sending to the controller. Will
 *                                be modified by this function.
 * @param inputdata_size   In:    Size of inputdata. Should be > 0.
 */
static void app_handle_cyclic_data (
   pnet_t * net,
   app_data_t * p_appdata,
   bool button_pressed,
   uint8_t data_ctr,
   uint8_t * p_inputdata,
   uint16_t inputdata_size)
{
   uint16_t slot = 0;
   uint16_t subslot = 0;
   uint8_t inputdata_iocs = PNET_IOXS_BAD;  /* Consumer status from PLC */
   uint8_t outputdata_iops = PNET_IOXS_BAD; /* Producer status from PLC */
   uint8_t outputdata[APP_DATASIZE_OUTPUT];
   uint16_t outputdata_length = 0;
   uint8_t iops = PNET_IOXS_BAD;
   app_subslot_t * p_subslot = NULL;
   bool outputdata_is_updated = false; /* Not used in this application */
   bool received_led_state = false;    /* LED for cyclic data */
   static bool previous_led_state = false;

   /* Prepare input data (for sending to IO-controller) */
   /* Lowest 7 bits: Counter    Most significant bit: Button */
   p_inputdata[0] = data_ctr;
   if (button_pressed)
   {
      p_inputdata[0] |= 0x80;
   }
   else
   {
      p_inputdata[0] &= 0x7F;
   }

   /* Set data for custom input modules, if any */
   for (slot = 0; slot < PNET_MAX_SLOTS; slot++)
   {
      for (subslot = 0; subslot < PNET_MAX_SUBSLOTS; subslot++)
      {
         iops = PNET_IOXS_BAD;
         p_subslot = &p_appdata->main_api.slots[slot].subslots[subslot];

         if (p_subslot->plugged && app_subslot_is_input (p_subslot))
         {
            if (p_subslot->p_in_data != NULL)
            {
               iops = PNET_IOXS_GOOD;
            }

            (void)pnet_input_set_data_and_iops (
               net,
               APP_API,
               slot,
               p_subslot->subslot_nbr,
               p_inputdata,
               inputdata_size,
               iops);

            (void)pnet_input_get_iocs (
               net,
               APP_API,
               slot,
               p_subslot->subslot_nbr,
               &inputdata_iocs);

            if (p_appdata->arguments.verbosity > 0)
            {
               if (inputdata_iocs == PNET_IOXS_BAD)
               {
                  printf ("The controller reports IOCS_BAD for slot %u\n", slot);
               }
               else if (inputdata_iocs != PNET_IOXS_GOOD)
               {
                  printf (
                     "The controller reports IOCS %u for slot %u\n",
                     inputdata_iocs,
                     slot);
               }
            }
         }

         if (p_subslot->plugged && app_subslot_is_output (p_subslot))
         {
            outputdata_length = sizeof (outputdata);
            pnet_output_get_data_and_iops (
               net,
               APP_API,
               slot,
               p_subslot->subslot_nbr,
               &outputdata_is_updated,
               outputdata,
               &outputdata_length,
               &outputdata_iops);

            /* Set LED state */
            if (
               outputdata_length == APP_DATASIZE_OUTPUT &&
               outputdata_iops == PNET_IOXS_GOOD)
            {
               if (outputdata_length == APP_DATASIZE_OUTPUT)
               {
                  /* Extract LED state from most significant bit */
                  received_led_state = (outputdata[0] & 0x80) > 0;
                  /* Set LED state */
                  if (received_led_state != previous_led_state)
                  {
                     app_set_led (APP_DATA_LED_ID, received_led_state);
                  }
                  previous_led_state = received_led_state;
               }
               else
               {
                  printf (
                     "Wrong outputdata length: %u or IOPS: %u\n",
                     outputdata_length,
                     outputdata_iops);
               }
            }
         }
      }
   }
}

/**
 * Set alarm, diagnostics and logbook entries.
 *
 * Alternates between these functions each time the button2 is pressed:
 *  - pnet_alarm_send_process_alarm()
 *  - pnet_diag_std_add()
 *  - pnet_diag_std_update()
 *  - pnet_diag_usi_add()
 *  - pnet_diag_usi_update()
 *  - pnet_diag_usi_remove()
 *  - pnet_diag_std_remove()
 *  - pnet_create_log_book_entry()
 *
 * Uses first input module, if available.
 *
 * @param net                    InOut: p-net stack instance
 * @param arep                   In:    Arep
 * @param button_pressed         In:    True if button is pressed.
 * @param button_pressed_prev    In:    True if button was pressed last time.
 * @param alarm_allowed          InOut: True if alarm can be sent, false if
 *                                      waiting for alarm ACK.
 * @param data_ctr               In:    Data counter.
 * @param p_input_slots          In:    Array describing if there is a plugged
 *                                      input module in corresponding slot.
 * @param alarm_payload          InOut: Alarm payload for sending to the
 *                                      controller. Will be modified by this
 *                                      function.
 */
static void app_handle_send_alarm (
   pnet_t * net,
   uint32_t arep,
   bool button_pressed,
   bool button_pressed_prev,
   bool * alarm_allowed,
   app_data_t * p_appdata,
   uint8_t * alarm_payload)
{
   static app_demo_state_t state = APP_DEMO_STATE_ALARM_SEND;
   uint16_t slot = 0;
   bool found_inputslot = false;
   uint16_t subslot_array_index = 0;
   app_subslot_t * p_subslot = NULL;
   pnet_pnio_status_t pnio_status = {0};
   pnet_diag_source_t diag_source = {
      .api = APP_API,
      .slot = 0,
      .subslot = 0,
      .ch = APP_DIAG_CHANNEL_NUMBER,
      .ch_grouping = PNET_DIAG_CH_INDIVIDUAL_CHANNEL,
      .ch_direction = APP_DIAG_CHANNEL_DIRECTION};

   /* Trigger only just when the button is pressed */
   if ((button_pressed == false) || (button_pressed_prev == true))
   {
      return;
   }

   /* Look for first input slot */
   while (!found_inputslot && (slot < PNET_MAX_SLOTS))
   {
      for (subslot_array_index = 0;
           !found_inputslot && (subslot_array_index < PNET_MAX_SUBSLOTS);
           subslot_array_index++)
      {
         p_subslot =
            &p_appdata->main_api.slots[slot].subslots[subslot_array_index];
         if (app_subslot_is_input (p_subslot))
         {
            found_inputslot = true;
            break;
         }
      }
      if (!found_inputslot)
      {
         slot++;
      }
   }
   if (!found_inputslot)
   {
      printf ("Did not find any input module in the slots. Skipping sending "
              "demo alarm.\n");
      return;
   }

   diag_source.slot = slot;
   diag_source.subslot = p_subslot->subslot_nbr;

   switch (state)
   {
   case APP_DEMO_STATE_ALARM_SEND:
      if (*alarm_allowed == true)
      {
         alarm_payload[0]++;
         printf (
            "Sending process alarm from slot %u subslot %u USI %u to "
            "IO-controller. Payload: 0x%x\n",
            slot,
            p_subslot->subslot_nbr,
            APP_ALARM_USI,
            alarm_payload[0]);
         pnet_alarm_send_process_alarm (
            net,
            arep,
            APP_API,
            slot,
            p_subslot->subslot_nbr,
            APP_ALARM_USI,
            APP_ALARM_PAYLOAD_SIZE,
            alarm_payload);
         *alarm_allowed = false; /* Not allowed until ACK received */
      }
      else
      {
         printf ("Could not send process alarm, as alarm_allowed == false\n");
      }
      break;

   case APP_DEMO_STATE_DIAG_STD_ADD:
      printf (
         "Adding standard diagnosis. Slot %u subslot %u channel %u\n",
         diag_source.slot,
         diag_source.subslot,
         diag_source.ch);
      pnet_diag_std_add (
         net,
         &diag_source,
         APP_DIAG_CHANNEL_NUMBER_OF_BITS,
         PNET_DIAG_CH_PROP_MAINT_QUALIFIED,
         CHANNEL_ERRORTYPE_NETWORK_COMPONENT_FUNCTION_MISMATCH,
         EXTENDED_CHANNEL_ERRORTYPE_FRAME_DROPPED,
         123, /* Number of dropped frames */
         APP_DIAG_QUAL_SEVERITY);
      break;

   case APP_DEMO_STATE_DIAG_STD_UPDATE:
      printf (
         "Updating standard diagnosis. Slot %u subslot %u channel %u\n",
         diag_source.slot,
         diag_source.subslot,
         diag_source.ch);
      pnet_diag_std_update (
         net,
         &diag_source,
         CHANNEL_ERRORTYPE_NETWORK_COMPONENT_FUNCTION_MISMATCH,
         EXTENDED_CHANNEL_ERRORTYPE_FRAME_DROPPED,
         1234 /* Number of dropped frames */
      );
      break;

   case APP_DEMO_STATE_DIAG_STD_REMOVE:
      printf (
         "Removing standard diagnosis. Slot %u subslot %u channel %u\n",
         diag_source.slot,
         diag_source.subslot,
         diag_source.ch);
      pnet_diag_std_remove (
         net,
         &diag_source,
         CHANNEL_ERRORTYPE_NETWORK_COMPONENT_FUNCTION_MISMATCH,
         EXTENDED_CHANNEL_ERRORTYPE_FRAME_DROPPED);
      break;

   case APP_DEMO_STATE_DIAG_USI_ADD:
      printf (
         "Adding USI diagnosis. Slot %u subslot %u\n",
         slot,
         p_subslot->subslot_nbr);
      pnet_diag_usi_add (
         net,
         APP_API,
         slot,
         p_subslot->subslot_nbr,
         APP_DIAG_CUSTOM_USI,
         (uint8_t *)"diagdata_1");
      break;

   case APP_DEMO_STATE_DIAG_USI_UPDATE:
      printf (
         "Updating USI diagnosis. Slot %u subslot %u\n",
         slot,
         p_subslot->subslot_nbr);
      pnet_diag_usi_add (
         net,
         APP_API,
         slot,
         p_subslot->subslot_nbr,
         APP_DIAG_CUSTOM_USI,
         (uint8_t *)"diagdata_2");
      break;

   case APP_DEMO_STATE_DIAG_USI_REMOVE:
      printf (
         "Removing USI diagnosis. Slot %u subslot %u\n",
         slot,
         p_subslot->subslot_nbr);
      pnet_diag_usi_remove (
         net,
         APP_API,
         slot,
         p_subslot->subslot_nbr,
         APP_DIAG_CUSTOM_USI);
      break;

   case APP_DEMO_STATE_LOGBOOK_ENTRY:
      printf (
         "Writing to logbook. Error_code1: %02X Error_code2: %02X  Entry "
         "detail: 0x%08X\n",
         APP_LOGBOOK_ERROR_CODE_1,
         APP_LOGBOOK_ERROR_CODE_2,
         APP_LOGBOOK_ENTRY_DETAIL);
      pnio_status.error_code = APP_LOGBOOK_ERROR_CODE;
      pnio_status.error_decode = APP_LOGBOOK_ERROR_DECODE;
      pnio_status.error_code_1 = APP_LOGBOOK_ERROR_CODE_1;
      pnio_status.error_code_2 = APP_LOGBOOK_ERROR_CODE_2;
      pnet_create_log_book_entry (
         net,
         arep,
         &pnio_status,
         APP_LOGBOOK_ENTRY_DETAIL);
      break;
   }

   switch (state)
   {
   case APP_DEMO_STATE_ALARM_SEND:
      state = APP_DEMO_STATE_DIAG_STD_ADD;
      break;
   case APP_DEMO_STATE_DIAG_STD_ADD:
      state = APP_DEMO_STATE_DIAG_STD_UPDATE;
      break;
   case APP_DEMO_STATE_DIAG_STD_UPDATE:
      state = APP_DEMO_STATE_DIAG_USI_ADD;
      break;
   case APP_DEMO_STATE_DIAG_USI_ADD:
      state = APP_DEMO_STATE_DIAG_USI_UPDATE;
      break;
   case APP_DEMO_STATE_DIAG_USI_UPDATE:
      state = APP_DEMO_STATE_DIAG_USI_REMOVE;
      break;
   case APP_DEMO_STATE_DIAG_USI_REMOVE:
      state = APP_DEMO_STATE_DIAG_STD_REMOVE;
      break;
   case APP_DEMO_STATE_DIAG_STD_REMOVE:
      state = APP_DEMO_STATE_LOGBOOK_ENTRY;
      break;
   default:
   case APP_DEMO_STATE_LOGBOOK_ENTRY:
      state = APP_DEMO_STATE_ALARM_SEND;
      break;
   }
}

void app_loop_forever (pnet_t * net, app_data_t * p_appdata)
{
   uint32_t mask = EVENT_READY_FOR_DATA | EVENT_TIMER | EVENT_ALARM |
                   EVENT_ABORT;
   uint32_t flags = 0;
   bool button1_pressed = false;
   bool button2_pressed = false;
   bool button2_pressed_previous = false;
   uint32_t tick_ctr_buttons = 0;
   uint32_t tick_ctr_update_data = 0;
   static uint8_t data_ctr = 0;
   uint8_t alarm_payload[APP_ALARM_PAYLOAD_SIZE] = {0};
   p_appdata->main_api.arep = UINT32_MAX;

   if (p_appdata->arguments.verbosity > 0)
   {
      printf ("Waiting for connect request from IO-controller\n\n");
   }

   /* Main loop */
   for (;;)
   {
      os_event_wait (p_appdata->main_events, mask, &flags, OS_WAIT_FOREVER);
      if (flags & EVENT_READY_FOR_DATA)
      {
         os_event_clr (p_appdata->main_events, EVENT_READY_FOR_DATA);

         app_handle_send_application_ready (
            net,
            p_appdata->main_api.arep,
            p_appdata->arguments.verbosity);
      }
      else if (flags & EVENT_ALARM)
      {
         os_event_clr (p_appdata->main_events, EVENT_ALARM); /* Re-arm */

         app_handle_send_alarm_ack (
            net,
            p_appdata->main_api.arep,
            p_appdata->arguments.verbosity,
            &p_appdata->alarm_arg);
      }
      else if (flags & EVENT_TIMER)
      {
         os_event_clr (p_appdata->main_events, EVENT_TIMER); /* Re-arm */
         tick_ctr_buttons++;
         tick_ctr_update_data++;

         /* Read buttons */
         if (
            (p_appdata->main_api.arep != UINT32_MAX) &&
            (tick_ctr_buttons > APP_TICKS_READ_BUTTONS))
         {
            tick_ctr_buttons = 0;

            app_get_button (p_appdata, 0, &button1_pressed);
            app_get_button (p_appdata, 1, &button2_pressed);
         }

         /* Set input and output data */
         if (
            (p_appdata->main_api.arep != UINT32_MAX) &&
            (tick_ctr_update_data > APP_TICKS_UPDATE_DATA))
         {
            tick_ctr_update_data = 0;

            app_handle_cyclic_data (
               net,
               p_appdata,
               button1_pressed,
               data_ctr++,
               p_appdata->inputdata,
               sizeof (p_appdata->inputdata));
         }

         /* Create alarm on first input slot (if any) when button2 is pressed */
         if (p_appdata->main_api.arep != UINT32_MAX)
         {
            app_handle_send_alarm (
               net,
               p_appdata->main_api.arep,
               button2_pressed,
               button2_pressed_previous,
               &p_appdata->alarm_allowed,
               p_appdata,
               alarm_payload);
            button2_pressed_previous = button2_pressed;
         }

         pnet_handle_periodic (net);
      }
      else if (flags & EVENT_ABORT)
      {
         os_event_clr (p_appdata->main_events, EVENT_ABORT); /* Re-arm */

         if (p_appdata->arguments.verbosity > 0)
         {
            printf ("Aborting the application\n\n");
         }

         /* Reset main */
         p_appdata->main_api.arep = UINT32_MAX;
         p_appdata->alarm_allowed = true;
         button1_pressed = false;
         button2_pressed = false;
      }
   }
}
