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
 * @brief Implements the Discovery and basic Configuration Protocol (DCP).
 *
 * DCP runs over Ethernet layer 2 (not IP or UDP).
 *
 * The main DCP messages are:
 *  - GET
 *  - SET
 *  - IDENTIFY (asking for a particular device)
 *  - HELLO (broadcast to indicate the presence of a device)
 *
 * Registers the different frame IDs for these messages with the frame handler.
 *
 * Responses to IDENTIFY are delayed (by an amount calculated from the
 * MAC address), as many devices might respond to the same request.
 *
 * The SAM (Source Address ?) makes sure that that just a single remote
 * MAC address is used for the communication.
 * A timer of 3 seconds is used for the SAM.
 */

#ifdef UNIT_TEST
#endif

#include <string.h>
#include <inttypes.h>

#include "pf_includes.h"
#include "pf_block_reader.h"
#include "pf_block_writer.h"

/*
 * Various constants from the standard document.
 */
#define PF_DCP_SERVICE_GET      0x03
#define PF_DCP_SERVICE_SET      0x04
#define PF_DCP_SERVICE_IDENTIFY 0x05
#define PF_DCP_SERVICE_HELLO    0x06

#define PF_DCP_BLOCK_PADDING 0x00

#define PF_DCP_SERVICE_TYPE_REQUEST       0x00
#define PF_DCP_SERVICE_TYPE_SUCCESS       0x01
#define PF_DCP_SERVICE_TYPE_NOT_SUPPORTED 0x05

#define PF_DCP_HEADER_SIZE    10
#define PF_DCP_BLOCK_HDR_SIZE 4

#define PF_DCP_HELLO_FRAME_ID   0xfefc
#define PF_DCP_GET_SET_FRAME_ID 0xfefd
#define PF_DCP_ID_REQ_FRAME_ID  0xfefe
#define PF_DCP_ID_RES_FRAME_ID  0xfeff

#define PF_DCP_MC_MAC_ADD_FILTER_BASE 0x20

#define PF_DCP_SIGNAL_LED_HALF_INTERVAL     500000 /* 0.5 seconds */
#define PF_DCP_SIGNAL_LED_NUMBER_OF_FLASHES 3

/* Client hold time, 3 seconds.
   See Profinet 2.4 Services, section 6.3.11.2.2 */
#define PF_DCP_SAM_TIMEOUT 3000000 /* microseconds */

CC_PACKED_BEGIN
typedef struct CC_PACKED pf_dcp_header
{
   uint8_t service_id;
   uint8_t service_type;
   uint32_t xid;
   uint16_t response_delay_factor;
   uint16_t data_length;
} pf_dcp_header_t;
CC_PACKED_END
CC_STATIC_ASSERT (PF_DCP_HEADER_SIZE == sizeof (pf_dcp_header_t));

CC_PACKED_BEGIN
typedef struct CC_PACKED pf_dcp_block_hdr
{
   uint8_t option;
   uint8_t sub_option;
   uint16_t block_length;
} pf_dcp_block_hdr_t;
CC_PACKED_END
CC_STATIC_ASSERT (PF_DCP_BLOCK_HDR_SIZE == sizeof (pf_dcp_block_hdr_t));

/* Some values that can be set in a SET request block.
 *
 * Note: A block can only contain one of these values, so making this a
 * union instead of a struct is safe while saving stack space.
 */
typedef union pf_dcp_set_request_block_value
{
   pf_ip_suite_t ip_suite;
   pf_full_ip_suite_t full_ip_suite;
   char station_name[PNET_STATION_NAME_MAX_SIZE];
} pf_dcp_set_request_block_value_t;

typedef struct pf_dcp_parsed_set_request_block
{
   uint8_t opt;
   uint8_t sub;
   pf_dcp_block_error_values_t error;
} pf_dcp_parsed_set_request_block_t;

/*
 * This is the standard DCP HELLO broadcast address (MAC).
 */
static const pnet_ethaddr_t dcp_mc_addr_hello = {
   {0x01, 0x0e, 0xcf, 0x00, 0x00, 0x01}};

#define PF_MAC_NIL                                                             \
   {                                                                           \
      {                                                                        \
         0, 0, 0, 0, 0, 0                                                      \
      }                                                                        \
   }

static const pnet_ethaddr_t mac_nil = PF_MAC_NIL;

/*
 * A list of options that we can get/set/filter.
 * Use in identify filter and identify response.
 */
typedef struct pf_dcp_opt_sub
{
   uint8_t opt;
   uint8_t sub;
} pf_dcp_opt_sub_t;

/* Device options to be included for example in DCP IDENTIFY responses */
static const pf_dcp_opt_sub_t device_options[] = {
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

#if PNET_OPTION_SECURITY_LEVEL_1
/**
 * @internal
 * Check if any active AR has the "RejectDCPsetRequests" property set.
 *
 * @param net              InOut: The p-net stack instance
 * @return true if any active AR has the reject_dcp_set_requests property set
 *         false if not.
 */
static bool pf_dcp_reject_set_requests (pnet_t * net)
{
   uint16_t ix;
   for (ix = 0; ix < NELEMENTS (net->cmrpc_ar); ix++)
   {
      if (net->cmrpc_ar[ix].in_use)
      {
         if (net->cmrpc_ar[ix].ar_param.ar_properties.reject_dcp_set_requests)
         {
            return true;
         }
      }
   }
   return false;
}
#endif /* PNET_OPTION_SECURITY_LEVEL_1 */

/**
 * @internal
 * Send a delayed DCP response to an IDENTIFY request.
 *
 * The delay is important as many devices might respond to the same request.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    DCP responder data. Should be pnal_buf_t.
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 *                                stored tasks. Not used here.
 */
static void pf_dcp_responder (pnet_t * net, void * arg, uint32_t current_time)
{
   pnal_buf_t * p_buf = (pnal_buf_t *)arg;

   pf_scheduler_reset_handle (&net->dcp_identresp_timeout);

   if (p_buf == NULL)
   {
      return;
   }

   if (pf_eth_send_on_management_port (net, p_buf) > 0)
   {
      LOG_DEBUG (PNET_LOG, "DCP(%d): Sent a DCP identify response.\n", __LINE__);
   }

   pnal_buf_free (p_buf);
   net->dcp_delayed_response_waiting = false;
}

/**
 * @internal
 * Clear SAM
 * SAM limits the communication to a single remote MAC address
 * for 3 seconds. This operation implements the timeout callback.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    Not used.
 * @param current_time     In:    Not used.
 */
static void pf_dcp_clear_sam (pnet_t * net, void * arg, uint32_t current_time)
{
   LOG_DEBUG (
      PF_DCP_LOG,
      "DCP(%d): SAM timeout. Clear stored remote MAC address.\n",
      __LINE__);
   net->dcp_sam = mac_nil;
   pf_scheduler_reset_handle (&net->dcp_sam_timeout);
}

/**
 * @internal
 * Restart SAM timeout
 * SAM limits the communication to a single remote MAC address
 * for 3 seconds.
 *
 * @param net              InOut: The p-net stack instance
 * @param mac              In:    Remote MAC address
 */
static void pf_dcp_restart_sam_timeout (pnet_t * net, const pnet_ethaddr_t * mac)
{
   /* Make sure no other MAC address is used in the DCP communication
    * for 3 seconds */
   LOG_DEBUG (
      PF_DCP_LOG,
      "DCP(%d): Update SAM remote MAC address, and restart timeout.\n",
      __LINE__);

   memcpy (&net->dcp_sam, mac, sizeof (net->dcp_sam));
   (void)pf_scheduler_restart (
      net,
      PF_DCP_SAM_TIMEOUT,
      pf_dcp_clear_sam,
      NULL,
      &net->dcp_sam_timeout);
}

/**
 * @internal
 * Check SAM
 * SAM limits the communication to a single remote MAC address
 * for 3 seconds.
 *
 * @param net              InOut: The p-net stack instance
 * @param mac              In: Remote MAC address
 * @return true if DCP communication is allowed with respect to SAM.
 *         false if not.
 */
static bool pf_dcp_check_sam (pnet_t * net, const pnet_ethaddr_t * mac)
{
   return ((memcmp (&net->dcp_sam, &mac_nil, sizeof (net->dcp_sam))) == 0) ||
          (memcmp (&net->dcp_sam, mac, sizeof (net->dcp_sam)) == 0);
}

/**
 * @internal
 * Check that address is not a multi-cast address
 * and also that it matches the local MAC address.
 *
 * This check was added to handle situation where network
 * interface is in promiscuous mode and the need to filter out
 * DCP requests intended for other devices.
 *
 * @param local_mac        In: Local MAC address. Should be the
 *                             MAC address of main (DAP) interface.
 * @param destination_mac  In: Destination MAC address
 * @return true if destination address is ok and the packet shall
 *         be accepted. false if not.
 */
bool pf_dcp_check_destination_address (
   const pnet_ethaddr_t * local_mac,
   const pnet_ethaddr_t * destination_mac)
{
   return (
      ((destination_mac->addr[0] & 1) == 0) && /* Not multi-cast */
      (memcmp (destination_mac, local_mac, sizeof (*destination_mac)) == 0));
}

/**
 * @internal
 * Check that address matches the local MAC address,
 * or that it matches one of two multi-cast addresses:
 * - 01:0e:cf:00:00:00 (used with Identify All)
 * - 01:0e:cf:00:00:xx
 *   where xx = 0x20 + ({md5 hash of station name} & 0x1f).
 *
 * This check is needed to support the "pruning" feature.
 * If PNET_OPTION_MC_PRUNING is disabled then this function will allow
 * every destination address.
 *
 * @param local_mac        In: Local MAC address. Should be the
 *                             MAC address of main (DAP) interface.
 * @param dest_mac         In: Destination MAC address
 * @return true if destination address is ok and shall packet shall
 *         be accepted. false if not.
 */
bool pf_dcp_filter_destination_address (
   pnet_t * net,
   const pnet_ethaddr_t * dest_mac)
{
#if PNET_OPTION_MC_PRUNING
   /* PN-AL-protocol:
    * Table 21 – DCP_MulticastMACAdd range for filterable Identify
    */
   uint8_t selector = PF_DCP_MC_MAC_ADD_FILTER_BASE;

   /* PN-AL-protocol: Formula (4) */
   selector += net->md5_out[15] & 0x1F;
   return (
      (memcmp (dest_mac, &dcp_mc_addr_hello, sizeof (*dest_mac))) &&
      ((dest_mac->addr[5] == 0) || (dest_mac->addr[5] == selector)));
#else
   return true;
#endif
}

/**
 * @internal
 * Extract a DCP header from a frame.
 *
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_hdr            Out:   Destination buffer.
 */
static void pf_dcp_get_header (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_dcp_header_t * p_hdr)
{
   p_hdr->service_id = pf_get_byte (p_info, p_pos);
   p_hdr->service_type = pf_get_byte (p_info, p_pos);
   p_hdr->xid = pf_get_uint32 (p_info, p_pos);
   p_hdr->response_delay_factor = pf_get_uint16 (p_info, p_pos);
   p_hdr->data_length = pf_get_uint16 (p_info, p_pos);
}

/**
 * @internal
 * Extract a DCP block header from a frame.
 *
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_hdr            Out:   Destination buffer.
 */
static void pf_dcp_get_block_header (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_dcp_block_hdr_t * p_hdr)
{
   p_hdr->option = pf_get_byte (p_info, p_pos);
   p_hdr->sub_option = pf_get_byte (p_info, p_pos);
   p_hdr->block_length = pf_get_uint16 (p_info, p_pos);
}

/**
 * @internal
 * Add a block to a buffer, possibly including a block_info.
 *
 * @param p_dst            InOut: Destination buffer info.
 * @param p_dst_pos        InOut: Position in the destination buffer.
 * @param opt              In:    Option key.
 * @param sub              In:    Sub-option key.
 * @param with_block_info  In:    If true then add block_info argument.
 * @param block_info       In:    The block info argument.
 * @param value_length     In:    The length in bytes of the p_value data.
 * @param p_value          In:    The source data.
 * @return
 */
static int pf_dcp_put_block (
   pf_put_info_t * p_dst,
   uint16_t * p_dst_pos,
   uint8_t opt,
   uint8_t sub,
   bool with_block_info,
   uint16_t block_info,
   uint16_t value_length, /* Not including length of block_info */
   const void * p_value)
{
   uint16_t b_len;

   if ((*p_dst_pos + value_length) >= p_dst->len)
   {
      return 0; /* Skip excess data silently */
   }

   /* Adjust written block length if block_info is included */
   b_len = value_length;
   if (with_block_info)
   {
      b_len += sizeof (b_len);
   }

   pf_put_byte (opt, p_dst, p_dst_pos);
   pf_put_byte (sub, p_dst, p_dst_pos);
   pf_put_uint16 (true, b_len, p_dst, p_dst_pos);
   if (with_block_info == true)
   {
      pf_put_uint16 (true, block_info, p_dst, p_dst_pos);
   }
   if ((p_value != NULL) && (value_length > 0))
   {
      pf_put_mem (p_value, value_length, p_dst, p_dst_pos);
   }

   /* Add padding to align on uint16_t */
   while ((*p_dst_pos) & 1)
   {
      pf_put_byte (0, p_dst, p_dst_pos);
   }

   return 0;
}

/**
 * @internal
 * Handle one block in a DCP get request, by inserting it into the response.
 *
 * @param net                 InOut: The p-net stack instance
 * @param p_dst               InOut: Destination buffer info.
 * @param p_dst_pos           InOut: Position in the destination buffer.
 * @param opt                 In:    Option key.
 * @param sub                 In:    Sub-option key.
 * @param request_is_identify In:    Usage in response to Identify request
 *                                   (skips some blocks)
 * @param alias_name          In:    Alias name appended if != NULL
 * @return
 */
static int pf_dcp_get_req (
   pnet_t * net,
   pf_put_info_t * p_dst,
   uint16_t * p_dst_pos,
   uint8_t opt,
   uint8_t sub,
   bool request_is_identify,
   const char * alias_name)
{
   int ret = 0; /* Assume all OK */
   uint8_t block_error = PF_DCP_BLOCK_ERROR_NO_ERROR;
   uint8_t negative_response_data[3]; /* For negative get */
   uint16_t block_info = 0;
   uint16_t value_length = 0;
   uint8_t * p_value = NULL;
   bool skip = false; /* When true: Do not insert block in response */
   pf_full_ip_suite_t full_ip_suite;
   uint16_t temp16;
   uint8_t device_role[2]; /* DeviceRoleValue: DeviceRoleDetails, Padding */
   uint16_t ix;

   /* Get the data */
   ret = pf_cmina_dcp_get_req (
      net,
      opt,
      sub,
      &value_length,
      &p_value,
      &block_error);

   /* Some very specific options require special treatment */
   switch (opt)
   {
   case PF_DCP_OPT_IP:
      switch (sub)
      {
      case PF_DCP_SUB_IP_PAR:
         memcpy (
            &full_ip_suite.ip_suite,
            p_value,
            sizeof (full_ip_suite.ip_suite));
         full_ip_suite.ip_suite.ip_addr =
            CC_TO_BE32 (full_ip_suite.ip_suite.ip_addr);
         full_ip_suite.ip_suite.ip_mask =
            CC_TO_BE32 (full_ip_suite.ip_suite.ip_mask);
         full_ip_suite.ip_suite.ip_gateway =
            CC_TO_BE32 (full_ip_suite.ip_suite.ip_gateway);
         p_value = (uint8_t *)&full_ip_suite.ip_suite;

         block_info = ((full_ip_suite.ip_suite.ip_addr == 0) &&
                       (full_ip_suite.ip_suite.ip_mask == 0) &&
                       (full_ip_suite.ip_suite.ip_gateway == 0))
                         ? 0
                         : BIT (0);
         /* ToDo: We do not yet support DHCP (block_info, BIT(1)) */
         /* ToDo: We do not yet report on "IP address conflict" (block_info,
          * BIT(7)) */
         break;
      case PF_DCP_SUB_IP_SUITE:
         memcpy (&full_ip_suite, p_value, sizeof (full_ip_suite));
         full_ip_suite.ip_suite.ip_addr =
            CC_TO_BE32 (full_ip_suite.ip_suite.ip_addr);
         full_ip_suite.ip_suite.ip_mask =
            CC_TO_BE32 (full_ip_suite.ip_suite.ip_mask);
         full_ip_suite.ip_suite.ip_gateway =
            CC_TO_BE32 (full_ip_suite.ip_suite.ip_gateway);
         for (ix = 0; ix < NELEMENTS (full_ip_suite.dns_addr); ix++)
         {
            full_ip_suite.dns_addr[ix] = CC_TO_BE32 (full_ip_suite.dns_addr[ix]);
         }
         p_value = (uint8_t *)&full_ip_suite;

         block_info = ((full_ip_suite.ip_suite.ip_addr == 0) &&
                       (full_ip_suite.ip_suite.ip_mask == 0) &&
                       (full_ip_suite.ip_suite.ip_gateway == 0))
                         ? 0
                         : BIT (0);
         /* ToDo: We do not yet support DHCP (block_info, BIT(1)) */
         /* ToDo: We do not yet report on "IP address conflict" (block_info,
          * BIT(7)) */
         break;
      case PF_DCP_SUB_IP_MAC:
         skip = request_is_identify;
         break;
      default:
         break;
      }
      break;
   case PF_DCP_OPT_DEVICE_PROPERTIES:
      switch (sub)
      {
      case PF_DCP_SUB_DEV_PROP_OPTIONS:
         if (sizeof (device_options) > 0)
         {
            p_value = (uint8_t *)device_options;
            value_length = sizeof (device_options);
            ret = 0;
         }
         else
         {
            skip = true;
            ret = 0; /* This is OK - just omit the entire block. */
         }
         break;
      case PF_DCP_SUB_DEV_PROP_GATEWAY:
         if (value_length == sizeof (temp16))
         {
            memcpy (&temp16, p_value, sizeof (temp16));
            temp16 = CC_TO_BE16 (temp16);
            p_value = (uint8_t *)&temp16;
         }
         else
         {
            ret = -1;
         }
         break;
      case PF_DCP_SUB_DEV_PROP_NAME:
         value_length = (uint16_t)strlen ((char *)p_value);
         break;
      case PF_DCP_SUB_DEV_PROP_ALIAS:
         skip = (alias_name == NULL);
         if (!skip)
         {
            value_length = (uint16_t)strlen (alias_name);
            p_value = (uint8_t *)alias_name;
            ret = 0;
         }
         break;
      case PF_DCP_SUB_DEV_PROP_VENDOR:
         value_length = (uint16_t)strlen ((char *)p_value);
         break;
      case PF_DCP_SUB_DEV_PROP_ROLE:
         device_role[0] = *p_value;
         device_role[1] = 0; /* Padding */
         p_value = device_role;
         value_length = sizeof (device_role);
         break;
      case PF_DCP_SUB_DEV_PROP_ID:
      case PF_DCP_SUB_DEV_PROP_INSTANCE:
      case PF_DCP_SUB_DEV_PROP_OEM_ID:
      default:
         break;
      }
      break;
   case PF_DCP_OPT_DEVICE_INITIATIVE:
      if (value_length == sizeof (temp16))
      {
         memcpy (&temp16, p_value, sizeof (temp16));
         temp16 = CC_TO_BE16 (temp16);
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

   if (skip == false)
   {
      if (ret == 0)
      {
         ret = pf_dcp_put_block (
            p_dst,
            p_dst_pos,
            opt,
            sub,
            true,
            block_info,
            value_length,
            p_value);
      }
      else
      {
         /* GetNegResBlock consists of:
          * - option = Control                              1 byte
          * - suboption = Response                          1 byte
          * - block length                                  2 bytes
          * - type of option involved (option + suboption)  2 bytes
          * - block error                                   1 byte
          */
         negative_response_data[0] = opt;
         negative_response_data[1] = sub;
         negative_response_data[2] = block_error;
         ret = pf_dcp_put_block (
            p_dst,
            p_dst_pos,
            PF_DCP_OPT_CONTROL,
            PF_DCP_SUB_CONTROL_RESPONSE,
            false,
            0,
            sizeof (negative_response_data),
            negative_response_data);
      }
   }

   return ret;
}

/**
 * @internal
 * Blink Profinet signal LED.
 *
 * This functions blinks the Profinet signal LED 3 times at 1 Hz.
 * The LED is accessed via the pnet_signal_led_ind() callback function.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    The current state (number of remaining
 *                                transitions)
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 *                                stored tasks.
 *                                Not used here.
 */
static void pf_dcp_control_signal_led (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{
   uint32_t state = (uint32_t)(uintptr_t)arg;

   if ((state % 2) == 1)
   {
      /* Turn LED on */
      if (pf_fspm_signal_led_ind (net, true) != 0)
      {
         LOG_ERROR (
            PF_DCP_LOG,
            "DCP(%d): Could not turn signal LED on\n",
            __LINE__);
      }
   }
   else
   {
      /* Turn LED off */
      if (pf_fspm_signal_led_ind (net, false) != 0)
      {
         LOG_ERROR (
            PF_DCP_LOG,
            "DCP(%d): Could not turn signal LED off\n",
            __LINE__);
      }
   }

   if ((state > 0) && (state < 200)) /* Plausibility test */
   {
      state--;

      /* Schedule another round */
      (void)pf_scheduler_add (
         net,
         PF_DCP_SIGNAL_LED_HALF_INTERVAL,
         pf_dcp_control_signal_led,
         (void *)(uintptr_t)state,
         &net->dcp_led_timeout);
   }
   else
   {
      pf_scheduler_reset_handle (&net->dcp_led_timeout);
   }
}

/**
 * @internal
 * Trigger blinking Profinet signal LED.
 *
 * If the LED already is blinking, do not restart the blinking.
 *
 * @param net              InOut: The p-net stack instance
 * @return 0 on success
 *         -1 on error
 */
int pf_dcp_trigger_signal_led (pnet_t * net)
{
   if (pf_scheduler_is_running (&net->dcp_led_timeout) == false)
   {
      LOG_INFO (
         PF_DCP_LOG,
         "DCP(%d): Received request to flash LED\n",
         __LINE__);
      (void)pf_scheduler_add (
         net,
         PF_DCP_SIGNAL_LED_HALF_INTERVAL,
         pf_dcp_control_signal_led,
         (void *)(2 * PF_DCP_SIGNAL_LED_NUMBER_OF_FLASHES - 1),
         &net->dcp_led_timeout);
   }
   else
   {
      LOG_INFO (
         PF_DCP_LOG,
         "DCP(%d): Received request to flash LED, but it is already "
         "flashing.\n",
         __LINE__);
   }

   return 0;
}

/**
 * @internal
 * Parse the value in one block in a DCP set request.
 *
 * Triggers the application callback \a pnet_reset_ind() for some values.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_buf            In:    The source frame buffer.
 * @param value_pos        In:    Position in source frame buffer.
 * @param value_length     In:    The size in bytes of the block value.
 * @param opt              In:    Option key.
 * @param sub              In:    Sub-option key.
 * @param block_qualifier  In:    Block qualifier.
 * @param p_result         Out:   The result of the parsing.
 */
static void pf_dcp_parse_block_value_in_set_request (
   pnet_t * net,
   const pnal_buf_t * p_buf,
   uint16_t value_pos,
   uint16_t value_length,
   uint8_t opt,
   uint8_t sub,
   uint16_t block_qualifier,
   pf_dcp_parsed_set_request_block_t * p_result)
{
   int ret = -1;
   pf_dcp_set_request_block_value_t value;
   pf_get_info_t value_info =
   {
      .is_big_endian = true,
      .p_frame = p_buf,
      .p_array = NULL,
      .len = pnal_buf_get_size (p_buf),
      .result = PF_PARSE_OK,
   };

   p_result->opt = opt;
   p_result->sub = sub;
   /* Assume negative response initially */
   p_result->error = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;

#if PNET_OPTION_SECURITY_LEVEL_1
   /* Reject all set requests except signal LED when an AR with
    * reject_dcp_set_requests property set is active.
    */
   if (
      pf_dcp_reject_set_requests (net) &&
      (!((opt == PF_DCP_OPT_CONTROL) && (sub == PF_DCP_SUB_CONTROL_SIGNAL))))
   {
      LOG_DEBUG (PNET_LOG, "DCP(%d): reject set request.\n", __LINE__);
      p_result->error = PF_DCP_BLOCK_ERROR_SET_NOT_POSSIBLE;
      return;
   }
#endif /* PNET_OPTION_SECURITY_LEVEL_1 */

   switch (opt)
   {
   case PF_DCP_OPT_IP:
      switch (sub)
      {
      case PF_DCP_SUB_IP_MAC:
         /* Can't set MAC address */
         break;
      case PF_DCP_SUB_IP_PAR:
         if (value_length == sizeof (value.ip_suite))
         {
            pf_get_ip_suite (&value_info, &value_pos, &value.ip_suite);

            if (value_info.result == PF_PARSE_OK)
            {
               ret = pf_cmina_dcp_set_ind (
                  net,
                  opt,
                  sub,
                  block_qualifier,
                  value_length,
                  (uint8_t *)&value.ip_suite,
                  &p_result->error);
            }
         }
         break;
      case PF_DCP_SUB_IP_SUITE:
         if (value_length == sizeof (value.full_ip_suite))
         {
            pf_get_full_ip_suite (
               &value_info,
               &value_pos,
               &value.full_ip_suite);

            if (value_info.result == PF_PARSE_OK)
            {
               ret = pf_cmina_dcp_set_ind (
                  net,
                  opt,
                  sub,
                  block_qualifier,
                  value_length,
                  (uint8_t *)&value.full_ip_suite,
                  &p_result->error);
            }
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
         pf_get_mem_s (
            &value_info,
            &value_pos,
            value_length,
            value.station_name,
            sizeof (value.station_name));

         if (value_info.result == PF_PARSE_OK)
         {
            ret = pf_cmina_dcp_set_ind (
               net,
               opt,
               sub,
               block_qualifier,
               value_length,
               (uint8_t *)value.station_name,
               &p_result->error);
         }
         break;
      case PF_DCP_SUB_DEV_PROP_VENDOR:
      case PF_DCP_SUB_DEV_PROP_ID:
      case PF_DCP_SUB_DEV_PROP_ROLE:
      case PF_DCP_SUB_DEV_PROP_OPTIONS:
      case PF_DCP_SUB_DEV_PROP_ALIAS:
      case PF_DCP_SUB_DEV_PROP_INSTANCE:
      case PF_DCP_SUB_DEV_PROP_OEM_ID:
      case PF_DCP_SUB_DEV_PROP_GATEWAY:
         /* Can't set these */
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
         p_result->error = PF_DCP_BLOCK_ERROR_NO_ERROR;
         ret = 0;
         break;
      case PF_DCP_SUB_CONTROL_STOP:
         if (block_qualifier == net->dcp_global_block_qualifier)
         {
            p_result->error = PF_DCP_BLOCK_ERROR_NO_ERROR;
            ret = 0;
         }
         else
         {
            ret = 0;
         }
         break;
      case PF_DCP_SUB_CONTROL_SIGNAL:
         /* Schedule a state-machine that flashes "the LED" for 3s, 1Hz. */
         ret = pf_dcp_trigger_signal_led (net);
         break;
      case PF_DCP_SUB_CONTROL_RESPONSE:
         /* Can't set this */
         break;
      case PF_DCP_SUB_CONTROL_FACTORY_RESET:
      case PF_DCP_SUB_CONTROL_RESET_TO_FACTORY:
         if (
            value_length != 0 ||
            pf_cmina_dcp_set_ind (
               net,
               opt,
               sub,
               block_qualifier,
               0,
               NULL, /* No value to set */
               &p_result->error) != 0)
         {
            LOG_ERROR (
               PF_DCP_LOG,
               "DCP(%d): Could not perform Factory Reset\n",
               __LINE__);
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
      p_result->error = PF_DCP_BLOCK_ERROR_OPTION_NOT_SUPPORTED;
      break;
   }

   if (ret == 0)
   {
      p_result->error = PF_DCP_BLOCK_ERROR_NO_ERROR;
   }
}

/**
 * @internal
 * Insert a DCP header in a buffer.
 *
 * @param p_hdr            In:    The DCP header to insert.
 * @param p_dst            InOut: Destination buffer info.
 * @param p_dst_pos        InOut: Position in the destination buffer.
 */
static void pf_dcp_put_header (
   const pf_dcp_header_t * p_hdr,
   pf_put_info_t * p_dst,
   uint16_t * p_dst_pos)
{
   pf_put_byte (p_hdr->service_id, p_dst, p_dst_pos);
   pf_put_byte (p_hdr->service_type, p_dst, p_dst_pos);
   pf_put_uint32 (true, p_hdr->xid, p_dst, p_dst_pos);
   pf_put_uint16 (true, p_hdr->response_delay_factor, p_dst, p_dst_pos);
   pf_put_uint16 (true, p_hdr->data_length, p_dst, p_dst_pos);
}

/**
 * @internal
 * Insert one block in a DCP set response.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_dst            InOut: Destination buffer info.
 * @param p_dst_pos        InOut: Position in the destination buffer.
 * @param p_result         In:    The result of the parsed request.
 */
static void pf_dcp_put_block_in_set_response (
   pnet_t * net,
   pf_put_info_t * p_dst,
   uint16_t * p_dst_pos,
   const pf_dcp_parsed_set_request_block_t * p_result)
{
   const uint8_t response_data[3] =
   {
      p_result->opt,
      p_result->sub,
      p_result->error,
   };

   /* SetResBlock and SetNegResBlock consists of:
    * - option = Control                              1 byte
    * - suboption = Response                          1 byte
    * - block length                                  2 bytes
    * - type of option involved (option + suboption)  2 bytes
    * - block error                                   1 byte  (=0 when no error)
    */
   (void)pf_dcp_put_block (
      p_dst,
      p_dst_pos,
      PF_DCP_OPT_CONTROL,
      PF_DCP_SUB_CONTROL_RESPONSE,
      false,
      0,
      sizeof (response_data),
      response_data);
}

/**
 * @internal
 * Handle a DCP Set or Get request.
 *
 * Triggers the application callback \a pnet_reset_ind() for some values.
 * Might set the IP address.
 *
 * This is a callback for the frame handler. Arguments should fulfill
 * pf_eth_frame_handler_t
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:    The frame id.
 * @param p_buf            InOut: The Ethernet frame. Will be freed.
 * @param frame_id_pos     In:    Position of the frame id in the buffer.
 *                                Depends on whether VLAN tagging is used.
 * @param p_arg            In:    Not used.
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
static int pf_dcp_get_set (
   pnet_t * net,
   uint16_t frame_id,
   pnal_buf_t * p_buf,
   uint16_t frame_id_pos,
   void * p_arg)
{
   uint16_t src_pos;
   uint16_t src_dcplen;
   pf_ethernet_header_t src_ethhdr;
   pf_dcp_header_t src_dcphdr;
   pnal_buf_t * p_rsp = NULL;
   uint16_t dst_pos;
   uint16_t dst_start;
   const pnet_ethaddr_t * mac_address = pf_cmina_get_device_macaddr (net);
   uint8_t dst_service_type = PF_DCP_SERVICE_TYPE_SUCCESS; /* Assume success */

   if (p_buf == NULL)
   {
      return 1; /* Buffer handled */
   }

   /* Extract info from the request */
   src_pos = 0;
   pf_get_info_t src_info =
   {
      .is_big_endian = true,
      .p_frame = p_buf,
      .p_array = NULL,
      .len = pnal_buf_get_size (p_buf),
      .result = PF_PARSE_OK,
   };
   pf_get_ethernet_header (&src_info, &src_pos, &src_ethhdr);
   src_pos = frame_id_pos;
   src_pos += sizeof (uint16_t); /* FrameId */
   pf_dcp_get_header (&src_info, &src_pos, &src_dcphdr);
   src_dcplen = src_pos + src_dcphdr.data_length;

   if (src_info.result != PF_PARSE_OK || src_dcplen > src_info.len)
   {
      goto out;
   }

   if (pf_dcp_check_sam (net, &src_ethhdr.src) == false)
   {
      goto out;
   }

   if (pf_dcp_check_destination_address (mac_address, &src_ethhdr.dest) == false)
   {
      goto out;
   }

   p_rsp = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE); /* Get a transmit buffer
                                                     for the response */
   if (p_rsp == NULL)
   {
      goto out;
   }

   /* Prepare the response */
   pf_put_info_t dst =
   {
      .p_frame = p_rsp,
      .p_array = NULL,
      .len = pnal_buf_get_size (p_rsp),
      .result = PF_PUT_OK,
   };

   /* Go to first block */
   dst_pos = 0;
   dst_pos += sizeof (pf_ethernet_header_t);
   dst_pos += sizeof (uint16_t); /* FrameId */
   dst_pos += sizeof (pf_dcp_header_t);

   /* Save position for later */
   dst_start = dst_pos;

   if (src_dcphdr.service_id == PF_DCP_SERVICE_SET)
   {
      LOG_DEBUG (
         PF_DCP_LOG,
         "DCP(%d): Incoming DCP Set request. Xid: %" PRIu32 "\n",
         __LINE__,
         src_dcphdr.xid);

      while (src_pos < src_dcplen)
      {
         pf_dcp_block_hdr_t src_block_hdr;
         uint16_t src_block_len;
         uint16_t src_block_qualifier;
         pf_dcp_parsed_set_request_block_t result;

         pf_dcp_get_block_header (&src_info, &src_pos, &src_block_hdr);
         src_block_qualifier = pf_get_uint16 (&src_info, &src_pos);
         src_block_len = src_block_hdr.block_length -  sizeof (uint16_t);

         if (src_info.result != PF_PARSE_OK || src_pos + src_block_len > src_dcplen)
         {
            break; /* Bad block - ignore remaining blocks */
         }

         pf_dcp_parse_block_value_in_set_request (
            net,
            p_buf,
            src_pos,
            src_block_len,
            src_block_hdr.option,
            src_block_hdr.sub_option,
            src_block_qualifier,
            &result);
         pf_dcp_put_block_in_set_response (
            net,
            &dst,
            &dst_pos,
            &result);

         /* Point to next block */
         src_pos += src_block_len;

         /* Skip padding to align on uint16_t */
         while (src_pos & 1)
         {
            src_pos++;
         }
      }

      pf_dcp_restart_sam_timeout (net, &src_ethhdr.src);
   }
   else if (src_dcphdr.service_id == PF_DCP_SERVICE_GET)
   {
      LOG_DEBUG (
         PF_DCP_LOG,
         "DCP(%d): Incoming DCP Get request. Xid: %" PRIu32 "\n",
         __LINE__,
         src_dcphdr.xid);
      while (src_dcplen >= (src_pos + sizeof (uint8_t) + sizeof (uint8_t)))
      {
         uint8_t option = pf_get_byte (&src_info, &src_pos);
         uint8_t sub_option = pf_get_byte (&src_info, &src_pos);

         if (src_info.result != PF_PARSE_OK)
         {
            break; /* Bad block - ignore remaining blocks */
         }

         (void)pf_dcp_get_req (
            net,
            &dst,
            &dst_pos,
            option,
            sub_option,
            false,
            false);
      }

      pf_dcp_restart_sam_timeout (net, &src_ethhdr.src);
   }
   else
   {
      LOG_ERROR (
         PF_DCP_LOG,
         "DCP(%d): Unknown DCP service id %u. Xid: %" PRIu32 "\n",
         __LINE__,
         (unsigned int)src_dcphdr.service_id,
         src_dcphdr.xid);
      dst_service_type = PF_DCP_SERVICE_TYPE_NOT_SUPPORTED;
   }

   uint16_t dst_data_length = dst_pos - dst_start;

   pnal_buf_set_size (p_rsp, dst_pos);
   dst_pos = 0;

   /* Set eth header in the response */
   const pf_ethernet_header_t ethhdr =
   {
      .dest = src_ethhdr.src,
      .src = *mac_address,
      .type = PNAL_ETHTYPE_PROFINET,
   };
   pf_put_ethernet_header (&ethhdr, &dst, &dst_pos);

   /* Insert frame ID into response */
   pf_put_uint16 (
      true,
      PF_DCP_GET_SET_FRAME_ID,
      &dst,
      &dst_pos);

   /* Copy DCP header from the request, and modify what is needed. */
   const pf_dcp_header_t dcphdr =
   {
      .service_id = src_dcphdr.service_id,
      .service_type = dst_service_type,
      .xid = src_dcphdr.xid,
      .response_delay_factor = 0,
      .data_length = dst_data_length,
   };
   pf_dcp_put_header (&dcphdr, &dst, &dst_pos);

   if (dst.result != PF_PUT_OK)
   {
      LOG_ERROR (
         PF_DCP_LOG,
         "DCP(%d): Could not build DCP Get/Set response\n",
         __LINE__);
      goto out;
   }

   if (pf_eth_send_on_management_port (net, p_rsp) > 0)
   {
      LOG_DEBUG (PF_DCP_LOG, "DCP(%d): Sent DCP Get/Set response\n", __LINE__);
   }

   if (src_dcphdr.service_id == PF_DCP_SERVICE_SET)
   {
      pf_cmina_dcp_set_commit (net);
   }

   /* Send LLDP _after_ the response in order to pass I/O-tester tests. */
   pf_pdport_lldp_restart_transmission (net);

out:
   if (p_buf != NULL)
   {
      pnal_buf_free (p_buf);
   }
   if (p_rsp != NULL)
   {
      pnal_buf_free (p_rsp);
   }

   return 1; /* Buffer handled */
}

/**
 * @internal
 * Handle an incoming HELLO message
 *
 * The HELLO indication is used by another device to find its MC peer.
 *
 * This is a callback for the frame handler. Arguments should fulfill
 * pf_eth_frame_handler_t
 *
 * ToDo: Device-device communication (MC) is not yet implemented.
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:    The frame id.
 * @param p_buf            InOut: The Ethernet frame. Will be freed.
 * @param frame_id_pos     In:    Position of the frame id in the buffer.
 *                                Depends on whether VLAN tagging is used.
 * @param p_arg            In:    Not used.
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
static int pf_dcp_hello_ind (
   pnet_t * net,
   uint16_t frame_id,
   pnal_buf_t * p_buf,
   uint16_t frame_id_pos,
   void * p_arg) /* Not used */
{
   pf_get_info_t src_info =
   {
      .is_big_endian = true,
      .p_frame = p_buf,
      .p_array = NULL,
      .len = pnal_buf_get_size (p_buf),
      .result = PF_PARSE_OK,
   };
   uint16_t src_pos = 0;
   pf_ethernet_header_t eth_hdr;

   if (p_buf == NULL)
   {
      return 1; /* Means: handled */
   }

   pf_get_ethernet_header (&src_info, &src_pos, &eth_hdr);
   if (src_info.result != PF_PARSE_OK)
   {
      goto out1;
   }

   if (
      memcmp (
         eth_hdr.dest.addr,
         dcp_mc_addr_hello.addr,
         sizeof (dcp_mc_addr_hello.addr)) != 0)
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
       * NameOfStationBlockRes = NameOfStationType, DCPBlockLength, BlockInfo,
       * NameOfStationValue NameOfStationType = DevicePropertiesOption (=0x02),
       * SuboptionNameOfStation (=0x02)
       */

      /*
       * IPParameterBlockRes = IPParameterType, DCPBlockLength, BlockInfo,
       * IPParameterValue IPParameterType = IPOption (=0x01)
       */

      /*
       * DeviceIDBlockRes = DeviceIDType, DCPBlockLength, BlockInfo,
       * DeviceIDValue
       */

      /*
       * DeviceOptionsBlockRes = DeviceOptionsType, DCPBlockLength, BlockInfo,
       * DeviceOptionsValue
       */

      /*
       * DeviceVendorBlockRes = DeviceVendorType, DCPBlockLength, BlockInfo,
       * DeviceVendorValue
       */

      /*
       * DeviceRoleBlockRes = DeviceRoleType, DCPBlockLength, BlockInfo,
       * DeviceRoleValue
       */

      /*
       * DeviceInitiativeBlockRes = DeviceInitiativeType, DCPBlockLength,
       * BlockInfo, DeviceInitiativeValue
       */
   }

out1:
   pnal_buf_free (p_buf);
   return 1; /* Means: handled */
}

int pf_dcp_hello_req (pnet_t * net)
{
   pnal_buf_t * p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   uint16_t dst_pos;
   uint16_t dst_start_pos;
   uint16_t value_length;
   uint8_t * p_value;
   uint8_t block_error;
   uint16_t temp16;
   pf_ip_suite_t ip_suite;

   if (p_buf == NULL)
   {
      return -1;
   }

   /* Prepare the response */
   pf_put_info_t dst =
   {
      .p_frame = p_buf,
      .p_array = NULL,
      .len = pnal_buf_get_size (p_buf),
      .result = PF_PUT_OK,
   };

   /* Go to first block */
   dst_pos = 0;
   dst_pos += sizeof (pf_ethernet_header_t);
   dst_pos += sizeof (uint16_t); /* FrameId */
   dst_pos += sizeof (pf_dcp_header_t);

   /* Save position for later */
   dst_start_pos = dst_pos;

   if (
      pf_cmina_dcp_get_req (
         net,
         PF_DCP_OPT_DEVICE_PROPERTIES,
         PF_DCP_SUB_DEV_PROP_NAME,
         &value_length,
         &p_value,
         &block_error) == 0)
   {
      LOG_DEBUG (
         PF_DCP_LOG,
         "DCP(%d): Sending DCP Hello request. Station name: %s\n",
         __LINE__,
         (char *)p_value);
      (void)pf_dcp_put_block (
         &dst,
         &dst_pos,
         PF_DCP_OPT_DEVICE_PROPERTIES,
         PF_DCP_SUB_DEV_PROP_NAME,
         true,
         0,
         (uint16_t)strlen ((char *)p_value),
         p_value);
   }

   if (
      pf_cmina_dcp_get_req (
         net,
         PF_DCP_OPT_IP,
         PF_DCP_SUB_IP_PAR,
         &value_length,
         &p_value,
         &block_error) == 0)
   {
      memcpy (&ip_suite, p_value, sizeof (ip_suite));
      ip_suite.ip_addr = CC_TO_BE32 (ip_suite.ip_addr);
      ip_suite.ip_mask = CC_TO_BE32 (ip_suite.ip_mask);
      ip_suite.ip_gateway = CC_TO_BE32 (ip_suite.ip_gateway);
      (void)pf_dcp_put_block (
         &dst,
         &dst_pos,
         PF_DCP_OPT_IP,
         PF_DCP_SUB_IP_PAR,
         true,
         0,
         value_length,
         &ip_suite);
   }

   if (
      pf_cmina_dcp_get_req (
         net,
         PF_DCP_OPT_DEVICE_PROPERTIES,
         PF_DCP_SUB_DEV_PROP_ID,
         &value_length,
         &p_value,
         &block_error) == 0)
   {
      (void)pf_dcp_put_block (
         &dst,
         &dst_pos,
         PF_DCP_OPT_DEVICE_PROPERTIES,
         PF_DCP_SUB_DEV_PROP_ID,
         true,
         0,
         value_length,
         p_value);
   }

   if (
      pf_cmina_dcp_get_req (
         net,
         PF_DCP_OPT_DEVICE_PROPERTIES,
         PF_DCP_SUB_DEV_PROP_ROLE,
         &value_length,
         &p_value,
         &block_error) == 0)
   {
      /* DeviceRoleValue: DeviceRoleDetails, Padding */
      const uint8_t device_role[2] = {*p_value, 0};
      (void)pf_dcp_put_block (
         &dst,
         &dst_pos,
         PF_DCP_OPT_DEVICE_PROPERTIES,
         PF_DCP_SUB_DEV_PROP_ROLE,
         true,
         0,
         sizeof (device_role),
         device_role);
   }

   if (
      pf_cmina_dcp_get_req (
         net,
         PF_DCP_OPT_DEVICE_INITIATIVE,
         PF_DCP_SUB_DEV_INITIATIVE_SUPPORT,
         &value_length,
         &p_value,
         &block_error) == 0)
   {
      temp16 = CC_TO_BE16 (*p_value);
      (void)pf_dcp_put_block (
         &dst,
         &dst_pos,
         PF_DCP_OPT_DEVICE_INITIATIVE,
         PF_DCP_SUB_DEV_INITIATIVE_SUPPORT,
         true,
         0,
         value_length,
         &temp16);
   }

   uint16_t dcp_size = dst_pos - dst_start_pos;

   pnal_buf_set_size (p_buf, dst_pos);
   dst_pos = 0;

   const pf_ethernet_header_t ethhdr =
   {
      .dest = dcp_mc_addr_hello,
      .src = *pf_cmina_get_device_macaddr (net),
      .type = PNAL_ETHTYPE_PROFINET,
   };
   pf_put_ethernet_header (&ethhdr, &dst, &dst_pos);

   /* Insert FrameId */
   pf_put_uint16 (
      true,
      PF_DCP_HELLO_FRAME_ID,
      &dst,
      &dst_pos);

   const pf_dcp_header_t dcphdr =
   {
      .service_id = PF_DCP_SERVICE_HELLO,
      .service_type = PF_DCP_SERVICE_TYPE_REQUEST,
      .xid = 1,
      .response_delay_factor = 0,
      .data_length = dcp_size,
   };
   pf_dcp_put_header (&dcphdr, &dst, &dst_pos);

   if (dst.result == PF_PUT_OK)
   {
      (void)pf_eth_send_on_management_port (net, p_buf);
   }
   else
   {
      LOG_ERROR (
         PF_DCP_LOG,
         "DCP(%d): Could not build DCP Hello request\n",
         __LINE__);
   }

   pnal_buf_free (p_buf);

   return 0;
}

/**
 * @internal
 * Calculate the delay used when sending responses to DCP identify request.
 *
 * Given a large number of MAC addresses, the output is approximately random.
 *
 * The max delay will be (response_delay_factor - 1) x 10 ms.
 *
 * The resulting delay is in the range 0 to 64 s (in steps of 10 ms).
 *
 * See Profinet 2.4 Services 6.3.11.3.4
 *     Profinet 2.4 Protocol 4.3.1.3.5 (formula)
 *
 * @param mac_address            In:    MAC address
 * @param response_delay_factor  In:    Response delay factor, from the request.
 *                                      Allowed range 1 to 6400.
 * @return delay in microseconds
 */
uint32_t pf_dcp_calculate_response_delay (
   const pnet_ethaddr_t * mac_address,
   uint16_t response_delay_factor)
{
   uint16_t random_number = 0;
   uint32_t spread = 0; /* Naming from the standard */

   if ((response_delay_factor == 0) || (response_delay_factor > 6400))
   {
      return 0;
   }

   random_number = mac_address->addr[4] * 0x100 + mac_address->addr[5];
   spread = random_number % response_delay_factor;
   return spread * 10 * 1000;
}

typedef struct pf_dcp_parsed_identify_request
{
   int match;
   pf_ethernet_header_t ethhdr;
   pf_dcp_header_t dcphdr;
   const char * p_alias_name;
   char alias[PF_ALIAS_NAME_MAX_SIZE]; /* Terminated string */

   /* For diagnostic logging */
#if LOG_INFO_ENABLED(PF_DCP_LOG)
   bool identify_all;
   char stationname[PNET_STATION_NAME_MAX_SIZE]; /* Terminated string */
#endif
} pf_dcp_parsed_identify_request_t;

/**
 * @internal
 * Parse a DCP Identify request.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_buf            In:    The source frame buffer.
 * @param frame_id_pos     In:    Position of the frame id in the buffer.
 *                                Depends on whether VLAN tagging is used.
 * @param result           Out:   The parsed result.
 * @return  0  if parsing was successful
 *         -1  on error
 */
static int pf_dcp_parse_identify_request (
   pnet_t * net,
   const pnal_buf_t * p_buf,
   uint16_t frame_id_pos,
   pf_dcp_parsed_identify_request_t * result)
{
   bool first = true;   /* First of the blocks */
   bool filter = false; /* Is it IdentifyFilter or IdentifyAll? */
   uint16_t src_pos = 0;
   uint16_t src_dcplen = 0;

   memset (result, 0, sizeof (*result));

   /*
    * IdentifyReqBlock = DeviceRoleBlock ^ DeviceVendorBlock ^ DeviceIDBlock ^
    * DeviceOptionsBlock ^ OEMDeviceIDBlock ^ MACAddressBlock ^ IPParameterBlock
    * ^ DHCPParameterBlock ^ ManufacturerSpecificParameterBlock
    */

   /* Setup access to the request */
   pf_get_info_t src_info =
   {
      .is_big_endian = true,
      .p_frame = p_buf,
      .p_array = NULL,
      .len = pnal_buf_get_size (p_buf),
      .result = PF_PARSE_OK,
   };
   pf_get_ethernet_header (&src_info, &src_pos, &result->ethhdr);
   src_pos = frame_id_pos;
   src_pos += sizeof (uint16_t); /* FrameId */
   pf_dcp_get_header (&src_info, &src_pos, &result->dcphdr);
   src_dcplen = src_pos + result->dcphdr.data_length;

   if (
      src_info.result != PF_PARSE_OK ||
      pf_dcp_filter_destination_address (net, &result->ethhdr.dest) == false ||
      src_dcplen > src_info.len)
   {
      return -1;
   }

   result->match = true; /* So far so good */
   while ((first || (filter && result->match)) &&
          (src_dcplen > src_pos))
   {
      uint16_t value_length;
      uint8_t * p_value;
      uint8_t block_error;
      pf_dcp_block_hdr_t src_block_hdr;
      uint16_t src_block_len;
      uint16_t saved_src_pos;

      /* The block header is expected to be 16-bit aligned */
      pf_dcp_get_block_header (&src_info, &src_pos, &src_block_hdr);
      src_block_len = src_block_hdr.block_length;
      saved_src_pos = src_pos;

      if (src_info.result != PF_PARSE_OK || src_pos + src_block_len > src_dcplen)
      {
         return -1;
      }

      /*
       * At the start of the loop src_pos shall point to the block value. Also,
       * src_block_len shall be set to block_length of the current buffer.
       */
      /*
       * DCP-Identify-ReqPDU = DCP-IdentifyFilter-ReqPDU ^
       * DCP-IdentifyAll-ReqPDU
       *
       * DCP-IdentifyAll-ReqPDU = DCP-MC-Header, AllSelectorOption (=0xff),
       * SuboptionAllSelector (=0xff), DCPBlockLength (=0x02)
       * DCP-IdentifyFilter-ReqPDU = DCP-MC-Header, [NameOfStationBlock] ^
       * [AliasNameBlock], [IdentifyReqBlock] * a
       *
       * DCP-MC-Header = ServiceID, ServiceType, Xid, ResponseDelayFactor,
       * DCPDataLength
       *
       * IdentifyReqBlock = DeviceRoleBlock ^ DeviceVendorBlock ^
       * DeviceIDBlock ^ DeviceOptionsBlock ^ OEMDeviceIDBlock ^
       * MACAddressBlock ^ IPParameterBlock ^ DHCPParameterBlock ^
       * ManufacturerSpecificParameterBlock
       */
      if (
         pf_cmina_dcp_get_req (
            net,
            src_block_hdr.option,
            src_block_hdr.sub_option,
            &value_length,
            &p_value,
            &block_error) == 0)
      {
         switch (src_block_hdr.option)
         {
         case PF_DCP_OPT_ALL:
            switch (src_block_hdr.sub_option)
            {
            case PF_DCP_SUB_ALL:
#if LOG_INFO_ENABLED(PF_DCP_LOG)
               result->identify_all = true;
#endif
               /* ToDo: Is there a bug in the PNIO tester? It sends
                * src_block_len == 0! */
               if (
                  (first == true) &&
                  ((src_block_len == 0x02) || (src_block_len == 0x00)))
               {
                  filter = false;
               }
               else
               {
                  return -1;
               }
               break;
            default:
               return -1;
            }
            break;

         case PF_DCP_OPT_IP:
            if (filter == false)
            {
               return -1;
            }
            /*
             * MACAddressBlock = 0x01, 0x01, DCPBlockLength,
             * MACAddressValue IPParameterBlock = 0x01, 0x02,
             * DCPBlockLength, IPAddress, Subnetmask, StandardGateway
             * Note: No query for FullSuite
             */
            switch (src_block_hdr.sub_option)
            {
            case PF_DCP_SUB_IP_MAC:
               /* Fall-through */
            case PF_DCP_SUB_IP_PAR:
               if (
                  (pf_get_memcmp (&src_info, &src_pos, value_length, p_value) != 0) ||
                  (src_block_len != value_length))
               {
                  result->match = false;
               }
               break;
            default:
               return -1;
            }
            break;

         case PF_DCP_OPT_DEVICE_PROPERTIES:
            /*
             * DeviceVendorBlock = 0x02, 0x01, DCPBlockLength,
             * DeviceVendorValue
             *
             * NameOfStationBlock = 0x02, 0x02, DCPBlockLength,
             * NameOfStationValue
             *
             * DeviceIDBlock = 0x02, 0x03, DCPBlockLength, VendorIDHigh,
             * VendorIDLow, DeviceIDHigh, DeviceIDLow
             *
             * DeviceRoleBlock = 0x02, 0x04, DCPBlockLength, DeviceRoleValue
             *
             * DeviceOptionsBlock = 0x02, 0x05, DCPBlockLength, (Option,
             * Suboption)
             *
             * AliasNameBlock = 0x02, 0x06, DCPBlockLength, AliasNameValue
             *
             * OEMDeviceIDBlock = 0x02, 0x08, DCPBlockLength, VendorIDHigh,
             * VendorIDLow, DeviceIDHigh, DeviceIDLow
             */
            switch (src_block_hdr.sub_option)
            {
            case PF_DCP_SUB_DEV_PROP_NAME:
#if LOG_INFO_ENABLED(PF_DCP_LOG)
               pf_get_mem (
                  &src_info,
                  &src_pos,
                  MIN (src_block_len, sizeof (result->stationname) - 1),
                  result->stationname);
               src_pos = saved_src_pos;
#endif
              if (
                  (pf_get_memcmp (&src_info, &src_pos, src_block_len, p_value) == 0) &&
                  (p_value[src_block_len] == '\0'))
               {
                  if (first == true)
                  {
                     filter = true;
                  }
               }
               else
               {
                  result->match = false;
               }
               break;
            case PF_DCP_SUB_DEV_PROP_VENDOR:
               /* Fall-through */
            case PF_DCP_SUB_DEV_PROP_ID:
               /* Fall-through */
            case PF_DCP_SUB_DEV_PROP_ROLE:
               /* Fall-through */
            case PF_DCP_SUB_DEV_PROP_OPTIONS:
               /* Fall-through */
            case PF_DCP_SUB_DEV_PROP_OEM_ID:
               /* Fall-through */
            case PF_DCP_SUB_DEV_PROP_INSTANCE:
               /* Fall-through */
            case PF_DCP_SUB_DEV_PROP_GATEWAY:
               if (filter == false)
               {
                  return -1;
               }
               if (
                  (pf_get_memcmp (&src_info, &src_pos, value_length, p_value) != 0) ||
                  (src_block_len != value_length))
               {
                  result->match = false;
               }
               break;
            default:
               return -1;
            }
            break;

         case PF_DCP_OPT_DHCP:
            if (filter == false)
            {
               return -1;
            }
            if (
               (pf_get_memcmp (&src_info, &src_pos, value_length, p_value) != 0) ||
               (src_block_len != value_length))
            {
               result->match = false;
            }
            break;
         default:
            return -1;
         }
      }
      else if (
         (src_block_hdr.option == PF_DCP_OPT_DEVICE_PROPERTIES) &&
         (src_block_hdr.sub_option == PF_DCP_SUB_DEV_PROP_ALIAS))
      {
         if (src_block_len < PF_ALIAS_NAME_MAX_SIZE)
         {
            pf_get_mem (&src_info, &src_pos, src_block_len, result->alias);
            result->alias[src_block_len] = '\0';
            if (pf_lldp_is_alias_matching (net, result->alias))
            {
               if (first == true)
               {
                  result->p_alias_name = result->alias;
                  filter = true;
               }
            }
            else
            {
               result->match = false;
            }
         }
         else
         {
            result->match = false;
         }
      }
      else
      {
         LOG_DEBUG (
            PF_DCP_LOG,
            "DCP(%d): Unknown incoming DCP identify request.\n",
            __LINE__);
         return -1; /* Give up on bad request */
      }

      src_pos = saved_src_pos + src_block_len;

      /* Skip padding to align on uint16_t */
      while (src_pos & 1)
      {
         src_pos++;
      }

      if (src_info.result != PF_PARSE_OK)
      {
         return -1;
      }

      first = false;
   }

   return 0; /* All OK */
}

static int pf_dcp_construct_identify_response (
   pnet_t * net,
   pnal_buf_t * p_rsp,
   const pf_ethernet_header_t * p_src_ethhdr,
   const pf_dcp_header_t * p_src_dcphdr,
   const char * p_req_alias_name)
{
   uint16_t dst_pos;
   uint16_t dst_start;

   /* Prepare the response */
   pf_put_info_t dst =
   {
      .p_frame = p_rsp,
      .p_array = NULL,
      .len = pnal_buf_get_size (p_rsp),
      .result = PF_PUT_OK,
   };

   /* Go to first block */
   dst_pos = 0;
   dst_pos += sizeof (pf_ethernet_header_t);
   dst_pos += sizeof (uint16_t); /* FrameId */
   dst_pos += sizeof (pf_dcp_header_t);

   /* Save position for later */
   dst_start = dst_pos;

   /* Build the response */
   for (uint16_t ix = 0; ix < NELEMENTS (device_options); ix++)
   {
      pf_dcp_get_req (
         net,
         &dst,
         &dst_pos,
         device_options[ix].opt,
         device_options[ix].sub,
         true,
         p_req_alias_name);
   }

   uint16_t dcp_size = dst_pos - dst_start;
   pnal_buf_set_size (p_rsp, dst_pos);

   dst_pos = 0;

   const pf_ethernet_header_t ethhdr =
   {
      .dest = p_src_ethhdr->src,
      .src = *pf_cmina_get_device_macaddr (net),
      .type = PNAL_ETHTYPE_PROFINET,
   };
   pf_put_ethernet_header (&ethhdr, &dst, &dst_pos);

   /* frame ID */
   pf_put_uint16 (
      true,
      PF_DCP_ID_RES_FRAME_ID,
      &dst,
      &dst_pos);

   const pf_dcp_header_t dcphdr =
   {
      .service_id = p_src_dcphdr->service_id,
      .service_type = PF_DCP_SERVICE_TYPE_SUCCESS,
      .xid = p_src_dcphdr->xid,
      .response_delay_factor = 0,
      .data_length = dcp_size,
   };
   pf_dcp_put_header (&dcphdr, &dst, &dst_pos);

   return dst.result == PF_PUT_OK ? 0 : -1;
}

/**
 * @internal
 * Handle an incoming DCP identify request.
 *
 * The request may contain filter conditions. Only respond if ALL conditions
 * match.
 *
 * The response is sent with a delay, as many devices might respond to the
 * request.
 *
 * This is a callback for the frame handler. Arguments should fulfill
 * pf_eth_frame_handler_t
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:    The frame id.
 * @param p_buf            In:    The Ethernet frame.
 * @param frame_id_pos     In:    Position of the frame id in the buffer.
 *                                Depends on whether VLAN tagging is used.
 * @param p_arg            In:    Not used.
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
static int pf_dcp_identify_req (
   pnet_t * net,
   uint16_t frame_id,
   pnal_buf_t * p_buf,
   uint16_t frame_id_pos,
   void * p_arg) /* Not used */
{
   pf_dcp_parsed_identify_request_t req;
   int ret = 0; /* Assume all OK */

   /*
    * IdentifyReqBlock = DeviceRoleBlock ^ DeviceVendorBlock ^ DeviceIDBlock ^
    * DeviceOptionsBlock ^ OEMDeviceIDBlock ^ MACAddressBlock ^ IPParameterBlock
    * ^ DHCPParameterBlock ^ ManufacturerSpecificParameterBlock
    */

   if (p_buf == NULL)
   {
      return 1; /* Means: handled */
   }

   /* Only one pending response is supported */
   if (net->dcp_delayed_response_waiting == true)
   {
      LOG_DEBUG (
         PF_DCP_LOG,
         "DCP(%d): DCP request ignored (response pending in scheduler)\n",
         __LINE__);

      goto out1;
   }
   ret = pf_dcp_parse_identify_request (net, p_buf, frame_id_pos, &req);

   if ((ret == 0) && (req.match == true))
   {
      uint32_t response_delay;
      pnal_buf_t * p_rsp = NULL;
      const pnet_ethaddr_t * mac_address = pf_cmina_get_device_macaddr (net);

      p_rsp = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE); /* Get a transmit buffer
                                                      for the response */
      if (p_rsp == NULL)
      {
         LOG_ERROR (
            PF_DCP_LOG,
            "DCP(%d): Could not allocate memory for response to incoming DCP "
            "identify request.\n",
            __LINE__);

         goto out1;
      }

      ret = pf_dcp_construct_identify_response (
         net,
         p_rsp,
         &req.ethhdr,
         &req.dcphdr,
         req.p_alias_name);
      if (ret != 0)
      {
         LOG_ERROR (
            PF_DCP_LOG,
            "DCP(%d): Could not build DCP Identify response\n",
            __LINE__);
         pnal_buf_free (p_rsp);
         goto out1;
      }

      response_delay = pf_dcp_calculate_response_delay (
         mac_address,
         req.dcphdr.response_delay_factor);

      if (
         pf_scheduler_add (
            net,
            response_delay,
            pf_dcp_responder,
            p_rsp,
            &net->dcp_identresp_timeout) == 0)
      {
         /* Note: Do not free p_rsp, it is used in pf_dcp_responder() */

         net->dcp_delayed_response_waiting = true;

#if LOG_INFO_ENABLED(PF_DCP_LOG)
      LOG_INFO (
         PF_DCP_LOG,
         "DCP(%d): Responding to incoming DCP identify request. All: %d "
         "StationName: %s Alias: %s  Delay %" PRIu32 " us. Xid: %" PRIu32
         "\n",
         __LINE__,
         req.identify_all,
         req.stationname, /* Terminated */
         req.alias,       /* Terminated */
         response_delay,
         req.dcphdr.xid);
#endif
      }
      else
      {
         LOG_ERROR (
            PF_DCP_LOG,
            "DCP(%d): Failed to schedule response to DCP identity request\n",
            __LINE__);

         pnal_buf_free (p_rsp);
      }
   }
   else
   {
#if LOG_INFO_ENABLED(PF_DCP_LOG)
      LOG_INFO (
         PF_DCP_LOG,
         "DCP(%d): No match for incoming DCP identify request. All: %d "
         "StationName: %s Alias: %s Xid: %" PRIu32 "\n",
         __LINE__,
         req.identify_all,
         req.stationname, /* Terminated */
         req.alias,       /* Terminated */
         req.dcphdr.xid);
#endif
   }

out1:
   if (p_buf != NULL)
   {
      pnal_buf_free (p_buf);
   }

   return 1; /* Means: handled */
}

void pf_dcp_exit (pnet_t * net)
{
   pf_eth_frame_id_map_remove (net, PF_DCP_HELLO_FRAME_ID);
   pf_eth_frame_id_map_remove (net, PF_DCP_GET_SET_FRAME_ID);
   pf_eth_frame_id_map_remove (net, PF_DCP_ID_REQ_FRAME_ID);
}

void pf_dcp_init (pnet_t * net)
{
   net->dcp_global_block_qualifier = 0;
   net->dcp_delayed_response_waiting = false;
   net->dcp_sam = mac_nil;
   pf_scheduler_init_handle (&net->dcp_sam_timeout, "dcp_sam");
   pf_scheduler_init_handle (&net->dcp_led_timeout, "dcp_led");
   pf_scheduler_init_handle (&net->dcp_identresp_timeout, "dcp_identresp");

   LOG_DEBUG (
      PF_DCP_LOG,
      "DCP(%d): Activate. Register frame handlers.\n",
      __LINE__);

   /* Insert handlers for our specific frame_ids */
   pf_eth_frame_id_map_add (net, PF_DCP_HELLO_FRAME_ID, pf_dcp_hello_ind, NULL);
   pf_eth_frame_id_map_add (net, PF_DCP_GET_SET_FRAME_ID, pf_dcp_get_set, NULL);
   pf_eth_frame_id_map_add (
      net,
      PF_DCP_ID_REQ_FRAME_ID,
      pf_dcp_identify_req,
      NULL);
}
