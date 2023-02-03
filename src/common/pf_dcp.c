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

#define PF_DCP_SIGNAL_LED_HALF_INTERVAL     500000 /* 0.5 seconds */
#define PF_DCP_SIGNAL_LED_NUMBER_OF_FLASHES 3

/* Client hold time, 3 seconds.
   See Profinet 2.4 Services, section 6.3.11.2.2 */
#define PF_DCP_SAM_TIMEOUT 3000000 /* microseconds */

CC_PACKED_BEGIN
typedef struct CC_PACKED pf_ethhdr
{
   pnet_ethaddr_t dest;
   pnet_ethaddr_t src;
   uint16_t type; /* Note: network endianness */
} pf_ethhdr_t;
CC_PACKED_END

CC_PACKED_BEGIN
typedef struct CC_PACKED pf_dcp_header
{
   uint8_t service_id;
   uint8_t service_type;
   uint32_t xid;                   /* Note: network endianness */
   uint16_t response_delay_factor; /* Note: network endianness */
   uint16_t data_length;           /* Note: network endianness */
} pf_dcp_header_t;
CC_PACKED_END
CC_STATIC_ASSERT (PF_DCP_HEADER_SIZE == sizeof (pf_dcp_header_t));

CC_PACKED_BEGIN
typedef struct CC_PACKED pf_dcp_block_hdr
{
   uint8_t option;
   uint8_t sub_option;
   uint16_t block_length; /* Note: network endianness */
} pf_dcp_block_hdr_t;
CC_PACKED_END
CC_STATIC_ASSERT (PF_DCP_BLOCK_HDR_SIZE == sizeof (pf_dcp_block_hdr_t));

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
 * @return true if destination address is ok and shall packet shall
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
 * Add a block to a buffer, possibly including a block_info.
 *
 * @param p_dst            Out:   The destination buffer.
 * @param p_dst_pos        InOut: Position in the destination buffer.
 * @param dst_max          In:    Size of destination buffer.
 * @param opt              In:    Option key.
 * @param sub              In:    Sub-option key.
 * @param with_block_info  In:    If true then add block_info argument.
 * @param block_info       In:    The block info argument.
 * @param value_length     In:    The length in bytes of the p_value data.
 * @param p_value          In:    The source data.
 * @return
 */
static int pf_dcp_put_block (
   uint8_t * p_dst,
   uint16_t * p_dst_pos,
   uint16_t dst_max,
   uint8_t opt,
   uint8_t sub,
   bool with_block_info,
   uint16_t block_info,
   uint16_t value_length, /* Not including length of block_info */
   const void * p_value)
{
   uint16_t b_len;

   if ((*p_dst_pos + value_length) >= dst_max)
   {
      return 0; /* Skip excess data silently */
   }

   /* Adjust written block length if block_info is included */
   b_len = value_length;
   if (with_block_info)
   {
      b_len += sizeof (b_len);
   }

   pf_put_byte (opt, dst_max, p_dst, p_dst_pos);
   pf_put_byte (sub, dst_max, p_dst, p_dst_pos);
   pf_put_uint16 (true, b_len, dst_max, p_dst, p_dst_pos);
   if (with_block_info == true)
   {
      pf_put_uint16 (true, block_info, dst_max, p_dst, p_dst_pos);
   }
   if ((p_value != NULL) && (value_length > 0))
   {
      pf_put_mem (p_value, value_length, dst_max, p_dst, p_dst_pos);
   }

   /* Add padding to align on uint16_t */
   while ((*p_dst_pos) & 1)
   {
      pf_put_byte (0, dst_max, p_dst, p_dst_pos);
   }

   return 0;
}

/**
 * @internal
 * Handle one block in a DCP get request, by inserting it into the response.
 *
 * @param net                 InOut: The p-net stack instance
 * @param p_dst               Out:   The destination buffer.
 * @param p_dst_pos           InOut: Position in the destination buffer.
 * @param dst_max             In:    Size of destination buffer.
 * @param opt                 In:    Option key.
 * @param sub                 In:    Sub-option key.
 * @param request_is_identify In:    Usage in response to Identify request
 *                                   (skips some blocks)
 * @param alias_name          In:    Alias name appended if != NULL
 * @return
 */
static int pf_dcp_get_req (
   pnet_t * net,
   uint8_t * p_dst,
   uint16_t * p_dst_pos,
   uint16_t dst_max,
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
            htonl (full_ip_suite.ip_suite.ip_addr);
         full_ip_suite.ip_suite.ip_mask =
            htonl (full_ip_suite.ip_suite.ip_mask);
         full_ip_suite.ip_suite.ip_gateway =
            htonl (full_ip_suite.ip_suite.ip_gateway);
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
            htonl (full_ip_suite.ip_suite.ip_addr);
         full_ip_suite.ip_suite.ip_mask =
            htonl (full_ip_suite.ip_suite.ip_mask);
         full_ip_suite.ip_suite.ip_gateway =
            htonl (full_ip_suite.ip_suite.ip_gateway);
         for (ix = 0; ix < NELEMENTS (full_ip_suite.dns_addr); ix++)
         {
            full_ip_suite.dns_addr[ix] = htonl (full_ip_suite.dns_addr[ix]);
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
            temp16 = htons (temp16);
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
         value_length += 1;
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
         temp16 = htons (temp16);
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
            dst_max,
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
            dst_max,
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
 * Handle one block in a DCP set request, by inserting it into the response.
 *
 * Triggers the application callback \a pnet_reset_ind() for some values.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_dst            Out:   The destination buffer.
 * @param p_dst_pos        InOut: Position in the destination buffer.
 * @param dst_max          In:    Size of destination buffer.
 * @param opt              In:    Option key.
 * @param sub              In:    Sub-option key.
 * @param block_info       In:    The block info argument.
 * @param value_length     In:    The length in bytes of the p_value data.
 * @param p_value          In:    The source data.
 * @return
 */
static int pf_dcp_set_req (
   pnet_t * net,
   uint8_t * p_dst,
   uint16_t * p_dst_pos,
   uint16_t dst_max,
   uint8_t opt,
   uint8_t sub,
   uint16_t block_qualifier,
   uint16_t value_length,
   const uint8_t * p_value)
{
   int ret = -1;
   uint8_t response_data[3];
   pf_full_ip_suite_t full_ip_suite;
   uint16_t ix;

   response_data[0] = opt;
   response_data[1] = sub;
   /* Assume negative response initially */
   response_data[2] = PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED;

   switch (opt)
   {
   case PF_DCP_OPT_IP:
      switch (sub)
      {
      case PF_DCP_SUB_IP_MAC:
         /* Can't set MAC address */
         break;
      case PF_DCP_SUB_IP_PAR:
         if (value_length == sizeof (full_ip_suite.ip_suite))
         {
            memcpy (
               &full_ip_suite.ip_suite,
               p_value,
               sizeof (full_ip_suite.ip_suite));
            full_ip_suite.ip_suite.ip_addr =
               ntohl (full_ip_suite.ip_suite.ip_addr);
            full_ip_suite.ip_suite.ip_mask =
               ntohl (full_ip_suite.ip_suite.ip_mask);
            full_ip_suite.ip_suite.ip_gateway =
               ntohl (full_ip_suite.ip_suite.ip_gateway);

            ret = pf_cmina_dcp_set_ind (
               net,
               opt,
               sub,
               block_qualifier,
               value_length,
               (uint8_t *)&full_ip_suite.ip_suite,
               &response_data[2]);
         }
         break;
      case PF_DCP_SUB_IP_SUITE:
         if (value_length == sizeof (full_ip_suite.ip_suite))
         {
            memcpy (&full_ip_suite, p_value, sizeof (full_ip_suite));
            full_ip_suite.ip_suite.ip_addr =
               ntohl (full_ip_suite.ip_suite.ip_addr);
            full_ip_suite.ip_suite.ip_mask =
               ntohl (full_ip_suite.ip_suite.ip_mask);
            full_ip_suite.ip_suite.ip_gateway =
               ntohl (full_ip_suite.ip_suite.ip_gateway);
            for (ix = 0; ix < NELEMENTS (full_ip_suite.dns_addr); ix++)
            {
               full_ip_suite.dns_addr[ix] = ntohl (full_ip_suite.dns_addr[ix]);
            }

            ret = pf_cmina_dcp_set_ind (
               net,
               opt,
               sub,
               block_qualifier,
               value_length,
               (uint8_t *)&full_ip_suite.ip_suite,
               &response_data[2]);
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
         ret = pf_cmina_dcp_set_ind (
            net,
            opt,
            sub,
            block_qualifier,
            value_length,
            p_value,
            &response_data[2]);
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
         response_data[2] = PF_DCP_BLOCK_ERROR_NO_ERROR;
         ret = 0;
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
         ret = pf_dcp_trigger_signal_led (net);
         break;
      case PF_DCP_SUB_CONTROL_RESPONSE:
         /* Can't set this */
         break;
      case PF_DCP_SUB_CONTROL_FACTORY_RESET:
      case PF_DCP_SUB_CONTROL_RESET_TO_FACTORY:
         if (
            pf_cmina_dcp_set_ind (
               net,
               opt,
               sub,
               block_qualifier,
               value_length,
               p_value,
               &response_data[2]) != 0)
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
      response_data[2] = PF_DCP_BLOCK_ERROR_OPTION_NOT_SUPPORTED;
      break;
   }

   if (ret == 0)
   {
      response_data[2] = PF_DCP_BLOCK_ERROR_NO_ERROR;
   }

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
      dst_max,
      PF_DCP_OPT_CONTROL,
      PF_DCP_SUB_CONTROL_RESPONSE,
      false,
      0,
      sizeof (response_data),
      response_data);

   return ret;
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
   uint8_t * p_src;
   uint16_t src_pos;
   uint16_t src_dcplen;
   uint16_t src_block_len;
   uint16_t src_block_qualifier;
   pf_ethhdr_t * p_src_ethhdr;
   pf_dcp_header_t * p_src_dcphdr;
   pf_dcp_block_hdr_t * p_src_block_hdr;
   pnal_buf_t * p_rsp = NULL;
   uint8_t * p_dst;
   uint16_t dst_pos;
   uint16_t dst_start;
   pf_ethhdr_t * p_dst_ethhdr;
   pf_dcp_header_t * p_dst_dcphdr;
   const pnet_ethaddr_t * mac_address = pf_cmina_get_device_macaddr (net);

   if (p_buf == NULL)
   {
      return 1; /* Buffer handled */
   }

   /* Extract info from the request */
   p_src = (uint8_t *)p_buf->payload;
   src_pos = 0;
   p_src_ethhdr = (pf_ethhdr_t *)&p_src[src_pos];

   src_pos = frame_id_pos;
   src_pos += sizeof (uint16_t); /* FrameId */
   p_src_dcphdr = (pf_dcp_header_t *)&p_src[src_pos];

   src_pos += sizeof (pf_dcp_header_t);
   src_dcplen = (src_pos + ntohs (p_src_dcphdr->data_length));

   if (src_dcplen > p_buf->len)
   {
      goto out;
   }

   p_rsp = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE); /* Get a transmit buffer
                                                   for the response */
   if (p_rsp == NULL)
   {
      goto out;
   }

   if (pf_dcp_check_sam (net, &p_src_ethhdr->src) == false)
   {
      goto out;
   }

   if (pf_dcp_check_destination_address (mac_address, &p_src_ethhdr->dest) == false)
   {
      goto out;
   }

   /* Prepare the response */
   p_dst = (uint8_t *)p_rsp->payload;
   dst_pos = 0;
   p_dst_ethhdr = (pf_ethhdr_t *)&p_dst[dst_pos];
   dst_pos += sizeof (pf_ethhdr_t);

   /* Insert frame ID into response */
   p_dst[dst_pos++] = ((PF_DCP_GET_SET_FRAME_ID & 0xff00) >> 8);
   p_dst[dst_pos++] = PF_DCP_GET_SET_FRAME_ID & 0xff;

   p_dst_dcphdr = (pf_dcp_header_t *)&p_dst[dst_pos];
   dst_pos += sizeof (pf_dcp_header_t);

   /* Save position for later */
   dst_start = dst_pos;

   /* Set eth header in the response */
   memcpy (
      p_dst_ethhdr->dest.addr,
      p_src_ethhdr->src.addr,
      sizeof (pnet_ethaddr_t));
   memcpy (p_dst_ethhdr->src.addr, mac_address->addr, sizeof (pnet_ethaddr_t));
   p_dst_ethhdr->type = htons (PNAL_ETHTYPE_PROFINET);

   /* Copy DCP header from the request, and modify what is needed. */
   *p_dst_dcphdr = *p_src_dcphdr;
   p_dst_dcphdr->service_type = PF_DCP_SERVICE_TYPE_SUCCESS;
   p_dst_dcphdr->response_delay_factor = htons (0);

   if (p_src_dcphdr->service_id == PF_DCP_SERVICE_SET)
   {
      LOG_DEBUG (
         PF_DCP_LOG,
         "DCP(%d): Incoming DCP Set request. Xid: %" PRIu32 "\n",
         __LINE__,
         ntohl (p_src_dcphdr->xid));
      p_src_block_hdr = (pf_dcp_block_hdr_t *)&p_src[src_pos];
      src_pos += sizeof (*p_src_block_hdr); /* Point to the block value */
      src_block_len = ntohs (p_src_block_hdr->block_length);

      while (src_dcplen >= (src_pos + src_block_len))
      {
         /* Extract block qualifier */
         src_block_qualifier = ntohs (*(uint16_t *)&p_src[src_pos]);
         src_pos += sizeof (uint16_t);
         src_block_len -= sizeof (uint16_t);

         (void)pf_dcp_set_req (
            net,
            p_dst,
            &dst_pos,
            PF_FRAME_BUFFER_SIZE,
            p_src_block_hdr->option,
            p_src_block_hdr->sub_option,
            src_block_qualifier,
            src_block_len,
            &p_src[src_pos]);

         /* Point to next block */
         src_pos += src_block_len;

         /* Skip padding to align on uint16_t */
         while (src_pos & 1)
         {
            src_pos++;
         }
         p_src_block_hdr = (pf_dcp_block_hdr_t *)&p_src[src_pos];
         src_pos += sizeof (*p_src_block_hdr); /* Point to the block value
                                                */
         src_block_len = ntohs (p_src_block_hdr->block_length);
      }

      pf_dcp_restart_sam_timeout (net, &p_src_ethhdr->src);
   }
   else if (p_src_dcphdr->service_id == PF_DCP_SERVICE_GET)
   {
      LOG_DEBUG (
         PF_DCP_LOG,
         "DCP(%d): Incoming DCP Get request. Xid: %" PRIu32 "\n",
         __LINE__,
         ntohl (p_src_dcphdr->xid));
      while (src_dcplen >= (src_pos + sizeof (uint8_t) + sizeof (uint8_t)))
      {
         (void)pf_dcp_get_req (
            net,
            p_dst,
            &dst_pos,
            PF_FRAME_BUFFER_SIZE,
            p_src[src_pos],
            p_src[src_pos + 1],
            false,
            false);

         /* Point to next block */
         src_pos += sizeof (uint8_t) + sizeof (uint8_t);
      }

      pf_dcp_restart_sam_timeout (net, &p_src_ethhdr->src);
   }
   else
   {
      LOG_ERROR (
         PF_DCP_LOG,
         "DCP(%d): Unknown DCP service id %u. Xid: %" PRIu32 "\n",
         __LINE__,
         (unsigned)p_src_dcphdr->service_id,
         ntohl (p_src_dcphdr->xid));
      p_dst_dcphdr->service_type = PF_DCP_SERVICE_TYPE_NOT_SUPPORTED;
   }

   /* Insert final response length and ship it! */
   p_dst_dcphdr->data_length = htons (dst_pos - dst_start);
   p_rsp->len = dst_pos;

   if (pf_eth_send_on_management_port (net, p_rsp) > 0)
   {
      LOG_DEBUG (PF_DCP_LOG, "DCP(%d): Sent DCP Get/Set response\n", __LINE__);
   }

   if (p_src_dcphdr->service_id == PF_DCP_SERVICE_SET)
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
   pf_ethhdr_t * p_eth_hdr = p_buf->payload;

   if (
      memcmp (
         p_eth_hdr->dest.addr,
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
   pnal_buf_free (p_buf);
   return 1; /* Means: handled */
}

int pf_dcp_hello_req (pnet_t * net)
{
   pnal_buf_t * p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   uint8_t * p_dst;
   uint16_t dst_pos;
   uint16_t dst_start_pos;
   pf_ethhdr_t * p_ethhdr;
   pf_dcp_header_t * p_dcphdr;
   uint16_t value_length;
   uint8_t * p_value;
   uint8_t block_error;
   uint16_t temp16;
   pf_ip_suite_t ip_suite;
   const pnet_ethaddr_t * mac_address = pf_cmina_get_device_macaddr (net);

   if (p_buf == NULL)
   {
      return -1;
   }

   p_dst = (uint8_t *)p_buf->payload;
   if (p_dst == NULL)
   {
      pnal_buf_free (p_buf);
      return -1;
   }

   dst_pos = 0;
   p_ethhdr = (pf_ethhdr_t *)&p_dst[dst_pos];
   memcpy (
      p_ethhdr->dest.addr,
      dcp_mc_addr_hello.addr,
      sizeof (p_ethhdr->dest.addr));
   memcpy (p_ethhdr->src.addr, mac_address->addr, sizeof (pnet_ethaddr_t));

   p_ethhdr->type = htons (PNAL_ETHTYPE_PROFINET);
   dst_pos += sizeof (pf_ethhdr_t);

   /* Insert FrameId */
   p_dst[dst_pos++] = (PF_DCP_HELLO_FRAME_ID & 0xff00) >> 8;
   p_dst[dst_pos++] = PF_DCP_HELLO_FRAME_ID & 0x00ff;

   p_dcphdr = (pf_dcp_header_t *)&p_dst[dst_pos];
   p_dcphdr->service_id = PF_DCP_SERVICE_HELLO;
   p_dcphdr->service_type = PF_DCP_SERVICE_TYPE_REQUEST;
   p_dcphdr->xid = htonl (1);
   p_dcphdr->response_delay_factor = htons (0);
   p_dcphdr->data_length = htons (0); /* At the moment */
   dst_pos += sizeof (pf_dcp_header_t);

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
         p_dst,
         &dst_pos,
         PF_FRAME_BUFFER_SIZE,
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
      ip_suite.ip_addr = htonl (ip_suite.ip_addr);
      ip_suite.ip_mask = htonl (ip_suite.ip_mask);
      ip_suite.ip_gateway = htonl (ip_suite.ip_gateway);
      (void)pf_dcp_put_block (
         p_dst,
         &dst_pos,
         PF_FRAME_BUFFER_SIZE,
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
         p_dst,
         &dst_pos,
         PF_FRAME_BUFFER_SIZE,
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
         PF_DCP_SUB_DEV_PROP_OEM_ID,
         &value_length,
         &p_value,
         &block_error) == 0)
   {
      (void)pf_dcp_put_block (
         p_dst,
         &dst_pos,
         PF_FRAME_BUFFER_SIZE,
         PF_DCP_OPT_DEVICE_PROPERTIES,
         PF_DCP_SUB_DEV_PROP_OEM_ID,
         true,
         0,
         value_length,
         p_value);
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
      temp16 = htons (*p_value);
      (void)pf_dcp_put_block (
         p_dst,
         &dst_pos,
         PF_FRAME_BUFFER_SIZE,
         PF_DCP_OPT_DEVICE_INITIATIVE,
         PF_DCP_SUB_DEV_INITIATIVE_SUPPORT,
         true,
         0,
         value_length,
         &temp16);
   }

   /* Insert final response length and ship it! */
   p_dcphdr->data_length = htons (dst_pos - dst_start_pos);
   p_buf->len = dst_pos;

   (void)pf_eth_send_on_management_port (net, p_buf);

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
   int ret = 0; /* Assume all OK */
   uint16_t ix;
   bool first = true;   /* First of the blocks */
   bool match = false;  /* Is it for us? */
   bool filter = false; /* Is it IdentifyFilter or IdentifyAll? */
   uint8_t * p_src;
   uint16_t src_pos = 0;
   pf_ethhdr_t * p_src_ethhdr;
   pf_dcp_header_t * p_src_dcphdr;
   uint16_t src_dcplen = 0;
   pf_dcp_block_hdr_t * p_src_block_hdr;
   uint16_t src_block_len;

   pnal_buf_t * p_rsp = NULL;
   uint8_t * p_dst;
   uint16_t dst_pos = 0;
   uint16_t dst_start = 0;
   pf_ethhdr_t * p_dst_ethhdr;
   pf_dcp_header_t * p_dst_dcphdr;

   uint32_t response_delay;
   uint16_t value_length;
   uint8_t * p_value;
   uint8_t block_error;
   const pnet_ethaddr_t * mac_address = pf_cmina_get_device_macaddr (net);
   char alias[PF_ALIAS_NAME_MAX_SIZE]; /** Terminated */
   char * p_req_alias_name = NULL;

   /* For diagnostic logging */
#if LOG_INFO_ENABLED(PF_DCP_LOG)
   bool identify_all = false;
   uint16_t stationname_position = 0;
   uint16_t stationname_len = 0;
   uint16_t alias_position = 0;
   uint16_t alias_len = 0;
#endif

   /*
    * IdentifyReqBlock = DeviceRoleBlock ^ DeviceVendorBlock ^ DeviceIDBlock ^
    * DeviceOptionsBlock ^ OEMDeviceIDBlock ^ MACAddressBlock ^ IPParameterBlock
    * ^ DHCPParameterBlock ^ ManufacturerSpecificParameterBlock
    */

   if (p_buf == NULL)
   {
      return 1; /* Means: handled */
   }

   /* Setup access to the request */
   p_src = (uint8_t *)p_buf->payload;
   p_src_ethhdr = (pf_ethhdr_t *)p_src;
   src_pos = frame_id_pos;
   src_pos += sizeof (uint16_t); /* FrameId */
   p_src_dcphdr = (pf_dcp_header_t *)&p_src[src_pos];
   src_pos += sizeof (pf_dcp_header_t);
   src_dcplen = (src_pos + ntohs (p_src_dcphdr->data_length));

   if (src_dcplen > p_buf->len)
   {
      goto out1;
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

   /* Prepare the response */
   p_dst = (uint8_t *)p_rsp->payload;
   dst_pos = 0;
   p_dst_ethhdr = (pf_ethhdr_t *)p_dst;
   dst_pos += sizeof (pf_ethhdr_t);
   /* frame ID */
   p_dst[dst_pos++] = ((PF_DCP_ID_RES_FRAME_ID & 0xff00) >> 8);
   p_dst[dst_pos++] = PF_DCP_ID_RES_FRAME_ID & 0xff;

   p_dst_dcphdr = (pf_dcp_header_t *)&p_dst[dst_pos];
   dst_pos += sizeof (pf_dcp_header_t);

   /* Save position for later */
   dst_start = dst_pos;

   memcpy (
      p_dst_ethhdr->dest.addr,
      p_src_ethhdr->src.addr,
      sizeof (pnet_ethaddr_t));
   memcpy (p_dst_ethhdr->src.addr, mac_address->addr, sizeof (pnet_ethaddr_t));
   p_dst_ethhdr->type = htons (PNAL_ETHTYPE_PROFINET);

   /* Start with the request header and modify what is needed. */
   *p_dst_dcphdr = *p_src_dcphdr;
   p_dst_dcphdr->service_type = PF_DCP_SERVICE_TYPE_SUCCESS;
   p_dst_dcphdr->response_delay_factor = htons (0);

   /* The block header is expected to be 16-bit aligned */
   p_src_block_hdr = (pf_dcp_block_hdr_t *)&p_src[src_pos];
   src_pos += sizeof (*p_src_block_hdr); /* Point to the block value */

   src_block_len = ntohs (p_src_block_hdr->block_length);

   match = true; /* So far so good */
   while ((ret == 0) && (first || (filter && match)) &&
          (src_dcplen >= (src_pos + src_block_len)) &&
          (dst_pos < PF_FRAME_BUFFER_SIZE))
   {
      /*
       * At the start of the loop p_src_block_hdr shall point to the source
       * block header and src_pos shall point to the block value. Also,
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
            p_src_block_hdr->option,
            p_src_block_hdr->sub_option,
            &value_length,
            &p_value,
            &block_error) == 0)
      {
         switch (p_src_block_hdr->option)
         {
         case PF_DCP_OPT_ALL:
            switch (p_src_block_hdr->sub_option)
            {
            case PF_DCP_SUB_ALL:
#if LOG_INFO_ENABLED(PF_DCP_LOG)
               identify_all = true;
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
                * MACAddressBlock = 0x01, 0x01, DCPBlockLength,
                * MACAddressValue IPParameterBlock = 0x01, 0x02,
                * DCPBlockLength, IPAddress, Subnetmask, StandardGateway
                * Note: No query for FullSuite
                */
               switch (p_src_block_hdr->sub_option)
               {
               case PF_DCP_SUB_IP_MAC:
                  if (
                     (memcmp (p_value, &p_src[src_pos], value_length) != 0) ||
                     (src_block_len != value_length))
                  {
                     match = false;
                  }
                  break;
               case PF_DCP_SUB_IP_PAR:
                  if (
                     (memcmp (p_value, &p_src[src_pos], value_length) != 0) ||
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
            switch (p_src_block_hdr->sub_option)
            {
            case PF_DCP_SUB_DEV_PROP_VENDOR:
               if (filter == true)
               {
                  if (
                     (memcmp (p_value, &p_src[src_pos], value_length) != 0) ||
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
#if LOG_INFO_ENABLED(PF_DCP_LOG)
               stationname_position = src_pos;
               stationname_len = src_block_len;
#endif
               if (
                  (memcmp (p_value, &p_src[src_pos], src_block_len) == 0) &&
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
                  if (
                     (memcmp (p_value, &p_src[src_pos], value_length) != 0) ||
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
                  if (
                     (memcmp (p_value, &p_src[src_pos], value_length) != 0) ||
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
                  if (
                     (memcmp (p_value, &p_src[src_pos], value_length) != 0) ||
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
                  if (
                     (memcmp (p_value, &p_src[src_pos], value_length) != 0) ||
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
            case PF_DCP_SUB_DEV_PROP_INSTANCE:
               if (filter == true)
               {
                  if (
                     (memcmp (p_value, &p_src[src_pos], value_length) != 0) ||
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
                  if (
                     (memcmp (p_value, &p_src[src_pos], value_length) != 0) ||
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
               if (
                  (memcmp (p_value, &p_src[src_pos], value_length) != 0) ||
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
         src_block_len = ntohs (p_src_block_hdr->block_length);
         src_pos += sizeof (*p_src_block_hdr);
      }
      else if (
         (p_src_block_hdr->option == PF_DCP_OPT_DEVICE_PROPERTIES) &&
         (p_src_block_hdr->sub_option == PF_DCP_SUB_DEV_PROP_ALIAS))
      {
#if LOG_INFO_ENABLED(PF_DCP_LOG)
         alias_position = src_pos;
         alias_len = src_block_len;
#endif
         if (src_block_len < PF_ALIAS_NAME_MAX_SIZE)
         {
            memcpy (alias, &p_src[src_pos], src_block_len);
            alias[src_block_len] = '\0';
            if (pf_lldp_is_alias_matching (net, alias))
            {
               if (first == true)
               {
                  p_req_alias_name = alias;
                  filter = true;
               }
            }
            else
            {
               match = false;
            }
         }
         else
         {
            match = false;
         }

         src_pos += src_block_len;

         /* Skip padding to align on uint16_t */
         while (src_pos & 1)
         {
            src_pos++;
         }
         /* Prepare for the next round */
         p_src_block_hdr = (pf_dcp_block_hdr_t *)&p_src[src_pos];
         src_block_len = ntohs (p_src_block_hdr->block_length);
         src_pos += sizeof (*p_src_block_hdr);
      }
      else
      {
         LOG_DEBUG (
            PF_DCP_LOG,
            "DCP(%d): Unknown incoming DCP identify request.\n",
            __LINE__);
         ret = -1; /* Give up on bad request */
      }

      first = false;
   }

   if ((ret == 0) && (match == true))
   {

      /* Build the response */
      for (ix = 0; ix < NELEMENTS (device_options); ix++)
      {
         pf_dcp_get_req (
            net,
            p_dst,
            &dst_pos,
            PF_FRAME_BUFFER_SIZE,
            device_options[ix].opt,
            device_options[ix].sub,
            true,
            p_req_alias_name);
      }

      /* Insert final response length and ship it! */
      p_dst_dcphdr->data_length = htons (dst_pos - dst_start);
      p_rsp->len = dst_pos;

      response_delay = pf_dcp_calculate_response_delay (
         mac_address,
         ntohs (p_src_dcphdr->response_delay_factor));

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
         "StationName: %.*s Alias: %.*s  Delay %" PRIu32 " us. Xid: %" PRIu32
         "\n",
         __LINE__,
         identify_all,
         stationname_len,
         &p_src[stationname_position], /* Not terminated */
         alias_len,
         &p_src[alias_position], /* Not terminated */
         response_delay,
         ntohl (p_src_dcphdr->xid));
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
         "StationName: %.*s Alias: %.*s Xid: %" PRIu32 "\n",
         __LINE__,
         identify_all,
         stationname_len,
         &p_src[stationname_position], /* Not terminated */
         alias_len,
         &p_src[alias_position], /* Not terminated */
         ntohl (p_src_dcphdr->xid));
#endif

      pnal_buf_free (p_rsp);
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
