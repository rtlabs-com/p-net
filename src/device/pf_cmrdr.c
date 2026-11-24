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

static inline void pf_cmrdr_init_read_result (
   pf_iod_read_result_t * result,
   pf_iod_read_request_t const * request)
{
   result->sequence_number = request->sequence_number;
   result->ar_uuid = request->ar_uuid;
   result->api = request->api;
   result->slot_number = request->slot_number;
   result->subslot_number = request->subslot_number;
   result->index = request->index;
   result->record_data_length = 0;
   result->add_data_1 = 0;
   result->add_data_2 = 0;
}

static void pf_cmrdr_read_user_index (
   pnet_t * net,
   pf_ar_t * ar,
   pf_iod_read_request_t const * request,
   pf_put_info_t * info,
   uint16_t * pos,
   pnet_result_t * result)
{
   pf_iod_read_result_t read_result;
   uint8_t * data;
   uint16_t size;
   uint16_t data_offset;
   uint16_t length_offset;
   int r;

   size = (info->len - *pos);
   if (request->record_data_length < size)
   {
      size = request->record_data_length;
   }
   r = pf_fspm_cm_read_ind (net, ar, request, &data, &size, result);

   /* Encode read result, with length set to 0, for now. */
   pf_cmrdr_init_read_result (&read_result, request);
   read_result.add_data_1 = result->add_data_1;
   read_result.add_data_2 = result->add_data_2;
   pf_put_read_result (true, &read_result, info, pos, &length_offset);

   /* If data was provided, copy data and update length. */
   if ((r == 0) && (size > 0) && (data != NULL))
   {
      data_offset = *pos;
      pf_put_mem (data, size, info, pos);
      if (info->result == PF_PUT_OK)
      {
         /* Insert actual length. */
         pf_put_uint32 (true, (*pos - data_offset), info, &length_offset);
      }
   }
}

static void pf_cmrdr_read_im_index (
   pnet_t * net,
   pf_iod_read_request_t const * request,
   pf_put_info_t * info,
   uint16_t * pos,
   pnet_result_t * result)
{
   pf_iod_read_result_t read_result;
   uint16_t data_offset;
   uint16_t length_offset;
   int r;
   union
   {
      pnet_im_0_t im0;
      pnet_im_1_t im1;
      pnet_im_2_t im2;
      pnet_im_3_t im3;
      pnet_im_4_t im4;
   } data;

   switch (request->index)
   {
   case PF_IDX_SUB_IM_0:
      r = pf_fspm_read_im_0 (net, request, &data.im0, result);
      break;
   case PF_IDX_SUB_IM_1:
      r = pf_fspm_read_im_1 (net, request, &data.im1, result);
      break;
   case PF_IDX_SUB_IM_2:
      r = pf_fspm_read_im_2 (net, request, &data.im2, result);
      break;
   case PF_IDX_SUB_IM_3:
      r = pf_fspm_read_im_3 (net, request, &data.im3, result);
      break;
   case PF_IDX_SUB_IM_4:
      r = pf_fspm_read_im_4 (net, request, &data.im4, result);
      break;
   default:
      pf_set_error (
         result,
         PNET_ERROR_CODE_READ,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      r = -1;
      break;
   }

   /* Encode read result, with length set to 0, for now. */
   pf_cmrdr_init_read_result (&read_result, request);
   read_result.add_data_1 = result->add_data_1;
   read_result.add_data_2 = result->add_data_2;
   pf_put_read_result (true, &read_result, info, pos, &length_offset);

   /* If data was provided, copy data and update length. */
   if (r == 0)
   {
      data_offset = *pos;
      switch (request->index)
      {
      case PF_IDX_SUB_IM_0:
         pf_put_im_0 (true, &data.im0, info, pos);
         break;
      case PF_IDX_SUB_IM_1:
         if (
            (strlen (data.im1.im_tag_function) > 0) &&
            (strlen (data.im1.im_tag_location) > 0))
         {
            pf_put_im_1 (true, &data.im1, info, pos);
         }
         break;
      case PF_IDX_SUB_IM_2:
         if (strlen (data.im2.im_date) > 0)
         {
            pf_put_im_2 (true, &data.im2, info, pos);
         }
         break;
      case PF_IDX_SUB_IM_3:
         if (strlen (data.im3.im_descriptor) > 0)
         {
            pf_put_im_3 (true, &data.im3, info, pos);
         }
         break;
      case PF_IDX_SUB_IM_4:
         pf_put_im_4 (true, &data.im4, info, pos);
         break;
      default:
         break;
      }
      if (info->result == PF_PUT_OK)
      {
         /* Insert actual length. */
         pf_put_uint32 (true, (*pos - data_offset), info, &length_offset);
      }
      else
      {
         *pos = data_offset;
      }
   }
}

static void pf_cmrdr_special_read (
   pnet_t * net,
   pf_ar_t * ar,
   pf_iod_read_request_t const * request,
   pf_put_info_t * info,
   uint16_t * pos,
   pnet_result_t * result)
{
   pf_iod_read_result_t read_result;
   uint16_t data_offset;
   uint16_t length_offset;

   /* Encode read result, with length set to 0, for now. */
   pf_cmrdr_init_read_result (&read_result, request);
   read_result.add_data_1 = result->add_data_1;
   read_result.add_data_2 = result->add_data_2;
   pf_put_read_result (true, &read_result, info, pos, &length_offset);
   data_offset = *pos;

   switch (request->index)
   {
   case PF_IDX_DEV_IM_0_FILTER_DATA:
      /* Block-writer knows where to fetch and how to build the answer. */
      pf_put_im_0_filter_data (net, true, info, pos);
      break;
#if PNET_OPTION_SNMP && PNET_OPTION_SECURITY_LEVEL_1
   case PF_IDX_CIM_SNMP_REAL:
      pf_put_snmp_cim_snmp_real (
         net,
         true,
         net->snmp_config.control_mode,
         info,
         pos);
      break;
   case PF_IDX_CIM_SNMP_ADJUST:
      if (net->snmp_config.snmp_adjust_active)
      {
         pf_put_snmp_cim_snmp_adjust (
            net,
            true,
            net->snmp_config.control_mode,
            info,
            pos);
      }
      break;
#endif /* PNET_OPTION_SNMP && PNET_OPTION_SECURITY_LEVEL_1 */

   case PF_IDX_DEV_LOGBOOK_DATA:
      pf_put_log_book_data (true, &net->fspm_log_book, info, pos);
      break;

   /* Block-writer knows where to fetch and how to build the answer to ID
    * data requests. */
   case PF_IDX_SUB_EXP_ID_DATA:
      pf_put_exp_ident_data (
         ar,
         true,
         PNET_BLOCK_VERSION_LOW_1,
         PF_RECORD_DATA_SCOPE_SUBSLOT,
         request->api,
         request->slot_number,
         request->subslot_number,
         info,
         pos);
      break;
   case PF_IDX_SUB_REAL_ID_DATA:
      pf_put_real_ident_data (
         net,
         true,
         PNET_BLOCK_VERSION_LOW_1,
         PF_RECORD_DATA_SCOPE_SUBSLOT,
         NULL,
         request->api,
         request->slot_number,
         request->subslot_number,
         info,
         pos);
      break;
   case PF_IDX_SLOT_EXP_ID_DATA:
      pf_put_exp_ident_data (
         ar,
         true,
         PNET_BLOCK_VERSION_LOW_1,
         PF_RECORD_DATA_SCOPE_SLOT,
         request->api,
         request->slot_number,
         0,
         info,
         pos);
      break;
   case PF_IDX_SLOT_REAL_ID_DATA:
      pf_put_real_ident_data (
         net,
         true,
         PNET_BLOCK_VERSION_LOW_1,
         PF_RECORD_DATA_SCOPE_SLOT,
         NULL,
         request->api,
         request->slot_number,
         0,
         info,
         pos);
      break;
   case PF_IDX_AR_EXP_ID_DATA:
      pf_put_exp_ident_data (
         ar,
         true,
         PNET_BLOCK_VERSION_LOW_1,
         PF_RECORD_DATA_SCOPE_AR,
         0,
         0,
         0,
         info,
         pos);
      break;
   case PF_IDX_AR_REAL_ID_DATA:
      pf_put_real_ident_data (
         net,
         true,
         PNET_BLOCK_VERSION_LOW_1,
         PF_RECORD_DATA_SCOPE_AR,
         ar,
         0,
         0,
         0,
         info,
         pos);
      break;
   case PF_IDX_API_REAL_ID_DATA:
      pf_put_real_ident_data (
         net,
         true,
         PNET_BLOCK_VERSION_LOW_1,
         PF_RECORD_DATA_SCOPE_API,
         NULL,
         request->api,
         0,
         0,
         info,
         pos);
      break;

   case PF_IDX_DEV_API_DATA:
      pf_put_real_ident_data (
         net,
         true,
         PNET_BLOCK_VERSION_LOW,
         PF_RECORD_DATA_SCOPE_DEVICE,
         NULL,
         0,
         0,
         0,
         info,
         pos);
      break;

   case PF_IDX_SUB_INPUT_DATA:
   {
      uint8_t subslot_data[PF_FRAME_BUFFER_SIZE]; /* Max possible array size */
      uint8_t iocs[255];                          /* Max possible array size */
      uint8_t iops[255];                          /* Max possible array size */
      uint8_t iocs_len = 0;
      uint8_t iops_len = 0;
      uint16_t data_len = 0;

      /* Sub-module data to the controller */
      data_len = sizeof (subslot_data);
      iops_len = sizeof (iops);
      iocs_len = sizeof (iocs);
      if (
         pf_ppm_get_data_and_iops (
            net,
            request->api,
            request->slot_number,
            request->subslot_number,
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
            request->api,
            request->slot_number,
            request->subslot_number,
            iocs,
            &iocs_len) != 0)
      {
         LOG_DEBUG (PNET_LOG, "CMRDR(%d): Could not get CPM IOCS\n", __LINE__);
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
            info,
            pos);
      }
      else
      {
         pf_set_error (
            result,
            PNET_ERROR_CODE_READ,
            PNET_ERROR_DECODE_PNIORW,
            PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
            PNET_ERROR_CODE_2_PNIORW (10));
      }
      break;
   }
   case PF_IDX_SUB_OUTPUT_DATA:
   {
      uint8_t subslot_data[PF_FRAME_BUFFER_SIZE]; /* Max possible array size */
      uint8_t iocs[255];                          /* Max possible array size */
      uint8_t iops[255];                          /* Max possible array size */
      uint8_t iocs_len = 0;
      uint8_t iops_len = 0;
      uint16_t data_len = 0;
      bool new_flag = false;

      /* Sub-module data from the controller. */
      data_len = sizeof (subslot_data);
      iops_len = sizeof (iops);
      iocs_len = sizeof (iocs);
      if (
         pf_cpm_get_data_and_iops (
            net,
            request->api,
            request->slot_number,
            request->subslot_number,
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
            request->api,
            request->slot_number,
            request->subslot_number,
            iocs,
            &iocs_len) != 0)
      {
         LOG_DEBUG (PNET_LOG, "CMRDR(%d): Could not get PPM IOCS\n", __LINE__);
         iocs_len = 0;
      }
      if ((data_len + iops_len + iocs_len) > 0)
      {
         bool substitute_active = false;
         int n = iops_len;

         while (!substitute_active && (n > 0))
         {
            --n;
            substitute_active = (iops[n] != PNET_IOXS_GOOD);
         }

         /* Have all data */
         /* ToDo: Support substitution values and mode */
         pf_put_output_data (
            true,
            substitute_active,
            iocs_len,
            iocs,
            iops_len,
            iops,
            data_len,
            subslot_data,
            0,
            subslot_data,
            info,
            pos);
      }
      else
      {
         pf_set_error (
            result,
            PNET_ERROR_CODE_READ,
            PNET_ERROR_DECODE_PNIORW,
            PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
            PNET_ERROR_CODE_2_PNIORW (10));
      }
      break;
   }
   case PF_IDX_SUB_DIAGNOSIS_CH:
      pf_put_diagnosis_subslot (
         net,
         true,
         PF_DIAG_FILTER_FAULT_STD,
         ar,
         request->api,
         request->slot_number,
         request->subslot_number,
         info,
         pos);
      break;
   case PF_IDX_SUB_DIAGNOSIS_ALL:
      pf_put_diagnosis_subslot (
         net,
         true,
         PF_DIAG_FILTER_FAULT_ALL,
         ar,
         request->api,
         request->slot_number,
         request->subslot_number,
         info,
         pos);
      break;
   case PF_IDX_SUB_DIAGNOSIS_DMQS:
      pf_put_diagnosis_subslot (
         net,
         true,
         PF_DIAG_FILTER_ALL,
         ar,
         request->api,
         request->slot_number,
         request->subslot_number,
         info,
         pos);
      break;
   case PF_IDX_SUB_DIAG_MAINT_REQ_CH:
   case PF_IDX_SUB_DIAG_MAINT_REQ_ALL:
      pf_put_diagnosis_subslot (
         net,
         true,
         PF_DIAG_FILTER_M_REQ,
         ar,
         request->api,
         request->slot_number,
         request->subslot_number,
         info,
         pos);
      break;
   case PF_IDX_SUB_DIAG_MAINT_DEM_CH:
   case PF_IDX_SUB_DIAG_MAINT_DEM_ALL:
      pf_put_diagnosis_subslot (
         net,
         true,
         PF_DIAG_FILTER_M_DEM,
         ar,
         request->api,
         request->slot_number,
         request->subslot_number,
         info,
         pos);
      break;

   case PF_IDX_SLOT_DIAGNOSIS_CH:
      pf_put_diagnosis_slot (
         net,
         true,
         PF_DIAG_FILTER_FAULT_STD,
         ar,
         request->api,
         request->slot_number,
         info,
         pos);
      break;
   case PF_IDX_SLOT_DIAGNOSIS_ALL:
      pf_put_diagnosis_slot (
         net,
         true,
         PF_DIAG_FILTER_FAULT_ALL,
         ar,
         request->api,
         request->slot_number,
         info,
         pos);
      break;
   case PF_IDX_SLOT_DIAGNOSIS_DMQS:
      pf_put_diagnosis_slot (
         net,
         true,
         PF_DIAG_FILTER_ALL,
         ar,
         request->api,
         request->slot_number,
         info,
         pos);
      break;
   case PF_IDX_SLOT_DIAG_MAINT_REQ_CH:
   case PF_IDX_SLOT_DIAG_MAINT_REQ_ALL:
      pf_put_diagnosis_slot (
         net,
         true,
         PF_DIAG_FILTER_M_REQ,
         ar,
         request->api,
         request->slot_number,
         info,
         pos);
      break;
   case PF_IDX_SLOT_DIAG_MAINT_DEM_CH:
   case PF_IDX_SLOT_DIAG_MAINT_DEM_ALL:
      pf_put_diagnosis_slot (
         net,
         true,
         PF_DIAG_FILTER_M_DEM,
         ar,
         request->api,
         request->slot_number,
         info,
         pos);
      break;

   case PF_IDX_API_DIAGNOSIS_CH:
      pf_put_diagnosis_api (
         net,
         true,
         PF_DIAG_FILTER_FAULT_STD,
         request->api,
         info,
         pos);
      break;
   case PF_IDX_API_DIAGNOSIS_ALL:
      pf_put_diagnosis_api (
         net,
         true,
         PF_DIAG_FILTER_FAULT_ALL,
         request->api,
         info,
         pos);
      break;
   case PF_IDX_API_DIAGNOSIS_DMQS:
      pf_put_diagnosis_api (
         net,
         true,
         PF_DIAG_FILTER_ALL,
         request->api,
         info,
         pos);
      break;
   case PF_IDX_API_DIAG_MAINT_REQ_CH:
   case PF_IDX_API_DIAG_MAINT_REQ_ALL:
      pf_put_diagnosis_api (
         net,
         true,
         PF_DIAG_FILTER_M_REQ,
         request->api,
         info,
         pos);
      break;
   case PF_IDX_API_DIAG_MAINT_DEM_CH:
   case PF_IDX_API_DIAG_MAINT_DEM_ALL:
      pf_put_diagnosis_api (
         net,
         true,
         PF_DIAG_FILTER_M_DEM,
         request->api,
         info,
         pos);
      break;

   case PF_IDX_AR_DIAGNOSIS_CH:
      pf_put_diagnosis_ar (
         net,
         true,
         PF_DIAG_FILTER_FAULT_STD,
         ar,
         info,
         pos);
      break;
   case PF_IDX_AR_DIAGNOSIS_ALL:
      pf_put_diagnosis_ar (
         net,
         true,
         PF_DIAG_FILTER_FAULT_ALL,
         ar,
         info,
         pos);
      break;
   case PF_IDX_AR_DIAGNOSIS_DMQS:
      pf_put_diagnosis_ar (net, true, PF_DIAG_FILTER_ALL, ar, info, pos);
      break;
   case PF_IDX_AR_DIAG_MAINT_REQ_CH:
   case PF_IDX_AR_DIAG_MAINT_REQ_ALL:
      pf_put_diagnosis_ar (net, true, PF_DIAG_FILTER_M_REQ, ar, info, pos);
      break;
   case PF_IDX_AR_DIAG_MAINT_DEM_CH:
   case PF_IDX_AR_DIAG_MAINT_DEM_ALL:
      pf_put_diagnosis_ar (net, true, PF_DIAG_FILTER_M_DEM, ar, info, pos);
      break;

   case PF_IDX_DEV_DIAGNOSIS_DMQS:
      pf_put_diagnosis_device (net, true, PF_DIAG_FILTER_ALL, info, pos);
      break;

   case PF_IDX_DEV_CONN_MON_TRIGGER:
      /* Restart timeout period. See pf_cmsm_rm_read_ind() */
      break;

   case PF_IDX_API_AR_DATA:
      pf_put_ar_data (net, true, true, request->api, info, pos);
      break;
   case PF_IDX_DEV_AR_DATA:
      pf_put_ar_data (net, true, false, 0, info, pos);
      break;
   case PF_IDX_AR_MOD_DIFF:
      if (ar && ar->io_ar)
      {
         (void)pf_cmdev_generate_module_diff (net, ar);
         pf_put_ar_diff (true, ar, info, pos);
      }
      break;

   case PF_IDX_SUB_PDPORT_DATA_REAL:
   case PF_IDX_SUB_PDPORTDATAREALEXT:
   case PF_IDX_SUB_PDPORT_DATA_ADJ:
   case PF_IDX_SUB_PDPORT_DATA_CHECK:
   case PF_IDX_DEV_PDREAL_DATA:
   case PF_IDX_DEV_PDEXP_DATA:
   case PF_IDX_SUB_PDPORT_STATISTIC:
   case PF_IDX_SUB_PDINTF_ADJUST:
      if (pf_pdport_read_ind (net, ar, request, info, pos) != 0)
      {
         pf_set_error (
            result,
            PNET_ERROR_CODE_READ,
            PNET_ERROR_DECODE_PNIORW,
            PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
            PNET_ERROR_CODE_2_PNIORW (10));
      }
      break;
#if PNET_OPTION_MRP
   case PF_IDX_SUB_PDINTF_MRP_REAL:
   case PF_IDX_SUB_PDINTF_MRP_CHECK:
   case PF_IDX_SUB_PDINTF_MRP_ADJUST:
   case PF_IDX_SUB_PDPORT_MRP_ADJUST:
   case PF_IDX_SUB_PDPORT_MRP_REAL:
      if (pf_pdport_mrp_read_ind (net, ar, request, info, pos) != 0)
      {
         pf_set_error (
            result,
            PNET_ERROR_CODE_READ,
            PNET_ERROR_DECODE_PNIORW,
            PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
            PNET_ERROR_CODE_2_PNIORW (10));
      }
      break;
#endif
   case PF_IDX_SUB_PDINTF_REAL:
      /* Only implemented for the interface subslot. */
      if (
         (request->slot_number == PNET_SLOT_DAP_IDENT) &&
         (request->subslot_number == PNET_SUBSLOT_DAP_INTERFACE_1_IDENT))
      {
         char station_name[PNET_STATION_NAME_MAX_SIZE]; /* Terminated */

         pf_cmina_get_station_name (net, station_name);
         pf_put_pdinterface_data_real (
            true,
            pf_cmina_get_device_macaddr (net),
            pf_cmina_get_ipaddr (net),
            pf_cmina_get_netmask (net),
            pf_cmina_get_gateway (net),
            station_name,
            info,
            pos);
      }
      else
      {
         pf_set_error (
            result,
            PNET_ERROR_CODE_READ,
            PNET_ERROR_DECODE_PNIORW,
            PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
            PNET_ERROR_CODE_2_PNIORW (10));
      }
      break;

   case PF_IDX_DEV_CIM_ELECTRIC_POWER_REAL:
   {
      pf_electric_power_device_data_t ep_device = {0};
      pf_electric_power_port_data_t ep_ports[PNET_MAX_PHYSICAL_PORTS] = {0};

      pf_cim_get_device_data (
         net,
         request->slot_number,
         request->subslot_number,
         &ep_device);
      pf_cim_get_port_data (
         net,
         PNET_MAX_PHYSICAL_PORTS,
         request->slot_number,
         request->subslot_number,
         ep_ports);
      pf_put_cim_electric_power_real (
         true,
         &ep_device,
         PNET_MAX_PHYSICAL_PORTS,
         ep_ports,
         request->slot_number,
         request->subslot_number,
         info,
         pos);
      break;
   }
   default:
      LOG_INFO (
         PNET_LOG,
         "CMRDR(%d): Index 0x%04X is not supported.\n",
         __LINE__,
         request->index);
      pf_set_error (
         result,
         PNET_ERROR_CODE_READ,
         PNET_ERROR_DECODE_PNIORW,
         PNET_ERROR_CODE_1_ACC_INVALID_INDEX,
         PNET_ERROR_CODE_2_PNIORW (10));
      break;
   }
   if (info->result == PF_PUT_OK)
   {
      /* Insert actual length. */
      pf_put_uint32 (true, (*pos - data_offset), info, &length_offset);
   }
   else
   {
      *pos = data_offset;
   }
}

int pf_cmrdr_rm_read_ind (
   pnet_t * net,
   pf_ar_t * ar,
   pf_iod_read_request_t const * request,
   pnet_result_t * result,
   uint16_t output_size,
   uint8_t * output,
   uint16_t * pos)
{
   pf_put_info_t info = {
      .p_frame = NULL,
      .p_array = output,
      .len = output_size,
      .result = PF_PUT_OK,
   };

   if (pf_index_is_user_specific (request->index))
   {
      pf_cmrdr_read_user_index (net, ar, request, &info, pos, result);
   }
   else if (
      (PF_IDX_SUB_IM_0 <= request->index) &&
      (request->index <= PF_IDX_SUB_IM_15))
   {
      pf_cmrdr_read_im_index (net, request, &info, pos, result);
   }
   else
   {
      pf_cmrdr_special_read (net, ar, request, &info, pos, result);
   }

   (void)pf_cmsm_cm_read_ind (net, ar, request);

   return ((info.result == PF_PUT_OK) ? 0 : -1);
}

const char * pf_index_to_logstring (uint16_t index)
{
   if (pf_index_is_user_specific (index))
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
   case PF_IDX_CIM_SNMP_ADJUST:
      return "CIMSNMPAdjust";
   case PF_IDX_CIM_SNMP_REAL:
      return "CIMSNMPReal";
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
   case PF_IDX_DEV_CIM_ELECTRIC_POWER_REAL:
      return "CIM Electric PowerReal";
   case PF_IDX_DEV_CONN_MON_TRIGGER:
      return "nothing, but is restarting the timeout period";
   default:
      return "unknown";
   }
}
