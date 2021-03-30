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

#include <string.h>
#include "pf_includes.h"
#include "pf_block_writer.h"

/**
 * @file
 * @brief Implements the Context Management Read Record Responder protocol
 * machine device (CMRDR)
 *
 * Contains mainly one function \a pf_cmrdr_rm_read_ind(),
 * that handles a RPC parameter read request (and read implicit request).
 *
 * Triggers the \a pnet_read_ind() user callback for some values.
 *
 * This implementation of CMRDR has no internal state.
 * Every call to \a pf_cmrdr_rm_read_ind() finishes by returning the result.
 * Since there are no internal static variables there is also no need
 * for a POWER-ON state.
 */

int pf_cmrdr_rm_read_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_read_request_t * p_read_request,
   pnet_result_t * p_read_status,
   uint16_t res_size,
   uint8_t * p_res,
   uint16_t * p_pos)
{
   int ret = -1;
   pf_iod_read_result_t read_result;
   uint8_t * p_data = NULL;
   uint16_t data_length_pos = 0;
   uint16_t start_pos = 0;
   uint8_t iocs[255];                          /* Max possible array size */
   uint8_t iops[255];                          /* Max possible array size */
   uint8_t subslot_data[PF_FRAME_BUFFER_SIZE]; /* Max possible array size */
   uint8_t iocs_len = 0;
   uint8_t iops_len = 0;
   uint16_t data_len = 0;
   bool new_flag = false;
   char station_name[PNET_STATION_NAME_MAX_SIZE]; /* Terminated*/

   if (
      (p_read_request->slot_number == PNET_SLOT_DAP_IDENT) &&
      (pf_port_subslot_is_dap_port_id (net, p_read_request->subslot_number)))
   {
      LOG_INFO (
         PNET_LOG,
         "CMRDR(%d): PLC is reading slot %u subslot 0x%04X index 0x%04X "
         "(local port %d) \"%s\"\n",
         __LINE__,
         p_read_request->slot_number,
         p_read_request->subslot_number,
         p_read_request->index,
         pf_port_dap_subslot_to_local_port (net, p_read_request->subslot_number),
         pf_index_to_logstring (p_read_request->index));
   }
   else
   {
      LOG_INFO (
         PNET_LOG,
         "CMRDR(%d): PLC is reading slot %u subslot 0x%04X index 0x%04X "
         "\"%s\"\n",
         __LINE__,
         p_read_request->slot_number,
         p_read_request->subslot_number,
         p_read_request->index,
         pf_index_to_logstring (p_read_request->index));
   }

   read_result.sequence_number = p_read_request->sequence_number;
   read_result.ar_uuid = p_read_request->ar_uuid;
   read_result.api = p_read_request->api;
   read_result.slot_number = p_read_request->slot_number;
   read_result.subslot_number = p_read_request->subslot_number;
   read_result.index = p_read_request->index;

   /* Get the data from FSPM or the application, if possible. */
   data_len = res_size - *p_pos;
   if (
      pf_fspm_cm_read_ind (
         net,
         p_ar,
         p_read_request,
         &p_data,
         &data_len,
         p_read_status) != 0)
   {
      data_len = 0; /* Handled: No data available. */
   }

   /*
    * Just insert a dummy value for now.
    * It will be properly computed after the answer has been written to the
    * buffer.
    */
   read_result.record_data_length = 0;

   /*
    * The only result that can fail is from FSPM or the application.
    * All well-known indices are handled without error.
    * It is therefore OK to write the result already now.
    * This is needed in order to get the correct position for the actual answer.
    */
   read_result.add_data_1 = p_read_status->add_data_1;
   read_result.add_data_2 = p_read_status->add_data_2;

   /* Also: Retrieve a position to write the total record length to. */
   pf_put_read_result (
      true,
      &read_result,
      res_size,
      p_res,
      p_pos,
      &data_length_pos);

   /*
    * Even if the application or FSPM provided an answer.
    * If there is another answer then use that!!
    */
   start_pos = *p_pos;
   if (p_read_request->index <= PF_IDX_USER_MAX)
   {
      /* Provided by application - accept whatever it says. */
      if (*p_pos + data_len <= res_size)
      {
         memcpy (&p_res[*p_pos], p_data, data_len);
         *p_pos += data_len;
         ret = 0;
      }
   }
   else
   {
      switch (p_read_request->index)
      {
      case PF_IDX_DEV_IM_0_FILTER_DATA:
         /* Block-writer knows where to fetch and how to build the answer. */
         pf_put_im_0_filter_data (net, true, res_size, p_res, p_pos);
         ret = 0;
         break;

      /* I&M data has been provided by FSPM. Accept whatever it says.
       * Put it into the response after verifying the data length. */
      case PF_IDX_SUB_IM_0:
         if ((data_len == sizeof (pnet_im_0_t)) && (*p_pos + data_len < res_size))
         {
            pf_put_im_0 (true, (pnet_im_0_t *)p_data, res_size, p_res, p_pos);
            ret = 0;
         }
         break;
      case PF_IDX_SUB_IM_1:
         if ((data_len == sizeof (pnet_im_1_t)) && (*p_pos + data_len < res_size))
         {
            os_mutex_lock (net->fspm_im_mutex);
            pf_put_im_1 (true, (pnet_im_1_t *)p_data, res_size, p_res, p_pos);
            os_mutex_unlock (net->fspm_im_mutex);
            ret = 0;
         }
         break;
      case PF_IDX_SUB_IM_2:
         if ((data_len == sizeof (pnet_im_2_t)) && (*p_pos + data_len < res_size))
         {
            os_mutex_lock (net->fspm_im_mutex);
            pf_put_im_2 (true, (pnet_im_2_t *)p_data, res_size, p_res, p_pos);
            os_mutex_unlock (net->fspm_im_mutex);
            ret = 0;
         }
         break;
      case PF_IDX_SUB_IM_3:
         if ((data_len == sizeof (pnet_im_3_t)) && (*p_pos + data_len < res_size))
         {
            os_mutex_lock (net->fspm_im_mutex);
            pf_put_im_3 (true, (pnet_im_3_t *)p_data, res_size, p_res, p_pos);
            os_mutex_unlock (net->fspm_im_mutex);
            ret = 0;
         }
         break;
      case PF_IDX_SUB_IM_4:
         if ((data_len == sizeof (pnet_im_4_t)) && (*p_pos + data_len < res_size))
         {
            os_mutex_lock (net->fspm_im_mutex);
            pf_put_record_data_read (
               true,
               PF_BT_IM_4,
               data_len,
               p_data,
               res_size,
               p_res,
               p_pos);
            os_mutex_unlock (net->fspm_im_mutex);
            ret = 0;
         }
         break;

      /* Logbook data has been provided by FSPM. Accept whatever it says.
       * Put it into the response after verifying the data length. */
      case PF_IDX_DEV_LOGBOOK_DATA:
         pf_put_log_book_data (
            true,
            (pf_log_book_t *)p_data,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;

      /* Block-writer knows where to fetch and how to build the answer to ID
       * data requests. */
      case PF_IDX_SUB_EXP_ID_DATA:
         pf_put_ident_data (
            net,
            true,
            PNET_BLOCK_VERSION_LOW_1,
            PF_BT_EXPECTED_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            p_ar,
            p_read_request->api,
            p_read_request->slot_number,
            p_read_request->subslot_number,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_SUB_REAL_ID_DATA:
         pf_put_ident_data (
            net,
            true,
            PNET_BLOCK_VERSION_LOW_1,
            PF_BT_REAL_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            p_read_request->subslot_number,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_SLOT_EXP_ID_DATA:
         pf_put_ident_data (
            net,
            true,
            PNET_BLOCK_VERSION_LOW_1,
            PF_BT_EXPECTED_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_SLOT,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            p_ar,
            p_read_request->api,
            p_read_request->slot_number,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_SLOT_REAL_ID_DATA:
         pf_put_ident_data (
            net,
            true,
            PNET_BLOCK_VERSION_LOW_1,
            PF_BT_REAL_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_SLOT,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_EXP_ID_DATA:
         pf_put_ident_data (
            net,
            true,
            PNET_BLOCK_VERSION_LOW_1,
            PF_BT_EXPECTED_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_DEVICE,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            p_ar,
            0,
            0,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_REAL_ID_DATA:
         pf_put_ident_data (
            net,
            true,
            PNET_BLOCK_VERSION_LOW_1,
            PF_BT_REAL_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_DEVICE,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            p_ar,
            0,
            0,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_API_REAL_ID_DATA:
         pf_put_ident_data (
            net,
            true,
            PNET_BLOCK_VERSION_LOW_1,
            PF_BT_REAL_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_API,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            NULL,
            p_read_request->api,
            0,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;

      case PF_IDX_DEV_API_DATA:
         pf_put_ident_data (
            net,
            true,
            PNET_BLOCK_VERSION_LOW,
            PF_BT_API_DATA,
            PF_DEV_FILTER_LEVEL_DEVICE,
            PF_DEV_FILTER_LEVEL_API_ID,
            NULL,
            0,
            0,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;

      case PF_IDX_SUB_INPUT_DATA:
         /* Sub-module data to the controller */
         data_len = sizeof (subslot_data);
         iops_len = sizeof (iops);
         iocs_len = sizeof (iocs);
         if (
            pf_ppm_get_data_and_iops (
               net,
               p_read_request->api,
               p_read_request->slot_number,
               p_read_request->subslot_number,
               subslot_data,
               &data_len,
               iops,
               &iops_len) != 0)
         {
            LOG_DEBUG (
               PNET_LOG,
               "CMRDR(%d): Could not get PPM data and IOPS\n",
               __LINE__);
            data_len = 0;
            iops_len = 0;
         }
         if (
            pf_cpm_get_iocs (
               net,
               p_read_request->api,
               p_read_request->slot_number,
               p_read_request->subslot_number,
               iocs,
               &iocs_len) != 0)
         {
            LOG_DEBUG (
               PNET_LOG,
               "CMRDR(%d): Could not get CPM IOCS\n",
               __LINE__);
            iocs_len = 0;
         }
         if ((data_len + iops_len + iocs_len) > 0)
         {
            /* Have all data */
            pf_put_input_data (
               true,
               iocs_len,
               iocs,
               iops_len,
               iops,
               data_len,
               subslot_data,
               res_size,
               p_res,
               p_pos);
            ret = 0;
         }
         break;
      case PF_IDX_SUB_OUTPUT_DATA:
         /* Sub-module data from the controller. */
         data_len = sizeof (subslot_data);
         iops_len = sizeof (iops);
         iocs_len = sizeof (iocs);
         if (
            pf_cpm_get_data_and_iops (
               net,
               p_read_request->api,
               p_read_request->slot_number,
               p_read_request->subslot_number,
               &new_flag,
               subslot_data,
               &data_len,
               iops,
               &iops_len) != 0)
         {
            LOG_DEBUG (
               PNET_LOG,
               "CMRDR(%d): Could not get CPM data and IOPS\n",
               __LINE__);
            data_len = 0;
            iops_len = 0;
         }
         if (
            pf_ppm_get_iocs (
               net,
               p_read_request->api,
               p_read_request->slot_number,
               p_read_request->subslot_number,
               iocs,
               &iocs_len) != 0)
         {
            LOG_DEBUG (
               PNET_LOG,
               "CMRDR(%d): Could not get PPM IOCS\n",
               __LINE__);
            iocs_len = 0;
         }
         if ((data_len + iops_len + iocs_len) > 0)
         {
            /* Have all data */
            /* ToDo: Support substitution values, mode and active flag */
            pf_put_output_data (
               true,
               false,
               iocs_len,
               iocs,
               iops_len,
               iops,
               data_len,
               subslot_data,
               0,
               subslot_data,
               res_size,
               p_res,
               p_pos);
            ret = 0;
         }
         break;

      /* Block-writer knows where to fetch and how to build the answer to diag
       * data requests. */
      case PF_IDX_SUB_DIAGNOSIS_CH:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            PF_DIAG_FILTER_FAULT_STD,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            p_read_request->subslot_number,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_SUB_DIAGNOSIS_ALL:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            PF_DIAG_FILTER_FAULT_ALL,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            p_read_request->subslot_number,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_SUB_DIAGNOSIS_DMQS:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            PF_DIAG_FILTER_ALL,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            p_read_request->subslot_number,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_SUB_DIAG_MAINT_REQ_CH:
      case PF_IDX_SUB_DIAG_MAINT_REQ_ALL:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            PF_DIAG_FILTER_M_REQ,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            p_read_request->subslot_number,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_SUB_DIAG_MAINT_DEM_CH:
      case PF_IDX_SUB_DIAG_MAINT_DEM_ALL:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_SUBSLOT,
            PF_DIAG_FILTER_M_DEM,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            p_read_request->subslot_number,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;

      case PF_IDX_SLOT_DIAGNOSIS_CH:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_SLOT,
            PF_DIAG_FILTER_FAULT_STD,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_SLOT_DIAGNOSIS_ALL:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_SLOT,
            PF_DIAG_FILTER_FAULT_ALL,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_SLOT_DIAGNOSIS_DMQS:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_SLOT,
            PF_DIAG_FILTER_ALL,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_SLOT_DIAG_MAINT_REQ_CH:
      case PF_IDX_SLOT_DIAG_MAINT_REQ_ALL:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_SLOT,
            PF_DIAG_FILTER_M_REQ,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_SLOT_DIAG_MAINT_DEM_CH:
      case PF_IDX_SLOT_DIAG_MAINT_DEM_ALL:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_SLOT,
            PF_DIAG_FILTER_M_DEM,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;

      case PF_IDX_API_DIAGNOSIS_CH:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_API,
            PF_DIAG_FILTER_FAULT_STD,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_API_DIAGNOSIS_ALL:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_API,
            PF_DIAG_FILTER_FAULT_ALL,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_API_DIAGNOSIS_DMQS:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_API,
            PF_DIAG_FILTER_ALL,
            NULL,
            p_read_request->api,
            0,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_API_DIAG_MAINT_REQ_CH:
      case PF_IDX_API_DIAG_MAINT_REQ_ALL:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_API,
            PF_DIAG_FILTER_M_REQ,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_API_DIAG_MAINT_DEM_CH:
      case PF_IDX_API_DIAG_MAINT_DEM_ALL:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_API,
            PF_DIAG_FILTER_M_DEM,
            NULL,
            p_read_request->api,
            p_read_request->slot_number,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;

      case PF_IDX_AR_DIAGNOSIS_CH:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_DEVICE,
            PF_DIAG_FILTER_FAULT_STD,
            p_ar,
            0,
            0,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_DIAGNOSIS_ALL:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_DEVICE,
            PF_DIAG_FILTER_FAULT_ALL,
            p_ar,
            0,
            0,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_DIAGNOSIS_DMQS:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_DEVICE,
            PF_DIAG_FILTER_ALL,
            p_ar,
            0,
            0,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_DIAG_MAINT_REQ_CH:
      case PF_IDX_AR_DIAG_MAINT_REQ_ALL:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_DEVICE,
            PF_DIAG_FILTER_M_REQ,
            p_ar,
            0,
            0,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_DIAG_MAINT_DEM_CH:
      case PF_IDX_AR_DIAG_MAINT_DEM_ALL:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_DEVICE,
            PF_DIAG_FILTER_M_DEM,
            p_ar,
            0,
            0,
            0,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;

      case PF_IDX_DEV_DIAGNOSIS_DMQS:
         pf_put_diag_data (
            net,
            true,
            PF_DEV_FILTER_LEVEL_DEVICE,
            PF_DIAG_FILTER_ALL,
            NULL, /* Do not filter by AR */
            0,    /* API */
            0,    /* Slot */
            0,    /* Subslot */
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;

      case PF_IDX_DEV_CONN_MON_TRIGGER:
         /* Restart timeout period. See pf_cmsm_rm_read_ind() */
         ret = 0;
         break;

      case PF_IDX_API_AR_DATA:
         pf_put_ar_data (
            net,
            true,
            p_ar,
            p_read_request->api,
            res_size,
            p_res,
            p_pos);
         ret = 0;
         break;
      case PF_IDX_DEV_AR_DATA:
         pf_put_ar_data (net, true, NULL, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_MOD_DIFF:
         /* On implicit with p_ar == NULL pf_cmdev_generate_submodule_diff
          * will return -1
          * In this case pf_put_ar_diff will not give an empty response
          */
         (void)pf_cmdev_generate_submodule_diff (net, p_ar);
         pf_put_ar_diff (true, p_ar, res_size, p_res, p_pos);
         ret = 0;
         break;

      case PF_IDX_SUB_PDPORT_DATA_REAL:
      case PF_IDX_SUB_PDPORT_DATA_ADJ:
      case PF_IDX_SUB_PDPORT_DATA_CHECK:
      case PF_IDX_DEV_PDREAL_DATA:
      case PF_IDX_DEV_PDEXP_DATA:
      case PF_IDX_SUB_PDPORT_STATISTIC:
      case PF_IDX_SUB_PDINTF_ADJUST:
         ret = pf_pdport_read_ind (
            net,
            p_ar,
            p_read_request,
            &read_result,
            res_size,
            p_res,
            p_pos);
         break;

      case PF_IDX_SUB_PDINTF_REAL:
         /* Only check if this is the port subslot */
         if (
            (p_read_request->slot_number == PNET_SLOT_DAP_IDENT) &&
            (p_read_request->subslot_number ==
             PNET_SUBSLOT_DAP_INTERFACE_1_IDENT))
         {
            pf_cmina_get_station_name (net, station_name);

            pf_put_pdinterface_data_real (
               true,
               pf_cmina_get_device_macaddr (net),
               pf_cmina_get_ipaddr (net),
               pf_cmina_get_netmask (net),
               pf_cmina_get_gateway (net),
               station_name,
               res_size,
               p_res,
               p_pos);
            ret = 0;
         }
         else
         {
            ret = -1;
         }
         break;
      default:
         LOG_INFO (
            PNET_LOG,
            "CMRDR(%d): Index 0x%04X is not supported.\n",
            __LINE__,
            p_read_request->index);
         ret = -1;
         break;
      }
   }

   if (ret != 0)
   {
      LOG_INFO (
         PNET_LOG,
         "CMRDR(%d): Could not read index 0x%04X for slot %u subslot 0x%04X.\n",
         __LINE__,
         p_read_request->index,
         p_read_request->slot_number,
         p_read_request->subslot_number);

      p_read_status->pnio_status.error_code = PNET_ERROR_CODE_READ;
      p_read_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
      p_read_status->pnio_status.error_code_1 =
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX;
      p_read_status->pnio_status.error_code_2 = 10;
      ret = 0;
   }

   read_result.record_data_length = *p_pos - start_pos;
   pf_put_uint32 (
      true,
      read_result.record_data_length,
      res_size,
      p_res,
      &data_length_pos); /* Insert actual data length */

   /* Restart timer */
   if (pf_cmsm_cm_read_ind (net, p_ar, p_read_request) != 0)
   {
      ret = -1;
   }

   return ret;
}

const char * pf_index_to_logstring (uint16_t index)
{
   if (index <= PF_IDX_USER_MAX)
   {
      return "user defined";
   }

   switch (index)
   {
   case PF_IDX_SUB_EXP_ID_DATA:
      return "ExpectedIdentificationData for one subslot";
   case PF_IDX_SUB_REAL_ID_DATA:
      return "RealIdentificationData for one subslot";
   case PF_IDX_SUB_DIAGNOSIS_CH:
      return "Diagnosis in channel coding for one subslot";
   case PF_IDX_SUB_DIAGNOSIS_ALL:
      return "Diagnosis in all codings for one subslot";
   case PF_IDX_SUB_DIAGNOSIS_DMQS:
      return "Diagnosis, Maintenance, Qualified and Status for one subslot";
   case PF_IDX_SUB_DIAG_MAINT_REQ_CH:
      return "Maintenance required in channel coding for one subslot";
   case PF_IDX_SUB_DIAG_MAINT_DEM_CH:
      return "Maintenance demanded in channel coding for one subslot";
   case PF_IDX_SUB_DIAG_MAINT_REQ_ALL:
      return "Maintenance required in all codings for one subslot";
   case PF_IDX_SUB_DIAG_MAINT_DEM_ALL:
      return "Maintenance demanded in all codings for one subslot";
   case PF_IDX_SUB_SUBSTITUTE:
      return "SubstituteValue for one subslot";
   case PF_IDX_SUB_PDIR:
      return "PDIRSubframeData for one subslot";
   case PF_IDX_SUB_PDPORTDATAREALEXT:
      return "PDPortDataRealExtended for one subslot";
   case PF_IDX_SUB_INPUT_DATA:
      return "inputdata for one subslot";
   case PF_IDX_SUB_OUTPUT_DATA:
      return "outputdata for one subslot";
   case PF_IDX_SUB_PDPORT_DATA_REAL:
      return "PDPortDataReal for one subslot";
   case PF_IDX_SUB_PDPORT_DATA_CHECK:
      return "PDPortDataCheck for one subslot";
   case PF_IDX_SUB_PDIR_DATA:
      return "PDIRData for one subslot";
   case PF_IDX_SUB_PDSYNC_ID0:
      return "PDSyncData for one subslot SyncID=0";
   case PF_IDX_SUB_PDPORT_DATA_ADJ:
      return "PDPortDataAdjust for one subslot";
   case PF_IDX_SUB_ISOCHRONUOUS_DATA:
      return "IsochronousModeData for one subslot";
   case PF_IDX_SUB_PDTIME:
      return "PDTimeData for one subslot";
   case PF_IDX_SUB_PDINTF_MRP_REAL:
      return "PDInterfaceMrpDataReal for one subslot";
   case PF_IDX_SUB_PDINTF_MRP_CHECK:
      return "PDInterfaceMrpDataCheck for one subslot";
   case PF_IDX_SUB_PDINTF_MRP_ADJUST:
      return "PDInterfaceMrpDataAdjust for one subslot";
   case PF_IDX_SUB_PDPORT_MRP_ADJUST:
      return "PDPortMrpDataAdjust for one subslot";
   case PF_IDX_SUB_PDPORT_MRP_REAL:
      return "PDPortMrpDataReal for one subslot";
   case PF_IDX_SUB_PDPORT_MRPIC_REAL:
      return "PDPortMrpIcDataAdjust for one subslot";
   case PF_IDX_SUB_PDPORT_MRPIC_CHECK:
      return "PDPortMrpIcDataCheck for one subslot";
   case PF_IDX_SUB_PDPORT_MRPIC_ADJUST:
      return "PDPortMrpIcDataReal for one subslot";
   case PF_IDX_SUB_PDPORT_FO_REAL:
      return "PDPortFODataReal for one subslot";
   case PF_IDX_SUB_PDPORT_FO_CHECK:
      return "PDPortFODataCheck for one subslot";
   case PF_IDX_SUB_PDPORT_FO_ADJUST:
      return "PDPortFODataAdjust for one subslot";
   case PF_IDX_SUB_PDPORT_SPF_CHECK:
      return "PDPortSFPDataCheck for one subslot";
   case PF_IDX_SUB_PDNC_CHECK:
      return "PDNCDataCheck for one subslot";
   case PF_IDX_SUB_PDINTF_ADJUST:
      return "PDInterfaceAdjust for one subslot";
   case PF_IDX_SUB_PDPORT_STATISTIC:
      return "PDPortStatistic for one subslot";
   case PF_IDX_SUB_PDINTF_REAL:
      return "PDInterfaceDataReal for one subslot";
   case PF_IDX_SUB_PDINTF_FSU_ADJUST:
      return "PDInterfaceFSUDataAdjust";
   case PF_IDX_SUB_PE_ENTITY_STATUS:
      return "PE_EntityStatusData for one subslot";
   case PF_IDX_SUB_COMBINED_OBJ_CONTAINER:
      return "CombinedObjectContainer";
   case PF_IDX_SUB_RS_ADJUST_OBSERVER:
      return "RS_AdjustObserver";
   case PF_IDX_TSN_NETWORK_CONTROL_DATA_REAL:
      return "TSNNetworkControlDataReal";
   case PF_IDX_TSN_STREAM_PATH_DATA:
      return "TSNStreamPathData";
   case PF_IDX_TSN_SYNC_TREE_DATA:
      return "TSNSyncTreeData";
   case PF_IDX_TSN_UPLOAD_NETWORK_ATTRIBUTES:
      return "TSNUploadNetworkAttributes";
   case PF_IDX_TSN_EXPECTED_NETWORK_ATTRIBUTES:
      return "TSNExpectedNetworkAttributes";
   case PF_IDX_TSN_NETWORK_CONTROL_DATA_ADJUST:
      return "TSNNetworkControlDataAdjust";
   case PF_IDX_SUB_PDINTF_SECURITY_ADJUST:
      return "PDInterfaceSecurityAdjust for one subslot";
   case PF_IDX_SUB_IM_0:
      return "I&M0";
   case PF_IDX_SUB_IM_1:
      return "I&M1";
   case PF_IDX_SUB_IM_2:
      return "I&M2";
   case PF_IDX_SUB_IM_3:
      return "I&M3";
   case PF_IDX_SUB_IM_4:
      return "I&M4";
   case PF_IDX_SUB_IM_5:
      return "I&M5";
   case PF_IDX_SUB_IM_6:
      return "I&M6";
   case PF_IDX_SUB_IM_7:
      return "I&M7";
   case PF_IDX_SUB_IM_8:
      return "I&M8";
   case PF_IDX_SUB_IM_9:
      return "I&M9";
   case PF_IDX_SUB_IM_10:
      return "I&M10";
   case PF_IDX_SUB_IM_11:
      return "I&M11";
   case PF_IDX_SUB_IM_12:
      return "I&M12";
   case PF_IDX_SUB_IM_13:
      return "I&M13";
   case PF_IDX_SUB_IM_14:
      return "I&M14";
   case PF_IDX_SUB_IM_15:
      return "I&M15";
   case PF_IDX_SLOT_EXP_ID_DATA:
      return "ExpectedIdentificationData for one slot";
   case PF_IDX_SLOT_REAL_ID_DATA:
      return "RealIdentificationData for one slot";
   case PF_IDX_SLOT_DIAGNOSIS_CH:
      return "Diagnosis in channel coding for one slot";
   case PF_IDX_SLOT_DIAGNOSIS_ALL:
      return "Diagnosis in all codings for one slot";
   case PF_IDX_SLOT_DIAGNOSIS_DMQS:
      return "Diagnosis, Maintenance, Qualified and Status for one slot";
   case PF_IDX_SLOT_DIAG_MAINT_REQ_CH:
      return "Maintenance required in channel coding for one slot";
   case PF_IDX_SLOT_DIAG_MAINT_DEM_CH:
      return "Maintenance demanded in channel coding for one slot";
   case PF_IDX_SLOT_DIAG_MAINT_REQ_ALL:
      return "Maintenance required in all codings for one slot";
   case PF_IDX_SLOT_DIAG_MAINT_DEM_ALL:
      return "Maintenance demanded in all codings for one slot";
   case PF_IDX_AR_EXP_ID_DATA:
      return "ExpectedIdentificationData for one AR";
   case PF_IDX_AR_REAL_ID_DATA:
      return "RealIdentificationData for one AR";
   case PF_IDX_AR_MOD_DIFF:
      return "ModuleDiffBlock for one AR";
   case PF_IDX_AR_DIAGNOSIS_CH:
      return "Diagnosis in channel coding for one AR";
   case PF_IDX_AR_DIAGNOSIS_ALL:
      return "Diagnosis in all codings for one AR";
   case PF_IDX_AR_DIAGNOSIS_DMQS:
      return "Diagnosis, Maintenance, Qualified and Status for one AR";
   case PF_IDX_AR_DIAG_MAINT_REQ_CH:
      return "Maintenance required in channel coding for one AR";
   case PF_IDX_AR_DIAG_MAINT_DEM_CH:
      return "Maintenance demanded in channel coding for one AR";
   case PF_IDX_AR_DIAG_MAINT_REQ_ALL:
      return "Maintenance required in all codings for one AR";
   case PF_IDX_AR_DIAG_MAINT_DEM_ALL:
      return "Maintenance demanded in all codings for one AR";
   case PF_IDX_AR_PE_ENTITY_FILTER_DATA:
      return "PE_EntityFilterData for one AR";
   case PF_IDX_AR_PE_ENTITY_STATUS_DATA:
      return "PE_EntityStatusData for one AR";
   case PF_IDX_AR_WRITE_MULTIPLE:
      return "WriteMultiple";
   case PF_IDX_APPLICATION_READY:
      return "ApplicationReadyBlock, closes parameterization";
   case PF_IDX_AR_FSU_DATA_ADJUST:
      return "ARFSUDataAdjust data for one AR";
   case PF_IDX_AR_RS_GET_EVENT:
      return "RS_GetEvent";
   case PF_IDX_AR_RS_ACK_EVENT:
      return "RS_AckEvent";
   case PF_IDX_API_REAL_ID_DATA:
      return "RealIdentificationData for one API";
   case PF_IDX_API_DIAGNOSIS_CH:
      return "Diagnosis in channel coding for one API";
   case PF_IDX_API_DIAGNOSIS_ALL:
      return "Diagnosis in all codings for one API";
   case PF_IDX_API_DIAGNOSIS_DMQS:
      return "Diagnosis, Maintenance, Qualified and Status for one API";
   case PF_IDX_API_DIAG_MAINT_REQ_CH:
      return "Maintenance required in channel coding for one API";
   case PF_IDX_API_DIAG_MAINT_DEM_CH:
      return "Maintenance demanded in channel coding for one API";
   case PF_IDX_API_DIAG_MAINT_REQ_ALL:
      return "Maintenance required in all codings for one API";
   case PF_IDX_API_DIAG_MAINT_DEM_ALL:
      return "Maintenance demanded in all codings for one API";
   case PF_IDX_API_AR_DATA:
      return "ARData for one API";
   case PF_IDX_DEV_DIAGNOSIS_DMQS:
      return "Diagnosis, Maintenance, Qualified and Status for one device";
   case PF_IDX_DEV_AR_DATA:
      return "ARData";
   case PF_IDX_DEV_API_DATA:
      return "APIData";
   case PF_IDX_DEV_LOGBOOK_DATA:
      return "LogBookData";
   case PF_IDX_DEV_PDEV_DATA:
      return "PdevData";
   case PF_IDX_DEV_IM_0_FILTER_DATA:
      return "I&M0FilterData";
   case PF_IDX_DEV_PDREAL_DATA:
      return "PDRealData";
   case PF_IDX_DEV_PDEXP_DATA:
      return "PDExpectedData";
   case PF_IDX_DEV_AUTO_CONFIGURATION:
      return "AutoConfiguration";
   case PF_IDX_DEV_GSD_UPLOAD:
      return "GSD upload";
   case PF_IDX_DEV_PE_ENTITY_FILTER_DATA:
      return "PE_EntityFilterData";
   case PF_IDX_DEV_PE_ENTITY_STATUS_DATA:
      return "PE_EntityStatusData";
   case PF_IDX_DEV_ASSET_MANAGEMENT_1:
      return "AssetManagementData1";
   case PF_IDX_DEV_ASSET_MANAGEMENT_2:
      return "AssetManagementData2";
   case PF_IDX_DEV_ASSET_MANAGEMENT_3:
      return "AssetManagementData3";
   case PF_IDX_DEV_ASSET_MANAGEMENT_4:
      return "AssetManagementData4";
   case PF_IDX_DEV_ASSET_MANAGEMENT_5:
      return "AssetManagementData5";
   case PF_IDX_DEV_ASSET_MANAGEMENT_6:
      return "AssetManagementData6";
   case PF_IDX_DEV_ASSET_MANAGEMENT_7:
      return "AssetManagementData7";
   case PF_IDX_DEV_ASSET_MANAGEMENT_8:
      return "AssetManagementData8";
   case PF_IDX_DEV_ASSET_MANAGEMENT_9:
      return "AssetManagementData9";
   case PF_IDX_DEV_ASSET_MANAGEMENT_10:
      return "AssetManagementData10";
   case PF_IDX_TSN_STREAM_ADD:
      return "TSN stream add";
   case PF_IDX_PDRSI_INSTANCDES:
      return "PDRsiInstances";
   case PF_IDX_TSN_STREAM_REMOVE:
      return "TSN stream remove";
   case PF_IDX_TSN_STREAM_RENEW:
      return "TSN stream renew";
   case PF_IDX_DEV_CONN_MON_TRIGGER:
      return "nothing, but is restarting the timeout period";
   default:
      return "unknown";
   }
}
