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
 * @brief RPC End Point Mapper
 *
 */

#include <string.h>
#include "pf_includes.h"
#include "pf_block_writer.h"

static const pf_rpc_uuid_type_t null_rpc_handle =
   {0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0}};

static const pf_rpc_uuid_type_t uuid_object = {
   0x00000000,
   0x0000,
   0x0000,
   0x00,
   0x00,
   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static const pf_rpc_uuid_type_t uuid_epmap_interface = {
   0xE1AF8308,
   0x5D1F,
   0x11C9,
   0x91,
   0xA4,
   {0x08, 0x00, 0x2B, 0x14, 0xA0, 0xFA}};

static const pf_rpc_uuid_type_t uuid_io_device_interface = {
   0xDEA00001,
   0x6C97,
   0x11D1,
   0x82,
   0x71,
   {0x00, 0xA0, 0x24, 0x42, 0xDF, 0x7D}};

static const pf_rpc_uuid_type_t uuid_io_object_instance = {
   0xDEA00000,
   0x6C97,
   0x11D1,
   0x82,
   0x71,
   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static const pf_rpc_uuid_type_t uuid_ndr = {
   0x8A885D04,
   0x1CEB,
   0x11C9,
   0x9F,
   0xE8,
   {0x08, 0x00, 0x2B, 0x10, 0x48, 0x60}};

/**
 * Generate rpc epm handle
 * @param timestamp     In: Timestamp
 * @param mac_address   In: Mac address
 * @param p_handle      Out: Generated handle
 */
static void pf_generate_epm_handle (
   uint32_t timestamp,
   pnet_ethaddr_t mac_address,
   pf_rpc_handle_t * p_handle)
{
   p_handle->rpc_entry_handle = 0;
   p_handle->handle_uuid.time_low = (timestamp & 0x0000FFFF);
   p_handle->handle_uuid.time_mid = (timestamp & 0x00FF0000);
   p_handle->handle_uuid.time_hi_and_version = (timestamp & 0xFF000000) |
                                               0x1000;
   p_handle->handle_uuid.clock_hi_and_reserved = 0x80;
   p_handle->handle_uuid.clock_low = /* 0x11 */ 0x0C;
   p_handle->handle_uuid.node[0] = mac_address.addr[0];
   p_handle->handle_uuid.node[1] = mac_address.addr[1];
   p_handle->handle_uuid.node[2] = mac_address.addr[2];
   p_handle->handle_uuid.node[3] = mac_address.addr[3];
   p_handle->handle_uuid.node[4] = mac_address.addr[4];
   p_handle->handle_uuid.node[5] = mac_address.addr[5];
}

/**
 * Initialize protocol tower configuration including floors
 * @param p_tower    Out: Protocol tower configuration
 * @param epm_type   In: End point type, EPM or PNIO
 * @param udp_port   In: UDP protocol port
 */
static void pf_init_rpc_tower_entry (
   pf_rpc_tower_t * p_tower,
   pf_rpc_epm_type_t epm_type,
   uint16_t udp_port)
{
   /* Floor 1 EPMv4 */
   p_tower->floor_1.protocol_id = PF_RPC_PROTOCOL_UUID;
   if (epm_type == PF_EPM_TYPE_EPMV4)
   {
      memcpy (
         &p_tower->floor_1.uuid,
         &uuid_epmap_interface,
         sizeof (p_tower->floor_1.uuid));
      p_tower->floor_1.version_major = PF_RPC_FLOOR_VERSION_EPMv4;
   }
   else
   {
      memcpy (
         &p_tower->floor_1.uuid,
         &uuid_io_device_interface,
         sizeof (p_tower->floor_1.uuid));
      p_tower->floor_1.version_major = PF_RPC_FLOOR_VERSION_PNIO;
   }
   p_tower->floor_1.version_minor = PF_RPC_FLOOR_VERSION_MINOR;

   /* Floor 2 32bit NDR*/
   p_tower->floor_2.protocol_id = PF_RPC_PROTOCOL_UUID;
   memcpy (&p_tower->floor_2.uuid, &uuid_ndr, sizeof (p_tower->floor_2.uuid));
   p_tower->floor_2.version_major = PF_RPC_FLOOR_VERSION_NPR;
   p_tower->floor_2.version_minor = PF_RPC_FLOOR_VERSION_MINOR;

   /* Floor 3 RPC*/
   p_tower->floor_3.protocol_id = PF_RPC_PROTOCOL_CONNECTIONLESS;
   p_tower->floor_3.version_minor = PF_RPC_FLOOR_VERSION_MINOR;

   /* Floor 4 UDP Listening port*/
   p_tower->floor_4.protocol_id = PF_RPC_PROTOCOL_DOD_UDP;
   p_tower->floor_4.port = htons (udp_port);

   /* Floor 5 IP Address */
   p_tower->floor_5.protocol_id = PF_RPC_PROTOCOL_DOD_IP;
   p_tower->floor_5.ip_address = 0;
}

/**
 * Add EPMv4 endpoint to lookup response
 * @param net              InOut: The p-net stack instance
 * @param p_rpc_req        In:    Rpc header.
 * @param p_lookup_req     In:    Lookup request.
 * @param p_lookup_rsp     Out:   Lookup response.
 * @param p_read_status    Out:   Read pnio status
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrdr_add_epmv4_entry (
   pnet_t * net,
   const pf_rpc_header_t * p_rpc_req,
   const pf_rpc_lookup_req_t * p_lookup_req,
   pf_rpc_lookup_rsp_t * p_lookup_rsp,
   pnet_result_t * p_read_status)
{
   pf_generate_epm_handle (
      os_get_current_time_us(),
      net->pf_interface.main_port.mac_address,
      &p_lookup_rsp->rpc_handle);

   p_lookup_rsp->num_entry++;

   memcpy (
      &p_lookup_rsp->rpc_entry.object_uuid,
      &uuid_object,
      sizeof (uuid_object));
   p_lookup_rsp->rpc_entry.offset = 0;
   p_lookup_rsp->rpc_entry.actual_count++;
   p_lookup_rsp->rpc_entry.max_count = p_lookup_rsp->rpc_entry.actual_count;

   pf_init_rpc_tower_entry (
      &p_lookup_rsp->rpc_entry.tower_entry,
      PF_EPM_TYPE_EPMV4,
      PF_RPC_SERVER_PORT);

   p_lookup_rsp->return_code = PF_RPC_OK;
   return 0;
}

/**
 * Add PNIO endpoint to lookup response
 * @param net              InOut: The p-net stack instance
 * @param p_rpc_req        In:    Rpc header.
 * @param p_lookup_req     In:    Lookup request.
 * @param p_lookup_rsp     Out:   Lookup response.
 * @param p_read_status    Out:   Read pnio status
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrdr_add_pnio_entry (
   pnet_t * net,
   const pf_rpc_header_t * p_rpc_req,
   const pf_rpc_lookup_req_t * p_lookup_req,
   pf_rpc_lookup_rsp_t * p_lookup_rsp,
   pnet_result_t * p_read_status)
{
   pf_generate_epm_handle (
      os_get_current_time_us(),
      net->pf_interface.main_port.mac_address,
      &p_lookup_rsp->rpc_handle);

   /*Set the number of entries */
   p_lookup_rsp->num_entry++;

   /*Increment the RPC Entry counter*/
   p_lookup_rsp->rpc_entry.actual_count++;

   /*Offset is always 0*/
   p_lookup_rsp->rpc_entry.offset = 0;

   /*Fill in entry object UUID with the correct information*/
   memcpy (
      &p_lookup_rsp->rpc_entry.object_uuid,
      &uuid_io_object_instance,
      sizeof (uuid_io_object_instance));

   /*Interface index (Todo Assume 1 interface for now)*/
   p_lookup_rsp->rpc_entry.object_uuid.node[0] = 0x00;
   p_lookup_rsp->rpc_entry.object_uuid.node[1] = 0x01;
   /*Device ID*/
   p_lookup_rsp->rpc_entry.object_uuid.node[2] =
      net->fspm_cfg.device_id.device_id_hi;
   p_lookup_rsp->rpc_entry.object_uuid.node[3] =
      net->fspm_cfg.device_id.device_id_lo;
   /*Vendor ID*/
   p_lookup_rsp->rpc_entry.object_uuid.node[4] =
      net->fspm_cfg.device_id.vendor_id_hi;
   p_lookup_rsp->rpc_entry.object_uuid.node[5] =
      net->fspm_cfg.device_id.vendor_id_lo;

   pf_init_rpc_tower_entry (
      &p_lookup_rsp->rpc_entry.tower_entry,
      PF_EPM_TYPE_PNIO,
      p_lookup_req->udpPort);

   p_lookup_rsp->return_code = PF_RPC_OK;
   return 0;
}

/**
 * Handle epm read all inquiry request
 * PN-AL-protocol (Mar20) Section 4.10.3
 *
 * PN-AL-services (Mar20) Figure 3
 * Sequence Chart for reading the EndPointMapper
 * Base our return on the Sequence Number in RPC Request Header
 *
 * @param net              InOut: The p-net stack instance
 * @param p_rpc_req        In:    Rpc header.
 * @param p_lookup_req     In:    Lookup request.
 * @param p_lookup_rsp     Out:   Lookup response.
 * @param p_read_status    Out:   Read pnio status
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrdr_inquiry_read_all_reg_ind (
   pnet_t * net,
   const pf_rpc_header_t * p_rpc_req,
   const pf_rpc_lookup_req_t * p_lookup_req,
   pf_rpc_lookup_rsp_t * p_lookup_rsp,
   pnet_result_t * p_read_status)
{
   int ret = -1;

   switch (p_rpc_req->sequence_nmb)
   {
   case 0:
   {
      /* Check if handle is all NULL */
      if (
         memcmp (
            &p_lookup_req->rpc_handle,
            &null_rpc_handle,
            sizeof (null_rpc_handle)) == 0)
      {
         /* Send the EPM interface information */
         LOG_INFO (
            PF_RPC_LOG,
            "EPM(%d): PLC is requesting EPM interface information.\n",
            __LINE__);
         ret = pf_cmrdr_add_epmv4_entry (
            net,
            p_rpc_req,
            p_lookup_req,
            p_lookup_rsp,
            p_read_status);
      }
   }
   break;
   case 1:
      /* Check if handle is NOT NULL */
      if (
         memcmp (
            &p_lookup_req->rpc_handle,
            &null_rpc_handle,
            sizeof (null_rpc_handle)) != 0)
      {
         /* Send the PNIO interface information */
         LOG_INFO (
            PF_RPC_LOG,
            "EPM(%d): PLC is requesting PNIO interface information via EPM.\n",
            __LINE__);
         ret = pf_cmrdr_add_pnio_entry (
            net,
            p_rpc_req,
            p_lookup_req,
            p_lookup_rsp,
            p_read_status);
      }
      break;
   default:
      break;
   }
   return ret;
}

/**
 * Handle epm lookup request
 *
 * @param net              InOut: The p-net stack instance
 * @param p_rpc_req        In:    Rpc header.
 * @param p_lookup_req     In:    Lookup request.
 * @param p_read_status    Out:   Read pnio status
 * @param res_size         In:    The size of the output buffer.
 * @param p_res            Out:   The output buffer.
 * @param p_pos            InOut: Position in the output buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmrpc_lookup_request (
   pnet_t * net,
   const pf_rpc_header_t * p_rpc_req,
   const pf_rpc_lookup_req_t * p_lookup_req,
   pnet_result_t * p_read_status,
   uint16_t res_size,
   uint8_t * p_res,
   uint16_t * p_pos)
{
   LOG_INFO (
      PF_RPC_LOG,
      "EPM(%d): Received endpoint mapper (EPM) request.\n",
      __LINE__);

   pf_rpc_lookup_rsp_t lookup_rsp;

   memset (&lookup_rsp, 0, sizeof (lookup_rsp));
   lookup_rsp.rpc_entry.tower_entry.p_cfg = &net->fspm_cfg;
   lookup_rsp.rpc_entry.max_count = 1;
   lookup_rsp.return_code = PF_RPC_NOT_REGISTERED;

   if (p_lookup_req->max_entries > 0)
   {
      switch (p_lookup_req->inquiry_type)
      {
      case PF_RPC_INQUIRY_READ_ALL_REGISTERED_INTERFACES:
         pf_cmrdr_inquiry_read_all_reg_ind (
            net,
            p_rpc_req,
            p_lookup_req,
            &lookup_rsp,
            p_read_status);
         break;
      case PF_RPC_INQUIRY_READ_ALL_OBJECTS_FOR_ONE_INTERFACE:
      case PF_RPC_INQUIRY_READ_ALL_INTERFACES_INCLUDING_OBJECTS:
      case PF_RPC_INQUIRY_READ_ONE_INTERFACE_WITH_ONE_OBJECT:
         /* Support is optional and not implemented */
         /* FALLTHROUGH */
      default:
         /* Send PF_RPC_NOT_REGISTERED response */
         break;
      }
   }

   /* Write response buffer */
   pf_put_lookup_response_data (net, false, &lookup_rsp, res_size, p_res, p_pos);
   return 0;
}
