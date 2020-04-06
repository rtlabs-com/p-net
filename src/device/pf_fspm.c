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
#define os_udp_recvfrom mock_os_udp_recvfrom
#define os_udp_sendto mock_os_udp_sendto
#define os_udp_open mock_os_udp_open
#define os_udp_close mock_os_udp_close

#define os_eth_init mock_os_eth_init
#endif

#include <string.h>
#include "osal.h"
#include "pf_includes.h"
#include "pf_block_reader.h"

int pf_fspm_init(
   pnet_t                  *net,
   const pnet_cfg_t        *p_cfg)
{
   /*
    * Save a copy before doing anything else.
    * Some init functions may ask for it.
    */
   net->fspm_cfg = *p_cfg;

   /* Also save the default settings */
   net->p_fspm_default_cfg = p_cfg;

   if (net->fspm_log_book_mutex == NULL)
   {
      net->fspm_log_book_mutex = os_mutex_create();
   }

   return 0;
}

void pf_fspm_create_log_book_entry(
   pnet_t                     *net,
   uint32_t                   arep,
   const pnet_pnio_status_t   *p_pnio_status,
   uint32_t                   entry_detail)
{
   uint16_t put;
   uint32_t time = 0;
   pf_ar_t     *p_ar = NULL;
   pf_uuid_t   *p_ar_uuid = NULL;

   if (pf_ar_find_by_arep(net, arep, &p_ar) == 0)
   {
      p_ar_uuid = &p_ar->ar_param.ar_uuid;
      os_mutex_lock(net->fspm_log_book_mutex);
      put = net->fspm_log_book.put;
      net->fspm_log_book.put++;
      if (net->fspm_log_book.put >= NELEMENTS(net->fspm_log_book.entries))
      {
         net->fspm_log_book.put = 0;
         net->fspm_log_book.wrap = true;
      }

      time = os_get_current_time_us();
      net->fspm_log_book.entries[put].time_ts.status = PF_TS_STATUS_LOCAL_ARB;
      net->fspm_log_book.entries[put].time_ts.sec_hi = 0;
      net->fspm_log_book.entries[put].time_ts.sec_lo = time/1000000;
      net->fspm_log_book.entries[put].time_ts.nano_sec = (time%1000000)*1000;

      net->fspm_log_book.entries[put].ar_uuid = *p_ar_uuid;
      net->fspm_log_book.entries[put].pnio_status = *p_pnio_status;
      net->fspm_log_book.entries[put].entry_detail = entry_detail;
      os_mutex_unlock(net->fspm_log_book_mutex);
   }
}

/******************* Execute user callbacks *********************************/

int pf_fspm_exp_module_ind(
   pnet_t                  *net,
   uint32_t                api,
   uint16_t                slot,
   uint32_t                module_ident)
{
   int ret = -1;

   if (net->fspm_cfg.exp_module_cb != NULL)
   {
      ret = net->fspm_cfg.exp_module_cb(net, net->fspm_cfg.cb_arg, api, slot, module_ident);
   }

   return ret;
}

int pf_fspm_exp_submodule_ind(
   pnet_t                  *net,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint32_t                module_ident,
   uint32_t                submodule_ident)
{
   int ret = -1;

   if (net->fspm_cfg.exp_submodule_cb != NULL)
   {
      ret = net->fspm_cfg.exp_submodule_cb(net, net->fspm_cfg.cb_arg, api, slot, subslot, module_ident, submodule_ident);
   }

   return ret;
}

int pf_fspm_data_status_changed(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iocr_t               *p_iocr,
   uint8_t                 changes,
   uint8_t                 data_status)
{
   int ret = -1;

   if (net->fspm_cfg.new_data_status_cb != NULL)
   {
      ret = net->fspm_cfg.new_data_status_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, p_iocr->crep, changes, data_status);
   }

   return ret;
}

int pf_fspm_ccontrol_cnf(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_result_t           *p_result)
{
   int ret = 0;

   if (net->fspm_cfg.ccontrol_cb != NULL)
   {
      ret = net->fspm_cfg.ccontrol_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, p_result);
   }

   return ret;
}

int pf_fspm_cm_read_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iod_read_request_t   *p_read_request,
   uint8_t                 **pp_read_data,
   uint16_t                *p_read_length,
   pnet_result_t           *p_read_status)
{
   int ret = -1;

   if (p_read_request->index <= PF_IDX_USER_MAX)
   {
      /* Application-specific data records */
      if (net->fspm_cfg.read_cb != NULL)
      {
         ret = net->fspm_cfg.read_cb(net, net->fspm_cfg.cb_arg,
            p_ar->arep, p_read_request->api,
            p_read_request->slot_number, p_read_request->subslot_number,
            p_read_request->index, p_read_request->sequence_number,
            pp_read_data, p_read_length, p_read_status);
      }
      else
      {
         p_read_status->pnio_status.error_code = PNET_ERROR_CODE_READ;
         p_read_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_read_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_APP_NOT_SUPPORTED;
         p_read_status->pnio_status.error_code_2 = 0;
      }
   }
   else if ((PF_IDX_SUB_IM_0 <= p_read_request->index) && (p_read_request->index <= PF_IDX_SUB_IM_15))
   {
      /* Read I&M data - This is handled internally. */
      switch (p_read_request->index)
      {
      case PF_IDX_SUB_IM_0:
         if (*p_read_length >= sizeof(net->fspm_cfg.im_0_data))
         {
            *pp_read_data = (uint8_t *)&net->fspm_cfg.im_0_data;
            *p_read_length = sizeof(net->fspm_cfg.im_0_data);
            ret = 0;
         }
         else
         {
            p_read_status->pnio_status.error_code = PNET_ERROR_CODE_READ;
            p_read_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
            p_read_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_APP_READ_ERROR;
            p_read_status->pnio_status.error_code_2 = 0;
         }
         break;
      case PF_IDX_SUB_IM_1:
         *pp_read_data = (uint8_t *)&net->fspm_cfg.im_1_data;
         *p_read_length = sizeof(net->fspm_cfg.im_1_data);
         ret = 0;
         break;
      case PF_IDX_SUB_IM_2:
         *pp_read_data = (uint8_t *)&net->fspm_cfg.im_2_data;
         *p_read_length = sizeof(net->fspm_cfg.im_2_data);
         ret = 0;
         break;
      case PF_IDX_SUB_IM_3:
         *pp_read_data = (uint8_t *)&net->fspm_cfg.im_3_data;
         *p_read_length = sizeof(net->fspm_cfg.im_3_data);
         ret = 0;
         break;
      case PF_IDX_SUB_IM_4:
         *pp_read_data = (uint8_t *)&net->fspm_cfg.im_4_data;
         *p_read_length = sizeof(net->fspm_cfg.im_4_data);
         ret = 0;
         break;
      default:
         /* Nothing if data not available here */
         break;
      }
   }
   else if (p_read_request->index == PF_IDX_DEV_LOGBOOK_DATA)
   {
      *pp_read_data = (uint8_t *)&net->fspm_log_book;
      *p_read_length = sizeof(net->fspm_log_book);
      ret = 0;
   }
   else
   {
      /* Nothing if data not available here */
   }

   return ret;
}

int pf_fspm_cm_write_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iod_write_request_t  *p_write_request,
   uint16_t                write_length,
   uint8_t                 *p_write_data,
   pnet_result_t           *p_write_status)
{
   int                     ret = -1;
   pf_get_info_t           get_info;
   uint16_t                pos = 0;

   if (p_write_request->index <= PF_IDX_USER_MAX)
   {
      if (net->fspm_cfg.write_cb != NULL)
      {
         ret = net->fspm_cfg.write_cb(net, net->fspm_cfg.cb_arg,
            p_ar->arep, p_write_request->api,
            p_write_request->slot_number, p_write_request->subslot_number,
            p_write_request->index, p_write_request->sequence_number,
            write_length, p_write_data, p_write_status);
      }
      else
      {
         p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
         p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_APP_NOT_SUPPORTED;
         p_write_status->pnio_status.error_code_2 = 0;
      }
   }
   else if ((PF_IDX_SUB_IM_0 <= p_write_request->index) && (p_write_request->index <= PF_IDX_SUB_IM_15))
   {
      /* Write I&M data - This is handled internally. */
      switch (p_write_request->index)
      {
      case PF_IDX_SUB_IM_0:      /* read-only */
         p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
         p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_ACCESS_DENIED;
         p_write_status->pnio_status.error_code_2 = 0;
         ret = 0;
         break;
      case PF_IDX_SUB_IM_1:
         get_info.result = PF_PARSE_OK;
         get_info.p_buf = p_write_data;
         get_info.is_big_endian = true;
         get_info.len = write_length;      /* Bytes in input buffer */

         pf_get_im_1(&get_info, &pos, &net->fspm_cfg.im_1_data);
         if ((get_info.result == PF_PARSE_OK) && (pos == write_length))
         {
            ret = 0;
         }
         else
         {
            p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
            p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
            p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR;
            p_write_status->pnio_status.error_code_2 = 0;
         }
         break;
      case PF_IDX_SUB_IM_2:
         /* Do not count the terminator byte */
         if (write_length == (sizeof(net->fspm_cfg.im_2_data) - 1))
         {
            memcpy(&net->fspm_cfg.im_2_data, p_write_data, sizeof(net->fspm_cfg.im_2_data) - 1);
            net->fspm_cfg.im_2_data.im_date[sizeof(net->fspm_cfg.im_2_data) - 1] = '\0';
            ret = 0;
         }
         else
         {
            p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
            p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
            p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR;
            p_write_status->pnio_status.error_code_2 = 0;
         }
         break;
      case PF_IDX_SUB_IM_3:
         /* Do not count the terminator byte */
         if (write_length == (sizeof(net->fspm_cfg.im_3_data) - 1))
         {
            memcpy(&net->fspm_cfg.im_3_data, p_write_data, sizeof(net->fspm_cfg.im_3_data) - 1);
            net->fspm_cfg.im_3_data.im_descriptor[sizeof(net->fspm_cfg.im_3_data) - 1] = '\0';
            ret = 0;
         }
         else
         {
            p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
            p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
            p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR;
            p_write_status->pnio_status.error_code_2 = 0;
         }
         break;
      case PF_IDX_SUB_IM_4:
         if (write_length == sizeof(net->fspm_cfg.im_4_data))
         {
            memcpy(&net->fspm_cfg.im_4_data, p_write_data, sizeof(net->fspm_cfg.im_4_data));
            ret = 0;
         }
         else
         {
            p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
            p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
            p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR;
            p_write_status->pnio_status.error_code_2 = 0;
         }
         break;
      default:
         p_write_status->pnio_status.error_code = PNET_ERROR_CODE_WRITE;
         p_write_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
         p_write_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_INVALID_INDEX;
         p_write_status->pnio_status.error_code_2 = 0;
         break;
      }
   }
   else
   {
      /* Nothing if write not possible here */
   }

   return ret;
}

int pf_fspm_clear_im_data(
   pnet_t                  *net)
{
   memset(net->fspm_cfg.im_1_data.im_tag_function, ' ', sizeof(net->fspm_cfg.im_1_data.im_tag_function));
   net->fspm_cfg.im_1_data.im_tag_function[sizeof(net->fspm_cfg.im_1_data.im_tag_function) - 1] = '\0';
   memset(net->fspm_cfg.im_1_data.im_tag_location, ' ', sizeof(net->fspm_cfg.im_1_data.im_tag_location));
   net->fspm_cfg.im_1_data.im_tag_location[sizeof(net->fspm_cfg.im_1_data.im_tag_location) - 1] = '\0';
   memset(net->fspm_cfg.im_2_data.im_date, ' ', sizeof(net->fspm_cfg.im_2_data.im_date));
   net->fspm_cfg.im_2_data.im_date[sizeof(net->fspm_cfg.im_2_data.im_date) - 1] = '\0';
   memset(net->fspm_cfg.im_3_data.im_descriptor, ' ', sizeof(net->fspm_cfg.im_3_data.im_descriptor));
   net->fspm_cfg.im_3_data.im_descriptor[sizeof(net->fspm_cfg.im_3_data.im_descriptor) - 1] = '\0';
   memset(net->fspm_cfg.im_4_data.im_signature, 0, sizeof(net->fspm_cfg.im_4_data.im_signature));

   return 0;
}

void pf_fspm_get_cfg(
   pnet_t                  *net,
   pnet_cfg_t              **pp_cfg)
{
   if (pp_cfg != NULL)
   {
      *pp_cfg = &net->fspm_cfg;
   }
}

void pf_fspm_get_default_cfg(
   pnet_t                  *net,
   const pnet_cfg_t        **pp_cfg)
{
   if (pp_cfg != NULL)
   {
      *pp_cfg = net->p_fspm_default_cfg;
   }
}

int pf_fspm_cm_connect_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_result_t           *p_result)
{
   int ret = 0;

   if (net->fspm_cfg.connect_cb != NULL)
   {
      ret = net->fspm_cfg.connect_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, p_result);
   }

   return ret;
}

int pf_fspm_cm_release_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_result_t           *p_result)
{
   int ret = 0;

   if (net->fspm_cfg.release_cb != NULL)
   {
      ret = net->fspm_cfg.release_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, p_result);
   }

   return ret;
}

int pf_fspm_cm_dcontrol_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_control_command_t  control_command,
   pnet_result_t           *p_result)
{
   int ret = 0;

   if (net->fspm_cfg.dcontrol_cb != NULL)
   {
      ret = net->fspm_cfg.dcontrol_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, control_command, p_result);
   }

   return ret;
}

int pf_fspm_state_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_event_values_t     event)
{
   int ret = 0;

   switch (event)
   {
   case    PNET_EVENT_ABORT:     LOG_INFO(PNET_LOG, "CMDEV event ABORT\n"); break;
   case    PNET_EVENT_STARTUP:   LOG_INFO(PNET_LOG, "CMDEV event STARTUP\n"); break;
   case    PNET_EVENT_PRMEND:    LOG_INFO(PNET_LOG, "CMDEV event PRMEND\n"); break;
   case    PNET_EVENT_APPLRDY:   LOG_INFO(PNET_LOG, "CMDEV event APPLRDY\n"); break;
   case    PNET_EVENT_DATA:      LOG_INFO(PNET_LOG, "CMDEV event DATA\n"); break;
   }
   if (net->fspm_cfg.state_cb != NULL)
   {
      ret = net->fspm_cfg.state_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, event);
   }

   return ret;
}

int pf_fspm_aplmr_alarm_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                data_len,
   uint16_t                data_usi,
   uint8_t                 *p_data)
{
   int ret = 0;

   if (net->fspm_cfg.alarm_ind_cb != NULL)
   {
      ret = net->fspm_cfg.alarm_ind_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, api, slot, subslot, data_len, data_usi, p_data);
   }

   return ret;
}

int pf_fspm_aplmi_alarm_cnf(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_pnio_status_t      *p_pnio_status)
{
   int ret = 0;

   if (net->fspm_cfg.alarm_cnf_cb != NULL)
   {
      ret = net->fspm_cfg.alarm_cnf_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, p_pnio_status);
   }

   return ret;
}

int pf_fspm_aplmr_alarm_ack_cnf(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   int                     res)
{
   int ret = 0;

   if (net->fspm_cfg.alarm_ack_cnf_cb != NULL)
   {
      ret = net->fspm_cfg.alarm_ack_cnf_cb(net, net->fspm_cfg.cb_arg, p_ar->arep, res);
   }

   return ret;
}

int pf_fspm_reset_ind(
   pnet_t                  *net,
   bool                    should_reset_application,
   uint16_t                reset_mode)
{
   int ret = 0;

   if (net->fspm_cfg.reset_cb != NULL)
   {
      ret = net->fspm_cfg.reset_cb(net, net->fspm_cfg.cb_arg, should_reset_application, reset_mode);
   }

   return ret;
}
