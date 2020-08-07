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

/**
 * @file
 * @brief Helper functions for CMRPC
 *
 * Located in a separate file to enabling mocking during tests.
 *
 */

#include <string.h>
#include "pf_includes.h"
#include "pf_block_writer.h"

static const pf_rpc_uuid_type_t null_rpc_handle = {0,0,0,0,0,{0,0,0,0,0,0}};

static const pf_rpc_uuid_type_t     uuid_object = {
												0x00000000,
												0x0000,
												0x0000,
												0x00,
												0x00,
												{0x00,0x00,0x00,0x00,0x00,0x00}
											  };

static const pf_rpc_uuid_type_t     uuid_epmap_interface = {
												0xE1AF8308,
												0x5D1F,
												0x11C9,
												0x91,
												0xA4,
												{0x08,0x00,0x2B,0x14,0xA0,0xFA}
											  };

static const pf_rpc_uuid_type_t     uuid_io_device_interface = {
												0xDEA00001,
												0x6C97,
												0x11D1,
												0x82,
												0x71,
												{0x00,0xA0,0x24,0x42,0xDF,0x7D}
											  };

static pf_rpc_uuid_type_t     		uuid_io_object_instance = {
												0xDEA00000,
												0x6C97,
												0x11D1,
												0x82,
												0x71,
												{0x00,0x00,0x00,0x00,0x00,0x00}
											  };

static const pf_rpc_uuid_type_t     uuid_ndr = {
												0x8A885D04,
												0x1CEB,
												0x11C9,
												0x9F,
												0xE8,
												{0x08,0x00,0x2B,0x10,0x48,0x60}
											  };

/**
 * @internal
 * Generate an UUID, inspired by version 1, variant 1
 *
 * See RFC 4122
 *
 * @param timestamp        In:   Some kind of timestamp
 * @param session_number   In:   Session number
 * @param mac_address      In:   MAC address
 * @param p_uuid           Out:  The generated UUID
 */
void pf_generate_uuid(
   uint32_t                timestamp,
   uint32_t                session_number,
   pnet_ethaddr_t            mac_address,
   pf_uuid_t               *p_uuid)
{
   p_uuid->data1 = timestamp;
   p_uuid->data2 = session_number >> 16;
   p_uuid->data3 = (session_number >> 8) & 0x00ff;
   p_uuid->data3 |= 1 << 12;  /* UUID version 1 */
   p_uuid->data4[0] = 0x80;  /* UUID variant 1 */
   p_uuid->data4[1] = session_number & 0xff;
   p_uuid->data4[2] = mac_address.addr[0];
   p_uuid->data4[3] = mac_address.addr[1];
   p_uuid->data4[4] = mac_address.addr[2];
   p_uuid->data4[5] = mac_address.addr[3];
   p_uuid->data4[6] = mac_address.addr[4];
   p_uuid->data4[7] = mac_address.addr[5];
}

static void pf_generate_handle(
   uint32_t                timestamp,
   pnet_ethaddr_t          mac_address,
   pf_rpc_handle_t        *p_handle)
{
	p_handle->rpc_entry_handle 					= 0;
   p_handle->handle_uuid.time_low 				= (timestamp&0x0000FFFF);
   p_handle->handle_uuid.time_mid 				= (timestamp&0x00FF0000);
   p_handle->handle_uuid.time_hi_and_version 	= (timestamp&0xFF000000)|0x1000;
   p_handle->handle_uuid.clock_hi_and_reserved 	= 0x80;
   p_handle->handle_uuid.clock_low 				= /* 0x11 */0x0C;
   p_handle->handle_uuid.node[0] 				= mac_address.addr[0];
   p_handle->handle_uuid.node[1] 				= mac_address.addr[1];
   p_handle->handle_uuid.node[2] 				= mac_address.addr[2];
   p_handle->handle_uuid.node[3] 				= mac_address.addr[3];
   p_handle->handle_uuid.node[4] 				= mac_address.addr[4];
   p_handle->handle_uuid.node[5] 				= mac_address.addr[5];
}

typedef enum pf_floor_uuid_type
{
	PF_FLOOR_UUID_EPMv4 = 0,
	PF_FLOOR_UUID_PNIO,
	PF_FLOOR_UUID_NDR,
	PF_FLOOR_UUID_RPC,
	PF_FLOOR_UUID_UDP_DST,
	PF_FLOOR_UUID_UDP_SRC,
	PF_FLOOR_UUID_IP_ADDR
}pf_floor_uuid_type_t;

typedef enum pf_rpc_protocol_type
{
	PF_RPC_PROTOCOL_DOD_UDP 		= 0x08,
	PF_RPC_PROTOCOL_DOD_IP 			= 0x09,
	PF_RPC_PROTOCOL_CONNECTIONLESS 	= 0x0a,
	PF_RPC_PROTOCOL_UUID			= 0x0d
}pf_rpc_protocol_type_t;

/* Reference PN-AL-protocol_2722_v24_May19 Section 4.10.3 on 
 * how this information is relayed back to the requestor*/
static int pf_cmrdr_add_epmv4_entry(
	    pnet_t                  *net,
	    pf_ar_t                 *p_ar,
	    pf_rpc_header_t         *p_rpc_req,
	    pf_rpc_lookup_req_t     *p_lkup_request,
	    pf_rpc_lookup_rsp_t 	*p_rsp_lookup,
	    pnet_result_t           *p_read_status,
	    uint16_t                res_size,
	    uint8_t                 *p_res,
	    uint16_t                *p_pos)
{
	pf_rpc_entry_t 	*pEntry = p_rsp_lookup->rpc_entries;			/* RPC Entry pointer */
	pf_rpc_tower_t 	*pTower = &pEntry->tower_entry;					/* Tower Pointer */
	uint32_t		*p_floor_pos = &pTower->floor_length1;			/* Floor data size */
	uint8_t       	*p_floor_data = &pTower->floor_data[0];			/* Floor data pointer */
	pnet_im_0_t 	*pIm0Data 	= &net->fspm_cfg.im_0_data;	/* IM&0 Data*/
	
	uint16_t		maxFloorDataSize = 
			(uint16_t)(sizeof(pTower->floor_data)/sizeof(pTower->floor_data[0]));
	
	/*Set the handle information*/
	pf_generate_handle(os_get_current_time_us(), 
    		net->fspm_cfg.eth_addr, 
    		&p_rsp_lookup->rpc_handle);
	
	/*Set the number of entries */
	p_rsp_lookup->num_entry++;
	
	/*Increment the RPC Entry counter*/
	pEntry->actual_count++;
	pEntry->max_count = pEntry->actual_count;
	
	/*Offset is always 0*/
	pEntry->offset = 0;
	
	/*Fill in entry object UUID*/
	memcpy(&pEntry->object_uuid,&uuid_object,sizeof(uuid_object));
	
	/* Fill in the tower referentID*/
	pTower->referentID = RPC_TOWER_REFERENTID;

	/*Floor Count*/
	pf_put_rpc_floor_count(net, false, RPC_TOWER_FLOOR_COUNT, maxFloorDataSize,p_floor_data,(uint16_t*)p_floor_pos);
	
	/* Floor 1 EPMv4 */
	pf_put_rpc_floor_with_uuid(net, false, 
			PF_RPC_PROTOCOL_UUID, 
			uuid_epmap_interface,
			RPC_FLOOR_VERSION_EPMv4,
			RPC_FLOOR_VERSION_MINOR,
			maxFloorDataSize,
			p_floor_data,
			(uint16_t*)p_floor_pos);

	/* Floor 2 32bit NDR*/
	pf_put_rpc_floor_with_uuid(net, false, 
			PF_RPC_PROTOCOL_UUID, 
			uuid_ndr,
			RPC_FLOOR_VERSION_NPR,
			RPC_FLOOR_VERSION_MINOR,
			maxFloorDataSize,
			p_floor_data,
			(uint16_t*)p_floor_pos);
	
	/* Floor 3 RPC*/
	pf_put_rpc_floor_rpc(net, false,
			PF_RPC_PROTOCOL_CONNECTIONLESS,
			RPC_FLOOR_VERSION_MINOR,
			maxFloorDataSize,
			p_floor_data,
			(uint16_t*)p_floor_pos);
			
	/* Floor 4 UDP Listening port*/
	pf_put_rpc_floor_udp(net, false,
			PF_RPC_PROTOCOL_DOD_UDP,
			htons(PF_RPC_SERVER_PORT),
			maxFloorDataSize,
			p_floor_data,
			(uint16_t*)p_floor_pos);
	
	/* Floor 5 IP Address */
	pf_put_rpc_floor_ip(net, false,
			PF_RPC_PROTOCOL_DOD_IP,
			0,
			maxFloorDataSize,
			p_floor_data,
			(uint16_t*)p_floor_pos);
	
	/*Set the floor lengths to the same value*/
	pTower->floor_length2 = *p_floor_pos;
	
	/*Add annotation data*/
	pTower->annotation_offset = 0;
	pTower->annotation_length = (uint32_t)(sizeof(pTower->annotation_data)/sizeof(pTower->annotation_data[0]));
	snprintf((char*)pTower->annotation_data,pTower->annotation_length,"%-51s%-2d%-3c%-3d%-3d%-2d",
			pIm0Data->deviceType,
			pIm0Data->im_hardware_revision,
			pIm0Data->sw_revision_prefix,
			pIm0Data->im_sw_revision_functional_enhancement,
			pIm0Data->im_sw_revision_bug_fix,
			pIm0Data->im_sw_revision_internal_change);
	
	p_rsp_lookup->return_code = PF_RPC_OK;
	
	/* Now add all this information into the response buffer */
	pf_put_lookup_response_data(net, false, p_rsp_lookup, res_size, p_res, p_pos);
	return 0;
}

/* Reference PN-AL-protocol_2722_v24_May19 Section 4.10.3 on 
 * how this information is relayed back to the requestor*/
static int pf_cmrdr_add_pnio_entry(
	    pnet_t                  *net,
	    pf_ar_t                 *p_ar,
	    pf_rpc_header_t         *p_rpc_req,
	    pf_rpc_lookup_req_t     *p_lkup_request,
	    pf_rpc_lookup_rsp_t 	*p_rsp_lookup,
	    pnet_result_t           *p_read_status,
	    uint16_t                res_size,
	    uint8_t                 *p_res,
	    uint16_t                *p_pos)
{
	pf_rpc_entry_t 	*pEntry = p_rsp_lookup->rpc_entries;			/* RPC Entry pointer */
	pf_rpc_tower_t 	*pTower = &pEntry->tower_entry;					/* Tower Pointer */
	uint32_t		*p_floor_pos = &pTower->floor_length1;			/* Floor data size */
	uint8_t       	*p_floor_data = &pTower->floor_data[0];			/* Floor data pointer */
	pnet_im_0_t 	*pIm0Data 	= &net->fspm_cfg.im_0_data;	/* IM&0 Data*/

	uint16_t		maxFloorDataSize = 
			(uint16_t)(sizeof(pTower->floor_data)/sizeof(pTower->floor_data[0]));

	/*Set the handle information*/
	pf_generate_handle(os_get_current_time_us(), 
			net->fspm_cfg.eth_addr, 
			&p_rsp_lookup->rpc_handle);

	/*Set the number of entries */
	p_rsp_lookup->num_entry++;

	/*Increment the RPC Entry counter*/
	pEntry->actual_count++;
	pEntry->max_count = pEntry->actual_count;

	/*Offset is always 0*/
	pEntry->offset = 0;

	/*Fill in entry object UUID with the correct information*/
	
	/*Interface index (Assume 1 interface for now)*/
	uuid_io_object_instance.node[0] = 0x00;
	uuid_io_object_instance.node[1] = 0x01;
	/*Device ID*/
	uuid_io_object_instance.node[2] = net->fspm_cfg.device_id.device_id_hi;
	uuid_io_object_instance.node[3] = net->fspm_cfg.device_id.device_id_lo;
	/*Vendor ID*/
	uuid_io_object_instance.node[4] = net->fspm_cfg.device_id.vendor_id_hi;
	uuid_io_object_instance.node[5] = net->fspm_cfg.device_id.vendor_id_lo;
	
	/*Copy data over now*/
	memcpy(&pEntry->object_uuid,&uuid_io_object_instance,sizeof(uuid_object));

	/* Fill in the tower referentID*/
	pTower->referentID = RPC_TOWER_REFERENTID;

	/*Floor Count*/
	pf_put_rpc_floor_count(net, false, RPC_TOWER_FLOOR_COUNT, maxFloorDataSize,p_floor_data,(uint16_t*)p_floor_pos);

	/* Floor 1 PNIO */
	pf_put_rpc_floor_with_uuid(net, false, 
			PF_RPC_PROTOCOL_UUID, 
			uuid_io_device_interface,
			RPC_FLOOR_VERSION_PNIO,
			RPC_FLOOR_VERSION_MINOR,
			maxFloorDataSize,
			p_floor_data,
			(uint16_t*)p_floor_pos);

	/* Floor 2 32bit NDR*/
	pf_put_rpc_floor_with_uuid(net, false, 
			PF_RPC_PROTOCOL_UUID, 
			uuid_ndr,
			RPC_FLOOR_VERSION_NPR,
			RPC_FLOOR_VERSION_MINOR,
			maxFloorDataSize,
			p_floor_data,
			(uint16_t*)p_floor_pos);

	/* Floor 3 RPC*/
	pf_put_rpc_floor_rpc(net, false,
			PF_RPC_PROTOCOL_CONNECTIONLESS,
			RPC_FLOOR_VERSION_MINOR,
			maxFloorDataSize,
			p_floor_data,
			(uint16_t*)p_floor_pos);
			
	/* Floor 4 UDP Listening port*/
	pf_put_rpc_floor_udp(net, false,
			PF_RPC_PROTOCOL_DOD_UDP,
			htons(p_lkup_request->udpPort),
			maxFloorDataSize,
			p_floor_data,
			(uint16_t*)p_floor_pos);

	/* Floor 5 IP Address */
	pf_put_rpc_floor_ip(net, false,
			PF_RPC_PROTOCOL_DOD_IP,
			0,
			maxFloorDataSize,
			p_floor_data,
			(uint16_t*)p_floor_pos);

	/*Set the floor lengths to the same value*/
	pTower->floor_length2 = *p_floor_pos;

	/*Add annotation data*/
	pTower->annotation_offset = 0;
	pTower->annotation_length = (uint32_t)(sizeof(pTower->annotation_data)/sizeof(pTower->annotation_data[0]));
	snprintf((char*)pTower->annotation_data,pTower->annotation_length,"%-51s%-2d%-3c%-3d%-3d%-2d",
			pIm0Data->deviceType,
			pIm0Data->im_hardware_revision,
			pIm0Data->sw_revision_prefix,
			pIm0Data->im_sw_revision_functional_enhancement,
			pIm0Data->im_sw_revision_bug_fix,
			pIm0Data->im_sw_revision_internal_change);

	p_rsp_lookup->return_code = PF_RPC_OK;

	/* Now add all this information into the response buffer */
	pf_put_lookup_response_data(net, false, p_rsp_lookup, res_size, p_res, p_pos);
	return 0;
}

static int pf_cmrdr_inquiry_read_all_reg_ind(
	    pnet_t                  *net,
	    pf_ar_t                 *p_ar,
	    pf_rpc_header_t         *p_rpc_req,
	    pf_rpc_lookup_req_t     *p_lkup_request,
	    pf_rpc_lookup_rsp_t 	*p_rsp_lookup,
	    pnet_result_t           *p_read_status,
	    uint16_t                res_size,
	    uint8_t                 *p_res,
	    uint16_t                *p_pos)
{
	int ret = -1;	/*Set to ERROR*/
	
	/*Base our return on the Sequence Number in RPC Request Header*/
	switch(p_rpc_req->sequence_nmb)
	{
		case 0:
		{
			/*Check if handle is all NULL (it should be!)*/
			if( memcmp(&p_lkup_request->rpc_handle,&null_rpc_handle,sizeof(null_rpc_handle)) == 0)
			{
				/*Send the endpoint interface information to the requestor*/
				ret = pf_cmrdr_add_epmv4_entry(net, p_ar, p_rpc_req, p_lkup_request, p_rsp_lookup, p_read_status, res_size, p_res, p_pos);
			}
		}
			break;
		case 1:
			/*Check if handle is NOT NULL (it should be!)*/
			if( memcmp(&p_lkup_request->rpc_handle,&null_rpc_handle,sizeof(null_rpc_handle)) != 0)
			{
				/*Send the PNIO interface information information to the requestor*/
				ret = pf_cmrdr_add_pnio_entry(net, p_ar, p_rpc_req, p_lkup_request, p_rsp_lookup, p_read_status, res_size, p_res, p_pos);
			}
			break;
		default:
			break;
	}
	
	if(ret == -1)
	{
	   /*Always send an empty response*/
		pf_put_lookup_response_data(net, true, p_rsp_lookup, res_size, p_res, p_pos);
		ret = 0;
	}
	
	return ret;
}
 
int pf_cmrpc_lookup_request(
    pnet_t                  *net,
    pf_ar_t                 *p_ar,
    pf_rpc_header_t         *p_rpc_req,
    pf_rpc_lookup_req_t     *p_lkup_request,
    pnet_result_t           *p_read_status,
    uint16_t                res_size,
    uint8_t                 *p_res,
    uint16_t                *p_pos)
{
    int                     ret = -1;
    pf_rpc_lookup_rsp_t 	rsp_lookup;
    
    
    memset(&rsp_lookup, 0, sizeof(rsp_lookup));
    
    /*Assign a blank entry point*/
    rsp_lookup.rpc_entries = (pf_rpc_entry_t*)malloc(sizeof(pf_rpc_entry_t));
	memset( rsp_lookup.rpc_entries,0,sizeof(pf_rpc_entry_t));
	rsp_lookup.rpc_entries->next_entry = NULL;
	
	/*Max Count is always 1 */
	rsp_lookup.rpc_entries->max_count = 1;
	
    /*Assume the worst*/
    rsp_lookup.return_code = PF_RPC_NOT_REGISTERED;
    
	if(p_lkup_request->max_entries < 1)
	{
	   /*Send empty response*/
		pf_put_lookup_response_data(net, false,&rsp_lookup, res_size, p_res, p_pos);
		ret = 0;
	}
	else
	{
		switch (p_lkup_request->inquiry_type)
		{
		case PF_RPC_INQUIRY_READ_ALL_REGISTERED_INTERFACES:
			ret = pf_cmrdr_inquiry_read_all_reg_ind(net, p_ar, p_rpc_req, p_lkup_request,&rsp_lookup, p_read_status, res_size, p_res, p_pos);
			break;
			/*The following are unsupported at the moment*/
		case PF_RPC_INQUIRY_READ_ALL_OBJECTS_FOR_ONE_INTERFACE:
		case PF_RPC_INQUIRY_READ_ALL_INTERFACES_INCLUDING_OBJECTS:
		case PF_RPC_INQUIRY_READ_ONE_INTERFACE_WITH_ONE_OBJECT:
		default:
			rsp_lookup.return_code = PF_RPC_NOT_REGISTERED;
		   /*Send empty response*/
			pf_put_lookup_response_data(net, true, &rsp_lookup, res_size, p_res, p_pos);
			ret = 0;
		   break;
		}
	}
	


    return ret;
}

