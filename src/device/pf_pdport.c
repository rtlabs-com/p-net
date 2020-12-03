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
 * Load PDPort data from nvm
 *
 * @param net              InOut: The p-net stack instance
 *
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_pdport_load (pnet_t * net)
{
   int ret = -1;
   pnet_pdport_nvm_t file_port;
   const char * p_file_directory = NULL;
   pf_port_t * p_port_data = NULL;

   (void)pf_cmina_get_file_directory (net, &p_file_directory);

   if (
      pf_file_load (
         p_file_directory,
         PNET_FILENAME_PDPORT,
         &file_port,
         sizeof (pnet_pdport_nvm_t)) == 0)
   {
      LOG_DEBUG (
         PNET_LOG,
         "PDPORT(%d): Did read PDPort settings from nvm.\n",
         __LINE__);

      p_port_data = pf_port_get_state (net, PNET_PORT_1);

      p_port_data->pdport.check.peer = file_port.peer;
      if (
         p_port_data->pdport.check.peer.length_peer_port_name > 0 ||
         p_port_data->pdport.check.peer.length_peer_station_name > 0)
      {
         p_port_data->pdport.check.active = true;
      }
      ret = 0;
   }
   return ret;
}

/**
 * Store PDPort data to nvm
 *
 * @param net              InOut: The p-net stack instance
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_pdport_save (pnet_t * net)
{
   int ret = 0;
   int save_result = 0;
   pnet_pdport_nvm_t output_port_nvm = {0};
   pnet_pdport_nvm_t temporary_buffer;
   const char * p_file_directory = NULL;
   pf_port_t * p_port_data = pf_port_get_state (net, PNET_PORT_1);

   memcpy (
      &output_port_nvm.peer,
      &p_port_data->pdport.check.peer,
      sizeof (output_port_nvm.peer));

   (void)pf_cmina_get_file_directory (net, &p_file_directory);

   save_result = pf_file_save_if_modified (
      p_file_directory,
      PNET_FILENAME_PDPORT,
      &output_port_nvm,
      &temporary_buffer,
      sizeof (pnet_pdport_nvm_t));
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

/**
 * @internal
 * Get DAP subslot for using local port number
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. N, where N is the total
 *                                number of local ports used by p-net stack.
 * @return DAP subslot number for port identity
 */
static uint16_t pf_pdport_loc_port_num_to_dap_subslot (int loc_port_num)
{
   CC_ASSERT (loc_port_num == PNET_PORT_1);
   return PNET_SUBSLOT_DAP_INTERFACE_1_PORT_0_IDENT;
}

/**
 * @internal
 * Initialize pdport diagnostic source
 * @param diag_source      InOut: Diag source to be initialized
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. N, where N is the total
 *                                number of local ports used by p-net stack.
 * @return Index in port array
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
 * @param net              InOut: The p-net stack instance
 */
static void pf_pdport_remove_all_diag (pnet_t * net)
{
   pnet_diag_source_t diag_source = {0};

   pf_pdport_init_diag_source (&diag_source, PNET_PORT_1);

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
   if (pf_pdport_load (net) != 0)
   {
      /* Create file if missing */
      (void)pf_pdport_save (net);
   }
   return 0;
}

/**
 * Reset port configuration data for all ports.
 * Clear configuration in nvm.
 * Remove diagnosis
 *
 * @param net              InOut: The p-net stack instance
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred
 */
int pf_pdport_reset_all (pnet_t * net)
{
   const char * p_file_directory = NULL;
   pf_port_t * p_port_data = pf_port_get_state (net, PNET_PORT_1);

   (void)pf_cmina_get_file_directory (net, &p_file_directory);
   pf_file_clear (p_file_directory, PNET_FILENAME_PDPORT);

   memset (&p_port_data->pdport, 0, sizeof (p_port_data->pdport));
   pf_pdport_remove_all_diag (net);

   return 0;
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
   int loc_port_num = PNET_PORT_1;
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   switch (p_read_req->index)
   {
   case PF_IDX_SUB_PDPORT_DATA_REAL:
      if (
         (p_read_req->slot_number == PNET_SLOT_DAP_IDENT) &&
         (p_read_req->subslot_number ==
          PNET_SUBSLOT_DAP_INTERFACE_1_PORT_0_IDENT))
      {
         pf_put_pdport_data_real (
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
   case PF_IDX_SUB_PDPORT_DATA_ADJ:
      /* Only check if this is the port subslot */
      if (
         (p_read_req->slot_number == PNET_SLOT_DAP_IDENT) &&
         (p_read_req->subslot_number ==
          PNET_SUBSLOT_DAP_INTERFACE_1_PORT_0_IDENT))
      {
         if (p_port_data->pdport.adjust.active)
         {
            pf_put_pdport_data_adj (
               &p_port_data->pdport.adjust.peer_to_peer_boundary,
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
      /* Only check if this is the port subslot */
      if (
         (p_read_req->slot_number == PNET_SLOT_DAP_IDENT) &&
         (p_read_req->subslot_number ==
          PNET_SUBSLOT_DAP_INTERFACE_1_PORT_0_IDENT))
      {
         /* Todo map subslot to port */
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
      break;
   case PF_IDX_DEV_PDREAL_DATA:
      if (
         (p_read_req->slot_number == PNET_SLOT_DAP_IDENT) &&
         ((p_read_req->subslot_number == PNET_SLOT_DAP_IDENT) ||
          (p_read_req->subslot_number == PNET_SUBMOD_DAP_IDENT)))
      {
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
         (p_read_req->slot_number == PNET_SLOT_DAP_IDENT) &&
         ((p_read_req->subslot_number == PNET_SUBSLOT_DAP_INTERFACE_1_IDENT) ||
          (p_read_req->subslot_number ==
           PNET_SUBSLOT_DAP_INTERFACE_1_PORT_0_IDENT)))
      {
         pf_put_pdport_statistics (
            &net->interface_statistics,
            true,
            p_read_res,
            res_size,
            p_res,
            p_pos);
         ret = 0;
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
 *                                Valid range: 1 .. N, where N is the total
 *                                number of local ports used by p-net stack.
 * @param p_lldp_peer_info In:    Currently received peer info
 */
static void pf_pdport_check_peer_station_name (
   pnet_t * net,
   int loc_port_num,
   const pnet_lldp_peer_info_t * p_lldp_peer_info)
{
   bool peer_stationname_is_correct = false;
   pnet_diag_source_t diag_source = {0};
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   pf_check_peer_t * p_wanted_peer = &p_port_data->pdport.check.peer;

   pf_pdport_init_diag_source (&diag_source, loc_port_num);

   if (p_port_data->pdport.check.active == true)

   if (
      strncmp (
         p_lldp_peer_info->chassis_id.string,
         (char *)p_wanted_peer->peer_station_name,
         p_lldp_peer_info->chassis_id.len) == 0)
   {
      peer_stationname_is_correct = true;
   }

   if (peer_stationname_is_correct == false)
   {
      LOG_DEBUG (
         PNET_LOG,
         "PDPORT(%d): Sending peer station name mismatch alarm: %s (Port: "
         "%s) "
         "Wanted peer station name: %.*s (Port: %.*s)\n",
         __LINE__,
         p_lldp_peer_info->chassis_id.string,
         p_lldp_peer_info->port_id,
         p_wanted_peer->length_peer_station_name,
         p_wanted_peer->peer_station_name,
         p_wanted_peer->length_peer_port_name,
         p_wanted_peer->peer_port_name);

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
         "PDPORT(%d): Peer station name is correct: %s (Port: "
         "%s). Remove diagnosis.\n",
         __LINE__,
         p_lldp_peer_info->chassis_id.string,
         p_lldp_peer_info->port_id);
      (void)pf_diag_std_remove (
         net,
         &diag_source,
         PF_WRT_ERROR_REMOTE_MISMATCH,
         PF_WRT_ERROR_PEER_STATIONNAME_MISMATCH);
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
 *                                Valid range: 1 .. N, where N is the total
 *                                number of local ports used by p-net stack.
 * @param p_lldp_peer_info In:    Peer info
 */
static void pf_pdport_check_peer_port_name (
   pnet_t * net,
   int loc_port_num,
   const pnet_lldp_peer_info_t * p_lldp_peer_info)
{
   bool peer_portname_is_correct = false;
   pnet_diag_source_t diag_source = {0};
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);
   pf_check_peer_t * p_wanted_peer = &p_port_data->pdport.check.peer;

   pf_pdport_init_diag_source (&diag_source, loc_port_num);

   if (
      strncmp (
         p_lldp_peer_info->port_id,
         (char *)p_wanted_peer->peer_port_name,
         p_wanted_peer->length_peer_port_name) == 0)
   {
      peer_portname_is_correct = true;
   }

   pf_pdport_init_diag_source (&diag_source, loc_port_num);

   if (peer_portname_is_correct == false)
   {
      LOG_DEBUG (
         PNET_LOG,
         "PDPORT(%d): Sending peer port name mismatch alarm: %s (Port: "
         "%s) "
         "Wanted peer station name: %.*s (Port: %.*s)\n",
         __LINE__,
         p_lldp_peer_info->chassis_id.string,
         p_lldp_peer_info->port_id,
         p_wanted_peer->length_peer_station_name,
         p_wanted_peer->peer_station_name,
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
         "PDPORT(%d): Peer port name is correct: %s (Port: "
         "%s). Remove diagnosis.\n",
         __LINE__,
         p_lldp_peer_info->chassis_id.string,
         p_lldp_peer_info->port_id);
      (void)pf_diag_std_remove (
         net,
         &diag_source,
         PF_WRT_ERROR_REMOTE_MISMATCH,
         PF_WRT_ERROR_PEER_PORTNAME_MISMATCH);
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
 *                                Valid range: 1 .. N, where N is the total
 *                                number of local ports used by p-net stack.
 * @param p_lldp_peer_info In:    Peer info
 */
static void pf_pdport_run_peer_check (
   pnet_t * net,
   int loc_port_num,
   const pnet_lldp_peer_info_t * p_lldp_peer_info)
{
   pf_port_t * p_port_data = pf_port_get_state (net, loc_port_num);

   if (p_port_data->pdport.check.active == true)
   {
      pf_pdport_check_peer_port_name (net, loc_port_num, p_lldp_peer_info);

      pf_pdport_check_peer_station_name (net, loc_port_num, p_lldp_peer_info);
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
   uint8_t * p_bytes, /* Not const as it is used in a pf_get_info_t */
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
            /* ToDo - Map request to port based on slot information in
             * port_data_check variable */

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

            pf_pdport_run_peer_check (
               net,
               PNET_PORT_1,
               &net->port[0].lldp.peer_info);

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
   uint8_t * p_bytes, /* Not const as it is used in a pf_get_info_t */
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
   int loc_port_num,
   uint8_t * p_bytes,
   uint16_t data_length,
   pnet_result_t * p_result)
{
   int ret = -1;

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
      (void)pf_pdport_save (net);
   }
   return ret;
}

void pf_pdport_peer_indication (
   pnet_t * net,
   int loc_port_num,
   const pnet_lldp_peer_info_t * p_lldp_peer_info)
{
   pf_pdport_run_peer_check (net, loc_port_num, p_lldp_peer_info);
}
