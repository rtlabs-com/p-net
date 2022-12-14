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
 * @brief Utility functions for interpreting data in buffers.
 *
 * For example uint16, protocol headers and protocol messages can be
 * read from the buffers.
 *
 * Most functions have (at least) an input buffer and a position in the buffer
 * as input arguments.
 *
 */

#ifdef UNIT_TEST

#endif

#include <inttypes.h>
#include <string.h>

#include "pf_includes.h"
#include "pf_block_reader.h"

void pf_get_mem (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   uint16_t dest_size,
   void * p_dest)
{
   if (p_info->result != PF_PARSE_OK)
   {
      /* Preserve first error */
   }
   else if (((*p_pos) + dest_size) > p_info->len)
   {
      /*
       * Reached end of buffer
       * If this is a chain of pbuff then goto the next pbuff,
       * else this should never happen.
       */
      LOG_DEBUG (PNET_LOG, "BR(%d): Unexpected end of input data\n", __LINE__);
      p_info->result = PF_PARSE_END_OF_INPUT;
   }
   else if (p_info->p_buf == NULL)
   {
      p_info->result = PF_PARSE_NULL_POINTER;
   }
   else
   {
      memcpy (p_dest, &p_info->p_buf[*p_pos], dest_size);
      (*p_pos) += dest_size;
   }
}

uint8_t pf_get_byte (pf_get_info_t * p_info, uint16_t * p_pos)
{
   uint8_t res = 0;

   if (p_info->result != PF_PARSE_OK)
   {
      /* Preserve first error */
   }
   else if (*p_pos >= p_info->len)
   {
      /* Reached end of buffer */
      LOG_DEBUG (
         PNET_LOG,
         "BR(%d): End of buffer reached when looking for a byte\n",
         __LINE__);
      p_info->result = PF_PARSE_END_OF_INPUT;
   }
   else if (p_info->p_buf == NULL)
   {
      p_info->result = PF_PARSE_NULL_POINTER;
   }
   else
   {
      res = p_info->p_buf[*p_pos];
      (*p_pos)++;
   }

   return res;
}

uint16_t pf_get_uint16 (pf_get_info_t * p_info, uint16_t * p_pos)
{
   uint16_t res = 0;

   if (p_info->result != PF_PARSE_OK)
   {
      /* Keep first error */
   }
   else if (p_info->is_big_endian)
   {
      res = ((uint16_t)pf_get_byte (p_info, p_pos) << 8) +
            (uint16_t)pf_get_byte (p_info, p_pos);
   }
   else
   {
      res = (uint16_t)pf_get_byte (p_info, p_pos) +
            ((uint16_t)pf_get_byte (p_info, p_pos) << 8);
   }

   return res;
}

uint32_t pf_get_uint32 (pf_get_info_t * p_info, uint16_t * p_pos)
{
   uint32_t res = 0;

   if (p_info->is_big_endian)
   {
      res = ((uint32_t)pf_get_byte (p_info, p_pos) << 24) +
            ((uint32_t)pf_get_byte (p_info, p_pos) << 16) +
            ((uint32_t)pf_get_byte (p_info, p_pos) << 8) +
            (uint32_t)pf_get_byte (p_info, p_pos);
   }
   else
   {
      res = (uint32_t)pf_get_byte (p_info, p_pos) +
            ((uint32_t)pf_get_byte (p_info, p_pos) << 8) +
            ((uint32_t)pf_get_byte (p_info, p_pos) << 16) +
            ((uint32_t)pf_get_byte (p_info, p_pos) << 24);
   }

   return res;
}

/**
 * @internal
 * Extract a UUID from a buffer.
 * @param p_info           InOut:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param p_dest           Out:  Destination buffer.
 */
static void pf_get_uuid (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_uuid_t * p_dest)
{
   p_dest->data1 = pf_get_uint32 (p_info, p_pos);
   p_dest->data2 = pf_get_uint16 (p_info, p_pos);
   p_dest->data3 = pf_get_uint16 (p_info, p_pos);
   pf_get_mem (p_info, p_pos, sizeof (p_dest->data4), p_dest->data4);
}

/**
 * @internal
 * Extract a rpc epm handle from a buffer.
 * @param p_info           InOut:The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param p_handle         Out:  Destination buffer.
 */
static void pf_get_handle (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_rpc_handle_t * p_handle)
{
   p_handle->rpc_entry_handle = pf_get_uint32 (p_info, p_pos);
   p_handle->handle_uuid.time_low = pf_get_uint32 (p_info, p_pos);
   p_handle->handle_uuid.time_mid = pf_get_uint16 (p_info, p_pos);
   p_handle->handle_uuid.time_hi_and_version = pf_get_uint16 (p_info, p_pos);
   p_handle->handle_uuid.clock_hi_and_reserved = pf_get_byte (p_info, p_pos);
   p_handle->handle_uuid.clock_low = pf_get_byte (p_info, p_pos);
   pf_get_mem (
      p_info,
      p_pos,
      sizeof (p_handle->handle_uuid.node),
      p_handle->handle_uuid.node);
}

/**
 * @internal
 * Extract bits from a uint32_t.
 * The extracted bits are placed at bit 0 in the returned uint32_t.
 *
 *                               A  B  C                      Input. pos = 6,
 * len = 3 Bit number  ... 12 11 10  9  8  7  6  5  4  3  2  1  0
 *
 *                  ... 0  0  0  0  0  0  0  0  0  A  B  C    Output
 *
 * @param bits             In:   The bit field.
 * @param pos              In:   First bit to extract.
 * @param len              In:   Number of bits to extract.
 * @return The extracted bits
 */
uint32_t pf_get_bits (uint32_t bits, uint8_t pos, uint8_t len)
{
   if (len == 0)
   {
      LOG_DEBUG (PNET_LOG, "BR(%d): len is 0 (zero)\n", __LINE__);
      return 0;
   }
   else if ((pos + len) > 32)
   {
      LOG_ERROR (PNET_LOG, "BR(%d): pos %u + len %u > 32\n", __LINE__, pos, len);
      return 0;
   }
   else if ((pos == 0) && (len == 32))
   {
      return bits;
   }
   else
   {

      return (bits >> pos) & (BIT (len) - 1u);
   }
}

/**
 * @internal
 * Extract a frame descriptor from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_fd             Out:   Destination buffer.
 */
static void pf_get_frame_descriptor (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_frame_descriptor_t * p_fd)
{
   p_fd->slot_number = pf_get_uint16 (p_info, p_pos);
   p_fd->subslot_number = pf_get_uint16 (p_info, p_pos);
   p_fd->frame_offset = pf_get_uint16 (p_info, p_pos);
}

/**
 * @internal
 * Extract an IOCR API entry from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_ae             Out:   Destination buffer.
 */
static void pf_get_iocr_api_entry (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_api_entry_t * p_ae)
{
   uint16_t ix;

   p_ae->api = pf_get_uint32 (p_info, p_pos);

   p_ae->nbr_io_data = pf_get_uint16 (p_info, p_pos);
   for (ix = 0; ix < p_ae->nbr_io_data; ix++)
   {
      pf_get_frame_descriptor (p_info, p_pos, &p_ae->io_data[ix]);
   }

   p_ae->nbr_iocs = pf_get_uint16 (p_info, p_pos);
   for (ix = 0; ix < p_ae->nbr_iocs; ix++)
   {
      pf_get_frame_descriptor (p_info, p_pos, &p_ae->iocs[ix]);
   }
}

/**
 * @internal
 * Extract an expected sub-module from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_sub            Out:   Destination buffer.
 */
static void pf_get_exp_submodule (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_exp_submodule_t * submodule)
{
   uint16_t temp_u16;

   submodule->subslot_number = pf_get_uint16 (p_info, p_pos);
   submodule->ident_number = pf_get_uint32 (p_info, p_pos);
   /* subslot_properties */
   temp_u16 = pf_get_uint16 (p_info, p_pos);
   submodule->properties.type = pf_get_bits (temp_u16, 0, 2);
   submodule->properties.sharedInput = (pf_get_bits (temp_u16, 2, 1) != 0);
   submodule->properties.reduce_input_submodule_data_length =
      (pf_get_bits (temp_u16, 3, 1) != 0);
   submodule->properties.reduce_output_submodule_data_length =
      (pf_get_bits (temp_u16, 4, 1) != 0);
   submodule->properties.discard_ioxs = (pf_get_bits (temp_u16, 5, 1) != 0);

   /* At least one submodule data descriptor */
   submodule->data_descriptor[0].data_direction = pf_get_uint16 (p_info, p_pos);
   submodule->data_descriptor[0].submodule_data_length =
      pf_get_uint16 (p_info, p_pos);
   submodule->data_descriptor[0].length_iocs = pf_get_byte (p_info, p_pos);
   submodule->data_descriptor[0].length_iops = pf_get_byte (p_info, p_pos);
   submodule->nbr_data_descriptors = 1;
   /* May have one more */
   if (submodule->properties.type == PNET_DIR_IO)
   {
      submodule->data_descriptor[1].data_direction =
         pf_get_uint16 (p_info, p_pos);
      submodule->data_descriptor[1].submodule_data_length =
         pf_get_uint16 (p_info, p_pos);
      submodule->data_descriptor[1].length_iocs = pf_get_byte (p_info, p_pos);
      submodule->data_descriptor[1].length_iops = pf_get_byte (p_info, p_pos);
      submodule->nbr_data_descriptors = 2;
   }

   LOG_DEBUG (
      PNET_LOG,
      "BR(%d):   Subslot 0x%04x. Expected submodule 0x%" PRIx32
      " with direction %s\n",
      __LINE__,
      submodule->subslot_number,
      submodule->ident_number,
      pf_cmdev_submod_dir_to_string (submodule->properties.type));
}

/* ======================== Public functions ======================== */

void pf_get_block_header (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_block_header_t * p_hdr)
{
   p_hdr->block_type = pf_get_uint16 (p_info, p_pos);
   p_hdr->block_length = pf_get_uint16 (p_info, p_pos);
   p_hdr->block_version_high = pf_get_byte (p_info, p_pos);
   p_hdr->block_version_low = pf_get_byte (p_info, p_pos);
}

void pf_get_ar_param (pf_get_info_t * p_info, uint16_t * p_pos, pf_ar_t * p_ar)
{
   uint32_t temp_u32;
   uint16_t str_len;

   p_ar->ar_param.ar_type = pf_get_uint16 (p_info, p_pos);
   pf_get_uuid (p_info, p_pos, &p_ar->ar_param.ar_uuid);
   p_ar->ar_param.session_key = pf_get_uint16 (p_info, p_pos);
   pf_get_mem (
      p_info,
      p_pos,
      sizeof (p_ar->ar_param.cm_initiator_mac_add),
      &p_ar->ar_param.cm_initiator_mac_add);
   pf_get_uuid (p_info, p_pos, &p_ar->ar_param.cm_initiator_object_uuid);
   /* ar_properties */
   temp_u32 = pf_get_uint32 (p_info, p_pos);
   p_ar->ar_param.ar_properties.state = pf_get_bits (temp_u32, 0, 3);
   p_ar->ar_param.ar_properties.supervisor_takeover_allowed =
      (pf_get_bits (temp_u32, 3, 1) != 0);
   p_ar->ar_param.ar_properties.parameterization_server =
      pf_get_bits (temp_u32, 4, 1);
   p_ar->ar_param.ar_properties.device_access =
      (pf_get_bits (temp_u32, 8, 1) != 0);
   p_ar->ar_param.ar_properties.companion_ar =
      (uint16_t)pf_get_bits (temp_u32, 9, 2);
   p_ar->ar_param.ar_properties.acknowledge_companion_ar =
      (pf_get_bits (temp_u32, 11, 1) != 0);
   p_ar->ar_param.ar_properties.combined_object_container =
      (pf_get_bits (temp_u32, 29, 1) != 0);
   p_ar->ar_param.ar_properties.startup_mode =
      (pf_get_bits (temp_u32, 30, 1) != 0);
   p_ar->ar_param.ar_properties.pull_module_alarm_allowed =
      (pf_get_bits (temp_u32, 31, 1) != 0);

   p_ar->ar_param.cm_initiator_activity_timeout_factor =
      pf_get_uint16 (p_info, p_pos);
   p_ar->ar_param.cm_initiator_udp_rt_port = pf_get_uint16 (p_info, p_pos);

   str_len = pf_get_uint16 (p_info, p_pos);
   p_ar->ar_param.cm_initiator_station_name_len = str_len;
   if (str_len > sizeof (p_ar->ar_param.cm_initiator_station_name) - 1)
   {
      str_len = sizeof (p_ar->ar_param.cm_initiator_station_name) - 1;
   }
   pf_get_mem (p_info, p_pos, str_len, p_ar->ar_param.cm_initiator_station_name);
   p_ar->ar_param.cm_initiator_station_name[str_len] = '\0';
}

void pf_get_iocr_param (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   uint16_t ix,
   pf_ar_t * p_ar)
{
   uint32_t temp_u32;
   uint16_t temp_u16;
   uint16_t iy;

   p_ar->iocrs[ix].param.iocr_type = pf_get_uint16 (p_info, p_pos);
   p_ar->iocrs[ix].param.iocr_reference = pf_get_uint16 (p_info, p_pos);
   p_ar->iocrs[ix].param.lt_field = pf_get_uint16 (p_info, p_pos);
   /* iocr_Properties */
   temp_u32 = pf_get_uint32 (p_info, p_pos);
   p_ar->iocrs[ix].param.iocr_properties.rt_class =
      pf_get_bits (temp_u32, 0, 4);
   p_ar->iocrs[ix].param.iocr_properties.reserved_1 =
      (pf_get_bits (temp_u32, 4, 9) != 0);
   p_ar->iocrs[ix].param.iocr_properties.reserved_2 =
      (pf_get_bits (temp_u32, 13, 11) != 0);
   p_ar->iocrs[ix].param.iocr_properties.reserved_3 =
      (pf_get_bits (temp_u32, 24, 8) != 0);

   p_ar->iocrs[ix].param.c_sdu_length = pf_get_uint16 (p_info, p_pos);
   p_ar->iocrs[ix].param.frame_id = pf_get_uint16 (p_info, p_pos);
   p_ar->iocrs[ix].param.send_clock_factor = pf_get_uint16 (p_info, p_pos);
   p_ar->iocrs[ix].param.reduction_ratio = pf_get_uint16 (p_info, p_pos);
   p_ar->iocrs[ix].param.phase = pf_get_uint16 (p_info, p_pos);
   p_ar->iocrs[ix].param.sequence = pf_get_uint16 (p_info, p_pos);
   p_ar->iocrs[ix].param.frame_send_offset = pf_get_uint32 (p_info, p_pos);
   p_ar->iocrs[ix].param.watchdog_factor = pf_get_uint16 (p_info, p_pos);
   p_ar->iocrs[ix].param.data_hold_factor = pf_get_uint16 (p_info, p_pos);
   /* iocr_tag_header */
   temp_u16 = pf_get_uint16 (p_info, p_pos);
   p_ar->iocrs[ix].param.iocr_tag_header.vlan_id =
      pf_get_bits (temp_u16, 0, 11);
   p_ar->iocrs[ix].param.iocr_tag_header.iocr_user_priority =
      pf_get_bits (temp_u16, 13, 3);

   pf_get_mem (
      p_info,
      p_pos,
      sizeof (p_ar->iocrs[ix].param.iocr_multicast_mac_add),
      &p_ar->iocrs[ix].param.iocr_multicast_mac_add);

   p_ar->iocrs[ix].param.nbr_apis = pf_get_uint16 (p_info, p_pos);
   for (iy = 0; iy < p_ar->iocrs[ix].param.nbr_apis; iy++)
   {
      pf_get_iocr_api_entry (p_info, p_pos, &p_ar->iocrs[ix].param.apis[iy]);
   }
}

void pf_get_exp_api_module (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_ar_t * p_ar)
{
   uint16_t ix;
   uint16_t iy;
   uint32_t exp_api;
   uint16_t slot_number;
   pf_exp_api_t * api;
   pf_exp_api_t * this_api;
   pf_exp_module_t * module;
   uint16_t nbr_exp_api;

   api = NULL;
   module = NULL;
   nbr_exp_api = pf_get_uint16 (p_info, p_pos); /* In this block */

   for (ix = 0; ix < nbr_exp_api; ix++)
   {
      /* Get one module description */
      exp_api = pf_get_uint32 (p_info, p_pos);

      /* Find the API if we are augmenting it */
      for (iy = 0; iy < p_ar->exp_ident.nbr_apis; iy++)
      {
         this_api = &p_ar->exp_ident.api[iy];
         if ((this_api->valid == true) && (this_api->api == exp_api))
         {
            api = this_api;
         }
      }

      /* If this is the first mention of this API */
      if ((api == NULL) && (p_ar->exp_ident.nbr_apis < PNET_MAX_API))
      {
         /* Allocate a new API */
         api = &p_ar->exp_ident.api[p_ar->exp_ident.nbr_apis];
         p_ar->exp_ident.nbr_apis++;

         api->api = exp_api;
         api->nbr_modules = 0;
         api->nbr_diff_modules = 0;
      }

      if (api == NULL)
      {
         /* This error condition is reported by caller. */
         p_info->result = PF_PARSE_OUT_OF_API_RESOURCES;
         LOG_DEBUG (
            PNET_LOG,
            "BR(%d): Out of expected API resources\n",
            __LINE__);
      }
      else
      {
         slot_number = pf_get_uint16 (p_info, p_pos);

         /* Get a new module. */
         if (api->nbr_modules < PNET_MAX_SLOTS)
         {
            module = &api->module[api->nbr_modules];
            api->nbr_modules++;

            module->slot_number = slot_number;
            module->ident_number = pf_get_uint32 (p_info, p_pos);
            module->properties = pf_get_uint16 (p_info, p_pos);
            module->nbr_submodules = pf_get_uint16 (p_info, p_pos);

            LOG_DEBUG (
               PNET_LOG,
               "BR(%d): Slot %u. Expected module 0x%" PRIx32 " with %u "
               "submodules.\n",
               __LINE__,
               module->slot_number,
               module->ident_number,
               module->nbr_submodules);

            for (iy = 0; iy < module->nbr_submodules; iy++)
            {
               pf_get_exp_submodule (p_info, p_pos, &module->submodule[iy]);
            }
            api->valid = true;
         }
         else
         {
            /* This error condition is reported by caller. */
            p_info->result = PF_PARSE_OUT_OF_EXP_SUBMODULE_RESOURCES;
            LOG_ERROR (
               PNET_LOG,
               "BR(%d): Too many modules used (not enough slots). Out of "
               "expected module resources.\n",
               __LINE__);
         }
      }
   }
}

void pf_get_alarm_cr_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_ar_t * p_ar)
{
   uint32_t temp_u32;
   uint16_t temp_u16;

   p_ar->alarm_cr_request.alarm_cr_type = pf_get_uint16 (p_info, p_pos);
   p_ar->alarm_cr_request.lt_field = pf_get_uint16 (p_info, p_pos);
   /* alarm_cr_properties */
   temp_u32 = pf_get_uint32 (p_info, p_pos);
   p_ar->alarm_cr_request.alarm_cr_properties.priority =
      (pf_get_bits (temp_u32, 0, 1) != 0);
   p_ar->alarm_cr_request.alarm_cr_properties.transport_udp =
      (pf_get_bits (temp_u32, 1, 1) != 0);

   p_ar->alarm_cr_request.rta_timeout_factor = pf_get_uint16 (p_info, p_pos);
   p_ar->alarm_cr_request.rta_retries = pf_get_uint16 (p_info, p_pos);
   p_ar->alarm_cr_request.local_alarm_reference = pf_get_uint16 (p_info, p_pos);
   p_ar->alarm_cr_request.max_alarm_data_length = pf_get_uint16 (p_info, p_pos);

   temp_u16 = pf_get_uint16 (p_info, p_pos);
   p_ar->alarm_cr_request.alarm_cr_tag_header_high.vlan_id =
      pf_get_bits (temp_u16, 0, 12);
   p_ar->alarm_cr_request.alarm_cr_tag_header_high.alarm_user_priority =
      pf_get_bits (temp_u16, 13, 3);

   temp_u16 = pf_get_uint16 (p_info, p_pos);
   p_ar->alarm_cr_request.alarm_cr_tag_header_low.vlan_id =
      pf_get_bits (temp_u16, 0, 12);
   p_ar->alarm_cr_request.alarm_cr_tag_header_low.alarm_user_priority =
      pf_get_bits (temp_u16, 13, 3);
}

#if PNET_OPTION_PARAMETER_SERVER
void pf_get_ar_prm_server_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_ar_t * p_ar)
{
   pf_get_uuid (p_info, p_pos, &p_ar->prm_server.parameter_server_object_uuid);
   /* parameter_server_properties - not used */
   (void)pf_get_uint32 (p_info, p_pos);

   p_ar->prm_server.cm_initiator_activity_timeout_factor =
      pf_get_uint16 (p_info, p_pos);
   p_ar->prm_server.parameter_server_station_name_len =
      pf_get_uint16 (p_info, p_pos);
   pf_get_mem (
      p_info,
      p_pos,
      p_ar->prm_server.parameter_server_station_name_len,
      p_ar->prm_server.parameter_server_station_name);
   p_ar->prm_server.parameter_server_station_name
      [p_ar->prm_server.parameter_server_station_name_len] = '\0';
}
#endif

#if PNET_OPTION_IR
/**
 * Extract an DFP IOCR block from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_dfp_iocr       Out:   Contains the destination structure.
 */
static void pf_get_dfp_iocr (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_dfp_iocr_t * p_dfp_iocr)
{
   p_dfp_iocr->iocr_reference = pf_get_uint16 (p_info, p_pos);
   p_dfp_iocr->subframe_offset = pf_get_uint16 (p_info, p_pos);

   p_dfp_iocr->subframe_data.subframe_position = pf_get_byte (p_info, p_pos);
   p_dfp_iocr->subframe_data.subframe_length = pf_get_byte (p_info, p_pos);
}

void pf_get_ir_info_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_ar_t * p_ar)
{
   uint16_t ix;

   pf_get_uuid (p_info, p_pos, &p_ar->ir_info.ir_data_uuid);
   p_ar->ir_info.nbr_iocrs = pf_get_uint16 (p_info, p_pos);

   for (ix = 0; ix < p_ar->ir_info.nbr_iocrs; ix++)
   {
      pf_get_dfp_iocr (p_info, p_pos, &p_ar->ir_info.dfp_iocrs[ix]);
   }
}
#endif

#if PNET_OPTION_MC_CR
void pf_get_mcr_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   uint16_t ix,
   pf_ar_t * p_ar)
{
   p_ar->mcr_request[ix].iocr_reference = pf_get_uint16 (p_info, p_pos);
   p_ar->mcr_request[ix].address_resolution_properties.protocol =
      pf_get_byte (p_info, p_pos);
   p_ar->mcr_request[ix].address_resolution_properties.resolution_factor =
      pf_get_byte (p_info, p_pos);
   p_ar->mcr_request[ix].mci_timeout_factor = pf_get_uint16 (p_info, p_pos);
   p_ar->mcr_request[ix].length_provider_station_name =
      pf_get_uint16 (p_info, p_pos);

   pf_get_mem (
      p_info,
      p_pos,
      p_ar->mcr_request[ix].length_provider_station_name,
      p_ar->mcr_request[ix].provider_station_name);
   p_ar->mcr_request[ix]
      .provider_station_name[p_ar->mcr_request[ix].length_provider_station_name] =
      '\0';
}
#endif

void pf_get_ar_rpc_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_ar_t * p_ar)
{
   p_ar->ar_rpc_request.initiator_rpc_server_port =
      pf_get_uint16 (p_info, p_pos);
}

#if PNET_OPTION_AR_VENDOR_BLOCKS
void pf_get_ar_vendor_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   uint16_t ix,
   pf_ar_t * p_ar)
{
   p_ar->ar_vendor_request[ix].ap_structure_identifier =
      pf_get_uint32 (p_info, p_pos);
   p_ar->ar_vendor_request[ix].api = pf_get_uint32 (p_info, p_pos);
   p_ar->ar_vendor_request[ix].length_data_request =
      pf_get_uint16 (p_info, p_pos);
   pf_get_mem (
      p_info,
      p_pos,
      p_ar->ar_vendor_request[ix].length_data_request,
      p_ar->ar_vendor_request[ix].data_request);
}
#endif

void pf_get_control (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_control_block_t * p_req)
{
   /* 2 padding bytes */
   (void)pf_get_uint16 (p_info, p_pos);
   pf_get_uuid (p_info, p_pos, &p_req->ar_uuid);
   p_req->session_key = pf_get_uint16 (p_info, p_pos);

   p_req->alarm_sequence_number = pf_get_uint16 (p_info, p_pos);

   /* Command and properties are always Big-Endian on the wire!! */
   p_req->control_command = pf_get_uint16 (p_info, p_pos);
   p_req->control_block_properties = pf_get_uint16 (p_info, p_pos);
}

void pf_get_ndr_data (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_ndr_data_t * p_ndr)
{
   p_ndr->args_maximum = pf_get_uint32 (p_info, p_pos);
   p_ndr->args_length = pf_get_uint32 (p_info, p_pos);
   p_ndr->array.maximum_count = pf_get_uint32 (p_info, p_pos);
   p_ndr->array.offset = pf_get_uint32 (p_info, p_pos);
   p_ndr->array.actual_count = pf_get_uint32 (p_info, p_pos);
}

void pf_get_dce_rpc_header (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_rpc_header_t * p_rpc)
{
   uint8_t temp_uint8;

   p_rpc->version = pf_get_byte (p_info, p_pos);
   p_rpc->packet_type = pf_get_byte (p_info, p_pos) & 0x1f; /* Only 5 LSB
                                                               according to spec
                                                             */

   /* flags */
   temp_uint8 = pf_get_byte (p_info, p_pos);
   p_rpc->flags.last_fragment =
      pf_get_bits (temp_uint8, PF_RPC_F_LAST_FRAGMENT, 1);
   p_rpc->flags.fragment = pf_get_bits (temp_uint8, PF_RPC_F_FRAGMENT, 1);
   p_rpc->flags.no_fack = pf_get_bits (temp_uint8, PF_RPC_F_NO_FACK, 1);
   p_rpc->flags.maybe = pf_get_bits (temp_uint8, PF_RPC_F_MAYBE, 1);
   p_rpc->flags.idempotent = pf_get_bits (temp_uint8, PF_RPC_F_IDEMPOTENT, 1);
   p_rpc->flags.broadcast = pf_get_bits (temp_uint8, PF_RPC_F_BROADCAST, 1);

   /* flags2 */
   temp_uint8 = pf_get_byte (p_info, p_pos);
   p_rpc->flags2.cancel_pending =
      pf_get_bits (temp_uint8, PF_RPC_F2_CANCEL_PENDING, 1);

   /* Data repr */
   temp_uint8 = pf_get_byte (p_info, p_pos);
   p_rpc->is_big_endian = (pf_get_bits (temp_uint8, 4, 4) == 0);
   p_info->is_big_endian = p_rpc->is_big_endian;

   /* Float repr  - Assume IEEE */
   (void)pf_get_byte (p_info, p_pos);
   p_rpc->float_repr = 0;

   /* Reserved */
   p_rpc->reserved = pf_get_byte (p_info, p_pos);

   p_rpc->serial_high = pf_get_byte (p_info, p_pos);
   pf_get_uuid (p_info, p_pos, &p_rpc->object_uuid);
   pf_get_uuid (p_info, p_pos, &p_rpc->interface_uuid);
   pf_get_uuid (p_info, p_pos, &p_rpc->activity_uuid);
   p_rpc->server_boot_time = pf_get_uint32 (p_info, p_pos);
   p_rpc->interface_version = pf_get_uint32 (p_info, p_pos);
   p_rpc->sequence_nmb = pf_get_uint32 (p_info, p_pos);
   p_rpc->opnum = pf_get_uint16 (p_info, p_pos);
   p_rpc->interface_hint = pf_get_uint16 (p_info, p_pos);
   p_rpc->activity_hint = pf_get_uint16 (p_info, p_pos);
   p_rpc->length_of_body = pf_get_uint16 (p_info, p_pos);
   p_rpc->fragment_nmb = pf_get_uint16 (p_info, p_pos);
   p_rpc->auth_protocol = pf_get_byte (p_info, p_pos);
   p_rpc->serial_low = pf_get_byte (p_info, p_pos);
}

void pf_get_read_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_iod_read_request_t * p_req)
{
   uint16_t ix;

   p_req->sequence_number = pf_get_uint16 (p_info, p_pos);
   pf_get_uuid (p_info, p_pos, &p_req->ar_uuid);
   p_req->api = pf_get_uint32 (p_info, p_pos);
   p_req->slot_number = pf_get_uint16 (p_info, p_pos);
   p_req->subslot_number = pf_get_uint16 (p_info, p_pos);
   p_req->padding[0] = pf_get_byte (p_info, p_pos);
   p_req->padding[1] = pf_get_byte (p_info, p_pos);
   p_req->index = pf_get_uint16 (p_info, p_pos);
   p_req->record_data_length = pf_get_uint32 (p_info, p_pos);
   pf_get_uuid (p_info, p_pos, &p_req->target_ar_uuid);
   for (ix = 0; ix < NELEMENTS (p_req->rw_padding); ix++)
   {
      p_req->rw_padding[ix] = pf_get_byte (p_info, p_pos);
   }
}

void pf_get_epm_lookup_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_rpc_lookup_req_t * p_req)
{
   p_req->inquiry_type = pf_get_uint32 (p_info, p_pos);
   p_req->object_id = pf_get_uint32 (p_info, p_pos);
   pf_get_uuid (p_info, p_pos, &p_req->object_uuid);
   p_req->interface_id = pf_get_uint32 (p_info, p_pos);
   pf_get_uuid (p_info, p_pos, &p_req->interface_uuid);
   p_req->interface_ver_major = pf_get_uint16 (p_info, p_pos);
   p_req->interface_ver_minor = pf_get_uint16 (p_info, p_pos);
   p_req->version_option = pf_get_uint32 (p_info, p_pos);
   pf_get_handle (p_info, p_pos, &p_req->rpc_handle);
   p_req->max_entries = pf_get_uint32 (p_info, p_pos);
}

void pf_get_write_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_iod_write_request_t * p_req)
{
   uint16_t ix;

   p_req->sequence_number = pf_get_uint16 (p_info, p_pos);
   pf_get_uuid (p_info, p_pos, &p_req->ar_uuid);
   p_req->api = pf_get_uint32 (p_info, p_pos);
   p_req->slot_number = pf_get_uint16 (p_info, p_pos);
   p_req->subslot_number = pf_get_uint16 (p_info, p_pos);
   p_req->padding[0] = pf_get_byte (p_info, p_pos);
   p_req->padding[1] = pf_get_byte (p_info, p_pos);
   p_req->index = pf_get_uint16 (p_info, p_pos);
   p_req->record_data_length = pf_get_uint32 (p_info, p_pos);
   for (ix = 0; ix < NELEMENTS (p_req->rw_padding); ix++)
   {
      p_req->rw_padding[ix] = pf_get_byte (p_info, p_pos);
   }
}

void pf_get_im_1 (pf_get_info_t * p_info, uint16_t * p_pos, pnet_im_1_t * p_im_1)
{
   pf_get_mem (
      p_info,
      p_pos,
      sizeof (p_im_1->im_tag_function) - 1,
      p_im_1->im_tag_function);
   p_im_1->im_tag_function[sizeof (p_im_1->im_tag_function) - 1] = '\0';
   pf_get_mem (
      p_info,
      p_pos,
      sizeof (p_im_1->im_tag_location) - 1,
      p_im_1->im_tag_location);
   p_im_1->im_tag_location[sizeof (p_im_1->im_tag_location) - 1] = '\0';
}

void pf_get_alarm_fixed (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_alarm_fixed_t * p_alarm_fixed)
{
   uint32_t temp_u32;

   /* The fixed part is not a "block" so do not expect a block header */
   p_alarm_fixed->dst_ref = pf_get_uint16 (p_info, p_pos);
   p_alarm_fixed->src_ref = pf_get_uint16 (p_info, p_pos);

   temp_u32 = pf_get_byte (p_info, p_pos);
   p_alarm_fixed->pdu_type.type = pf_get_bits (temp_u32, 0, 4);
   p_alarm_fixed->pdu_type.version = pf_get_bits (temp_u32, 4, 4);

   temp_u32 = pf_get_byte (p_info, p_pos);
   p_alarm_fixed->add_flags.window_size = pf_get_bits (temp_u32, 0, 4);
   p_alarm_fixed->add_flags.tack = pf_get_bits (temp_u32, 4, 1);

   p_alarm_fixed->send_seq_num = pf_get_uint16 (p_info, p_pos);
   p_alarm_fixed->ack_seq_nbr = pf_get_uint16 (p_info, p_pos);
}

void pf_get_alarm_data (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_alarm_data_t * p_alarm_data)
{
   uint16_t temp_u16;

   p_alarm_data->alarm_type = pf_get_uint16 (p_info, p_pos);
   p_alarm_data->api_id = pf_get_uint32 (p_info, p_pos);
   p_alarm_data->slot_nbr = pf_get_uint16 (p_info, p_pos);
   p_alarm_data->subslot_nbr = pf_get_uint16 (p_info, p_pos);
   p_alarm_data->module_ident = pf_get_uint32 (p_info, p_pos);
   p_alarm_data->submodule_ident = pf_get_uint32 (p_info, p_pos);

   /* AlarmSpecifier */
   temp_u16 = pf_get_uint16 (p_info, p_pos);
   p_alarm_data->sequence_number = pf_get_bits (temp_u16, 0, 11);
   p_alarm_data->alarm_specifier.channel_diagnosis =
      pf_get_bits (temp_u16, 11, 1);
   p_alarm_data->alarm_specifier.manufacturer_diagnosis =
      pf_get_bits (temp_u16, 12, 1);
   p_alarm_data->alarm_specifier.submodule_diagnosis =
      pf_get_bits (temp_u16, 13, 1);
   p_alarm_data->alarm_specifier.ar_diagnosis = pf_get_bits (temp_u16, 15, 1);
}

void pf_get_alarm_ack (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_alarm_data_t * p_alarm_data)
{
   uint16_t temp_u16;

   p_alarm_data->alarm_type = pf_get_uint16 (p_info, p_pos);
   p_alarm_data->api_id = pf_get_uint32 (p_info, p_pos);
   p_alarm_data->slot_nbr = pf_get_uint16 (p_info, p_pos);
   p_alarm_data->subslot_nbr = pf_get_uint16 (p_info, p_pos);

   /* AlarmSpecifier */
   temp_u16 = pf_get_uint16 (p_info, p_pos);
   p_alarm_data->sequence_number = pf_get_bits (temp_u16, 0, 11);
   p_alarm_data->alarm_specifier.channel_diagnosis =
      pf_get_bits (temp_u16, 11, 1);
   p_alarm_data->alarm_specifier.manufacturer_diagnosis =
      pf_get_bits (temp_u16, 12, 1);
   p_alarm_data->alarm_specifier.submodule_diagnosis =
      pf_get_bits (temp_u16, 13, 1);
   p_alarm_data->alarm_specifier.ar_diagnosis = pf_get_bits (temp_u16, 15, 1);
}

void pf_get_pnio_status (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pnet_pnio_status_t * p_status)
{
   p_status->error_code = pf_get_byte (p_info, p_pos);
   p_status->error_decode = pf_get_byte (p_info, p_pos);
   p_status->error_code_1 = pf_get_byte (p_info, p_pos);
   p_status->error_code_2 = pf_get_byte (p_info, p_pos);
}

void pf_get_port_data_check (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_port_data_check_t * p_port_data_check)
{
   uint8_t dummy[4];

   pf_get_mem (p_info, p_pos, 2, dummy); /* Padding */
   p_port_data_check->slot_number = pf_get_uint16 (p_info, p_pos);
   p_port_data_check->subslot_number = pf_get_uint16 (p_info, p_pos);
   pf_get_block_header (p_info, p_pos, &p_port_data_check->block_header);
}

/**
 * Extract CheckPeer data block from buffer.
 *
 * @param p_info           InOut: The parser information.
 * @param p_pos            InOut: The current parsing position.
 * @param p_check_peer:    Out:   Destination structure
 */
static void get_check_peer (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_check_peer_t * p_check_peer)
{
   /* TODO: Validate length_peer_port_name and length_peer_station_name */

   p_check_peer->length_peer_port_name = pf_get_byte (p_info, p_pos);

   pf_get_mem (
      p_info,
      p_pos,
      p_check_peer->length_peer_port_name,
      p_check_peer->peer_port_name);
   p_check_peer->peer_port_name[p_check_peer->length_peer_port_name] = '\0';

   p_check_peer->length_peer_station_name = pf_get_byte (p_info, p_pos);

   pf_get_mem (
      p_info,
      p_pos,
      p_check_peer->length_peer_station_name,
      p_check_peer->peer_station_name);
   p_check_peer->peer_station_name[p_check_peer->length_peer_station_name] =
      '\0';
}

void pf_get_port_data_check_check_peers (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   uint8_t max_peers,
   pf_check_peers_t * p_check_peers)
{
   uint8_t i;

   p_check_peers->number_of_peers = pf_get_byte (p_info, p_pos);
   if (p_check_peers->number_of_peers > max_peers)
   {
      LOG_ERROR (
         PNET_LOG,
         "BR(%d): Unsupported number of check peers\n",
         __LINE__);
      p_check_peers->number_of_peers = max_peers;
   }

   for (i = 0; i < p_check_peers->number_of_peers; i++)
   {
      get_check_peer (p_info, p_pos, &p_check_peers->peers[i]);
   }
}

void pf_get_port_data_adjust (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_port_data_adjust_t * p_port_data_adjust)
{
   uint8_t dummy[4];

   pf_get_mem (p_info, p_pos, 2, dummy); /* Padding */
   p_port_data_adjust->slot_number = pf_get_uint16 (p_info, p_pos);
   p_port_data_adjust->subslot_number = pf_get_uint16 (p_info, p_pos);
   pf_get_block_header (p_info, p_pos, &p_port_data_adjust->block_header);
}

void pf_get_port_data_adjust_peer_to_peer_boundary (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_adjust_peer_to_peer_boundary_t * p_boundary)
{
   uint8_t dummy[4];
   uint32_t temp;

   memset (p_boundary, 0, sizeof (*p_boundary));

   pf_get_mem (p_info, p_pos, 2, dummy); /* Padding */

   temp = pf_get_uint32 (p_info, p_pos);
   p_boundary->peer_to_peer_boundary.do_not_send_LLDP_frames =
      pf_get_bits (temp, 0, 1);
   p_boundary->peer_to_peer_boundary.do_not_send_pctp_delay_request_frames =
      pf_get_bits (temp, 1, 1);
   p_boundary->peer_to_peer_boundary.do_not_send_path_delay_request_frames =
      pf_get_bits (temp, 2, 1);

   p_boundary->adjust_properties = pf_get_uint16 (p_info, p_pos);
}

void pf_get_interface_adjust (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_lldp_name_of_device_mode_t * name_of_device_mode)
{
   uint8_t dummy[2];
   uint32_t temp;

   pf_get_mem (p_info, p_pos, 2, dummy); /* Padding */

   temp = pf_get_uint32 (p_info, p_pos);
   *name_of_device_mode = pf_get_bits (temp, 0, 1);
}
