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
 * @brief Implements the Context Management Ip and Name Assignment protocol
 * machine (CMINA)
 *
 * Handles assignment of station name and IP address.
 * Manages sending DCP hello requests.
 *
 * Resets the communication stack and user application.
 *
 * States are SETUP, SET_NAME, SET_IP and W_CONNECT.
 *
 * Implements setters and getters for net->cmina_current_dcp_ase, so other
 * parts of the stack don't need to know these internal details.
 *
 */

#ifdef UNIT_TEST
#define pnal_get_port_statistics mock_pnal_get_port_statistics
#define pnal_set_ip_suite        mock_pnal_set_ip_suite
#define pf_bg_worker_start_job   mock_pf_bg_worker_start_job
#endif

#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "pf_includes.h"

/**
 * @internal
 * Trigger sending a DCP HELLO message.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * If this was not the last requested HELLO message then re-schedule another
 * call after 1 s.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    Not used.
 * @param current_time     In:    Not used.
 */
static void pf_cmina_send_hello (pnet_t * net, void * arg, uint32_t current_time)
{
   if (
      (net->cmina_state == PF_CMINA_STATE_W_CONNECT) &&
      (net->cmina_hello_count > 0))
   {
      (void)pf_dcp_hello_req (net);

      /* Reschedule */
      net->cmina_hello_count--;
      (void)pf_scheduler_add (
         net,
         PF_CMINA_FS_HELLO_INTERVAL * 1000,
         pf_cmina_send_hello,
         NULL,
         &net->cmina_hello_timeout);
   }
   else
   {
      pf_scheduler_reset_handle (&net->cmina_hello_timeout);
      net->cmina_hello_count = 0;
   }
}

void pf_cmina_save_ase (pnet_t * net, pf_cmina_dcp_ase_t * p_ase)
{
   pf_cmina_dcp_ase_t ase_nvm;
   pf_cmina_dcp_ase_t temporary_buffer;
   char ip_string[PNAL_INET_ADDRSTR_SIZE] = {0};      /** Terminated string */
   char netmask_string[PNAL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   char gateway_string[PNAL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   const char * p_file_directory = pf_cmina_get_file_directory (net);
   int res = 0;

   /* This operation is called from the background worker.
    * Ase data is protected by a mutex.
    */
   os_mutex_lock (net->cmina_mutex);
   ase_nvm = *p_ase;
   os_mutex_unlock (net->cmina_mutex);

   pf_cmina_ip_to_string (ase_nvm.full_ip_suite.ip_suite.ip_addr, ip_string);
   pf_cmina_ip_to_string (
      ase_nvm.full_ip_suite.ip_suite.ip_mask,
      netmask_string);
   pf_cmina_ip_to_string (
      ase_nvm.full_ip_suite.ip_suite.ip_gateway,
      gateway_string);

   res = pf_file_save_if_modified (
      p_file_directory,
      PF_FILENAME_IP,
      p_ase,
      &temporary_buffer,
      sizeof (pf_cmina_dcp_ase_t));
   switch (res)
   {
   case 2:
      LOG_DEBUG (
         PF_DCP_LOG,
         "CMINA(%d): First nvm saving of IP settings. "
         "IP: %s Netmask: %s Gateway: %s Station name: \"%s\"\n",
         __LINE__,
         ip_string,
         netmask_string,
         gateway_string,
         ase_nvm.station_name);
      break;
   case 1:
      LOG_DEBUG (
         PF_DCP_LOG,
         "CMINA(%d): Updating nvm stored IP settings. "
         "IP: %s Netmask: %s Gateway: %s Station name: \"%s\"\n",
         __LINE__,
         ip_string,
         netmask_string,
         gateway_string,
         ase_nvm.station_name);
      break;
   case 0:
      LOG_DEBUG (
         PF_DCP_LOG,
         "CMINA(%d): No storing of nvm IP settings (no changes). "
         "IP: %s Netmask: %s Gateway: %s Station name: \"%s\"\n",
         __LINE__,
         ip_string,
         netmask_string,
         gateway_string,
         ase_nvm.station_name);
      break;
   default:
   case -1:
      LOG_ERROR (
         PF_DCP_LOG,
         "CMINA(%d): Failed to store nvm IP settings.\n",
         __LINE__);
      break;
   }
}

int pf_cmina_set_default_cfg (pnet_t * net, uint16_t reset_mode)
{
   int ret = -1;
   const pnet_cfg_t * p_cfg = NULL;
   uint16_t ix;
   bool reset_user_application = false;
   pf_cmina_dcp_ase_t file_ase;
   char ip_string[PNAL_INET_ADDRSTR_SIZE] = {0};      /** Terminated string */
   char netmask_string[PNAL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   char gateway_string[PNAL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   uint32_t ip = 0;
   uint32_t netmask = 0;
   uint32_t gateway = 0;
   const char * p_file_directory = pf_cmina_get_file_directory (net);

   LOG_DEBUG (
      PF_DCP_LOG,
      "CMINA(%d): Setting default configuration. Reset mode: %u\n",
      __LINE__,
      reset_mode);

   pf_fspm_get_default_cfg (net, &p_cfg);
   if (p_cfg != NULL)
   {
      net->cmina_nonvolatile_dcp_ase.device_initiative = p_cfg->send_hello ? 1
                                                                           : 0;
      net->cmina_nonvolatile_dcp_ase.device_role = 1; /* Means: PNIO Device */

      memcpy (
         net->cmina_nonvolatile_dcp_ase.mac_address.addr,
         net->pf_interface.main_port.mac_address.addr,
         sizeof (pnet_ethaddr_t));

      strcpy (net->cmina_nonvolatile_dcp_ase.port_name, ""); /* Terminated */

      net->cmina_nonvolatile_dcp_ase.device_id = p_cfg->device_id;
      net->cmina_nonvolatile_dcp_ase.oem_device_id = p_cfg->oem_device_id;

      net->cmina_nonvolatile_dcp_ase.dhcp_enable =
         p_cfg->if_cfg.ip_cfg.dhcp_enable;
      if (reset_mode == 0) /* Power-on reset */
      {
         /* Read from file (nvm) */
         if (
            pf_file_load (
               p_file_directory,
               PF_FILENAME_IP,
               &file_ase,
               sizeof (file_ase)) == 0)
         {
            LOG_DEBUG (
               PF_DCP_LOG,
               "CMINA(%d): Did read IP parameters from nvm\n",
               __LINE__);
            ip = file_ase.full_ip_suite.ip_suite.ip_addr;
            netmask = file_ase.full_ip_suite.ip_suite.ip_mask;
            gateway = file_ase.full_ip_suite.ip_suite.ip_gateway;
            memcpy (
               net->cmina_nonvolatile_dcp_ase.station_name,
               file_ase.station_name,
               sizeof (net->cmina_nonvolatile_dcp_ase.station_name));
            net->cmina_nonvolatile_dcp_ase.station_name
               [sizeof (net->cmina_nonvolatile_dcp_ase.station_name) - 1] =
               '\0';
         }
         else
         {
            /* Use values for IP and station name from user configuration */
            LOG_DEBUG (
               PF_DCP_LOG,
               "CMINA(%d): Could not yet read IP parameters from nvm. Use "
               "values from user configuration.\n",
               __LINE__);
            PNAL_IP4_ADDR_TO_U32 (
               ip,
               p_cfg->if_cfg.ip_cfg.ip_addr.a,
               p_cfg->if_cfg.ip_cfg.ip_addr.b,
               p_cfg->if_cfg.ip_cfg.ip_addr.c,
               p_cfg->if_cfg.ip_cfg.ip_addr.d);
            PNAL_IP4_ADDR_TO_U32 (
               netmask,
               p_cfg->if_cfg.ip_cfg.ip_mask.a,
               p_cfg->if_cfg.ip_cfg.ip_mask.b,
               p_cfg->if_cfg.ip_cfg.ip_mask.c,
               p_cfg->if_cfg.ip_cfg.ip_mask.d);
            PNAL_IP4_ADDR_TO_U32 (
               gateway,
               p_cfg->if_cfg.ip_cfg.ip_gateway.a,
               p_cfg->if_cfg.ip_cfg.ip_gateway.b,
               p_cfg->if_cfg.ip_cfg.ip_gateway.c,
               p_cfg->if_cfg.ip_cfg.ip_gateway.d);
            memcpy (
               net->cmina_nonvolatile_dcp_ase.station_name,
               p_cfg->station_name,
               sizeof (net->cmina_nonvolatile_dcp_ase.station_name));
            net->cmina_nonvolatile_dcp_ase.station_name
               [sizeof (net->cmina_nonvolatile_dcp_ase.station_name) - 1] =
               '\0';
         }

         net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite.ip_addr = ip;
         net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite.ip_mask =
            netmask;
         net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite.ip_gateway =
            gateway;

         pf_cmina_ip_to_string (ip, ip_string);
         pf_cmina_ip_to_string (netmask, netmask_string);
         pf_cmina_ip_to_string (gateway, gateway_string);
         LOG_DEBUG (
            PF_DCP_LOG,
            "CMINA(%d): Using "
            "IP: %s Netmask: %s Gateway: %s Station name: \"%s\"\n",
            __LINE__,
            ip_string,
            netmask_string,
            gateway_string,
            net->cmina_nonvolatile_dcp_ase.station_name);
      }

      if (reset_mode == 1 || reset_mode >= 3)
      {
         /* Reset application parameters */
         reset_user_application = true;

         LOG_DEBUG (
            PF_DCP_LOG,
            "CMINA(%d): Reset application parameters.\n",
            __LINE__);

         /* Reset I&M data */
         (void)pf_fspm_clear_im_data (net);

#if PNET_OPTION_SNMP
         /* According to section 8.4 "Behavior to ResetToFactory" in
          * "Test case specification: Behavior" the MIB data should be reset
          * in all supported reset modes. This seems to contradict
          * PN-AL-Services ch. 6.3.11.3.2.
          */
         pf_snmp_data_clear (net);
#endif
      }

      if (reset_mode == 2 || reset_mode >= 3)
      {
         /* Reset communication parameters */
         LOG_DEBUG (
            PF_DCP_LOG,
            "CMINA(%d): Reset communication parameters.\n",
            __LINE__);

         /* Reset IP suite */
         net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite.ip_addr = 0;
         net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite.ip_mask = 0;
         net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite.ip_gateway = 0;

         /* Clear name of station */
         memset (
            net->cmina_nonvolatile_dcp_ase.station_name,
            0,
            sizeof (net->cmina_nonvolatile_dcp_ase.station_name));

         pf_file_clear (p_file_directory, PF_FILENAME_IP);
         pf_file_clear (p_file_directory, PF_FILENAME_DIAGNOSTICS);
#if PNET_OPTION_SNMP
         pf_snmp_data_clear (net);
#endif
         pf_pdport_reset_all (net);
      }

      if (reset_mode > 0)
      {
         /* User callback */
         pf_fspm_reset_ind (net, reset_user_application, reset_mode);
      }

      net->cmina_nonvolatile_dcp_ase.standard_gw_value = 0; /* Means: OwnIP is
                                                               treated as no
                                                               gateway */
      net->cmina_nonvolatile_dcp_ase.instance_id.high = 0;
      net->cmina_nonvolatile_dcp_ase.instance_id.low = 0;

      strncpy (
         net->cmina_nonvolatile_dcp_ase.product_name,
         p_cfg->product_name,
         sizeof (net->cmina_nonvolatile_dcp_ase.product_name));
      net->cmina_nonvolatile_dcp_ase
         .product_name[sizeof (net->cmina_nonvolatile_dcp_ase.product_name) - 1] =
         '\0';
      /* Remove trailing spaces */
      ix = (uint16_t)strlen (net->cmina_nonvolatile_dcp_ase.product_name);
      while ((ix > 1) &&
             (net->cmina_nonvolatile_dcp_ase.product_name[ix] == ' '))
      {
         ix--;
      }
      net->cmina_nonvolatile_dcp_ase.product_name[ix] = '\0';

      (void)pf_bg_worker_start_job (net, PF_BGJOB_SAVE_ASE_NVM_DATA);

      /* Init the current communication values */
      net->cmina_current_dcp_ase = net->cmina_nonvolatile_dcp_ase;

      ret = 0;
   }

   return ret;
}

void pf_cmina_dcp_set_commit (pnet_t * net)
{
   int res = 0;
   char ip_string[PNAL_INET_ADDRSTR_SIZE] = {0};      /** Terminated string */
   char netmask_string[PNAL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   char gateway_string[PNAL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   bool permanent = true;

   if (net->cmina_commit_ip_suite == false)
   {
      LOG_INFO (
         PF_DCP_LOG,
         "CMINA(%d): Did not set IP address. Is there a connection to the PLC, "
         "or is the station name not set?\n",
         __LINE__);

      return;
   }

   if (
      memcmp (
         &net->cmina_current_dcp_ase.full_ip_suite.ip_suite,
         &net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite,
         sizeof (pf_ip_suite_t)) != 0)
   {
      permanent = false;
   }
   else if (
      strcmp (
         net->cmina_current_dcp_ase.station_name,
         net->cmina_nonvolatile_dcp_ase.station_name) != 0)
   {
      permanent = false;
   }

   pf_cmina_ip_to_string (
      net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_addr,
      ip_string);
   pf_cmina_ip_to_string (
      net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_mask,
      netmask_string);
   pf_cmina_ip_to_string (
      net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_gateway,
      gateway_string);
   LOG_INFO (
      PF_DCP_LOG,
      "CMINA(%d): Setting IP: %s Netmask: %s Gateway: %s Station name: "
      "\"%s\" "
      "Permanent: %u\n",
      __LINE__,
      ip_string,
      netmask_string,
      gateway_string,
      net->cmina_current_dcp_ase.station_name,
      permanent);

   net->cmina_commit_ip_suite = false;
   res = pnal_set_ip_suite (
      net->pf_interface.main_port.name,
      &net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_addr,
      &net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_mask,
      &net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_gateway,
      net->cmina_current_dcp_ase.station_name,
      permanent);

   if (res != 0)
   {
      LOG_ERROR (
         PF_DCP_LOG,
         "CMINA(%d): Failed to set network parameters\n",
         __LINE__);
   }
}

int pf_cmina_init (pnet_t * net)
{
   int ret = 0;

   net->cmina_mutex = os_mutex_create();
   net->cmina_hello_count = 0;
   pf_scheduler_init_handle (&net->cmina_hello_timeout, "hello");
   net->cmina_error_decode = 0;
   net->cmina_error_code_1 = 0;
   net->cmina_commit_ip_suite = false;
   net->cmina_state = PF_CMINA_STATE_SETUP;

   /* Collect the DCP ASE database */
   memset (
      &net->cmina_nonvolatile_dcp_ase,
      0,
      sizeof (net->cmina_nonvolatile_dcp_ase));
   (void)pf_cmina_set_default_cfg (net, 0); /* Populates
                                               net->cmina_current_dcp_ase and
                                               net->cmina_nonvolatile_dcp_ase */

   if (strlen (net->cmina_current_dcp_ase.station_name) == 0)
   {
      LOG_DEBUG (
         PF_DCP_LOG,
         "CMINA(%d): No station name available. Will not send any HELLO "
         "messages.\n",
         __LINE__);

      if (net->cmina_current_dcp_ase.dhcp_enable == true)
      {
         /* Case 4 in Profinet 2.4 Table 1096 */
         /* No station name, DHCP enabled */

         /* ToDo: Send DHCP discover request */
      }
      else if (
         (net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_addr == 0) &&
         (net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_mask == 0) &&
         (net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_gateway == 0))
      {
         /* Case 3 in Profinet 2.4 Table 1096 */
         /* No station name, no DHCP enabled, no IP address */
         /* Ignore - Expect IP to be set by DCP later */
         net->cmina_commit_ip_suite = true;
      }
      else
      {
         /* Case 3 in Profinet 2.4 Table 1096 */
         /* No station name, no DHCP enabled, has IP address */
         /* Start IP */
         net->cmina_commit_ip_suite = true;
      }

      net->cmina_state = PF_CMINA_STATE_SET_NAME;
   }
   else
   {
      if (
         (net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_addr == 0) &&
         (net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_mask == 0) &&
         (net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_gateway == 0))
      {
         LOG_DEBUG (
            PF_DCP_LOG,
            "CMINA(%d): No IP address available. Will not send any HELLO "
            "messages.\n",
            __LINE__);

         if (net->cmina_current_dcp_ase.dhcp_enable == true)
         {
            /* Case 4 in Profinet 2.4 Table 1096 */
            /* Has station name, DHCP enabled, no IP address */

            /* ToDo: Send DHCP discover request */
         }
         else
         {
            /* Case 3 in Profinet 2.4 Table 1096 */
            /* Has station name, DHCP not enabled, no IP address */
            /* Ignore - Expect station name and IP to be set by DCP later */
            net->cmina_commit_ip_suite = true;
         }

         net->cmina_state = PF_CMINA_STATE_SET_IP;
      }
      else
      {
         /* Case 2 in Profinet 2.4 Table 1096 */
         /* Has station name, has IP address */
         /* Start IP */
         net->cmina_commit_ip_suite = true;

         if (net->cmina_current_dcp_ase.device_initiative == 1)
         {
            net->cmina_hello_count = PF_CMINA_FS_HELLO_RETRY;
            LOG_DEBUG (
               PF_DCP_LOG,
               "CMINA(%d): Start sending %u HELLO messages.\n",
               __LINE__,
               net->cmina_hello_count);

            /* Send first HELLO now! */
            (void)pf_scheduler_add (
               net,
               0,
               pf_cmina_send_hello,
               NULL,
               &net->cmina_hello_timeout);
         }
         else
         {
            LOG_DEBUG (
               PF_DCP_LOG,
               "CMINA(%d): Sending of HELLO messages is disabled in "
               "configuration.\n",
               __LINE__);
         }

         net->cmina_state = PF_CMINA_STATE_W_CONNECT;
      }
   }

   /* Change IP address if necessary */
   pf_cmina_dcp_set_commit (net);

   return ret;
}

/**
 * Abort active ARs with error code
 * @param net        InOut: The p-net stack instance
 * @param err_code   In: Reason ARs are aborted
 * return Number of aborted ARs
 */
static int pf_cmina_abort_active_ars (pnet_t * net, uint8_t err_code)
{
   int ix = 0;
   pf_ar_t * p_ar = NULL;
   int nbr_of_aborted_ars = 0;

   for (ix = 0; ix < PNET_MAX_AR; ix++)
   {
      p_ar = pf_ar_find_by_index (net, ix);
      if ((p_ar != NULL) && (p_ar->in_use == true))
      {
         p_ar->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
         p_ar->err_code = err_code;
         pf_cmdev_state_ind (net, p_ar, PNET_EVENT_ABORT);
         nbr_of_aborted_ars++;
      }
   }
   return nbr_of_aborted_ars;
}

/**
 * Count number of active ARs
 * @param net        InOut: The p-net stack instance
 * return Number of active ARs
 */
int pf_cmina_nbr_of_active_ars (pnet_t * net)
{
   int ix = 0;
   pf_ar_t * p_ar = NULL;
   int nbr_of_active_ars = 0;

   for (ix = 0; ix < PNET_MAX_AR; ix++)
   {
      p_ar = pf_ar_find_by_index (net, ix);
      if ((p_ar != NULL) && (p_ar->in_use == true))
      {
         nbr_of_active_ars++;
      }
   }
   return nbr_of_active_ars;
}

int pf_cmina_dcp_set_ind (
   pnet_t * net,
   uint8_t opt,
   uint8_t sub,
   uint16_t block_qualifier,
   uint16_t value_length,
   const uint8_t * p_value,
   uint8_t * p_block_error)
{
   int ret = -1;
   bool have_ip = false; /* cmina_current_dcp_ase has IP address after this call
                          */
   bool have_name = false; /* cmina_current_dcp_ase has station name after this
                              call */
   bool have_dhcp = false; /* Not yet supported */
   bool change_ip = false; /* We have got a request to change IP address */
   bool change_name = false; /* We have got a request to change station name */
   bool change_dhcp = false; /* We have got a request to change DHCP settings */
   bool reset_to_factory = false; /* We have got a request to do a factory reset
                                   */
   char ip_string[PNAL_INET_ADDRSTR_SIZE] = {0};      /** Terminated string */
   char netmask_string[PNAL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */
   char gateway_string[PNAL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */

   bool temp = ((block_qualifier & 1) == 0);
   uint16_t reset_mode = 0;

   /* Stop sending Hello packets */
   pf_scheduler_remove_if_running (net, &net->cmina_hello_timeout);

   /* Parse incoming DCP SET, without caring about actual CMINA state.
      Update cmina_current_dcp_ase and cmina_nonvolatile_dcp_ase*/
   switch (opt)
   {
   case PF_DCP_OPT_IP:
      switch (sub)
      {
      case PF_DCP_SUB_IP_PAR:
         if (
            value_length ==
            sizeof (net->cmina_current_dcp_ase.full_ip_suite.ip_suite))
         {
            if (pf_cmina_is_ipsuite_valid ((pf_ip_suite_t *)p_value))
            {
               /* Case 13 in Profinet 2.4 Table 1096 "CMINA state table" */
               /* Set IP address etc */
               change_ip =
                  (memcmp (
                      &net->cmina_current_dcp_ase.full_ip_suite.ip_suite,
                      p_value,
                      value_length) != 0);
               memcpy (
                  &net->cmina_current_dcp_ase.full_ip_suite.ip_suite,
                  p_value,
                  sizeof (net->cmina_current_dcp_ase.full_ip_suite.ip_suite));

               pf_cmina_ip_to_string (
                  net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_addr,
                  ip_string);
               pf_cmina_ip_to_string (
                  net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_mask,
                  netmask_string);
               pf_cmina_ip_to_string (
                  net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_gateway,
                  gateway_string);
               LOG_DEBUG (
                  PF_DCP_LOG,
                  "CMINA(%d): The incoming set request is about changing IP "
                  "(not DNS). IP: %s Netmask: %s Gateway: %s Temporary: %u\n",
                  __LINE__,
                  ip_string,
                  netmask_string,
                  gateway_string,
                  temp);

               if (temp == true)
               {
                  memset (
                     &net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite,
                     0,
                     sizeof (pf_ip_suite_t));
               }
               else
               {
                  net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite =
                     net->cmina_current_dcp_ase.full_ip_suite.ip_suite;
               }
               ret = 0;
            }
            else
            {
               /* Case 10, 22 in Profinet 2.4 Table 1096 "CMINA state table" */
               LOG_INFO (
                  PF_DCP_LOG,
                  "CMINA(%d): Got incoming request to change IP suite to an "
                  "illegal value.\n",
                  __LINE__);
               *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET;
            }
         }
         else
         {
            /* Case 12, 23 in Profinet 2.4 Table 1096 "CMINA state table" */
            *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET;
         }
         break;
      case PF_DCP_SUB_IP_SUITE:
         if (
            value_length <
            sizeof (net->cmina_current_dcp_ase.full_ip_suite)) /* Why less than?
                                                                */
         {
            if (pf_cmina_is_full_ipsuite_valid ((pf_full_ip_suite_t *)p_value))
            {
               /* Case 13 in Profinet 2.4 Table 1096 "CMINA state table" */
               /* Set IP address and DNS etc */
               change_ip =
                  (memcmp (
                      &net->cmina_current_dcp_ase.full_ip_suite,
                      p_value,
                      value_length) != 0);
               memcpy (
                  &net->cmina_current_dcp_ase.full_ip_suite,
                  p_value,
                  value_length);

               pf_cmina_ip_to_string (
                  net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_addr,
                  ip_string);
               pf_cmina_ip_to_string (
                  net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_mask,
                  netmask_string);
               pf_cmina_ip_to_string (
                  net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_gateway,
                  gateway_string);
               LOG_DEBUG (
                  PF_DCP_LOG,
                  "CMINA(%d): The incoming set request is about changing IP "
                  "and DNS. IP: %s Netmask: %s Gateway: %s Temporary: %u\n",
                  __LINE__,
                  ip_string,
                  netmask_string,
                  gateway_string,
                  temp);

               if (temp == true)
               {
                  memset (
                     &net->cmina_nonvolatile_dcp_ase.full_ip_suite,
                     0,
                     sizeof (pf_full_ip_suite_t));
               }
               else
               {
                  net->cmina_nonvolatile_dcp_ase.full_ip_suite =
                     net->cmina_current_dcp_ase.full_ip_suite;
               }
               ret = 0;
            }
            else
            {
               /* Case 10,22 in Profinet 2.4 Table 1096 "CMINA state table" */
               LOG_INFO (
                  PF_DCP_LOG,
                  "CMINA(%d): Got incoming request to change full IP suite to "
                  "an illegal value.\n",
                  __LINE__);
               *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET;
            }
         }
         else
         {
            *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET;
         }
         break;
      default:
         /* Case 12, 23 in Profinet 2.4 Table 1096 "CMINA state table" */
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         break;
      }
      break;
   case PF_DCP_OPT_DEVICE_PROPERTIES:
      switch (sub)
      {
      case PF_DCP_SUB_DEV_PROP_NAME:
         if (value_length < sizeof (net->cmina_current_dcp_ase.station_name))
         {
            if (pf_cmina_is_stationname_valid ((char *)p_value, value_length))
            {
               /* Case 13, 27 in Profinet 2.4 Table "CMINA state table" */
               /* Set station name */
               change_name =
                  ((strncmp (
                       net->cmina_current_dcp_ase.station_name,
                       (char *)p_value,
                       value_length) != 0) ||
                   (strlen (net->cmina_current_dcp_ase.station_name) !=
                    value_length));

               strncpy (
                  net->cmina_current_dcp_ase.station_name,
                  (char *)p_value,
                  value_length);
               net->cmina_current_dcp_ase.station_name[value_length] = '\0';
               LOG_INFO (
                  PF_DCP_LOG,
                  "CMINA(%d): The incoming set request is about changing "
                  "station name. New name: \"%s\"  Temporary: %u\n",
                  __LINE__,
                  net->cmina_current_dcp_ase.station_name,
                  temp);

               if (temp == true)
               {
                  memset (
                     net->cmina_nonvolatile_dcp_ase.station_name,
                     0,
                     sizeof (net->cmina_nonvolatile_dcp_ase.station_name));
               }
               else
               {
                  strcpy (
                     net->cmina_nonvolatile_dcp_ase.station_name,
                     net->cmina_current_dcp_ase.station_name); /* It always
                                                                     fits */
               }
               ret = 0;
            }
            else
            {
               /* Case 10, 22 in Profinet 2.4 Table 1096 "CMINA state table"*/
               LOG_INFO (
                  PF_DCP_LOG,
                  "CMINA(%d): Got incoming request to change station name to "
                  "an illegal value.\n",
                  __LINE__);
               *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET;
            }
         }
         else
         {
            *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET;
         }
         break;
      default:
         /* Case 12, 23 in Profinet 2.4 Table 1096 "CMINA state table" */
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         break;
      }
      break;
   case PF_DCP_OPT_DHCP:
      LOG_INFO (PF_DCP_LOG, "CMINA(%d): Trying to set DHCP\n", __LINE__);
      *p_block_error = PF_DCP_BLOCK_ERROR_OPTION_NOT_SUPPORTED;
      change_dhcp = true;
      break;
   case PF_DCP_OPT_CONTROL:
      if (sub == PF_DCP_SUB_CONTROL_FACTORY_RESET)
      {
         /* Case 15, 30 in Profinet 2.4 Table 1096 "CMINA state table" */
         reset_to_factory = true;
         reset_mode = 99; /* Reset all */

         ret = 0;
      }
      else if (sub == PF_DCP_SUB_CONTROL_RESET_TO_FACTORY)
      {
         /* For additional information on reset modes see
          * PN-AL-protocol (Mar20) Table 85
          * BlockQualifier with option ControlOption and suboption
          * SuboptionResetToFactory.
          * Note that these values does not match the
          * wireshark decoding which includes bit 0.
          */
         reset_mode = block_qualifier >> 1;

         /* Case 15, 30 in Profinet 2.4 Table 1096 "CMINA state table" */
         reset_to_factory = true;

         ret = 0;
      }
      break;
   default:
      /* Case 12, 23 in Profinet 2.4 Table 1096 "CMINA state table" */
      *p_block_error = PF_DCP_BLOCK_ERROR_OPTION_NOT_SUPPORTED;
      break;
   }

   /* Use parsed DCP SET request, depending on actual CMINA state */
   if (ret == 0)
   {
      /* Save to file */
      (void)pf_bg_worker_start_job (net, PF_BGJOB_SAVE_ASE_NVM_DATA);

      /* Evaluate what we have and where to go */
      have_name = (strlen (net->cmina_current_dcp_ase.station_name) > 0);
      have_ip =
         ((net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_addr != 0) ||
          (net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_mask != 0) ||
          (net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_gateway != 0));
      have_dhcp = false; /* ToDo: Not supported here */

      switch (net->cmina_state)
      {
      case PF_CMINA_STATE_SETUP:
         /* Should not occur */
         net->cmina_error_decode = PNET_ERROR_DECODE_PNIORW;
         net->cmina_error_code_1 = PNET_ERROR_CODE_1_ACC_STATE_CONFLICT;
         ret = -1;
         break;
      case PF_CMINA_STATE_SET_NAME:
      case PF_CMINA_STATE_SET_IP:
         if ((change_name == true) || (change_ip == true))
         {
            if ((have_name == true) && (have_ip == true))
            {
               /* Case 5, 13 in Profinet 2.4 Table 1096"CMINA state table" */
               /* Change name or change IP */
               /* Stop DHCP timer */
               net->cmina_commit_ip_suite = true;

               net->cmina_state = PF_CMINA_STATE_W_CONNECT;

               ret = 0;
            }
            else
            {
               /* Stop IP */
               if (have_name == false)
               {
                  /* Case 15 in Profinet 2.4 Table 1096 "CMINA state table" */
                  /* Reset name */
                  net->cmina_state = PF_CMINA_STATE_SET_NAME;
               }
               else if (have_ip == false)
               {
                  /* Case 15 in Profinet 2.4 Table 1096 "CMINA state table" */
                  /* Reset IP */
                  net->cmina_state = PF_CMINA_STATE_SET_IP;
               }
            }
         }
         else if (change_dhcp == true)
         {
            if (have_dhcp == true)
            {
               /* Case 14 in Profinet 2.4 Table 1096 "CMINA state table" */
               /* Change DHCP */
               memset (
                  &net->cmina_current_dcp_ase.full_ip_suite.ip_suite,
                  0,
                  sizeof (net->cmina_current_dcp_ase.full_ip_suite.ip_suite));
               if (temp == false)
               {
                  net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite =
                     net->cmina_current_dcp_ase.full_ip_suite.ip_suite;
               }
               /* dhcp_discover_req */
               net->cmina_state = PF_CMINA_STATE_SET_IP;
            }
            else
            {
               /* Case 14 in Profinet 2.4 Table 1096 "CMINA state table" */
               /* Reset DHCP */
               /* Stop DHCP */
            }
         }
         break;
      case PF_CMINA_STATE_W_CONNECT:
         if (
            (change_name == false) && (change_ip == false) &&
            (reset_to_factory == false))
         {
            /* Case 24 in Profinet 2.4 Table 1096 "CMINA state table" */
            /* No change of name or IP. All OK */
         }
         else if ((have_name == false) || (reset_to_factory == true))
         {
            int aborted_ars = 0;

            /* Case 27, 30 in Profinet 2.4 Table 1096 "CMINA state table" */
            /* Reset name or reset to factory */
            /* Abort active ARs */
            if (reset_to_factory == true)
            {
               aborted_ars = pf_cmina_abort_active_ars (
                  net,
                  PNET_ERROR_CODE_2_ABORT_DCP_RESET_TO_FACTORY);
            }
            else
            {
               aborted_ars = pf_cmina_abort_active_ars (
                  net,
                  PNET_ERROR_CODE_2_ABORT_DCP_STATION_NAME_CHANGED);
            }

            if (aborted_ars <= 0)
            {
               /* 37 */
               /* Stop IP */
            }
         }
         else if (
            ((have_name == true) && (change_name == true)) ||
            (have_ip == false))
         {
            int aborted_ars = 0;
            /* Case 27, 28 in Profinet 2.4 Table 1096 "CMINA state table" */
            /* Change name or reset IP */
            /* Abort active ARs */
            aborted_ars = pf_cmina_abort_active_ars (
               net,
               PNET_ERROR_CODE_2_ABORT_DCP_STATION_NAME_CHANGED);

            if (aborted_ars <= 0)
            {
               /* 39 */
               net->cmina_commit_ip_suite = true;
            }

            net->cmina_state = PF_CMINA_STATE_SET_IP;

            ret = 0;
         }
         else if ((have_ip == true) && (change_ip == true))
         {
            /* Change IP if no active connection*/

            if (pf_cmina_nbr_of_active_ars (net) == 0)
            {
               /* Case 25 in Profinet 2.4 Table 1096 "CMINA state table" */
               /* Change IP, no active connection */
               net->cmina_commit_ip_suite = true;

               ret = 0;
            }
            else
            {
               /* Case 26 in Profinet 2.4 Table 1096 "CMINA state table" */
               /* Change IP, active connection */
               net->cmina_error_decode = PNET_ERROR_DECODE_PNIORW;
               net->cmina_error_code_1 =
                  PNET_ERROR_CODE_1_ACC_INVALID_PARAMETER;
               ret = -1;
            }
         }
         else
         {
            /* 43 */
            net->cmina_error_decode = PNET_ERROR_DECODE_PNIORW;
            net->cmina_error_code_1 = PNET_ERROR_CODE_1_ACC_INVALID_PARAMETER;
            ret = -1;
         }
         break;
      }
   }

   /*
    * Case 5 in Profinet 2.4 Table 1096 "CMINA state table" Do_Check
    */
   if (
      ((net->cmina_state == PF_CMINA_STATE_SET_IP) ||
       (net->cmina_state == PF_CMINA_STATE_SET_NAME)) &&
      (have_name == true) && (have_ip == true))
   {
      net->cmina_state = PF_CMINA_STATE_W_CONNECT;
   }

   if (reset_to_factory == true)
   {
      /* Handle reset to factory here */
      /* Case 15 in Profinet 2.4 Table 1096 "CMINA state table" */
      ret = pf_cmina_set_default_cfg (net, reset_mode);
   }

   return ret;
}

int pf_cmina_dcp_get_req (
   pnet_t * net,
   uint8_t opt,
   uint8_t sub,
   uint16_t * p_value_length,
   uint8_t ** pp_value,
   uint8_t * p_block_error)
{
   int ret = 0; /* Assume all OK */

   switch (opt)
   {
   case PF_DCP_OPT_ALL:
      break;
   case PF_DCP_OPT_IP:
      switch (sub)
      {
      case PF_DCP_SUB_IP_MAC:
         *p_value_length = sizeof (net->cmina_current_dcp_ase.mac_address);
         *pp_value = (uint8_t *)&net->cmina_current_dcp_ase.mac_address;
         break;
      case PF_DCP_SUB_IP_PAR:
         *p_value_length =
            sizeof (net->cmina_current_dcp_ase.full_ip_suite.ip_suite);
         *pp_value =
            (uint8_t *)&net->cmina_current_dcp_ase.full_ip_suite.ip_suite;
         break;
      case PF_DCP_SUB_IP_SUITE:
         *p_value_length = sizeof (net->cmina_current_dcp_ase.full_ip_suite);
         *pp_value = (uint8_t *)&net->cmina_current_dcp_ase.full_ip_suite;
         break;
      default:
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         ret = -1;
         break;
      }
      break;
   case PF_DCP_OPT_DEVICE_PROPERTIES:
      switch (sub)
      {
      case PF_DCP_SUB_DEV_PROP_VENDOR:
         *p_value_length = sizeof (net->cmina_current_dcp_ase.product_name) -
                           1; /* Skip terminator */
         *pp_value = (uint8_t *)net->cmina_current_dcp_ase.product_name;
         break;
      case PF_DCP_SUB_DEV_PROP_NAME:
         *p_value_length = sizeof (net->cmina_current_dcp_ase.station_name);
         *pp_value = (uint8_t *)net->cmina_current_dcp_ase.station_name;
         break;
      case PF_DCP_SUB_DEV_PROP_ID:
         *p_value_length = sizeof (net->cmina_current_dcp_ase.device_id);
         *pp_value = (uint8_t *)&net->cmina_current_dcp_ase.device_id;
         break;
      case PF_DCP_SUB_DEV_PROP_ROLE:
         *p_value_length = sizeof (net->cmina_current_dcp_ase.device_role);
         *pp_value = (uint8_t *)&net->cmina_current_dcp_ase.device_role;
         break;
      case PF_DCP_SUB_DEV_PROP_OPTIONS:
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         ret = -1;
         break;
      case PF_DCP_SUB_DEV_PROP_INSTANCE:
         *p_value_length = sizeof (net->cmina_current_dcp_ase.instance_id);
         *pp_value = (uint8_t *)&net->cmina_current_dcp_ase.instance_id;
         break;
      case PF_DCP_SUB_DEV_PROP_OEM_ID:
         *p_value_length = sizeof (net->cmina_current_dcp_ase.oem_device_id);
         *pp_value = (uint8_t *)&net->cmina_current_dcp_ase.oem_device_id;
         break;
      case PF_DCP_SUB_DEV_PROP_GATEWAY:
         *p_value_length =
            sizeof (net->cmina_current_dcp_ase.standard_gw_value);
         *pp_value = (uint8_t *)&net->cmina_current_dcp_ase.standard_gw_value;
         break;
      case PF_DCP_SUB_DEV_PROP_ALIAS:
      default:
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         ret = -1;
         break;
      }
      break;
   case PF_DCP_OPT_DHCP:
      /* ToDo: No support for DHCP yet, because it needs a way to get/set in
       * lpiw. */
      switch (sub)
      {
      case PF_DCP_SUB_DHCP_HOSTNAME:
      case PF_DCP_SUB_DHCP_VENDOR_SPEC:
      case PF_DCP_SUB_DHCP_SERVER_ID:
      case PF_DCP_SUB_DHCP_PAR_REQ_LIST:
      case PF_DCP_SUB_DHCP_CLASS_ID:
      case PF_DCP_SUB_DHCP_CLIENT_ID:
         /* 61, 1+strlen(), 0, string */
      case PF_DCP_SUB_DHCP_FQDN:
      case PF_DCP_SUB_DHCP_UUID_CLIENT_ID:
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         ret = -1;
         break;
      case PF_DCP_SUB_DHCP_CONTROL:
         /* Can't get this */
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         ret = -1;
         break;
      default:
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         ret = -1;
         break;
      }
      break;
   case PF_DCP_OPT_CONTROL:
      /* Can't get control */
      *p_block_error = PF_DCP_BLOCK_ERROR_OPTION_NOT_SUPPORTED;
      ret = -1;
      break;
   case PF_DCP_OPT_DEVICE_INITIATIVE:
      switch (sub)
      {
      case PF_DCP_SUB_DEV_INITIATIVE_SUPPORT:
         *p_value_length =
            sizeof (net->cmina_current_dcp_ase.device_initiative);
         *pp_value = (uint8_t *)&net->cmina_current_dcp_ase.device_initiative;
         break;
      default:
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         ret = -1;
         break;
      }
      break;
   default:
      *p_block_error = PF_DCP_BLOCK_ERROR_OPTION_NOT_SUPPORTED;
      ret = -1;
      break;
   }

   if (ret == 0)
   {
      *p_block_error = PF_DCP_BLOCK_ERROR_NO_ERROR;
   }

   return ret;
}

const char * pf_cmina_get_file_directory (const pnet_t * net)
{
   return net->fspm_cfg.file_directory;
}

void pf_cmina_get_station_name (const pnet_t * net, char * station_name)
{
   snprintf (
      station_name,
      PNET_STATION_NAME_MAX_SIZE,
      "%s",
      net->cmina_current_dcp_ase.station_name);
}

pnal_ipaddr_t pf_cmina_get_ipaddr (const pnet_t * net)
{
   return net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_addr;
}

pnal_ipaddr_t pf_cmina_get_netmask (const pnet_t * net)
{
   return net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_mask;
}

pnal_ipaddr_t pf_cmina_get_gateway (const pnet_t * net)
{
   return net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_gateway;
}

const pnet_ethaddr_t * pf_cmina_get_device_macaddr (const pnet_t * net)
{
   return &net->pf_interface.main_port.mac_address;
}

/************************* Utilities ******************************************/

int pf_cmina_remove_all_data_files (const char * file_directory)
{
   pf_file_clear (file_directory, PF_FILENAME_IM);
   pf_file_clear (file_directory, PF_FILENAME_IP);
   pf_file_clear (file_directory, PF_FILENAME_DIAGNOSTICS);
#if PNET_OPTION_SNMP
   pf_snmp_remove_data_files (file_directory);
#endif
   pf_pdport_remove_data_files (file_directory);

   return 0;
}

bool pf_cmina_has_timed_out (
   uint32_t now_us,
   uint32_t previous_us,
   uint16_t period,
   uint16_t factor)
{
   return (now_us - previous_us) >= ((uint32_t)(period * factor) * 1000 / 32);
}

/*************** Diagnostic strings *****************************************/

void pf_cmina_ip_to_string (pnal_ipaddr_t ip, char * outputstring)
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

/**
 * @internal
 * Return a string representation of the CMINA state.
 * @param net              InOut: The p-net stack instance
 * @return  a string representation of the CMINA state.
 */
static const char * pf_cmina_state_to_string (pnet_t * net)
{
   const char * s = "<unknown>";
   switch (net->cmina_state)
   {
   case PF_CMINA_STATE_SETUP:
      s = "PF_CMINA_STATE_SETUP";
      break;
   case PF_CMINA_STATE_SET_NAME:
      s = "PF_CMINA_STATE_SET_NAME";
      break;
   case PF_CMINA_STATE_SET_IP:
      s = "PF_CMINA_STATE_SET_IP";
      break;
   case PF_CMINA_STATE_W_CONNECT:
      s = "PF_CMINA_STATE_W_CONNECT";
      break;
   }

   return s;
}

/**
 * @internal
 * Print an IPv4 address (without newline)
 *
 * @param ip      In: IP address
 */
void pf_ip_address_show (uint32_t ip)
{
   char ip_string[PNAL_INET_ADDRSTR_SIZE] = {0}; /** Terminated string */

   pf_cmina_ip_to_string (ip, ip_string);
   printf ("%s", ip_string);
}

void pf_cmina_port_statistics_show (pnet_t * net)
{
   int port;
   pf_port_iterator_t port_iterator;
   pnal_port_stats_t stats;
   pf_port_t * p_port_data;

   if (pnal_get_port_statistics (net->pf_interface.main_port.name, &stats) == 0)
   {
      printf (
         "Main interface %10s  In: %" PRIu32 " bytes %" PRIu32
         " errors %" PRIu32 " discards  Out: %" PRIu32 " bytes %" PRIu32
         " errors %" PRIu32 " discards\n",
         net->pf_interface.main_port.name,
         stats.if_in_octets,
         stats.if_in_errors,
         stats.if_in_discards,
         stats.if_out_octets,
         stats.if_out_errors,
         stats.if_out_discards);
   }
   else
   {
      printf (
         "Did not find main interface %s\n",
         net->pf_interface.main_port.name);
   }

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);

      if (pnal_get_port_statistics (p_port_data->netif.name, &stats) == 0)
      {
         printf (
            "Port           %10s  In: %" PRIu32 " bytes %" PRIu32
            " errors %" PRIu32 " discards  Out: %" PRIu32 " bytes %" PRIu32
            " errors %" PRIu32 " discards\n",
            p_port_data->netif.name,
            stats.if_in_octets,
            stats.if_in_errors,
            stats.if_in_discards,
            stats.if_out_octets,
            stats.if_out_errors,
            stats.if_out_discards);
      }
      else
      {
         printf (
            "Did not find statistics for port %s\n",
            p_port_data->netif.name);
      }

      port = pf_port_get_next (&port_iterator);
   }
}

void pf_cmina_show (pnet_t * net)
{
   const pnet_cfg_t * p_cfg = NULL;

   pf_fspm_get_default_cfg (net, &p_cfg);

   printf ("CMINA\n");
   printf (
      "state                          : %s\n",
      pf_cmina_state_to_string (net));
   printf ("Default station_name           : \"%s\"\n", p_cfg->station_name);
   printf (
      "Perm station_name              : \"%s\"\n",
      net->cmina_nonvolatile_dcp_ase.station_name);
   printf (
      "Temp station_name              : \"%s\"\n",
      net->cmina_current_dcp_ase.station_name);
   printf ("\n");

   printf ("Default product_name           : \"%s\"\n", p_cfg->product_name);
   printf (
      "Perm product_name              : \"%s\"\n",
      net->cmina_nonvolatile_dcp_ase.product_name);
   printf (
      "Temp product_name              : \"%s\"\n",
      net->cmina_current_dcp_ase.product_name);
   printf ("\n");

   printf (
      "Default IP  Netmask  Gateway   : %u.%u.%u.%u  ",
      (unsigned)p_cfg->if_cfg.ip_cfg.ip_addr.a,
      (unsigned)p_cfg->if_cfg.ip_cfg.ip_addr.b,
      (unsigned)p_cfg->if_cfg.ip_cfg.ip_addr.c,
      (unsigned)p_cfg->if_cfg.ip_cfg.ip_addr.d);
   printf (
      "%u.%u.%u.%u  ",
      (unsigned)p_cfg->if_cfg.ip_cfg.ip_mask.a,
      (unsigned)p_cfg->if_cfg.ip_cfg.ip_mask.b,
      (unsigned)p_cfg->if_cfg.ip_cfg.ip_mask.c,
      (unsigned)p_cfg->if_cfg.ip_cfg.ip_mask.d);
   printf (
      "%u.%u.%u.%u\n",
      (unsigned)p_cfg->if_cfg.ip_cfg.ip_gateway.a,
      (unsigned)p_cfg->if_cfg.ip_cfg.ip_gateway.b,
      (unsigned)p_cfg->if_cfg.ip_cfg.ip_gateway.c,
      (unsigned)p_cfg->if_cfg.ip_cfg.ip_gateway.d);

   printf ("Perm    IP  Netmask  Gateway   : ");
   pf_ip_address_show (
      net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite.ip_addr);
   printf ("  ");
   pf_ip_address_show (
      net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite.ip_mask);
   printf ("  ");
   pf_ip_address_show (
      net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite.ip_gateway);

   printf ("\nTemp    IP  Netmask  Gateway   : ");
   pf_ip_address_show (
      net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_addr);
   printf ("  ");
   pf_ip_address_show (
      net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_mask);
   printf ("  ");
   pf_ip_address_show (
      net->cmina_current_dcp_ase.full_ip_suite.ip_suite.ip_gateway);
   printf ("\n");

   printf (
      "MAC                            : %02x:%02x:%02x:%02x:%02x:%02x\n\n",
      net->pf_interface.main_port.mac_address.addr[0],
      net->pf_interface.main_port.mac_address.addr[1],
      net->pf_interface.main_port.mac_address.addr[2],
      net->pf_interface.main_port.mac_address.addr[3],
      net->pf_interface.main_port.mac_address.addr[4],
      net->pf_interface.main_port.mac_address.addr[5]);

   pf_cmina_port_statistics_show (net);
}

/************************* Validate incoming data ***************************/

/**
 * @internal
 * Check if a station name is valid
 *
 * Defined in Profinet 2.4, section 4.3.1.4.16.2:
 *
 * - 1 or more labels, separated by [.]
 * - Total length is 1 to 240
 * - Label length is 1 to 63
 * - Labels consist of [a-z0-9-]
 * - Labels do not start with [-]
 * - Labels do not end with [-]
 * - Labels do not use multiple concatenated [-] except for IETF RFC 5890
 * - The first label does not have the form "port-xyz" or "port-xyz-abcde"
 *     with a, b, c, d, e, x, y, z = 0...9
 * - Station-names do not have the form a.b.c.d with a, b, c, d = 0...999
 *
 * Also empty station name is valid here (indicating that no station name has
 * been set).
 *
 * @param station_name     In: Station name
 * @param len              In: Length of incoming string without terminator.
 * @return  0  if the name is valid
 *          -1 if the name is invalid
 */

bool pf_cmina_is_stationname_valid (const char * station_name, uint16_t len)
{
   uint16_t i;
   uint16_t label_offset = 0;
   char prev = 'a';
   uint16_t n_labels = 1;
   uint16_t n_non_digit = 0;
   uint8_t c = 0;

   /* - Empty station name is valid here (indicating that no station name has
    * been set) */
   if (len == 0)
   {
      return true;
   }

   if (!station_name)
   {
      return false;
   }

   /*
    * - Labels do not start with [-]
    * - Total length is 1 to 240
    */
   if ((station_name[0] == '-') || (len >= PNET_STATION_NAME_MAX_SIZE))
   {
      return false;
   }

   /*
    * - The first label does not have the form "port-xyz" or "port-xyz-abcde"
    *   with a, b, c, d, e, x, y, z = 0...9
    */
   if (strncmp (station_name, "port-", 5) == 0)
   {
      if ((len == 8) || (len == 14))
      {
         uint16_t n_char = 0;
         for (i = 5; i < len; i++)
         {
            c = station_name[i];
            if ((!isdigit (c)) && !(c == '-'))
            {
               n_char++;
            }
         }

         if (n_char == 0)
         {
            return false;
         }
      }
   }

   for (i = 0; i < len; i++)
   {
      c = station_name[i];
      if (c == '.')
      {
         n_labels++;
         label_offset = 0;

         /* - Labels do not end with [-] */
         if (prev == '-')
         {
            return false;
         }
      }
      /* - Labels consist of [a-z0-9-] */
      else if (!((islower (c) || isdigit (c) || (c == '-'))))
      {
         return false;
      }
      else
      {
         /* - Labels do not start with [-] */
         if ((label_offset == 0) && (c == '-'))
         {
            return false;
         }

         /* - Label length is 1 to 63 */
         if (label_offset >= 63)
         {
            return false;
         }

         if (!isdigit (c))
         {
            n_non_digit++;
         }
         label_offset++;
      }
      prev = c;
   }

   /* - Station-names do not have the form a.b.c.d with a, b, c, d = 0...999 */
   if ((n_labels == 4) && (n_non_digit == 0))
   {
      return false;
   }

   /* - Labels do not end with [-] */
   if (prev == '-')
   {
      return false;
   }
   return true;
}

/**
 * @internal
 * Check if an IPv4 suite is valid.
 *
 * Defined in Profinet 2.4, section 4.3.1.4.21.5
 *
 * @param ipsuite          In: IP suite with IP, netmask, gateway
 * @return  0  if the IP suite is valid
 *          -1 if the IP suite is invalid
 */
bool pf_cmina_is_ipsuite_valid (const pf_ip_suite_t * p_ipsuite)
{
   if (!pf_cmina_is_netmask_valid (p_ipsuite->ip_mask))
   {
      return false;
   }
   if (!pf_cmina_is_ipaddress_valid (p_ipsuite->ip_mask, p_ipsuite->ip_addr))
   {
      return false;
   }
   if (!pf_cmina_is_gateway_valid (
          p_ipsuite->ip_addr,
          p_ipsuite->ip_mask,
          p_ipsuite->ip_gateway))
   {
      return false;
   }

   return true;
}

/**
 * @internal
 * Check if a full IPv4 suite is valid.
 *
 * Defined in Profinet 2.4, section
 *
 * @param full_ipsuite     In: IP suite with IP, netmask, gateway and DNS
 * addresses
 * @return  0  if the IP suite is valid
 *          -1 if the IP suite is invalid
 */
bool pf_cmina_is_full_ipsuite_valid (const pf_full_ip_suite_t * p_full_ipsuite)
{
   if (!pf_cmina_is_ipsuite_valid (&p_full_ipsuite->ip_suite))
   {
      return false;
   }

   /* TODO validate DNS addresses */

   return true;
}

/**
 * @internal
 * Check if an IPv4 address is valid
 *
 * Defined in Profinet 2.4, section 4.3.1.4.21.2
 *
 * @param netmask          In: Netmask
 * @param netmask          In: IP address
 * @return  0  if the IP address is valid
 *          -1 if the IP address is invalid
 */
bool pf_cmina_is_ipaddress_valid (pnal_ipaddr_t netmask, pnal_ipaddr_t ip)
{
   uint32_t host_part = ip & ~netmask;

   if ((netmask == 0) && (ip == 0))
   {
      return true;
   }
   if (!pf_cmina_is_netmask_valid (netmask))
   {
      return false;
   }
   if ((host_part == 0) || (host_part == ~netmask))
   {
      return false;
   }
   if (ip <= PNAL_MAKEU32 (0, 255, 255, 255))
   {
      return false;
   }
   else if (
      (ip >= PNAL_MAKEU32 (127, 0, 0, 0)) &&
      (ip <= PNAL_MAKEU32 (127, 255, 255, 255)))
   {
      return false;
   }
   else if (
      (ip >= PNAL_MAKEU32 (224, 0, 0, 0)) &&
      (ip <= PNAL_MAKEU32 (239, 255, 255, 255)))
   {
      return false;
   }
   else if (
      (ip >= PNAL_MAKEU32 (240, 0, 0, 0)) &&
      (ip <= PNAL_MAKEU32 (255, 255, 255, 255)))
   {
      return false;
   }
   return true;
}

/**
 * @internal
 * Check if a netmask is valid
 *
 * Defined in Profinet 2.4, section 4.3.1.4.21.3
 *
 * @param netmask          In: netmask
 * @return  0  if the netmask is valid
 *          -1 if the netmask is invalid
 */
bool pf_cmina_is_netmask_valid (pnal_ipaddr_t netmask)
{
   if (!(netmask & (~netmask >> 1)))
   {
      return true;
   }
   else
   {
      return false;
   }
}

/**
 * @internal
 * Check if a gateway address is valid
 *
 * Defined in Profinet 2.4, section 4.3.1.4.21.4
 *
 * @param ip               In: IP address
 * @param netmask          In: Netmask
 * @param gateway          In: Gateway address
 * @return  0  if the gateway address is valid
 *          -1 if the gateway address is invalid
 */
bool pf_cmina_is_gateway_valid (
   pnal_ipaddr_t ip,
   pnal_ipaddr_t netmask,
   pnal_ipaddr_t gateway)
{
   if ((gateway != 0) && ((ip & netmask) != (gateway & netmask)))
   {
      return false;
   }
   return true;
}
