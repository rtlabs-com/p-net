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
 *       uint16_t res_len,
 *       uint8_t * p_bytes,
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
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_block_header (
   bool is_big_endian,
   pf_block_type_values_t bh_type,
   uint16_t bh_length,
   uint8_t bh_ver_high,
   uint8_t bh_ver_low,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t temp_u16 = (uint16_t)bh_type;

   pf_put_uint16 (is_big_endian, temp_u16, res_len, p_bytes, p_pos);
   pf_put_uint16 (
      is_big_endian,
      (bh_length + sizeof (pf_block_header_t)) - 4,
      res_len,
      p_bytes,
      p_pos); /* Always (-4)!! */
   pf_put_byte (bh_ver_high, res_len, p_bytes, p_pos);
   pf_put_byte (bh_ver_low, res_len, p_bytes, p_pos);
}

/**
 * @internal
 * Insert a string into a buffer.
 *
 * This function inserts a fixed sized string into a buffer.
 * If the string is shorter than the destination size then the destination
 * is padded with spaces.
 * The destination buffer does not contain any terminating NUL byte.
 * @param p_src            In:    The string.
 * @param src_size         In:    The destination string size.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_str (
   const void * p_src,
   uint16_t src_size,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t str_len = (uint16_t)strlen (p_src);
   uint16_t ix;

   if (((*p_pos) + src_size) > res_len)
   {
      /* Reached end of buffer */
      LOG_DEBUG (PNET_LOG, "BW(%d): Output buffer is full\n", __LINE__);
      p_bytes = NULL;
   }

   if (p_bytes != NULL)
   {
      memcpy (&p_bytes[*p_pos], p_src, str_len);
      (*p_pos) += str_len;
      /* Pad with spaces - size also includes NUL-terminator. Do not write into
       * that pos. */
      for (ix = str_len; ix < src_size - 1; ix++)
      {
         pf_put_byte (' ', res_len, p_bytes, p_pos);
      }
   }
}

/**
 * @internal
 * Insert a UUID into a buffer.
 * @param is_big_endian    In:    true if buffer is big-endian.
 * @param p_uuid           In:    The UUID to insert.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_uuid (
   bool is_big_endian,
   const pf_uuid_t * p_uuid,
   uint16_t res_len, /* Sizeof p_bytes buf */
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_put_uint32 (is_big_endian, p_uuid->data1, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_uuid->data2, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_uuid->data3, res_len, p_bytes, p_pos);
   pf_put_mem (p_uuid->data4, sizeof (p_uuid->data4), res_len, p_bytes, p_pos);
}

/**
 * Put rpc UUID into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_uuid           In:    The UUID to insert.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_uuid (
   bool is_big_endian,
   const pf_rpc_uuid_type_t * p_uuid,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_put_uint32 (is_big_endian, p_uuid->time_low, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_uuid->time_mid, res_len, p_bytes, p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_uuid->time_hi_and_version,
      res_len,
      p_bytes,
      p_pos);
   pf_put_byte (p_uuid->clock_hi_and_reserved, res_len, p_bytes, p_pos);
   pf_put_byte (p_uuid->clock_low, res_len, p_bytes, p_pos);
   pf_put_mem (p_uuid->node, sizeof (p_uuid->node), res_len, p_bytes, p_pos);
}
/* ======================== Public functions */

void pf_put_mem (
   const void * p_src,
   uint16_t src_size, /* Bytes to copy */
   uint16_t res_len,  /* Sizeof p_bytes buf */
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   if (((*p_pos) + src_size) > res_len)
   {
      /* Reached end of buffer */
      LOG_DEBUG (PNET_LOG, "BW(%d): Output buffer is full\n", __LINE__);
      p_bytes = NULL;
   }

   if (p_bytes != NULL)
   {
      memcpy (&p_bytes[*p_pos], p_src, src_size);
      (*p_pos) += src_size;
   }
}

void pf_put_mem_overlapping (
   const void * p_src,
   uint16_t src_size, /* Bytes to copy */
   uint16_t res_len,  /* Sizeof p_bytes buf */
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   if (((*p_pos) + src_size) > res_len)
   {
      /* Reached end of buffer */
      LOG_DEBUG (PNET_LOG, "BW(%d): Output buffer is full\n", __LINE__);
      p_bytes = NULL;
   }

   if (p_bytes != NULL)
   {
      memmove (&p_bytes[*p_pos], p_src, src_size);
      (*p_pos) += src_size;
   }
}

void pf_put_byte (
   uint8_t val,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   if (*p_pos + 1 > res_len)
   {
      /* Reached end of buffer */
      LOG_DEBUG (PNET_LOG, "BW(%d): End of buffer reached\n", __LINE__);
      p_bytes = NULL;
   }

   if (p_bytes != NULL)
   {
      p_bytes[*p_pos] = val;
      (*p_pos)++;
   }
}

void pf_put_uint16 (
   bool is_big_endian,
   uint16_t val,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   if (is_big_endian)
   {
      pf_put_byte ((val >> 8) & 0xff, res_len, p_bytes, p_pos);
      pf_put_byte (val & 0xff, res_len, p_bytes, p_pos);
   }
   else
   {
      pf_put_byte (val & 0xff, res_len, p_bytes, p_pos);
      pf_put_byte ((val >> 8) & 0xff, res_len, p_bytes, p_pos);
   }
}

void pf_put_uint32 (
   bool is_big_endian,
   uint32_t val,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   if (is_big_endian)
   {
      pf_put_byte ((val >> 24) & 0xff, res_len, p_bytes, p_pos);
      pf_put_byte ((val >> 16) & 0xff, res_len, p_bytes, p_pos);
      pf_put_byte ((val >> 8) & 0xff, res_len, p_bytes, p_pos);
      pf_put_byte (val & 0xff, res_len, p_bytes, p_pos);
   }
   else
   {
      pf_put_byte (val & 0xff, res_len, p_bytes, p_pos);
      pf_put_byte ((val >> 8) & 0xff, res_len, p_bytes, p_pos);
      pf_put_byte ((val >> 16) & 0xff, res_len, p_bytes, p_pos);
      pf_put_byte ((val >> 24) & 0xff, res_len, p_bytes, p_pos);
   }
}

void pf_put_padding (
   uint16_t n_bytes,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t i;
   for (i = 0; i < n_bytes; i++)
   {
      pf_put_byte (0, res_len, p_bytes, p_pos);
   }
}

void pf_put_padding_align (
   uint16_t start_position,
   uint16_t align,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   while (((*p_pos) - start_position) % align != 0)
   {
      pf_put_byte (0, res_len, p_bytes, p_pos);
   }
}

/**
 * @internal
 * Insert timestamp
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_time_ts        In:    The timestamp
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
void pf_put_time_timestamp (
   bool is_big_endian,
   const pf_log_book_ts_t * p_time_ts,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_put_uint16 (is_big_endian, p_time_ts->sec_hi, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_time_ts->sec_lo, res_len, p_bytes, p_pos);
   pf_put_uint32 (is_big_endian, p_time_ts->nano_sec, res_len, p_bytes, p_pos);
}

void pf_put_ar_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_param.ar_type,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uuid (is_big_endian, &p_ar->ar_param.ar_uuid, res_len, p_bytes, p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_param.session_key,
      res_len,
      p_bytes,
      p_pos);

   pf_put_mem (
      &p_ar->ar_result.cm_responder_mac_add,
      sizeof (p_ar->ar_result.cm_responder_mac_add),
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_result.responder_udp_rt_port,
      res_len,
      p_bytes,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_iocr_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t ix,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->iocrs[ix].result.iocr_type,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->iocrs[ix].result.iocr_reference,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->iocrs[ix].result.frame_id,
      res_len,
      p_bytes,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_alarm_cr_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->alarm_cr_result.alarm_cr_type,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->alarm_cr_result.remote_alarm_reference,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->alarm_cr_result.max_alarm_data_length,
      res_len,
      p_bytes,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
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
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_submodule_diff (
   bool is_big_endian,
   const pf_submodule_diff_t * p_diff,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint32_t temp_u32 = 0;
   uint16_t temp_u16 = 0;

   pf_put_uint16 (
      is_big_endian,
      p_diff->subslot_number,
      res_len,
      p_bytes,
      p_pos);

   if (p_diff->submodule_state.ident_info == PF_SUBMOD_PLUG_NO)
   {
      pf_put_uint32 (is_big_endian, 0, res_len, p_bytes, p_pos);
   }
   else
   {
      pf_put_uint32 (
         is_big_endian,
         p_diff->submodule_ident_number,
         res_len,
         p_bytes,
         p_pos);
   }
   /* submodule_state */
   pf_put_bits (p_diff->submodule_state.add_info, 3, 0, &temp_u32);
   pf_put_bits (p_diff->submodule_state.advice_avail, 1, 3, &temp_u32);
   pf_put_bits (p_diff->submodule_state.maintenance_required, 1, 4, &temp_u32);
   pf_put_bits (p_diff->submodule_state.maintenance_demanded, 1, 5, &temp_u32);
   pf_put_bits (p_diff->submodule_state.fault, 1, 6, &temp_u32);
   pf_put_bits (p_diff->submodule_state.ar_info, 4, 7, &temp_u32);
   pf_put_bits (p_diff->submodule_state.ident_info, 4, 11, &temp_u32);
   pf_put_bits (p_diff->submodule_state.format_indicator, 1, 15, &temp_u32);
   temp_u16 = (uint16_t)temp_u32;
   pf_put_uint16 (is_big_endian, temp_u16, res_len, p_bytes, p_pos);
}

/**
 * @internal
 * Insert a module diff into a buffer.
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_diff           In:    The module diff information.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_module_diff (
   bool is_big_endian,
   const pf_module_diff_t * p_diff,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t ix;

   pf_put_uint16 (is_big_endian, p_diff->slot_number, res_len, p_bytes, p_pos);
   if (p_diff->module_state == PF_MOD_PLUG_NO_MODULE)
   {
      pf_put_uint32 (is_big_endian, 0, res_len, p_bytes, p_pos);
   }
   else
   {
      pf_put_uint32 (
         is_big_endian,
         p_diff->module_ident_number,
         res_len,
         p_bytes,
         p_pos);
   }
   pf_put_uint16 (is_big_endian, p_diff->module_state, res_len, p_bytes, p_pos);
   if (p_diff->module_state == PF_MOD_PLUG_NO_MODULE)
   {
      pf_put_uint32 (is_big_endian, 0, res_len, p_bytes, p_pos);
   }
   else
   {
      pf_put_uint16 (
         is_big_endian,
         p_diff->nbr_submodule_diffs,
         res_len,
         p_bytes,
         p_pos);
      for (ix = 0; ix < p_diff->nbr_submodule_diffs; ix++)
      {
         pf_put_submodule_diff (
            is_big_endian,
            &p_diff->submodule_diffs[ix],
            res_len,
            p_bytes,
            p_pos);
      }
   }
}

/**
 * @internal
 * Insert AR diff blocks into a buffer.
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_ar             In:    Contains the AR diff to insert.
 * @param api_ix           In:    The specific AR diff to add.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_api_diff (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t api_ix,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t mod_ix;

   pf_put_uint32 (
      is_big_endian,
      p_ar->api_diffs[api_ix].api,
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->api_diffs[api_ix].nbr_module_diffs,
      res_len,
      p_bytes,
      p_pos);
   for (mod_ix = 0; mod_ix < p_ar->api_diffs[api_ix].nbr_module_diffs; mod_ix++)
   {
      pf_put_module_diff (
         is_big_endian,
         &p_ar->api_diffs[api_ix].module_diffs[mod_ix],
         res_len,
         p_bytes,
         p_pos);
   }
}

void pf_put_ar_diff (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint16_t api_ix;

   if ((p_ar != NULL) && (p_ar->nbr_api_diffs > 0))
   {
      pf_put_block_header (
         is_big_endian,
         PF_BT_MODULE_DIFF_BLOCK,
         0, /* Dont know block_len yet */
         PNET_BLOCK_VERSION_HIGH,
         PNET_BLOCK_VERSION_LOW,
         res_len,
         p_bytes,
         p_pos);

      pf_put_uint16 (
         is_big_endian,
         p_ar->nbr_api_diffs,
         res_len,
         p_bytes,
         p_pos);

      for (api_ix = 0; api_ix < p_ar->nbr_api_diffs; api_ix++)
      {
         pf_put_api_diff (is_big_endian, p_ar, api_ix, res_len, p_bytes, p_pos);
      }

      /* Finally insert the block length into the block header */
      block_len = *p_pos - (block_pos + 4);
      block_pos += offsetof (pf_block_header_t, block_length); /* Point to
                                                                  correct place
                                                                */
      pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
   }
}

void pf_put_ar_rpc_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_rpc_result.responder_rpc_server_port,
      res_len,
      p_bytes,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_ar_server_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_server.length_cm_responder_station_name,
      res_len,
      p_bytes,
      p_pos);
   pf_put_mem (
      p_ar->ar_server.cm_responder_station_name,
      p_ar->ar_server.length_cm_responder_station_name,
      res_len,
      p_bytes,
      p_pos);

   /* Pad with zeroes until block length is a multiple of 4 bytes */
   pf_put_padding_align (block_pos, 4, res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

#if PNET_OPTION_AR_VENDOR_BLOCKS
void pf_put_ar_vendor_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t ix,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint32 (
      is_big_endian,
      p_ar->ar_vendor_request[ix].ap_structure_identifier,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_ar->ar_vendor_request[ix].api,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_vendor_result[ix].length_data_response,
      res_len,
      p_bytes,
      p_pos);
   pf_put_mem (
      p_ar->ar_vendor_result[ix].data_response,
      p_ar->ar_vendor_result[ix].length_data_response,
      res_len,
      p_bytes,
      p_pos);

   /* Pad with zeroes until block length is a multiple of 4 bytes */
   pf_put_padding_align (block_pos, 4, res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}
#endif

/**
 * @internal
 * Insert an IOCR struct into a buffer.
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_ar             In:    The AR instance.
 * @param ix               In:    The IOCR indice.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_iocr (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t ix,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint32_t temp32 = 0;
   uint8_t data_status = 0;

   pf_put_bits (p_ar->iocrs[ix].param.iocr_properties.rt_class, 4, 0, &temp32);
   pf_put_uint32 (is_big_endian, temp32, res_len, p_bytes, p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->iocrs[ix].param.iocr_type,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->iocrs[ix].result.frame_id,
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (is_big_endian, 0, res_len, p_bytes, p_pos);
   if (
      (p_ar->iocrs[ix].param.iocr_type == PF_IOCR_TYPE_INPUT) ||
      (p_ar->iocrs[ix].param.iocr_type == PF_IOCR_TYPE_MC_PROVIDER))
   {
      (void)pf_ppm_get_data_status (&p_ar->iocrs[ix].ppm, &data_status);
   }
   else
   {
      (void)pf_cpm_get_data_status (&p_ar->iocrs[ix].cpm, &data_status);
   }
   pf_put_byte (data_status, res_len, p_bytes, p_pos);
   pf_put_byte (0, res_len, p_bytes, p_pos);
}

#if PNET_OPTION_IR
static void pf_put_ir_info (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   /* ToDo: Not yet implemented */
}
#endif

#if PNET_OPTION_SR
static void pf_put_sr_info (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   /* ToDo: Not yet implemented */
}
#endif

#if PNET_OPTION_AR_VENDOR_BLOCKS
static void pf_put_ar_vendor_block_req (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t ix,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   /* ToDo: Not yet implemented */
}
static void pf_put_ar_vendor_block_res (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t ix,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   /* ToDo: Not yet implemented */
}
#endif

#if PNET_OPTION_FAST_STARTUP
static void pf_put_fsu_data (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   /* ToDo: Not yet implemented */
}
#endif

#if PNET_OPTION_SRL
static void pf_put_srl_data (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   /* ToDo: Not yet implemented */
}
#endif
#if PNET_OPTION_RS
static void pf_put_rs_info (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
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
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_one_ar (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   bool api_filter,
   uint32_t api_id,
   uint16_t res_len,
   uint8_t * p_bytes,
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
#if PNET_OPTION_PARAMETER_SERVER == 0
   pf_uuid_t nil_uuid = {0};
#endif

   /* ar_param */
   pf_put_uuid (is_big_endian, &p_ar->ar_param.ar_uuid, res_len, p_bytes, p_pos);
   pf_put_uuid (
      is_big_endian,
      &p_ar->ar_param.cm_initiator_object_uuid,
      res_len,
      p_bytes,
      p_pos);
#if PNET_OPTION_PARAMETER_SERVER
   pf_put_uuid (
      is_big_endian,
      &p_ar->prm_server.parameter_server_object_uuid,
      res_len,
      p_bytes,
      p_pos);
#else
   pf_put_uuid (is_big_endian, &nil_uuid, res_len, p_bytes, p_pos);
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
   pf_put_uint32 (is_big_endian, temp32, res_len, p_bytes, p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_param.ar_type,
      res_len,
      p_bytes,
      p_pos);

   /* alarm_cr_request/response */
   pf_put_uint16 (
      is_big_endian,
      p_ar->alarm_cr_request.alarm_cr_type,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->alarm_cr_request.local_alarm_reference,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->alarm_cr_result.remote_alarm_reference,
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_param.cm_initiator_udp_rt_port,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_result.responder_udp_rt_port,
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_ar->ar_param.cm_initiator_station_name_len,
      res_len,
      p_bytes,
      p_pos);
   pf_put_mem (
      p_ar->ar_param.cm_initiator_station_name,
      p_ar->ar_param.cm_initiator_station_name_len,
      res_len,
      p_bytes,
      p_pos);
   while (((*p_pos) % sizeof (uint32_t)) != 0)
   {
      pf_put_byte (0, res_len, p_bytes, p_pos);
   }
#if PNET_OPTION_PARAMETER_SERVER
   pf_put_uint16 (
      is_big_endian,
      p_ar->prm_server.parameter_server_station_name_len,
      res_len,
      p_bytes,
      p_pos);
   pf_put_mem (
      p_ar->prm_server.parameter_server_station_name,
      p_ar->prm_server.parameter_server_station_name_len,
      res_len,
      p_bytes,
      p_pos);
#else
   pf_put_uint16 (is_big_endian, 0, res_len, p_bytes, p_pos);
#endif
   while (((*p_pos) % sizeof (uint32_t)) != 0)
   {
      pf_put_byte (0, res_len, p_bytes, p_pos);
   }

   pf_put_uint16 (is_big_endian, p_ar->nbr_iocrs, res_len, p_bytes, p_pos);
   while (((*p_pos) % sizeof (uint32_t)) != 0)
   {
      pf_put_byte (0, res_len, p_bytes, p_pos);
   }
   for (ix = 0; ix < p_ar->nbr_iocrs; ix++)
   {
      pf_put_iocr (is_big_endian, p_ar, ix, res_len, p_bytes, p_pos);
   }

   pf_put_uint16 (is_big_endian, p_ar->nbr_exp_apis, res_len, p_bytes, p_pos);
   while (((*p_pos) % sizeof (uint32_t)) != 0)
   {
      pf_put_byte (0, res_len, p_bytes, p_pos);
   }
   for (ix = 0; ix < p_ar->nbr_exp_apis; ix++)
   {
      if ((api_filter == false) || (p_ar->exp_apis[ix].api == api_id))
      {
         pf_put_uint32 (
            is_big_endian,
            p_ar->exp_apis[ix].api,
            res_len,
            p_bytes,
            p_pos);
      }
   }

   /* Remember pos so the correct value can be inserted later */
   nbr_ar_data_pos = *p_pos;
   pf_put_uint16 (is_big_endian, 0, res_len, p_bytes, p_pos);
   while (((*p_pos) % sizeof (uint32_t)) != 0)
   {
      pf_put_byte (0, res_len, p_bytes, p_pos);
   }

#if PNET_OPTION_IR
   if (p_ar->nbr_ir_info > 0)
   {
      pf_put_ir_info (is_big_endian, p_ar, res_len, p_bytes, p_pos);
      nbr_ar_data++;
   }
#endif
#if PNET_OPTION_SR
   if (p_ar->sr_info.valid == true)
   {
      pf_put_sr_info (is_big_endian, p_ar, res_len, p_bytes, p_pos);
      nbr_ar_data++;
   }
#endif
#if PNET_OPTION_AR_VENDOR_BLOCKS
   for (ix = 0; ix < p_ar->nbr_ar_vendor; ix++)
   {
      pf_put_ar_vendor_block_req (
         is_big_endian,
         p_ar,
         ix,
         res_len,
         p_bytes,
         p_pos);
      nbr_ar_data++;
   }
   for (ix = 0; ix < p_ar->nbr_ar_vendor; ix++)
   {
      pf_put_ar_vendor_block_res (
         is_big_endian,
         p_ar,
         ix,
         res_len,
         p_bytes,
         p_pos);
      nbr_ar_data++;
   }
#endif
#if PNET_OPTION_FAST_STARTUP
   if (p_ar->fast_startup_data.valid == true)
   {
      pf_put_fsu_data (is_big_endian, p_ar, res_len, p_bytes, p_pos);
      nbr_ar_data++;
   }
#endif
#if PNET_OPTION_SRL
   if (p_ar->srl_data.valid == true)
   {
      pf_put_srl_data (is_big_endian, p_ar, res_len, p_bytes, p_pos);
      nbr_ar_data++;
   }
#endif
#if PNET_OPTION_RS
   if (p_ar->rs_info.valid == true)
   {
      pf_put_rs_info (is_big_endian, p_ar, res_len, p_bytes, p_pos);
      nbr_ar_data++;
   }
#endif
   pf_put_uint16 (
      is_big_endian,
      nbr_ar_data,
      res_len,
      p_bytes,
      &nbr_ar_data_pos);
}

void pf_put_ar_data (
   pnet_t * net,
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   /*
   BlockHeader, NumberOfARs, (AR)*
   */
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   uint16_t data_pos;
   pf_device_t * p_device = NULL;
   uint16_t cnt;
   uint16_t ix;
   pf_ar_t * p_ar_tmp = NULL;

   pf_put_block_header (
      is_big_endian,
      PF_BT_AR_DATA,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW_1,
      res_len,
      p_bytes,
      p_pos);

   data_pos = *p_pos;

   if (pf_cmdev_get_device (net, &p_device) == 0)
   {
      cnt = 0;
      if (p_ar == NULL)
      {
         /* Count the number of ARs */
         for (ix = 0; ix < PNET_MAX_AR; ix++)
         {
            p_ar_tmp = pf_ar_find_by_index (net, ix);
            if (p_ar_tmp != NULL)
            {
               if (
                  (p_ar_tmp->ar_param.ar_uuid.data1 != 0) &&
                  (p_ar_tmp->ar_param.cm_initiator_object_uuid.data1 != 0))
               {
                  cnt++;
               }
            }
         }
      }
      else
      {
         /* Count the number of ARs using this API */
         for (ix = 0; ix < p_ar->nbr_exp_apis; ix++)
         {
            if (p_ar->exp_apis[ix].api == api_id)
            {
               cnt++;
            }
         }
      }
      if (cnt > 0)
      {
         pf_put_uint16 (is_big_endian, cnt, res_len, p_bytes, p_pos);
      }
      if (p_ar == NULL)
      {
         /* Insert the ARs */
         for (ix = 0; ix < PNET_MAX_AR; ix++)
         {
            p_ar_tmp = pf_ar_find_by_index (net, ix);
            if (p_ar_tmp != NULL)
            {
               if (
                  (p_ar_tmp->ar_param.ar_uuid.data1 != 0) &&
                  (p_ar_tmp->ar_param.cm_initiator_object_uuid.data1 != 0))
               {
                  pf_put_one_ar (
                     is_big_endian,
                     p_ar_tmp,
                     false,
                     0,
                     res_len,
                     p_bytes,
                     p_pos);
               }
            }
         }
      }
      else
      {
         /* Insert the ARs using this API */
         for (ix = 0; ix < p_ar->nbr_exp_apis; ix++)
         {
            if (p_ar->exp_apis[ix].api == api_id)
            {
               pf_put_one_ar (
                  is_big_endian,
                  p_ar,
                  true,
                  api_id,
                  res_len,
                  p_bytes,
                  p_pos);
            }
         }
      }
   }

   if (data_pos < *p_pos)
   {
      /* Finally insert the block length into the block header */
      block_len = *p_pos - (block_pos + 4);
      block_pos += offsetof (pf_block_header_t, block_length); /* Point to
                                                                  correct place
                                                                */
      pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
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
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_byte (0, res_len, p_bytes, p_pos);
   pf_put_byte (0, res_len, p_bytes, p_pos);

   pf_put_uuid (is_big_endian, &p_res->ar_uuid, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_res->session_key, res_len, p_bytes, p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_res->alarm_sequence_number,
      res_len,
      p_bytes,
      p_pos);

   /* Command and properties are always Big-Endian on the wire!! */
   pf_put_uint16 (
      is_big_endian,
      p_res->control_command,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_res->control_block_properties,
      res_len,
      p_bytes,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_pnet_status (
   bool is_big_endian,
   const pnet_pnio_status_t * p_status,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint32_t pnio_result;

   pnio_result = 0x01000000u * p_status->error_code +
                 0x10000u * p_status->error_decode +
                 0x100u * p_status->error_code_1 + p_status->error_code_2;
   pf_put_uint32 (is_big_endian, pnio_result, res_len, p_bytes, p_pos);
}

void pf_put_dce_rpc_header (
   pf_rpc_header_t * p_rpc,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos,
   uint16_t * p_pos_body_len)
{
   uint32_t temp_u32;

   pf_put_byte (p_rpc->version, res_len, p_bytes, p_pos);
   pf_put_byte (p_rpc->packet_type, res_len, p_bytes, p_pos);
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
   pf_put_byte (temp_u32 % 0x100, res_len, p_bytes, p_pos);

   /* flags2 */
   pf_put_byte (0, res_len, p_bytes, p_pos);
   /* Data repr */
   temp_u32 = 0; /* Always ASCII */
   pf_put_bits (p_rpc->is_big_endian ? 0 : 1, 4, 4, &temp_u32);
   pf_put_byte (temp_u32 % 0x100, res_len, p_bytes, p_pos);
   /* Float repr */
   pf_put_byte (p_rpc->float_repr, res_len, p_bytes, p_pos);

   pf_put_byte (p_rpc->reserved, res_len, p_bytes, p_pos);
   pf_put_byte (p_rpc->serial_high, res_len, p_bytes, p_pos);
   pf_put_uuid (
      p_rpc->is_big_endian,
      &p_rpc->object_uuid,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uuid (
      p_rpc->is_big_endian,
      &p_rpc->interface_uuid,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uuid (
      p_rpc->is_big_endian,
      &p_rpc->activity_uuid,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint32 (
      p_rpc->is_big_endian,
      p_rpc->server_boot_time,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint32 (
      p_rpc->is_big_endian,
      p_rpc->interface_version,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint32 (
      p_rpc->is_big_endian,
      p_rpc->sequence_nmb,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (p_rpc->is_big_endian, p_rpc->opnum, res_len, p_bytes, p_pos);
   pf_put_uint16 (
      p_rpc->is_big_endian,
      p_rpc->interface_hint,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      p_rpc->is_big_endian,
      p_rpc->activity_hint,
      res_len,
      p_bytes,
      p_pos);
   *p_pos_body_len = *p_pos;
   pf_put_uint16 (
      p_rpc->is_big_endian,
      p_rpc->length_of_body,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      p_rpc->is_big_endian,
      p_rpc->fragment_nmb,
      res_len,
      p_bytes,
      p_pos);
   pf_put_byte (p_rpc->auth_protocol, res_len, p_bytes, p_pos);
   pf_put_byte (p_rpc->serial_low, res_len, p_bytes, p_pos);
}

void pf_put_record_data_read (
   bool is_big_endian,
   pf_block_type_values_t block_type,
   uint16_t raw_length,
   const uint8_t * p_raw_data,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_mem (p_raw_data, raw_length, res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_read_result (
   bool is_big_endian,
   const pf_iod_read_result_t * p_res,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_res->sequence_number,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uuid (is_big_endian, &p_res->ar_uuid, res_len, p_bytes, p_pos);
   pf_put_uint32 (is_big_endian, p_res->api, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_res->slot_number, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_res->subslot_number, res_len, p_bytes, p_pos);
   pf_put_byte (0, res_len, p_bytes, p_pos);
   pf_put_byte (0, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_res->index, res_len, p_bytes, p_pos);
   *p_data_length_pos = *p_pos;
   pf_put_uint32 (
      is_big_endian,
      p_res->record_data_length,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (is_big_endian, p_res->add_data_1, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_res->add_data_2, res_len, p_bytes, p_pos);
   pf_put_padding (NELEMENTS (p_res->rw_padding), res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

/**
 * @internal
 * Insert the sub-slot information into a buffer.
 *
 * Inserts subslot number and submodule ID.
 *
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param block_type       In:    Specifies REAL or EXP ident number to insert.
 * @param p_subslot        In:    The sub-slot instance.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_ident_subslot (
   bool is_big_endian,
   pf_block_type_values_t block_type,
   const pf_subslot_t * p_subslot,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_put_uint16 (
      is_big_endian,
      p_subslot->subslot_nbr,
      res_len,
      p_bytes,
      p_pos);
   if (block_type == PF_BT_EXPECTED_IDENTIFICATION_DATA)
   {
      pf_put_uint32 (
         is_big_endian,
         p_subslot->exp_submodule_ident_number,
         res_len,
         p_bytes,
         p_pos);
   }
   else
   {
      pf_put_uint32 (
         is_big_endian,
         p_subslot->submodule_ident_number,
         res_len,
         p_bytes,
         p_pos);
   }
}

/**
 * @internal
 * Insert the slot information into a buffer, with filter.
 *
 * Inserts slot number, module ID, number of subslots and then uses
 * \a pf_put_ident_subslot() to insert more info.
 *
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param block_type       In:    Specifies REAL or EXP ident number to insert.
 * @param filter_level     In:    The filter starting level.
 * @param stop_level       In:    The amount of detail to include (ending
 *                                level).
 * @param p_ar             In:    If != NULL then filter by AR.
 * @param p_slot           In:    The slot instance.
 * @param subslot_nbr      In:    Sub-slot number to filter by.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_ident_slot (
   bool is_big_endian,
   pf_block_type_values_t block_type,
   pf_dev_filter_level_t filter_level, /* API_ID, SLOT or SUBSLOT */
   pf_dev_filter_level_t stop_level, /* DEVICE, API_ID, API, SLOT or SUBSLOT */
   const pf_ar_t * p_ar,
   const pf_slot_t * p_slot,
   uint16_t subslot_nbr,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t cnt;
   uint16_t ix;
   const pf_subslot_t * p_subslot;

   pf_put_uint16 (is_big_endian, p_slot->slot_nbr, res_len, p_bytes, p_pos);
   if (block_type == PF_BT_EXPECTED_IDENTIFICATION_DATA)
   {
      pf_put_uint32 (
         is_big_endian,
         p_slot->exp_module_ident_number,
         res_len,
         p_bytes,
         p_pos);
   }
   else
   {
      pf_put_uint32 (
         is_big_endian,
         p_slot->module_ident_number,
         res_len,
         p_bytes,
         p_pos);
   }

   /* Count the number of active sub-slots */
   cnt = 0;
   for (ix = 0; ix < NELEMENTS (p_slot->subslots); ix++)
   {
      p_subslot = &p_slot->subslots[ix];
      if (p_subslot->in_use == true)
      {
         if ((p_ar == NULL) || (p_ar == p_subslot->p_ar))
         {
            if (filter_level >= PF_DEV_FILTER_LEVEL_SUBSLOT)
            {
               if (p_subslot->subslot_nbr == subslot_nbr)
               {
                  cnt++;
               }
            }
            else
            {
               cnt++;
            }
         }
      }
   }

   /* Insert number of subslots in use, even if there are none */
   pf_put_uint16 (is_big_endian, cnt, res_len, p_bytes, p_pos);

   /* Now add the actual subslot info - if requested */
   if (stop_level > PF_DEV_FILTER_LEVEL_SLOT)
   {
      for (ix = 0; ix < NELEMENTS (p_slot->subslots); ix++)
      {
         p_subslot = &p_slot->subslots[ix];
         if (p_subslot->in_use == true)
         {
            if ((p_ar == NULL) || (p_ar == p_subslot->p_ar))
            {
               if (filter_level >= PF_DEV_FILTER_LEVEL_SUBSLOT)
               {
                  if (p_subslot->subslot_nbr == subslot_nbr)
                  {
                     /* Call only for the matching subslot_nbr */
                     pf_put_ident_subslot (
                        is_big_endian,
                        block_type,
                        p_subslot,
                        res_len,
                        p_bytes,
                        p_pos);
                  }
               }
               else
               {
                  /* No filter: Call for all sub-slots that are in use */
                  pf_put_ident_subslot (
                     is_big_endian,
                     block_type,
                     p_subslot,
                     res_len,
                     p_bytes,
                     p_pos);
               }
            }
         }
      }
   }
}

/**
 * @internal
 * Insert the API information into a buffer, with filter.
 *
 * Inserts API ID, number of slots and then uses \a pf_put_ident_slot() to
 * insert more info.
 *
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param block_type       In:    Specifies REAL or EXP ident number to insert.
 * @param filter_level     In:    The filter starting level.
 * @param stop_level       In:    The amount of detail to include (ending
 *                                level).
 * @param p_ar             In:    If != NULL then filter by AR.
 * @param p_api            In:    The api instance.
 * @param slot_nbr         In:    Slot number to filter by.
 * @param subslot_nbr      In:    Sub-slot number to filter by.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_ident_api (
   bool is_big_endian,
   pf_block_type_values_t block_type,
   pf_dev_filter_level_t filter_level, /* API_ID, SLOT or SUBSLOT */
   pf_dev_filter_level_t stop_level, /* DEVICE, API_ID, API, SLOT or SUBSLOT */
   const pf_ar_t * p_ar,
   const pf_api_t * p_api,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t cnt;
   uint16_t ix;
   const pf_slot_t * p_slot;

   pf_put_uint32 (is_big_endian, p_api->api_id, res_len, p_bytes, p_pos);

   if (stop_level > PF_DEV_FILTER_LEVEL_API_ID)
   {
      /* Count the number of active slots */
      cnt = 0;
      for (ix = 0; ix < NELEMENTS (p_api->slots); ix++)
      {
         p_slot = &p_api->slots[ix];
         if (p_slot->in_use == true)
         {
            if ((p_ar == NULL) || (p_ar == p_slot->p_ar))
            {
               if (filter_level >= PF_DEV_FILTER_LEVEL_SLOT)
               {
                  if (p_slot->slot_nbr == slot_nbr)
                  {
                     cnt++;
                  }
               }
               else
               {
                  cnt++;
               }
            }
         }
      }
      /* Insert number of slots in use, even if there are none */
      pf_put_uint16 (is_big_endian, cnt, res_len, p_bytes, p_pos);

      if (stop_level > PF_DEV_FILTER_LEVEL_API)
      {
         /* Include slot (module) information */
         for (ix = 0; ix < NELEMENTS (p_api->slots); ix++)
         {
            p_slot = &p_api->slots[ix];
            if (p_slot->in_use == true)
            {
               if ((p_ar == NULL) || (p_ar == p_slot->p_ar))
               {
                  if (filter_level >= PF_DEV_FILTER_LEVEL_SLOT)
                  {
                     if (p_slot->slot_nbr == slot_nbr)
                     {
                        /* No filter: Call for all slots that are in use */
                        pf_put_ident_slot (
                           is_big_endian,
                           block_type,
                           filter_level,
                           stop_level,
                           p_ar,
                           p_slot,
                           subslot_nbr,
                           res_len,
                           p_bytes,
                           p_pos);
                     }
                  }
                  else
                  {
                     /* No filter: Call for all slots that are in use */
                     pf_put_ident_slot (
                        is_big_endian,
                        block_type,
                        filter_level,
                        stop_level,
                        p_ar,
                        p_slot,
                        subslot_nbr,
                        res_len,
                        p_bytes,
                        p_pos);
                  }
               }
            }
         }
      }
   }
}

/**
 * @internal
 * Put requested ident information into a buffer, with filter.
 *
 * Inserts number of APIs, and then uses \a pf_put_ident_api() to insert
 * more info.
 *
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param block_type       In:    Specifies REAL or EXP ident number to insert.
 * @param filter_level     In:    The filter starting level.
 * @param stop_level       In:    The amount of detail to include (ending
 *                                level).
 * @param p_ar             In:    If != NULL then filter by AR.
 * @param p_device         In:    The device instance.
 * @param api_id           In:    API id to filter by.
 * @param slot_nbr         In:    Slot number to filter by.
 * @param subslot_nbr      In:    Sub-slot number to filter by.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_ident_device (
   bool is_big_endian,
   pf_block_type_values_t block_type,
   pf_dev_filter_level_t filter_level, /* API_ID, SLOT or SUBSLOT */
   pf_dev_filter_level_t stop_level, /* DEVICE, API_ID, API, SLOT or SUBSLOT */
   const pf_ar_t * p_ar,
   const pf_device_t * p_device,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t ix;
   uint16_t cnt;
   const pf_api_t * p_api = NULL;

   /* Count the number of active APIs */
   cnt = 0;
   for (ix = 0; ix < NELEMENTS (p_device->apis); ix++)
   {
      p_api = &p_device->apis[ix];
      if (p_api->in_use == true)
      {
         if ((p_ar == NULL) || (p_ar == p_api->p_ar))
         {
            if (filter_level >= PF_DEV_FILTER_LEVEL_API)
            {
               if (p_api->api_id == api_id)
               {
                  cnt++;
               }
            }
            else
            {
               cnt++;
            }
         }
      }
   }

   /* Insert number of APIs in use, even if there are none */
   pf_put_uint16 (is_big_endian, cnt, res_len, p_bytes, p_pos);

   if (stop_level > PF_DEV_FILTER_LEVEL_DEVICE)
   {
      /* Include at least API ID information */
      for (ix = 0; ix < NELEMENTS (p_device->apis); ix++)
      {
         p_api = &p_device->apis[ix];
         if (p_api->in_use == true)
         {
            if ((p_ar == NULL) || (p_ar == p_api->p_ar))
            {
               if (filter_level >= PF_DEV_FILTER_LEVEL_API)
               {
                  if (p_api->api_id == api_id)
                  {
                     /* Call only for the matching API_id */
                     pf_put_ident_api (
                        is_big_endian,
                        block_type,
                        filter_level,
                        stop_level,
                        p_ar,
                        p_api,
                        slot_nbr,
                        subslot_nbr,
                        res_len,
                        p_bytes,
                        p_pos);
                  }
               }
               else
               {
                  /* No filter: Call for all APIs that are in use */
                  pf_put_ident_api (
                     is_big_endian,
                     block_type,
                     filter_level,
                     stop_level,
                     p_ar,
                     p_api,
                     slot_nbr,
                     subslot_nbr,
                     res_len,
                     p_bytes,
                     p_pos);
               }
            }
         }
      }
   }
}

void pf_put_ident_data (
   pnet_t * net,
   bool is_big_endian,
   uint8_t block_version_low,
   pf_block_type_values_t block_type,
   pf_dev_filter_level_t filter_level, /* DEVICE (no filter), API_ID, SLOT or
                                          SUBSLOT */
   pf_dev_filter_level_t stop_level, /* DEVICE, API_ID, API, SLOT or SUBSLOT */
   const pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t block_pos;
   uint16_t block_len = 0;
   uint16_t data_pos;
   pf_device_t * p_device = NULL;

   block_pos = *p_pos;
   pf_put_block_header (
      is_big_endian,
      block_type,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      block_version_low,
      res_len,
      p_bytes,
      p_pos);

   data_pos = *p_pos;
   if (pf_cmdev_get_device (net, &p_device) == 0)
   {
      if (!p_ar || (p_ar->ar_param.ar_type != PF_ART_IOSAR))
      {
         pf_put_ident_device (
            is_big_endian,
            block_type,
            filter_level,
            stop_level,
            p_ar,
            p_device,
            api_id,
            slot_nbr,
            subslot_nbr,
            res_len,
            p_bytes,
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
      pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
   }
   else
   {
      /* Nothing was inserted!! Report a NULL record */
      *p_pos = block_pos;
   }
}

void pf_put_im_0_filter_data (
   pnet_t * net,
   bool is_big_endian,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;
   pf_device_t * p_device = NULL;

   /* Insert block header for the read operation */
   block_pos = *p_pos;
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_0_FILTER_DATA_SUBMODULE,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      res_len,
      p_bytes,
      p_pos);

   pf_cmdev_get_device (net, &p_device);
   /*
    * ToDo: Walk the device tree:
    * If we encounter a sub-slot that contains I&M data then add it here.
    */
   pf_put_ident_device (
      is_big_endian,
      PF_BT_REAL_IDENTIFICATION_DATA,
      PF_DEV_FILTER_LEVEL_SUBSLOT,
      PF_DEV_FILTER_LEVEL_SUBSLOT,
      NULL,
      p_device,
      0,
      0,
      1,
      res_len,
      p_bytes,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);

   /* I&M0FilterDataDevice - insert only the first submodule of the DAP. */
   block_pos = *p_pos;
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_0_FILTER_DATA_DEVICE,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      res_len,
      p_bytes,
      p_pos);

   /* ToDo: Find the DAP and then insert it. */
   pf_put_ident_device (
      is_big_endian,
      PF_BT_REAL_IDENTIFICATION_DATA,
      PF_DEV_FILTER_LEVEL_SUBSLOT,
      PF_DEV_FILTER_LEVEL_SUBSLOT,
      NULL,
      p_device,
      0,
      0,
      1,
      res_len,
      p_bytes,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_im_0 (
   bool is_big_endian,
   const pnet_im_0_t * p_im_0,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_byte (p_im_0->im_vendor_id_hi, res_len, p_bytes, p_pos);
   pf_put_byte (p_im_0->im_vendor_id_lo, res_len, p_bytes, p_pos);
   pf_put_str (
      p_im_0->im_order_id,
      sizeof (p_im_0->im_order_id),
      res_len,
      p_bytes,
      p_pos);
   pf_put_str (
      p_im_0->im_serial_number,
      sizeof (p_im_0->im_serial_number),
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_im_0->im_hardware_revision,
      res_len,
      p_bytes,
      p_pos);
   pf_put_byte (p_im_0->im_sw_revision_prefix, res_len, p_bytes, p_pos);
   pf_put_byte (
      p_im_0->im_sw_revision_functional_enhancement,
      res_len,
      p_bytes,
      p_pos);
   pf_put_byte (p_im_0->im_sw_revision_bug_fix, res_len, p_bytes, p_pos);
   pf_put_byte (p_im_0->im_sw_revision_internal_change, res_len, p_bytes, p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_im_0->im_revision_counter,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (is_big_endian, p_im_0->im_profile_id, res_len, p_bytes, p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_im_0->im_profile_specific_type,
      res_len,
      p_bytes,
      p_pos);
   pf_put_byte (p_im_0->im_version_major, res_len, p_bytes, p_pos);
   pf_put_byte (p_im_0->im_version_minor, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_im_0->im_supported, res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_im_1 (
   bool is_big_endian,
   const pnet_im_1_t * p_im_1,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Insert block header for the read operation */
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_1,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      res_len,
      p_bytes,
      p_pos);

   pf_put_str (
      p_im_1->im_tag_function,
      sizeof (p_im_1->im_tag_function),
      res_len,
      p_bytes,
      p_pos);
   pf_put_str (
      p_im_1->im_tag_location,
      sizeof (p_im_1->im_tag_location),
      res_len,
      p_bytes,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_im_2 (
   bool is_big_endian,
   const pnet_im_2_t * p_im_2,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Insert block header for the read operation */
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_2,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      res_len,
      p_bytes,
      p_pos);

   pf_put_str (p_im_2, sizeof (pnet_im_2_t), res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_im_3 (
   bool is_big_endian,
   const pnet_im_3_t * p_im_3,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t block_pos = *p_pos;
   uint16_t block_len = 0;

   /* Insert block header for the read operation */
   pf_put_block_header (
      is_big_endian,
      PF_BT_IM_3,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      res_len,
      p_bytes,
      p_pos);

   pf_put_str (p_im_3, sizeof (pnet_im_3_t), res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_record_data_write (
   bool is_big_endian,
   pf_block_type_values_t block_type,
   uint16_t raw_length,
   uint8_t * p_raw_data,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_mem (p_raw_data, raw_length, res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_write_result (
   bool is_big_endian,
   const pf_iod_write_result_t * p_res,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_res->sequence_number,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uuid (is_big_endian, &p_res->ar_uuid, res_len, p_bytes, p_pos);
   pf_put_uint32 (is_big_endian, p_res->api, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_res->slot_number, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_res->subslot_number, res_len, p_bytes, p_pos);
   pf_put_byte (0, res_len, p_bytes, p_pos);
   pf_put_byte (0, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_res->index, res_len, p_bytes, p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_res->record_data_length,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (is_big_endian, p_res->add_data_1, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, p_res->add_data_2, res_len, p_bytes, p_pos);
   pf_put_pnet_status (
      is_big_endian,
      &p_res->pnio_status,
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding (NELEMENTS (p_res->rw_padding), res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);

   if (*p_pos >= res_len)
   {
      LOG_ERROR (
         PNET_LOG,
         "BW(%d): Output buffer is filled, while preparing DCP Write "
         "response.\n",
         __LINE__);
   }
}

void pf_put_log_book_data (
   bool is_big_endian,
   const pf_log_book_t * p_log_book,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (is_big_endian, cnt, res_len, p_bytes, p_pos);

   while (ix != p_log_book->put)
   {
      pf_put_time_timestamp (
         is_big_endian,
         &p_log_book->entries[ix].time_ts,
         res_len,
         p_bytes,
         p_pos);
      pf_put_uuid (
         is_big_endian,
         &p_log_book->entries[ix].ar_uuid,
         res_len,
         p_bytes,
         p_pos);
      pf_put_pnet_status (
         is_big_endian,
         &p_log_book->entries[ix].pnio_status,
         res_len,
         p_bytes,
         p_pos);
      pf_put_uint32 (
         is_big_endian,
         p_log_book->entries[ix].entry_detail,
         res_len,
         p_bytes,
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
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
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
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_diag_item (
   bool is_big_endian,
   const pf_diag_item_t * p_item,
   bool insert_usi,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   if (insert_usi == true)
   {
      pf_put_uint16 (is_big_endian, p_item->usi, res_len, p_bytes, p_pos);
   }

   switch (p_item->usi)
   {
   case PF_USI_CHANNEL_DIAGNOSIS:
      /* Insert std format diagnosis */
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_nbr,
         res_len,
         p_bytes,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_properties,
         res_len,
         p_bytes,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_error_type,
         res_len,
         p_bytes,
         p_pos);
      break;
   case PF_USI_EXTENDED_CHANNEL_DIAGNOSIS:
      /* Insert std format diagnosis */
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_nbr,
         res_len,
         p_bytes,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_properties,
         res_len,
         p_bytes,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_error_type,
         res_len,
         p_bytes,
         p_pos);

      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ext_ch_error_type,
         res_len,
         p_bytes,
         p_pos);
      pf_put_uint32 (
         is_big_endian,
         p_item->fmt.std.ext_ch_add_value,
         res_len,
         p_bytes,
         p_pos);
      break;
   case PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS:
      /* Insert std format diagnosis */
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_nbr,
         res_len,
         p_bytes,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_properties,
         res_len,
         p_bytes,
         p_pos);
      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ch_error_type,
         res_len,
         p_bytes,
         p_pos);

      pf_put_uint16 (
         is_big_endian,
         p_item->fmt.std.ext_ch_error_type,
         res_len,
         p_bytes,
         p_pos);
      pf_put_uint32 (
         is_big_endian,
         p_item->fmt.std.ext_ch_add_value,
         res_len,
         p_bytes,
         p_pos);

      pf_put_uint32 (
         is_big_endian,
         p_item->fmt.std.qual_ch_qualifier,
         res_len,
         p_bytes,
         p_pos);
      break;
   default:
      pf_put_mem (
         p_item->fmt.usi.manuf_data,
         p_item->fmt.usi.len,
         res_len,
         p_bytes,
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
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_diag_list (
   pnet_t * net,
   bool is_big_endian,
   pf_diag_filter_level_t diag_filter,
   uint16_t usi_filter,
   uint16_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t list_head,
   uint16_t res_len,
   uint8_t * p_bytes,
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
         res_len,
         p_bytes,
         p_pos);

      /* Insert API, slot and subslot */
      pf_put_uint32 (is_big_endian, api_id, res_len, p_bytes, p_pos);
      pf_put_uint16 (is_big_endian, slot_nbr, res_len, p_bytes, p_pos);
      pf_put_uint16 (is_big_endian, subslot_nbr, res_len, p_bytes, p_pos);

      /* Insert summary-channel, and its properties
       * The list only contains APPEARS, so it can be hardcoded here.
       * ToDo: More info into ch_properties here!
       */
      pf_put_uint16 (
         is_big_endian,
         PNET_CHANNEL_WHOLE_SUBMODULE,
         res_len,
         p_bytes,
         p_pos);
      PF_DIAG_CH_PROP_SPEC_SET (ch_properties, PF_DIAG_CH_PROP_SPEC_APPEARS);
      pf_put_uint16 (is_big_endian, ch_properties, res_len, p_bytes, p_pos);

      /* Insert USI */
      pf_put_uint16 (is_big_endian, usi_filter, res_len, p_bytes, p_pos);

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
               res_len,
               p_bytes,
               p_pos);
         }

         pf_cmdev_get_diag_item (net, p_item->next, &p_item);
      }

      /* Finally insert the block length into the block header */
      if (*p_pos > data_pos)
      {
         block_len = *p_pos - (block_pos + 4);
         block_pos += offsetof (pf_block_header_t, block_length);
         pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
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
 * This is done by calling pf_put_diag_list() for USI values of found diagnoses.
 *
 * @param net              InOut: The p-net stack instance
 * @param is_big_endian    In:    true if buffer is big-endian.
 * @param diag_filter      In:    Type of diag items to insert.
 * @param api_id           In:    The API number to write to buffer.
 * @param slot_nbr         In:    The slot number to write to buffer.
 * @param p_subslot        In:    The subslot instance.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_diag_subslot (
   pnet_t * net,
   bool is_big_endian,
   pf_diag_filter_level_t diag_filter,
   uint16_t api_id,
   uint16_t slot_nbr,
   const pf_subslot_t * p_subslot,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t usi = 0;
   int err =
      pf_cmdev_get_next_diagnosis_usi (net, p_subslot->diag_list, 0, &usi);

   while (err == 0)
   {
      pf_put_diag_list (
         net,
         is_big_endian,
         diag_filter,
         usi,
         api_id,
         slot_nbr,
         p_subslot->subslot_nbr,
         p_subslot->diag_list,
         res_len,
         p_bytes,
         p_pos);

      err =
         pf_cmdev_get_next_diagnosis_usi (net, p_subslot->diag_list, usi, &usi);
   }
}

/**
 * @internal
 * Insert diagnosis items of a slot into a buffer.
 *
 * This is done by calling pf_put_diag_subslot() for relevant subslots.
 *
 * @param net              InOut: The p-net stack instance
 * @param is_big_endian    In:    true if buffer is big-endian.
 * @param filter_level     In:    The filter ending level.
 * @param diag_filter      In:    The types of diag to insert.
 * @param p_ar             In:    If != NULL then filter by AR.
 * @param api_id           In:    API to insert
 * @param p_slot           In:    The slot instance.
 * @param subslot_nbr      In:    The sub-slot number to filter by.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_diag_slot (
   pnet_t * net,
   bool is_big_endian,
   pf_dev_filter_level_t filter_level,
   pf_diag_filter_level_t diag_filter,
   const pf_ar_t * p_ar,
   uint32_t api_id,
   const pf_slot_t * p_slot,
   uint16_t subslot_nbr,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t ix;
   const pf_subslot_t * p_subslot;

   for (ix = 0; ix < NELEMENTS (p_slot->subslots); ix++)
   {
      p_subslot = &p_slot->subslots[ix];
      if (p_subslot->in_use == true)
      {
         if ((p_ar == NULL) || (p_ar == p_subslot->p_ar))
         {
            if (filter_level >= PF_DEV_FILTER_LEVEL_SUBSLOT)
            {
               if (p_subslot->subslot_nbr == subslot_nbr)
               {
                  /* Call only for the matching subslot_nbr */
                  pf_put_diag_subslot (
                     net,
                     is_big_endian,
                     diag_filter,
                     api_id,
                     p_slot->slot_nbr,
                     p_subslot,
                     res_len,
                     p_bytes,
                     p_pos);
               }
            }
            else
            {
               /* No filter: Call for all sub-slots that are in use */
               pf_put_diag_subslot (
                  net,
                  is_big_endian,
                  diag_filter,
                  api_id,
                  p_slot->slot_nbr,
                  p_subslot,
                  res_len,
                  p_bytes,
                  p_pos);
            }
         }
      }
   }
}

/**
 * @internal
 * Insert diagnosis items of an API into a buffer.
 *
 * This is done by calling pf_put_diag_slot() for relevant slots.
 *
 * @param net              InOut: The p-net stack instance
 * @param is_big_endian    In:    true if buffer is big-endian.
 * @param filter_level     In:    The filter ending level.
 * @param diag_filter      In:    The types of diag to insert.
 * @param p_ar             In:    If != NULL then filter by AR.
 * @param p_api            In:    The API instance.
 * @param slot_nbr         In:    The slot number to filter by.
 * @param subslot_nbr      In:    The sub-slot number to filter by.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_diag_api (
   pnet_t * net,
   bool is_big_endian,
   pf_dev_filter_level_t filter_level,
   pf_diag_filter_level_t diag_filter,
   const pf_ar_t * p_ar,
   const pf_api_t * p_api,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t ix;
   const pf_slot_t * p_slot;

   for (ix = 0; ix < NELEMENTS (p_api->slots); ix++)
   {
      p_slot = &p_api->slots[ix];
      if (p_slot->in_use == true)
      {
         if ((p_ar == NULL) || (p_ar == p_slot->p_ar))
         {
            if (filter_level >= PF_DEV_FILTER_LEVEL_SLOT)
            {
               if (p_slot->slot_nbr == slot_nbr)
               {
                  /* Call only for the matching slot_nbr */
                  pf_put_diag_slot (
                     net,
                     is_big_endian,
                     filter_level,
                     diag_filter,
                     p_ar,
                     p_api->api_id,
                     p_slot,
                     subslot_nbr,
                     res_len,
                     p_bytes,
                     p_pos);
               }
            }
            else
            {
               /* No filter: Call for all slots that are in use */
               pf_put_diag_slot (
                  net,
                  is_big_endian,
                  filter_level,
                  diag_filter,
                  p_ar,
                  p_api->api_id,
                  p_slot,
                  subslot_nbr,
                  res_len,
                  p_bytes,
                  p_pos);
            }
         }
      }
   }
}

/**
 * @internal
 * Insert diagnosis items of a device into a buffer.
 *
 * This is done by calling pf_put_diag_api() for all relevant APIs.
 *
 * @param net              InOut: The p-net stack instance
 * @param is_big_endian    In:    true if buffer is big-endian.
 * @param filter_level     In:    The filter ending level.
 * @param diag_filter      In:    The types of diag to insert.
 * @param p_ar             In:    If != NULL then filter by AR.
 * @param p_device         In:    The device instance.
 * @param api_id           In:    The API id to filter by.
 * @param slot_nbr         In:    The slot number to filter by.
 * @param subslot_nbr      In:    The sub-slot number to filter by.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_diag_device (
   pnet_t * net,
   bool is_big_endian,
   pf_dev_filter_level_t filter_level,
   pf_diag_filter_level_t diag_filter,
   const pf_ar_t * p_ar, /* If != NULL only include those belonging to p_ar */
   const pf_device_t * p_device,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t ix;
   const pf_api_t * p_api;

   for (ix = 0; ix < NELEMENTS (p_device->apis); ix++)
   {
      p_api = &p_device->apis[ix];
      if (p_api->in_use == true)
      {
         if ((p_ar == NULL) || (p_ar == p_api->p_ar))
         {
            if (filter_level >= PF_DEV_FILTER_LEVEL_API)
            {
               if (p_api->api_id == api_id)
               {
                  /* Call only for the matching API_id */
                  pf_put_diag_api (
                     net,
                     is_big_endian,
                     filter_level,
                     diag_filter,
                     p_ar,
                     p_api,
                     slot_nbr,
                     subslot_nbr,
                     res_len,
                     p_bytes,
                     p_pos);
               }
            }
            else
            {
               /* No filter: Call for all APIs that are in use */
               pf_put_diag_api (
                  net,
                  is_big_endian,
                  filter_level,
                  diag_filter,
                  p_ar,
                  p_api,
                  slot_nbr,
                  subslot_nbr,
                  res_len,
                  p_bytes,
                  p_pos);
            }
         }
      }
   }
}

void pf_put_diag_data (
   pnet_t * net,
   bool is_big_endian,
   pf_dev_filter_level_t filter_level,
   pf_diag_filter_level_t diag_filter,
   const pf_ar_t * p_ar, /* If p_ar != NULL only include those belonging to p_ar
                          */
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_device_t * p_device = NULL;

   if (pf_cmdev_get_device (net, &p_device) == 0)
   {
      pf_put_diag_device (
         net,
         is_big_endian,
         filter_level,
         diag_filter,
         p_ar,
         p_device,
         api_id,
         slot_nbr,
         subslot_nbr,
         res_len,
         p_bytes,
         p_pos);
   }
}

void pf_put_alarm_fixed (
   bool is_big_endian,
   const pf_alarm_fixed_t * p_alarm_fixed,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint32_t temp_u32;
   uint8_t temp_u8;

   /* The fixed part is not a "block" so do not insert a block header */

   /* Destination and source */
   pf_put_uint16 (
      is_big_endian,
      p_alarm_fixed->dst_ref,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_alarm_fixed->src_ref,
      res_len,
      p_bytes,
      p_pos);

   /* Flags for PDU type, version, window size and transport ACK */
   temp_u32 = 0;
   pf_put_bits (p_alarm_fixed->pdu_type.type, 4, 0, &temp_u32);
   pf_put_bits (p_alarm_fixed->pdu_type.version, 4, 4, &temp_u32);
   temp_u8 = (uint8_t)temp_u32;
   pf_put_byte (temp_u8, res_len, p_bytes, p_pos);

   temp_u32 = 0;
   pf_put_bits (p_alarm_fixed->add_flags.window_size, 4, 0, &temp_u32);
   pf_put_bits (p_alarm_fixed->add_flags.tack, 1, 4, &temp_u32);
   temp_u8 = (uint8_t)temp_u32;
   pf_put_byte (temp_u8, res_len, p_bytes, p_pos);

   /* Sequence numbers for sending and ACK */
   pf_put_uint16 (
      is_big_endian,
      p_alarm_fixed->send_seq_num,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_alarm_fixed->ack_seq_nbr,
      res_len,
      p_bytes,
      p_pos);
}

/**
 * @internal
 * Insert maint_status into a buffer
 *
 * @param is_big_endian    In:    true if buffer is big-endian.
 * @param maint_status     In:    Maintenance status.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
void pf_put_maint_status (
   bool is_big_endian,
   uint32_t maint_status,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t block_pos = 0;
   uint16_t block_len = 0;

   pf_put_uint16 (is_big_endian, PF_USI_MAINTENANCE, res_len, p_bytes, p_pos);

   block_pos = *p_pos;
   pf_put_block_header (
      is_big_endian,
      PF_BT_MAINTENANCE_ITEM,
      0, /* Dont know block_len yet */
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      res_len,
      p_bytes,
      p_pos);

   pf_put_byte (0, res_len, p_bytes, p_pos);
   pf_put_byte (0, res_len, p_bytes, p_pos);

   pf_put_uint32 (is_big_endian, maint_status, res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   /* Point to correct place */
   block_pos += offsetof (pf_block_header_t, block_length);
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
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
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   /* Insert alarm type */
   pf_put_uint16 (
      is_big_endian,
      p_alarm_data->alarm_type,
      res_len,
      p_bytes,
      p_pos);

   /* Insert API, slot and subslot */
   pf_put_uint32 (is_big_endian, p_alarm_data->api_id, res_len, p_bytes, p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_alarm_data->slot_nbr,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint16 (
      is_big_endian,
      p_alarm_data->subslot_nbr,
      res_len,
      p_bytes,
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
         res_len,
         p_bytes,
         p_pos);
      pf_put_uint32 (
         is_big_endian,
         p_alarm_data->submodule_ident,
         res_len,
         p_bytes,
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
   pf_put_uint16 (is_big_endian, temp_u16, res_len, p_bytes, p_pos);

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
               res_len,
               p_bytes,
               p_pos);
         }
         pf_put_diag_item (
            is_big_endian,
            (pf_diag_item_t *)p_payload,
            true, /* Insert the USI field */
            res_len,
            p_bytes,
            p_pos);
         break;
      default:
         /* Manufacturer data */
         /* Starts with the USI and then followed by raw data bytes */
         pf_put_uint16 (is_big_endian, payload_usi, res_len, p_bytes, p_pos);
         if (payload_len > 0)
         {
            pf_put_mem (p_payload, payload_len, res_len, p_bytes, p_pos);
         }
         break;
      }
   }
   else
   {
      /* AlarmAck DATA message */
      pf_put_pnet_status (true, p_status, PF_FRAME_BUFFER_SIZE, p_bytes, p_pos);
   }

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
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
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (is_big_endian, sub_mode, res_len, p_bytes, p_pos);
   pf_put_mem (p_iocs, iocs_len, res_len, p_bytes, p_pos);
   pf_put_mem (p_data, data_len, res_len, p_bytes, p_pos);
   pf_put_mem (p_iops, iops_len, res_len, p_bytes, p_pos);

   /* Insert the block length into the substitution data block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
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
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      substitute_active_flag,
      res_len,
      p_bytes,
      p_pos);

   pf_put_byte (iocs_len, res_len, p_bytes, p_pos);
   pf_put_byte (iops_len, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, data_len, res_len, p_bytes, p_pos);

   pf_put_mem (p_iocs, iocs_len, res_len, p_bytes, p_pos);
   pf_put_mem (p_data, data_len, res_len, p_bytes, p_pos);
   pf_put_mem (p_iops, iops_len, res_len, p_bytes, p_pos);

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
         res_len,
         p_bytes,
         p_pos);
   }
   /* -------------- */

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_input_data (
   bool is_big_endian,
   uint8_t iocs_len,
   const uint8_t * p_iocs,
   uint8_t iops_len,
   const uint8_t * p_iops,
   uint16_t data_len,
   const uint8_t * p_data,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_byte (iocs_len, res_len, p_bytes, p_pos);
   pf_put_mem (p_iocs, iocs_len, res_len, p_bytes, p_pos);

   pf_put_byte (iops_len, res_len, p_bytes, p_pos);
   pf_put_mem (p_iops, iops_len, res_len, p_bytes, p_pos);

   pf_put_uint16 (is_big_endian, data_len, res_len, p_bytes, p_pos);
   pf_put_mem (p_data, data_len, res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_pdport_data_check (
   bool is_big_endian,
   uint16_t slot,
   uint16_t subslot,
   const pf_check_peer_t * check_peer,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

   /* Slot and subslot info */
   pf_put_uint16 (is_big_endian, slot, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, subslot, res_len, p_bytes, p_pos);

   /* PF_BT_CHECKPEERS Block header first */
   pf_put_block_header (
      is_big_endian,
      PF_BT_CHECKPEERS,
      5 + check_peer->length_peer_port_name +
         check_peer->length_peer_station_name,
      PNET_BLOCK_VERSION_HIGH,
      PNET_BLOCK_VERSION_LOW,
      res_len,
      p_bytes,
      p_pos);

   /* Number of Peers */
   pf_put_byte (1, res_len, p_bytes, p_pos);

   /* Length peer_id */
   pf_put_byte (check_peer->length_peer_port_name, res_len, p_bytes, p_pos);

   /* peer_id */
   pf_put_mem (
      &check_peer->peer_port_name,
      check_peer->length_peer_port_name,
      res_len,
      p_bytes,
      p_pos);

   /* Length ChassisID */
   pf_put_byte (check_peer->length_peer_station_name, res_len, p_bytes, p_pos);

   /* ChassisID */
   pf_put_mem (
      &check_peer->peer_station_name,
      check_peer->length_peer_station_name,
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding_align (block_pos, 4, res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_pdport_data_real (
   bool is_big_endian,
   uint16_t subslot,
   const pf_lldp_station_name_t * p_peer_station_name,
   const pf_lldp_port_name_t * p_peer_port_name,
   const pf_port_t * p_port_data,
   pf_mediatype_values_t media_type,
   const pnal_eth_status_t * p_eth_status,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

   /* Slot and subslot */
   pf_put_uint16 (is_big_endian, PNET_SLOT_DAP_IDENT, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, subslot, res_len, p_bytes, p_pos);

   /* Length OwnPortName */
   pf_put_byte (strlen (p_port_data->port_name), res_len, p_bytes, p_pos);

   /* OwnPortName */
   pf_put_mem (
      p_port_data->port_name,
      strlen (p_port_data->port_name),
      res_len,
      p_bytes,
      p_pos);

   /* Number of Peers */
   pf_put_byte (num_peers, res_len, p_bytes, p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

   /* Peer info */
   if (num_peers > 0)
   {
      /* Peer port name */
      pf_put_byte (p_peer_port_name->len, res_len, p_bytes, p_pos);
      pf_put_mem (
         p_peer_port_name->string,
         p_peer_port_name->len,
         res_len,
         p_bytes,
         p_pos);

      /* Peer station name */
      pf_put_byte (p_peer_station_name->len, res_len, p_bytes, p_pos);
      pf_put_mem (
         p_peer_station_name->string,
         p_peer_station_name->len,
         res_len,
         p_bytes,
         p_pos);

      pf_put_padding_align (block_pos, 4, res_len, p_bytes, p_pos);

      /* Line Delay */
      pf_put_uint32 (
         is_big_endian,
         p_peer_info->line_delay,
         res_len,
         p_bytes,
         p_pos);

      /* Peer MAC Addr */
      pf_put_mem (
         p_peer_info->mac_address.addr,
         sizeof (p_peer_info->mac_address.addr),
         res_len,
         p_bytes,
         p_pos);

      pf_put_padding (2, res_len, p_bytes, p_pos);
   } /* Peer info */

   /* MAUType */
   pf_put_uint16 (
      is_big_endian,
      p_eth_status->operational_mau_type,
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

   /* Domain Boundary */
   /* Todo- hardcoded value*/
   pf_put_uint32 (is_big_endian, 0, res_len, p_bytes, p_pos);

   /* Multicast Boundary */
   /* Todo- hardcoded value*/
   pf_put_uint32 (is_big_endian, 0, res_len, p_bytes, p_pos);

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

   pf_put_uint16 (is_big_endian, link_state, res_len, p_bytes, p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

   pf_put_uint32 (is_big_endian, media_type, res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_pdport_statistics (
   bool is_big_endian,
   const pnal_port_stats_t * p_port_stats,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

   pf_put_uint32 (
      is_big_endian,
      p_port_stats->if_in_octets,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_port_stats->if_out_octets,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_port_stats->if_in_discards,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_port_stats->if_out_discards,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_port_stats->if_in_errors,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      p_port_stats->if_out_errors,
      res_len,
      p_bytes,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_pdinterface_data_real (
   bool is_big_endian,
   const pnet_ethaddr_t * mac_address,
   pnal_ipaddr_t ip_address,
   pnal_ipaddr_t netmask,
   pnal_ipaddr_t gateway,
   const char * station_name,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   /* Station name */
   /* If station name length is 0, maybe mac string shall be used instead */
   pf_put_byte ((uint8_t)strlen (station_name), res_len, p_bytes, p_pos);
   pf_put_mem (station_name, strlen (station_name), res_len, p_bytes, p_pos);

   pf_put_padding_align (block_pos, 4, res_len, p_bytes, p_pos);

   /* DAP interface MAC address */
   pf_put_mem (
      &mac_address->addr,
      sizeof (pnet_ethaddr_t),
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

   /* IP Address */
   pf_put_uint32 (is_big_endian, ip_address, res_len, p_bytes, p_pos);

   /* Subnet Mask */
   pf_put_uint32 (is_big_endian, netmask, res_len, p_bytes, p_pos);

   /* Router  */
   pf_put_uint32 (is_big_endian, gateway, res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
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
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

   /* API */
   pf_put_uint32 (is_big_endian, api, res_len, p_bytes, p_pos);

   /* Slot and subslot */
   pf_put_uint16 (is_big_endian, PNET_SLOT_DAP_IDENT, res_len, p_bytes, p_pos);
   pf_put_uint16 (
      is_big_endian,
      PNET_SUBSLOT_DAP_INTERFACE_1_IDENT,
      res_len,
      p_bytes,
      p_pos);

   /*PDInterfaceDataReal*/
   pf_put_pdinterface_data_real (
      is_big_endian,
      mac_address,
      ip_address,
      netmask,
      gateway,
      station_name,
      res_len,
      p_bytes,
      p_pos);

   /*PDPortStatistics*/
   pf_put_pdport_statistics (
      is_big_endian,
      p_port_statistics,
      res_len,
      p_bytes,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
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
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

   /* API */
   pf_put_uint32 (is_big_endian, api, res_len, p_bytes, p_pos);

   /* Slot and subslot */
   pf_put_uint16 (is_big_endian, PNET_SLOT_DAP_IDENT, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, subslot, res_len, p_bytes, p_pos);

   /*PDPortDataReal*/
   pf_put_pdport_data_real (
      true,
      subslot,
      p_peer_station_name,
      p_peer_port_name,
      p_port_data,
      media_type,
      p_eth_status,
      res_len,
      p_bytes,
      p_pos);

   /*PDPortStatistics*/
   pf_put_pdport_statistics (
      is_big_endian,
      p_port_statistics,
      res_len,
      p_bytes,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

/**
 * @internal
 * Insert peer to peer boundary
 * @param is_big_endian           In:    Endianness of the destination buffer.
 * @param p_peer_to_peer_boundary In:    The p-net stack instance
 * @param res_len                 In:    Size of destination buffer.
 * @param p_bytes                 Out:   Destination buffer.
 * @param p_pos                   InOut: Position in destination buffer.
 */
void pf_put_peer_to_peer_boundary (
   bool is_big_endian,
   const pf_adjust_peer_to_peer_boundary_t * p_peer_to_peer_boundary,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

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

   pf_put_uint32 (is_big_endian, temp_u32, res_len, p_bytes, p_pos);

   /* Adjust Properties */
   pf_put_uint16 (
      is_big_endian,
      p_peer_to_peer_boundary->adjust_properties,
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_pdport_data_adj (
   bool is_big_endian,
   uint16_t subslot,
   const pf_adjust_peer_to_peer_boundary_t * p_peer_to_peer_boundary,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

   /* Slot and subslot */
   pf_put_uint16 (is_big_endian, PNET_SLOT_DAP_IDENT, res_len, p_bytes, p_pos);
   pf_put_uint16 (is_big_endian, subslot, res_len, p_bytes, p_pos);

   /* PeerToPeer_boundary*/
   pf_put_peer_to_peer_boundary (
      is_big_endian,
      p_peer_to_peer_boundary,
      res_len,
      p_bytes,
      p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

void pf_put_pd_interface_adj (
   bool is_big_endian,
   uint16_t subslot,
   pf_lldp_name_of_device_mode_t name_of_device_mode,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding (2, res_len, p_bytes, p_pos);

   /* MultipleInterfaceMode */
   temp_u32 = 0;
   pf_put_bits (name_of_device_mode, 1, 0, &temp_u32);
   pf_put_uint32 (is_big_endian, temp_u32, res_len, p_bytes, p_pos);

   /* Pad with zeroes until block length is a multiple of 4 bytes */
   pf_put_padding_align (block_pos, 4, res_len, p_bytes, p_pos);

   /* Finally insert the block length into the block header */
   block_len = *p_pos - (block_pos + 4);
   block_pos += offsetof (pf_block_header_t, block_length); /* Point to correct
                                                               place */
   pf_put_uint16 (is_big_endian, block_len, res_len, p_bytes, &block_pos);
}

/**
 * Put rpc epm floor count into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param count            In:    Floor count.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_floor_count (
   bool is_big_endian,
   uint32_t count,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_put_uint16 (is_big_endian, count, res_len, p_bytes, p_pos);
}

/**
 * Put rpc epm floor 1 configuration into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_floor          In:    Floor configuration.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_floor_1_uuid (
   bool is_big_endian,
   const pf_floor_uuid_t * p_floor,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t lhs_length = (uint16_t) (
      sizeof (p_floor->protocol_id) + sizeof (p_floor->uuid) +
      sizeof (p_floor->version_major));

   pf_put_uint16 (is_big_endian, lhs_length, res_len, p_bytes, p_pos);

   pf_put_byte (p_floor->protocol_id, res_len, p_bytes, p_pos);

   pf_put_rpc_uuid (is_big_endian, &p_floor->uuid, res_len, p_bytes, p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_floor->version_major,
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->version_minor)),
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_floor->version_minor,
      res_len,
      p_bytes,
      p_pos);
}

/**
 * Put rpc epm floor 2 configuration into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_floor          In:    Floor configuration.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_floor_2_uuid (
   bool is_big_endian,
   const pf_floor_uuid_t * p_floor,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_put_rpc_floor_1_uuid (is_big_endian, p_floor, res_len, p_bytes, p_pos);
}

/**
 * Put rpc epm floor 3 configuration into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_floor          In:    Floor configuration.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_floor_3_rpc (
   bool is_big_endian,
   const pf_floor_3_t * p_floor,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->protocol_id)),
      res_len,
      p_bytes,
      p_pos);

   pf_put_byte (p_floor->protocol_id, res_len, p_bytes, p_pos);

   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->version_minor)),
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (
      is_big_endian,
      p_floor->version_minor,
      res_len,
      p_bytes,
      p_pos);
}

/**
 * Put rpc epm floor 4 configuration into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_floor          In:    Floor configuration.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_floor_4_udp (
   bool is_big_endian,
   const pf_floor_4_t * p_floor,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->protocol_id)),
      res_len,
      p_bytes,
      p_pos);

   pf_put_byte (p_floor->protocol_id, res_len, p_bytes, p_pos);

   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->port)),
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint16 (is_big_endian, p_floor->port, res_len, p_bytes, p_pos);
}

/**
 * Put rpc epm floor 5 configuration into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_floor          In:    Floor configuration.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_floor_5_ip (
   bool is_big_endian,
   const pf_floor_5_t * p_floor,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->protocol_id)),
      res_len,
      p_bytes,
      p_pos);

   pf_put_byte (p_floor->protocol_id, res_len, p_bytes, p_pos);

   pf_put_uint16 (
      is_big_endian,
      (uint16_t)(sizeof (p_floor->ip_address)),
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint32 (is_big_endian, p_floor->ip_address, res_len, p_bytes, p_pos);
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
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_epm_annotation (
   const pnet_cfg_t * p_cfg,
   uint16_t res_len,
   uint8_t * p_bytes,
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
      p_cfg->product_name,
      p_cfg->im_0_data.im_order_id,
      p_cfg->im_0_data.im_hardware_revision,
      p_cfg->im_0_data.im_sw_revision_prefix,
      p_cfg->im_0_data.im_sw_revision_functional_enhancement,
      p_cfg->im_0_data.im_sw_revision_bug_fix,
      p_cfg->im_0_data.im_sw_revision_internal_change);

   pf_put_mem (
      annotation_data,
      sizeof (annotation_data),
      res_len,
      p_bytes,
      p_pos);
}

/**
 * Put rpc epm protocol tower into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_tower          In:    Protocol tower configuration.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_tower_entry (
   bool is_big_endian,
   const pf_rpc_tower_t * p_tower,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_put_uint32 (
      is_big_endian,
      PF_RPC_TOWER_REFERENTID,
      res_len,
      p_bytes,
      p_pos);
   pf_put_uint32 (
      is_big_endian,
      PF_RPC_EPM_ANNOTATION_OFFSET,
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint32 (
      is_big_endian,
      PF_RPC_EPM_ANNOTATION_LENGTH,
      res_len,
      p_bytes,
      p_pos);

   pf_put_rpc_epm_annotation (p_tower->p_cfg, res_len, p_bytes, p_pos);

   pf_put_uint32 (
      is_big_endian,
      PF_RPC_EPM_FLOOR_LENGTH,
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint32 (
      is_big_endian,
      PF_RPC_EPM_FLOOR_LENGTH,
      res_len,
      p_bytes,
      p_pos);

   pf_put_rpc_floor_count (
      is_big_endian,
      PF_RPC_TOWER_FLOOR_COUNT,
      res_len,
      p_bytes,
      p_pos);

   pf_put_rpc_floor_1_uuid (
      is_big_endian,
      &p_tower->floor_1,
      res_len,
      p_bytes,
      p_pos);

   pf_put_rpc_floor_2_uuid (
      is_big_endian,
      &p_tower->floor_2,
      res_len,
      p_bytes,
      p_pos);

   pf_put_rpc_floor_3_rpc (
      is_big_endian,
      &p_tower->floor_3,
      res_len,
      p_bytes,
      p_pos);

   pf_put_rpc_floor_4_udp (
      is_big_endian,
      &p_tower->floor_4,
      res_len,
      p_bytes,
      p_pos);

   pf_put_rpc_floor_5_ip (
      is_big_endian,
      &p_tower->floor_5,
      res_len,
      p_bytes,
      p_pos);
}

/**
 * Put rpc handle into a buffer
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_handle         In:    Rpc handle
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_handle (
   bool is_big_endian,
   const pf_rpc_handle_t * p_handle,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_put_uint32 (
      is_big_endian,
      p_handle->rpc_entry_handle,
      res_len,
      p_bytes,
      p_pos);

   pf_put_rpc_uuid (
      is_big_endian,
      &p_handle->handle_uuid,
      res_len,
      p_bytes,
      p_pos);
}

/**
 * Put rpc epm entry into a buffer. If no no protocol tower is
 * configured / actual_count == 0, a empty entry is written.
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_entry          In:    Rpc epm entry including protocol
 *                                tower configuration.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
static void pf_put_rpc_epm_entry (
   bool is_big_endian,
   const pf_rpc_entry_t * p_entry,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   pf_put_uint32 (is_big_endian, p_entry->max_count, res_len, p_bytes, p_pos);

   pf_put_uint32 (is_big_endian, p_entry->offset, res_len, p_bytes, p_pos);

   pf_put_uint32 (is_big_endian, p_entry->actual_count, res_len, p_bytes, p_pos);

   /* Add entry if not empty */
   if (p_entry->actual_count > 0)
   {
      pf_put_rpc_uuid (
         is_big_endian,
         &p_entry->object_uuid,
         res_len,
         p_bytes,
         p_pos);

      pf_put_tower_entry (
         is_big_endian,
         &p_entry->tower_entry,
         res_len,
         p_bytes,
         p_pos);
   }
}

void pf_put_lookup_response_data (
   pnet_t * net,
   bool is_big_endian,
   const pf_rpc_lookup_rsp_t * p_lookup_rsp,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos)
{
   uint16_t block_position = *p_pos;
   pf_put_rpc_handle (
      is_big_endian,
      &p_lookup_rsp->rpc_handle,
      res_len,
      p_bytes,
      p_pos);

   pf_put_uint32 (
      is_big_endian,
      p_lookup_rsp->num_entry,
      res_len,
      p_bytes,
      p_pos);

   pf_put_rpc_epm_entry (
      is_big_endian,
      &p_lookup_rsp->rpc_entry,
      res_len,
      p_bytes,
      p_pos);

   pf_put_padding_align (block_position, 2, res_len, p_bytes, p_pos);

   pf_put_uint32 (
      is_big_endian,
      p_lookup_rsp->return_code,
      res_len,
      p_bytes,
      p_pos);
}
