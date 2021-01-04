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
 * @param loc_port_num      In:   Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
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
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_pdport_load (pnet_t * net, int loc_port_num)
{
   int ret = -1;
   pf_pdport_t pdport_config;
   const char * p_file_directory = NULL;
   pf_port_t * p_port_data = NULL;

   (void)pf_cmina_get_file_directory (net, &p_file_directory);

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
      p_port_data->pdport.lldp_peer_timeout = false;

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
         pf_lldp_reset_peer_timeout (
            net,
            loc_port_num,
            PF_LLDP_INITIAL_PEER_TIMEOUT);
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
      pf_lldp_reset_peer_timeout (
         net,
         loc_port_num,
         PF_LLDP_INITIAL_PEER_TIMEOUT);
      pf_lldp_send_enable (net, loc_port_num);
   }

   return ret;
}

/**
 * Store PDPort data for one port to nvm
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_pdport_save (pnet_t * net, int loc_port_num)
{
   int ret = 0;
   int save_result = 0;
   pf_pdport_t pdport_config = {0};
   pf_pdport_t temporary_buffer;
   const char * p_file_directory = NULL;
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   memcpy (&pdport_config, &p_port_data->pdport, sizeof (pdport_config));

   (void)pf_cmina_get_file_directory (net, &p_file_directory);

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

uint16_t pf_pdport_loc_port_num_to_dap_subslot (int loc_port_num)
{
   uint16_t subslot =
      PNET_SUBSLOT_DAP_INTERFACE_1_PORT_1_IDENT + PNET_PORT_1 - loc_port_num;
   return subslot;
}

/**
 * @internal
 * Get local port from DAP port subslot
 *
 * Considers PNET_MAX_PORT
 *
 * @param subslot              In: Subslot number
 * @return The port number mapping to the subslot.
 *         0 if the subslot is not supported.
 */
static int pf_pdport_dap_subslot_to_local_port (uint16_t subslot)
{
   int port = PNET_PORT_1 + PNET_SUBSLOT_DAP_INTERFACE_1_PORT_1_IDENT - subslot;
   if (port < PNET_PORT_1 || port > PNET_MAX_PORT)
   {
      port = 0;
   }
   return port;
}

/**
 * @internal
 * Check if a DAP port subslot is mapped to a local port
 *
 * @param subslot              In: Subslot number
 * @return true  if the subslot is mapped to a local port.
 *         false if the subslot is not supported.
 */
static bool pf_pdport_subslot_is_dap_port_id (uint16_t subslot)
{
   int port = pf_pdport_dap_subslot_to_local_port (subslot);
   return (port != 0);
}

/**
 * @internal
 * Initialize pdport diagnostic source
 *
 * @param diag_source      InOut: Diag source to be initialized
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 */
static void pf_pdport_init_diag_source (
   pnet_diag_source_t * diag_source,
   int loc_port_num)
{
   diag_source->api = PNET_API_NO_APPLICATION_PROFILE;
   diag_source->slot = PNET_SLOT_DAP_IDENT;
   diag_source->subslot = pf_pdport_loc_port_num_to_dap_subslot (loc_port_num);
   diag_source->ch = PNET_CHANNEL_WHOLE_SUBMODULE;
   diag_source->ch_grouping = PNET_DIAG_CH_INDIVIDUAL_CHANNEL;
   diag_source->ch_direction = PNET_DIAG_CH_PROP_DIR_MANUF_SPEC;
}

/**
 * Delete all active diagnostics handled by pdport
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num      In:   Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 */
static void pf_pdport_remove_all_diag (pnet_t * net, int loc_port_num)
{
   pnet_diag_source_t diag_source = {0};

   pf_pdport_init_diag_source (&diag_source, loc_port_num);

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
}

/**
 * Init PDPort data.
 * Load port configuration from nvm.

 * If a check is loaded it will be run when pdport receives
 * data from lldp.
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
      pf_pdport_save (net, port);

      port = pf_port_get_next (&port_iterator);
   }
   return 0;
}

void pf_pdport_remove_data_files (const char * file_directory)
{
   int port;

   /* Do not use port iterator as net is not available */
   for (port = PNET_PORT_1; port <= PNET_MAX_PORT; port++)
   {
      pf_file_clear (file_directory, pf_pdport_get_filename (port));
   }
}

void pf_pdport_ar_connect_ind (pnet_t * net, const pf_ar_t * p_ar)
{
   (void)p_ar;
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
      port = pf_port_get_next (&port_iterator);
   }
}

void pf_pdport_lldp_restart (pnet_t * net)
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
   const pf_iod_read_result_t * p_read_res,
   uint16_t res_size,
   uint8_t * p_res,
   uint16_t * p_pos)
{
   int ret = -1;
   int loc_port_num;
   pnal_port_stats_t port_stats;
   pf_port_t * p_port_data;
   const pnet_port_cfg_t * p_port_cfg;
   uint16_t slot = p_read_req->slot_number;
   uint16_t subslot = p_read_req->subslot_number;

   switch (p_read_req->index)
   {
   case PF_IDX_SUB_PDPORT_DATA_REAL:
      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         (pf_pdport_subslot_is_dap_port_id (subslot) == true))
      {
         loc_port_num = pf_pdport_dap_subslot_to_local_port (subslot);
         pf_put_pdport_data_real (
            net,
            loc_port_num,
            true,
            p_read_res,
            res_size,
            p_res,
            p_pos);
         ret = 0;
      }
      break;
   case PF_IDX_SUB_PDPORT_DATA_ADJ:
      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         (pf_pdport_subslot_is_dap_port_id (subslot) == true))
      {
         loc_port_num = pf_pdport_dap_subslot_to_local_port (subslot);
         p_port_data = pf_port_get_state (net, loc_port_num);
         if (p_port_data->pdport.adjust.active)
         {
            pf_put_pdport_data_adj (
               &p_port_data->pdport.adjust.peer_to_peer_boundary,
               subslot,
               true,
               p_read_res,
               res_size,
               p_res,
               p_pos);
         }
         ret = 0;
      }
      break;
   case PF_IDX_SUB_PDPORT_DATA_CHECK:
      if (slot == PNET_SLOT_DAP_IDENT)
      {
         if (pf_pdport_subslot_is_dap_port_id (subslot) == true)
         {
            loc_port_num = pf_pdport_dap_subslot_to_local_port (subslot);
            p_port_data = pf_port_get_state (net, loc_port_num);
            if (p_port_data->pdport.check.active)
            {
               pf_put_pdport_data_check (
                  &p_port_data->pdport.check.peer,
                  true,
                  p_read_res,
                  res_size,
                  p_res,
                  p_pos);
            }
            ret = 0;
         }
         else if (subslot == PNET_SUBSLOT_DAP_WHOLE_MODULE)
         {
            /* Todo - how should read using subslot 0 be handled?
             * include all checks in response?
             */
            loc_port_num = PNET_PORT_1;
            p_port_data = pf_port_get_state (net, loc_port_num);
            if (p_port_data->pdport.check.active)
            {
               pf_put_pdport_data_check (
                  &p_port_data->pdport.check.peer,
                  true,
                  p_read_res,
                  res_size,
                  p_res,
                  p_pos);
            }
            ret = 0;
         }
      }
      break;
   case PF_IDX_DEV_PDREAL_DATA:
      if (
         (slot == PNET_SLOT_DAP_IDENT) &&
         ((subslot == PNET_SUBSLOT_DAP_WHOLE_MODULE) ||
          (subslot == PNET_SUBSLOT_DAP_IDENT)))
      {
         /*Todo - handle multiple ports and interface level */
         loc_port_num = PNET_PORT_1;
         pf_put_pd_real_data (
            net,
            true,
            loc_port_num,
            p_read_res,
            res_size,
            p_res,
            p_pos);
         ret = 0;
      }
      break;
   case PF_IDX_DEV_PDEXP_DATA:
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
          (pf_pdport_subslot_is_dap_port_id (subslot) == true)))
      {
         loc_port_num = pf_pdport_dap_subslot_to_local_port (subslot);

         if (loc_port_num != 0)
         {
            p_port_cfg = pf_lldp_get_port_config (net, loc_port_num);
            if (
               pnal_get_port_statistics (
                  p_port_cfg->phy_port.if_name,
                  &port_stats) == 0)
            {
               pf_put_pdport_statistics (
                  &port_stats,
                  true,
                  p_read_res,
                  res_size,
                  p_res,
                  p_pos);
               ret = 0;
            }
         }
         else
         {
            /* TODO What should be provided for
                    PNET_SUBSLOT_DAP_INTERFACE_1_IDENT? */
            LOG_DEBUG (
               PNET_LOG,
               "PDPORT(%d): PF_IDX_SUB_PDPORT_STATISTIC for "
               "PNET_SUBSLOT_DAP_INTERFACE_1_IDENT is not yet implemented\n",
               __LINE__);
         }
      }
      break;
   default:
      ret = -1;
      break;
   }
   return ret;
}

/**
 * @internal
 * Run peer station name check
 *
 * Compare llpd peer information with configured pdport peer.
 * Trigger an alarm if wrong peer is connected.
 * Remove diagnosis if expected peer data is received.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 */
static void pf_pdport_check_peer_station_name (pnet_t * net, int loc_port_num)
{
   bool peer_stationname_is_correct = false;
   pnet_diag_source_t diag_source = {0};
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   pf_check_peer_t * p_wanted_peer = &p_port_data->pdport.check.peer;
   pf_lldp_station_name_t lldp_station_name = {0};

   if (pf_lldp_get_peer_station_name (net, loc_port_num, &lldp_station_name) == 0)
   {
      pf_pdport_init_diag_source (&diag_source, loc_port_num);

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
         LOG_DEBUG (
            PNET_LOG,
            "PDPORT(%d): Sending peer station name mismatch alarm: \"%.*s\". "
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
            "Remove diagnosis.\n",
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
 * Compare llpd peer information with configured pdport peer.
 * Trigger an diagnosis if unexpected peer data is connected.
 * Remove diagnosis if expected peer data is received.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 */
static void pf_pdport_check_peer_port_name (pnet_t * net, int loc_port_num)
{
   bool peer_portname_is_correct = false;
   pnet_diag_source_t diag_source = {0};
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   pf_check_peer_t * p_wanted_peer = &p_port_data->pdport.check.peer;
   pf_lldp_port_id_t lldp_port_id = {0};

   if (pf_lldp_get_peer_port_id (net, loc_port_num, &lldp_port_id) == 0)
   {
      pf_pdport_init_diag_source (&diag_source, loc_port_num);

      if (
         strncmp (
            lldp_port_id.string,
            (char *)p_wanted_peer->peer_port_name,
            lldp_port_id.len) == 0)
      {
         peer_portname_is_correct = true;
      }

      if (peer_portname_is_correct == false)
      {
         LOG_DEBUG (
            PNET_LOG,
            "PDPORT(%d): Sending peer port name mismatch alarm: \"%.*s\". "
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
            "Remove diagnosis.\n",
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

/**
 * @internal
 * Run peer check
 *
 * Compare llpd peer information with configured pdport peer.
 * Trigger an diagnosis if unexpected peer data is connected.
 * Remove diagnosis if expected peer data is received.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 */
static void pf_pdport_run_peer_check (pnet_t * net, int loc_port_num)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   if (p_port_data->pdport.check.active == true)
   {
      pf_pdport_check_peer_port_name (net, loc_port_num);
      pf_pdport_check_peer_station_name (net, loc_port_num);
   }
   else
   {
      LOG_DEBUG (
         PNET_LOG,
         "PDPORT(%d): We do not check peer name or portID, so no port change "
         "alarm is sent.\n",
         __LINE__);
   }
}

static void pf_pdport_peer_lldp_timeout_alarm (pnet_t * net, int loc_port_num)
{
   pnet_diag_source_t diag_source = {0};

   pf_pdport_init_diag_source (&diag_source, loc_port_num);

   LOG_DEBUG (
      PNET_LOG,
      "PDPORT(%d): Sending no peer detected alarm port %u",
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

      if (p_port_data->pdport.lldp_peer_timeout)
      {
         p_port_data->pdport.lldp_peer_timeout = false;
         pf_pdport_peer_lldp_timeout_alarm (net, port);
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
 *                                Valid range: 1 .. PNET_MAX_PORT
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
   pf_get_info_t get_info;
   pf_port_data_check_t port_data_check = {0};
   pf_check_peers_t check_peers = {0};
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   get_info.result = PF_PARSE_OK;
   get_info.p_buf = p_bytes;
   get_info.is_big_endian = true;
   get_info.len = p_datalength;

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
               "PDPORT(%d): PLC is writing PDPort data check. Slot: %u "
               "Subslot: "
               "0x%04x Peers: %u First peer station name: %.*s Port: %.*s\n",
               __LINE__,
               port_data_check.slot_number,
               port_data_check.subslot_number,
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
 *                                Valid range: 1 .. PNET_MAX_PORT
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
   pf_get_info_t get_info;
   pf_port_data_adjust_t port_data_adjust = {0};
   pf_adjust_peer_to_peer_boundary_t boundary = {0};
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   get_info.result = PF_PARSE_OK;
   get_info.p_buf = p_bytes;
   get_info.is_big_endian = true;
   get_info.len = p_datalength;

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
            "PDPORT(%d): PLC is writing PDPort data adjust. Do not send LLDP: "
            "%u\n",
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
      pf_pdport_dap_subslot_to_local_port (p_write_req->subslot_number);

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
   }

   if (ret == 0)
   {
      (void)pf_pdport_save (net, loc_port_num);
   }
   return ret;
}

void pf_pdport_peer_indication (
   pnet_t * net,
   int loc_port_num,
   const pf_lldp_peer_info_t * p_lldp_peer_info)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   p_port_data->pdport.lldp_peer_info_updated = true;
}

void pf_pdport_peer_lldp_timeout (pnet_t * net, int loc_port_num)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   p_port_data->pdport.lldp_peer_timeout = true;
}
