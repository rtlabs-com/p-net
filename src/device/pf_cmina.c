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
 * @brief Implements the Context Management Ip and Name Assignment protocol machine (CMINA)
 *
 * Handles assignment of station name and IP address.
 * Manages sending DCP hello requests.
 *
 * Resets the communication stack and user application.
 */


#ifdef UNIT_TEST
#define os_set_ip_suite mock_os_set_ip_suite
#endif

#include <string.h>
#include <ctype.h>

#include "pf_includes.h"

static const char             *hello_sync_name = "hello";


/**
 * @internal
 * Send a DCP HELLO message.
 *
 * This is a callback for the scheduler. Arguments should fulfill pf_scheduler_timeout_ftn_t
 *
 * If this was not the last requested HELLO message then re-schedule another call after 1 s.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:   Not used.
 * @param current_time     In:   Not used.
 */
static void pf_cmina_send_hello(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                current_time)
{
   if ((net->cmina_state == PF_CMINA_STATE_W_CONNECT) &&
       (net->cmina_hello_count > 0))
   {
      (void)pf_dcp_hello_req(net);

      /* Reschedule */
      net->cmina_hello_count--;
      if (pf_scheduler_add(net, PF_CMINA_FS_HELLO_INTERVAL*1000,
         hello_sync_name, pf_cmina_send_hello, NULL, &net->cmina_hello_timeout) != 0)
      {
         net->cmina_hello_timeout = UINT32_MAX;
      }
   }
   else
   {
      net->cmina_hello_timeout = UINT32_MAX;
      net->cmina_hello_count = 0;
   }
}

int pf_cmina_set_default_cfg(
   pnet_t                  *net,
   uint16_t                reset_mode)
{
   int                     ret = -1;
   const pnet_cfg_t        *p_cfg = NULL;
   uint16_t                ix;
   bool                    should_reset_user_application = false;

   LOG_DEBUG(PF_DCP_LOG,"CMINA(%d): Setting default configuration. Reset mode: %u\n", __LINE__, reset_mode);

   pf_fspm_get_default_cfg(net, &p_cfg);
   if (p_cfg != NULL)
   {

      net->cmina_perm_dcp_ase.device_initiative = p_cfg->send_hello ? 1 : 0;
      net->cmina_perm_dcp_ase.device_role = 1;            /* Means: PNIO Device */

      memcpy(net->cmina_perm_dcp_ase.mac_address.addr, p_cfg->eth_addr.addr, sizeof(os_ethaddr_t));

      strcpy(net->cmina_perm_dcp_ase.port_name, "");      /* Terminated */
      strncpy(net->cmina_perm_dcp_ase.manufacturer_specific_string, p_cfg->manufacturer_specific_string, sizeof(net->cmina_perm_dcp_ase.manufacturer_specific_string));
      net->cmina_perm_dcp_ase.manufacturer_specific_string[sizeof(net->cmina_perm_dcp_ase.manufacturer_specific_string) - 1] = '\0';

      net->cmina_perm_dcp_ase.device_id = p_cfg->device_id;
      net->cmina_perm_dcp_ase.oem_device_id = p_cfg->oem_device_id;

      net->cmina_perm_dcp_ase.dhcp_enable = p_cfg->dhcp_enable;
      if (reset_mode == 0)                /* Power-on reset */
      {
         /* ToDo: Get from permanent pool */
         /* osal_get_perm_pool((uint8_t *)&net->cmina_perm_dcp_ase, sizeof(net->cmina_perm_dcp_ase)); */

         OS_IP4_ADDR_TO_U32(net->cmina_perm_dcp_ase.full_ip_suite.ip_suite.ip_addr,
            p_cfg->ip_addr.a, p_cfg->ip_addr.b, p_cfg->ip_addr.c, p_cfg->ip_addr.d);
         OS_IP4_ADDR_TO_U32(net->cmina_perm_dcp_ase.full_ip_suite.ip_suite.ip_mask,
            p_cfg->ip_mask.a, p_cfg->ip_mask.b, p_cfg->ip_mask.c, p_cfg->ip_mask.d);
         OS_IP4_ADDR_TO_U32(net->cmina_perm_dcp_ase.full_ip_suite.ip_suite.ip_gateway,
            p_cfg->ip_gateway.a, p_cfg->ip_gateway.b, p_cfg->ip_gateway.c, p_cfg->ip_gateway.d);
         memcpy(net->cmina_perm_dcp_ase.name_of_station, p_cfg->station_name, sizeof(net->cmina_perm_dcp_ase.name_of_station));
         net->cmina_perm_dcp_ase.name_of_station[sizeof(net->cmina_perm_dcp_ase.name_of_station) - 1] = '\0';
      }
      if (reset_mode == 1 || reset_mode == 99)  /* Reset application parameters */
      {
         should_reset_user_application = true;

         /* Reset I&M data */
         ret = pf_fspm_clear_im_data(net);
      }
      if (reset_mode == 2 || reset_mode == 99)  /* Reset communication parameters */
      {
         /* Reset IP suite */
         net->cmina_perm_dcp_ase.full_ip_suite.ip_suite.ip_addr = 0;
         net->cmina_perm_dcp_ase.full_ip_suite.ip_suite.ip_mask = 0;
         net->cmina_perm_dcp_ase.full_ip_suite.ip_suite.ip_gateway = 0;
         /* Clear name of station */
         memset(net->cmina_perm_dcp_ase.name_of_station, 0, sizeof(net->cmina_perm_dcp_ase.name_of_station));
      }
      if (reset_mode > 0)
      {
         /* User callback */
         (void) pf_fspm_reset_ind(net, should_reset_user_application, reset_mode);
      }

      net->cmina_perm_dcp_ase.standard_gw_value = 0;         /* Means: OwnIP is treated as no gateway */

      net->cmina_perm_dcp_ase.instance_id.high = 0;
      net->cmina_perm_dcp_ase.instance_id.low = 0;

      strncpy(net->cmina_perm_dcp_ase.device_vendor, p_cfg->device_vendor, sizeof(net->cmina_perm_dcp_ase.device_vendor));
      net->cmina_perm_dcp_ase.device_vendor[sizeof(net->cmina_perm_dcp_ase.device_vendor)-1] = '\0';
      /* Remove trailing spaces */
      ix = (uint16_t)strlen(net->cmina_perm_dcp_ase.device_vendor);
      while ((ix > 1) && (net->cmina_perm_dcp_ase.device_vendor[ix] == ' '))
      {
         ix--;
      }
      net->cmina_perm_dcp_ase.device_vendor[ix] = '\0';

      /* Init the temp values */
      net->cmina_temp_dcp_ase = net->cmina_perm_dcp_ase;


      ret = 0;
   }

   return ret;
}

void pf_cmina_dcp_set_commit(
   pnet_t                  *net)
{
   /* Assume permanent changes, possibly modify later on */
   bool                    permanent = true;

   if (net->cmina_commit_ip_suite == true)
   {
      LOG_DEBUG(PF_DCP_LOG,"CMINA(%d): Setting IP address: %u.%u.%u.%u  Station name: %s\n",
         __LINE__,
         (unsigned)((net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_addr >> 24) & 0xFF),
         (unsigned)((net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_addr >> 16) & 0xFF),
         (unsigned)((net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_addr >> 8) & 0xFF),
         (unsigned)(net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_addr & 0xFF),
         net->cmina_temp_dcp_ase.name_of_station);
      net->cmina_commit_ip_suite = false;
      os_set_ip_suite(net->interface_name,
                      &net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_addr,
                      &net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_mask,
                      &net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_gateway,
                      net->cmina_temp_dcp_ase.name_of_station,
                      permanent);
   }
}

int pf_cmina_init(
   pnet_t                  *net)
{
   int                     ret = 0;

   net->cmina_hello_count = 0;
   net->cmina_hello_timeout = UINT32_MAX;
   net->cmina_error_decode = 0;
   net->cmina_error_code_1 = 0;
   net->cmina_commit_ip_suite = false;
   net->cmina_state = PF_CMINA_STATE_SETUP;

   /* Collect the DCP ASE database */
   memset(&net->cmina_perm_dcp_ase, 0, sizeof(net->cmina_perm_dcp_ase));
   (void)pf_cmina_set_default_cfg(net, 0);

   if (strlen(net->cmina_temp_dcp_ase.name_of_station) == 0)
   {
      if (net->cmina_temp_dcp_ase.dhcp_enable == true)
      {
         /* 1 */
         /* ToDo: Send DHCP discover request */
      }
      else if ((net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_addr == 0) &&
               (net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_mask == 0) &&
               (net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_gateway == 0))
      {
         /* 2 */
         /* Ignore - Expect IP to be set by DCP later */
      }
      else
      {
         /* 3 */
         /* Start IP */
         net->cmina_commit_ip_suite = true;
      }

      net->cmina_state = PF_CMINA_STATE_SET_NAME;
   }
   else
   {
      if ((net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_addr == 0) &&
          (net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_mask == 0) &&
          (net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_gateway == 0))
      {
         if (net->cmina_temp_dcp_ase.dhcp_enable == true)
         {
            /* 4 */
            /* ToDo: Send DHCP discover request */
         }
         else
         {
            /* 5 */
            /* Ignore - Expect station name and IP to be set by DCP later */
         }

         net->cmina_state = PF_CMINA_STATE_SET_IP;
      }
      else
      {
         /* 6, 7 */
         /* Start IP */
         net->cmina_commit_ip_suite = true;

         if (net->cmina_perm_dcp_ase.device_initiative == 1)
         {
            /* 6 */
            net->cmina_hello_count = PF_CMINA_FS_HELLO_RETRY;
            /* Send first HELLO now! */
            ret = pf_scheduler_add(net, 0, hello_sync_name, pf_cmina_send_hello, NULL, &net->cmina_hello_timeout);
            if (ret != 0)
            {
               net->cmina_hello_timeout = UINT32_MAX;
            }
         }

         net->cmina_state = PF_CMINA_STATE_W_CONNECT;
      }
   }

   /* Change IP address if necessary */
   pf_cmina_dcp_set_commit(net);

   return ret;
}

int pf_cmina_dcp_set_ind(
   pnet_t                  *net,
   uint8_t                 opt,
   uint8_t                 sub,
   uint16_t                block_qualifier,
   uint16_t                value_length,
   uint8_t                 *p_value,
   uint8_t                 *p_block_error)
{
   int                     ret = -1;
   bool                    have_ip = false;
   bool                    have_name = false;
   bool                    have_dhcp = false;
   bool                    change_ip = false;
   bool                    change_name = false;
   bool                    change_dhcp = false;
   bool                    reset_to_factory = false;
   pf_ar_t                 *p_ar = NULL;
   uint16_t                ix;
   bool                    found = false;
   bool                    temp = ((block_qualifier & 1) == 0);
   uint16_t                reset_mode = block_qualifier >> 1;

   /* stop sending Hello packets */
   if (net->cmina_hello_timeout != UINT32_MAX)
   {
      pf_scheduler_remove(net, hello_sync_name, net->cmina_hello_timeout);
      net->cmina_hello_timeout = UINT32_MAX;
   }
   switch (opt)
   {
   case PF_DCP_OPT_IP:
      switch (sub)
      {
      case PF_DCP_SUB_IP_PAR:
         if (value_length == sizeof(net->cmina_temp_dcp_ase.full_ip_suite.ip_suite))
         {
            if (pf_cmina_is_ipsuite_valid((pf_ip_suite_t*)p_value))
            {
               change_ip = (memcmp(&net->cmina_temp_dcp_ase.full_ip_suite.ip_suite, p_value, value_length) != 0);

               memcpy(&net->cmina_temp_dcp_ase.full_ip_suite.ip_suite, p_value, sizeof(net->cmina_temp_dcp_ase.full_ip_suite.ip_suite));
               LOG_INFO(PF_DCP_LOG,"CMINA(%d): The incoming set request is about changing IP. PF_DCP_SUB_IP_PAR New IP: 0x%"PRIxLEAST32"\n", __LINE__, net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_addr);
               if (temp == false)
               {
                  net->cmina_perm_dcp_ase.full_ip_suite.ip_suite = net->cmina_temp_dcp_ase.full_ip_suite.ip_suite;
               }
               ret = 0;
            }
            else
            {
               LOG_INFO(PF_DCP_LOG,"CMINA(%d): Got incoming request to change IP suite to an illegal value.\n", __LINE__);
               *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET;
            }
         }
         else
         {
            *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET;
         }
         break;
      case PF_DCP_SUB_IP_SUITE:
         if (value_length < sizeof(net->cmina_temp_dcp_ase.full_ip_suite))   /* Why less than? */
         {
            if (pf_cmina_is_full_ipsuite_valid((pf_full_ip_suite_t *)p_value))
            {
               LOG_INFO(PF_DCP_LOG,"CMINA(%d): The incoming set request is about changing IP. PF_DCP_SUB_IP_SUITE\n", __LINE__);
               change_ip = (memcmp(&net->cmina_temp_dcp_ase.full_ip_suite, p_value, value_length) != 0);

               memcpy(&net->cmina_temp_dcp_ase.full_ip_suite, p_value, value_length);
               if (temp == false)
               {
                  net->cmina_perm_dcp_ase.full_ip_suite = net->cmina_temp_dcp_ase.full_ip_suite;
               }
               ret = 0;
            }
            else
            {
               LOG_INFO(PF_DCP_LOG,"CMINA(%d): Got incoming request to change full IP suite to an illegal value.\n", __LINE__);
               *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET;
            }
         }
         else
         {
            *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET;
         }
         break;
      default:
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         break;
      }
      break;
   case PF_DCP_OPT_DEVICE_PROPERTIES:
      switch (sub)
      {
      case PF_DCP_SUB_DEV_PROP_NAME:
         if (value_length < sizeof(net->cmina_temp_dcp_ase.name_of_station))
         {
            if (pf_cmina_is_stationname_valid((char *)p_value, value_length))
            {
               change_name = ((strncmp(net->cmina_temp_dcp_ase.name_of_station, (char *)p_value, value_length) != 0) &&
                     (strlen(net->cmina_temp_dcp_ase.name_of_station) != value_length));

               strncpy(net->cmina_temp_dcp_ase.name_of_station, (char *)p_value, value_length);
               net->cmina_temp_dcp_ase.name_of_station[value_length] = '\0';
               LOG_INFO(PF_DCP_LOG,"CMINA(%d): The incoming set request is about changing station name. New name: %s\n", __LINE__, net->cmina_temp_dcp_ase.name_of_station);

               if (temp == false)
               {
                  strcpy(net->cmina_perm_dcp_ase.name_of_station, net->cmina_temp_dcp_ase.name_of_station);   /* It always fits */
               }
               else
               {
                  memset(net->cmina_perm_dcp_ase.name_of_station, 0, sizeof(net->cmina_perm_dcp_ase.name_of_station));
               }
               ret = 0;
            }
            else
            {
               LOG_INFO(PF_DCP_LOG,"CMINA(%d): Got incoming request to change station name to an illegal value.\n", __LINE__);
               *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET;
            }
         }
         else
         {
            *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET;
         }
         break;
      default:
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         break;
      }
      break;
   case PF_DCP_OPT_DHCP:
      LOG_INFO(PF_DCP_LOG,"CMINA(%d): Trying to set DHCP\n", __LINE__);
      *p_block_error = PF_DCP_BLOCK_ERROR_OPTION_NOT_SUPPORTED;
      change_dhcp = true;
      break;
   case PF_DCP_OPT_CONTROL:
      if (sub == PF_DCP_SUB_CONTROL_FACTORY_RESET)
      {
         reset_to_factory = true;
         reset_mode = 99;        /* Reset all */

         ret = 0;
      }
      else if (sub == PF_DCP_SUB_CONTROL_RESET_TO_FACTORY)
      {
         reset_to_factory = true;

         ret = 0;
      }
      break;
   default:
      *p_block_error = PF_DCP_BLOCK_ERROR_OPTION_NOT_SUPPORTED;
      break;
   }

   if (ret == 0)
   {
      /* Evaluate what we have and where to go */
      have_name = (strlen(net->cmina_temp_dcp_ase.name_of_station) > 0);
      have_ip = ((net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_addr != 0) ||
                 (net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_mask != 0) ||
                 (net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_gateway != 0));
      have_dhcp = false;   /* ToDo: Not supported here */

      switch (net->cmina_state)
      {
      case PF_CMINA_STATE_SETUP:
         /* Should not occur */
         net->cmina_error_decode = PNET_ERROR_DECODE_PNIORW;
         net->cmina_error_code_1  = PNET_ERROR_CODE_1_ACC_STATE_CONFLICT;
         ret = -1;
         break;
      case PF_CMINA_STATE_SET_NAME:
      case PF_CMINA_STATE_SET_IP:
         if (reset_to_factory == true)
         {
            /* Handle reset to factory here */
            ret = pf_cmina_set_default_cfg(net, reset_mode);
         }

         if ((change_name == true) || (change_ip == true))
         {
            if ((have_name == true) && (have_ip == true))
            {
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
                  net->cmina_state = PF_CMINA_STATE_SET_NAME;
               }
               else if (have_ip == false)
               {
                  net->cmina_state = PF_CMINA_STATE_SET_IP;
               }
            }
         }
         else if (change_dhcp == true)
         {
            if (have_dhcp == true)
            {
               memset(&net->cmina_temp_dcp_ase.full_ip_suite.ip_suite, 0, sizeof(net->cmina_temp_dcp_ase.full_ip_suite.ip_suite));
               if (temp == false)
               {
                  net->cmina_perm_dcp_ase.full_ip_suite.ip_suite = net->cmina_temp_dcp_ase.full_ip_suite.ip_suite;
               }
               /* dhcp_discover_req */
               net->cmina_state = PF_CMINA_STATE_SET_IP;
            }
            else
            {
               /* Stop DHCP */
            }
         }
         break;
      case PF_CMINA_STATE_W_CONNECT:
         if (reset_to_factory == true)
         {
            /* Handle reset to factory here */
            ret = pf_cmina_set_default_cfg(net, reset_mode);
         }

         if ((change_name == false) && (change_ip == false))
         {
            /* all OK */
         }
         else if ((have_name == false) || (reset_to_factory == true))
         {
            /* Any connection active ? */
            found = false;
            for (ix = 0; ix < PNET_MAX_AR; ix++)
            {
               p_ar = pf_ar_find_by_index(net, ix);
               if ((p_ar != NULL) && (p_ar->in_use == true))
               {
                  found = true;
                  /* 38 */
                  pf_cmdev_state_ind(net, p_ar, PNET_EVENT_ABORT);
               }
            }

            if (found == false)
            {
               /* 37 */
               /* Stop IP */
            }
         }
         else if (((have_name == true) && (change_name == true)) ||
               (have_ip == false))
         {
            /* Any connection active ? */
            found = false;
            for (ix = 0; ix < PNET_MAX_AR; ix++)
            {
               p_ar = pf_ar_find_by_index(net, ix);
               if ((p_ar != NULL) && (p_ar->in_use == true))
               {
                  found = true;
                  /* 40 */
                  pf_cmdev_state_ind(net, p_ar, PNET_EVENT_ABORT);
               }
            }

            if (found == false)
            {
               /* 39 */
               net->cmina_commit_ip_suite = true;
            }

            net->cmina_state = PF_CMINA_STATE_SET_IP;

            ret = 0;
         }
         else if ((have_ip == true) && (change_ip == true))
         {
            /* Any connection active ?? */
            found = false;
            for (ix = 0; ix < PNET_MAX_AR; ix++)
            {
               p_ar = pf_ar_find_by_index(net, ix);
               if ((p_ar != NULL) && (p_ar->in_use == true))
               {
                  found = true;
               }
            }

            if (found == false)
            {
               /* 41 */
               net->cmina_commit_ip_suite = true;

               ret = 0;
            }
            else
            {
               /* 42 */
               /* Not allowed to change IP if AR active */
               net->cmina_error_decode = PNET_ERROR_DECODE_PNIORW;
               net->cmina_error_code_1  = PNET_ERROR_CODE_1_ACC_INVALID_PARAMETER;
               ret = -1;
            }
         }
         else
         {
            /* 43 */
            net->cmina_error_decode = PNET_ERROR_DECODE_PNIORW;
            net->cmina_error_code_1  = PNET_ERROR_CODE_1_ACC_INVALID_PARAMETER;
            ret = -1;
         }
         break;
      }
   }

   return ret;
}

int pf_cmina_dcp_get_req(
   pnet_t                  *net,
   uint8_t                 opt,
   uint8_t                 sub,
   uint16_t                *p_value_length,
   uint8_t                 **pp_value,
   uint8_t                 *p_block_error)
{
   int                     ret = 0;    /* Assume all OK */

   switch (opt)
   {
   case PF_DCP_OPT_ALL:
      break;
   case PF_DCP_OPT_IP:
      switch (sub)
      {
      case PF_DCP_SUB_IP_MAC:
         *p_value_length = sizeof(net->cmina_temp_dcp_ase.mac_address);
         *pp_value = (uint8_t *)&net->cmina_temp_dcp_ase.mac_address;
         break;
      case PF_DCP_SUB_IP_PAR:
         *p_value_length = sizeof(net->cmina_temp_dcp_ase.full_ip_suite.ip_suite);
         *pp_value = (uint8_t *)&net->cmina_temp_dcp_ase.full_ip_suite.ip_suite;
         break;
      case PF_DCP_SUB_IP_SUITE:
         *p_value_length = sizeof(net->cmina_temp_dcp_ase.full_ip_suite);
         *pp_value = (uint8_t *)&net->cmina_temp_dcp_ase.full_ip_suite;
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
         *p_value_length = sizeof(net->cmina_temp_dcp_ase.device_vendor) - 1;   /* Skip terminator */
         *pp_value = (uint8_t *)net->cmina_temp_dcp_ase.device_vendor;
         break;
      case PF_DCP_SUB_DEV_PROP_NAME:
         *p_value_length = sizeof(net->cmina_temp_dcp_ase.name_of_station);
         *pp_value = (uint8_t *)net->cmina_temp_dcp_ase.name_of_station;
         break;
      case PF_DCP_SUB_DEV_PROP_ID:
         *p_value_length = sizeof(net->cmina_temp_dcp_ase.device_id);
         *pp_value = (uint8_t *)&net->cmina_temp_dcp_ase.device_id;
         break;
      case PF_DCP_SUB_DEV_PROP_ROLE:
         *p_value_length = sizeof(net->cmina_temp_dcp_ase.device_role);
         *pp_value = (uint8_t *)&net->cmina_temp_dcp_ase.device_role;
         break;
      case PF_DCP_SUB_DEV_PROP_OPTIONS:
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         ret = -1;
         break;
      case PF_DCP_SUB_DEV_PROP_ALIAS:
#if 0
         *p_value_length = sizeof(net->cmina_temp_dcp_ase.);
         *pp_value = &net->cmina_temp_dcp_ase.;
#else
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         ret = -1;
#endif
         break;
      case PF_DCP_SUB_DEV_PROP_INSTANCE:
         *p_value_length = sizeof(net->cmina_temp_dcp_ase.instance_id);
         *pp_value = (uint8_t *)&net->cmina_temp_dcp_ase.instance_id;
         break;
      case PF_DCP_SUB_DEV_PROP_OEM_ID:
         *p_value_length = sizeof(net->cmina_temp_dcp_ase.oem_device_id);
         *pp_value = (uint8_t *)&net->cmina_temp_dcp_ase.oem_device_id;
         break;
      case PF_DCP_SUB_DEV_PROP_GATEWAY:
         *p_value_length = sizeof(net->cmina_temp_dcp_ase.standard_gw_value);
         *pp_value = (uint8_t *)&net->cmina_temp_dcp_ase.standard_gw_value;
         break;
      default:
         *p_block_error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;
         ret = -1;
         break;
      }
      break;
   case PF_DCP_OPT_DHCP:
      /* ToDo: No support for DHCP yet, because it needs a way to get/set in lpiw. */
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
         /* Cant get this */
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
      /* Cant get control */
      *p_block_error = PF_DCP_BLOCK_ERROR_OPTION_NOT_SUPPORTED;
      ret = -1;
      break;
   case PF_DCP_OPT_DEVICE_INITIATIVE:
      switch (sub)
      {
      case PF_DCP_SUB_DEV_INITIATIVE_SUPPORT:
         *p_value_length = sizeof(net->cmina_temp_dcp_ase.device_initiative);
         *pp_value = (uint8_t *)&net->cmina_temp_dcp_ase.device_initiative;
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

int pf_cmina_get_station_name(
   pnet_t                  *net,
   const char **pp_station_name)
{
   *pp_station_name = net->cmina_temp_dcp_ase.name_of_station;
   return 0;
}

int pf_cmina_get_ipaddr(
   pnet_t                  *net,
   os_ipaddr_t             *p_ipaddr)
{
   *p_ipaddr = net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_addr;
   return 0;
}


/*************** Diagnostic strings *****************************************/

/**
 * @internal
 * Return a string representation of the CMINA state.
 * @param net              InOut: The p-net stack instance
 * @return  a string representation of the CMINA state.
 */
static const char *pf_cmina_state_to_string(
   pnet_t                  *net)
{
   const char *s = "<unknown>";
   switch (net->cmina_state)
   {
   case PF_CMINA_STATE_SETUP: s = "PF_CMINA_STATE_SETUP"; break;
   case PF_CMINA_STATE_SET_NAME: s = "PF_CMINA_STATE_SET_NAME"; break;
   case PF_CMINA_STATE_SET_IP: s = "PF_CMINA_STATE_SET_IP"; break;
   case PF_CMINA_STATE_W_CONNECT: s = "PF_CMINA_STATE_W_CONNECT"; break;
   }

   return s;
}

/**
 * @internal
 * Print an IPv4 address (without newline)
 *
 * @param ip      In: IP address
*/
void pf_ip_address_show(uint32_t ip){
   printf("%u.%u.%u.%u",
      (unsigned)((ip >> 24) & 0xFF),
      (unsigned)((ip >> 16) & 0xFF),
      (unsigned)((ip >> 8) & 0xFF),
      (unsigned)(ip & 0xFF));
}

void pf_cmina_show(
   pnet_t                  *net)
{
   const pnet_cfg_t        *p_cfg = NULL;

   pf_fspm_get_default_cfg(net, &p_cfg);

   printf("CMINA state : %s\n\n", pf_cmina_state_to_string(net));

   printf("Default name_of_station        : <%s>\n", p_cfg->station_name);
   printf("Perm name_of_station           : <%s>\n", net->cmina_perm_dcp_ase.name_of_station);
   printf("Temp name_of_station           : <%s>\n", net->cmina_temp_dcp_ase.name_of_station);
   printf("\n");
   printf("Default device_vendor          : <%s>\n", p_cfg->device_vendor);
   printf("Perm device_vendor             : <%s>\n", net->cmina_perm_dcp_ase.device_vendor);
   printf("Temp device_vendor             : <%s>\n", net->cmina_temp_dcp_ase.device_vendor);
   printf("\n");
   printf("Default IP  Netmask  Gateway   : %u.%u.%u.%u  ",
      (unsigned)p_cfg->ip_addr.a, (unsigned)p_cfg->ip_addr.b, (unsigned)p_cfg->ip_addr.c, (unsigned)p_cfg->ip_addr.d);
   printf("%u.%u.%u.%u  ",
      (unsigned)p_cfg->ip_mask.a, (unsigned)p_cfg->ip_mask.b, (unsigned)p_cfg->ip_mask.c, (unsigned)p_cfg->ip_mask.d);
   printf("%u.%u.%u.%u\n",
      (unsigned)p_cfg->ip_gateway.a, (unsigned)p_cfg->ip_gateway.b, (unsigned)p_cfg->ip_gateway.c, (unsigned)p_cfg->ip_gateway.d);
   printf("Perm    IP  Netmask  Gateway   : ");
   pf_ip_address_show(net->cmina_perm_dcp_ase.full_ip_suite.ip_suite.ip_addr);
   printf("  ");
   pf_ip_address_show(net->cmina_perm_dcp_ase.full_ip_suite.ip_suite.ip_mask);
   printf("  ");
   pf_ip_address_show(net->cmina_perm_dcp_ase.full_ip_suite.ip_suite.ip_gateway);
   printf("\nTemp    IP  Netmask  Gateway   : ");
   pf_ip_address_show(net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_addr);
   printf("  ");
   pf_ip_address_show(net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_mask);
   printf("  ");
   pf_ip_address_show(net->cmina_temp_dcp_ase.full_ip_suite.ip_suite.ip_gateway);
   printf("\n");
   printf("MAC                            : %02x:%02x:%02x:%02x:%02x:%02x\n",
          p_cfg->eth_addr.addr[0],
          p_cfg->eth_addr.addr[1],
          p_cfg->eth_addr.addr[2],
          p_cfg->eth_addr.addr[3],
          p_cfg->eth_addr.addr[4],
          p_cfg->eth_addr.addr[5]);
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
 * Also empty station name is valid here (indicating that no station name has been set).
 *
 * @param station_name     In: Station name
 * @param len              In: Length of incoming string without terminator.
 * @return  0  if the name is valid
 *          -1 if the name is invalid
 */

bool pf_cmina_is_stationname_valid(
   const char*             station_name,
   uint16_t                len)
{
   uint16_t i;
   uint16_t label_offset = 0;
   char prev = 'a';
   uint16_t n_labels = 1;
   uint16_t n_non_digit = 0;

   /* - Empty station name is valid here (indicating that no station name has been set) */
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
   if ((station_name[0] == '-') || (len > 240))
   {
      return false;
   }

   /*
    * - The first label does not have the form "port-xyz" or "port-xyz-abcde"
    *   with a, b, c, d, e, x, y, z = 0...9
    */
   if (strncmp(station_name, "port-", 5) == 0)
   {
      if ((len == 8) || (len == 14))
      {
         uint16_t n_char = 0;
         for (i = 5; i < len; i++)
         {
            if ((!isdigit(station_name[i])) &&
                !(station_name[i]== '-'))
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
      char c = station_name[i];
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
      else if (!((islower(c) || isdigit(c) || (c == '-'))))
      {
         return false;
      }
      else
      {
         /* - Labels do not start with [-] */
         if ((label_offset == 0) &&
             (c == '-'))
         {
            return false;
         }

         /* - Label length is 1 to 63 */
         if (label_offset >= 63)
         {
            return false;
         }

         if (!isdigit(c))
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
 * Check if an IP suite is valid.
 *
 * Defined in Profinet 2.4, section 4.3.1.4.21.5
 *
 * @param ipsuite          In: IP suite with IP, netmask, gateway
 * @return  0  if the IP suite is valid
 *          -1 if the IP suite is invalid
 */
bool pf_cmina_is_ipsuite_valid(
   pf_ip_suite_t           *p_ipsuite)
{
   if (!pf_cmina_is_netmask_valid(p_ipsuite->ip_mask))
   {
      return false;
   }
   if (!pf_cmina_is_ipaddress_valid(p_ipsuite->ip_mask, p_ipsuite->ip_addr))
   {
      return false;
   }
   if (!pf_cmina_is_gateway_valid(p_ipsuite->ip_addr, p_ipsuite->ip_mask, p_ipsuite->ip_gateway))
   {
      return false;
   }

   return true;
}

/**
 * @internal
 * Check if a full IP suite is valid.
 *
 * Defined in Profinet 2.4, section
 *
 * @param full_ipsuite     In: IP suite with IP, netmask, gateway and DNS adresses
 * @return  0  if the IP suite is valid
 *          -1 if the IP suite is invalid
 */
bool pf_cmina_is_full_ipsuite_valid(
   pf_full_ip_suite_t      *p_full_ipsuite)
{
   if (!pf_cmina_is_ipsuite_valid(&p_full_ipsuite->ip_suite))
   {
      return false;
   }

   /* TODO validate DNS addresses */

   return true;
}

/**
 * @internal
 * Check if an IP address is valid
 *
 * Defined in Profinet 2.4, section 4.3.1.4.21.2
 *
 * @param netmask          In: Netmask
 * @param netmask          In: IP address
 * @return  0  if the IP address is valid
 *          -1 if the IP address is invalid
 */
bool pf_cmina_is_ipaddress_valid(
   os_ipaddr_t            netmask,
   os_ipaddr_t            ip)
{
   uint32_t host_part = ip & ~netmask;

   if ((netmask == 0) && (ip == 0))
   {
      return true;
   }
   if (!pf_cmina_is_netmask_valid(netmask))
   {
      return false;
   }
   if ((host_part == 0) || (host_part == ~netmask))
   {
      return false;
   }
   if (ip <= OS_MAKEU32(0, 255, 255, 255))
   {
      return false;
   }
   else if ((ip >= OS_MAKEU32(127, 0, 0, 0)) &&
            (ip <= OS_MAKEU32(127, 255, 255, 255)))
   {
      return false;
   }
   else if ((ip >=OS_MAKEU32(224, 0, 0, 0)) &&
            (ip <= OS_MAKEU32(239, 255, 255, 255)))
   {
      return false;
   }
   else if ((ip >= OS_MAKEU32(240, 0, 0, 0)) &&
            (ip <= OS_MAKEU32(255, 255, 255, 255)))
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
bool pf_cmina_is_netmask_valid(
   os_ipaddr_t            netmask)
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
bool pf_cmina_is_gateway_valid(
   os_ipaddr_t             ip,
   os_ipaddr_t             netmask,
   os_ipaddr_t             gateway)
{
   if ((gateway != 0) &&
       ((ip & netmask) != (gateway & netmask)))
   {
      return false;
   }
   return true;
}
