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

#ifdef UNIT_TEST
#define pnal_eth_get_status      mock_pnal_eth_get_status
#define pnal_get_port_statistics mock_pnal_get_port_statistics
#define pf_bg_worker_start_job   mock_pf_bg_worker_start_job
#endif

#include "pf_includes.h"
#include "pf_block_writer.h"
#include "pf_block_reader.h"
#include <string.h>

/**
 * @file
 * @brief Management Physical Device Port (PD Port) data
 */

/**
 * Get configuration file name for one port
 *
 * Asserts for invalid port number.
 *
 * @param loc_port_num      In:   Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @return  The configuration file name
 */
static const char * pf_pdport_get_filename (int loc_port_num)
{
   const char * filename = NULL;
   switch (loc_port_num)
   {
   case PNET_PORT_1:
      filename = PF_FILENAME_PDPORT_1;
      break;
   case PNET_PORT_2:
      filename = PF_FILENAME_PDPORT_2;
      break;
   case PNET_PORT_3:
      filename = PF_FILENAME_PDPORT_3;
      break;
   case PNET_PORT_4:
      filename = PF_FILENAME_PDPORT_4;
      break;
   default:
      CC_ASSERT (false);
   }
   return filename;
}

/**
 * Load PDPort data for one port from nvm
 *
 * Enables or disable LLDP transmission.
 * Checks presence of peer.
 *
 * Asserts for invalid port number.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_pdport_load (pnet_t * net, int loc_port_num)
{
   int ret = -1;
   pf_pdport_t pdport_config;
   const char * p_file_directory = pf_cmina_get_file_directory (net);
   pf_port_t * p_port_data = NULL;

   if (
      pf_file_load (
         p_file_directory,
         pf_pdport_get_filename (loc_port_num),
         &pdport_config,
         sizeof (pdport_config)) == 0)
   {
      p_port_data = pf_port_get_state (net, loc_port_num);

      memcpy (&p_port_data->pdport, &pdport_config, sizeof (pdport_config));
      p_port_data->pdport.lldp_peer_info_updated = false;

      if (p_port_data->pdport.check.active == true)
      {
         LOG_DEBUG (
            PNET_LOG,
            "PDPORT(%d): Did read PDPort settings from nvm. Monitoring peer "
            "on port %u. Station name: \"%.*s\" Port: \"%.*s\"\n",
            __LINE__,
            loc_port_num,
            p_port_data->pdport.check.peer.length_peer_station_name,
            p_port_data->pdport.check.peer.peer_station_name,
            p_port_data->pdport.check.peer.length_peer_port_name,
            p_port_data->pdport.check.peer.peer_port_name);

         /* Check peers directly at the next stack invocation. As this is done
            at startup, there is likely no peer info available and a diagnosis
            will be set.
            Note that the application must have time to plug the relevant
            submodule before any diagnosis can be triggered, why we must
            wait to next periodic invocation before doing the check. */
         pf_lldp_restart_peer_timeout (net, loc_port_num, 0);
      }
      else
      {
         LOG_DEBUG (
            PNET_LOG,
            "PDPORT(%d): Did read PDPort settings from nvm. No monitoring "
            "of peer on port %u.\n",
            __LINE__,
            loc_port_num);
      }

      if (p_port_data->pdport.adjust.active == true)
      {
         if (
            p_port_data->pdport.adjust.peer_to_peer_boundary
               .peer_to_peer_boundary.do_not_send_LLDP_frames == 1)
         {
            pf_lldp_send_disable (net, loc_port_num);
         }
         else
         {
            pf_lldp_send_enable (net, loc_port_num);
         }
      }
      else
      {
         pf_lldp_send_enable (net, loc_port_num);
      }

      ret = 0;
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "PDPORT(%d): Could not read from nvm, so no monitoring of "
         "peer on port %u.\n",
         __LINE__,
         loc_port_num);
      pf_lldp_send_enable (net, loc_port_num);
   }

   return ret;
}

/**
 * Store PDPort data for one port to nvm
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_pdport_save (pnet_t * net, int loc_port_num)
{
   int ret = 0;
   int save_result = 0;
   pf_pdport_t pdport_config = {0};
   pf_pdport_t temporary_buffer;
   const char * p_file_directory = pf_cmina_get_file_directory (net);
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   os_mutex_lock (net->pf_interface.port_mutex);
   memcpy (&pdport_config, &p_port_data->pdport, sizeof (pdport_config));
   os_mutex_unlock (net->pf_interface.port_mutex);

   save_result = pf_file_save_if_modified (
      p_file_directory,
      pf_pdport_get_filename (loc_port_num),
      &pdport_config,
      &temporary_buffer,
      sizeof (pf_pdport_t));

   switch (save_result)
   {
   case 2:
      LOG_INFO (
         PNET_LOG,
         "PDPORT(%d): First nvm saving of port settings.\n",
         __LINE__);
      break;
   case 1:
      LOG_INFO (
         PNET_LOG,
         "PDPORT(%d): Updating nvm stored port settings.\n",
         __LINE__);
      break;
   case 0:
      LOG_DEBUG (
         PNET_LOG,
         "PDPORT(%d): No storing of nvm port settings (no changes).\n",
         __LINE__);
      break;
   default:
   case -1:
      ret = -1;
      LOG_ERROR (
         PNET_LOG,
         "PDPORT(%d): Failed to store nvm port settings.\n",
         __LINE__);
      break;
   }
   return ret;
}

int pf_pdport_save_all (pnet_t * net)
{
   int port;
   pf_port_iterator_t port_iterator;

   if (net->pf_interface.port_mutex == NULL)
   {
      /* Handle case during init when port data is not yet initialized. */
      return -1;
   }

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   while (port != 0)
   {
      (void)pf_pdport_save (net, port);
      port = pf_port_get_next (&port_iterator);
   }
   return 0;
}

bool pf_pdport_is_a_fast_port_in_use (pnet_t * net)
{
   int loc_port_num;
   pf_port_iterator_t port_iterator;
   pnal_eth_status_t eth_status;
   bool found = false;
   bool fast_enough = false;
#if LOG_DEBUG_ENABLED(PNET_LOG)
   const pnet_port_cfg_t * port_config;
#endif

   pf_port_init_iterator_over_ports (net, &port_iterator);
   loc_port_num = pf_port_get_next (&port_iterator);
   while (loc_port_num != 0)
   {
      eth_status = pf_pdport_get_eth_status (net, loc_port_num);
      fast_enough = eth_status.operational_mau_type >=
                    PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX;

#if LOG_DEBUG_ENABLED(PNET_LOG)
      port_config = pf_port_get_config (net, loc_port_num);
      LOG_DEBUG (
         PNET_LOG,
         "PDPORT(%d): Checking local Ethernet port %u \"%s\". MAU type: %u  "
         "%s.  Running: %s.\n",
         __LINE__,
         loc_port_num,
         port_config->netif_name,
         eth_status.operational_mau_type,
         fast_enough ? "Fast enough" : "Too slow",
         eth_status.running ? "Yes" : "No");
#endif
      if (fast_enough && eth_status.running)
      {
         /* Do not return early, to list all ports in the debug output */
         found = true;
      }

      loc_port_num = pf_port_get_next (&port_iterator);
   }

   return found;
}

/**
 * @internal
 * Initialize pdport diagnostic source
 *
 * If the local port number is out of range this operation will assert.
 *
 * @param net              InOut: The p-net stack instance
 * @param diag_source      InOut: Diag source to be initialized
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
static void pf_pdport_init_diag_source (
   pnet_t * net,
   pnet_diag_source_t * diag_source,
   int loc_port_num)
{
   diag_source->api = PNET_API_NO_APPLICATION_PROFILE;
   diag_source->slot = PNET_SLOT_DAP_IDENT;
   diag_source->subslot =
      pf_port_loc_port_num_to_dap_subslot (net, loc_port_num);
   diag_source->ch = PNET_CHANNEL_WHOLE_SUBMODULE;
   diag_source->ch_grouping = PNET_DIAG_CH_INDIVIDUAL_CHANNEL;
   diag_source->ch_direction = PNET_DIAG_CH_PROP_DIR_MANUF_SPEC;
}

/**
 * Delete all active diagnostics handled by pdport
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports.
 */
static void pf_pdport_remove_all_diag (pnet_t * net, int loc_port_num)
{
   pnet_diag_source_t diag_source = {0};

   pf_pdport_init_diag_source (net, &diag_source, loc_port_num);

   LOG_DEBUG (
      PNET_LOG,
      "PDPORT(%d): Remove diagnosis about remote mismatch (if any) "
      "for port %u.\n",
      __LINE__,
      loc_port_num);

   (void)pf_diag_std_remove (
      net,
      &diag_source,
      PF_WRT_ERROR_REMOTE_MISMATCH,
      PF_WRT_ERROR_PEER_STATIONNAME_MISMATCH);

   (void)pf_diag_std_remove (
      net,
      &diag_source,
      PF_WRT_ERROR_REMOTE_MISMATCH,
      PF_WRT_ERROR_PEER_PORTNAME_MISMATCH);

   (void)pf_diag_std_remove (
      net,
      &diag_source,
      PF_WRT_ERROR_REMOTE_MISMATCH,
      PF_WRT_ERROR_NO_PEER_DETECTED);

   (void)pf_diag_std_remove (
      net,
      &diag_source,
      PF_WRT_ERROR_REMOTE_MISMATCH,
      PF_WRT_ERROR_PEER_MRP_DOMAIN_MISMATCH);
}

/**
 * Init PDPort data.
 * Load port configuration from nvm.

 * If a check is loaded it will be run when PDPort receives data from LLDP.
 *
 * @param net              InOut: The p-net stack instance
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred
 */
int pf_pdport_init (pnet_t * net)
{
   int port;
   pf_port_iterator_t port_iterator;

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   while (port != 0)
   {
      if (pf_pdport_load (net, port) != 0)
      {
         /* Create file if missing */
         (void)pf_pdport_save (net, port);
      }

      port = pf_port_get_next (&port_iterator);
   }
   return 0;
}

int pf_pdport_reset_all (pnet_t * net)
{
   int port;
   pf_port_iterator_t port_iterator;
   pf_port_t * p_port_data = NULL;

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);
      memset (&p_port_data->pdport, 0, sizeof (p_port_data->pdport));
      pf_pdport_remove_all_diag (net, port);

      port = pf_port_get_next (&port_iterator);
   }

   (void)pf_bg_worker_start_job (net, PF_BGJOB_SAVE_PDPORT_NVM_DATA);

   return 0;
}

void pf_pdport_remove_data_files (const char * file_directory)
{
   int port;

   /* Do not use port iterator as net is not available */
   for (port = PNET_PORT_1; port <= PNET_MAX_PHYSICAL_PORTS; port++)
   {
      pf_file_clear (file_directory, pf_pdport_get_filename (port));
   }
}

static void pf_pdport_reset_ar (pnet_t * net, pf_ar_t const * p_ar)
{
   int port;
   pf_port_t * p_port_data = NULL;
   pf_exp_ident_t * exp_ident;
   pf_exp_api_t * exp_api;
   pf_exp_module_t * exp_mod;
   pf_exp_submodule_t * exp_submod;
   int a, m, s;

   exp_ident = &p_ar->io_ar->exp_ident;
   for (a = 0; a < exp_ident->nbr_apis; ++a)
   {
      exp_api = &exp_ident->api[a];
      if (exp_api->api == PNET_API_NO_APPLICATION_PROFILE)
      {
         for (m = 0; m < exp_api->nbr_modules; ++m)
         {
            exp_mod = &exp_api->module[m];

            if (exp_mod->ident_number == PNET_MOD_DAP_IDENT)
            {
               for (s = 0; s < exp_mod->nbr_submodules; ++s)
               {
                  exp_submod = &exp_mod->submodule[s];

                  port = pf_port_dap_subslot_to_local_port (
                     net,
                     exp_submod->subslot_number);
                  if (port != 0)
                  {
                     p_port_data = pf_port_get_state (net, port);
                     memset (
                        &p_port_data->pdport,
                        0,
                        sizeof (p_port_data->pdport));
                     pf_pdport_remove_all_diag (net, port);
                  }
               }
            }
         }
      }
   }
   (void)pf_bg_worker_start_job (net, PF_BGJOB_SAVE_PDPORT_NVM_DATA);
}

void pf_pdport_ar_connect_ind (pnet_t * net, const pf_ar_t * p_ar)
{
   (void)p_ar;
   int port;
   pf_port_iterator_t port_iterator;
   pf_port_t * p_port_data = NULL;

   if ((p_ar->io_ar != NULL) && (p_ar->ar_state != PF_AR_STATE_BACKUP))
   {
      pf_pdport_reset_ar (net, p_ar);
   }

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);

   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);
      if (p_port_data->pdport.adjust.active == true)
      {
         if (
            p_port_data->pdport.adjust.peer_to_peer_boundary
               .peer_to_peer_boundary.do_not_send_LLDP_frames == 1)
         {
            pf_lldp_send_disable (net, port);
         }
         else
         {
            pf_lldp_send_enable (net, port);
         }
      }
      port = pf_port_get_next (&port_iterator);
   }
}

void pf_pdport_lldp_restart_transmission (pnet_t * net)
{
   int port;
   pf_port_iterator_t port_iterator;
   pf_port_t * p_port_data = NULL;

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);

   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);

      if (p_port_data->pdport.adjust.active == true)
      {
         if (
            p_port_data->pdport.adjust.peer_to_peer_boundary
               .peer_to_peer_boundary.do_not_send_LLDP_frames == 1)
         {
            pf_lldp_send_disable (net, port);
         }
         else
         {
            pf_lldp_send_enable (net, port);
         }
      }
      else
      {
         pf_lldp_send_enable (net, port);
      }

      port = pf_port_get_next (&port_iterator);
   }
}

int pf_pdport_read_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_read_request_t * p_read_req,
   pf_put_info_t * p_res,
   uint16_t * p_pos)
{
   int ret = -1;
   int loc_port_num;
   pf_port_iterator_t port_iterator;
   pnal_port_stats_t port_statistics;
   pf_port_t * p_port_data;
   uint16_t slot = p_read_req->slot_number;
   uint16_t subslot = p_read_req->subslot_number;
   uint16_t index = p_read_req->index;
   const char * netif_name;
   pf_lldp_station_name_t peer_station_name;
   pf_lldp_port_name_t peer_port_name;
   pnal_eth_status_t eth_status;
   char station_name[PNET_STATION_NAME_MAX_SIZE]; /* Terminated*/
   uint16_t subslot_for_port;

   switch (index)
   {
   case PF_IDX_SUB_PDPORT_DATA_REAL:
      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         (pf_port_subslot_is_dap_port_id (net, subslot)))
      {
         loc_port_num = pf_port_dap_subslot_to_local_port (net, subslot);

         p_port_data = pf_port_get_state (net, loc_port_num);
         eth_status = pf_pdport_get_eth_status_filtered_mau (net, loc_port_num);
         /* Look up peer info. Subfields .len indicates whether peer is found */
         (void)pf_lldp_get_peer_station_name (
            net,
            loc_port_num,
            &peer_station_name);
         (void)pf_lldp_get_peer_port_name (net, loc_port_num, &peer_port_name);

         pf_put_pdport_data_real (
            true,
            subslot,
            &peer_station_name,
            &peer_port_name,
            p_port_data,
            pf_port_get_media_type (eth_status.operational_mau_type),
            &eth_status,
            p_res,
            p_pos);
         ret = 0;
      }
      break;

   case PF_IDX_SUB_PDPORT_DATA_ADJ:
      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         (pf_port_subslot_is_dap_port_id (net, subslot)))
      {
         loc_port_num = pf_port_dap_subslot_to_local_port (net, subslot);
         p_port_data = pf_port_get_state (net, loc_port_num);
         if (p_port_data->pdport.adjust.active)
         {
            pf_put_pdport_data_adj (
               true,
               subslot,
               &p_port_data->pdport.adjust.peer_to_peer_boundary,
               p_res,
               p_pos);
         }
         ret = 0;
      }
      break;

   case PF_IDX_SUB_PDPORT_DATA_CHECK:
      if (slot == PNET_SLOT_DAP_IDENT)
      {
         if (pf_port_subslot_is_dap_port_id (net, subslot))
         {
            loc_port_num = pf_port_dap_subslot_to_local_port (net, subslot);
            p_port_data = pf_port_get_state (net, loc_port_num);
            if (p_port_data->pdport.check.active)
            {
               pf_put_pdport_data_check (
                  true,
                  slot,
                  subslot,
                  &p_port_data->pdport.check.peer,
                  p_res,
                  p_pos);
            }
            ret = 0;
         }
      }
      break;

   case PF_IDX_DEV_PDREAL_DATA:
      /* Combined interface and port info and statistics */
      pf_cmina_get_station_name (net, station_name);
      if (
         pnal_get_port_statistics (
            net->pf_interface.main_port.name,
            &port_statistics) != 0)
      {
         memset (&port_statistics, 0, sizeof (port_statistics));
      }

      pf_put_pd_multiblock_interface_and_statistics (
         true,
         p_read_req->api,
         pf_cmina_get_device_macaddr (net),
         pf_cmina_get_ipaddr (net),
         pf_cmina_get_netmask (net),
         pf_cmina_get_gateway (net),
         station_name,
         &port_statistics,
#if PNET_OPTION_MRP
         &net->mrp_data.config,
#else
         NULL,
#endif
         p_res,
         p_pos);

      pf_port_init_iterator_over_ports (net, &port_iterator);
      loc_port_num = pf_port_get_next (&port_iterator);
      while (loc_port_num != 0)
      {
         subslot_for_port =
            pf_port_loc_port_num_to_dap_subslot (net, loc_port_num);
         p_port_data = pf_port_get_state (net, loc_port_num);
#if PNET_OPTION_MRP
         pf_mrp_port_t * p_port_mrp = &p_port_data->mrp;
#endif
         eth_status = pf_pdport_get_eth_status_filtered_mau (net, loc_port_num);
         if (
            pnal_get_port_statistics (
               p_port_data->netif.name,
               &port_statistics) != 0)
         {
            memset (&port_statistics, 0, sizeof (port_statistics));
         }

         /* Look up peer info. Subfields .len indicates whether peer is found
          */
         (void)pf_lldp_get_peer_station_name (
            net,
            loc_port_num,
            &peer_station_name);
         (void)pf_lldp_get_peer_port_name (net, loc_port_num, &peer_port_name);

         pf_put_pd_multiblock_port_and_statistics (
            true,
            p_read_req->api,
            subslot_for_port,
            &peer_station_name,
            &peer_port_name,
            p_port_data,
            pf_port_get_media_type (eth_status.operational_mau_type),
            &eth_status,
            &port_statistics,
#if PNET_OPTION_MRP
            p_port_mrp,
#else
            NULL,
#endif
            p_res,
            p_pos);

         loc_port_num = pf_port_get_next (&port_iterator);
      }
      ret = 0;
      break;

   case PF_IDX_DEV_PDEXP_DATA:
      LOG_INFO (
         PNET_LOG,
         "PDPORT(%d): PD exp data is not yet implemented.\n",
         __LINE__);

      /* ToDo: Implement when LLDP is done */
      /*
       * MultipleBlockHeader, { [PDPortDataCheck] a, [PDPortDataAdjust] a,
       *                        [PDInterfaceMrpDataAdjust],
       * [PDInterfaceMrpDataCheck], [PDPortMrpDataAdjust],
       * [PDPortFODataAdjust], [PDPortFODataCheck], [PDNCDataCheck],
       *                        [PDInterface-FSUDataAdjust],
       * [PDInterfaceAdjust], [PDPortMrpIcDataAdjust], [PDPortMrpIcDataCheck]
       * } a The fields SlotNumber and SubslotNumber shall be ignored b Each
       * submodule's data (for example interface or port) need its own
       * MultipleBlockHeader
       */
      ret = 0;
      break;

   case PF_IDX_SUB_PDPORT_STATISTIC:
      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         ((subslot == PNET_SUBSLOT_DAP_INTERFACE_1_IDENT) ||
          (pf_port_subslot_is_dap_port_id (net, subslot))))
      {
         loc_port_num = pf_port_dap_subslot_to_local_port (net, subslot);

         if (loc_port_num == 0)
         {
            netif_name = net->pf_interface.main_port.name;
         }
         else
         {
            p_port_data = pf_port_get_state (net, loc_port_num);
            netif_name = p_port_data->netif.name;
         }

         if (pnal_get_port_statistics (netif_name, &port_statistics) == 0)
         {
            pf_put_pdport_statistics (
               true,
               &port_statistics,
               p_res,
               p_pos);
            ret = 0;
         }
      }
      break;

   case PF_IDX_SUB_PDINTF_ADJUST:
      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         (subslot == PNET_SUBSLOT_DAP_INTERFACE_1_IDENT))
      {
         if (net->pf_interface.name_of_device_mode.active == true)
         {
            pf_put_pd_interface_adj (
               true,
               PNET_SUBSLOT_DAP_INTERFACE_1_IDENT,
               net->pf_interface.name_of_device_mode.mode,
               p_res,
               p_pos);
         }
         ret = 0;
      }
      break;

   case PF_IDX_SUB_PDPORTDATAREALEXT:
      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         (pf_port_subslot_is_dap_port_id (net, subslot)))
      {
         loc_port_num = pf_port_dap_subslot_to_local_port (net, subslot);

         p_port_data = pf_port_get_state (net, loc_port_num);
         eth_status = pf_pdport_get_eth_status_filtered_mau (net, loc_port_num);
         /* Look up peer info. Subfields .len indicates whether peer is found */
         (void)pf_lldp_get_peer_station_name (
            net,
            loc_port_num,
            &peer_station_name);
         (void)pf_lldp_get_peer_port_name (net, loc_port_num, &peer_port_name);

         pf_put_pdport_data_real_extended (
            true,
            subslot,
            &peer_station_name,
            &peer_port_name,
            p_port_data,
            pf_port_get_media_type (eth_status.operational_mau_type),
            &eth_status,
            p_res,
            p_pos);
         ret = 0;
      }
      break;

   default:
      break;
   }
   return ret;
}

/**
 * @internal
 * Set no-peer-detected diagnosis if peer is missing.
 * Remove the diagnosis if peer is available.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
static void pf_pdport_check_no_peer_detected (pnet_t * net, int loc_port_num)
{
   pnet_diag_source_t diag_source;
   uint32_t peer_timestamp;

   pf_pdport_init_diag_source (net, &diag_source, loc_port_num);

   if (pf_lldp_get_peer_timestamp (net, loc_port_num, &peer_timestamp) == 0)
   {
      /* Peer available, remove no-peer-detected diagnosis */
      LOG_DEBUG (
         PNET_LOG,
         "PDPORT(%d): Peer is available on port %u. Remove no-peer-detected "
         "diagnosis, if any.\n",
         __LINE__,
         loc_port_num);
      (void)pf_diag_std_remove (
         net,
         &diag_source,
         PF_WRT_ERROR_REMOTE_MISMATCH,
         PF_WRT_ERROR_NO_PEER_DETECTED);
   }
   else
   {
      /* Peer missing, set no-peer-detected diagnosis */
      LOG_INFO (
         PNET_LOG,
         "PDPORT(%d): Setting no-peer-detected diagnosis for port %u\n",
         __LINE__,
         loc_port_num);
      (void)pf_diag_std_add (
         net,
         &diag_source,
         PNET_DIAG_CH_PROP_TYPE_UNSPECIFIED,
         PNET_DIAG_CH_PROP_MAINT_FAULT,
         PF_WRT_ERROR_REMOTE_MISMATCH,
         PF_WRT_ERROR_NO_PEER_DETECTED,
         0,
         0);
   }
}

/**
 * @internal
 * Run peer station name check
 *
 * Compare LLDP peer information with configured pdport peer.
 * Trigger a diagnosis if wrong peer is connected.
 * Remove diagnosis if expected peer data is received.
 *
 * Does nothing if no peer is available.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
static void pf_pdport_check_peer_station_name (pnet_t * net, int loc_port_num)
{
   bool peer_stationname_is_correct = false;
   pnet_diag_source_t diag_source = {0};
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   pf_check_peer_t * p_wanted_peer = &p_port_data->pdport.check.peer;
   pf_lldp_station_name_t lldp_station_name;

   if (pf_lldp_get_peer_station_name (net, loc_port_num, &lldp_station_name) == 0)
   {
      pf_pdport_init_diag_source (net, &diag_source, loc_port_num);

      if (
         strncmp (
            lldp_station_name.string,
            (char *)p_wanted_peer->peer_station_name,
            lldp_station_name.len) == 0)
      {
         peer_stationname_is_correct = true;
      }

      if (peer_stationname_is_correct == false)
      {
         LOG_INFO (
            PNET_LOG,
            "PDPORT(%d): Setting peer station name mismatch diagnosis: "
            "\"%.*s\". "
            "Wanted peer station name: \"%.*s\"\n",
            __LINE__,
            (int)lldp_station_name.len,
            lldp_station_name.string,
            p_wanted_peer->length_peer_station_name,
            p_wanted_peer->peer_station_name);

         (void)pf_diag_std_add (
            net,
            &diag_source,
            PNET_DIAG_CH_PROP_TYPE_UNSPECIFIED,
            PNET_DIAG_CH_PROP_MAINT_FAULT,
            PF_WRT_ERROR_REMOTE_MISMATCH,
            PF_WRT_ERROR_PEER_STATIONNAME_MISMATCH,
            0,
            0);
      }
      else
      {
         LOG_DEBUG (
            PNET_LOG,
            "PDPORT(%d): Peer station name is correct: \"%.*s\". "
            "Remove diagnosis, if any.\n",
            __LINE__,
            (int)lldp_station_name.len,
            lldp_station_name.string);
         (void)pf_diag_std_remove (
            net,
            &diag_source,
            PF_WRT_ERROR_REMOTE_MISMATCH,
            PF_WRT_ERROR_PEER_STATIONNAME_MISMATCH);
      }
   }
}

/**
 * @internal
 * Run peer port name check
 *
 * Compare LLDP peer information with configured pdport peer.
 * Trigger an diagnosis if unexpected peer data is connected.
 * Remove diagnosis if expected peer data is received.
 *
 * Does nothing if no peer is available.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
static void pf_pdport_check_peer_port_name (pnet_t * net, int loc_port_num)
{
   bool peer_portname_is_correct = false;
   pnet_diag_source_t diag_source = {0};
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   pf_check_peer_t * p_wanted_peer = &p_port_data->pdport.check.peer;
   pf_lldp_port_id_t lldp_port_id;

   if (pf_lldp_get_peer_port_id (net, loc_port_num, &lldp_port_id) == 0)
   {
      pf_pdport_init_diag_source (net, &diag_source, loc_port_num);

      if (
         strncmp (
            lldp_port_id.string,
         (char *)p_wanted_peer->peer_port_name,
         p_wanted_peer->length_peer_port_name) == 0)
      {
         peer_portname_is_correct = true;
      }

      if (peer_portname_is_correct == false)
      {
         LOG_INFO (
            PNET_LOG,
            "PDPORT(%d): Setting peer port name mismatch diagnosis: \"%.*s\". "
            "Wanted peer port name: \"%.*s\"\n",
            __LINE__,
            (int)lldp_port_id.len,
            lldp_port_id.string,
            p_wanted_peer->length_peer_port_name,
            p_wanted_peer->peer_port_name);

         (void)pf_diag_std_add (
            net,
            &diag_source,
            PNET_DIAG_CH_PROP_TYPE_UNSPECIFIED,
            PNET_DIAG_CH_PROP_MAINT_FAULT,
            PF_WRT_ERROR_REMOTE_MISMATCH,
            PF_WRT_ERROR_PEER_PORTNAME_MISMATCH,
            0,
            0);
      }
      else
      {
         LOG_DEBUG (
            PNET_LOG,
            "PDPORT(%d): Peer port name is correct: \"%.*s\". "
            "Remove diagnosis, if any.\n",
            __LINE__,
            (int)lldp_port_id.len,
            lldp_port_id.string);
         (void)pf_diag_std_remove (
            net,
            &diag_source,
            PF_WRT_ERROR_REMOTE_MISMATCH,
            PF_WRT_ERROR_PEER_PORTNAME_MISMATCH);
      }
   }
}


#if PNET_OPTION_MRP

/* check for domain mismatch on this port compared to what was configured in
 * mrp interface adjust  */
void pf_pdport_check_mrp_domain_mismatch (pnet_t * net, int loc_port_num)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   pf_mrp_port_t * p_mrp = &p_port_data->mrp;

   /* if lldp domain mismatch check is enabled compare desired domain with
    * that of current port neighbor */

   /* only one instance supported currently */
   if (net->mrp_data.check.instance[p_mrp->instance].flags.lldp_domain)
   {
      pnet_diag_source_t diag_source;

      pf_pdport_init_diag_source (net, &diag_source, loc_port_num);

      if (memcmp (
             p_mrp->domain.data,
             p_port_data->lldp.peer_info.mrp_port_status.domain.data,
             sizeof (mrp_domain_uuid_t)))
      {
         char peer_uuid[MRP_UUID_STRING_SIZE];
         char port_uuid[MRP_UUID_STRING_SIZE];

         pf_mrp_domain_uuid_str (
            &p_port_data->lldp.peer_info.mrp_port_status.domain,
            peer_uuid);
         pf_mrp_domain_uuid_str (&p_mrp->domain, port_uuid);

         LOG_DEBUG (
            PF_MRP_LOG,
            "PDPORT(%d): Domain mismatch detected on port %d\n",
            __LINE__,
            loc_port_num);

         LOG_DEBUG (
            PF_MRP_LOG,
            "PDPORT(%d): Peer UUID : %s\n",
            __LINE__,
            peer_uuid);

         LOG_DEBUG (
            PF_MRP_LOG,
            "PDPORT(%d): Port UUID : %s\n",
            __LINE__,
            port_uuid);

         /* set diagnosis alarm */
         (void)pf_diag_std_add (
            net,
            &diag_source,
            PNET_DIAG_CH_PROP_TYPE_UNSPECIFIED,
            PNET_DIAG_CH_PROP_MAINT_FAULT,
            PF_WRT_ERROR_REMOTE_MISMATCH,
            PF_WRT_ERROR_PEER_MRP_DOMAIN_MISMATCH,
            0,
            0);
      }
      else
      {
         /* clear diagnosis error */
         (void)pf_diag_std_remove (
            net,
            &diag_source,
            PF_WRT_ERROR_REMOTE_MISMATCH,
            PF_WRT_ERROR_PEER_MRP_DOMAIN_MISMATCH);
      }
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "PDPORT(%d): We do not check mrp domain.\n",
         __LINE__);
   }
}

#endif

/**
 * @internal
 * Run peer check if active
 *
 * Compare LLDP peer information with configured PDPort peer, if checking is
 * active.
 *
 * Trigger an diagnosis if unexpected peer data is connected.
 * Remove diagnosis if expected peer data is received.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
static void pf_pdport_run_peer_check (pnet_t * net, int loc_port_num)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   if (p_port_data->pdport.check.active == true)
   {
      pf_pdport_check_no_peer_detected (net, loc_port_num);
      pf_pdport_check_peer_port_name (net, loc_port_num);
      pf_pdport_check_peer_station_name (net, loc_port_num);
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "PDPORT(%d): We do not check peer name or portID, so no port-change "
         "diagnosis is triggered.\n",
         __LINE__);
   }

#if PNET_OPTION_MRP
   if (net->mrp_data.check_active)
   {
      pf_pdport_check_mrp_domain_mismatch (net, loc_port_num);
   }
#endif
}

/**
 * @internal
 * Handle that an Ethernet link goes down.
 *
 * Trigger a diagnosis if a check is enabled.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
static void pf_pdport_handle_link_down (pnet_t * net, int loc_port_num)
{
   LOG_INFO (
      PNET_LOG,
      "PDPORT(%d): Link went down on port %u.\n",
      __LINE__,
      loc_port_num);
   pf_lldp_stop_peer_timeout (net, loc_port_num);
   pf_lldp_invalidate_peer_info (net, loc_port_num);
   pf_pdport_peer_indication (net, loc_port_num);
}

/**
 * @internal
 * Handle that an Ethernet link goes up.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
static void pf_pdport_handle_link_up (pnet_t * net, int loc_port_num)
{
   LOG_INFO (
      PNET_LOG,
      "PDPORT(%d): Link went up on port %u.\n",
      __LINE__,
      loc_port_num);
}

/**
 * @internal
 * Monitor the Ethernet link status
 *
 * Set appropriate diagnosis, if check is enabled.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
static void pf_pdport_monitor_link (pnet_t * net, int loc_port_num)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   pnal_eth_status_t eth_status = pf_pdport_get_eth_status (net, loc_port_num);

   if (p_port_data->netif.previous_is_link_up == false && eth_status.running)
   {
      pf_pdport_handle_link_up (net, loc_port_num);
   }
   else if (p_port_data->netif.previous_is_link_up && eth_status.running == false)
   {
      pf_pdport_handle_link_down (net, loc_port_num);
   }

   p_port_data->netif.previous_is_link_up = eth_status.running;
}

/**
 * @internal
 * Trigger monitoring the state of one Ethernet link.
 *
 * Set appropriate diagnosis, if check is enabled.
 * Will loop through all Ethernet links, one per call.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * Re-schedules itself.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    Not used
 * @param current_time     In:    Not used.
 */
static void pf_lldp_trigger_linkmonitor (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{

   int port =
      pf_port_get_next_repeat_cyclic (&net->pf_interface.link_monitor_iterator);

   (void)pf_bg_worker_start_job (net, PF_BGJOB_UPDATE_PORTS_STATUS);

   pf_pdport_monitor_link (net, port);

   if (
      pf_scheduler_add (
         net,
         PF_LINK_MONITOR_INTERVAL,
         pf_lldp_trigger_linkmonitor,
         NULL,
         &net->pf_interface.link_monitor_timeout) != 0)
   {
      LOG_ERROR (
         PF_LLDP_LOG,
         "LLDP(%d): Failed to reschedule Ethernet link check\n",
         __LINE__);
   }
}

void pf_pdport_start_linkmonitor (pnet_t * net)
{
   if (
      pf_scheduler_add (
         net,
         PF_LINK_MONITOR_INTERVAL,
         pf_lldp_trigger_linkmonitor,
         NULL,
         &net->pf_interface.link_monitor_timeout) != 0)
   {
      LOG_ERROR (
         PF_LLDP_LOG,
         "LLDP(%d): Failed to schedule Ethernet link check\n",
         __LINE__);
   }
}

void pf_pdport_periodic (pnet_t * net)
{
   int port;
   pf_port_iterator_t port_iterator;
   pf_port_t * p_port_data = NULL;

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);

   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);

      if (p_port_data->pdport.lldp_peer_info_updated)
      {
         p_port_data->pdport.lldp_peer_info_updated = false;
         pf_pdport_run_peer_check (net, port);
      }

      port = pf_port_get_next (&port_iterator);
   }
}

/**
 * @internal
 * Write PDPort data check
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param p_write_req      In:    The IODWrite request.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @param p_bytes          In:    Input data
 * @param p_datalength     In:    Size of the data to write.
 * @param p_result         Out:   Detailed error information.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_pdport_write_data_check (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_req,
   int loc_port_num,
   const uint8_t * p_bytes,
   uint16_t p_datalength,
   pnet_result_t * p_result)
{
   int ret = -1;
   uint16_t pos = 0;
   pf_port_data_check_t port_data_check = {0};
   pf_check_peers_t check_peers = {0};
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   pf_get_info_t get_info =
   {
      .result = PF_PARSE_OK,
      .p_frame = NULL,
      .p_array = p_bytes,
      .is_big_endian = true,
      .len = p_datalength,
   };

   pf_get_port_data_check (&get_info, &pos, &port_data_check);

   switch (port_data_check.block_header.block_type)
   {
   case PF_BT_CHECKPEERS:
      pf_get_port_data_check_check_peers (
         &get_info,
         &pos,
         PF_CHECK_PEERS_PER_PORT,
         &check_peers);
      if (get_info.result == PF_PARSE_OK)
      {
         if (check_peers.number_of_peers >= 1)
         {
            /* There is max one peer in the check_peers */
            p_port_data->pdport.check.peer = check_peers.peers[0];
            p_port_data->pdport.check.active = true;

            LOG_INFO (
               PNET_LOG,
               "PDPORT(%d): New PDPort data check. Peers: %u Wanted first peer "
               "station name: %.*s Port: %.*s\n",
               __LINE__,
               check_peers.number_of_peers,
               check_peers.peers[0].length_peer_station_name,
               check_peers.peers[0].peer_station_name,
               check_peers.peers[0].length_peer_port_name,
               check_peers.peers[0].peer_port_name);

            pf_pdport_run_peer_check (net, loc_port_num);

            ret = 0;
         }
         else
         {
            LOG_ERROR (
               PNET_LOG,
               "PDPORT(%d): Wrong incoming PDPort data check number of peers: "
               "%u. Slot: %u Subslot: 0x%04x\n",
               __LINE__,
               check_peers.number_of_peers,
               port_data_check.slot_number,
               port_data_check.subslot_number);
         }
      }
      else
      {
         LOG_ERROR (
            PNET_LOG,
            "PDPORT(%d): Failed to parse incoming PDPort data check.\n",
            __LINE__);
      }
      break;
   default:
      LOG_ERROR (
         PF_RPC_LOG,
         "PDPORT(%d): Unsupported port data check block type 0x%x\n",
         __LINE__,
         port_data_check.block_header.block_type);
      break;
   }
   return ret;
}

/**
 * @internal
 * Write PDPort data adjust
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param p_write_req      In:    The IODWrite request.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @param p_bytes          In:    Input data
 * @param p_datalength     In:    Size of the data to write.
 * @param p_result         Out:   Detailed error information.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_pdport_write_data_adj (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_req,
   int loc_port_num,
   const uint8_t * p_bytes,
   uint16_t p_datalength,
   pnet_result_t * p_result)
{
   int ret = -1;
   uint16_t pos = 0;
   pf_port_data_adjust_t port_data_adjust = {0};
   pf_adjust_peer_to_peer_boundary_t boundary;
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   pf_get_info_t get_info =
   {
      .result = PF_PARSE_OK,
      .p_frame = NULL,
      .p_array = p_bytes,
      .is_big_endian = true,
      .len = p_datalength,
   };

   pf_get_port_data_adjust (&get_info, &pos, &port_data_adjust);

   switch (port_data_adjust.block_header.block_type)
   {
   case PF_BT_PEER_TO_PEER_BOUNDARY:
      pf_get_port_data_adjust_peer_to_peer_boundary (&get_info, &pos, &boundary);
      if (get_info.result == PF_PARSE_OK)
      {
         p_port_data->pdport.adjust.active = true;
         p_port_data->pdport.adjust.peer_to_peer_boundary = boundary;

         LOG_INFO (
            PNET_LOG,
            "PDPORT(%d): New PDPort data adjust. Do not send LLDP: %u\n",
            __LINE__,
            boundary.peer_to_peer_boundary.do_not_send_LLDP_frames);

         if (
            p_port_data->pdport.adjust.peer_to_peer_boundary
               .peer_to_peer_boundary.do_not_send_LLDP_frames == 1)
         {
            pf_lldp_send_disable (net, loc_port_num);
         }
         else
         {
            pf_lldp_send_enable (net, loc_port_num);
         }

         ret = 0;
      }
      else
      {
         LOG_ERROR (
            PNET_LOG,
            "CPDPORT(%d): Failed to parse incoming PDPort data adjust.\n",
            __LINE__);
      }
      break;
   default:
      LOG_ERROR (
         PF_RPC_LOG,
         "PDPORT(%d): Unsupported incoming port data adjust block type 0x%x\n",
         __LINE__,
         port_data_adjust.block_header.block_type);
      break;
   }
   return ret;
}

/**
 * @internal
 * Write PDPort interface adjust
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param p_write_req      In:    The IODWrite request.
 * @param p_bytes          In:    Input data
 * @param p_datalength     In:    Size of the data to write.
 * @param p_result         Out:   Detailed error information.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_pdport_write_interface_adj (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_req,
   const uint8_t * p_bytes,
   uint16_t p_datalength,
   pnet_result_t * p_result)
{
   uint16_t pos = 0;
   pf_get_info_t get_info =
   {
      .result = PF_PARSE_OK,
      .p_frame = NULL,
      .p_array = p_bytes,
      .is_big_endian = true,
      .len = p_datalength,
   };

   pf_get_interface_adjust (
      &get_info,
      &pos,
      &net->pf_interface.name_of_device_mode.mode);
   net->pf_interface.name_of_device_mode.active = true;

   LOG_INFO (
      PNET_LOG,
      "PDPORT(%d): PD interface adjust. PLC is setting LLDP mode: %s\n",
      __LINE__,
      net->pf_interface.name_of_device_mode.mode ==
            PF_LLDP_NAME_OF_DEVICE_MODE_STANDARD
         ? "Standard"
         : "Legacy");

   pf_pdport_lldp_restart_transmission (net);

   return 0;
}

int pf_pdport_check_all (pnet_t * net)
{
   int port;
   pf_port_iterator_t port_iterator;

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   while (port != 0)
   {
      pf_pdport_peer_indication (net, port);
      port = pf_port_get_next (&port_iterator);
   }
   return 0;
}

#if PNET_OPTION_MRP

/* PF_IDX_SUB_PDINTF_MRP_CHECK */

static int pf_pdport_write_intf_mrp_data_check (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_req,
   int loc_port_num,
   const uint8_t * p_bytes,
   uint16_t pos,
   uint16_t p_datalength,
   pnet_result_t * p_result)
{
   pf_get_info_t get_info;
   pf_mrp_data_check_t mrp_check = {0};

   get_info.result = PF_PARSE_OK;
   get_info.p_frame = NULL;
   get_info.p_array = p_bytes;
   get_info.is_big_endian = true;
   get_info.len = p_datalength;

   /* this block header mrp_data_check is already processed higher up in
    * call-chain */

   pf_get_interface_mrp_data_check (
      &get_info,
      &pos,
      PF_MRP_NBR_INSTANCES,
      &mrp_check);

   if (get_info.result == PF_PARSE_OK)
   {
      int i;

      LOG_INFO (
         PF_MRP_LOG,
         "PDPORT(%d): mrp data check (%d instances)\n",
         __LINE__,
         mrp_check.nbr_instances);

      /* log changes */
      for (i = 0; i < mrp_check.nbr_instances; i++)
      {
         char domain_str[MRP_UUID_STRING_SIZE];

         pf_mrp_domain_uuid_str (&mrp_check.instance[i].domain, domain_str);
         LOG_INFO (
            PF_MRP_LOG,
            "PDPORT(%d) : [%d] domain_uuid [%s] diag_check:0x%d lldp_check:%d\n",
            __LINE__,
            mrp_check.instance[i].instance,
            domain_str,
            mrp_check.instance[i].flags.redundance_mgr_diag,
            mrp_check.instance[i].flags.lldp_domain);

         if (
            net->mrp_data.check.instance[i].flags.lldp_domain !=
            mrp_check.instance[i].flags.lldp_domain)
         {
            LOG_INFO (
               PF_MRP_LOG,
               "PDPORT(%d) lldp mrp domain check is now set to %d\n",
               __LINE__,
               mrp_check.instance[i].flags.lldp_domain);
         }

         /* store mrp check info and activate checking via periodic check */
         net->mrp_data.check = mrp_check;
         net->mrp_data.check_active = true;

         /* if domain mismatch check is enabled, run peer check now */
         if (mrp_check.instance[i].flags.lldp_domain)
            pf_pdport_check_all (net);
      }
   }
   else
   {
      LOG_ERROR (
         PF_MRP_LOG,
         "PDPORT(%d): Failed to parse incoming interface mrp data check.\n",
         __LINE__);
      return -1;
   }

   return 0;
}

/* PF_IDX_SUB_PDINTF_MRP_ADJUST */

static int pf_pdport_write_intf_mrp_data_adjust (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_req,
   int loc_port_num,
   const uint8_t * p_bytes,
   uint16_t pos,
   uint16_t p_datalength,
   pnet_result_t * p_result)
{
   pf_get_info_t get_info;
   pf_intf_mrp_data_adjust_t mrp_adjust = {0};

   get_info.result = PF_PARSE_OK;
   get_info.p_frame = NULL;
   get_info.p_array = p_bytes;
   get_info.is_big_endian = true;
   get_info.len = p_datalength;

   pf_get_interface_mrp_data_adjust (
      &get_info,
      &pos,
      PF_MRP_NBR_INSTANCES,
      &mrp_adjust);

   if (get_info.result == PF_PARSE_OK)
   {
      int i;

      /* only one instance supported (checked in
       * pf_get_interface_mrp_data_adjust) */

      LOG_INFO (
         PF_MRP_LOG,
         "PDPORT(%d): mrp data adjust (%d instances)\n",
         __LINE__,
         mrp_adjust.nbr_instances);

      for (i = 0; i < mrp_adjust.nbr_instances; i++)
      {
         char domain_str[MRP_UUID_STRING_SIZE];

         net->mrp_data.config.domain_name.len =
            mrp_adjust.instance[i].domain_len;
         memcpy (
            net->mrp_data.config.domain_name.name,
            mrp_adjust.instance[i].domain_name,
            mrp_adjust.instance[i].domain_len);

         /* null terminate for convenience */
         net->mrp_data.config.domain_name
            .name[mrp_adjust.instance[i].domain_len] = 0;

         pf_mrp_domain_uuid_str (&mrp_adjust.instance[i].domain, domain_str);

         LOG_INFO (
            PF_MRP_LOG,
            "PDPORT(%d) : [%d] domain_uuid [%s]\n",
            __LINE__,
            mrp_adjust.instance[i].instance,
            domain_str);
         LOG_INFO (
            PF_MRP_LOG,
            "PDPORT(%d) : [%d] domain_name %s\n",
            __LINE__,
            mrp_adjust.instance[i].instance,
            mrp_adjust.instance[i].domain_name);

         /* Send one update with port status off and then remove mrp port status
          * tlv from lldp frames */

         if (net->mrp_data.config.role != mrp_adjust.instance[i].role)
            net->mrp_data.config.role_updated = true;

         net->mrp_data.config.role = mrp_adjust.instance[i].role;

         if (mrp_adjust.instance[i].role == MEDIA_REDUNDANCY_DISABLED)
         {
            pf_mrp_disable (net);

            LOG_INFO (
               PF_MRP_LOG,
               "PDPORT(%d) : media redundancy disabled.\n",
               __LINE__);
         }
         else if (mrp_adjust.instance[i].role == MEDIA_REDUNDANCY_CLIENT)
         {
            memcpy (
               net->mrp_data.config.domain.data,
               mrp_adjust.instance[i].domain.data,
               sizeof (mrp_domain_uuid_t));
            net->mrp_data.config.client_params =
               mrp_adjust.instance[i].client_params;
            pf_mrp_enable (net);

            LOG_INFO (
               PF_MRP_LOG,
               "PDPORT(%d) : media redundancy client enabled.\n",
               __LINE__);
            LOG_INFO (
               PF_MRP_LOG,
               "PDPORT(%d) : [%d] client redundancy params\n",
               __LINE__,
               mrp_adjust.instance[i].instance);
            LOG_INFO (
               PF_MRP_LOG,
               "PDPORT(%d) : [%d] link_up_T %d ms link_down_T %d ms max_T:%d "
               "ms\n",
               __LINE__,
               mrp_adjust.instance[i].instance,
               mrp_adjust.instance[i].client_params.link_down_T,
               mrp_adjust.instance[i].client_params.link_up_T,
               mrp_adjust.instance[i].client_params.link_nr_max);
         }
         else
         {
            LOG_INFO (
               PF_MRP_LOG,
               "PDPORT(%d): mrp role %d not supported\n",
               __LINE__,
               net->mrp_data.config.role);
            return -1;
         }
      }
   }
   else
   {
      LOG_ERROR (
         PF_MRP_LOG,
         "PDPORT(%d): Failed to parse incoming interface mrp data adjust.\n",
         __LINE__);
      return -1;
   }

   return 0;
}

/* PF_IDX_SUB_PDPORT_MRP_ADJUST */

static int pf_pdport_write_port_mrp_adjust (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_req,
   int loc_port_num,
   const uint8_t * p_bytes,
   uint16_t pos,
   uint16_t p_datalength,
   pnet_result_t * p_result)
{
   pf_get_info_t get_info;
   uint8_t instance;
   mrp_domain_uuid_t domain;

   get_info.result = PF_PARSE_OK;
   get_info.p_frame = NULL;
   get_info.p_array = p_bytes;
   get_info.is_big_endian = true;
   get_info.len = p_datalength;

   /* this block header is already processed higher up in call-chain */

   pos += 1; /* 1 byte padding */

   instance = pf_get_byte (&get_info, &pos);
   pf_get_mem (&get_info, &pos, 16, domain.data);

   if (get_info.result == PF_PARSE_OK)
   {
      char domain_str[MRP_UUID_STRING_SIZE];
      pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
      pf_mrp_port_t * p_port_mrp = &p_port_data->mrp;

      pf_mrp_domain_uuid_str (&domain, domain_str);

      LOG_INFO (
         PF_MRP_LOG,
         "PDPORT(%d): mrp port data adjust instance %d domain_uuid [%s]\n",
         __LINE__,
         instance,
         domain_str);

      memcpy (p_port_mrp->domain.data, domain.data, sizeof (mrp_domain_uuid_t));
      p_port_mrp->instance = instance;
   }
   else
   {
      LOG_ERROR (
         PF_MRP_LOG,
         "PDPORT(%d): Failed to parse incoming port mrp data adjust.\n",
         __LINE__);
      return -1;
   }

   return 0;
}

/* main handler for all mrp read attributes to both ports
   and interface  */
int pf_pdport_mrp_read_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_read_request_t * p_read_req,
   pf_put_info_t * p_res,
   uint16_t * p_pos)
{
   int ret = -1;
   int loc_port_num;

   pf_port_t * p_port_data;
   uint16_t slot = p_read_req->slot_number;
   uint16_t subslot = p_read_req->subslot_number;

   uint16_t index = p_read_req->index;

   switch (index)
   {
   case PF_IDX_SUB_PDINTF_MRP_REAL:
      /* interface specific read */
      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         (subslot == PNET_SUBSLOT_DAP_INTERFACE_1_IDENT))
      {
         pf_mrp_config_t * mrp = &net->mrp_data.config;

         pf_put_pdinterface_mrp_data_real (
            true,
            &mrp->domain,
            mrp->role,
            mrp->version,
            &mrp->domain_name,
            &mrp->client_params,
            p_res,
            p_pos);
         ret = 0;
      }
      break;

   case PF_IDX_SUB_PDINTF_MRP_ADJUST:

      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         (subslot == PNET_SUBSLOT_DAP_INTERFACE_1_IDENT))
      {
         /* send back current config if data check is active */
         if (net->mrp_data.check_active)
         {
            pf_put_intf_mrp_data_adjust (
               true,
               subslot,
               &net->mrp_data.config,
               p_res,
               p_pos);
         }
         ret = 0;
      }
      break;

   case PF_IDX_SUB_PDINTF_MRP_CHECK:
   {
      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         (subslot == PNET_SUBSLOT_DAP_INTERFACE_1_IDENT))
      {
         /* only send back config if check is active and mrp enabled */
         if (
            net->mrp_data.check_active &&
            (net->mrp_data.config.role == MEDIA_REDUNDANCY_CLIENT))
         {
            pf_put_intf_mrp_data_check (
               true,
               subslot,
               &net->mrp_data.check,
               p_res,
               p_pos);
         }
         ret = 0;
      }
   }
   break;

   case PF_IDX_SUB_PDPORT_MRP_ADJUST:

      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         (pf_port_subslot_is_dap_port_id (net, subslot)))
      {
         loc_port_num = pf_port_dap_subslot_to_local_port (net, subslot);
         p_port_data = pf_port_get_state (net, loc_port_num);
         pf_mrp_port_t * p_port_mrp = &p_port_data->mrp;

         if (
            net->mrp_data.check_active &&
            (net->mrp_data.config.role == MEDIA_REDUNDANCY_CLIENT))
         {
            pf_put_pdport_mrp_data_adjust (
               true,
               subslot,
               p_port_mrp->instance,
               &p_port_mrp->domain,
               p_res,
               p_pos);
         }
         ret = 0;
      }

      break;

   case PF_IDX_SUB_PDPORT_MRP_REAL:

      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         (pf_port_subslot_is_dap_port_id (net, subslot)))
      {
         loc_port_num = pf_port_dap_subslot_to_local_port (net, subslot);
         p_port_data = pf_port_get_state (net, loc_port_num);
         pf_mrp_port_t * p_port_mrp = &p_port_data->mrp;

         pf_put_pdport_mrp_data_real (
            true,
            subslot,
            p_port_mrp->instance,
            &p_port_mrp->domain,
            p_res,
            p_pos);
         ret = 0;
      }

      break;

   default:
      break;
   }

   return ret;
}

/* main handler for all mrp write attributes to both ports
   and interface  */

int pf_pdport_write_mrp_req (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_req,
   const uint8_t * p_bytes,
   uint16_t pos,
   uint16_t data_length,
   pnet_result_t * p_result)
{
   int ret = -1;
   int loc_port_num =
      pf_port_dap_subslot_to_local_port (net, p_write_req->subslot_number);

   LOG_INFO (
      PF_MRP_LOG,
      "PDPORT(%d): write index 0x%x subslot: 0x%x (lport %d)\n",
      __LINE__,
      p_write_req->index,
      p_write_req->subslot_number,
      loc_port_num);

   if (loc_port_num > 0)
   {
      /* port specific writes */
      switch (p_write_req->index)
      {
      case PF_IDX_SUB_PDPORT_MRP_ADJUST:
         ret = pf_pdport_write_port_mrp_adjust (
            net,
            p_ar,
            p_write_req,
            loc_port_num,
            p_bytes,
            pos,
            data_length,
            p_result);
         break;

      default:
         LOG_WARNING (
            PNET_LOG,
            "attribute 0x%x write error for slot %d subslot %d",
            p_write_req->index,
            p_write_req->slot_number,
            p_write_req->subslot_number);
         break;
      }
      if (ret == 0)
      {
         (void)pf_bg_worker_start_job (net, PF_BGJOB_SAVE_MRP_DATA);
      }
   }
   else
   {
      /* interface specific writes (subslot 0x8000) */
      switch (p_write_req->index)
      {
      /* parameters that should be checked for consistency */
      case PF_IDX_SUB_PDINTF_MRP_CHECK:
         if (
            (p_write_req->slot_number == PNET_SLOT_DAP_IDENT) &&
            (p_write_req->subslot_number == PNET_SUBMOD_DAP_INTERFACE_1_IDENT))
         {
            ret = pf_pdport_write_intf_mrp_data_check (
               net,
               p_ar,
               p_write_req,
               loc_port_num,
               p_bytes,
               pos,
               data_length,
               p_result);
         }
         break;

      case PF_IDX_SUB_PDINTF_MRP_ADJUST:

         if (
            (p_write_req->slot_number == PNET_SLOT_DAP_IDENT) &&
            (p_write_req->subslot_number == PNET_SUBMOD_DAP_INTERFACE_1_IDENT))
         {
            ret = pf_pdport_write_intf_mrp_data_adjust (
               net,
               p_ar,
               p_write_req,
               loc_port_num,
               p_bytes,
               pos,
               data_length,
               p_result);
         }
         break;

      case PF_IDX_SUB_PDINTF_MRP_REAL:
         LOG_WARNING (
            PNET_LOG,
            "attribute 0x%x not writeable, resetting MRP\n",
            p_write_req->index);
         /* tbd -- bad parametrization should reset configuration back to
          * defaults as per GSDML) and For now only reset mrp but need to have a
          * discussion on whether this behavior also applies
          * to other parameters */
         pf_mrp_data_init (net, true);
         break;

      default:
         LOG_WARNING (
            PNET_LOG,
            "attribute 0x%x write error\n",
            p_write_req->index);
         break;
      }
   }

   if (ret == -1)
   {
      pf_set_error (
         p_result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_ACCESS_DENIED,
         PNET_ERROR_CODE_2_PNIORW (0));
   }

   return ret;
}

#endif // #if PNET_OPTION_MRP

int pf_pdport_write_req (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_req,
   const uint8_t * p_bytes,
   uint16_t data_length,
   pnet_result_t * p_result)
{
   int ret = -1;
   int loc_port_num =
      pf_port_dap_subslot_to_local_port (net, p_write_req->subslot_number);

   if (loc_port_num > 0)
   {
      switch (p_write_req->index)
      {
      case PF_IDX_SUB_PDPORT_DATA_CHECK:
         ret = pf_pdport_write_data_check (
            net,
            p_ar,
            p_write_req,
            loc_port_num,
            p_bytes,
            data_length,
            p_result);
         break;
      case PF_IDX_SUB_PDPORT_DATA_ADJ:
         ret = pf_pdport_write_data_adj (
            net,
            p_ar,
            p_write_req,
            loc_port_num,
            p_bytes,
            data_length,
            p_result);
         break;
      default:
         break;
      }

      if (ret == 0)
      {
         (void)pf_bg_worker_start_job (net, PF_BGJOB_SAVE_PDPORT_NVM_DATA);
      }
   }
   else
   {
      switch (p_write_req->index)
      {
      case PF_IDX_SUB_PDINTF_ADJUST:
         ret = pf_pdport_write_interface_adj (
            net,
            p_ar,
            p_write_req,
            p_bytes,
            data_length,
            p_result);
         break;
      default:
         break;
      }
   }

   return ret;
}

void pf_pdport_peer_indication (pnet_t * net, int loc_port_num)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   p_port_data->pdport.lldp_peer_info_updated = true;
}

void pf_pdport_update_eth_status (pnet_t * net)
{
   int loc_port_num;
   pf_port_iterator_t port_iterator;
   pf_port_t * p_port_data = NULL;
   pnal_eth_status_t eth_status;

   if (net->pf_interface.port_mutex == NULL)
   {
      /* Handle case during init when port data is not yet initialized. */
      return;
   }

   pf_port_init_iterator_over_ports (net, &port_iterator);
   loc_port_num = pf_port_get_next (&port_iterator);

   while (loc_port_num != 0)
   {
      p_port_data = pf_port_get_state (net, loc_port_num);
      if (pnal_eth_get_status (p_port_data->netif.name, &eth_status) != 0)
      {
         memset (&eth_status, 0, sizeof (eth_status));
         LOG_ERROR (
            PNET_LOG,
            "PDPORT(%d): Failed to read link status for port %u.\n",
            __LINE__,
            loc_port_num);
      }
      os_mutex_lock (net->pf_interface.port_mutex);
      p_port_data->eth_status = eth_status;
      os_mutex_unlock (net->pf_interface.port_mutex);

      loc_port_num = pf_port_get_next (&port_iterator);
   }
}

/**
 * Replace the Ethernet MAU type with the default value, if
 * the actual value is unknown.
 *
 * @param mau_type         In: MAU type read from hardware
 * @param running          In: True if link is up
 * @param default_mau_type In: Default MAU type from config
 * @return  Filtered MAU type
 */
static pnal_eth_mau_t pf_pdport_filter_mau_type (
   pnal_eth_mau_t mau_type,
   bool running,
   pnal_eth_mau_t default_mau_type)
{
   /** The MAU type 0 represents "unknown" when the link is down.
    *  When the link is up a MAU type 0 represents "Radio". */
   if (mau_type == PNAL_ETH_MAU_UNKNOWN && !running)
   {
      return default_mau_type;
   }

   return mau_type;
}

/**
 * Replace the Ethernet MAU type extension with the default value, if
 * the actual value is unknown.
 *
 * @param mau_type                   In: MAU type read from hardware
 * @param running                    In: True if link is up
 * @param default_mau_type_extension In: Default MAU type extension
 *                                       from config
 * @return  Filtered MAU type extension
 */
static pnal_eth_mau_ext_t pf_pdport_filter_mau_type_extension (
   pnal_eth_mau_ext_t mau_type_extension,
   bool running,
   pnal_eth_mau_ext_t default_mau_type_extension)
{
   /** The MAU type extension 0 represents "unknown" when the link is down.
    *  When the link is up a MAU type extension 0 represents "No extension". */
   if (mau_type_extension == PNAL_ETH_MAU_EXTENSION_NONE && !running)
   {
      return default_mau_type_extension;
   }

   return mau_type_extension;
}

pnal_eth_status_t pf_pdport_get_eth_status (pnet_t * net, int loc_port_num)
{
   pnal_eth_status_t eth_status;
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   os_mutex_lock (net->pf_interface.port_mutex);
   eth_status = p_port_data->eth_status;
   os_mutex_unlock (net->pf_interface.port_mutex);

   return eth_status;
}

pnal_eth_status_t pf_pdport_get_eth_status_filtered_mau (
   pnet_t * net,
   int loc_port_num)
{
   const pnet_port_cfg_t * p_port_config =
      pf_port_get_config (net, loc_port_num);
   pnal_eth_status_t eth_status = pf_pdport_get_eth_status (net, loc_port_num);

   eth_status.operational_mau_type = pf_pdport_filter_mau_type (
      eth_status.operational_mau_type,
      eth_status.running,
      p_port_config->default_mau_type);

   eth_status.mau_type_extension = pf_pdport_filter_mau_type_extension (
      eth_status.mau_type_extension,
      eth_status.running,
      p_port_config->default_mau_type_extension);

   return eth_status;
}
