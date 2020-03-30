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
 * @brief Implements the Context Management Read Record Responder protocol machine device (CMRDR)
 *
 * Contains a single function \a pf_cmrdr_rm_read_ind(),
 * that handles a RPC parameter read request.
 *
 * This implementation of CMRDR has no internal state.
 * Every call to pf_cmrdr_rm_read_ind finishes by returning the result.
 * Since there are no internal static variables there is also no need
 * for a POWER-ON state.
 */


int pf_cmrdr_rm_read_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iod_read_request_t   *p_read_request,
   pnet_result_t           *p_read_status,
   uint16_t                res_size,
   uint8_t                 *p_res,
   uint16_t                *p_pos)
{
   int                     ret = -1;
   pf_iod_read_result_t    read_result;
   uint8_t                 *p_data = NULL;
   uint16_t                data_length_pos = 0;
   uint16_t                start_pos = 0;
   uint8_t                 iocs[255];              /* Max possible array size */
   uint8_t                 iops[255];              /* Max possible array size */
   uint8_t                 subslot_data[1500];     /* Max possible array size */
   uint8_t                 iocs_len = 0;
   uint8_t                 iops_len = 0;
   uint16_t                data_len = 0;
   bool                    new_flag = false;

   read_result.sequence_number = p_read_request->sequence_number;
   read_result.ar_uuid = p_read_request->ar_uuid;
   read_result.api = p_read_request->api;
   read_result.slot_number = p_read_request->slot_number;
   read_result.subslot_number = p_read_request->subslot_number;
   read_result.index = p_read_request->index;

   /*
    * Let FSPM or the application provide the value if possible.
    */
   data_len = res_size - *p_pos;
   ret = pf_fspm_cm_read_ind(net, p_ar, p_read_request, &p_data, &data_len, p_read_status);
   if (ret != 0)
   {
      data_len = 0;
      ret = 0;    /* Handled: No data available. */
   }

   /*
    * Just insert a dummy value for now.
    * It will be properly computed after the answer has been written to the buffer.
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
   pf_put_read_result(true, &read_result, res_size, p_res, p_pos, &data_length_pos);

   /*
    * Even if the application or FSPM provided an answer.
    * If there is another answer then use that!!
    */
   ret = -1;
   start_pos = *p_pos;
   if (p_read_request->index <= PF_IDX_USER_MAX)
   {
      /* Provided by application - accept whatever it says. */
      if (*p_pos + data_len < res_size)
      {
         memcpy(&p_res[*p_pos], p_data, data_len);
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
         pf_put_im_0_filter_data(net, true, res_size, p_res, p_pos);
         ret = 0;
         break;

      /*
       * I&M data are provided by FSPM.
       * Accept whatever it says (after verifying the data length).
       */
      case PF_IDX_SUB_IM_0:
         if ((data_len == sizeof(pnet_im_0_t)) && (*p_pos + data_len < res_size))
         {
            pf_put_im_0(true, (pnet_im_0_t *)p_data, res_size, p_res, p_pos);
            ret = 0;
         }
         break;
      case PF_IDX_SUB_IM_1:
         if ((data_len == sizeof(pnet_im_1_t)) && (*p_pos + data_len < res_size))
         {
            pf_put_im_1(true, (pnet_im_1_t *)p_data, res_size, p_res, p_pos);
            ret = 0;
         }
         break;
      case PF_IDX_SUB_IM_2:
         if ((data_len == sizeof(pnet_im_2_t)) && (*p_pos + data_len < res_size))
         {
            pf_put_im_2(true, (pnet_im_2_t *)p_data, res_size, p_res, p_pos);
            ret = 0;
         }
         break;
      case PF_IDX_SUB_IM_3:
         if ((data_len == sizeof(pnet_im_3_t)) && (*p_pos + data_len < res_size))
         {
            pf_put_im_3(true, (pnet_im_3_t *)p_data, res_size, p_res, p_pos);
            ret = 0;
         }
         break;
      case PF_IDX_SUB_IM_4:
         if ((data_len == sizeof(pnet_im_4_t)) && (*p_pos + data_len < res_size))
         {
            pf_put_record_data_read(true, PF_BT_IM_4, data_len, p_data, res_size, p_res, p_pos);
            ret = 0;
         }
         break;

      /*
       * I&M data are provided by FSPM.
       * Accept whatever it says (after verifying the data length).
       */
      case PF_IDX_DEV_LOGBOOK_DATA:
         /* Provided by FSPM. Accept whatever it says. */
         pf_put_log_book_data(true, (pf_log_book_t *)p_data, res_size, p_res, p_pos);
         ret = 0;
         break;

      /* Block-writer knows where to fetch and how to build the answer to ID data requests. */
      case PF_IDX_SUB_EXP_ID_DATA:
         pf_put_ident_data(net, true, PNET_BLOCK_VERSION_LOW_1, PF_BT_EXPECTED_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_SUBSLOT, PF_DEV_FILTER_LEVEL_SUBSLOT,
            p_ar, p_read_request->api, p_read_request->slot_number, p_read_request->subslot_number,
            res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_SUB_REAL_ID_DATA:
         pf_put_ident_data(net, true, PNET_BLOCK_VERSION_LOW_1, PF_BT_REAL_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_SUBSLOT, PF_DEV_FILTER_LEVEL_SUBSLOT,
            NULL, p_read_request->api, p_read_request->slot_number, p_read_request->subslot_number,
            res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_SLOT_EXP_ID_DATA:
         pf_put_ident_data(net, true, PNET_BLOCK_VERSION_LOW_1, PF_BT_EXPECTED_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_SLOT, PF_DEV_FILTER_LEVEL_SUBSLOT,
            p_ar, p_read_request->api, p_read_request->slot_number, 0,
            res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_SLOT_REAL_ID_DATA:
         pf_put_ident_data(net, true, PNET_BLOCK_VERSION_LOW_1, PF_BT_REAL_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_SLOT, PF_DEV_FILTER_LEVEL_SUBSLOT,
            NULL, p_read_request->api, p_read_request->slot_number, 0,
            res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_EXP_ID_DATA:
         pf_put_ident_data(net, true, PNET_BLOCK_VERSION_LOW_1, PF_BT_EXPECTED_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_DEVICE, PF_DEV_FILTER_LEVEL_SUBSLOT,
            p_ar, 0, 0, 0,
            res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_REAL_ID_DATA:
         pf_put_ident_data(net, true, PNET_BLOCK_VERSION_LOW_1, PF_BT_REAL_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_DEVICE, PF_DEV_FILTER_LEVEL_SUBSLOT,
            p_ar, 0, 0, 0,
            res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_API_REAL_ID_DATA:
         pf_put_ident_data(net, true, PNET_BLOCK_VERSION_LOW_1, PF_BT_REAL_IDENTIFICATION_DATA,
            PF_DEV_FILTER_LEVEL_API, PF_DEV_FILTER_LEVEL_SUBSLOT,
            NULL, p_read_request->api,
            0, 0, res_size, p_res, p_pos);
         ret = 0;
         break;

      case PF_IDX_DEV_API_DATA:
         pf_put_ident_data(net, true, PNET_BLOCK_VERSION_LOW, PF_BT_API_DATA,
            PF_DEV_FILTER_LEVEL_DEVICE, PF_DEV_FILTER_LEVEL_API_ID,
            NULL, 0, 0, 0, res_size, p_res, p_pos);
         ret = 0;
         break;

      case PF_IDX_SUB_INPUT_DATA:
         /* Sub-module data to the controller */
         data_len = sizeof(subslot_data);
         iops_len = sizeof(iops);
         iocs_len = sizeof(iocs);
         if (pf_ppm_get_data_and_iops(net, p_read_request->api, p_read_request->slot_number, p_read_request->subslot_number,
            subslot_data, &data_len, iops, &iops_len) != 0)
         {
            LOG_DEBUG(PNET_LOG, "CMRDR(%d): Could not get PPM data and IOPS\n", __LINE__);
            data_len = 0;
            iops_len = 0;
         }
         if (pf_cpm_get_iocs(net, p_read_request->api, p_read_request->slot_number, p_read_request->subslot_number, iocs, &iocs_len) != 0)
         {
            LOG_DEBUG(PNET_LOG, "CMRDR(%d): Could not get CPM IOCS\n", __LINE__);
            iocs_len = 0;
         }
         if ((data_len + iops_len + iocs_len) > 0)
         {
            /* Have all data */
            pf_put_input_data(true, iocs_len, iocs, iops_len, iops, data_len, subslot_data, res_size, p_res, p_pos);
            ret = 0;
         }
         break;
      case PF_IDX_SUB_OUTPUT_DATA:
         /* Sub-module data from the controller. */
         data_len = sizeof(subslot_data);
         iops_len = sizeof(iops);
         iocs_len = sizeof(iocs);
         if (pf_cpm_get_data_and_iops(net, p_read_request->api, p_read_request->slot_number, p_read_request->subslot_number,
            &new_flag, subslot_data, &data_len, iops, &iops_len) != 0)
         {
            LOG_DEBUG(PNET_LOG, "CMRDR(%d): Could not get CPM data and IOPS\n", __LINE__);
            data_len = 0;
            iops_len = 0;
         }
         if (pf_ppm_get_iocs(net, p_read_request->api, p_read_request->slot_number, p_read_request->subslot_number, iocs, &iocs_len) != 0)
         {
            LOG_DEBUG(PNET_LOG, "CMRDR(%d): Could not get PPM IOCS\n", __LINE__);
            iocs_len = 0;
         }
         if ((data_len + iops_len + iocs_len) > 0)
         {
            /* Have all data */
            /* ToDo: Support substitution values, mode and active flag */
            pf_put_output_data(true, false, iocs_len, iocs, iops_len, iops, data_len, subslot_data, 0, subslot_data, res_size, p_res, p_pos);
            ret = 0;
         }
         break;

      /* Block-writer knows where to fetch and how to build the answer to diag data requests. */
      case PF_IDX_SUB_DIAGNOSIS_CH:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_SUBSLOT, PF_DIAG_FILTER_FAULT_STD, NULL, p_read_request->api, p_read_request->slot_number, p_read_request->subslot_number, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_SUB_DIAGNOSIS_ALL:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_SUBSLOT, PF_DIAG_FILTER_FAULT_ALL, NULL, p_read_request->api, p_read_request->slot_number, p_read_request->subslot_number, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_SUB_DIAGNOSIS_DMQS:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_SUBSLOT, PF_DIAG_FILTER_ALL, NULL, p_read_request->api, p_read_request->slot_number, p_read_request->subslot_number, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_SUB_DIAG_MAINT_REQ_CH:
      case PF_IDX_SUB_DIAG_MAINT_REQ_ALL:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_SUBSLOT, PF_DIAG_FILTER_M_REQ, NULL, p_read_request->api, p_read_request->slot_number, p_read_request->subslot_number, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_SUB_DIAG_MAINT_DEM_CH:
      case PF_IDX_SUB_DIAG_MAINT_DEM_ALL:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_SUBSLOT, PF_DIAG_FILTER_M_DEM, NULL, p_read_request->api, p_read_request->slot_number, p_read_request->subslot_number, res_size, p_res, p_pos);
         ret = 0;
         break;

      case PF_IDX_SLOT_DIAGNOSIS_CH:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_SLOT, PF_DIAG_FILTER_FAULT_STD, NULL, p_read_request->api, p_read_request->slot_number, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_SLOT_DIAGNOSIS_ALL:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_SLOT, PF_DIAG_FILTER_FAULT_ALL, NULL, p_read_request->api, p_read_request->slot_number, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_SLOT_DIAGNOSIS_DMQS:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_SLOT, PF_DIAG_FILTER_ALL, NULL, p_read_request->api, p_read_request->slot_number, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_SLOT_DIAG_MAINT_REQ_CH:
      case PF_IDX_SLOT_DIAG_MAINT_REQ_ALL:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_SLOT, PF_DIAG_FILTER_M_REQ, NULL, p_read_request->api, p_read_request->slot_number, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_SLOT_DIAG_MAINT_DEM_CH:
      case PF_IDX_SLOT_DIAG_MAINT_DEM_ALL:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_SLOT, PF_DIAG_FILTER_M_DEM, NULL, p_read_request->api, p_read_request->slot_number, 0, res_size, p_res, p_pos);
         ret = 0;
         break;

      case PF_IDX_API_DIAGNOSIS_CH:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_API, PF_DIAG_FILTER_FAULT_STD, NULL, p_read_request->api, p_read_request->slot_number, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_API_DIAGNOSIS_ALL:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_API, PF_DIAG_FILTER_FAULT_ALL, NULL, p_read_request->api, p_read_request->slot_number, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_API_DIAGNOSIS_DMQS:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_API, PF_DIAG_FILTER_ALL, NULL, p_read_request->api, 0, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_API_DIAG_MAINT_REQ_CH:
      case PF_IDX_API_DIAG_MAINT_REQ_ALL:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_API, PF_DIAG_FILTER_M_REQ, NULL, p_read_request->api, p_read_request->slot_number, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_API_DIAG_MAINT_DEM_CH:
      case PF_IDX_API_DIAG_MAINT_DEM_ALL:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_API, PF_DIAG_FILTER_M_DEM, NULL, p_read_request->api, p_read_request->slot_number, 0, res_size, p_res, p_pos);
         ret = 0;
         break;

      case PF_IDX_AR_DIAGNOSIS_CH:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_DEVICE, PF_DIAG_FILTER_FAULT_STD, p_ar, 0, 0, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_DIAGNOSIS_ALL:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_DEVICE, PF_DIAG_FILTER_FAULT_ALL, p_ar, 0, 0, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_DIAGNOSIS_DMQS:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_DEVICE, PF_DIAG_FILTER_ALL, p_ar, 0, 0, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_DIAG_MAINT_REQ_CH:
      case PF_IDX_AR_DIAG_MAINT_REQ_ALL:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_DEVICE, PF_DIAG_FILTER_M_REQ, p_ar, 0, 0, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_DIAG_MAINT_DEM_CH:
      case PF_IDX_AR_DIAG_MAINT_DEM_ALL:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_DEVICE, PF_DIAG_FILTER_M_DEM, p_ar, 0, 0, 0, res_size, p_res, p_pos);
         ret = 0;
         break;

      case PF_IDX_DEV_DIAGNOSIS_DMQS:
         pf_put_diag_data(net, true, PF_DEV_FILTER_LEVEL_DEVICE, PF_DIAG_FILTER_ALL, NULL, 0, 0, 0, res_size, p_res, p_pos);
         ret = 0;
         break;

      case PF_IDX_DEV_CONN_MON_TRIGGER:
         ret = 0;
         break;

      case PF_IDX_API_AR_DATA:
         pf_put_ar_data(net, true, p_ar, p_read_request->api, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_DEV_AR_DATA:
         pf_put_ar_data(net, true, NULL, 0, res_size, p_res, p_pos);
         ret = 0;
         break;
      case PF_IDX_AR_MOD_DIFF:
         pf_put_ar_diff(true, p_ar, res_size, p_res, p_pos);
         ret = 0;
         break;

      case PF_IDX_SUB_PDPORT_DATA_REAL:
         /* ToDo: Implement properly when LLDP is done */
         /*
          * BlockHeader, Padding, Padding, SlotNumber, SubslotNumber, LengthOwnPortName, OwnPortName,
          * NumberOfPeers, [Padding*] a,
          * ToDo: Find out where the closing paran goes. It is missing in the spec!!
          * [LengthPeerPortName, PeerPortName, LengthPeerStationName, PeerStationName, [Padding*] a,
          *  LineDelay e, PeerMACAddress b, [Padding*] a ]*, MAUType c, [Padding*] a, Reserved d,
          *  RTClass3_PortStatus, MulticastBoundary, LinkState, [Padding*] a, MediaType
          * a The number of padding octets shall be adapted to make the block Unsigned32 aligned.
          * b This field contains the interface MAC address of the peer
          * c See Table 641
          * d The number of reserved octets shall be 2.
          * e The local calculated LineDelay shall be used.
          */
         ret = 0;
         break;
      case PF_IDX_SUB_PDPORT_DATA_CHECK:
         /* ToDo: Implement properly when LLDP is done */
         /*
          * BlockHeader, Padding, Padding, SlotNumber, SubslotNumber,
          * { [CheckPeers], [CheckLineDelay], [CheckMAUType a], [CheckLinkState],
          *   [CheckSyncDifference], [CheckMAUTypeDifference], [CheckMAUTypeExtension] b }
          * a The implementation of the MAUType check shall take the AutoNegotiation settings into account. A diagnosis with severity fault shall be issued if an AutoNegotiation error is detected after the next LinkDown / LinkUp do to the used AutoNegotiation settings.
          * b Only possible if CheckMAUType is part of the list
          */
         ret = 0;
         break;
      case PF_IDX_SUB_PDPORT_DATA_ADJ:
         /* ToDo: Implement properly when LLDP is done */
         /*
          * BlockHeader, Padding, Padding, SlotNumber, SubslotNumber,
          * { [AdjustDomainBoundary], [AdjustMulticastBoundary],
          *   [AdjustMAUType ^ AdjustLinkState],
          *   [AdjustPeerToPeerBoundary], [AdjustDCPBoundary],
          *   [AdjustPreambleLength], [AdjustMAUTypeExtension] a }
          * a Only possible if AdjustMAUType is part of the list
          */
         ret = 0;
         break;
      case PF_IDX_DEV_PDREAL_DATA:
         /* ToDo: Implement when LLDP is done */
         /*
          * MultipleBlockHeader, { [PDPortDataReal] b d, [PDPortDataRealExtended] b d,
          *                        [PDInterfaceMrpDataReal], [PDPortMrpDataReal],
          *                        [PDPortFODataReal] a, [PDInterfaceDataReal],
          *                        [PDPortStatistic], [PDPortMrpIcDataReal] }
          * a There shall be no FiberOpticManufacturerSpecific information
          * b The fields SlotNumber and SubslotNumber shall be ignored
          * c Each submodule's data (for example interface or port) need its own MultipleBlockHeader
          * d If MAUTypeExtension is used, both blocks shall be provided
          */
         ret = -1;
         break;
      case PF_IDX_DEV_PDEXP_DATA:
         /* ToDo: Implement when LLDP is done */
         /*
          * MultipleBlockHeader, { [PDPortDataCheck] a, [PDPortDataAdjust] a,
          *                        [PDInterfaceMrpDataAdjust], [PDInterfaceMrpDataCheck],
          *                        [PDPortMrpDataAdjust], [PDPortFODataAdjust],
          *                        [PDPortFODataCheck], [PDNCDataCheck],
          *                        [PDInterface-FSUDataAdjust], [PDInterfaceAdjust],
          *                        [PDPortMrpIcDataAdjust], [PDPortMrpIcDataCheck] }
          * a The fields SlotNumber and SubslotNumber shall be ignored
          * b Each submodule's data (for example interface or port) need its own MultipleBlockHeader
          */
         ret = -1;
         break;

      default:
         LOG_INFO(PNET_LOG, "cmrdr(%d): No support for reading index 0x%x\n", __LINE__, p_read_request->index);
         ret = -1;
         break;
      }
   }

   if (ret != 0)
   {
      p_read_status->pnio_status.error_code = PNET_ERROR_CODE_READ;
      p_read_status->pnio_status.error_decode = PNET_ERROR_DECODE_PNIORW;
      p_read_status->pnio_status.error_code_1 = PNET_ERROR_CODE_1_ACC_INVALID_INDEX;
      p_read_status->pnio_status.error_code_2 = 10;
      ret = 0;
   }

   read_result.record_data_length = *p_pos - start_pos;
   pf_put_uint32(true, read_result.record_data_length, res_size, p_res, &data_length_pos);   /* Insert actual data length */

   ret = pf_cmsm_cm_read_ind(net, p_ar, p_read_request);

   return ret;
}
