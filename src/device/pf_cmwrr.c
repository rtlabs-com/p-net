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
#include "pf_block_reader.h"
#include "pf_block_writer.h"


void pf_cmwrr_init(
   pnet_t                  *net)
{
   net->cmwrr_state = PF_CMWRR_STATE_IDLE;
}

/**
 * @internal
 * Return a string representation of a CMWRR state.
 * @param state            In:   The CMWRR state
 * @return A string representation of the CMWRR state.
 */
static const char *pf_cmwrr_state_to_string(
   pf_cmwrr_state_values_t state)
{
   const char *s = "<unknown>";

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

void pf_cmwrr_show(
   pnet_t                  *net,
   pf_ar_t                 *p_ar)
{
   const char *s = pf_cmwrr_state_to_string(net->cmwrr_state);

   printf("CMWRR state           = %s\n", s);
   (void)s;
}

int pf_cmwrr_cmdev_state_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_event_values_t     event)
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
      LOG_ERROR(PNET_LOG,"CMWRR(%d): BAD state in cmwrr %u\n", __LINE__, (unsigned)net->cmwrr_state);
      break;
   }

   return -1;
}

static void pf_cmwrr_pdportcheck(
		pnet_t                  *net,
		pf_ar_t                 *p_ar,
		pf_iod_write_request_t  *p_write_request,
		uint8_t                 *p_bytes,
		uint16_t                p_datalength,
		pnet_result_t           *p_result)
{
	uint8_t 		*pData = p_bytes;
	uint16_t 		tmpShort = 0;
	uint8_t 		tmpByte = 0;
	pnet_alarm_spec_t	alarm_spec;
	pf_diag_item_t 		diag_item;
	
    /* Request:
     * PDPortDataCheck
     *    blockHeader
     *       blabla
     *    padding(2)		 <= Request Pointer starts here
     *    slotNumber
     *    subslotNumber
     *    checkPeers
     *       blockHeader
     *          blabla
     *       numberOfPeers
     *       lengthPeerPortID
     *       peerPortID
     *       lengthPeerChassisID
     *       peerChassisId
     */
	
	/*Jump over the padding */
	pData += sizeof(p_write_request->padding);
	/*Jump over the slot/subslot */
	pData += sizeof(p_write_request->slot_number);
	pData += sizeof(p_write_request->subslot_number);
	
	/*Sanity check the block header*/
	memcpy(&tmpShort,pData,sizeof(tmpShort));
	tmpShort = htons(tmpShort);
	pData +=2;
	
	if(tmpShort!=PF_BT_CHECKPEERS)
	{
		return;
	}
	/*Jump over the block*/
	pData +=4;
	
	/*How many peer's are recorded?*/
	tmpByte = (uint8_t)pData[0];
	
	if(1>tmpByte)
	{
		return;
	}
	
	pData++;
	
	/* Set the PortID length */
	net->fspm_cfg.lldp_peer_req.PeerPortIDLen=(uint8_t)pData[0];
	pData++;
	
	/* Copy over the information */
	memcpy(&net->fspm_cfg.lldp_peer_req.PeerPortID, pData, net->fspm_cfg.lldp_peer_req.PeerPortIDLen);
	/* Null terminate */
	net->fspm_cfg.lldp_peer_req.PeerPortID[net->fspm_cfg.lldp_peer_req.PeerPortIDLen]='\0';
	pData +=net->fspm_cfg.lldp_peer_req.PeerPortIDLen;

	/* Set the PeerChassisID length */
	net->fspm_cfg.lldp_peer_req.PeerChassisIDLen=(uint8_t)pData[0];
	pData++;
	
	/* Copy over the information */
	memcpy(&net->fspm_cfg.lldp_peer_req.PeerChassisID, pData, net->fspm_cfg.lldp_peer_req.PeerChassisIDLen);
	
	/* Null terminate */
	net->fspm_cfg.lldp_peer_req.PeerChassisID[net->fspm_cfg.lldp_peer_req.PeerChassisIDLen]='\0';
	pData +=net->fspm_cfg.lldp_peer_req.PeerChassisIDLen;
	
	/*Use the TTL as a flag for number of peers connected to the device */
	net->fspm_cfg.lldp_peer_req.TTL = net->fspm_cfg.lldp_peer_cfg.TTL;

	/*Inform the system that there is a difference now*/
	uint32_t	api_ix = 0,
				mod_ix = 0,
				sub_ix = 0;
	uint16_t	errCode = 0;
	bool		bFound = false,
				is_problem = false;
	/*Check if there is a problem*/
	if( (memcmp(net->fspm_cfg.lldp_peer_req.PeerChassisID,net->fspm_cfg.lldp_peer_cfg.PeerChassisID,net->fspm_cfg.lldp_peer_req.PeerChassisIDLen) != 0))
	{
		is_problem = true;
		errCode = PF_WRT_ERROR_PORTID_MISMATCH;
	}
	if((memcmp(net->fspm_cfg.lldp_peer_req.PeerPortID,net->fspm_cfg.lldp_peer_cfg.PeerPortID,net->fspm_cfg.lldp_peer_req.PeerPortIDLen) != 0))
	{
		is_problem = true;
		errCode = PF_WRT_ERROR_CHASSISID_MISMATCH;
	}

	if(is_problem)
	{
		api_ix = p_ar->nbr_api_diffs;
		mod_ix = p_ar->api_diffs[api_ix].nbr_module_diffs;
		sub_ix = p_ar->api_diffs[api_ix].module_diffs[mod_ix].nbr_submodule_diffs;
		
		/*Search Modules*/
		for(int i = 0;i<p_ar->exp_apis[p_write_request->api].nbr_modules;i++)
		{
			/*Search SubModules*/
			for(int j = 0;j<p_ar->exp_apis[p_write_request->api].modules[i].nbr_submodules;j++)
			{
				if((p_ar->exp_apis[p_write_request->api].modules[i].slot_number == p_write_request->slot_number) &&
						(p_ar->exp_apis[p_write_request->api].modules[i].submodules[j].subslot_number == p_write_request->subslot_number))
				{
					/*Set the api to the request api */
					p_ar->api_diffs[api_ix].api = p_write_request->api;

					/*Set the slot number to the request slot number */
					p_ar->api_diffs[api_ix].module_diffs[mod_ix].slot_number 			=
							p_ar->exp_apis[p_write_request->api].modules[i].slot_number;
					
					/*Set the modID to the request modID */
					p_ar->api_diffs[api_ix].module_diffs[mod_ix].module_ident_number	= 
							p_ar->exp_apis[p_write_request->api].modules[i].module_ident_number;
					
					/*Set the subSlot to the request subSlot */
					p_ar->api_diffs[api_ix].module_diffs[mod_ix].submodule_diffs[sub_ix].subslot_number 		= 
							p_ar->exp_apis[p_write_request->api].modules[i].submodules[j].subslot_number;
					
					/*Set the subModID to the request subModID */
					p_ar->api_diffs[api_ix].module_diffs[mod_ix].submodule_diffs[sub_ix].submodule_ident_number	= 
							p_ar->exp_apis[p_write_request->api].modules[i].submodules[j].submodule_ident_number;

					/*Set the subMod error state*/
					p_ar->api_diffs[api_ix].module_diffs[mod_ix].submodule_diffs[sub_ix].submodule_state.fault = true;
					
					/* Increment the counter */
					p_ar->nbr_api_diffs++;
					p_ar->api_diffs[api_ix].nbr_module_diffs++;
					p_ar->api_diffs[api_ix].module_diffs[mod_ix].nbr_submodule_diffs++;
					

					memset(&diag_item,0,sizeof(pf_diag_item_t));
					
					diag_item.alarm_spec.channel_diagnosis 		= true;
					diag_item.alarm_spec.manufacturer_diagnosis = false;
					diag_item.alarm_spec.submodule_diagnosis 	= true;
					diag_item.alarm_spec.ar_diagnosis 			= true;
					  
					  /* Set Diagnostic Information Second */
		              PNET_DIAG_CH_PROP_SPEC_SET(diag_item.fmt.std.ch_properties, PNET_DIAG_CH_PROP_SPEC_APPEARS);
		              diag_item.usi = PF_USI_EXTENDED_CHANNEL_DIAGNOSIS;
		              diag_item.fmt.std.ch_nbr = PF_USI_CHANNEL_DIAGNOSIS;
		              diag_item.fmt.std.ch_error_type = PF_WRT_ERROR_REMOTE_MISMATCH;
		              diag_item.fmt.std.ext_ch_error_type = PF_WRT_ERROR_PORTID_MISMATCH;
		              diag_item.fmt.std.ext_ch_add_value = 0;
		              diag_item.fmt.std.qual_ch_qualifier = 0;
		              diag_item.next = 0;
		              
					/* Add the diagnostic block*/
					pf_diag_add(net,
							p_ar,
							api_ix,
							p_ar->exp_apis[p_write_request->api].modules[i].slot_number,
							p_ar->exp_apis[p_write_request->api].modules[i].submodules[j].subslot_number,
							PF_USI_CHANNEL_DIAGNOSIS,			/*Channel Number */
							diag_item.fmt.std.ch_properties,	/*Channel Properties */
							PF_WRT_ERROR_REMOTE_MISMATCH,		/*Channel Error Type: Remote Mismatch*/
							errCode,							/*Ext Channel Error Type: peer chassisid mismatch*/
							0,									/* Ext Channel Add Value */
							0,									/* Channel Qualifier ? */
							PF_USI_EXTENDED_CHANNEL_DIAGNOSIS,	/* USI*/
							&diag_item.alarm_spec,				/*Alarm Specifications */
							NULL);								/*Data*/
					
					bFound = true;
					break;
				}
			}
			
			if(bFound)
			{
				break;
			}
	
		}
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
 * @param p_ar             In:   The AR instance.
 * @param p_write_request  In:   The IODWrite request.
 * @param p_req_buf        In:   The request buffer.
 * @param data_length      In:   Size of the data to write.
 * @param p_req_pos        InOut:Position within the request buffer.
 * @param p_result         Out:  Detailed error information.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmwrr_write(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iod_write_request_t  *p_write_request,
   uint8_t                 *p_req_buf,
   uint16_t                data_length,
   uint16_t                *p_req_pos,
   pnet_result_t           *p_result)
{
   int ret = -1;

   if (p_write_request->index < 0x7fff)
   {
      ret = pf_fspm_cm_write_ind(net, p_ar, p_write_request,
         data_length, &p_req_buf[*p_req_pos], p_result);
   }
   else if ((PF_IDX_SUB_IM_0 <= p_write_request->index) && (p_write_request->index <= PF_IDX_SUB_IM_15))
   {
      ret = pf_fspm_cm_write_ind(net, p_ar, p_write_request,
         data_length, &p_req_buf[*p_req_pos], p_result);
   }
   else
   {
      switch (p_write_request->index)
      {
      case PF_IDX_SUB_PDPORT_DATA_CHECK:
    	  pf_cmwrr_pdportcheck(net, p_ar, p_write_request,&p_req_buf[*p_req_pos],data_length,p_result );
         ret = 0;
         break;
      case PF_IDX_SUB_PDPORT_DATA_ADJ:
         /* ToDo: Handle the request. */
         break;
      default:
         break;
      }
   }

   (*p_req_pos) += data_length;

   return ret;
}

int pf_cmwrr_rm_write_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iod_write_request_t  *p_write_request,
   pf_iod_write_result_t   *p_write_result,
   pnet_result_t           *p_result,
   uint8_t                 *p_req_buf,    /* request buffer */
   uint16_t                data_length,
   uint16_t                *p_req_pos)    /* In/out: position in request buffer */
{
   int                     ret = -1;

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
         ret = pf_cmwrr_write(net, p_ar, p_write_request, p_req_buf,
            data_length, p_req_pos, p_result);
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
         ret = pf_cmwrr_write(net, p_ar, p_write_request, p_req_buf,
            data_length, p_req_pos, p_result);
      }
      break;
   }

   p_write_result->add_data_1 = p_result->add_data_1;
   p_write_result->add_data_2 = p_result->add_data_2;

   ret = pf_cmsm_cm_write_ind(net, p_ar, p_write_request);

   return ret;
}
