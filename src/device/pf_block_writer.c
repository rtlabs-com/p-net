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
 * @brief Utility functions for writing data into buffers.
 *
 * For example uint16, time stamps and protocol headers can be written to the
 * buffers.
 *
 * Most functions have (at least) a buffer and a position in the buffer
 * as input arguments.
 *
 * Try to keep this file as clean as possible:
 *  - Avoid calling functions from other files
 *  - Pass data to be written as arguments (avoid passing in pnet_t * net)
 *
 * The functions should in general look like:
 *
 *    void pf_put_whatever (
 *       bool is_big_endian,
 *       const whatever * p_whatever,
 *       pf_put_info_t * p_info,
 *       uint16_t * p_pos);
 */

#ifdef UNIT_TEST

#endif

#include <string.h>

#include "pf_includes.h"
#include "pf_block_writer.h"

#define STRINGIFY(s)   STRINGIFIED (s)
#define STRINGIFIED(s) #s

/**
 * @internal
 * Insert a block header into a buffer.
 * @param is_big_endian    In:    true if buffer is big-endian.
 * @param bh_type          In:    Block type.
 * @param bh_length        In:    Block length.
 * @param bh_ver_high      In:    Block version high.
 * @param bh_ver_low       In:    Block version low.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_block_header (
   bool is_big_endian,
   pf_block_type_values_t bh_type,
   uint16_t bh_length,
   uint8_t bh_ver_high,
   uint8_t bh_ver_low,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t temp_u16 = (uint16_t)bh_type;

   pf_put_uint16 (is_big_endian, temp_u16, p_info, p_pos);
   pf_put_uint16 (
      is_big_endian,
      (bh_length + sizeof (pf_block_header_t)) - 4,
      p_info,
      p_pos); /* Always (-4)!! */
   pf_put_byte (bh_ver_high, p_info, p_pos);
   pf_put_byte (bh_ver_low, p_info, p_pos);
}

/**
 * @internal
 * Insert a string into a buffer.
 *
 * This function inserts a fixed sized string into a buffer.
 * If the string is shorter than the destination size then the destination
 * is padded with spaces.
 * The destination buffer does not contain any terminating NUL byte.
 * @param p_src            In:    The string (NUL-terminated).
 * @param src_size         In:    The destination string size (includes NUL-byte).
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_str (
   const void * p_src,
   uint16_t src_size,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   if (p_info->result != PF_PUT_OK)
   {
      /* Preserve first error */
      return;
   }

   uint16_t str_len = (uint16_t)strlen (p_src);

   if (str_len >= src_size)
   {
      /* Too large input */
      LOG_DEBUG (PNET_LOG,
         "BW(%d): String is too long (%u chars, but max is %u)\n",
         __LINE__,
         str_len,
         src_size - 1);
      p_info->result = PF_PUT_TOO_LONG_STRING;
      return;
   }

   pf_put_mem (p_src, str_len, p_info, p_pos);

   /* Pad with spaces - src_size also includes NUL-terminator. Do not write into
    * that pos. */
   pf_put_padding_bytes (' ', src_size - str_len - 1, p_info, p_pos);
}

/**
 * @internal
 * Insert a UUID into a buffer.
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_uuid           In:    The UUID to insert.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_uuid (
   bool is_big_endian,
   const pf_uuid_t * p_uuid,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_uint32 (is_big_endian, p_uuid->time_low, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_uuid->time_mid, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_uuid->time_hi_and_version, p_info, p_pos);
   pf_put_byte (p_uuid->clock_seq_hi_and_res, p_info, p_pos);
   pf_put_byte (p_uuid->clock_seq_low, p_info, p_pos);
   pf_put_mem (p_uuid->node, sizeof (p_uuid->node), p_info, p_pos);
}
/* ======================== Public functions */

void pf_put_mem (
   const void * p_src,
   uint16_t src_size, /* Bytes to copy */
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   if (p_info->result != PF_PUT_OK)
   {
      /* Preserve first error */
      return;
   }

   if (((*p_pos) + src_size) > p_info->len)
   {
      /* Reached end of buffer */
      LOG_DEBUG (PNET_LOG, "BW(%d): Output buffer is full\n", __LINE__);
      p_info->result = PF_PUT_END_OF_OUTPUT;
      return;
   }

   if (p_info->p_array != NULL)
   {
      CC_ASSERT (p_info->p_frame == NULL);

      memcpy (&p_info->p_array[*p_pos], p_src, src_size);
   }
   else
   {
      pnal_buf_write (p_info->p_frame, *p_pos, p_src, src_size);
   }
   (*p_pos) += src_size;
}

void pf_put_mem_overlapping (
   const void * p_src,
   uint16_t src_size, /* Bytes to copy */
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   if (p_info->result != PF_PUT_OK)
   {
      /* Preserve first error */
      return;
   }

   if (((*p_pos) + src_size) > p_info->len)
   {
      /* Reached end of buffer */
      LOG_DEBUG (PNET_LOG, "BW(%d): Output buffer is full\n", __LINE__);
      p_info->result = PF_PUT_END_OF_OUTPUT;
      return;
   }

   if (p_info->p_array != NULL)
   {
      CC_ASSERT (p_info->p_frame == NULL);

      memmove (&p_info->p_array[*p_pos], p_src, src_size);
   }
   else
   {
      /* Frame buffers may never overlap with other buffers */
      pnal_buf_write (p_info->p_frame, *p_pos, p_src, src_size);
   }
   (*p_pos) += src_size;
}

void pf_put_padding_bytes (
   uint8_t value,
   uint16_t n_bytes,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   if (p_info->result != PF_PUT_OK)
   {
      /* Preserve first error */
      return;
   }

   if (((*p_pos) + n_bytes) > p_info->len)
   {
      /* Reached end of buffer */
      LOG_DEBUG (PNET_LOG, "BW(%d): Output buffer is full\n", __LINE__);
      p_info->result = PF_PUT_END_OF_OUTPUT;
      return;
   }

   if (p_info->p_array != NULL)
   {
      CC_ASSERT (p_info->p_frame == NULL);

      memset (&p_info->p_array[*p_pos], value, n_bytes);
   }
   else
   {
      pnal_buf_fill (p_info->p_frame, *p_pos, value, n_bytes);
   }
   (*p_pos) += n_bytes;
}

void pf_put_byte (
   uint8_t val,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_mem (&val, sizeof (val), p_info, p_pos);
}

void pf_put_uint16 (
   bool is_big_endian,
   uint16_t val,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   if (is_big_endian)
   {
      val = CC_TO_BE16 (val);
   }
   else
   {
      val = CC_TO_LE16 (val);
   }
   pf_put_mem (&val, sizeof (val), p_info, p_pos);
}

void pf_put_uint32 (
   bool is_big_endian,
   uint32_t val,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   if (is_big_endian)
   {
      val = CC_TO_BE32 (val);
   }
   else
   {
      val = CC_TO_LE32 (val);
   }
   pf_put_mem (&val, sizeof (val), p_info, p_pos);
}

void pf_put_padding (
   uint16_t n_bytes,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_padding_bytes (0x00, n_bytes, p_info, p_pos);
}

void pf_put_padding_align (
   uint16_t start_position,
   uint16_t align,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   /* Cast to unsigned to ensure the result is within 0 <= n < align bytes */
   uint16_t n_bytes = (uint16_t)(align - ((*p_pos) - start_position)) % align;
   pf_put_padding_bytes (0x00, n_bytes, p_info, p_pos);
}

void pf_put_mac_address (
   const pnet_ethaddr_t * p_address,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_mem (p_address, sizeof (*p_address), p_info, p_pos);
}

void pf_put_ethernet_header (
   const pf_ethernet_header_t * p_header,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_mac_address (&p_header->dest, p_info, p_pos);
   pf_put_mac_address (&p_header->src, p_info, p_pos);
   pf_put_uint16 (true, p_header->type, p_info, p_pos);
}

/**
 * @internal
 * Insert timestamp
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_time_ts        In:    The timestamp
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
void pf_put_time_timestamp (
   bool is_big_endian,
   const pf_log_book_ts_t * p_time_ts,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_uint16 (is_big_endian, p_time_ts->sec_hi, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_time_ts->sec_lo, p_info, p_pos);
   pf_put_uint32 (is_big_endian, p_time_ts->nano_sec, p_info, p_pos);
}

void pf_put_ar_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   pf_put_block_header (
      is_big_endian,
      PF_BT_AR_BLOCK_RES,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_param.ar_type,
      p_info,
      p_pos);
   pf_put_uuid (is_big_endian, &p_ar->ar_param.ar_uuid, p_info, p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_param.session_key,
      p_info,
      p_pos);

   pf_put_mem (
      &p_ar->ar_result.cm_responder_mac_add,
      sizeof (p_ar->ar_result.cm_responder_mac_add),
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_result.responder_udp_rt_port,
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_iocr_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t ix,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   pf_put_block_header (
      is_big_endian,
      PF_BT_IOCR_BLOCK_RES,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->io_ar->iocrs[ix].result.iocr_type,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->io_ar->iocrs[ix].result.iocr_reference,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->io_ar->iocrs[ix].result.frame_id,
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_alarm_cr_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   pf_put_block_header (
      is_big_endian,
      PF_BT_ALARM_CR_BLOCK_RES,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->io_ar->alarm_cr_result.alarm_cr_type,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->io_ar->alarm_cr_result.local_alarm_reference,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->io_ar->alarm_cr_result.max_alarm_data_length,
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

/**
 * @internal
 * Insert bits into a uint32_t.
 * @param val              In:    The value of the bit field.
 * @param len              In:    The number of bits to insert.
 * @param pos              In:    The position in p_bits to insert the bits at.
 * @param p_bits           InOut: The resulting uint32_t.
 */
static void pf_put_bits (
   uint32_t val,
   uint16_t len,
   uint16_t pos,
   uint32_t * p_bits)
{
   /* Take (len) least significant bits from val and insert them into p_bits at
    * the correct position. */
   (*p_bits) |= (val & ((1u << len) - 1)) << pos;
}

/**
 * @internal
 * Insert a sub-module diff into a buffer.
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_diff           In:    The sub-module diff information.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_submodule_diff (
   bool is_big_endian,
   pf_exp_submodule_t const * submodule,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint32_t temp_u32 = 0;
   uint16_t temp_u16 = 0;

   pf_put_uint16 (
      is_big_endian,
      submodule->subslot_number,
      p_info,
      p_pos);

   if (submodule->state.ident_info == PF_IDENT_INFO_NO)
   {
      pf_put_uint32 (is_big_endian, 0, p_info, p_pos);
   }
   else
   {
      pf_put_uint32 (
         is_big_endian,
         submodule->ident_number,
         p_info,
         p_pos);
   }
   /* submodule_state */
   pf_put_bits (submodule->state.add_info, 3, 0, &temp_u32);
   pf_put_bits (submodule->state.advice_avail, 1, 3, &temp_u32);
   pf_put_bits (submodule->state.maintenance_required, 1, 4, &temp_u32);
   pf_put_bits (submodule->state.maintenance_demanded, 1, 5, &temp_u32);
   pf_put_bits (submodule->state.fault, 1, 6, &temp_u32);
   pf_put_bits (submodule->state.ar_info, 4, 7, &temp_u32);
   pf_put_bits (submodule->state.ident_info, 4, 11, &temp_u32);
   pf_put_bits (submodule->state.format_indicator, 1, 15, &temp_u32);
   temp_u16 = (uint16_t)temp_u32;
   pf_put_uint16 (is_big_endian, temp_u16, p_info, p_pos);
}

/**
 * @internal
 * Insert a module diff into a buffer.
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param module           In:    The expected module containing differences.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_module_diff (
   bool is_big_endian,
   pf_exp_module_t const * module,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t ix;

   pf_put_uint16 (is_big_endian, module->slot_number, p_info, p_pos);
   if (module->state == PF_MODULE_STATE_NO_MODULE)
   {
      pf_put_uint32 (is_big_endian, 0, p_info, p_pos);
   }
   else
   {
      pf_put_uint32 (
         is_big_endian,
         module->ident_number,
         p_info,
         p_pos);
   }
   pf_put_uint16 (is_big_endian, module->state, p_info, p_pos);
   if (module->state == PF_MODULE_STATE_NO_MODULE)
   {
      pf_put_uint32 (is_big_endian, 0, p_info, p_pos);
   }
   else
   {
      pf_put_uint16 (
         is_big_endian,
         module->nbr_diff_submodules,
         p_info,
         p_pos);
      for (ix = 0; ix < module->nbr_diff_submodules; ix++)
      {
         pf_put_submodule_diff (
            is_big_endian,
            &module->submodule[module->diff_submodule[ix]],
            p_info,
            p_pos);
      }
   }
}

/**
 * @internal
 * Insert an API diff into a buffer.
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param api              In:    The expected api containing differences.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_api_diff (
   bool is_big_endian,
   pf_exp_api_t const * api,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t mod_ix;

   pf_put_uint32 (is_big_endian, api->api, p_info, p_pos);

   pf_put_uint16 (is_big_endian, api->nbr_diff_modules, p_info, p_pos);
   for (mod_ix = 0; mod_ix < api->nbr_diff_modules; mod_ix++)
   {
      pf_put_module_diff (
         is_big_endian,
         &api->module[api->diff_module[mod_ix]],
         p_info,
         p_pos);
   }
}

void pf_put_ar_diff (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos;
   uint16_t block_len;
   uint16_t i;
   const pf_io_ar_t * io_ar = p_ar->io_ar;

   if (io_ar != NULL)
   {
      pf_exp_ident_t const * ident = &io_ar->exp_ident;

      if (ident->nbr_diff_apis > 0)
      {
         block_pos = *p_pos;
         block_len = 0; /* Dont know block_len yet */

         pf_put_block_header (
            is_big_endian,
            PF_BT_MODULE_DIFF_BLOCK,
            block_len,
            PNET_BLOCK_VERSION_HIGH,
            PNET_BLOCK_VERSION_LOW,
            p_info,
            p_pos);

         pf_put_uint16 (
            is_big_endian,
            ident->nbr_diff_apis,
            p_info,
            p_pos);

         for (i = 0; i < ident->nbr_diff_apis; ++i)
         {
            pf_put_api_diff (
               is_big_endian,
               &ident->api[ident->diff_api[i]],
               p_info,
               p_pos);
         }

         /* Finally insert the block length into the block header */
         block_len = *p_pos - (block_pos + 4);
         block_pos += offsetof (pf_block_header_t, block_length);
         pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
      }
   }
}

void pf_put_ar_rpc_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   pf_put_block_header (
      is_big_endian,
      PF_BT_AR_RPC_BLOCK_RES,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->io_ar->ar_rpc_result.responder_rpc_server_port,
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_ar_server_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   pf_put_block_header (
      is_big_endian,
      PF_BT_AR_SERVER_BLOCK,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->io_ar->ar_server.length_cm_responder_station_name,
      p_info,
      p_pos);
   pf_put_mem (
      p_ar->io_ar->ar_server.cm_responder_station_name,
      p_ar->io_ar->ar_server.length_cm_responder_station_name,
      p_info,
      p_pos);

   /* Pad with zeroes until block length is a multiple of 4 bytes */
   pf_put_padding_align (block_pos, 4, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

#if PNET_OPTION_AR_VENDOR_BLOCKS
void pf_put_ar_vendor_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t ix,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   pf_put_block_header (
      is_big_endian,
      PF_BT_AR_VENDOR_BLOCK_RES,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint32 (
      is_big_endian,
      p_ar->io_ar->ar_vendor_request[ix].ap_structure_identifier,
      p_info,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_ar->io_ar->ar_vendor_request[ix].api,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->io_ar->ar_vendor_result[ix].length_data_response,
      p_info,
      p_pos);
   pf_put_mem (
      p_ar->io_ar->ar_vendor_result[ix].data_response,
      p_ar->io_ar->ar_vendor_result[ix].length_data_response,
      p_info,
      p_pos);

   /* Pad with zeroes until block length is a multiple of 4 bytes */
   pf_put_padding_align (block_pos, 4, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}
#endif

/**
 * @internal
 * Insert an IOCR struct into a buffer.
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_ar             In:    The AR instance.
 * @param ix               In:    The IOCR indice.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_iocr (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t ix,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint32_t temp32 = 0;
   uint8_t data_status = 0;

   pf_iocr_t * p_iocr = p_ar->io_ar->iocrs + ix;
   pf_put_bits (p_iocr->param.iocr_properties.rt_class, 4, 0, &temp32);
   pf_put_uint32 (is_big_endian, temp32, p_info, p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_iocr->param.iocr_type,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_iocr->result.frame_id,
      p_info,
      p_pos);

   pf_put_uint16 (is_big_endian, 0, p_info, p_pos);
   if (
      (p_iocr->param.iocr_type == PF_IOCR_TYPE_INPUT) ||
      (p_iocr->param.iocr_type == PF_IOCR_TYPE_MC_PROVIDER))
   {
      (void)pf_ppm_get_data_status (&p_iocr->ppm, &data_status);
   }
   else
   {
      (void)pf_cpm_get_data_status (&p_iocr->cpm, &data_status);
   }
   pf_put_byte (data_status, p_info, p_pos);
   pf_put_byte (0, p_info, p_pos);
}

#if PNET_OPTION_IR
static void pf_put_ir_info (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   /* ToDo: Not yet implemented */
}
#endif

#if PNET_OPTION_SR
static void pf_put_sr_info (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint8_t prop;

   pf_put_block_header (
      is_big_endian,
      PF_BT_SR_INFO_BLOCK_REQ,
      6,
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   /* RDHF */
   pf_put_uint16 (
      is_big_endian,
      p_ar->sr.info.redundancy_data_hold_factor,
      p_info,
      p_pos);

   prop = (p_ar->sr.info.sr_properties.input_valid_on_backup ? BIT (0) : 0);
   prop |= (p_ar->sr.info.sr_properties.mode ? BIT(2) : 0);

   pf_put_byte (prop, p_info, p_pos);
   pf_put_byte (0, p_info, p_pos);
   pf_put_uint16 (is_big_endian, 0, p_info, p_pos);
}
#endif

#if PNET_OPTION_AR_VENDOR_BLOCKS
static void pf_put_ar_vendor_block_req (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t ix,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   /* ToDo: Not yet implemented */
}
static void pf_put_ar_vendor_block_res (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t ix,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   /* ToDo: Not yet implemented */
}
#endif

/**
 * Insert one AR to a buffer.
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_ar             In:    The AR to insert.
 * @param api_filter       In:    true => Insert only specific API.
 * @param api_id           In:    The api id.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_one_ar (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   bool api_filter,
   uint32_t api_id,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   /*
   (ARUUID, CMInitiatorObjectUUID, ParameterServerObjectUUID (b), ARProperties,
                ARType, AlarmCRType, LocalAlarmReference, RemoteAlarmReference,
   InitiatorUDPRTPort, ResponderUDPRTPort, StationNameLength,
   CMInitiatorStationName, [Padding*] (d), StationNameLength (c),
   [ParameterServerStationName], [Padding*] (d), NumberOfIOCRs, [Padding*] (d),
   (IOCRProperties, IOCRType, FrameID, APDU_Status (a))*, NumberOfAPIs,
   [Padding*] (d), API*, ARDataInfo)* Special case: IOSAR with
   ARProperties.DeviceAccess=1: NumberOfIOCRs := 0 AlarmCRType := 0 NumberOfAPIs
   := 0 a The APDU_Status.CycleCounter and APDU_Status.TransferStatus can be
   zero b The ParameterServerObjectUUID shall be NIL if not used c The
   StationNameLength shall be 0 if not used. In this case the field
   ParameterServer-StationName shall be omitted d The number of padding octets
   shall be adapted to make the following field Unsigned32 aligned.
   */
   uint32_t temp32 = 0;
   uint16_t ix;
   uint16_t nbr_ar_data_pos;
   uint16_t nbr_ar_data = 0;
   pf_io_ar_t * io_ar = p_ar->io_ar;
#if PNET_OPTION_PARAMETER_SERVER == 0
   pf_uuid_t nil_uuid = {0};
#endif

   /* ar_param */
   pf_put_uuid (is_big_endian, &p_ar->ar_param.ar_uuid, p_info, p_pos);
   pf_put_uuid (
      is_big_endian,
      &p_ar->ar_param.cm_initiator_object_uuid,
      p_info,
      p_pos);
#if PNET_OPTION_PARAMETER_SERVER
   pf_put_uuid (
      is_big_endian,
      &io_ar->prm_server.parameter_server_object_uuid,
      p_info,
      p_pos);
#else
   pf_put_uuid (is_big_endian, &nil_uuid, p_info, p_pos);
#endif

   pf_put_bits (p_ar->ar_param.ar_properties.state, 3, 0, &temp32);
   pf_put_bits (
      p_ar->ar_param.ar_properties.supervisor_takeover_allowed,
      1,
      3,
      &temp32);
   pf_put_bits (
      p_ar->ar_param.ar_properties.parameterization_server,
      1,
      4,
      &temp32);
   pf_put_bits (p_ar->ar_param.ar_properties.device_access, 1, 8, &temp32);
   pf_put_bits (p_ar->ar_param.ar_properties.companion_ar, 2, 9, &temp32);
   pf_put_bits (
      p_ar->ar_param.ar_properties.acknowledge_companion_ar,
      1,
      11,
      &temp32);
   pf_put_bits (
      p_ar->ar_param.ar_properties.reject_dcp_set_requests,
      1,
      27,
      &temp32);
   pf_put_bits (
      p_ar->ar_param.ar_properties.combined_object_container,
      1,
      29,
      &temp32);
   pf_put_bits (p_ar->ar_param.ar_properties.startup_mode, 1, 30, &temp32);
   pf_put_bits (
      p_ar->ar_param.ar_properties.pull_module_alarm_allowed,
      1,
      31,
      &temp32);
   pf_put_uint32 (is_big_endian, temp32, p_info, p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_param.ar_type,
      p_info,
      p_pos);

   /* alarm_cr_request/response */
   pf_put_uint16 (
      is_big_endian,
      io_ar->alarm_cr_request.alarm_cr_type,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      io_ar->alarm_cr_request.local_alarm_reference,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      io_ar->alarm_cr_result.local_alarm_reference,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_param.cm_initiator_udp_rt_port,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_result.responder_udp_rt_port,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_param.cm_initiator_station_name_len,
      p_info,
      p_pos);
   pf_put_mem (
      p_ar->ar_param.cm_initiator_station_name,
      p_ar->ar_param.cm_initiator_station_name_len,
      p_info,
      p_pos);
   pf_put_padding_align (0, sizeof (uint32_t), p_info, p_pos);
#if PNET_OPTION_PARAMETER_SERVER
   pf_put_uint16 (
      is_big_endian,
      io_ar->prm_server.parameter_server_station_name_len,
      p_info,
      p_pos);
   pf_put_mem (
      io_ar->prm_server.parameter_server_station_name,
      io_ar->prm_server.parameter_server_station_name_len,
      p_info,
      p_pos);
#else
   pf_put_uint16 (is_big_endian, 0, p_info, p_pos);
#endif
   pf_put_padding_align (0, sizeof (uint32_t), p_info, p_pos);
   pf_put_uint16 (is_big_endian, io_ar->nbr_iocrs, p_info, p_pos);
   pf_put_padding_align (0, sizeof (uint32_t), p_info, p_pos);
   for (ix = 0; ix < io_ar->nbr_iocrs; ix++)
   {
      pf_put_iocr (is_big_endian, p_ar, ix, p_info, p_pos);
   }

   pf_put_uint16 (
      is_big_endian,
      io_ar->exp_ident.nbr_apis,
      p_info,
      p_pos);
   pf_put_padding_align (0, sizeof (uint32_t), p_info, p_pos);
   for (ix = 0; ix < io_ar->exp_ident.nbr_apis; ix++)
   {
      if ((api_filter == false) || (io_ar->exp_ident.api[ix].api == api_id))
      {
         pf_put_uint32 (
            is_big_endian,
            io_ar->exp_ident.api[ix].api,
            p_info,
            p_pos);
      }
   }

   /* Remember pos so the correct value can be inserted later */
   nbr_ar_data_pos = *p_pos;
   pf_put_uint16 (is_big_endian, 0, p_info, p_pos);
   pf_put_padding_align (0, sizeof (uint32_t), p_info, p_pos);

#if PNET_OPTION_IR
   if (io_ar->nbr_ir_info > 0)
   {
      pf_put_ir_info (is_big_endian, p_ar, p_info, p_pos);
      nbr_ar_data++;
   }
#endif
#if PNET_OPTION_SR
   if (p_ar->sr.info.valid)
   {
      pf_put_sr_info (is_big_endian, p_ar, p_info, p_pos);
      nbr_ar_data++;
   }
#endif
#if PNET_OPTION_AR_VENDOR_BLOCKS
   for (ix = 0; ix < io_ar->nbr_ar_vendor; ix++)
   {
      pf_put_ar_vendor_block_req (
         is_big_endian,
         p_ar,
         ix,
         p_info,
         p_pos);
      nbr_ar_data++;
   }
   for (ix = 0; ix < io_ar->nbr_ar_vendor; ix++)
   {
      pf_put_ar_vendor_block_res (
         is_big_endian,
         p_ar,
         ix,
         p_info,
         p_pos);
      nbr_ar_data++;
   }
#endif
   pf_put_uint16 (
      is_big_endian,
      nbr_ar_data,
      p_info,
      &nbr_ar_data_pos);
}

void pf_put_ar_data (
   pnet_t * net,
   bool is_big_endian,
   bool filter_by_api,
   uint32_t api_id,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   /*
   BlockHeader, NumberOfARs, (AR)*
   */
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint16_t data_pos;
   uint16_t cnt;
   uint16_t i;
   uint16_t k;

   pf_put_block_header (
      is_big_endian,
      PF_BT_AR_DATA,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW_1,
      p_info,
      p_pos);

   data_pos = *p_pos;

   /* Count the ARs. */
   if (filter_by_api)
   {
      cnt = 0;
      for (i = 0; i < net->ar_order.length; ++i)
      {
         pf_exp_ident_t * exp_ident = &net->ar_order.ar[i]->io_ar->exp_ident;

         for (k = 0; k < exp_ident->nbr_apis; ++k)
         {
            if (exp_ident->api[k].api == api_id)
            {
               cnt++;
               break;
            }
         }
      }
   }
   else
   {
      cnt = net->ar_order.length;
   }

   if (cnt > 0)
   {
      /* Output the count. */
      pf_put_uint16 (is_big_endian, cnt, p_info, p_pos);

      /* Output the ARs. */
      for (i = 0; i < net->ar_order.length; ++i)
      {
         if (filter_by_api)
         {
            pf_exp_ident_t * exp_ident = &net->ar_order.ar[i]->io_ar->exp_ident;

            for (k = 0; k < exp_ident->nbr_apis; ++k)
            {
               if (exp_ident->api[k].api == api_id)
               {
                  break;
               }
            }
            if (!(k < exp_ident->nbr_apis))
            {
               continue;
            }
         }
         pf_put_one_ar (
            is_big_endian,
            net->ar_order.ar[i],
            false,
            0,
            p_info,
            p_pos);
      }
   }

   if (data_pos < *p_pos)
   {
      /* Finally insert the block length into the block header */
      block_len = *p_pos - (block_pos + 4);
      block_pos += offsetof (pf_block_header_t, block_length); /* Point to
                                                                  correct place
                                                                */
      pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
   }
   else
   {
      /* Nothing was inserted!! Report a NULL record */
      *p_pos = block_pos;
   }
}

void pf_put_control (
   bool is_big_endian,
   pf_block_type_values_t block_type,
   const pf_control_block_t * p_res,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   pf_put_block_header (
      is_big_endian,
      block_type,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_byte (0, p_info, p_pos);
   pf_put_byte (0, p_info, p_pos);

   pf_put_uuid (is_big_endian, &p_res->ar_uuid, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_res->session_key, p_info, p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_res->alarm_sequence_number,
      p_info,
      p_pos);

   /* Command and properties are always Big-Endian on the wire!! */
   pf_put_uint16 (
      is_big_endian,
      p_res->control_command,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_res->control_block_properties,
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_pnet_status (
   bool is_big_endian,
   const pnet_pnio_status_t * p_status,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint32_t pnio_result;

   pnio_result = 0x01000000u * p_status->error_code +
                 0x10000u * p_status->error_decode +
                 0x100u * p_status->error_code_1 + p_status->error_code_2;
   pf_put_uint32 (is_big_endian, pnio_result, p_info, p_pos);
}

void pf_put_dce_rpc_header (
   pf_rpc_header_t * p_rpc,
   pf_put_info_t * p_info,
   uint16_t * p_pos,
   uint16_t * p_pos_body_len)
{
   uint32_t temp_u32;

   pf_put_byte (p_rpc->version, p_info, p_pos);
   pf_put_byte (p_rpc->packet_type, p_info, p_pos);
   /* flags */
   temp_u32 = 0;
   pf_put_bits (
      p_rpc->flags.last_fragment ? 1 : 0,
      1,
      PF_RPC_F_LAST_FRAGMENT,
      &temp_u32);
   pf_put_bits (p_rpc->flags.fragment ? 1 : 0, 1, PF_RPC_F_FRAGMENT, &temp_u32);
   pf_put_bits (p_rpc->flags.no_fack ? 1 : 0, 1, PF_RPC_F_NO_FACK, &temp_u32);
   pf_put_bits (p_rpc->flags.maybe ? 1 : 0, 1, PF_RPC_F_MAYBE, &temp_u32);
   pf_put_bits (
      p_rpc->flags.idempotent ? 1 : 0,
      1,
      PF_RPC_F_IDEMPOTENT,
      &temp_u32);
   pf_put_bits (
      p_rpc->flags.broadcast ? 1 : 0,
      1,
      PF_RPC_F_BROADCAST,
      &temp_u32);
   pf_put_byte (temp_u32 % 0x100, p_info, p_pos);

   /* flags2 */
   pf_put_byte (0, p_info, p_pos);
   /* Data repr */
   temp_u32 = 0; /* Always ASCII */
   pf_put_bits (p_rpc->is_big_endian ? 0 : 1, 4, 4, &temp_u32);
   pf_put_byte (temp_u32 % 0x100, p_info, p_pos);
   /* Float repr */
   pf_put_byte (p_rpc->float_repr, p_info, p_pos);

   pf_put_byte (p_rpc->reserved, p_info, p_pos);
   pf_put_byte (p_rpc->serial_high, p_info, p_pos);
   pf_put_uuid (
      p_rpc->is_big_endian,
      &p_rpc->object_uuid,
      p_info,
      p_pos);
   pf_put_uuid (
      p_rpc->is_big_endian,
      &p_rpc->interface_uuid,
      p_info,
      p_pos);
   pf_put_uuid (
      p_rpc->is_big_endian,
      &p_rpc->activity_uuid,
      p_info,
      p_pos);
   pf_put_uint32 (
      p_rpc->is_big_endian,
      p_rpc->server_boot_time,
      p_info,
      p_pos);
   pf_put_uint32 (
      p_rpc->is_big_endian,
      p_rpc->interface_version,
      p_info,
      p_pos);
   pf_put_uint32 (
      p_rpc->is_big_endian,
      p_rpc->sequence_nmb,
      p_info,
      p_pos);
   pf_put_uint16 (p_rpc->is_big_endian, p_rpc->opnum, p_info, p_pos);
   pf_put_uint16 (
      p_rpc->is_big_endian,
      p_rpc->interface_hint,
      p_info,
      p_pos);
   pf_put_uint16 (
      p_rpc->is_big_endian,
      p_rpc->activity_hint,
      p_info,
      p_pos);
   *p_pos_body_len = *p_pos;
   pf_put_uint16 (
      p_rpc->is_big_endian,
      p_rpc->length_of_body,
      p_info,
      p_pos);
   pf_put_uint16 (
      p_rpc->is_big_endian,
      p_rpc->fragment_nmb,
      p_info,
      p_pos);
   pf_put_byte (p_rpc->auth_protocol, p_info, p_pos);
   pf_put_byte (p_rpc->serial_low, p_info, p_pos);
}

void pf_put_dce_rpc_cancel_ack (
   const pf_rpc_header_t * p_rpc_req,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_rpc_header_t rpc = *p_rpc_req;
   uint16_t pos_body_len;

   rpc.packet_type = PF_RPC_PT_CANCEL_ACK;
   rpc.flags.last_fragment = false;
   rpc.flags.fragment = false;
   rpc.flags.no_fack = true;
   rpc.flags.maybe = false;
   rpc.flags.idempotent = true;
   rpc.flags.broadcast = false;
   rpc.flags2.cancel_pending = false;
   rpc.length_of_body = 0;
   rpc.fragment_nmb = 0;

   pf_put_dce_rpc_header (&rpc, p_info, p_pos, &pos_body_len);
}

void pf_put_record_data_read (
   bool is_big_endian,
   pf_block_type_values_t block_type,
   uint16_t raw_length,
   const uint8_t * p_raw_data,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Insert block header for the read operation */
   pf_put_block_header (
      is_big_endian,
      block_type,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_mem (p_raw_data, raw_length, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_read_result (
   bool is_big_endian,
   const pf_iod_read_result_t * p_res,
   pf_put_info_t * p_info,
   uint16_t * p_pos,
   uint16_t * p_data_length_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Insert block header for the read operation */
   pf_put_block_header (
      is_big_endian,
      PF_BT_READ_RES,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_res->sequence_number,
      p_info,
      p_pos);
   pf_put_uuid (is_big_endian, &p_res->ar_uuid, p_info, p_pos);
   pf_put_uint32 (is_big_endian, p_res->api, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_res->slot_number, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_res->subslot_number, p_info, p_pos);
   pf_put_byte (0, p_info, p_pos);
   pf_put_byte (0, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_res->index, p_info, p_pos);
   *p_data_length_pos = *p_pos;
   pf_put_uint32 (
      is_big_endian,
      p_res->record_data_length,
      p_info,
      p_pos);
   pf_put_uint16 (is_big_endian, p_res->add_data_1, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_res->add_data_2, p_info, p_pos);
   pf_put_padding (NELEMENTS (p_res->rw_padding), p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

/**
 * @internal
 * Insert subslot information into a buffer.
 *
 * Inserts subslot number and submodule ID.
 *
 * @param big_endian       In:    Endianness of the destination buffer.
 * @param submodule        In:    The submodule.
 * @param ínfo             InOut: Destination buffer state.
 * @param pos              InOut: Position in destination buffer.
 */
static void pf_put_subslot_data (
   bool big_endian,
   pf_submodule_t const * submodule,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   pf_put_uint16 (big_endian, submodule->subslot_number, p_info, pos);
   pf_put_uint32 (big_endian, submodule->ident_number, p_info, pos);
}

/**
 * @internal
 * Insert slot information into a buffer.
 *
 * Inserts slot number and module ID.
 *
 * @param big_endian       In:    Endianness of the destination buffer.
 * @param slot             In:    The slot.
 * @param submodule        In:    The submodule. If NULL, insert all.
 * @param exp_module       In:    The expected module (if scope is AR).
 * @param ínfo             InOut: Destination buffer state.
 * @param pos              InOut: Position in destination buffer.
 */
static void pf_put_slot_data (
   bool big_endian,
   pf_slot_t const * slot,
   pf_submodule_t const * submodule,
   pf_exp_module_t const * exp_module,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   uint16_t count;
   uint16_t count_pos;
   uint16_t i;

   pf_put_uint16 (big_endian, slot->module->slot_number, p_info, pos);
   pf_put_uint32 (big_endian, slot->module->ident_number, p_info, pos);
   if (exp_module != NULL)
   {
      count = 0;
      count_pos = *pos;
      pf_put_uint16 (big_endian, count, p_info, pos);
      for (i = 0; i < exp_module->nbr_submodules; ++i)
      {
         if (
            pf_cmdev_get_submodule (
               slot,
               exp_module->submodule[i].subslot_number,
               (pf_submodule_t **)&submodule) == 0)
         {
            ++count;
            pf_put_subslot_data (big_endian, submodule, p_info, pos);
         }
      }
      pf_put_uint16 (big_endian, count, p_info, &count_pos);
   }
   else
   {
      if (submodule != NULL)
      {
         pf_put_uint16 (big_endian, 1, p_info, pos);
         pf_put_subslot_data (big_endian, submodule, p_info, pos);
      }
      else
      {
         pf_put_uint16 (big_endian, slot->nbr_submodules, p_info, pos);
         for (submodule = slot->submodule_list; submodule != NULL;
              submodule = submodule->next)
         {
            pf_put_subslot_data (big_endian, submodule, p_info, pos);
         }
      }
   }
}

/**
 * @internal
 * Insert API information into a buffer.
 *
 * Inserts API, and module information.
 *
 * @param big_endian       In:    Endianness of the destination buffer.
 * @param api              In:    The API.
 * @param scope            In:    The scope.
 * @param slot             In:    The slot. If NULL, insert all.
 * @param submodule        In:    The submodule. If NULL, insert all.
 * @param exp_api          In:    The expected API (if scope is AR).
 * @param ínfo             InOut: Destination buffer state.
 * @param pos              InOut: Position in destination buffer.
 */
static void pf_put_api_data (
   bool big_endian,
   pf_api_t const * api,
   pf_slot_t const * slot,
   pf_submodule_t const * submodule,
   pf_exp_api_t const * exp_api,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   uint16_t count;
   uint16_t count_pos;
   uint16_t i;

   pf_put_uint32 (big_endian, api->api_id, p_info, pos);
   if (exp_api != NULL)
   {
      count = 0;
      count_pos = *pos;
      pf_put_uint16 (big_endian, count, p_info, pos);
      for (i = 0; i < exp_api->nbr_modules; ++i)
      {
         if (
            pf_cmdev_get_slot (
               api,
               exp_api->module[i].slot_number,
               (pf_slot_t **)&slot) == 0)
         {
            ++count;
            pf_put_slot_data (
               big_endian,
               slot,
               NULL,
               &exp_api->module[i],
               p_info,
               pos);
         }
      }
      pf_put_uint16 (big_endian, count, p_info, &count_pos);
   }
   else
   {
      if (slot != NULL)
      {
         pf_put_uint16 (big_endian, 1, p_info, pos);
         pf_put_slot_data (big_endian, slot, submodule, NULL, p_info, pos);
      }
      else
      {
         pf_put_uint16 (big_endian, api->nbr_modules, p_info, pos);
         for (slot = api->slot_list; slot != NULL; slot = slot->next)
         {
            pf_put_slot_data (
               big_endian,
               slot,
               NULL,
               NULL,
               p_info,
               pos);
         }
      }
   }
}

static void pf_put_real_device_data (
   pnet_t * net,
   pf_device_t const * device,
   bool big_endian,
   pf_record_data_scope_t scope,
   pf_exp_ident_t const * exp_ident,
   uint32_t api,
   uint16_t slot_number,
   uint16_t subslot_number,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   uint16_t count;
   uint16_t count_pos;
   pf_api_t * this_api;
   pf_slot_t * this_slot;
   pf_submodule_t * this_submodule;
   uint16_t i;

   if (scope == PF_RECORD_DATA_SCOPE_AR)
   {
      if (exp_ident != NULL)
      {
         count = 0;
         count_pos = *pos;
         pf_put_uint16 (big_endian, count, p_info, pos);
         for (i = 0; i < exp_ident->nbr_apis; ++i)
         {
            if (pf_cmdev_get_api (net, exp_ident->api[i].api, &this_api) == 0)
            {
               ++count;
               pf_put_api_data (
                  big_endian,
                  this_api,
                  NULL,
                  NULL,
                  &exp_ident->api[i],
                  p_info,
                  pos);
            }
         }
         pf_put_uint16 (big_endian, count, p_info, &count_pos);
      }
   }
   else if (scope == PF_RECORD_DATA_SCOPE_DEVICE)
   {
      count = device->real_ident.nbr_apis;
      pf_put_uint16 (big_endian, count, p_info, pos);
      for (this_api = device->real_ident.api_list; this_api != NULL;
           this_api = this_api->next)
      {
         pf_put_uint32 (
            big_endian,
            this_api->api_id,
            p_info,
            pos);
      }
   }
   else
   {
      this_api = NULL;
      this_slot = NULL;
      this_submodule = NULL;
      if (pf_cmdev_get_api (net, api, &this_api) == 0)
      {
         if (scope != PF_RECORD_DATA_SCOPE_API)
         {
            if (pf_cmdev_get_slot (this_api, slot_number, &this_slot) == 0)
            {
               if (scope == PF_RECORD_DATA_SCOPE_SUBSLOT)
               {
                  (void)pf_cmdev_get_submodule (
                     this_slot,
                     subslot_number,
                     &this_submodule);
               }
            }
         }
      }

      if (
         ((scope == PF_RECORD_DATA_SCOPE_API) && (this_api != NULL)) ||
         ((scope == PF_RECORD_DATA_SCOPE_SLOT) && (this_slot != NULL)) ||
         ((scope == PF_RECORD_DATA_SCOPE_SUBSLOT) && (this_submodule != NULL)))
      {
         pf_put_uint16 (big_endian, 1, p_info, pos);
         pf_put_api_data (
            big_endian,
            this_api,
            this_slot,
            this_submodule,
            NULL,
            p_info,
            pos);
      }
   }
}

void pf_put_real_ident_data (
   pnet_t * net,
   bool big_endian,
   uint8_t block_version_low,
   pf_record_data_scope_t scope,
   pf_ar_t const * ar,
   uint32_t api,
   uint16_t slot_number,
   uint16_t subslot_number,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   uint16_t block_pos;
   uint16_t block_len;
   uint16_t data_pos;
   pf_device_t * device;

   block_len = 0; /* Dont know block_len yet */
   block_pos = *pos;
   pf_put_block_header (
      big_endian,
      (scope == PF_RECORD_DATA_SCOPE_DEVICE) ? PF_BT_API_DATA
                                             : PF_BT_REAL_IDENTIFICATION_DATA,
      block_len,
      PNET_BLOCK_VERSION_HIGH,
      block_version_low,
      p_info,
      pos);

   data_pos = *pos;
   device = NULL;
   if (pf_cmdev_get_device (net, &device) == 0)
   {
      if (scope == PF_RECORD_DATA_SCOPE_AR)
      {
         if ((ar != NULL) && (ar->ar_param.ar_type != PF_ART_IOSAR))
         {
            pf_put_real_device_data (
               net,
               device,
               big_endian,
               scope,
               &ar->io_ar->exp_ident,
               api,
               slot_number,
               subslot_number,
               p_info,
               pos);
         }
      }
      else
      {
         pf_put_real_device_data (
            net,
            device,
            big_endian,
            scope,
            NULL,
            api,
            slot_number,
            subslot_number,
            p_info,
            pos);
      }
   }

   if (data_pos < *pos)
   {
      /* Finally insert the block length into the block header */
      block_len = *pos - (block_pos + 4);
      block_pos += offsetof (pf_block_header_t, block_length); /* Point to
                                                                  correct place
                                                                */
      pf_put_uint16 (big_endian, block_len, p_info, &block_pos);
   }
   else
   {
      /* Nothing was inserted!! Report a NULL record */
      *pos = block_pos;
   }
}

/**
 * @internal
 * Insert the expected submodule information into a buffer.
 *
 * Inserts subslot number and submodule ID.
 *
 * @param big_endian       In:    Endianness of the destination buffer.
 * @param submodule        In:    The expected submodule.
 * @param ínfo             InOut: Destination buffer state.
 * @param pos              InOut: Position in destination buffer.
 */
static void pf_put_exp_submodule_data (
   bool big_endian,
   pf_exp_submodule_t const * submodule,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   pf_put_uint16 (big_endian, submodule->subslot_number, p_info, pos);
   pf_put_uint32 (big_endian, submodule->ident_number, p_info, pos);
}

/**
 * @internal
 * Insert the expected module information into a buffer.
 *
 * Inserts slot number, module ID, and information for one or all submodules.
 *
 * @param big_endian       In:    Endianness of the destination buffer.
 * @param module           In:    The expected module.
 * @param submodule        In:    Expected submodule. If NULL, insert all.
 * @param ínfo             InOut: Destination buffer state.
 * @param pos              InOut: Position in destination buffer.
 */
static void pf_put_exp_module_data (
   bool big_endian,
   pf_exp_module_t const * module,
   pf_exp_submodule_t const * submodule,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   uint16_t s;

   pf_put_uint16 (big_endian, module->slot_number, p_info, pos);
   pf_put_uint32 (big_endian, module->ident_number, p_info, pos);
   if (submodule != NULL)
   {
      pf_put_uint16 (big_endian, 1, p_info, pos);
      pf_put_exp_submodule_data (big_endian, submodule, p_info, pos);
   }
   else
   {
      pf_put_uint16 (big_endian, module->nbr_submodules, p_info, pos);
      for (s = 0; s < module->nbr_submodules; ++s)
      {
         pf_put_exp_submodule_data (
            big_endian,
            &module->submodule[s],
            p_info,
            pos);
      }
   }
}

/**
 * @internal
 * Insert the expected API information into a buffer.
 *
 * Inserts API and information for one or all modules.
 *
 * @param big_endian       In:    Endianness of the destination buffer.
 * @param api              In:    The expected API.
 * @param module           In:    Expected module. If NULL, insert all.
 * @param submodule        In:    Expected submodule. If NULL, insert all.
 * @param ínfo             InOut: Destination buffer state.
 * @param pos              InOut: Position in destination buffer.
 */
static void pf_put_exp_api_data (
   bool big_endian,
   pf_exp_api_t const * api,
   pf_exp_module_t const * module,
   pf_exp_submodule_t const * submodule,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   uint16_t m;

   pf_put_uint32 (big_endian, api->api, p_info, pos);
   if (module != NULL)
   {
      pf_put_uint16 (big_endian, 1, p_info, pos);
      pf_put_exp_module_data (big_endian, module, submodule, p_info, pos);
   }
   else
   {
      pf_put_uint16 (big_endian, api->nbr_modules, p_info, pos);
      for (m = 0; m < api->nbr_modules; ++m)
      {
         pf_put_exp_module_data (
            big_endian,
            &api->module[m],
            NULL,
            p_info,
            pos);
      }
   }
}

void pf_put_exp_ident_data (
   pf_ar_t const * ar,
   bool big_endian,
   uint8_t block_version_low,
   pf_record_data_scope_t scope,
   uint32_t api,
   uint16_t slot_number,
   uint16_t subslot_number,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   uint16_t block_pos;
   uint16_t block_len;
   pf_exp_ident_t const * exp_ident;
   pf_exp_api_t const * exp_api;
   pf_exp_module_t const * exp_module;
   pf_exp_submodule_t const * exp_submodule;
   uint16_t i;

   if (ar->io_ar == NULL)
   {
      /* Different Access Ways: RecordData length should be 0 for DA-AR */
      return;
   }

   block_len = 0; /* Dont know block_len yet */
   block_pos = *pos;
   pf_put_block_header (
      big_endian,
      PF_BT_EXPECTED_IDENTIFICATION_DATA,
      block_len,
      PNET_BLOCK_VERSION_HIGH,
      block_version_low,
      p_info,
      pos);

   exp_ident = &ar->io_ar->exp_ident;
   if (scope == PF_RECORD_DATA_SCOPE_AR)
   {
      pf_put_uint16 (big_endian, exp_ident->nbr_apis, p_info, pos);
      for (i = 0; i < exp_ident->nbr_apis; ++i)
      {
         pf_put_exp_api_data (
            big_endian,
            &exp_ident->api[i],
            NULL,
            NULL,
            p_info,
            pos);
      }
   }
   else
   {
      exp_api = NULL;
      exp_module = NULL;
      exp_submodule = NULL;
      for (i = 0; i < exp_ident->nbr_apis; ++i)
      {
         if (exp_ident->api[i].api == api)
         {
            exp_api = &exp_ident->api[i];
            break;
         }
      }

      if (exp_api != NULL)
      {
         for (i = 0; i < exp_api->nbr_modules; ++i)
         {
            if (exp_api->module[i].slot_number == slot_number)
            {
               exp_module = &exp_api->module[i];
               break;
            }
         }
      }

      if ((scope == PF_RECORD_DATA_SCOPE_SUBSLOT) && (exp_module != NULL))
      {
         for (i = 0; i < exp_module->nbr_submodules; ++i)
         {
            if (exp_module->submodule[i].subslot_number == subslot_number)
            {
               exp_submodule = &exp_module->submodule[i];
               break;
            }
         }
      }

      if (
         ((scope == PF_RECORD_DATA_SCOPE_SLOT) && (exp_module != NULL)) ||
         ((scope == PF_RECORD_DATA_SCOPE_SUBSLOT) && (exp_submodule != NULL)))
      {
         pf_put_uint16 (big_endian, 1, p_info, pos);
         pf_put_exp_api_data (
            big_endian,
            exp_api,
            exp_module,
            exp_submodule,
            p_info,
            pos);
      }
      else
      {
         pf_put_uint16 (big_endian, 0, p_info, pos);
      }
   }

   /* Finally insert the block length into the block header */
   block_len = *pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length);
   pf_put_uint16 (big_endian, block_len, p_info, &block_pos);
}

static bool pf_put_im_0_filter_data_info (
   pf_real_ident_t * real_ident,
   uint8_t flags,
   bool is_big_endian,
   pf_put_info_t * info,
   uint16_t * pos)
{
   pf_api_t * api;
   pf_slot_t * slot;
   pf_submodule_t * submodule;
   struct
   {
      uint16_t apis;
      uint16_t modules;
      uint16_t submodules;
   } no;
   struct
   {
      uint16_t api_count;
      uint16_t api;
      uint16_t module_count;
      uint16_t module;
      uint16_t submodule_count;
   } p;

   p.api_count = *pos;
   pf_put_uint16 (is_big_endian, 0, info, pos);
   p.api = *pos;
   no.apis = 0;
   for (api = real_ident->api_list; api != NULL; api = api->next)
   {
      pf_put_uint32 (is_big_endian, api->api_id, info, pos);
      p.module_count = *pos;
      pf_put_uint16 (is_big_endian, 0, info, pos);
      p.module = *pos;
      no.modules = 0;
      for (slot = api->slot_list; slot != NULL; slot = slot->next)
      {
         pf_put_uint16 (is_big_endian, slot->module->slot_number, info, pos);
         pf_put_uint32 (is_big_endian, slot->module->ident_number, info, pos);
         p.submodule_count = *pos;
         pf_put_uint16 (is_big_endian, 0, info, pos);
         no.submodules = 0;
         for (submodule = slot->submodule_list; submodule != NULL;
              submodule = submodule->next)
         {
            if ((submodule->flags & flags) == flags)
            {
               pf_put_uint16 (
                  is_big_endian,
                  submodule->subslot_number,
                  info,
                  pos);
               pf_put_uint32 (is_big_endian, submodule->ident_number, info, pos);
               ++no.submodules;
            }
         }
         if (no.submodules > 0)
         {
            pf_put_uint16 (
               is_big_endian,
               no.submodules,
               info,
               &p.submodule_count);
            p.module = *pos;
            ++no.modules;
         }
         else
         {
            *pos = p.module;
         }
      }
      if (no.modules > 0)
      {
         pf_put_uint16 (is_big_endian, no.modules, info, &p.module_count);
         p.api = *pos;
         ++no.apis;
      }
      else
      {
         *pos = p.api;
      }
   }
   if (no.apis > 0)
   {
      pf_put_uint16 (is_big_endian, no.apis, info, &p.api_count);
      return true;
   }
   return false;
}

void pf_put_im_0_filter_data (
   pnet_t * net,
   bool is_big_endian,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   pf_device_t * p_device = NULL;

   pf_cmdev_get_device (net, &p_device);

   /* I&M0FilterDataSubmodule */
   block_pos = *p_pos;
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_0_FILTER_DATA_SUBMODULE,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   (void)pf_put_im_0_filter_data_info (
      &p_device->real_ident,
      PNET_SMF_IM_CARRIER,
      is_big_endian,
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);

   /* I&M0FilterDataModule - only if any module representatives exist.*/
   block_pos = *p_pos;
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_0_FILTER_DATA_MODULE,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   if (pf_put_im_0_filter_data_info (
          &p_device->real_ident,
          (PNET_SMF_IM_CARRIER | PNET_SMF_IM_MODULE_REP),
          is_big_endian,
          p_info,
          p_pos))
   {
      /* Insert the block length into the block header */
      block_len = *p_pos - (block_pos + 4);
      block_pos += offsetof (pf_block_header_t, block_length); /* Point to
                                                                  correct place
                                                                */
      pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
   }
   else
   {
      /* Back up. */
      *p_pos = block_pos;
   }

   /* I&M0FilterDataDevice */
   block_pos = *p_pos;
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_0_FILTER_DATA_DEVICE,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   (void)pf_put_im_0_filter_data_info (
      &p_device->real_ident,
      (PNET_SMF_IM_CARRIER | PNET_SMF_IM_DEVICE_REP),
      is_big_endian,
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_im_0 (
   bool is_big_endian,
   const pnet_im_0_t * p_im_0,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Insert block header for the read operation */
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_0,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_byte (p_im_0->im_vendor_id_hi, p_info, p_pos);
   pf_put_byte (p_im_0->im_vendor_id_lo, p_info, p_pos);
   pf_put_str (
      p_im_0->im_order_id,
      sizeof (p_im_0->im_order_id),
      p_info,
      p_pos);
   pf_put_str (
      p_im_0->im_serial_number,
      sizeof (p_im_0->im_serial_number),
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_im_0->im_hardware_revision,
      p_info,
      p_pos);
   pf_put_byte (p_im_0->im_sw_revision_prefix, p_info, p_pos);
   pf_put_byte (
      p_im_0->im_sw_revision_functional_enhancement,
      p_info,
      p_pos);
   pf_put_byte (p_im_0->im_sw_revision_bug_fix, p_info, p_pos);
   pf_put_byte (p_im_0->im_sw_revision_internal_change, p_info, p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_im_0->im_revision_counter,
      p_info,
      p_pos);
   pf_put_uint16 (is_big_endian, p_im_0->im_profile_id, p_info, p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_im_0->im_profile_specific_type,
      p_info,
      p_pos);
   pf_put_byte (p_im_0->im_version_major, p_info, p_pos);
   pf_put_byte (p_im_0->im_version_minor, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_im_0->im_supported, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_im_1 (
   bool is_big_endian,
   const pnet_im_1_t * im_1,
   pf_put_info_t * info,
   uint16_t * pos)
{
   uint16_t block_pos = *pos;
   uint16_t block_len = 0;

   /* Insert block header for the read operation */
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_1,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      info,
      pos);

   pf_put_str (im_1->im_tag_function, sizeof (im_1->im_tag_function), info, pos);
   pf_put_str (im_1->im_tag_location, sizeof (im_1->im_tag_location), info, pos);

   /* Finally insert the block length into the block header */
   block_len = *pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, info, &block_pos);
}

void pf_put_im_2 (
   bool is_big_endian,
   const pnet_im_2_t * im_2,
   pf_put_info_t * info,
   uint16_t * pos)
{
   uint16_t block_pos = *pos;
   uint16_t block_len = 0;

   /* Insert block header for the read operation */
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_2,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      info,
      pos);

   pf_put_str (im_2->im_date, sizeof (im_2->im_date), info, pos);

   /* Finally insert the block length into the block header */
   block_len = *pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, info, &block_pos);
}

void pf_put_im_3 (
   bool is_big_endian,
   const pnet_im_3_t * im_3,
   pf_put_info_t * info,
   uint16_t * pos)
{
   uint16_t block_pos = *pos;
   uint16_t block_len = 0;

   /* Insert block header for the read operation */
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_3,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      info,
      pos);

   pf_put_str (im_3->im_descriptor, sizeof (im_3->im_descriptor), info, pos);

   /* Finally insert the block length into the block header */
   block_len = *pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, info, &block_pos);
}

void pf_put_im_4 (
   bool is_big_endian,
   const pnet_im_4_t * im_4,
   pf_put_info_t * info,
   uint16_t * pos)
{
   uint16_t block_pos = *pos;
   uint16_t block_len = 0;

   /* Insert block header for the read operation */
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_4,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      info,
      pos);

   pf_put_mem (im_4->im_signature, sizeof (im_4->im_signature), info, pos);

   /* Finally insert the block length into the block header */
   block_len = *pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, info, &block_pos);
}

void pf_put_record_data_write (
   bool is_big_endian,
   pf_block_type_values_t block_type,
   uint16_t raw_length,
   uint8_t * p_raw_data,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Insert block header for the read operation */
   pf_put_block_header (
      is_big_endian,
      block_type,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_mem (p_raw_data, raw_length, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_write_result (
   bool is_big_endian,
   const pf_iod_write_result_t * p_res,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Insert block header for the write operation */
   pf_put_block_header (
      is_big_endian,
      PF_BT_WRITE_RES,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_res->sequence_number,
      p_info,
      p_pos);
   pf_put_uuid (is_big_endian, &p_res->ar_uuid, p_info, p_pos);
   pf_put_uint32 (is_big_endian, p_res->api, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_res->slot_number, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_res->subslot_number, p_info, p_pos);
   pf_put_byte (0, p_info, p_pos);
   pf_put_byte (0, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_res->index, p_info, p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_res->record_data_length,
      p_info,
      p_pos);
   pf_put_uint16 (is_big_endian, p_res->add_data_1, p_info, p_pos);
   pf_put_uint16 (is_big_endian, p_res->add_data_2, p_info, p_pos);
   pf_put_pnet_status (
      is_big_endian,
      &p_res->pnio_status,
      p_info,
      p_pos);

   pf_put_padding (NELEMENTS (p_res->rw_padding), p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);

   if (*p_pos >= p_info->len)
   {
      LOG_ERROR (
         PNET_LOG,
         "BW(%d): Output buffer is filled, while preparing DCP Write "
         "response.\n",
         __LINE__);
      p_info->result = PF_PUT_END_OF_OUTPUT;
   }
}

void pf_put_log_book_data (
   bool is_big_endian,
   const pf_log_book_t * p_log_book,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint16_t ix;
   uint16_t cnt = 0;

   /* Insert block header for the write operation */
   pf_put_block_header (
      is_big_endian,
      PF_BT_LOG_BOOK_DATA,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   if (p_log_book->wrap == true)
   {
      ix = p_log_book->put + 1;
      if (ix >= NELEMENTS (p_log_book->entries))
      {
         ix = 0;
      }
      cnt = NELEMENTS (p_log_book->entries);
   }
   else
   {
      ix = 0;
      cnt = p_log_book->put;
   }

   pf_put_time_timestamp (
      is_big_endian,
      &p_log_book->time_ts,
      p_info,
      p_pos);

   pf_put_uint16 (is_big_endian, cnt, p_info, p_pos);

   while (ix != p_log_book->put)
   {
      pf_put_time_timestamp (
         is_big_endian,
         &p_log_book->entries[ix].time_ts,
         p_info,
         p_pos);
      pf_put_uuid (
         is_big_endian,
         &p_log_book->entries[ix].ar_uuid,
         p_info,
         p_pos);
      pf_put_pnet_status (
         is_big_endian,
         &p_log_book->entries[ix].pnio_status,
         p_info,
         p_pos);
      pf_put_uint32 (
         is_big_endian,
         p_log_book->entries[ix].entry_detail,
         p_info,
         p_pos);
      ix++;
      if (ix >= NELEMENTS (p_log_book->entries))
      {
         ix = 0;
      }
   }

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

/**
 * @internal
 * Insert one diagnosis item to a buffer.
 *
 * The resulting format is controlled by the USI of the diagnosis item:
 *  - Manufacturer specific format
 *  - Channel diagnosis (standard format)
 *  - Extended channel diagnosis (standard format)
 *  - Qualified channel diagnosis (standard format)
 *
 * Used both for creating an alarm frame and a diagnostics read answer frame.
 *
 * @param is_big_endian    In:    true if buffer is big-endian.
 * @param p_item           In:    The diag item to insert.
 * @param insert_usi       In:    Insert USI field if true.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_diag_item (
   bool is_big_endian,
   const pf_diag_item_t * p_item,
   bool insert_usi,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   if (insert_usi == true)
   {
      pf_put_uint16 (is_big_endian, p_item->usi, p_info, p_pos);
   }

   switch (p_item->usi)
   {
   case PF_USI_CHANNEL_DIAGNOSIS:
      /* Insert std format diagnosis */
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_nbr,
         p_info,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_properties,
         p_info,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_error_type,
         p_info,
         p_pos);
      break;
   case PF_USI_EXTENDED_CHANNEL_DIAGNOSIS:
      /* Insert std format diagnosis */
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_nbr,
         p_info,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_properties,
         p_info,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_error_type,
         p_info,
         p_pos);

      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ext_ch_error_type,
         p_info,
         p_pos);
      pf_put_uint32 (
         is_big_endian,
         p_item->fmt.std.ext_ch_add_value,
         p_info,
         p_pos);
      break;
   case PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS:
      /* Insert std format diagnosis */
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_nbr,
         p_info,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_properties,
         p_info,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_error_type,
         p_info,
         p_pos);

      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ext_ch_error_type,
         p_info,
         p_pos);
      pf_put_uint32 (
         is_big_endian,
         p_item->fmt.std.ext_ch_add_value,
         p_info,
         p_pos);

      pf_put_uint32 (
         is_big_endian,
         p_item->fmt.std.qual_ch_qualifier,
         p_info,
         p_pos);
      break;
   default:
      pf_put_mem (
         p_item->fmt.usi.manuf_data,
         p_item->fmt.usi.len,
         p_info,
         p_pos);
      break;
   }
}

/**
 * @internal
 * Insert a diagnosis item list (for a subslot) into a buffer.
 *
 * Note that all diagnoses inserted into this block in the buffer must be for
 * the same API, slot, subslot and USI.
 *
 * Insertion is done via pf_put_diag_item()
 *
 * @param net              InOut: The p-net stack instance
 * @param is_big_endian    In:    true if buffer is big-endian.
 * @param diag_filter      In:    Type of diag items to insert.
 * @param usi_filter       In:    USI value to filter by. Mandatory.
 * @param api_id           In:    The API number to write to buffer.
 * @param slot_nbr         In:    The slot number to write to buffer.
 * @param subslot_nbr      In:    The subslot number to write to buffer.
 * @param list_head        In:    Index of first item in the list to insert.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_diag_list (
   pnet_t * net,
   bool is_big_endian,
   pf_diag_filter_level_t diag_filter,
   uint16_t usi_filter,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t list_head,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t ch_properties = 0;
   pf_diag_item_t * p_item = NULL;
   bool insert;
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint16_t data_pos;

   /* Walk the list to insert all items */
   pf_cmdev_get_diag_item (net, list_head, &p_item);
   if (p_item != NULL)
   {
      /* Insert block header */
      pf_put_block_header (
         is_big_endian,
         PF_BT_DIAGNOSIS_DATA,
         0, /* Dont know block_len yet */
         PNET_BLOCK_VERSION_HIGH,
         PNET_BLOCK_VERSION_LOW_1,
         p_info,
         p_pos);

      /* Insert API, slot and subslot */
      pf_put_uint32 (is_big_endian, api_id, p_info, p_pos);
      pf_put_uint16 (is_big_endian, slot_nbr, p_info, p_pos);
      pf_put_uint16 (is_big_endian, subslot_nbr, p_info, p_pos);

      /* Insert summary-channel, and its properties
       * The list only contains APPEARS, so it can be hardcoded here.
       * ToDo: More info into ch_properties here!
       */
      pf_put_uint16 (
         is_big_endian,
         PNET_CHANNEL_WHOLE_SUBMODULE,
         p_info,
         p_pos);
      PF_DIAG_CH_PROP_SPEC_SET (ch_properties, PF_DIAG_CH_PROP_SPEC_APPEARS);
      pf_put_uint16 (is_big_endian, ch_properties, p_info, p_pos);

      /* Insert USI */
      pf_put_uint16 (is_big_endian, usi_filter, p_info, p_pos);

      data_pos = *p_pos;
      while (p_item != NULL)
      {

         /* Filter based on diagnosis type */
         insert = false;
         switch (diag_filter)
         {
         case PF_DIAG_FILTER_FAULT_STD:
            if (p_item->usi >= PF_USI_CHANNEL_DIAGNOSIS)
            {
               insert = true;
            }
            break;
         case PF_DIAG_FILTER_FAULT_ALL:
            if (
               (p_item->usi < PF_USI_CHANNEL_DIAGNOSIS) ||
               (PF_DIAG_CH_PROP_MAINT_GET (p_item->fmt.std.ch_properties) ==
                PNET_DIAG_CH_PROP_MAINT_FAULT))
            {
               insert = true;
            }
            break;
         case PF_DIAG_FILTER_ALL:
            insert = true;
            break;
         case PF_DIAG_FILTER_M_REQ:
            if (
               (p_item->usi < PF_USI_CHANNEL_DIAGNOSIS) ||
               (PF_DIAG_CH_PROP_MAINT_GET (p_item->fmt.std.ch_properties) ==
                PNET_DIAG_CH_PROP_MAINT_REQUIRED))
            {
               insert = true;
            }
            break;
         case PF_DIAG_FILTER_M_DEM:
            if (
               (p_item->usi < PF_USI_CHANNEL_DIAGNOSIS) ||
               (PF_DIAG_CH_PROP_MAINT_GET (p_item->fmt.std.ch_properties) ==
                PNET_DIAG_CH_PROP_MAINT_DEMANDED))
            {
               insert = true;
            }
            break;
         }

         /* Filter based on USI value */
         if (p_item->usi != usi_filter)
         {
            insert = false;
         }

         if (insert == true)
         {
            /* Insert diagnosis item */
            pf_put_diag_item (
               is_big_endian,
               p_item,
               false, /* Do not insert USI value */
               p_info,
               p_pos);
         }

         pf_cmdev_get_diag_item (net, p_item->next, &p_item);
      }

      /* Finally insert the block length into the block header */
      if (*p_pos > data_pos)
      {
         block_len = *p_pos - (block_pos + 4);
         block_pos += offsetof (pf_block_header_t, block_length);
         pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
      }
      else
      {
         /* Do not insert an empty block. */
         *p_pos = block_pos;
      }
   }
}

/**
 * @internal
 * Insert diagnosis items of a subslot into a buffer.
 *
 * @param net              InOut: The p-net stack instance
 * @param big_endian       In:    true if buffer is big-endian.
 * @param diag_filter      In:    Type of diag items to insert.
 * @param api              In:    The API number to write to buffer.
 * @param slot_number      In:    The slot number to write to buffer.
 * @param subslot          In:    The subslot instance.
 * @param ínfo             InOut: Destination buffer state.
 * @param pos              InOut: Position in destination buffer.
 */
static void pf_put_diag_subslot (
   pnet_t * net,
   bool big_endian,
   pf_diag_filter_level_t diag_filter,
   uint32_t api,
   uint16_t slot_number,
   pf_submodule_t const * submodule,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   uint16_t usi;
   int err;

   usi = 0;
   err = pf_cmdev_get_next_diagnosis_usi (net, submodule->diag_list, 0, &usi);
   while (err == 0)
   {
      pf_put_diag_list (
         net,
         big_endian,
         diag_filter,
         usi,
         api,
         slot_number,
         submodule->subslot_number,
         submodule->diag_list,
         p_info,
         pos);

      err =
         pf_cmdev_get_next_diagnosis_usi (net, submodule->diag_list, usi, &usi);
   }
}

/**
 * @internal
 * Insert diagnosis items of a slot into a buffer.
 *
 * @param net              InOut: The p-net stack instance
 * @param big_endian       In:    true if buffer is big-endian.
 * @param diag_filter      In:    Type of diag items to insert.
 * @param api              In:    The API number to write to buffer.
 * @param slot             In:    The slot instance.
 * @param ínfo             InOut: Destination buffer state.
 * @param pos              InOut: Position in destination buffer.
 */
static void pf_put_diag_slot (
   pnet_t * net,
   bool big_endian,
   pf_diag_filter_level_t diag_filter,
   uint32_t api,
   pf_slot_t const * slot,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   pf_submodule_t const * submodule;

   for (submodule = slot->submodule_list; submodule != NULL;
        submodule = submodule->next)
   {
      pf_put_diag_subslot (
         net,
         big_endian,
         diag_filter,
         api,
         slot->module->slot_number,
         submodule,
         p_info,
         pos);
   }
}

/**
 * @internal
 * Insert diagnosis items of a slot, with AR filtering, into a buffer.
 *
 * @param net              InOut: The p-net stack instance
 * @param big_endian       In:    true if buffer is big-endian.
 * @param diag_filter      In:    Type of diag items to insert.
 * @param api              In:    The API number to write to buffer.
 * @param slot             In:    The slot instance.
 * @param exp_module       In:    The expected module instance.
 * @param ínfo             InOut: Destination buffer state.
 * @param pos              InOut: Position in destination buffer.
 */
static void pf_put_diag_ar_slot (
   pnet_t * net,
   bool big_endian,
   pf_diag_filter_level_t diag_filter,
   uint32_t api,
   pf_slot_t const * slot,
   pf_exp_module_t const * exp_module,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   pf_exp_submodule_t const * exp_submodule;
   pf_submodule_t * submodule;
   uint16_t s;

   for (s = 0; s < exp_module->nbr_submodules; ++s)
   {
      exp_submodule = &exp_module->submodule[s];
      if (
         (pf_cmdev_get_submodule (
             slot,
             exp_submodule->subslot_number,
             &submodule) == 0) &&
         pf_cmdev_submodule_is_compatible (
            net,
            exp_submodule->ident_number,
            submodule->ident_number))
      {
         pf_put_diag_subslot (
            net,
            big_endian,
            diag_filter,
            api,
            slot->module->slot_number,
            submodule,
            p_info,
            pos);
      }
   }
}

/**
 * @internal
 * Insert diagnosis items of an API into a buffer.
 *
 * @param net              InOut: The p-net stack instance
 * @param big_endian       In:    true if buffer is big-endian.
 * @param diag_filter      In:    Type of diag items to insert.
 * @param api              In:    The API number to write to buffer.
 * @param ínfo             InOut: Destination buffer state.
 * @param pos              InOut: Position in destination buffer.
 */
static void pf_put_diag_api (
   pnet_t * net,
   bool big_endian,
   pf_diag_filter_level_t diag_filter,
   pf_api_t const * api,
   pf_put_info_t * info,
   uint16_t * pos)
{
   pf_slot_t const * slot;

   for (slot = api->slot_list; slot != NULL; slot = slot->next)
   {
      pf_put_diag_slot (
         net,
         big_endian,
         diag_filter,
         api->api_id,
         slot,
         info,
         pos);
   }
}

void pf_put_diagnosis_subslot (
   pnet_t * net,
   bool big_endian,
   pf_diag_filter_level_t diag_filter,
   pf_ar_t * ar,
   uint32_t api,
   uint16_t slot_number,
   uint16_t subslot_number,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   pf_slot_t * slot;
   pf_submodule_t * submodule;
   pf_exp_module_t * exp_module;
   pf_exp_submodule_t * exp_submodule;

   slot = NULL;
   if (pf_cmdev_get_slot_full (net, api, slot_number, &slot) != 0)
   {
      return;
   }
   submodule = NULL;
   if (pf_cmdev_get_submodule (slot, subslot_number, &submodule) != 0)
   {
      return;
   }
   if (ar != NULL)
   {
      exp_module = NULL;
      exp_submodule = NULL;
      if (
         (pf_cmdev_get_exp_mod (ar, api, slot_number, &exp_module) == 0) &&
         (pf_cmdev_get_exp_sub (
             ar,
             api,
             slot_number,
             subslot_number,
             &exp_submodule) == 0))
      {
         if (
            (exp_module->ident_number != slot->module->ident_number) ||
            !pf_cmdev_submodule_is_compatible (
               net,
               exp_submodule->ident_number,
               submodule->ident_number))
         {
            return;
         }
      }
   }
   pf_put_diag_subslot (
      net,
      big_endian,
      diag_filter,
      api,
      slot_number,
      submodule,
      p_info,
      pos);
}

void pf_put_diagnosis_slot (
   pnet_t * net,
   bool big_endian,
   pf_diag_filter_level_t diag_filter,
   pf_ar_t * ar,
   uint32_t api,
   uint16_t slot_number,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   pf_slot_t * slot;
   pf_exp_module_t * module;

   slot = NULL;
   if (pf_cmdev_get_slot_full (net, api, slot_number, &slot) != 0)
   {
      return;
   }
   module = NULL;
   if ((ar == NULL) || (pf_cmdev_get_exp_mod (ar, api, slot_number, &module) != 0))
   {
      pf_put_diag_slot (
         net,
         big_endian,
         diag_filter,
         api,
         slot,
         p_info,
         pos);
   }
   else if (module->ident_number == slot->module->ident_number)
   {
      pf_put_diag_ar_slot (
         net,
         big_endian,
         diag_filter,
         api,
         slot,
         module,
         p_info,
         pos);
   }
}

void pf_put_diagnosis_ar (
   pnet_t * net,
   bool big_endian,
   pf_diag_filter_level_t diag_filter,
   pf_ar_t * ar,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   pf_exp_api_t const * exp_api;
   pf_exp_module_t const * module;
   pf_api_t * real_api;
   pf_slot_t * slot;
   uint16_t a;
   uint16_t m;

   if (ar == NULL || ar->io_ar == NULL)
   {
      return;
   }
   for (a = 0; a < ar->io_ar->exp_ident.nbr_apis; ++a)
   {
      exp_api = &ar->io_ar->exp_ident.api[a];
      if (pf_cmdev_get_api (net, exp_api->api, &real_api) != 0)
      {
         continue; /* to the next API */
      }
      for (m = 0; m < exp_api->nbr_modules; ++m)
      {
         module = &exp_api->module[m];
         if (
            (pf_cmdev_get_slot (real_api, module->slot_number, &slot) != 0) ||
            (slot->module->ident_number != module->ident_number))
         {
            continue; /* to the next module */
         }
         pf_put_diag_ar_slot (
            net,
            big_endian,
            diag_filter,
            real_api->api_id,
            slot,
            module,
            p_info,
            pos);
      }
   }
}

void pf_put_diagnosis_api (
   pnet_t * net,
   bool big_endian,
   pf_diag_filter_level_t diag_filter,
   uint32_t api,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   pf_api_t * real_api;

   if (pf_cmdev_get_api (net, api, &real_api) == 0)
   {
      pf_put_diag_api (
         net,
         big_endian,
         diag_filter,
         real_api,
         p_info,
         pos);
   }
}

void pf_put_diagnosis_device (
   pnet_t * net,
   bool big_endian,
   pf_diag_filter_level_t diag_filter,
   pf_put_info_t * p_info,
   uint16_t * pos)
{
   pf_device_t * device;
   pf_api_t * api;

   device = NULL;
   if (pf_cmdev_get_device (net, &device) == 0)
   {
      for (api = device->real_ident.api_list; api != NULL; api = api->next)
      {
         pf_put_diag_api (net, big_endian, diag_filter, api, p_info, pos);
      }
   }
}

void pf_put_alarm_fixed (
   bool is_big_endian,
   const pf_alarm_fixed_t * p_alarm_fixed,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint32_t temp_u32;
   uint8_t temp_u8;

   /* The fixed part is not a "block" so do not insert a block header */

   /* Destination and source */
   pf_put_uint16 (
      is_big_endian,
      p_alarm_fixed->dst_ref,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_alarm_fixed->src_ref,
      p_info,
      p_pos);

   /* Flags for PDU type, version, window size and transport ACK */
   temp_u32 = 0;
   pf_put_bits (p_alarm_fixed->pdu_type.type, 4, 0, &temp_u32);
   pf_put_bits (p_alarm_fixed->pdu_type.version, 4, 4, &temp_u32);
   temp_u8 = (uint8_t)temp_u32;
   pf_put_byte (temp_u8, p_info, p_pos);

   temp_u32 = 0;
   pf_put_bits (p_alarm_fixed->add_flags.window_size, 4, 0, &temp_u32);
   pf_put_bits (p_alarm_fixed->add_flags.tack, 1, 4, &temp_u32);
   temp_u8 = (uint8_t)temp_u32;
   pf_put_byte (temp_u8, p_info, p_pos);

   /* Sequence numbers for sending and ACK */
   pf_put_uint16 (
      is_big_endian,
      p_alarm_fixed->send_seq_num,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_alarm_fixed->ack_seq_nbr,
      p_info,
      p_pos);
}

/**
 * @internal
 * Insert maint_status into a buffer
 *
 * @param is_big_endian    In:    true if buffer is big-endian.
 * @param maint_status     In:    Maintenance status.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
void pf_put_maint_status (
   bool is_big_endian,
   uint32_t maint_status,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = 0;
   uint16_t block_len = 0;

   pf_put_uint16 (is_big_endian, PF_USI_MAINTENANCE, p_info, p_pos);

   block_pos = *p_pos;
   pf_put_block_header (
      is_big_endian,
      PF_BT_MAINTENANCE_ITEM,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_byte (0, p_info, p_pos);
   pf_put_byte (0, p_info, p_pos);

   pf_put_uint32 (is_big_endian, maint_status, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   /* Point to correct place */
   block_pos += offsetof (pf_block_header_t, block_length);
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_alarm_block (
   bool is_big_endian,
   pf_block_type_values_t bh_type,
   const pf_alarm_data_t * p_alarm_data,
   uint32_t maint_status,
   uint16_t payload_usi, /* pf_usi_values_t */
   uint16_t payload_len,
   const uint8_t * p_payload, /* Union of may types - see below */
   const pnet_pnio_status_t * p_status,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint32_t temp_u16;
   uint32_t temp_u32;

   /* Insert block header for the alarm block */
   pf_put_block_header (
      is_big_endian,
      bh_type,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   /* Insert alarm type */
   pf_put_uint16 (
      is_big_endian,
      p_alarm_data->alarm_type,
      p_info,
      p_pos);

   /* Insert API, slot and subslot */
   pf_put_uint32 (is_big_endian, p_alarm_data->api_id, p_info, p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_alarm_data->slot_nbr,
      p_info,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_alarm_data->subslot_nbr,
      p_info,
      p_pos);

   /* Insert module and submodule info for AlarmNotification, but not for
    * AlarmAck */
   if (
      bh_type == PF_BT_ALARM_NOTIFICATION_LOW ||
      bh_type == PF_BT_ALARM_NOTIFICATION_HIGH)
   {
      pf_put_uint32 (
         is_big_endian,
         p_alarm_data->module_ident,
         p_info,
         p_pos);
      pf_put_uint32 (
         is_big_endian,
         p_alarm_data->submodule_ident,
         p_info,
         p_pos);
   }

   /* Insert sequence number and boolean flags */
   temp_u32 = 0;
   pf_put_bits (p_alarm_data->sequence_number, 11, 0, &temp_u32);
   pf_put_bits (
      p_alarm_data->alarm_specifier.channel_diagnosis,
      1,
      11,
      &temp_u32);
   pf_put_bits (
      p_alarm_data->alarm_specifier.manufacturer_diagnosis,
      1,
      12,
      &temp_u32);
   pf_put_bits (
      p_alarm_data->alarm_specifier.submodule_diagnosis,
      1,
      13,
      &temp_u32);
   pf_put_bits (p_alarm_data->alarm_specifier.ar_diagnosis, 1, 15, &temp_u32);
   temp_u16 = (uint16_t)temp_u32;
   pf_put_uint16 (is_big_endian, temp_u16, p_info, p_pos);

   if (
      bh_type == PF_BT_ALARM_NOTIFICATION_LOW ||
      bh_type == PF_BT_ALARM_NOTIFICATION_HIGH)
   {
      /* AlarmNotification DATA message */

      switch (payload_usi)
      {
      case 0:
         /* Nothing */
         break;
      case PF_USI_MAINTENANCE:
         /* This is actually an error */
         LOG_ERROR (PNET_LOG, "BW(%d): Illegal USI\n", payload_usi);
         break;
      case PF_USI_CHANNEL_DIAGNOSIS:
      case PF_USI_EXTENDED_CHANNEL_DIAGNOSIS:
      case PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS:
         /* Insert a maintenance item before the diagnosis item */
         if (maint_status != 0)
         {
            pf_put_maint_status (
               is_big_endian,
               maint_status,
               p_info,
               p_pos);
         }
         pf_put_diag_item (
            is_big_endian,
            (pf_diag_item_t *)p_payload,
            true, /* Insert the USI field */
            p_info,
            p_pos);
         break;
      default:
         /* Manufacturer data */
         /* Starts with the USI and then followed by raw data bytes */
         pf_put_uint16 (is_big_endian, payload_usi, p_info, p_pos);
         if (payload_len > 0)
         {
            pf_put_mem (p_payload, payload_len, p_info, p_pos);
         }
         break;
      }
   }
   else
   {
      /* AlarmAck DATA message */
      pf_put_pnet_status (true, p_status, p_info, p_pos);
   }

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_substitute_data (
   bool is_big_endian,
   uint16_t sub_mode,
   uint8_t iocs_len,
   const uint8_t * p_iocs,
   uint8_t iops_len,
   const uint8_t * p_iops,
   uint16_t data_len,
   const uint8_t * p_data,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Now the substitution data - this is an inner block (ToDo: Make own
    * function) */
   pf_put_block_header (
      is_big_endian,
      PF_BT_SUBSTITUTE_VALUE,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint16 (is_big_endian, sub_mode, p_info, p_pos);
   pf_put_mem (p_iocs, iocs_len, p_info, p_pos);
   pf_put_mem (p_data, data_len, p_info, p_pos);
   pf_put_mem (p_iops, iops_len, p_info, p_pos);

   /* Insert the block length into the substitution data block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_output_data (
   bool is_big_endian,
   bool sub_active,
   uint8_t iocs_len,
   const uint8_t * p_iocs,
   uint8_t iops_len,
   const uint8_t * p_iops,
   uint16_t data_len,
   const uint8_t * p_data,
   uint16_t sub_mode,
   const uint8_t * p_sub_data,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint16_t substitute_active_flag = sub_active ? 1 : 0;

   /* Insert block header for the output block */
   pf_put_block_header (
      is_big_endian,
      PF_BT_RECORD_OUTPUT_DATA_OBJECT,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      substitute_active_flag,
      p_info,
      p_pos);

   pf_put_byte (iocs_len, p_info, p_pos);
   pf_put_byte (iops_len, p_info, p_pos);
   pf_put_uint16 (is_big_endian, data_len, p_info, p_pos);

   pf_put_mem (p_iocs, iocs_len, p_info, p_pos);
   pf_put_mem (p_data, data_len, p_info, p_pos);
   pf_put_mem (p_iops, iops_len, p_info, p_pos);

   /* -------------- */
   if (p_sub_data != NULL)
   {
      pf_put_substitute_data (
         is_big_endian,
         sub_mode,
         iocs_len,
         p_iocs,
         iops_len,
         p_iops,
         data_len,
         p_sub_data,
         p_info,
         p_pos);
   }
   /* -------------- */

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_input_data (
   bool is_big_endian,
   uint8_t iocs_len,
   const uint8_t * p_iocs,
   uint8_t iops_len,
   const uint8_t * p_iops,
   uint16_t data_len,
   const uint8_t * p_data,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Insert block header for the output block */
   pf_put_block_header (
      is_big_endian,
      PF_BT_RECORD_INPUT_DATA_OBJECT,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_byte (iocs_len, p_info, p_pos);
   pf_put_mem (p_iocs, iocs_len, p_info, p_pos);

   pf_put_byte (iops_len, p_info, p_pos);
   pf_put_mem (p_iops, iops_len, p_info, p_pos);

   pf_put_uint16 (is_big_endian, data_len, p_info, p_pos);
   pf_put_mem (p_data, data_len, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_pdport_data_check (
   bool is_big_endian,
   uint16_t slot,
   uint16_t subslot,
   const pf_check_peer_t * check_peer,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_PDPORTCHECK,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* Slot and subslot info */
   pf_put_uint16 (is_big_endian, slot, p_info, p_pos);
   pf_put_uint16 (is_big_endian, subslot, p_info, p_pos);

   /* PF_BT_CHECKPEERS Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_CHECKPEERS,
      5 + check_peer->length_peer_port_name +
         check_peer->length_peer_station_name,
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   /* Number of Peers */
   pf_put_byte (1, p_info, p_pos);

   /* Length peer_id */
   pf_put_byte (check_peer->length_peer_port_name, p_info, p_pos);

   /* peer_id */
   pf_put_mem (
      &check_peer->peer_port_name,
      check_peer->length_peer_port_name,
      p_info,
      p_pos);

   /* Length ChassisID */
   pf_put_byte (check_peer->length_peer_station_name, p_info, p_pos);

   /* ChassisID */
   pf_put_mem (
      &check_peer->peer_station_name,
      check_peer->length_peer_station_name,
      p_info,
      p_pos);

   pf_put_padding_align (block_pos, 4, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_pdport_data_real (
   bool is_big_endian,
   uint16_t subslot,
   const pf_lldp_station_name_t * p_peer_station_name,
   const pf_lldp_port_name_t * p_peer_port_name,
   const pf_port_t * p_port_data,
   pf_mediatype_values_t media_type,
   const pnal_eth_status_t * p_eth_status,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint16_t link_state = 0;
   uint8_t num_peers = p_port_data->lldp.is_peer_info_received ? 1 : 0;
   const pf_lldp_peer_info_t * p_peer_info = &p_port_data->lldp.peer_info;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_PDPORTDATAREAL,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* Slot and subslot */
   pf_put_uint16 (is_big_endian, PNET_SLOT_DAP_IDENT, p_info, p_pos);
   pf_put_uint16 (is_big_endian, subslot, p_info, p_pos);

   /* Length OwnPortName */
   pf_put_byte (strlen (p_port_data->port_name), p_info, p_pos);

   /* OwnPortName */
   pf_put_mem (
      p_port_data->port_name,
      strlen (p_port_data->port_name),
      p_info,
      p_pos);

   /* Number of Peers */
   pf_put_byte (num_peers, p_info, p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* Peer info */
   if (num_peers > 0)
   {
      /* Peer port name */
      pf_put_byte (p_peer_port_name->len, p_info, p_pos);
      pf_put_mem (
         p_peer_port_name->string,
         p_peer_port_name->len,
         p_info,
         p_pos);

      /* Peer station name */
      pf_put_byte (p_peer_station_name->len, p_info, p_pos);
      pf_put_mem (
         p_peer_station_name->string,
         p_peer_station_name->len,
         p_info,
         p_pos);

      pf_put_padding_align (block_pos, 4, p_info, p_pos);

      /* Line Delay */
      pf_put_uint32 (
         is_big_endian,
         p_peer_info->line_delay,
         p_info,
         p_pos);

      /* Peer MAC Addr */
      pf_put_mem (
         p_peer_info->mac_address.addr,
         sizeof (p_peer_info->mac_address.addr),
         p_info,
         p_pos);

      pf_put_padding (2, p_info, p_pos);
   } /* Peer info */

   /* MAUType */
   pf_put_uint16 (
      is_big_endian,
      p_eth_status->operational_mau_type,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* Domain Boundary */
   /* Todo- hardcoded value*/
   pf_put_uint32 (is_big_endian, 0, p_info, p_pos);

   /* Multicast Boundary */
   /* Todo- hardcoded value*/
   pf_put_uint32 (is_big_endian, 0, p_info, p_pos);

   /* PN-AL-protocol (Mar20) section 5.2.13.23 for LinkState encoding */
   /* LinkState.Link */
   if (p_eth_status->running)
   {
      link_state = PF_PD_LINK_STATE_LINK_UP;
   }
   else
   {
      link_state = PF_PD_LINK_STATE_LINK_DOWN;
   }

   /* LinkState.Port */
   link_state |= (PF_PD_LINK_STATE_PORT_UNKNOWN << 8);

   pf_put_uint16 (is_big_endian, link_state, p_info, p_pos);

   pf_put_padding (2, p_info, p_pos);

   pf_put_uint32 (is_big_endian, media_type, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

static void pf_put_pdport_own_port (
   bool is_big_endian,
   const pf_port_t * p_port_data,
   pf_mediatype_values_t media_type,
   const pnal_eth_status_t * p_eth_status,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint16_t link_state = 0;
   pf_lldp_signal_delay_t line_delays;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_PDPORTOWNPORT,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* Length OwnPortName */
   pf_put_byte (strlen (p_port_data->port_name), p_info, p_pos);

   /* OwnPortName */
   pf_put_mem (
      p_port_data->port_name,
      strlen (p_port_data->port_name),
      p_info,
      p_pos);

   /* Pad with zeroes until block length is a multiple of 4 bytes */
   pf_put_padding_align (block_pos, 4, p_info, p_pos);

   /* LineDelay (locally calculated) */
   /* TODO: pf_lldp_get_signal_delays currently takes but does not use net */
   pf_lldp_get_signal_delays (NULL, p_port_data->port_num, &line_delays);
   pf_put_uint32 (
      is_big_endian,
      line_delays.cable_delay_local,
      p_info,
      p_pos);

   /* MediaType */
   pf_put_uint32 (is_big_endian, media_type, p_info, p_pos);

   /* MulticastBoundary */
   /* Todo- hardcoded value*/
   pf_put_uint32 (is_big_endian, 0, p_info, p_pos);

   /* MAUType */
   pf_put_uint16 (
      is_big_endian,
      p_eth_status->operational_mau_type,
      p_info,
      p_pos);

   /* MAUTypeExtension */
   pf_put_uint16 (
      is_big_endian,
      p_eth_status->mau_type_extension,
      p_info,
      p_pos);

   /* LinkState */
   /* PN-AL-protocol (Mar20) section 5.2.13.23 for LinkState encoding */
   /* LinkState.Link */
   if (p_eth_status->running)
   {
      link_state = PF_PD_LINK_STATE_LINK_UP;
   }
   else
   {
      link_state = PF_PD_LINK_STATE_LINK_DOWN;
   }

   /* LinkState.Port */
   link_state |= (PF_PD_LINK_STATE_PORT_UNKNOWN << 8);

   pf_put_uint16 (is_big_endian, link_state, p_info, p_pos);

   /* RTClass3_PortStatus */
   /* Todo- hardcoded value*/
   pf_put_byte (0, p_info, p_pos);

   /* Pad with zeroes until block length is a multiple of 4 bytes */
   pf_put_padding_align (block_pos, 4, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

static void pf_put_pdport_neighbours (
   bool is_big_endian,
   const pf_lldp_station_name_t * p_peer_station_name,
   const pf_lldp_port_name_t * p_peer_port_name,
   const pf_port_t * p_port_data,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint8_t num_peers = p_port_data->lldp.is_peer_info_received ? 1 : 0;
   const pf_lldp_peer_info_t * p_peer_info = &p_port_data->lldp.peer_info;
   uint16_t i;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_PDPORTNEIGHBOURS,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* NumberOfPeers */
   pf_put_byte (num_peers, p_info, p_pos);

   /* Pad with zeroes until block length is a multiple of 4 bytes */
   pf_put_padding_align (block_pos, 4, p_info, p_pos);

   /* TODO: fix for multiple peers, not just local peer (also pdport_real) */
   for (i = 0; i < num_peers; ++i)
   {
      /* LineDelay (from LLDP) */
      pf_put_uint32 (
         is_big_endian,
         p_peer_info->line_delay,
         p_info,
         p_pos);

      /* MAUType */
      pf_put_uint16 (
         is_big_endian,
         p_peer_info->phy_config.operational_mau_type,
         p_info,
         p_pos);

      /* MAUTypeExtension */
      pf_put_uint16 (
         is_big_endian,
         p_peer_info->mau_type_extension,
         p_info,
         p_pos);

      /* Peer MAC Addr */
      pf_put_mem (
         p_peer_info->mac_address.addr,
         sizeof (p_peer_info->mac_address.addr),
         p_info,
         p_pos);

      /* Peer port name */
      pf_put_byte (p_peer_port_name->len, p_info, p_pos);
      pf_put_mem (
         p_peer_port_name->string,
         p_peer_port_name->len,
         p_info,
         p_pos);

      /* Peer station name */
      pf_put_byte (p_peer_station_name->len, p_info, p_pos);
      pf_put_mem (
         p_peer_station_name->string,
         p_peer_station_name->len,
         p_info,
         p_pos);

      /* Pad with zeroes until block length is a multiple of 4 bytes */
      pf_put_padding_align (block_pos, 4, p_info, p_pos);
   }

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_pdport_data_real_extended (
   bool is_big_endian,
   uint16_t subslot,
   const pf_lldp_station_name_t * p_peer_station_name,
   const pf_lldp_port_name_t * p_peer_port_name,
   const pf_port_t * p_port_data,
   pf_mediatype_values_t media_type,
   const pnal_eth_status_t * p_eth_status,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint8_t num_peers = p_port_data->lldp.is_peer_info_received ? 1 : 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_PDPORTDATAREALEXT,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* Slot and subslot */
   pf_put_uint16 (is_big_endian, PNET_SLOT_DAP_IDENT, p_info, p_pos);
   pf_put_uint16 (is_big_endian, subslot, p_info, p_pos);

   /* OwnPort */
   pf_put_pdport_own_port (
      is_big_endian,
      p_port_data,
      media_type,
      p_eth_status,
      p_info,
      p_pos);

   /* Neighbours */
   if (num_peers > 0)
   {
      pf_put_pdport_neighbours (
         is_big_endian,
         p_peer_station_name,
         p_peer_port_name,
         p_port_data,
         p_info,
         p_pos);
   }

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_pdport_mrp_data_adjust (
   bool is_big_endian,
   uint16_t subslot,
   uint8_t mrp_instance,
   const mrp_domain_uuid_t * domain,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_PDPORT_MRP_DATA_ADJUST,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_HIGH,
      p_info,
      p_pos);

   pf_put_padding (1, p_info, p_pos);
   pf_put_byte (mrp_instance, p_info, p_pos);

   pf_put_mem (
      domain->data,
      sizeof (mrp_domain_uuid_t),
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_pdport_mrp_data_real (
   bool is_big_endian,
   uint16_t subslot,
   uint8_t mrp_instance,
   const mrp_domain_uuid_t * domain,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_PDPORT_MRP_DATA_REAL,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_HIGH,
      p_info,
      p_pos);

   pf_put_padding (1, p_info, p_pos);

   pf_put_byte (mrp_instance, p_info, p_pos);
   pf_put_mem (
      domain->data,
      sizeof (mrp_domain_uuid_t),
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_pdport_statistics (
   bool is_big_endian,
   const pnal_port_stats_t * p_port_stats,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_PORT_STATISTICS,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   pf_put_uint32 (
      is_big_endian,
      p_port_stats->if_in_octets,
      p_info,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_port_stats->if_out_octets,
      p_info,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_port_stats->if_in_discards,
      p_info,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_port_stats->if_out_discards,
      p_info,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_port_stats->if_in_errors,
      p_info,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_port_stats->if_out_errors,
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_pdinterface_data_real (
   bool is_big_endian,
   const pnet_ethaddr_t * mac_address,
   pnal_ipaddr_t ip_address,
   pnal_ipaddr_t netmask,
   pnal_ipaddr_t gateway,
   const char * station_name,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_INTERFACE_REAL_DATA,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   /* Station name */
   /* If station name length is 0, maybe mac string shall be used instead */
   pf_put_byte ((uint8_t)strlen (station_name), p_info, p_pos);
   pf_put_mem (station_name, strlen (station_name), p_info, p_pos);

   pf_put_padding_align (block_pos, 4, p_info, p_pos);

   /* DAP interface MAC address */
   pf_put_mem (
      &mac_address->addr,
      sizeof (pnet_ethaddr_t),
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* IP Address */
   pf_put_uint32 (is_big_endian, ip_address, p_info, p_pos);

   /* Subnet Mask */
   pf_put_uint32 (is_big_endian, netmask, p_info, p_pos);

   /* Router  */
   pf_put_uint32 (is_big_endian, gateway, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

/*
   PDInterface- MrpDataReal with BlockVersionLow = 1

   Special case MRP_Role := “Media Redundancy disabled”:
      BlockHeader, Padding, Padding, MRP_DomainUUID, MRP_Role, MRP_Version,
     MRP_LengthDomainName, MRP_DomainName, [Padding*] a01

   Special case MRP_Role := “Media Redundancy Client”:
      BlockHeader, Padding, Padding, MRP_DomainUUID, MRP_Role, MRP_Version,
      MRP_LengthDomainName, MRP_DomainName, [Padding*] a, MrpClientParams,
      [Padding*] a

   BlockHeader, Padding, MRP_DomainUUID
*/

void pf_put_pdinterface_mrp_data_real (
   bool is_big_endian,
   const mrp_domain_uuid_t * domain,
   mrp_role_t role,
   mrp_version_t version,
   const mrp_domain_name_t * domain_name,
   const pf_mrp_client_params_t * client_params,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_INTERFACE_MRP_DATA_REAL,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_HIGH,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);
   pf_put_mem (
      domain->data,
      sizeof (mrp_domain_uuid_t),
      p_info,
      p_pos);
   pf_put_uint16 (is_big_endian, role, p_info, p_pos);
   pf_put_uint16 (is_big_endian, version, p_info, p_pos);

   pf_put_byte (domain_name->len, p_info, p_pos);
   pf_put_mem (domain_name->name, domain_name->len, p_info, p_pos);

   /* Pad with zeroes until block length is a multiple of 4 bytes */
   pf_put_padding_align (block_pos, 4, p_info, p_pos);

   if (role == MEDIA_REDUNDANCY_CLIENT)
   {
      /* special case, add client redundancy parameters block */
      uint16_t client_block_pos = *p_pos;

      pf_put_block_header (
         is_big_endian,
         PF_BT_MRP_CLIENT_PARAMS,
         0, /* Dont know block_len yet */
         PNET_BLOCK_VERSION_HIGH,
         PNET_BLOCK_VERSION_LOW,
         p_info,
         p_pos);

      pf_put_uint16 (
         is_big_endian,
         client_params->link_down_T,
         p_info,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         client_params->link_up_T,
         p_info,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         client_params->link_nr_max,
         p_info,
         p_pos);

      /* Pad with zeroes until block length is a multiple of 4 bytes */
      pf_put_padding_align (block_pos, 4, p_info, p_pos);

      /* Finally insert the block length into the block header */
      block_len = *p_pos - (client_block_pos + 4);
      client_block_pos += offsetof (pf_block_header_t, block_length); /* Point
                                                                  to correct
                                                                  place */
      pf_put_uint16 (
         is_big_endian,
         block_len,
         p_info,
         &client_block_pos);
   }

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_pd_multiblock_interface_and_statistics (
   bool is_big_endian,
   uint32_t api,
   const pnet_ethaddr_t * mac_address,
   pnal_ipaddr_t ip_address,
   pnal_ipaddr_t netmask,
   pnal_ipaddr_t gateway,
   const char * station_name,
   const pnal_port_stats_t * p_port_statistics,
   const pf_mrp_config_t * mrp_real,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_MULTIPLEBLOCK_HEADER,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* API */
   pf_put_uint32 (is_big_endian, api, p_info, p_pos);

   /* Slot and subslot */
   pf_put_uint16 (is_big_endian, PNET_SLOT_DAP_IDENT, p_info, p_pos);
   pf_put_uint16 (
      is_big_endian,
      PNET_SUBSLOT_DAP_INTERFACE_1_IDENT,
      p_info,
      p_pos);

   /*PDInterfaceDataReal*/
   pf_put_pdinterface_data_real (
      is_big_endian,
      mac_address,
      ip_address,
      netmask,
      gateway,
      station_name,
      p_info,
      p_pos);

   /* PDInterfaceMrpDataReal */
   if (mrp_real)
   {
      pf_put_pdinterface_mrp_data_real (
         true,
         &mrp_real->domain,
         mrp_real->role,
         mrp_real->version,
         &mrp_real->domain_name,
         &mrp_real->client_params,
         p_info,
         p_pos);
   }

   /*PDPortStatistics*/
   pf_put_pdport_statistics (
      is_big_endian,
      p_port_statistics,
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_pd_multiblock_port_and_statistics (
   bool is_big_endian,
   uint32_t api,
   uint16_t subslot,
   const pf_lldp_station_name_t * p_peer_station_name,
   const pf_lldp_port_name_t * p_peer_port_name,
   const pf_port_t * p_port_data,
   pf_mediatype_values_t media_type,
   const pnal_eth_status_t * p_eth_status,
   const pnal_port_stats_t * p_port_statistics,
   const pf_mrp_port_t * p_mrp_port,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_MULTIPLEBLOCK_HEADER,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* API */
   pf_put_uint32 (is_big_endian, api, p_info, p_pos);

   /* Slot and subslot */
   pf_put_uint16 (is_big_endian, PNET_SLOT_DAP_IDENT, p_info, p_pos);
   pf_put_uint16 (is_big_endian, subslot, p_info, p_pos);

   /*PDPortDataReal*/
   pf_put_pdport_data_real (
      true,
      subslot,
      p_peer_station_name,
      p_peer_port_name,
      p_port_data,
      media_type,
      p_eth_status,
      p_info,
      p_pos);

   /*PDPortDataRealExtended*/
   pf_put_pdport_data_real_extended (
      true,
      subslot,
      p_peer_station_name,
      p_peer_port_name,
      p_port_data,
      media_type,
      p_eth_status,
      p_info,
      p_pos);

   if (p_mrp_port)
   {
      /* PDPortMrpDataReal */
      pf_put_pdport_mrp_data_real (
         true,
         subslot,
         p_mrp_port->instance,
         &p_mrp_port->domain,
         p_info,
         p_pos);
   }

   /*PDPortStatistics*/
   pf_put_pdport_statistics (
      is_big_endian,
      p_port_statistics,
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

/**
 * @internal
 * Insert peer to peer boundary
 * @param is_big_endian           In:    Endianness of the destination buffer.
 * @param p_peer_to_peer_boundary In:    The p-net stack instance
 * @param p_info                  InOut: Destination buffer state.
 * @param p_pos                   InOut: Position in destination buffer.
 */
void pf_put_peer_to_peer_boundary (
   bool is_big_endian,
   const pf_adjust_peer_to_peer_boundary_t * p_peer_to_peer_boundary,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint32_t temp_u32 = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_PEER_TO_PEER_BOUNDARY,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* Adjusted Peer to Peer Boundary */
   temp_u32 = 0;
   pf_put_bits (
      p_peer_to_peer_boundary->peer_to_peer_boundary.do_not_send_LLDP_frames,
      1,
      0,
      &temp_u32);
   pf_put_bits (
      p_peer_to_peer_boundary->peer_to_peer_boundary
         .do_not_send_pctp_delay_request_frames,
      1,
      1,
      &temp_u32);
   pf_put_bits (
      p_peer_to_peer_boundary->peer_to_peer_boundary
         .do_not_send_path_delay_request_frames,
      1,
      2,
      &temp_u32);

   pf_put_uint32 (is_big_endian, temp_u32, p_info, p_pos);

   /* Adjust Properties */
   pf_put_uint16 (
      is_big_endian,
      p_peer_to_peer_boundary->adjust_properties,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_pdport_data_adj (
   bool is_big_endian,
   uint16_t subslot,
   const pf_adjust_peer_to_peer_boundary_t * p_peer_to_peer_boundary,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_BOUNDARY_ADJUST,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* Slot and subslot */
   pf_put_uint16 (is_big_endian, PNET_SLOT_DAP_IDENT, p_info, p_pos);
   pf_put_uint16 (is_big_endian, subslot, p_info, p_pos);

   /* PeerToPeer_boundary*/
   pf_put_peer_to_peer_boundary (
      is_big_endian,
      p_peer_to_peer_boundary,
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

/* PF_BT_INTERFACE_MRP_DATA_CHECK */
void pf_put_intf_mrp_data_check (
   bool is_big_endian,
   uint16_t subslot,
   const pf_mrp_data_check_t * check,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint32_t temp_u32 = 0;
   int i;

   /* if MaxMRP_Instances is present in GSDML v1.1 should be used here */

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_INTERFACE_MRP_DATA_CHECK,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_HIGH,
      p_info,
      p_pos);

   pf_put_padding (1, p_info, p_pos);

   /* Number of entries */
   pf_put_byte (PF_MRP_NBR_INSTANCES, p_info, p_pos);

   for (i = 0; i < check->nbr_instances; i++)
   {
      uint16_t instance_block_pos = *p_pos;

      pf_put_block_header (
         is_big_endian,
         PF_BT_MRP_INSTANCE_DATA_CHECK,
         0, /* Dont know block_len yet */
         PNET_BLOCK_VERSION_HIGH,
         PNET_BLOCK_VERSION_LOW,
         p_info,
         p_pos);
      pf_put_padding (1, p_info, p_pos);

      /* instance */
      pf_put_byte (i, p_info, p_pos);

      /* domain uuid */
      pf_put_mem (
         check->instance[i].domain.data,
         sizeof (mrp_domain_uuid_t),
         p_info,
         p_pos);

      /* mrp check flags */
      pf_put_bits (
         check->instance[i].flags.redundance_mgr_diag,
         1,
         0,
         &temp_u32);
      pf_put_bits (check->instance[i].flags.lldp_domain, 1, 1, &temp_u32);
      pf_put_uint32 (is_big_endian, temp_u32, p_info, p_pos);

      /* Finally insert the block length into the block header */
      block_len = *p_pos - (instance_block_pos + 4);
      instance_block_pos += offsetof (pf_block_header_t, block_length); /* Point
                                                                  to correct
                                                                  place */
      pf_put_uint16 (
         is_big_endian,
         block_len,
         p_info,
         &instance_block_pos);
   }

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

/* PF_BT_INTERFACE_MRP_DATA_ADJUST v1.1 */

void pf_put_intf_mrp_data_adjust (
   bool is_big_endian,
   uint16_t subslot,
   const pf_mrp_config_t * p_mrp,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   int i;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_INTERFACE_MRP_DATA_ADJUST,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_HIGH,
      p_info,
      p_pos);

   pf_put_padding (1, p_info, p_pos);

   /* Number of entries */
   pf_put_byte (PF_MRP_NBR_INSTANCES, p_info, p_pos);

   for (i = 0; i < PF_MRP_NBR_INSTANCES; i++)
   {
      uint16_t instance_block_pos = *p_pos;

      pf_put_block_header (
         is_big_endian,
         PF_BT_MRP_INSTANCE_DATA_ADJUST,
         0, /* Dont know block_len yet */
         PNET_BLOCK_VERSION_HIGH,
         PNET_BLOCK_VERSION_LOW,
         p_info,
         p_pos);

      pf_put_padding (1, p_info, p_pos);

      /* instance */
      pf_put_byte (i, p_info, p_pos);

      /* domain uuid */
      pf_put_mem (
         p_mrp->domain.data,
         sizeof (mrp_domain_uuid_t),
         p_info,
         p_pos);

      /* role */
      pf_put_uint16 (is_big_endian, p_mrp->role, p_info, p_pos);

      pf_put_padding (2, p_info, p_pos);

      /* domain name length */
      pf_put_byte (p_mrp->domain_name.len, p_info, p_pos);

      /* domain name */
      pf_put_mem (
         p_mrp->domain_name.name,
         p_mrp->domain_name.len,
         p_info,
         p_pos);

      /* client parameters (if enabled) */
      if (p_mrp->role == MEDIA_REDUNDANCY_CLIENT)
      {
         uint16_t client_param_block_pos = *p_pos;

         /* special case, add client redundancy parameters block */
         pf_put_block_header (
            is_big_endian,
            PF_BT_MRP_CLIENT_PARAMS,
            0, /* Dont know block_len yet */
            PNET_BLOCK_VERSION_HIGH,
            PNET_BLOCK_VERSION_LOW,
            p_info,
            p_pos);

         pf_put_uint16 (
            is_big_endian,
            p_mrp->client_params.link_down_T,
            p_info,
            p_pos);
         pf_put_uint16 (
            is_big_endian,
            p_mrp->client_params.link_up_T,
            p_info,
            p_pos);
         pf_put_uint16 (
            is_big_endian,
            p_mrp->client_params.link_nr_max,
            p_info,
            p_pos);

         /* Pad with zeroes until block length is a multiple of 4 bytes */
         // pf_put_padding_align (block_pos, 4, p_info, p_pos);

         /* Finally insert the block length into the block header */
         block_len = *p_pos - (client_param_block_pos + 4);
         client_param_block_pos +=
            offsetof (pf_block_header_t, block_length); /* Point to correct
                                              place */
         pf_put_uint16 (
            is_big_endian,
            block_len,
            p_info,
            &client_param_block_pos);
      }

      // pf_put_padding_align (block_pos, 4, p_info, p_pos);

      /* Finally insert the block length into the block header */
      block_len = *p_pos - (instance_block_pos + 4);
      instance_block_pos += offsetof (pf_block_header_t, block_length); /* Point
                                                                  to correct
                                                                  place */
      pf_put_uint16 (
         is_big_endian,
         block_len,
         p_info,
         &instance_block_pos);
   }

   // pf_put_padding_align (block_pos, 4, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

/* PF_BT_PDPORT_MRP_DATA_ADJUST */
void pf_put_pdport_mrp_data_adj (
   bool is_big_endian,
   uint16_t subslot,
   const mrp_domain_uuid_t * domain,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_PDPORT_MRP_DATA_ADJUST,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   pf_put_mem (
      domain->data,
      sizeof (mrp_domain_uuid_t),
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_pd_interface_adj (
   bool is_big_endian,
   uint16_t subslot,
   pf_lldp_name_of_device_mode_t name_of_device_mode,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint32_t temp_u32;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_INTERFACE_ADJUST,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_padding (2, p_info, p_pos);

   /* MultipleInterfaceMode */
   temp_u32 = 0;
   pf_put_bits (name_of_device_mode, 1, 0, &temp_u32);
   pf_put_uint32 (is_big_endian, temp_u32, p_info, p_pos);

   /* Pad with zeroes until block length is a multiple of 4 bytes */
   pf_put_padding_align (block_pos, 4, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

/**
 * Put rpc epm floor count into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param count            In:    Floor count.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_floor_count (
   bool is_big_endian,
   uint32_t count,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_uint16 (is_big_endian, count, p_info, p_pos);
}

/**
 * Put rpc epm floor 1 configuration into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_floor          In:    Floor configuration.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_floor_1_uuid (
   bool is_big_endian,
   const pf_floor_uuid_t * p_floor,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t lhs_length = (uint16_t) (
      sizeof (p_floor->protocol_id) + sizeof (p_floor->uuid) +
      sizeof (p_floor->version_major));

   pf_put_uint16 (is_big_endian, lhs_length, p_info, p_pos);

   pf_put_byte (p_floor->protocol_id, p_info, p_pos);

   pf_put_uuid (is_big_endian, &p_floor->uuid, p_info, p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_floor->version_major,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->version_minor)),
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_floor->version_minor,
      p_info,
      p_pos);
}

/**
 * Put rpc epm floor 2 configuration into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_floor          In:    Floor configuration.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_floor_2_uuid (
   bool is_big_endian,
   const pf_floor_uuid_t * p_floor,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_rpc_floor_1_uuid (is_big_endian, p_floor, p_info, p_pos);
}

/**
 * Put rpc epm floor 3 configuration into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_floor          In:    Floor configuration.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_floor_3_rpc (
   bool is_big_endian,
   const pf_floor_3_t * p_floor,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->protocol_id)),
      p_info,
      p_pos);

   pf_put_byte (p_floor->protocol_id, p_info, p_pos);

   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->version_minor)),
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_floor->version_minor,
      p_info,
      p_pos);
}

/**
 * Put rpc epm floor 4 configuration into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_floor          In:    Floor configuration.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_floor_4_udp (
   bool is_big_endian,
   const pf_floor_4_t * p_floor,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->protocol_id)),
      p_info,
      p_pos);

   pf_put_byte (p_floor->protocol_id, p_info, p_pos);

   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->port)),
      p_info,
      p_pos);

   pf_put_uint16 (is_big_endian, p_floor->port, p_info, p_pos);
}

/**
 * Put rpc epm floor 5 configuration into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_floor          In:    Floor configuration.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_floor_5_ip (
   bool is_big_endian,
   const pf_floor_5_t * p_floor,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->protocol_id)),
      p_info,
      p_pos);

   pf_put_byte (p_floor->protocol_id, p_info, p_pos);

   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->ip_address)),
      p_info,
      p_pos);

   pf_put_uint32 (is_big_endian, p_floor->ip_address, p_info, p_pos);
}

/**
 * Put rpc epm annotation into a buffer.
 * The annotation string is defined in
 * PN-AL-protocol (Mar20) Table 309 RPC Substitutions
 * RPC Annotation
 * Field             Type                 Section
 * DeviceType        VisibleString[25]    4.10.3.3.1
 * Blank
 * OrderID           VisibleString[20]    4.10.3.3.2
 * Blank
 * HWRevision        VisibleString[5]     4.10.3.3.3
 * Blank
 * SWRevisionPrefix  VisibleString[1]     4.10.3.3.4
 * SWRevision        VisibleString[9]     4.10.3.3.5
 * EndTerm           '\0'
 *
 * @param p_cfg            In:    Configuration data used to generate
 *                                annotation.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_epm_annotation (
   const pnet_cfg_t * p_cfg,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   char annotation_data[PF_RPC_EPM_ANNOTATION_LENGTH];

   snprintf (
      annotation_data,
      sizeof (annotation_data),
      /* clang-format off */
      "%-" STRINGIFY (PNET_PRODUCT_NAME_MAX_LEN) "s "
      "%-" STRINGIFY (PNET_ORDER_ID_MAX_LEN) "s "
      "%5u "
      "%c%3u%3u%3u",
      /* clang-format on */
      p_cfg->product_info.name,
      p_cfg->product_info.order_id,
      p_cfg->product_info.hw_revision,
      p_cfg->product_info.sw_revision.prefix,
      p_cfg->product_info.sw_revision.functional_enhancement,
      p_cfg->product_info.sw_revision.bug_fix,
      p_cfg->product_info.sw_revision.internal_change);

   pf_put_mem (
      annotation_data,
      sizeof (annotation_data),
      p_info,
      p_pos);
}

/**
 * Put rpc epm protocol tower into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_tower          In:    Protocol tower configuration.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_tower_entry (
   bool is_big_endian,
   const pf_rpc_tower_t * p_tower,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_uint32 (
      is_big_endian,
      PF_RPC_TOWER_REFERENTID,
      p_info,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      PF_RPC_EPM_ANNOTATION_OFFSET,
      p_info,
      p_pos);

   pf_put_uint32 (
      is_big_endian,
      PF_RPC_EPM_ANNOTATION_LENGTH,
      p_info,
      p_pos);

   pf_put_rpc_epm_annotation (p_tower->p_cfg, p_info, p_pos);

   pf_put_uint32 (
      is_big_endian,
      PF_RPC_EPM_FLOOR_LENGTH,
      p_info,
      p_pos);

   pf_put_uint32 (
      is_big_endian,
      PF_RPC_EPM_FLOOR_LENGTH,
      p_info,
      p_pos);

   pf_put_rpc_floor_count (
      is_big_endian,
      PF_RPC_TOWER_FLOOR_COUNT,
      p_info,
      p_pos);

   pf_put_rpc_floor_1_uuid (
      is_big_endian,
      &p_tower->floor_1,
      p_info,
      p_pos);

   pf_put_rpc_floor_2_uuid (
      is_big_endian,
      &p_tower->floor_2,
      p_info,
      p_pos);

   pf_put_rpc_floor_3_rpc (
      is_big_endian,
      &p_tower->floor_3,
      p_info,
      p_pos);

   pf_put_rpc_floor_4_udp (
      is_big_endian,
      &p_tower->floor_4,
      p_info,
      p_pos);

   pf_put_rpc_floor_5_ip (
      is_big_endian,
      &p_tower->floor_5,
      p_info,
      p_pos);
}

/**
 * Put rpc handle into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_handle         In:    Rpc handle
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_handle (
   bool is_big_endian,
   const pf_rpc_handle_t * p_handle,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_uint32 (
      is_big_endian,
      p_handle->rpc_entry_handle,
      p_info,
      p_pos);

   pf_put_uuid (
      is_big_endian,
      &p_handle->handle_uuid,
      p_info,
      p_pos);
}

/**
 * Put rpc epm entry into a buffer. If no no protocol tower is
 * configured / actual_count == 0, a empty entry is written.
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_entry          In:    Rpc epm entry including protocol
 *                                tower configuration.
 * @param ínfo             InOut: Destination buffer state.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_epm_entry (
   bool is_big_endian,
   const pf_rpc_entry_t * p_entry,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_uint32 (is_big_endian, p_entry->max_count, p_info, p_pos);

   pf_put_uint32 (is_big_endian, p_entry->offset, p_info, p_pos);

   pf_put_uint32 (is_big_endian, p_entry->actual_count, p_info, p_pos);

   /* Add entry if not empty */
   if (p_entry->actual_count > 0)
   {
      pf_put_uuid (
         is_big_endian,
         &p_entry->object_uuid,
         p_info,
         p_pos);

      pf_put_tower_entry (
         is_big_endian,
         &p_entry->tower_entry,
         p_info,
         p_pos);
   }
}

void pf_put_lookup_response_data (
   pnet_t * net,
   bool is_big_endian,
   const pf_rpc_lookup_rsp_t * p_lookup_rsp,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_position = *p_pos;
   pf_put_rpc_handle (
      is_big_endian,
      &p_lookup_rsp->rpc_handle,
      p_info,
      p_pos);

   pf_put_uint32 (
      is_big_endian,
      p_lookup_rsp->num_entry,
      p_info,
      p_pos);

   pf_put_rpc_epm_entry (
      is_big_endian,
      &p_lookup_rsp->rpc_entry,
      p_info,
      p_pos);

   pf_put_padding_align (block_position, 2, p_info, p_pos);

   pf_put_uint32 (
      is_big_endian,
      p_lookup_rsp->return_code,
      p_info,
      p_pos);
}

#if PNET_OPTION_SNMP && PNET_OPTION_SECURITY_LEVEL_1
void pf_put_snmp_cim_snmp_real (
   pnet_t * net,
   bool is_big_endian,
   pnal_snmp_control_mode_t snmp_control_mode,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_CIM_SNMP_REAL,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      (uint16_t)snmp_control_mode,
      p_info,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length);
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}

void pf_put_snmp_cim_snmp_adjust (
   pnet_t * net,
   bool is_big_endian,
   pnal_snmp_control_mode_t snmp_control_mode,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_CIM_SNMP_ADJUST,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      (uint16_t)snmp_control_mode,
      p_info,
      p_pos);

   /* community name lengths shall be 0 in read requests */
   pf_put_byte (0, p_info, p_pos);
   pf_put_byte (0, p_info, p_pos);

   pf_put_padding (4, p_info, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length);
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}
#endif /* PNET_OPTION_SNMP && PNET_OPTION_SECURITY_LEVEL_1 */

/**
 * @internal
 * Put Electric Power Device Data into a buffer.
 *
 * @param is_big_endian      In:    Endianness of the destination buffer.
 * @param slot_number        In:    The slot.
 * @param subslot_number     In:    The subslot.
 * @param p_info             InOut: Destination buffer state.
 * @param p_pos              InOut: Position in destination buffer.
 *
 * See PN-AL-Protocol, 5.1.2 Table 571 "IO APDU substitutions for CIM".
 */
static void pf_put_cim_station_element_id (
   bool is_big_endian,
   uint16_t slot_number,
   uint16_t subslot_number,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   pf_put_uint16 (is_big_endian, slot_number, p_info, p_pos);
   pf_put_uint16 (is_big_endian, subslot_number, p_info, p_pos);
}


/**
 * @internal
 * Put Electric Power Device Data into a buffer.
 *
 * See PN-AL-Protocol, section "Coding of the field ElectricPowerDeviceVoltage".
 *
 * @param is_big_endian      In:    Endianness of the destination buffer.
 * @param device_data        In:    Device electric data.
 * @param slot_number        In:    The slot.
 * @param subslot_number     In:    The subslot.
 * @param p_info             InOut: Destination buffer state.
 * @param p_pos              InOut: Position in destination buffer.
 */
static void pf_put_cim_electric_power_device_data (
   bool is_big_endian,
   const pf_electric_power_device_data_t * device_data,
   uint16_t slot_number,
   uint16_t subslot_number,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint32_t electric_power_device_voltage = 0;
   pf_put_bits (
      device_data->device_voltage.voltage,
      20,
      0,
      &electric_power_device_voltage);
   pf_put_bits (
      device_data->device_voltage.type,
      4,
      28,
      &electric_power_device_voltage);

   pf_put_cim_station_element_id (
      is_big_endian,
      slot_number,
      subslot_number,
      p_info,
      p_pos);

   pf_put_uint32 (
      is_big_endian,
      electric_power_device_voltage,
      p_info,
      p_pos);
}

/**
 * @internal
 * Put Electric Power Port Data into a buffer.
 *
 * See PN-AL-Protocol, section "Coding of the field ElectricPowerPortVoltage".
 *
 * @param is_big_endian      In:    Endianness of the destination buffer.
 * @param port_data          In:    Port electric data.
 * @param slot_number        In:    The slot.
 * @param subslot_number     In:    The subslot.
 * @param p_info             InOut: Destination buffer state.
 * @param p_pos              InOut: Position in destination buffer.
 */
static void pf_put_cim_electric_power_port_data (
   bool is_big_endian,
   const pf_electric_power_port_data_t * port_data,
   uint16_t slot_number,
   uint16_t subslot_number,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint32_t electric_power_port_voltage = 0;
   uint32_t electric_power_port_current = 0;
   pf_put_bits (
      port_data->port_voltage.voltage,
      20,
      0,
      &electric_power_port_voltage);
   pf_put_bits (
      port_data->port_voltage.type,
      4,
      28,
      &electric_power_port_voltage);

   pf_put_bits (
      port_data->port_current.current,
      16,
      0,
      &electric_power_port_current);
   pf_put_bits (
      port_data->port_current.current_limit,
      16,
      16,
      &electric_power_port_current);

   pf_put_cim_station_element_id (
      is_big_endian,
      slot_number,
      subslot_number,
      p_info,
      p_pos);

   pf_put_uint32 (
      is_big_endian,
      electric_power_port_voltage,
      p_info,
      p_pos);

   pf_put_uint32 (
      is_big_endian,
      electric_power_port_current,
      p_info,
      p_pos);
}

void pf_put_cim_electric_power_real (
   bool is_big_endian,
   const pf_electric_power_device_data_t * device_data,
   uint16_t num_ports,
   const pf_electric_power_port_data_t * port_entries,
   uint16_t slot_number,
   uint16_t subslot_number,
   pf_put_info_t * p_info,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint16_t i = 0;
   pf_electric_power_port_data_t default_port_data = {0};

   pf_put_block_header (
      is_big_endian,
      PF_BT_CIM_ELECTRIC_POWER_REAL,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      p_info,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      num_ports,
      p_info,
      p_pos);

   for (i = 0; i < num_ports; ++i)
   {
      if (port_entries[i].port_voltage.type == 0)
      {
         /* No electric data available */
         pf_put_cim_electric_power_port_data (
            is_big_endian,
            &default_port_data,
            slot_number,
            subslot_number,
            p_info,
            p_pos);
      }
      else
      {
         pf_put_cim_electric_power_port_data (
            is_big_endian,
            &port_entries[i],
            slot_number,
            subslot_number,
            p_info,
            p_pos);
      }
   }

   /* Mandatory for APL devices */
   if (device_data != NULL)
   {
      pf_put_cim_electric_power_device_data (
         is_big_endian,
         device_data,
         slot_number,
         subslot_number,
         p_info,
         p_pos);
   }

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, p_info, &block_pos);
}
