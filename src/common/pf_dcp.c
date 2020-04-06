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
#define os_eth_send mock_os_eth_send
#define os_set_led mock_os_set_led
#endif

#include <string.h>
#include "osal.h"
#include "pf_includes.h"
#include "pf_block_writer.h"

/*
 * Various constants from the standard document.
 */
#define PF_DCP_SERVICE_GET                      0x03
#define PF_DCP_SERVICE_SET                      0x04
#define PF_DCP_SERVICE_IDENTIFY                 0x05
#define PF_DCP_SERVICE_HELLO                    0x06

#define PF_DCP_BLOCK_PADDING                    0x00

#define PF_DCP_SERVICE_TYPE_REQUEST             0x00
#define PF_DCP_SERVICE_TYPE_SUCCESS             0x01
#define PF_DCP_SERVICE_TYPE_NOT_SUPPORTED       0x05

#define PF_DCP_HEADER_SIZE                      10
#define PF_DCP_BLOCK_HDR_SIZE                   4

#define PF_DCP_HELLO_FRAME_ID                   0xfefc
#define PF_DCP_GET_SET_FRAME_ID                 0xfefd
#define PF_DCP_ID_REQ_FRAME_ID                  0xfefe
#define PF_DCP_ID_RES_FRAME_ID                  0xfeff

CC_PACKED_BEGIN
typedef struct CC_PACKED pf_ethhdr
{
  pnet_ethaddr_t dest;
  pnet_ethaddr_t src;
  uint16_t  type;
} pf_ethhdr_t;
CC_PACKED_END

CC_PACKED_BEGIN
typedef struct CC_PACKED pf_dcp_header
{
   uint8_t                 service_id;
   uint8_t                 service_type;
   uint32_t                xid;
   uint16_t                response_delay_factor;
   uint16_t                data_length;
} pf_dcp_header_t;
CC_PACKED_END
CC_STATIC_ASSERT(PF_DCP_HEADER_SIZE == sizeof(pf_dcp_header_t));

CC_PACKED_BEGIN
typedef struct CC_PACKED pf_dcp_block_hdr
{
   uint8_t                 option;
   uint8_t                 sub_option;
   uint16_t                block_length;
} pf_dcp_block_hdr_t;
CC_PACKED_END
CC_STATIC_ASSERT(PF_DCP_BLOCK_HDR_SIZE == sizeof(pf_dcp_block_hdr_t));

/*
 * This is the standard DCP HELLO broadcast address (MAC).
 */
static const pnet_ethaddr_t dcp_mc_addr_hello =
{
   {0x01, 0x0e, 0xcf, 0x00, 0x00, 0x01 }
};

#define PF_MAC_NIL         {{0, 0, 0, 0, 0, 0}}

static const pnet_ethaddr_t mac_nil = PF_MAC_NIL;

/*
 * A list of options that we can get/set/filter.
 * Use in identify filter and identify response.
 */
typedef struct pf_dcp_opt_sub
{
   uint8_t                 opt;
   uint8_t                 sub;
} pf_dcp_opt_sub_t;

static const pf_dcp_opt_sub_t device_options[] =
{
   {PF_DCP_OPT_IP, PF_DCP_SUB_IP_PAR},
   {PF_DCP_OPT_IP, PF_DCP_SUB_IP_MAC},
   {PF_DCP_OPT_DEVICE_PROPERTIES, PF_DCP_SUB_DEV_PROP_VENDOR},
   {PF_DCP_OPT_DEVICE_PROPERTIES, PF_DCP_SUB_DEV_PROP_NAME},
   {PF_DCP_OPT_DEVICE_PROPERTIES, PF_DCP_SUB_DEV_PROP_ID},
   {PF_DCP_OPT_DEVICE_PROPERTIES, PF_DCP_SUB_DEV_PROP_ROLE},
   {PF_DCP_OPT_DEVICE_PROPERTIES, PF_DCP_SUB_DEV_PROP_OPTIONS},
   {PF_DCP_OPT_DEVICE_PROPERTIES, PF_DCP_SUB_DEV_PROP_ALIAS},
   {PF_DCP_OPT_CONTROL, PF_DCP_SUB_CONTROL_START},
   {PF_DCP_OPT_CONTROL, PF_DCP_SUB_CONTROL_STOP},
   {PF_DCP_OPT_CONTROL, PF_DCP_SUB_CONTROL_SIGNAL},
   {PF_DCP_OPT_CONTROL, PF_DCP_SUB_CONTROL_FACTORY_RESET},
   {PF_DCP_OPT_CONTROL, PF_DCP_SUB_CONTROL_RESET_TO_FACTORY},
#if 1
#endif
#if 0
   {PF_DCP_OPT_DHCP, PF_DCP_SUB_DHCP_HOSTNAME},
   {PF_DCP_OPT_DHCP, PF_DCP_SUB_DHCP_VENDOR_SPEC},
   {PF_DCP_OPT_DHCP, PF_DCP_SUB_DHCP_SERVER_ID},
   {PF_DCP_OPT_DHCP, PF_DCP_SUB_DHCP_PAR_REQ_LIST},
   {PF_DCP_OPT_DHCP, PF_DCP_SUB_DHCP_CLASS_ID},
   {PF_DCP_OPT_DHCP, PF_DCP_SUB_DHCP_CLIENT_ID},
   {PF_DCP_OPT_DHCP, PF_DCP_SUB_DHCP_FQDN},
   {PF_DCP_OPT_DHCP, PF_DCP_SUB_DHCP_UUID_CLIENT_ID},
#endif
//   {PF_DCP_OPT_DEVICE_INITIATIVE, PF_DCP_SUB_DEV_INITIATIVE_SUPPORT},
   {PF_DCP_OPT_ALL, PF_DCP_SUB_ALL},
};

/**
 * @internal
 * Send a DCP response (to a GET).
 *
 * This is a callback for the scheduler. Arguments should fulfill pf_scheduler_timeout_ftn_t
 *
 * @param net                 InOut: The p-net stack instance
 * @param arg                 In:   DCP responder data.
 * @param current_time        In:   The current time.
 */
static void pf_dcp_responder(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                current_time)
{
   os_buf_t                *p_buf = (os_buf_t *)arg;
   if (p_buf != NULL)
   {
      if (net->dcp_delayed_response_waiting == true)
      {
         if (os_eth_send(net->eth_handle, p_buf) <= 0)
         {
            LOG_ERROR(PNET_LOG, "pf_dcp(%d): Error from os_eth_send(dcp)\n", __LINE__);
         }
         os_buf_free(p_buf);
         net->dcp_delayed_response_waiting = false;
      }
   }
}

/**
 * @internal
 * Clear the sam.
 *
 * This is a callback for the scheduler. Arguments should fulfill pf_scheduler_timeout_ftn_t
 *
 * @param net                 InOut: The p-net stack instance
 * @param arg                 In:   Not used.
 * @param current_time        In:   Not used.
 */
static void pf_dcp_clear_sam(
   pnet_t                     *net,
   void                       *arg,
   uint32_t                   current_time)
{
   net->dcp_sam = mac_nil;
}

/**
 * @internal
 * Add a block to a buffer, possibly including a block_info.
 *
 * @param p_dst            In:   The destination buffer.
 * @param p_dst_pos        InOut:Position in the destination buffer.
 * @param dst_max          In:   Size of destination buffer.
 * @param opt              In:   Option key.
 * @param sub              In:   Sub-option key.
 * @param with_block_info  In:   If true then add block_info argument.
 * @param block_info       In:   The block info argument.
 * @param value_length     In:   The length in bytes of the p_value data.
 * @param p_value          In:   The source date.
 * @return
 */
static int pf_dcp_put_block(
   uint8_t                 *p_dst,
   uint16_t                *p_dst_pos,
   uint16_t                dst_max,
   uint8_t                 opt,
   uint8_t                 sub,
   bool                    with_block_info,
   uint16_t                block_info,
   uint16_t                value_length,     /* Not including length of block_info */
   const void              *p_value)
{
   int                     ret = -1;
   uint16_t                b_len;

   if ((*p_dst_pos + value_length) < dst_max)
   {
      /* Adjust written block length if block_info is included */
      b_len = value_length;
      if (with_block_info)
      {
         b_len += sizeof(b_len);
      }

      pf_put_byte(opt, dst_max, p_dst, p_dst_pos);
      pf_put_byte(sub, dst_max, p_dst, p_dst_pos);
      pf_put_uint16(true, b_len, dst_max, p_dst, p_dst_pos);
      if (with_block_info == true)
      {
         pf_put_uint16(true, block_info, dst_max, p_dst, p_dst_pos);
      }
      if ((p_value != NULL) && (value_length > 0))
      {
         pf_put_mem(p_value, value_length, dst_max, p_dst, p_dst_pos);
      }

      /* Add padding to align on uint16_t */
      while ((*p_dst_pos) & 1)
      {
         pf_put_byte(0, dst_max, p_dst, p_dst_pos);
      }
   }

   ret = 0;       /* Skip excess data silently !! */

   return ret;
}

/**
 * @internal
 * Handle a DCP get request.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_dst            In:   The destination buffer.
 * @param p_dst_pos        InOut:Position in the destination buffer.
 * @param dst_max          In:   Size of destination buffer.
 * @param opt              In:   Option key.
 * @param sub              In:   Sub-option key.
 * @return
 */
static int pf_dcp_get_req(
   pnet_t                  *net,
   uint8_t                 *p_dst,
   uint16_t                *p_dst_pos,
   uint16_t                dst_max,
   uint8_t                 opt,
   uint8_t                 sub)
{
   int                     ret = 0;       /* Assume all OK */
   uint8_t                 block_error = PF_DCP_BLOCK_ERROR_NO_ERROR;
   uint16_t                block_info = 0;
   uint16_t                value_length = 0;
   uint8_t                 *p_value = NULL;
   bool                    skip = false;
   pf_full_ip_suite_t      full_ip_suite;
   uint16_t                temp16;
   uint16_t                ix;

   /* Get the data */
   ret = pf_cmina_dcp_get_req(net, opt, sub, &value_length, &p_value, &block_error);

   /* Some very specific options require special treatment */
   switch (opt)
   {
   case PF_DCP_OPT_IP:
      switch (sub)
      {
      case PF_DCP_SUB_IP_PAR:
         memcpy(&full_ip_suite.ip_suite, p_value, sizeof(full_ip_suite.ip_suite));
         full_ip_suite.ip_suite.ip_addr = htonl(full_ip_suite.ip_suite.ip_addr);
         full_ip_suite.ip_suite.ip_mask = htonl(full_ip_suite.ip_suite.ip_mask);
         full_ip_suite.ip_suite.ip_gateway = htonl(full_ip_suite.ip_suite.ip_gateway);
         p_value = (uint8_t *)&full_ip_suite.ip_suite;

         block_info = ((full_ip_suite.ip_suite.ip_addr == 0) && (full_ip_suite.ip_suite.ip_mask == 0) && (full_ip_suite.ip_suite.ip_gateway == 0)) ? 0: BIT(0);
         /* ToDo: We do not yet report on "IP address conflict" (block_info, BIT(7)) */
         break;
      case PF_DCP_SUB_IP_SUITE:
         memcpy(&full_ip_suite, p_value, sizeof(full_ip_suite));
         full_ip_suite.ip_suite.ip_addr = htonl(full_ip_suite.ip_suite.ip_addr);
         full_ip_suite.ip_suite.ip_mask = htonl(full_ip_suite.ip_suite.ip_mask);
         full_ip_suite.ip_suite.ip_gateway = htonl(full_ip_suite.ip_suite.ip_gateway);
         for (ix = 0; ix < NELEMENTS(full_ip_suite.dns_addr); ix++)
         {
            full_ip_suite.dns_addr[ix] = htonl(full_ip_suite.dns_addr[ix]);
         }
         p_value = (uint8_t *)&full_ip_suite;

         block_info = ((full_ip_suite.ip_suite.ip_addr == 0) && (full_ip_suite.ip_suite.ip_mask == 0) && (full_ip_suite.ip_suite.ip_gateway == 0)) ? 0: BIT(0);
         /* ToDo: We do not yet report on "IP address conflict" (block_info, BIT(7)) */
         break;
      case PF_DCP_SUB_IP_MAC:
         skip = true;
         break;
      default:
         break;
      }
      break;
   case PF_DCP_OPT_DEVICE_PROPERTIES:
      switch (sub)
      {
      case PF_DCP_SUB_DEV_PROP_OPTIONS:
         if (sizeof(device_options) > 0)
         {
            p_value = (uint8_t *)&device_options;
            value_length = sizeof(device_options);
            ret = 0;
         }
         else
         {
            skip = true;
            ret = 0;    /* This is OK - just omit the entire block. */
         }
         break;
      case PF_DCP_SUB_DEV_PROP_GATEWAY:
         if (value_length == sizeof(temp16))
         {
            memcpy(&temp16, p_value, sizeof(temp16));
            temp16 = htons(temp16);
            p_value = (uint8_t *)&temp16;
         }
         else
         {
            ret = -1;
         }
         break;
      case PF_DCP_SUB_DEV_PROP_NAME:
         value_length = (uint16_t)strlen((char *)p_value);
         break;
      case PF_DCP_SUB_DEV_PROP_ALIAS:
         skip = true;
         break;
      case PF_DCP_SUB_DEV_PROP_VENDOR:
         value_length = (uint16_t)strlen((char *)p_value);
         break;
      case PF_DCP_SUB_DEV_PROP_ROLE:
         value_length += 1;
      case PF_DCP_SUB_DEV_PROP_ID:
      case PF_DCP_SUB_DEV_PROP_INSTANCE:
      case PF_DCP_SUB_DEV_PROP_OEM_ID:
      default:
         break;
      }
      break;
   case PF_DCP_OPT_DEVICE_INITIATIVE:
      if (value_length == sizeof(temp16))
      {
         memcpy(&temp16, p_value, sizeof(temp16));
         temp16 = htons(temp16);
         p_value = (uint8_t *)&temp16;
      }
      else
      {
         ret = -1;
      }
      break;
   case PF_DCP_OPT_CONTROL:
      skip = true;
      break;
   case PF_DCP_OPT_ALL:
      skip = true;
      break;
   case PF_DCP_OPT_DHCP:
   default:
      break;
   }

   if (ret == 0)
   {
      if (skip == false)
      {
         ret = pf_dcp_put_block(p_dst, p_dst_pos, dst_max, opt, sub, true, block_info, value_length, p_value);
      }
   }
   else
   {
      if (skip == false)
      {
         ret = pf_dcp_put_block(p_dst, p_dst_pos, dst_max, opt, sub, true, 0, sizeof(block_error), &block_error);
      }
   }

   return ret;
}

/**
 * @internal
 * Execute DCP control states.

 * This functions blinks the system LED 3 times at 1Hz.
 * The system LED is accessed via the os_set_led() osal function.
 *
 * This is a callback for the scheduler. Arguments should fulfill pf_scheduler_timeout_ftn_t
 *
 * @param net                 InOut: The p-net stack instance
 * @param arg                 In:   The current state.
 * @param current_time        In:   The current time.
 */
static void pf_dcp_control_signal(
   pnet_t                     *net,
   void                       *arg,
   uint32_t                   current_time)
{
   uint32_t                   state = (uint32_t)(uintptr_t)arg;

   if ((state % 2) == 1)
   {
      /* Turn LED on */
      os_set_led(0, 1);
   }
   else
   {
      /* Turn LED off */
      os_set_led(0, 0);
   }

   if ((state > 0) && (state < 200))   /* Plausibility test */
   {
      state--;
      /* Schedule another round in 500ms */
      pf_scheduler_add(net, 500*1000, "dcp",
         pf_dcp_control_signal, (void *)(uintptr_t)state, &net->dcp_timeout);

   }
}

/**
 * @internal
 * Handle a DCP set request.
 * @param net              InOut: The p-net stack instance
 * @param p_dst            In:   The destination buffer.
 * @param p_dst_pos        InOut:Position in the destination buffer.
 * @param dst_max          In:   Size of destination buffer.
 * @param opt              In:   Option key.
 * @param sub              In:   Sub-option key.
 * @param block_info       In:   The block info argument.
 * @param value_length     In:   The length in bytes of the p_value data.
 * @param p_value          In:   The source date.
 * @return
 */
static int pf_dcp_set_req(
   pnet_t                  *net,
   uint8_t                 *p_dst,
   uint16_t                *p_dst_pos,
   uint16_t                dst_max,
   uint8_t                 opt,
   uint8_t                 sub,
   uint16_t                block_qualifier,
   uint16_t                value_length,
   uint8_t                 *p_value)
{
   int                     ret = -1;
   uint8_t                 response_data[3]; /* For negative set */
   pf_full_ip_suite_t      full_ip_suite;
   uint16_t                ix;

   response_data[0] = opt;
   response_data[1] = sub;
   response_data[2] = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;

   switch (opt)
   {
   case PF_DCP_OPT_IP:
      switch (sub)
      {
      case PF_DCP_SUB_IP_MAC:
         /* Cant set MAC address */
         break;
      case PF_DCP_SUB_IP_PAR:
         if (value_length == sizeof(full_ip_suite.ip_suite))
         {
            memcpy(&full_ip_suite.ip_suite, p_value, sizeof(full_ip_suite.ip_suite));
            full_ip_suite.ip_suite.ip_addr = ntohl(full_ip_suite.ip_suite.ip_addr);
            full_ip_suite.ip_suite.ip_mask = ntohl(full_ip_suite.ip_suite.ip_mask);
            full_ip_suite.ip_suite.ip_gateway = ntohl(full_ip_suite.ip_suite.ip_gateway);

            ret = pf_cmina_dcp_set_ind(net, opt, sub, block_qualifier, value_length, (uint8_t *)&full_ip_suite.ip_suite, &response_data[2]);
         }
         break;
      case PF_DCP_SUB_IP_SUITE:
         if (value_length == sizeof(full_ip_suite.ip_suite))
         {
            memcpy(&full_ip_suite, p_value, sizeof(full_ip_suite));
            full_ip_suite.ip_suite.ip_addr = ntohl(full_ip_suite.ip_suite.ip_addr);
            full_ip_suite.ip_suite.ip_mask = ntohl(full_ip_suite.ip_suite.ip_mask);
            full_ip_suite.ip_suite.ip_gateway = ntohl(full_ip_suite.ip_suite.ip_gateway);
            for (ix = 0; ix < NELEMENTS(full_ip_suite.dns_addr); ix++)
            {
               full_ip_suite.dns_addr[ix] = ntohl(full_ip_suite.dns_addr[ix]);
            }

            ret = pf_cmina_dcp_set_ind(net, opt, sub, block_qualifier, value_length, (uint8_t *)&full_ip_suite.ip_suite, &response_data[2]);
         }
         break;
      default:
         break;
      }
      break;
   case PF_DCP_OPT_DEVICE_PROPERTIES:
      switch (sub)
      {
      case PF_DCP_SUB_DEV_PROP_NAME:
         ret = pf_cmina_dcp_set_ind(net, opt, sub, block_qualifier, value_length, p_value, &response_data[2]);
         break;
      case PF_DCP_SUB_DEV_PROP_VENDOR:
      case PF_DCP_SUB_DEV_PROP_ID:
      case PF_DCP_SUB_DEV_PROP_ROLE:
      case PF_DCP_SUB_DEV_PROP_OPTIONS:
      case PF_DCP_SUB_DEV_PROP_ALIAS:
      case PF_DCP_SUB_DEV_PROP_INSTANCE:
      case PF_DCP_SUB_DEV_PROP_OEM_ID:
      case PF_DCP_SUB_DEV_PROP_GATEWAY:
         /* Cant set these */
         break;
      default:
         break;
      }
      break;
   case PF_DCP_OPT_DHCP:
      switch (sub)
      {
      case PF_DCP_SUB_DHCP_HOSTNAME:
      case PF_DCP_SUB_DHCP_VENDOR_SPEC:
      case PF_DCP_SUB_DHCP_SERVER_ID:
      case PF_DCP_SUB_DHCP_PAR_REQ_LIST:
      case PF_DCP_SUB_DHCP_CLASS_ID:
      case PF_DCP_SUB_DHCP_CLIENT_ID:
      case PF_DCP_SUB_DHCP_FQDN:
      case PF_DCP_SUB_DHCP_UUID_CLIENT_ID:
         /* ToDo: DHCP Not yet implemented */
         break;
      case PF_DCP_SUB_DHCP_CONTROL:
         /* Can't touch this */
         break;
      default:
         break;
      }
      break;
   case PF_DCP_OPT_CONTROL:
      /* Just ignore if not supported */
      switch (sub)
      {
      case PF_DCP_SUB_CONTROL_START:
         net->dcp_global_block_qualifier = block_qualifier;
         break;
      case PF_DCP_SUB_CONTROL_STOP:
         if (block_qualifier == net->dcp_global_block_qualifier)
         {
            response_data[2] = PF_DCP_BLOCK_ERROR_NO_ERROR;
            ret = 0;
         }
         else
         {
            ret = 0;
         }
         break;
      case PF_DCP_SUB_CONTROL_SIGNAL:
         /* Schedule a state-machine that flashes "the LED" for 3s, 1Hz. */
         LOG_INFO(PF_DCP_LOG, "DCP(%d): Received request to flash LED\n", __LINE__);
         pf_scheduler_add(net, 500*1000, "dcp",
            pf_dcp_control_signal, (void *)(2*3 - 1), &net->dcp_timeout);    /* 3 flashes */
         ret = 0;
         break;
      case PF_DCP_SUB_CONTROL_RESPONSE:
         /* Can't set this */
         break;
      case PF_DCP_SUB_CONTROL_FACTORY_RESET:
      case PF_DCP_SUB_CONTROL_RESET_TO_FACTORY:
         if (pf_cmina_dcp_set_ind(net, opt, sub, block_qualifier, value_length, p_value, &response_data[2]) != 0)
         {
            LOG_ERROR(PF_DCP_LOG, "DCP(%d): Could not perform Factory Reset\n", __LINE__);
         }
         else
         {
            ret = 0;
         }
         break;
      default:
         break;
      }
      break;
   case PF_DCP_OPT_DEVICE_INITIATIVE:
   default:
      /* Can't set this */
      response_data[2] = PF_DCP_BLOCK_ERROR_OPTION_NOT_SUPPORTED;
      break;
   }

   if (ret == 0)
   {
      response_data[2] = PF_DCP_BLOCK_ERROR_NO_ERROR;
   }

   (void)pf_dcp_put_block(p_dst, p_dst_pos, dst_max,
      PF_DCP_OPT_CONTROL, PF_DCP_SUB_CONTROL_RESPONSE,
      false, 0, sizeof(response_data), response_data);

   return ret;
}

/**
 * @internal
 * Handle a DCP Set or Get request.
 *
 * This is a callback for the frame handler. Arguments should fulfill pf_eth_frame_handler_t
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:   The frame id.
 * @param p_buf            In:   The ethernet frame.
 * @param frame_id_pos     In:   Position of the frame id in the buffer.
 * @param p_arg            In:   Not used.
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
static int pf_dcp_get_set(
   pnet_t                  *net,
   uint16_t                frame_id,
   os_buf_t                *p_buf,
   uint16_t                frame_id_pos,
   void                    *p_arg)
{
   int                     ret = 0;       /* Means not handled */
   uint8_t                 *p_src;
   uint16_t                src_pos;
   uint16_t                src_dcplen;
   uint16_t                src_block_len;
   uint16_t                src_block_qualifier;
   pf_ethhdr_t             *p_src_ethhdr;
   pf_dcp_header_t         *p_src_dcphdr;
   pf_dcp_block_hdr_t      *p_src_block_hdr;
   os_buf_t                *p_rsp = NULL;
   uint8_t                 *p_dst;
   uint16_t                dst_pos;
   uint16_t                dst_start;
   pf_ethhdr_t             *p_dst_ethhdr;
   pf_dcp_header_t         *p_dst_dcphdr;
   const pnet_cfg_t        *p_cfg = NULL;

   pf_fspm_get_default_cfg(net, &p_cfg);

   if (p_buf != NULL)
   {
      /* Extract info from the request */
      p_src = (uint8_t*)p_buf->payload;
      src_pos = 0;
      p_src_ethhdr = (pf_ethhdr_t *)&p_src[src_pos];
      src_pos = frame_id_pos;
      src_pos += sizeof(uint16_t);        /* FrameId */
      p_src_dcphdr = (pf_dcp_header_t *)&p_src[src_pos];
      src_pos += sizeof(pf_dcp_header_t);
      src_dcplen = (src_pos + ntohs(p_src_dcphdr->data_length));

      p_rsp = os_buf_alloc(1500);        /* Get a transmit buffer for the response */
      if ((p_rsp != NULL) &&
          ((memcmp(&net->dcp_sam, &mac_nil, sizeof(net->dcp_sam)) == 0) ||
           (memcmp(&net->dcp_sam, &p_src_ethhdr->src, sizeof(net->dcp_sam)) == 0)) &&
          ((p_src_ethhdr->dest.addr[0] & 1) == 0))            /* Not multi-cast */
      {
         /* Prepare the response */
         p_dst = (uint8_t *)p_rsp->payload;
         dst_pos = 0;
         p_dst_ethhdr = (pf_ethhdr_t *)&p_dst[dst_pos];
         dst_pos += sizeof(pf_ethhdr_t);

         /* frame ID */
         p_dst[dst_pos++] = ((PF_DCP_GET_SET_FRAME_ID & 0xff00) >> 8);
         p_dst[dst_pos++] = PF_DCP_GET_SET_FRAME_ID & 0xff;

         p_dst_dcphdr = (pf_dcp_header_t *)&p_dst[dst_pos];
         dst_pos += sizeof(pf_dcp_header_t);

         /* Save position for later */
         dst_start = dst_pos;

         /* Set eth header in the response */
         memcpy(p_dst_ethhdr->dest.addr, p_src_ethhdr->src.addr, sizeof(pnet_ethaddr_t));
         memcpy (p_dst_ethhdr->src.addr, &p_cfg->eth_addr.addr, sizeof(pnet_ethaddr_t));
         p_dst_ethhdr->type = htons(OS_ETHTYPE_PROFINET);

         /* Start with the DCP request header and modify what is needed. */
         *p_dst_dcphdr = *p_src_dcphdr;
         p_dst_dcphdr->service_type = PF_DCP_SERVICE_TYPE_SUCCESS;
         p_dst_dcphdr->response_delay_factor = htons(0);

         if (p_src_dcphdr->service_id == PF_DCP_SERVICE_SET)
         {
            LOG_DEBUG(PF_DCP_LOG,"DCP(%d): Incoming DCP Set request\n", __LINE__);
            p_src_block_hdr = (pf_dcp_block_hdr_t *)&p_src[src_pos];
            src_pos += sizeof(*p_src_block_hdr);    /* Point to the block value */
            src_block_len = ntohs(p_src_block_hdr->block_length);

            while ((ret == 0) &&
                   (src_dcplen >= (src_pos + src_block_len)))
            {
               /* Extract block qualifier */
               src_block_qualifier = ntohs(*(uint16_t*)&p_src[src_pos]);
               src_pos += sizeof(uint16_t);
               src_block_len -= sizeof(uint16_t);

               ret = pf_dcp_set_req(net, p_dst, &dst_pos, 1500,
                  p_src_block_hdr->option, p_src_block_hdr->sub_option,
                  src_block_qualifier,
                  src_block_len, &p_src[src_pos]);

               /* Point to next block */
               src_pos += src_block_len;

               /* Skip padding to align on uint16_t */
               while (src_pos & 1)
               {
                  src_pos++;
               }
               p_src_block_hdr = (pf_dcp_block_hdr_t *)&p_src[src_pos];
               src_pos += sizeof(*p_src_block_hdr);    /* Point to the block value */
               src_block_len = ntohs(p_src_block_hdr->block_length);
            }

            (void)pf_scheduler_add(net, 3*1000*1000,      /* 3s */
               "dcp", pf_dcp_clear_sam, NULL, &net->dcp_sam_timeout);
         }
         else if (p_src_dcphdr->service_id == PF_DCP_SERVICE_GET)
         {
            LOG_DEBUG(PF_DCP_LOG,"DCP(%d): Incoming DCP Get request\n", __LINE__);
            while ((ret == 0) &&
                   (src_dcplen >= (src_pos + sizeof(uint8_t) + sizeof(uint8_t))))
            {
               ret = pf_dcp_get_req(net, p_dst, &dst_pos, 1500,
                  p_src[src_pos], p_src[src_pos + 1]);

               /* Point to next block */
               src_pos += sizeof(uint8_t) + sizeof(uint8_t);
            }

            (void)pf_scheduler_add(net, 3*1000*1000,      /* 3s */
               "dcp", pf_dcp_clear_sam, NULL, &net->dcp_sam_timeout);
         }
         else
         {
            LOG_ERROR(PF_DCP_LOG, "DCP(%d): Unknown DCP service id %u\n", __LINE__, (unsigned)p_src_dcphdr->service_id);
            p_dst_dcphdr->service_type = PF_DCP_SERVICE_TYPE_NOT_SUPPORTED;
         }

         /* Insert final response length and ship it! */
         p_dst_dcphdr->data_length = htons(dst_pos - dst_start);
         p_rsp->len = dst_pos;
         if (os_eth_send(net->eth_handle, p_rsp) <= 0)
         {
            LOG_ERROR(PNET_LOG, "pf_dcp(%d): Error from os_eth_send(dcp)\n", __LINE__);
         }
         else
         {
            LOG_DEBUG(PF_DCP_LOG,"DCP(%d): Sent DCP Get/Set response\n", __LINE__);
         }

         /* Send LLDP _after_ the response in order to pass I/O-tester tests. */
         if (p_src_dcphdr->service_id == PF_DCP_SERVICE_SET)
         {
            pf_cmina_dcp_set_commit(net);
            pf_lldp_send(net);
         }
      }
   }

   if (p_buf != NULL)
   {
      os_buf_free(p_buf);
   }
   if (p_rsp != NULL)
   {
      os_buf_free(p_rsp);
   }

   return 1;    /* Buffer handled */
}

/**
 * @internal
 * Hello indication is used by another device to find its MC peer.
 *
 * This is a callback for the frame handler. Arguments should fulfill pf_eth_frame_handler_t
 *
 * ToDo: Device-device communication (MC) is not yet implemented.
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:   The frame id.
 * @param p_buf            In:   The ethernet frame.
 * @param frame_id_pos     In:   Position of the frame id in the buffer.
 * @param p_arg            In:   Not used.
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
static int pf_dcp_hello_ind(
   pnet_t                  *net,
   uint16_t                frame_id,
   os_buf_t                *p_buf,
   uint16_t                frame_id_pos,
   void                    *p_arg)           /* Not used */
{
   pf_ethhdr_t             *p_eth_hdr = p_buf->payload;

   if (memcmp(p_eth_hdr->dest.addr, dcp_mc_addr_hello.addr, sizeof(dcp_mc_addr_hello.addr)) != 0)
   {
      /* Wrong dest eth address - discard. */
   }
   else
   {
      /*
       * input = DCP-MC-Header, NameOfStationBlockRes, IPParameterBlockRes,
       *    DeviceIDBlockRes, [DeviceOptionsBlockRes ^ DeviceVendorBlockRes],
       *    DeviceRoleBlockRes, DeviceInitiativeBlockRes
       */

      /*
       * NameOfStationBlockRes = NameOfStationType, DCPBlockLength, BlockInfo, NameOfStationValue
       * NameOfStationType = DevicePropertiesOption (=0x02), SuboptionNameOfStation (=0x02)
       */

      /*
       * IPParameterBlockRes = IPParameterType, DCPBlockLength, BlockInfo, IPParameterValue
       * IPParameterType = IPOption (=0x01)
       */

      /*
       * DeviceIDBlockRes = DeviceIDType, DCPBlockLength, BlockInfo, DeviceIDValue
       */

      /*
       * DeviceOptionsBlockRes = DeviceOptionsType, DCPBlockLength, BlockInfo, DeviceOptionsValue
       */

      /*
       * DeviceVendorBlockRes = DeviceVendorType, DCPBlockLength, BlockInfo, DeviceVendorValue
       */

      /*
       * DeviceRoleBlockRes = DeviceRoleType, DCPBlockLength, BlockInfo, DeviceRoleValue
       */

      /*
       * DeviceInitiativeBlockRes = DeviceInitiativeType, DCPBlockLength, BlockInfo, DeviceInitiativeValue
       */
   }
   os_buf_free(p_buf);
   return 1;      /* Means: handled */
}

int pf_dcp_hello_req(
   pnet_t                  *net)
{
   os_buf_t                *p_buf = os_buf_alloc(1500);
   uint8_t                 *p_dst;
   uint16_t                dst_pos;
   uint16_t                dst_start_pos;
   pf_ethhdr_t             *p_ethhdr;
   pf_dcp_header_t         *p_dcphdr;
   uint16_t                value_length;
   uint8_t                 *p_value;
   uint8_t                 block_error;
   uint16_t                temp16;
   pf_ip_suite_t           ip_suite;
   const pnet_cfg_t        *p_cfg = NULL;

   LOG_DEBUG(PF_DCP_LOG,"DCP(%d): Sending DCP Hello request\n", __LINE__);

   pf_fspm_get_default_cfg(net, &p_cfg);

   if (p_buf != NULL)
   {
      p_dst = (uint8_t *)p_buf->payload;
      if (p_dst != NULL)
      {
         dst_pos = 0;
         p_ethhdr = (pf_ethhdr_t *)&p_dst[dst_pos];
         memcpy(p_ethhdr->dest.addr, dcp_mc_addr_hello.addr, sizeof(p_ethhdr->dest.addr));
         memcpy(p_ethhdr->src.addr, p_cfg->eth_addr.addr, sizeof(pnet_ethaddr_t));

         p_ethhdr->type = htons(OS_ETHTYPE_PROFINET);
         dst_pos += sizeof(pf_ethhdr_t);

         /* Insert FrameId */
         p_dst[dst_pos++] = (PF_DCP_HELLO_FRAME_ID & 0xff00) >> 8;
         p_dst[dst_pos++] = PF_DCP_HELLO_FRAME_ID & 0x00ff;

         p_dcphdr = (pf_dcp_header_t *)&p_dst[dst_pos];
         p_dcphdr->service_id = PF_DCP_SERVICE_HELLO;
         p_dcphdr->service_type = PF_DCP_SERVICE_TYPE_REQUEST;
         p_dcphdr->xid = htonl(1);
         p_dcphdr->response_delay_factor = htons(0);
         p_dcphdr->data_length = htons(0);    /* At the moment */
         dst_pos += sizeof(pf_dcp_header_t);

         dst_start_pos = dst_pos;

         if (pf_cmina_dcp_get_req(net, PF_DCP_OPT_DEVICE_PROPERTIES, PF_DCP_SUB_DEV_PROP_NAME,
            &value_length, &p_value, &block_error) == 0)
         {
            (void)pf_dcp_put_block(p_dst, &dst_pos, 1500,
               PF_DCP_OPT_DEVICE_PROPERTIES, PF_DCP_SUB_DEV_PROP_NAME, true, 0,
				(uint16_t)strlen((char *)p_value), p_value);
         }

         if (pf_cmina_dcp_get_req(net, PF_DCP_OPT_IP, PF_DCP_SUB_IP_PAR,
            &value_length, &p_value, &block_error) == 0)
         {
            memcpy(&ip_suite, p_value, sizeof(ip_suite));
            ip_suite.ip_addr = htonl(ip_suite.ip_addr);
            ip_suite.ip_mask = htonl(ip_suite.ip_mask);
            ip_suite.ip_gateway = htonl(ip_suite.ip_gateway);
            (void)pf_dcp_put_block(p_dst, &dst_pos, 1500,
               PF_DCP_OPT_IP, PF_DCP_SUB_IP_PAR, true, 0,
               value_length, &ip_suite);
         }

         if (pf_cmina_dcp_get_req(net, PF_DCP_OPT_DEVICE_PROPERTIES, PF_DCP_SUB_DEV_PROP_ID,
            &value_length, &p_value, &block_error) == 0)
         {
            (void)pf_dcp_put_block(p_dst, &dst_pos, 1500,
               PF_DCP_OPT_DEVICE_PROPERTIES, PF_DCP_SUB_DEV_PROP_ID, true, 0,
               value_length, p_value);
         }
         if (pf_cmina_dcp_get_req(net, PF_DCP_OPT_DEVICE_PROPERTIES, PF_DCP_SUB_DEV_PROP_OEM_ID,
            &value_length, &p_value, &block_error) == 0)
         {
            (void)pf_dcp_put_block(p_dst, &dst_pos, 1500,
               PF_DCP_OPT_DEVICE_PROPERTIES, PF_DCP_SUB_DEV_PROP_OEM_ID, true, 0,
               value_length, p_value);
         }
         if (pf_cmina_dcp_get_req(net, PF_DCP_OPT_DEVICE_INITIATIVE, PF_DCP_SUB_DEV_INITIATIVE_SUPPORT,
            &value_length, &p_value, &block_error) == 0)
         {
            temp16 = htons(*p_value);
            (void)pf_dcp_put_block(p_dst, &dst_pos, 1500,
               PF_DCP_OPT_DEVICE_INITIATIVE, PF_DCP_SUB_DEV_INITIATIVE_SUPPORT, true, 0,
               value_length, &temp16);
         }

         /* Insert final response length and ship it! */
         p_dcphdr->data_length = htons(dst_pos - dst_start_pos);
         p_buf->len = dst_pos;
         if (os_eth_send(net->eth_handle, p_buf) <= 0)
         {
            LOG_ERROR(PNET_LOG, "pf_dcp(%d): Error from os_eth_send(dcp)\n", __LINE__);
         }
      }
      os_buf_free(p_buf);
   }

   return 0;
}

/**
 * @internal
 * Handle a DCP identify request.
 *
 * The request may contain filter conditions. Only respond if ALL conditions
 * match.
 *
 * This is a callback for the frame handler. Arguments should fulfill pf_eth_frame_handler_t
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:   The frame id.
 * @param p_buf            In:   The ethernet frame.
 * @param frame_id_pos     In:   Position of the frame id in the buffer.
 * @param p_arg            In:   Not used.
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
static int pf_dcp_identify_req(
   pnet_t                  *net,
   uint16_t                frame_id,
   os_buf_t                *p_buf,
   uint16_t                frame_id_pos,
   void                    *p_arg)           /* Not used */
{
   int                     ret = 0;          /* Assume all OK */
   uint16_t                ix;
   bool                    first = true;
   bool                    match = false;    /* Is it for us? */
   bool                    filter = false;

   uint8_t                 *p_src;
   uint16_t                src_pos = 0;
   pf_ethhdr_t             *p_src_ethhdr;
   pf_dcp_header_t         *p_src_dcphdr;
   uint16_t                src_dcplen = 0;
   pf_dcp_block_hdr_t      *p_src_block_hdr;
   uint16_t                src_block_len;

   os_buf_t                *p_rsp;
   uint8_t                 *p_dst;
   uint16_t                dst_pos = 0;
   uint16_t                dst_start = 0;
   pf_ethhdr_t             *p_dst_ethhdr;
   pf_dcp_header_t         *p_dst_dcphdr;

   uint32_t                response_delay;
   uint16_t                value_length;
   uint8_t                 *p_value;
   uint8_t                 block_error;
   const pnet_cfg_t        *p_cfg = NULL;

   pf_fspm_get_default_cfg(net, &p_cfg);

   LOG_DEBUG(PF_DCP_LOG,"DCP(%d): Incoming DCP identify request\n", __LINE__);
   /*
    * IdentifyReqBlock = DeviceRoleBlock ^ DeviceVendorBlock ^ DeviceIDBlock ^
    * DeviceOptionsBlock ^ OEMDeviceIDBlock ^ MACAddressBlock ^ IPParameterBlock ^
    * DHCPParameterBlock ^ ManufacturerSpecificParameterBlock
    */
   p_rsp = os_buf_alloc(1500);        /* Get a transmit buffer for the response */
   if ((p_buf != NULL) && (p_rsp != NULL))
   {
      /* Setup access to the request */
      p_src = (uint8_t *)p_buf->payload;
      p_src_ethhdr = (pf_ethhdr_t *)p_src;
      src_pos = frame_id_pos;
      src_pos += sizeof(uint16_t);        /* FrameId */
      p_src_dcphdr = (pf_dcp_header_t *)&p_src[src_pos];
      src_pos += sizeof(pf_dcp_header_t);
      src_dcplen = (src_pos + ntohs(p_src_dcphdr->data_length));

      /* Prepare the response */
      p_dst = (uint8_t *)p_rsp->payload;
      dst_pos = 0;
      p_dst_ethhdr = (pf_ethhdr_t *)p_dst;
      dst_pos += sizeof(pf_ethhdr_t);
      /* frame ID */
      p_dst[dst_pos++] = ((PF_DCP_ID_RES_FRAME_ID & 0xff00) >> 8);
      p_dst[dst_pos++] = PF_DCP_ID_RES_FRAME_ID & 0xff;

      p_dst_dcphdr = (pf_dcp_header_t *)&p_dst[dst_pos];
      dst_pos += sizeof(pf_dcp_header_t);

      /* Save position for later */
      dst_start = dst_pos;

      memcpy(p_dst_ethhdr->dest.addr, p_src_ethhdr->src.addr, sizeof(pnet_ethaddr_t));
      memcpy(p_dst_ethhdr->src.addr, p_cfg->eth_addr.addr, sizeof(pnet_ethaddr_t));
      p_dst_ethhdr->type = htons(OS_ETHTYPE_PROFINET);

      /* Start with the request header and modify what is needed. */
      *p_dst_dcphdr = *p_src_dcphdr;
      p_dst_dcphdr->service_type = PF_DCP_SERVICE_TYPE_SUCCESS;
      p_dst_dcphdr->response_delay_factor = htons(0);

      /* The block header is expected to be 16-bit aligned */
      p_src_block_hdr = (pf_dcp_block_hdr_t *)&p_src[src_pos];
      src_pos += sizeof(*p_src_block_hdr);    /* Point to the block value */

      src_block_len = ntohs(p_src_block_hdr->block_length);

      match = true;     /* So far so good */
      while ((ret == 0) &&
             (first || (filter && match)) &&
             (src_dcplen >= (src_pos + src_block_len)) &&
             (dst_pos < 1500))
      {
         /*
          * At the start of the loop p_src_block_hdr shall point to the source block header and
          * src_pos shall point to the block value.
          * Also, src_block_len shall be set to block_length of the current buffer.
          */
         /*
          * DCP-Identify-ReqPDU = DCP-IdentifyFilter-ReqPDU ^ DCP-IdentifyAll-ReqPDU
          *
          * DCP-IdentifyAll-ReqPDU = DCP-MC-Header, AllSelectorOption (=0xff), SuboptionAllSelector (=0xff), DCPBlockLength (=0x02)
          * DCP-IdentifyFilter-ReqPDU = DCP-MC-Header, [NameOfStationBlock] ^ [AliasNameBlock], [IdentifyReqBlock] * a
          *
          * DCP-MC-Header = ServiceID, ServiceType, Xid, ResponseDelayFactor, DCPDataLength
          *
          * IdentifyReqBlock = DeviceRoleBlock ^ DeviceVendorBlock ^ DeviceIDBlock ^
          *    DeviceOptionsBlock ^ OEMDeviceIDBlock ^ MACAddressBlock ^ IPParameterBlock ^
          *    DHCPParameterBlock ^ ManufacturerSpecificParameterBlock
          */
         if (pf_cmina_dcp_get_req(net, p_src_block_hdr->option, p_src_block_hdr->sub_option,
            &value_length, &p_value, &block_error) == 0)
         {
            switch (p_src_block_hdr->option)
            {
            case PF_DCP_OPT_ALL:
               switch(p_src_block_hdr->sub_option)
               {
               case PF_DCP_SUB_ALL:
                  /* ToDo: Is there a bug in the PNIO tester? It sends src_block_len == 0! */
                  if ((first == true) && ((src_block_len == 0x02) || (src_block_len == 0x00)))
                  {
                     filter = false;
                  }
                  else
                  {
                     ret = -1;
                  }
                  break;
               default:
                  ret = -1;
                  break;
               }
               break;
            case PF_DCP_OPT_IP:
               if (filter == true)
               {
                  /*
                   * MACAddressBlock = 0x01, 0x01, DCPBlockLength, MACAddressValue
                   * IPParameterBlock = 0x01, 0x02, DCPBlockLength, IPAddress, Subnetmask, StandardGateway
                   * Note: No query for FullSuite
                   */
                  switch (p_src_block_hdr->sub_option)
                  {
                  case PF_DCP_SUB_IP_MAC:
                     if ((memcmp(p_value, &p_src[src_pos], value_length) != 0) ||
                         (src_block_len != value_length))
                     {
                        match = false;
                     }
                     break;
                  case PF_DCP_SUB_IP_PAR:
                     if ((memcmp(p_value, &p_src[src_pos], value_length) != 0) ||
                         (src_block_len != value_length))
                     {
                        match = false;
                     }
                     break;
                  default:
                     ret = -1;
                     break;
                  }
               }
               else
               {
                  ret = -1;
               }
               break;
            case PF_DCP_OPT_DEVICE_PROPERTIES:
               /*
                * DeviceVendorBlock = 0x02, 0x01, DCPBlockLength, DeviceVendorValue
                * NameOfStationBlock = 0x02, 0x02, DCPBlockLength, NameOfStationValue
                * DeviceIDBlock = 0x02, 0x03, DCPBlockLength, VendorIDHigh, VendorIDLow, DeviceIDHigh, DeviceIDLow
                * DeviceRoleBlock = 0x02, 0x04, DCPBlockLength, DeviceRoleValue
                * DeviceOptionsBlock = 0x02, 0x05, DCPBlockLength, (Option, Suboption)*
                * AliasNameBlock = 0x02, 0x06, DCPBlockLength, AliasNameValue
                * Note: No query for options 0x07 and 0x09
                * OEMDeviceIDBlock = 0x02, 0x08, DCPBlockLength, VendorIDHigh, VendorIDLow, DeviceIDHigh, DeviceIDLow
                */
               switch(p_src_block_hdr->sub_option)
               {
               case PF_DCP_SUB_DEV_PROP_VENDOR:
                  if (filter == true)
                  {
                     if ((memcmp(p_value, &p_src[src_pos], value_length) != 0) ||
                         (src_block_len != value_length))
                     {
                        match = false;
                     }
                  }
                  else
                  {
                     ret = -1;
                  }
                  break;
               case PF_DCP_SUB_DEV_PROP_NAME:
                  if ((memcmp(p_value, &p_src[src_pos], src_block_len) == 0) &&
                      (p_value[src_block_len] == '\0'))
                  {
                     if (first == true)
                     {
                        filter = true;
                     }
                  }
                  else
                  {
                     match = false;
                  }
                  break;
               case PF_DCP_SUB_DEV_PROP_ID:
                  if (filter == true)
                  {
                     if ((memcmp(p_value, &p_src[src_pos], value_length) != 0) ||
                         (src_block_len != value_length))
                     {
                        match = false;
                     }
                  }
                  else
                  {
                     ret = -1;
                  }
                  break;
               case PF_DCP_SUB_DEV_PROP_ROLE:
                  if (filter == true)
                  {
                     p_src_block_hdr->block_length += 1;
                     if ((memcmp(p_value, &p_src[src_pos], value_length) != 0) ||
                         (src_block_len != value_length))
                     {
                        match = false;
                     }
                  }
                  else
                  {
                     ret = -1;
                  }
                  break;
               case PF_DCP_SUB_DEV_PROP_OPTIONS:
                  if (filter == true)
                  {
                     if ((memcmp(p_value, &p_src[src_pos], value_length) != 0) ||
                         (src_block_len != value_length))
                     {
                        match = false;
                     }
                  }
                  else
                  {
                     ret = -1;
                  }
                  break;
               case PF_DCP_SUB_DEV_PROP_OEM_ID:
                  if (filter == true)
                  {
                     if ((memcmp(p_value, &p_src[src_pos], value_length) != 0) ||
                         (src_block_len != value_length))
                     {
                        match = false;
                     }
                  }
                  else
                  {
                     ret = -1;
                  }
                  break;
   #if 0
               /* Currently we do not support alias names */
               case PF_DCP_SUB_DEV_PROP_ALIAS:
                  if ((memcmp(p_value, &p_src[src_pos], value_length) != 0) ||
                      (src_block_len != value_length))
                  {
                     if (first == true)
                     {
                        filter = true;
                     }
                  }
                  else
                  {
                     match = false;
                  }
   #endif
                  break;
               case PF_DCP_SUB_DEV_PROP_INSTANCE:
                  if (filter == true)
                  {
                     if ((memcmp(p_value, &p_src[src_pos], value_length) != 0) ||
                         (src_block_len != value_length))
                     {
                        match = false;
                     }
                  }
                  else
                  {
                     ret = -1;
                  }
                  break;
               case PF_DCP_SUB_DEV_PROP_GATEWAY:
                  if (filter == true)
                  {
                     if ((memcmp(p_value, &p_src[src_pos], value_length) != 0) ||
                         (src_block_len != value_length))
                     {
                        match = false;
                     }
                  }
                  else
                  {
                     ret = -1;
                  }
                  break;
               default:
                  ret = -1;
                  break;
               }
               break;
            case PF_DCP_OPT_DHCP:
               if (filter == true)
               {
                  if ((memcmp(p_value, &p_src[src_pos], value_length) != 0) ||
                      (src_block_len != value_length))
                  {
                     match = false;
                  }
               }
               else
               {
                  ret = -1;
               }
               break;
            default:
               ret = -1;
               break;
            }
            src_pos += src_block_len;

            /* Skip padding to align on uint16_t */
            while (src_pos & 1)
            {
               src_pos++;
            }
            /* Prepare for the next round */
            p_src_block_hdr = (pf_dcp_block_hdr_t *)&p_src[src_pos];
            src_block_len = ntohs(p_src_block_hdr->block_length);
            src_pos += sizeof(*p_src_block_hdr);
         }
         else
         {
            ret = -1;   /* Give up on bad request */
         }

         first = false;
      }

      if ((ret == 0) && (match == true))
      {
         /* Build the response */
         for (ix = 0; ix < NELEMENTS(device_options); ix++)
         {
            pf_dcp_get_req(net, p_dst, &dst_pos, 1500,
               device_options[ix].opt, device_options[ix].sub);
         }

         /* Insert final response length and ship it! */
         p_dst_dcphdr->data_length = htons(dst_pos - dst_start);
         p_rsp->len = dst_pos;

         net->dcp_delayed_response_waiting = true;
         response_delay = ((p_cfg->eth_addr.addr[4] * 256 + p_cfg->eth_addr.addr[5]) % ntohs(p_src_dcphdr->response_delay_factor)) * 10;
         pf_scheduler_add(net, response_delay*1000,
            "dcp", pf_dcp_responder, p_rsp, &net->dcp_timeout);
      }
      else
      {
         os_buf_free(p_rsp);
      }
   }

   if (p_buf != NULL)
   {
      os_buf_free(p_buf);
   }

   return 1;      /* Means: handled */
}

void pf_dcp_exit(
   pnet_t                  *net)
{
   pf_eth_frame_id_map_remove(net, PF_DCP_HELLO_FRAME_ID);
   pf_eth_frame_id_map_remove(net, PF_DCP_GET_SET_FRAME_ID);
   pf_eth_frame_id_map_remove(net, PF_DCP_ID_REQ_FRAME_ID);
}

void pf_dcp_init(
   pnet_t                  *net)
{
   net->dcp_global_block_qualifier = 0;
   net->dcp_delayed_response_waiting = false;
   net->dcp_sam_timeout = 0;
   net->dcp_timeout = 0;
   net->dcp_sam = mac_nil;

   /* Insert handlers for our specific frame_ids */
   pf_eth_frame_id_map_add(net, PF_DCP_HELLO_FRAME_ID, pf_dcp_hello_ind, NULL);
   pf_eth_frame_id_map_add(net, PF_DCP_GET_SET_FRAME_ID, pf_dcp_get_set, NULL);
   pf_eth_frame_id_map_add(net, PF_DCP_ID_REQ_FRAME_ID, pf_dcp_identify_req, NULL);
}
