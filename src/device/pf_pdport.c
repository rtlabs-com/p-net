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

      memcpy (&net->port[0].check.peer, &file_port, sizeof (pnet_pdport_nvm_t));
      if (
         net->port[0].check.peer.length_peer_port_name > 0 ||
         net->port[0].check.peer.length_peer_station_name > 0)
      {
         net->port[0].check.active = true;
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

   memcpy (
      &output_port_nvm.peer,
      &net->port[0].check.peer,
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
 * Init PDPort data.
 * Load port configuration from nvm.
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
 *
 * @param net              InOut: The p-net stack instance
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred
 */
int pf_pdport_reset_all (pnet_t * net)
{
   const char * p_file_directory = NULL;

   (void)pf_cmina_get_file_directory (net, &p_file_directory);
   pf_file_clear (p_file_directory, PNET_FILENAME_PDPORT);
   memset (&net->port[0], 0, sizeof (net->port[0]));
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
   switch (p_read_req->index)
   {
   case PF_IDX_SUB_PDPORT_DATA_REAL:
      if (
         (p_read_req->slot_number == PNET_SLOT_DAP_IDENT) &&
         (p_read_req->subslot_number ==
          PNET_SUBSLOT_DAP_INTERFACE_1_PORT_0_IDENT))
      {
         pf_put_pdport_data_real (net, true, p_read_res, res_size, p_res, p_pos);
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
         if (net->port[0].adjust.active)
         {
            pf_put_pdport_data_adj (
               &net->port[0].adjust.peer_to_peer_boundary,
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
         if (net->port[0].check.active)
         {
            pf_put_pdport_data_check (
               &net->port[0].check.peer,
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
         pf_put_pd_real_data (net, true, p_read_res, res_size, p_res, p_pos);
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
 * Write PDPort data check
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
static int pf_pdport_write_data_check (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_req,
   uint8_t * p_bytes, /* Not const as it is used in a pf_get_info_t */
   uint16_t p_datalength,
   pnet_result_t * p_result)
{
   int ret = -1;
   uint16_t pos = 0;
   pf_get_info_t get_info;
   pf_port_data_check_t port_data_check = {0};

   get_info.result = PF_PARSE_OK;
   get_info.p_buf = p_bytes;
   get_info.is_big_endian = true;
   get_info.len = p_datalength;

   pf_get_port_data_check (&get_info, &pos, &port_data_check);

   switch (port_data_check.block_header.block_type)
   {
   case PF_BT_CHECKPEERS:
   {
      pf_check_peers_t check_peer[1];
      pf_get_port_data_check_check_peers (&get_info, &pos, 1, check_peer);
      if (get_info.result == PF_PARSE_OK)
      {
         /* ToDo - Map request to port based on slot information in
          * port_data_check variable */
         net->port[0].check.peer = check_peer->peers[0];
         net->port[0].check.active = true;
         ret = 0;

         /* TODO Generate alarm on mismatch */
      }
   }
   break;
   default:
      LOG_ERROR (
         PF_RPC_LOG,
         "CMWRR(%d): Unsupported port data check block type 0x%x\n",
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
   uint8_t * p_bytes, /* Not const as it is used in a pf_get_info_t */
   uint16_t p_datalength,
   pnet_result_t * p_result)
{
   int ret = -1;
   uint16_t pos = 0;
   pf_get_info_t get_info;
   pf_port_data_adjust_t port_data_adjust = {0};

   get_info.result = PF_PARSE_OK;
   get_info.p_buf = p_bytes;
   get_info.is_big_endian = true;
   get_info.len = p_datalength;

   pf_get_port_data_adjust (&get_info, &pos, &port_data_adjust);

   switch (port_data_adjust.block_header.block_type)
   {
   case PF_BT_PEER_TO_PEER_BOUNDARY:
   {
      pf_adjust_peer_to_peer_boundary_t boundary;
      pf_get_port_data_adjust_peer_to_peer_boundary (&get_info, &pos, &boundary);
      if (get_info.result == PF_PARSE_OK)
      {
         net->port[0].adjust.active = true;
         net->port[0].adjust.peer_to_peer_boundary = boundary;
         ret = 0;
      }
   }
   break;
   default:
      LOG_ERROR (
         PF_RPC_LOG,
         "CMWRR(%d): Unsupported port data adjust block type 0x%x\n",
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
         p_bytes,
         data_length,
         p_result);
      break;
   case PF_IDX_SUB_PDPORT_DATA_ADJ:
      ret = pf_pdport_write_data_adj (
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

   if (ret == 0)
   {
      (void)pf_pdport_save (net);
   }
   return ret;
}
