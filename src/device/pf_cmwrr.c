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

#endif

#include "pf_includes.h"
#include "pf_block_reader.h"
#include "pf_block_writer.h"

/**
 * @file
 * @brief Implements the Context Management Write Record Responder protocol
 * machine device (CMWRR)
 *
 * Handles a RPC parameter write request.
 *
 * Triggers the \a pnet_write_ind() user callback for some values.
 */

void pf_cmwrr_init (pnet_t * net)
{
   net->cmwrr_state = PF_CMWRR_STATE_IDLE;
}

/**
 * @internal
 * Return a string representation of a CMWRR state.
 * @param state            In:   The CMWRR state
 * @return A string representation of the CMWRR state.
 */
static const char * pf_cmwrr_state_to_string (pf_cmwrr_state_values_t state)
{
   const char * s = "<unknown>";

   switch (state)
   {
   case PF_CMWRR_STATE_IDLE:
      s = "PF_CMWRR_STATE_IDLE";
      break;
   case PF_CMWRR_STATE_STARTUP:
      s = "PF_CMWRR_STATE_STARTUP";
      break;
   case PF_CMWRR_STATE_PRMEND:
      s = "PF_CMWRR_STATE_PRMEND";
      break;
   case PF_CMWRR_STATE_DATA:
      s = "PF_CMWRR_STATE_DATA";
      break;
   }

   return s;
}

void pf_cmwrr_show (pnet_t * net, pf_ar_t * p_ar)
{
   const char * s = pf_cmwrr_state_to_string (net->cmwrr_state);

   printf ("CMWRR state           = %s\n", s);
   (void)s;
}

int pf_cmwrr_cmdev_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_event_values_t event)
{
   switch (net->cmwrr_state)
   {
   case PF_CMWRR_STATE_IDLE:
      if (event == PNET_EVENT_STARTUP)
      {
         net->cmwrr_state = PF_CMWRR_STATE_STARTUP;
      }
      break;
   case PF_CMWRR_STATE_STARTUP:
      if (event == PNET_EVENT_PRMEND)
      {
         net->cmwrr_state = PF_CMWRR_STATE_PRMEND;
      }
      else if (event == PNET_EVENT_ABORT)
      {
         net->cmwrr_state = PF_CMWRR_STATE_IDLE;
      }
      break;
   case PF_CMWRR_STATE_PRMEND:
      if (event == PNET_EVENT_ABORT)
      {
         net->cmwrr_state = PF_CMWRR_STATE_IDLE;
      }
      else if (event == PNET_EVENT_APPLRDY)
      {
         net->cmwrr_state = PF_CMWRR_STATE_DATA;
      }
      break;
   case PF_CMWRR_STATE_DATA:
      if (event == PNET_EVENT_ABORT)
      {
         net->cmwrr_state = PF_CMWRR_STATE_IDLE;
      }
      break;
   default:
      LOG_ERROR (
         PNET_LOG,
         "CMWRR(%d): BAD state in cmwrr %u\n",
         __LINE__,
         (unsigned)net->cmwrr_state);
      break;
   }

   return -1;
}

static int pf_cmwrr_pdport_data_check (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_iod_write_request_t * p_write_request,
   uint8_t * p_bytes,
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

         /* Todo -  Generate Alarm on mismatch*/

         /* ToDo - Store peer check information*/
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

int pf_cmwrr_pdport_data_adj (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_iod_write_request_t * p_write_request,
   uint8_t * p_bytes,
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

/**
 * @internal
 * Write one data record.
 *
 * This function performs a write of one data record.
 * The index in the IODWrite request selects the item to write to.
 *
 * Triggers the \a pnet_write_ind() user callback for some values.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param p_write_request  In:   The IODWrite request.
 * @param p_req_buf        In:   The request buffer.
 * @param data_length      In:   Size of the data to write.
 * @param p_req_pos        InOut:Position within the request buffer.
 * @param p_result         Out:  Detailed error information.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmwrr_write (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_iod_write_request_t * p_write_request,
   uint8_t * p_req_buf,
   uint16_t data_length,
   uint16_t * p_req_pos,
   pnet_result_t * p_result)
{
   int ret = -1;

   if (p_write_request->index < 0x7fff)
   {
      ret = pf_fspm_cm_write_ind (
         net,
         p_ar,
         p_write_request,
         data_length,
         &p_req_buf[*p_req_pos],
         p_result);
   }
   else if (
      (PF_IDX_SUB_IM_0 <= p_write_request->index) &&
      (p_write_request->index <= PF_IDX_SUB_IM_15))
   {
      ret = pf_fspm_cm_write_ind (
         net,
         p_ar,
         p_write_request,
         data_length,
         &p_req_buf[*p_req_pos],
         p_result);
   }
   else
   {
      switch (p_write_request->index)
      {
      case PF_IDX_SUB_PDPORT_DATA_CHECK:
         ret = pf_cmwrr_pdport_data_check (
            net,
            p_ar,
            p_write_request,
            &p_req_buf[*p_req_pos],
            data_length,
            p_result);
         break;
      case PF_IDX_SUB_PDPORT_DATA_ADJ:
         ret = pf_cmwrr_pdport_data_adj (
            net,
            p_ar,
            p_write_request,
            &p_req_buf[*p_req_pos],
            data_length,
            p_result);
         break;
      default:
         break;
      }
   }

   (*p_req_pos) += data_length;

   return ret;
}

int pf_cmwrr_rm_write_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_iod_write_request_t * p_write_request,
   pf_iod_write_result_t * p_write_result,
   pnet_result_t * p_result,
   uint8_t * p_req_buf, /* request buffer */
   uint16_t data_length,
   uint16_t * p_req_pos) /* In/out: position in request buffer */
{
   int ret = -1;

   p_write_result->sequence_number = p_write_request->sequence_number;
   p_write_result->ar_uuid = p_write_request->ar_uuid;
   p_write_result->api = p_write_request->api;
   p_write_result->slot_number = p_write_request->slot_number;
   p_write_result->subslot_number = p_write_request->subslot_number;
   p_write_result->index = p_write_request->index;
   p_write_result->record_data_length = 0;

   switch (net->cmwrr_state)
   {
   case PF_CMWRR_STATE_IDLE:
      p_result->pnio_status.error_code = PNET_ERROR_CODE_PNIO;
      p_result->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
      p_result->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_STATE_CONFLICT;
      break;
   case PF_CMWRR_STATE_STARTUP:
      if (p_ar->ar_state == PF_AR_STATE_BACKUP)
      {
         p_result->pnio_status.error_code = PNET_ERROR_CODE_PNIO;
         p_result->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_result->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_BACKUP;
      }
      else
      {
         ret = pf_cmwrr_write (
            net,
            p_ar,
            p_write_request,
            p_req_buf,
            data_length,
            p_req_pos,
            p_result);
      }
      break;
   case PF_CMWRR_STATE_PRMEND:
      p_result->pnio_status.error_code = PNET_ERROR_CODE_PNIO;
      p_result->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
      p_result->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_STATE_CONFLICT;
      break;
   case PF_CMWRR_STATE_DATA:
      if (p_ar->ar_state == PF_AR_STATE_BACKUP)
      {
         p_result->pnio_status.error_code = PNET_ERROR_CODE_PNIO;
         p_result->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_result->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_BACKUP;
      }
      else
      {
         ret = pf_cmwrr_write (
            net,
            p_ar,
            p_write_request,
            p_req_buf,
            data_length,
            p_req_pos,
            p_result);
      }
      break;
   }

   p_write_result->add_data_1 = p_result->add_data_1;
   p_write_result->add_data_2 = p_result->add_data_2;

   /* Update the CMSM timer, which monitors start-up timeout */
   ret = pf_cmsm_cm_write_ind (net, p_ar, p_write_request);

   return ret;
}
