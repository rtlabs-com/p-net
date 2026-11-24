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
#include "pf_block_reader.h"

static int pf_cmwrr_cm_write_ind (
   pnet_t *,
   pf_ar_t const *,
   pf_iod_write_request_t const *,
   pf_get_info_t *,
   uint16_t,
   pnet_result_t *);

/**
 * @file
 * @brief Implements the Context Management Write Record Responder protocol
 * machine device (CMWRR)
 *
 * Handles a RPC parameter write request.
 *
 * Triggers the \a pnet_write_ind() user callback for some values.
 */

void pf_cmwrr_init (pnet_t * net, pf_ar_t * p_ar)
{
   p_ar->cmwrr_state = PF_CMWRR_STATE_IDLE;
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

void pf_cmwrr_show (const pnet_t * net, const pf_ar_t * p_ar)
{
   const char * s = pf_cmwrr_state_to_string (p_ar->cmwrr_state);

   printf ("CMWRR state           = %s\n", s);
   (void)s;
}

int pf_cmwrr_cmdev_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_event_values_t event)
{
   int ret = -1;
   pf_cmwrr_state_values_t initial_state = p_ar->cmwrr_state;

   LOG_DEBUG (
      PNET_LOG,
      "CMWRR(%d): Received event %s from CMDEV. Initial state %s for AREP "
      "%u.\n",
      __LINE__,
      pf_cmdev_event_to_string (event),
      pf_cmwrr_state_to_string (p_ar->cmwrr_state),
      p_ar->arep);

   switch (p_ar->cmwrr_state)
   {
   case PF_CMWRR_STATE_IDLE:
      if (event == PNET_EVENT_STARTUP)
      {
         p_ar->cmwrr_state = PF_CMWRR_STATE_STARTUP;
      }
      ret = 0;
      break;
   case PF_CMWRR_STATE_STARTUP:
      if (event == PNET_EVENT_PRMEND)
      {
         p_ar->cmwrr_state = PF_CMWRR_STATE_PRMEND;
      }
      else if (event == PNET_EVENT_ABORT)
      {
         p_ar->cmwrr_state = PF_CMWRR_STATE_IDLE;
      }
      ret = 0;
      break;
   case PF_CMWRR_STATE_PRMEND:
      if (event == PNET_EVENT_ABORT)
      {
         p_ar->cmwrr_state = PF_CMWRR_STATE_IDLE;
      }
      else if (event == PNET_EVENT_APPLRDY)
      {
         p_ar->cmwrr_state = PF_CMWRR_STATE_DATA;
      }
      ret = 0;
      break;
   case PF_CMWRR_STATE_DATA:
      if (event == PNET_EVENT_ABORT)
      {
         p_ar->cmwrr_state = PF_CMWRR_STATE_IDLE;
      }
      ret = 0;
      break;
   default:
      LOG_ERROR (
         PNET_LOG,
         "CMWRR(%d): BAD state in cmwrr %u\n",
         __LINE__,
         (unsigned)p_ar->cmwrr_state);
      break;
   }

   if (p_ar->cmwrr_state != initial_state)
   {
      LOG_DEBUG (
         PNET_LOG,
         "CMWRR(%d): New state state %s for AREP %u.\n",
         __LINE__,
         pf_cmwrr_state_to_string (p_ar->cmwrr_state),
         p_ar->arep);
   }

   return ret;
}

static int pf_cmwrr_write_coc (
   pnet_t * net,
   const pf_ar_t * ar,
   pf_iod_write_request_t const * write_request,
   pf_get_info_t * get_info,
   uint16_t pos,
   pnet_result_t * result)
{
   pf_iod_write_request_t request;
   pf_block_header_t header;
   uint16_t rp;

   /* Check that the target subslot corresponds to an interface submodule
      (subslot number 0x8i00 for some i). */
   if ((write_request->subslot_number & ~0x0F00) != 0x8000)
   {
      pf_set_error (
         result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      return -1;
   }
   rp = pos;
   while (rp < (pos + write_request->record_data_length))
   {
      pf_get_block_header (get_info, &rp, &header);
      (void)pf_get_byte (get_info, &rp);
      (void)pf_get_byte (get_info, &rp);
      request.api = pf_get_uint32 (get_info, &rp);
      request.slot_number = pf_get_uint16 (get_info, &rp);
      request.subslot_number = pf_get_uint16 (get_info, &rp);
      (void)pf_get_byte (get_info, &rp);
      (void)pf_get_byte (get_info, &rp);
      request.index = pf_get_uint16 (get_info, &rp);
      if (get_info->result != PF_PARSE_OK)
      {
         return -1;
      }

      if (header.block_length < 16)
      {
         pf_set_error (
            result,
            PNET_ERROR_CODE_WRITE,
            PNET_ERROR_DECODE_PNIORW,
            PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR,
            PNET_ERROR_CODE_2_PNIORW (0));
         return -1;
      }
      request.record_data_length = header.block_length - 16;

      if (
         (rp + request.record_data_length) >
         (pos + write_request->record_data_length))
      {
         pf_set_error (
            result,
            PNET_ERROR_CODE_WRITE,
            PNET_ERROR_DECODE_PNIORW,
            PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR,
            PNET_ERROR_CODE_2_PNIORW (0));
         return -1;
      }
      /* Check that the subslot corresponds to a port submodule of the given
         interface submodule (the offset of the subslot number from the interface
         subslot number should be in [1, 255]). */
      if (
         (request.subslot_number <= write_request->subslot_number) ||
         (request.subslot_number > (write_request->subslot_number + 255)))
      {
         pf_set_error (
            result,
            PNET_ERROR_CODE_WRITE,
            PNET_ERROR_DECODE_PNIORW,
            PNET_ERROR_CODE_1_ACC_INVALID_SLOT_SUBSLOT,
            PNET_ERROR_CODE_2_PNIORW (0));
         return -1;
      }
      /* Recursive combined object containers are not allowed. */
      if (request.index == PF_IDX_SUB_COMBINED_OBJ_CONTAINER)
      {
         pf_set_error (
            result,
            PNET_ERROR_CODE_WRITE,
            PNET_ERROR_DECODE_PNIORW,
            PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
            PNET_ERROR_CODE_2_PNIORW (10));
         return -1;
      }
      if (
         pf_cmwrr_cm_write_ind (
            net,
            ar,
            &request,
            get_info,
            rp,
            result) != 0)
      {
         return -1;
      }
      rp += request.record_data_length;

      /* Skip padding. */
      while (rp % 4)
         ++rp;
   }
   return 0;
}

static int pf_cmwrr_write_im_1 (
   pnet_t * net,
   pf_ar_t const * ar,
   pf_iod_write_request_t const * request,
   pf_get_info_t * get_info,
   pnet_result_t * result)
{
   pnet_im_1_t im_1;
   uint16_t pos = 0;

   pf_get_im_1 (get_info, &pos, &im_1);
   if ((get_info->result != PF_PARSE_OK) || (pos != get_info->len))
   {
      LOG_ERROR (
         PNET_LOG,
         "CMWRR(%d): Wrong length of incoming I&M1 data\n",
         __LINE__);
      pf_set_error (
         result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR,
         PNET_ERROR_CODE_2_PNIORW (0));
      return -1;
   }
   return pf_fspm_write_im_1 (
      net,
      request,
      pf_cmdev_prm_flag (
         ar,
         request->api,
         request->slot_number,
         request->subslot_number),
      &im_1,
      result);
}

static int pf_cmwrr_write_im_2 (
   pnet_t * net,
   pf_ar_t const * ar,
   pf_iod_write_request_t const * request,
   pf_get_info_t * get_info,
   pnet_result_t * result)
{
   pnet_im_2_t im_2;
   uint16_t pos = 0;

   pf_get_im_2 (get_info, &pos, &im_2);
   if ((get_info->result != PF_PARSE_OK) || (pos != get_info->len))
   {
      LOG_ERROR (
         PNET_LOG,
         "CMWRR(%d): Wrong length of incoming I&M2 data\n",
         __LINE__);
      pf_set_error (
         result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR,
         PNET_ERROR_CODE_2_PNIORW (0));
      return -1;
   }
   return pf_fspm_write_im_2 (
      net,
      request,
      pf_cmdev_prm_flag (
         ar,
         request->api,
         request->slot_number,
         request->subslot_number),
      &im_2,
      result);
}

static int pf_cmwrr_write_im_3 (
   pnet_t * net,
   pf_ar_t const * ar,
   pf_iod_write_request_t const * request,
   pf_get_info_t * get_info,
   pnet_result_t * result)
{
   pnet_im_3_t im_3;
   uint16_t pos = 0;

   pf_get_im_3 (get_info, &pos, &im_3);
   if ((get_info->result != PF_PARSE_OK) || (pos != get_info->len))
   {
      LOG_ERROR (
         PNET_LOG,
         "CMWRR(%d): Wrong length of incoming I&M3 data\n",
         __LINE__);
      pf_set_error (
         result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR,
         PNET_ERROR_CODE_2_PNIORW (0));
      return -1;
   }
   return pf_fspm_write_im_3 (
      net,
      request,
      pf_cmdev_prm_flag (
         ar,
         request->api,
         request->slot_number,
         request->subslot_number),
      &im_3,
      result);
}

static int pf_cmwrr_write_im_4 (
   pnet_t * net,
   pf_ar_t const * ar,
   pf_iod_write_request_t const * request,
   pf_get_info_t * get_info,
   pnet_result_t * result)
{
   pnet_im_4_t im_4;
   uint16_t pos = 0;

   pf_get_im_4 (get_info, &pos, &im_4);
   if ((get_info->result != PF_PARSE_OK) || (pos != get_info->len))
   {
      LOG_ERROR (
         PNET_LOG,
         "CMWRR(%d): Wrong length of incoming I&M4 data\n",
         __LINE__);
      pf_set_error (
         result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR,
         PNET_ERROR_CODE_2_PNIORW (0));
      return -1;
   }
   return pf_fspm_write_im_4 (
      net,
      request,
      pf_cmdev_prm_flag (
         ar,
         request->api,
         request->slot_number,
         request->subslot_number),
      &im_4,
      result);
}

static int pf_cmwrr_write_im_index (
   pnet_t * net,
   pf_ar_t const * ar,
   pf_iod_write_request_t const * request,
   uint16_t length,
   uint8_t const * data,
   pnet_result_t * result)
{
   pf_submodule_t * submodule;
   pf_get_info_t get_info;

   if (
      (request->index == PF_IDX_SUB_IM_0) ||
      (pf_cmdev_get_submodule_full (
          net,
          request->api,
          request->slot_number,
          request->subslot_number,
          &submodule) != 0) ||
      ((submodule->flags & PNET_SMF_IM_CARRIER) == 0))
   {
      /* Access / access denied */
      pf_set_error (
         result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_ACCESS_DENIED,
         PNET_ERROR_CODE_2_PNIORW (0));
      return -1;
   }

   get_info.result = PF_PARSE_OK;
   get_info.p_frame = NULL;
   get_info.p_array = data;
   get_info.is_big_endian = true;
   get_info.len = length;

   switch (request->index)
   {
   case PF_IDX_SUB_IM_1:
      return pf_cmwrr_write_im_1 (net, ar, request, &get_info, result);
      break;
   case PF_IDX_SUB_IM_2:
      return pf_cmwrr_write_im_2 (net, ar, request, &get_info, result);
      break;
   case PF_IDX_SUB_IM_3:
      return pf_cmwrr_write_im_3 (net, ar, request, &get_info, result);
      break;
   case PF_IDX_SUB_IM_4:
      return pf_cmwrr_write_im_4 (net, ar, request, &get_info, result);
      break;
   default:
      pf_set_error (
         result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      break;
   }
   return -1;
}

/**
 * @internal
 * Skip the block header at the start of the record data of a write.
 *
 * @param write_request    In:    The write request.
 * @param get_info         InOut: Parser state of the request.
 * @param req_pos          In:    Position of the record data.
 * @param pos              Out:   Position after the block header.
 * @param result           Out:   Detailed error information if returning != 0
 *                                and the request could be parsed.
 * @return  0  if the block header is within the record data.
 *          -1 if it is not, or if the request could not be parsed.
 */
static int pf_cmwrr_skip_block_header (
   pf_iod_write_request_t const * write_request,
   pf_get_info_t * get_info,
   uint16_t req_pos,
   uint16_t * pos,
   pnet_result_t * result)
{
   pf_block_header_t block_header;

   *pos = req_pos;
   pf_get_block_header (get_info, pos, &block_header);
   if (get_info->result != PF_PARSE_OK)
   {
      return -1;
   }
   if ((uint32_t)(*pos - req_pos) > write_request->record_data_length)
   {
      pf_set_error (
         result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR,
         PNET_ERROR_CODE_2_PNIORW (0));
      return -1;
   }
   return 0;
}

static int pf_cmwrr_cm_write_ind (
   pnet_t * net,
   pf_ar_t const * p_ar,
   pf_iod_write_request_t const * p_write_request,
   pf_get_info_t * get_info,
   uint16_t req_pos,
   pnet_result_t * p_result)
{
   uint16_t rpos;

   if (pf_index_is_user_specific (p_write_request->index))
   {
      /* User defined indexes */
      return pf_fspm_cm_write_ind (
         net,
         p_ar,
         p_write_request,
         p_write_request->record_data_length,
         &get_info->p_array[req_pos],
         p_result);
   }
   else if (
      (PF_IDX_SUB_IM_0 <= p_write_request->index) &&
      (p_write_request->index <= PF_IDX_SUB_IM_15))
   {
      if (
         pf_cmwrr_skip_block_header (
            p_write_request,
            get_info,
            req_pos,
            &rpos,
            p_result) != 0)
      {
         return -1;
      }

      /* I&M data */
      return pf_cmwrr_write_im_index (
         net,
         p_ar,
         p_write_request,
         p_write_request->record_data_length - (rpos - req_pos),
         &get_info->p_array[rpos],
         p_result);
   }
   else
   {
      switch (p_write_request->index)
      {
      case PF_IDX_SUB_COMBINED_OBJ_CONTAINER:
         return pf_cmwrr_write_coc (
            net,
            p_ar,
            p_write_request,
            get_info,
            req_pos,
            p_result);
         break;
#if PNET_OPTION_SNMP && PNET_OPTION_SECURITY_LEVEL_1
      case PF_IDX_CIM_SNMP_ADJUST:
         if (
            pf_cmwrr_skip_block_header (
               p_write_request,
               get_info,
               req_pos,
               &rpos,
               p_result) != 0)
         {
            return -1;
         }

         return pf_cim_write_request (
            net,
            p_write_request,
            &get_info->p_array[rpos],
            p_write_request->record_data_length - (rpos - req_pos),
            p_result);
         break;
#endif /* PNET_OPTION_SNMP && PNET_OPTION_SECURITY_LEVEL_1 */
      case PF_IDX_SUB_PDPORT_DATA_CHECK:
      case PF_IDX_SUB_PDPORT_DATA_ADJ:
      case PF_IDX_SUB_PDINTF_ADJUST:
         if (
            pf_cmwrr_skip_block_header (
               p_write_request,
               get_info,
               req_pos,
               &rpos,
               p_result) != 0)
         {
            return -1;
         }

         return pf_pdport_write_req (
            net,
            p_ar,
            p_write_request,
            &get_info->p_array[rpos],
            p_write_request->record_data_length - (rpos - req_pos),
            p_result);
         break;
#if PNET_OPTION_MRP
      case PF_IDX_SUB_PDINTF_MRP_REAL:
      case PF_IDX_SUB_PDINTF_MRP_CHECK:
      case PF_IDX_SUB_PDINTF_MRP_ADJUST:
      case PF_IDX_SUB_PDPORT_MRP_ADJUST:
      case PF_IDX_SUB_PDPORT_MRP_REAL:
         if (
            pf_cmwrr_skip_block_header (
               p_write_request,
               get_info,
               req_pos,
               &rpos,
               p_result) != 0)
         {
            return -1;
         }

         return pf_pdport_write_mrp_req (
            net,
            p_ar,
            p_write_request,
            get_info->p_array,
            rpos,
            req_pos + p_write_request->record_data_length,
            p_result);
         break;
#endif
      default:
         break;
      }
      return -1;
   }
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
 * @param p_ar             In:    The AR instance.
 * @param p_write_request  In:    The IODWrite request.
 * @param p_req_buf        In:    The request buffer.
 * @param data_length      In:    Size of the data to write.
 * @param p_req_pos        In:    Position within the request buffer.
 * @param p_result         Out:   Detailed error information if returning != 0
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmwrr_write (
   pnet_t * net,
   const pf_ar_t * p_ar,
   pf_iod_write_request_t const * p_write_request,
   pf_get_info_t * get_info,
   uint16_t req_pos,
   pnet_result_t * p_result)
{
   int ret = -1;
   bool access_granted = false;
   pf_submodule_t * submodule = NULL;

   if (
      (p_write_request->slot_number == PNET_SLOT_DAP_IDENT) &&
      (pf_port_subslot_is_dap_port_id (net, p_write_request->subslot_number)))
   {
      LOG_INFO (
         PNET_LOG,
         "CMWRR(%d): PLC is writing slot %u subslot 0x%04X index 0x%04X "
         "(local port %u) \"%s\" for AREP %u\n",
         __LINE__,
         p_write_request->slot_number,
         p_write_request->subslot_number,
         p_write_request->index,
         pf_port_dap_subslot_to_local_port (
            net,
            p_write_request->subslot_number),
         pf_index_to_logstring (p_write_request->index),
         p_ar->arep);
   }
   else
   {
      LOG_INFO (
         PNET_LOG,
         "CMWRR(%d): PLC is writing slot %u subslot 0x%04X index 0x%04X "
         "\"%s\" for AREP %u\n",
         __LINE__,
         p_write_request->slot_number,
         p_write_request->subslot_number,
         p_write_request->index,
         pf_index_to_logstring (p_write_request->index),
         p_ar->arep);
   }

   if (
      pf_cmdev_get_submodule_full (
         net,
         p_write_request->api,
         p_write_request->slot_number,
         p_write_request->subslot_number,
         &submodule) == 0)
   {
      if (p_ar->ar_param.ar_properties.device_access)
      {
         /* Device Access ARs may write to data records regardless of ownership
          * if the application allows it.
          */
         if (net->fspm_cfg.da_write_cb)
         {
            access_granted =
               (net->fspm_cfg.da_write_cb (
                   net,
                   net->fspm_cfg.cb_arg,
                   p_ar->arep,
                   p_write_request->api,
                   p_write_request->slot_number,
                   p_write_request->subslot_number,
                   p_write_request->index) == 0);
         }
         else
         {
            /* Default behavior: allow I&M1, deny otherwise */
            access_granted = (p_write_request->index == PF_IDX_SUB_IM_1);
         }
      }
      else
      {
         access_granted =
            (((submodule->ownsm_state == PF_OWNSM_STATE_IOC) ||
              (submodule->ownsm_state == PF_OWNSM_STATE_IOS)) &&
             pf_cmrpc_ar_set_equal (submodule->owner, p_ar));
      }
   }

   if (access_granted)
   {
      ret = pf_cmwrr_cm_write_ind (
         net,
         p_ar,
         p_write_request,
         get_info,
         req_pos,
         p_result);
   }
   else
   {
      pf_set_error (
         p_result,
         PNET_ERROR_CODE_WRITE,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_ACCESS_DENIED,
         PNET_ERROR_CODE_2_PNIORW (0));
   }

   if (ret != 0)
   {
      LOG_INFO (
         PNET_LOG,
         "CMRWRR(%d): Could not write index 0x%04X for slot %u subslot "
         "0x%04X. Error 0x%02X %02X %02X %02X Add %u %u\n",
         __LINE__,
         p_write_request->index,
         p_write_request->slot_number,
         p_write_request->subslot_number,
         p_result->pnio_status.error_code,
         p_result->pnio_status.error_decode,
         p_result->pnio_status.error_code_1,
         p_result->pnio_status.error_code_2,
         p_result->add_data_1,
         p_result->add_data_2);
   }
   return ret;
}

int pf_cmwrr_rm_write_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_request,
   pf_iod_write_result_t * p_write_result,
   pnet_result_t * p_result,
   pf_get_info_t * get_info,
   uint16_t * p_req_pos)
{
   int ret = -1;

   p_write_result->sequence_number = p_write_request->sequence_number;
   p_write_result->ar_uuid = p_write_request->ar_uuid;
   p_write_result->api = p_write_request->api;
   p_write_result->slot_number = p_write_request->slot_number;
   p_write_result->subslot_number = p_write_request->subslot_number;
   p_write_result->index = p_write_request->index;
   p_write_result->record_data_length = 0;

   switch (p_ar->cmwrr_state)
   {
   case PF_CMWRR_STATE_IDLE:
      p_result->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
      p_result->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
      p_result->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_STATE_CONFLICT;
      break;
   case PF_CMWRR_STATE_STARTUP:
      if ((p_ar->io_ar != NULL) && p_ar->ar_state == PF_AR_STATE_BACKUP)
      {
         p_result->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
         p_result->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_result->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_BACKUP;
      }
      else
      {
         ret = pf_cmwrr_write (
            net,
            p_ar,
            p_write_request,
            get_info,
            *p_req_pos,
            p_result);
      }
      break;
   case PF_CMWRR_STATE_PRMEND:
      p_result->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
      p_result->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
      p_result->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_STATE_CONFLICT;
      break;
   case PF_CMWRR_STATE_DATA:
      if (p_ar->ar_state == PF_AR_STATE_BACKUP)
      {
         p_result->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
         p_result->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_result->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_BACKUP;
      }
      else
      {
         ret = pf_cmwrr_write (
            net,
            p_ar,
            p_write_request,
            get_info,
            *p_req_pos,
            p_result);
      }
      break;
   }
   *p_req_pos += p_write_request->record_data_length;

   p_write_result->add_data_1 = p_result->add_data_1;
   p_write_result->add_data_2 = p_result->add_data_2;

   /* Update the CMSM timer, which monitors start-up timeout */
   if (pf_cmsm_cm_write_ind (net, p_ar, p_write_request) != 0)
   {
      ret = -1;
   }

   return ret;
}
