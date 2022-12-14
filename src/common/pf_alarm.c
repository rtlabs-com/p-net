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
 * @brief Alarm implementation
 *
 * Sends and receives low- and high prio alarm Ethernet frames.
 *
 * A timer resends alarms if not acknowledged.
 *
 * - ALPMI Sends alarm (initiator)
 * - ALPMR Receives alarm (responder)
 * - APMR  Receives high- and low priority alarm Ethernet frames from the
 *         IO-controller.
 * - APMS  Sends alarm Ethernet frames to IO-controller
 *
 * See the documentation of this project for an overview of alarm implementation
 * details.
 *
 * Note that ALPMX is a general term for ALPMI and ALPMR, and APMX is a
 * general term for APMS and APMR.
 *
 * There is one APMS+APMR pair for low prio messages and one pair for high prio
 * messages. Similarly there is one ALPMI+ALPMR pair for low prio and one pair
 * for high prio.
 *
 * A frame handler is registered for each of the high prio and low prio
 * alarm frame IDs.
 *
 * Each AR has two alarm receive queues, one for low prio and one for high
 * prio alarm frames. The thread responsible for processing incoming frames
 * puts frames into the queues, and another thread (driven by the
 * periodic task) retrieves messages from the receive queues.
 * A message in an input queue contains a pointer to an incoming frame buffer.
 *
 * Each AR has its own alarm send queue. As alarms can only be processed
 * one-by-one, a send queue is necessary. The same thread puts messages
 * into the send queue and retrieves messages from the send queue.
 * A message in the alarm send queue contains the full alarm, including payload.
 *
 * There are convenience functions to send different types of alarms, for
 * example process alarms.
 *
 * a_data means acyclic data.
 *
 */

#ifdef UNIT_TEST
#endif

/*
 * ToDo:
 * 1) Send UDP frames.
 */

#include <string.h>

#include "pf_includes.h"
#include "pf_block_reader.h"
#include "pf_block_writer.h"

#define PF_FRAME_ID_ALARM_HIGH 0xfc01
#define PF_FRAME_ID_ALARM_LOW  0xfe01

#define ALARM_VLAN_PRIO_LOW  5
#define ALARM_VLAN_PRIO_HIGH 6

#if PNET_MAX_ALARMS < 1
#error "PNET_MAX_ALARMS needs to be at least 1"
#endif

/* TODO Add reference to Profinet specification section */
#if PNET_MAX_ALARM_PAYLOAD_DATA_SIZE > 1408
#error "PNET_MAX_ALARM_PAYLOAD_DATA_SIZE is too large"
#endif

#if PNET_MAX_DIAG_MANUF_DATA_SIZE > 1396
#error "PNET_MAX_DIAG_MANUF_DATA_SIZE is too large"
#endif

CC_STATIC_ASSERT (PNET_MAX_ALARM_PAYLOAD_DATA_SIZE >= sizeof (pf_diag_item_t));

/*************** Diagnostic strings *****************************************/

/**
 * @internal
 * Return a string representation of alarm PDU type (message type).
 * @param pdu_type         In:    alarm PDU type
 *                                See pf_alarm_pdu_type_values_t
 * @return  A string representing the alarm PDU type.
 */
static const char * pf_alarm_pdu_type_to_string (uint8_t pdu_type)
{
   const char * s = NULL;

   switch (pdu_type)
   {
   case PF_RTA_PDU_TYPE_DATA:
      s = "PF_RTA_PDU_TYPE_DATA";
      break;
   case PF_RTA_PDU_TYPE_NACK:
      s = "PF_RTA_PDU_TYPE_NACK";
      break;
   case PF_RTA_PDU_TYPE_ACK:
      s = "PF_RTA_PDU_TYPE_ACK";
      break;
   case PF_RTA_PDU_TYPE_ERR:
      s = "PF_RTA_PDU_TYPE_ERR";
      break;
   default:
      s = "<error>";
      break;
   }

   return s;
}

/**
 * @internal
 * Return a string representation of the given ALPMI state.
 * @param state            In:    The ALPMI state.
 * @return  A string representing the ALPMI state.
 */
static const char * pf_alarm_alpmi_state_to_string (
   pf_alpmi_state_values_t state)
{
   const char * s = "<error>";

   switch (state)
   {
   case PF_ALPMI_STATE_W_START:
      s = "PF_ALPMI_STATE_W_START";
      break;
   case PF_ALPMI_STATE_W_ALARM:
      s = "PF_ALPMI_STATE_W_ALARM";
      break;
   case PF_ALPMI_STATE_W_ACK:
      s = "PF_ALPMI_STATE_W_ACK";
      break;
   }

   return s;
}

/**
 * @internal
 * Return a string representation of the given ALPMR state.
 * @param state            In:    The ALPMR state.
 * @return  A string representing the ALPMR state.
 */
static const char * pf_alarm_alpmr_state_to_string (
   pf_alpmr_state_values_t state)
{
   const char * s = "<error>";

   switch (state)
   {
   case PF_ALPMR_STATE_W_START:
      s = "PF_ALPMR_STATE_W_START";
      break;
   case PF_ALPMR_STATE_W_NOTIFY:
      s = "PF_ALPMR_STATE_W_NOTIFY";
      break;
   case PF_ALPMR_STATE_W_USER_ACK:
      s = "PF_ALPMR_STATE_W_USER_ACK";
      break;
   case PF_ALPMR_STATE_W_TACK:
      s = "PF_ALPMR_STATE_W_TACK";
      break;
   }

   return s;
}

/**
 * @internal
 * Return a string representation of the given APMS state.
 * @param state            In:    The APMS state.
 * @return  A string representing the APMS state.
 */
static const char * pf_alarm_apms_state_to_string (pf_apms_state_values_t state)
{
   const char * s = "<error>";

   switch (state)
   {
   case PF_APMS_STATE_CLOSED:
      s = "PF_APMS_STATE_CLOSED";
      break;
   case PF_APMS_STATE_OPEN:
      s = "PF_APMS_STATE_OPEN";
      break;
   case PF_APMS_STATE_WTACK:
      s = "PF_APMS_STATE_WTACK";
      break;
   }

   return s;
}

/**
 * @internal
 * Return a string representation of the given APMR state.
 * @param state            In:    The APMR state.
 * @return  A string representing the APMR state.
 */
static const char * pf_alarm_apmr_state_to_string (pf_apmr_state_values_t state)
{
   const char * s = "<error>";

   switch (state)
   {
   case PF_APMR_STATE_CLOSED:
      s = "PF_APMR_STATE_CLOSED";
      break;
   case PF_APMR_STATE_OPEN:
      s = "PF_APMR_STATE_OPEN";
      break;
   case PF_APMR_STATE_WCNF:
      s = "PF_APMR_STATE_WCNF";
      break;
   }

   return s;
}

void pf_alarm_show (const pf_ar_t * p_ar)
{
   printf ("Alarms\n");
   printf ("  PNET_MAX_ALARMS             = %d\n", PNET_MAX_ALARMS);
   printf ("Alarms   (low prio)\n");
   printf (
      "  alpmi_state                 = %s\n",
      pf_alarm_alpmi_state_to_string (p_ar->alpmx[0].alpmi_state));
   printf (
      "  alpmr_state                 = %s\n",
      pf_alarm_alpmr_state_to_string (p_ar->alpmx[0].alpmr_state));
   printf (
      "  apms_state                  = %s\n",
      pf_alarm_apms_state_to_string (p_ar->apmx[0].apms_state));
   printf (
      "  apmr_state                  = %s\n",
      pf_alarm_apmr_state_to_string (p_ar->apmx[0].apmr_state));
   printf (
      "  exp_seq_count               = 0x%x\n",
      (unsigned)p_ar->apmx[0].exp_seq_count);
   printf (
      "  exp_seq_count_o             = 0x%x\n",
      (unsigned)p_ar->apmx[0].exp_seq_count_o);
   printf (
      "  send_seq_count              = 0x%x\n",
      (unsigned)p_ar->apmx[0].send_seq_count);
   printf (
      "  send_seq_count_o            = 0x%x\n",
      (unsigned)p_ar->apmx[0].send_seq_count_o);
   printf (
      "  sequence_number             = %u\n",
      (unsigned)p_ar->alpmx[0].sequence_number);
   printf (
      "  Number of frames in incoming queue = %u\n",
      p_ar->apmx[0].alarm_receive_q.accountant.count);
   printf ("Alarms   (high prio)\n");
   printf (
      "  alpmi_state                 = %s\n",
      pf_alarm_alpmi_state_to_string (p_ar->alpmx[1].alpmi_state));
   printf (
      "  alpmr_state                 = %s\n",
      pf_alarm_alpmr_state_to_string (p_ar->alpmx[1].alpmr_state));
   printf (
      "  apms_state                  = %s\n",
      pf_alarm_apms_state_to_string (p_ar->apmx[1].apms_state));
   printf (
      "  apmr_state                  = %s\n",
      pf_alarm_apmr_state_to_string (p_ar->apmx[1].apmr_state));
   printf (
      "  exp_seq_count               = 0x%x\n",
      (unsigned)p_ar->apmx[1].exp_seq_count);
   printf (
      "  exp_seq_count_o             = 0x%x\n",
      (unsigned)p_ar->apmx[1].exp_seq_count_o);
   printf (
      "  send_seq_count              = 0x%x\n",
      (unsigned)p_ar->apmx[1].send_seq_count);
   printf (
      "  send_seq_count_o            = 0x%x\n",
      (unsigned)p_ar->apmx[1].send_seq_count_o);
   printf (
      "  sequence_number             = %u\n",
      (unsigned)p_ar->alpmx[1].sequence_number);
   printf (
      "  Number of frames in incoming queue = %u\n",
      p_ar->apmx[1].alarm_receive_q.accountant.count);
}

/*****************************************************************************/

void pf_alarm_init (pnet_t * net)
{
   /* Enable alarms from the start */
   net->global_alarm_enable = true;
}

/**
 * @internal
 * Send an ALARM error indication to CMSU.
 *
 * ALPMI: ALPMI_Error_ind
 * ALPMR: ALPMR_Error_ind
 * APMS:  APMS_Error_ind
 * APMR:  APMR_Error_ind
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           InOut: The APMX instance.
 * @param err_cls          In:    The ERRCLS variable. ErrorCode1.
 * @param err_code         In:    The ERRCODE variable. ErrorCode2.
 */
static void pf_alarm_error_ind (
   pnet_t * net,
   pf_apmx_t * p_apmx,
   uint8_t err_cls,
   uint8_t err_code)
{
   /*
    * ALPMI: PNET_ERROR_CODE_2_CMSU_AR_ALARM_SEND;
    * ALPMI: PNET_ERROR_CODE_2_CMSU_AR_ALARM_ACK_SEND;
    *
    * ALPMR: PNET_ERROR_CODE_2_CMSU_AR_ALARM_IND
    *
    * APMR:  PNET_ERROR_CODE_2_CMSU_AR_ALARM_IND
    */
   LOG_INFO (
      PF_ALARM_LOG,
      "Alarm(%d): Setting err_cls 0x%02x err_code 0x%02x\n",
      __LINE__,
      err_cls,
      err_code);
   (void)pf_cmsu_alarm_error_ind (net, p_apmx->p_ar, err_cls, err_code);
}

/**
 * @internal
 * Activate the ALPMI and ALPMR instances.
 *
 * ALPMI: ALPMI_Activate_req
 * ALPMI: ALPMI_Activate_cnf(+/-) via return value and err_cls/err_code
 * ALPMR: ALPMR_Activate_req
 * ALPMR: ALPMR_Activate_cnf(+/-) via return value and err_cls/err_code
 *
 * @param p_ar             InOut: The AR instance.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_alpmx_activate (pf_ar_t * p_ar)
{
   int ret = 0; /* Assume all goes well */
   uint16_t ix;

   for (ix = 0; ix < NELEMENTS (p_ar->alpmx); ix++)
   {
      if (
         p_ar->alpmx[ix].alpmi_state == PF_ALPMI_STATE_W_START &&
         p_ar->alpmx[ix].alpmr_state == PF_ALPMR_STATE_W_START)
      {
         /* Save the remote side MAC address for easy retrieval. */
         memcpy (
            &p_ar->alpmx[ix].da,
            &p_ar->ar_param.cm_initiator_mac_add,
            sizeof (p_ar->alpmx[ix].da));

         p_ar->alpmx[ix].prev_sequence_number = 0xffff;
         p_ar->alpmx[ix].sequence_number = 0;

         p_ar->alpmx[ix].alpmr_state = PF_ALPMR_STATE_W_NOTIFY;
         p_ar->alpmx[ix].alpmi_state = PF_ALPMI_STATE_W_ALARM;
      }
      else if (p_ar->alpmx[ix].alpmi_state != PF_ALPMI_STATE_W_START)
      {
         /* This is ALPMI_Activate_cnf(-) */
         p_ar->err_cls = PNET_ERROR_CODE_1_ALPMI;
         p_ar->err_code = PNET_ERROR_CODE_2_ALPMI_WRONG_STATE;

         ret = -1;
      }
      else
      {
         /* This is ALPMR_Activate_cnf(-) */
         p_ar->err_cls = PNET_ERROR_CODE_1_ALPMR;
         p_ar->err_code = PNET_ERROR_CODE_2_ALPMR_WRONG_STATE;

         ret = -1;
      }
   }

   return ret;
}

/**
 * @internal
 * Close all the ALPMI and ALPMR instances.
 *
 * ALPMI: ALPMI_Close_req
 * ALPMI: ALPMI_Close_cnf(+)
 * ALPMR: ALPMR_Close_req
 * ALPMR: ALPMR_Close_cnf    via return value and err_cls/err_code
 *
 * @param p_ar             InOut: The AR instance.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_alpmx_close (pf_ar_t * p_ar)
{
   int ret = 0; /* Assume all goes well */
   uint16_t ix;

   /* This is ALPMI_Close_req */
   for (ix = 0; ix < NELEMENTS (p_ar->alpmx); ix++)
   {
      p_ar->alpmx[ix].alpmi_state = PF_ALPMI_STATE_W_START;
   }

   /* This is ALPMR_Close_req */
   for (ix = 0; ix < NELEMENTS (p_ar->alpmx); ix++)
   {
      switch (p_ar->alpmx[ix].alpmr_state)
      {
      case PF_ALPMR_STATE_W_START:
         /* Nothing special */
         break;
      case PF_ALPMR_STATE_W_NOTIFY:
      case PF_ALPMR_STATE_W_USER_ACK:
      case PF_ALPMR_STATE_W_TACK:
         /*
          * According to the spec we should call pf_alarm_error_ind here with
          * ERRCLS and ERRCODE. Since this avalanches into CMDEV which calls
          * pf_cmsu_cmdev_state_ind which calls this function we would create an
          * eternal loop. So dont call here.
          *
          * CMSU is the only caller of pf_alarm_close - why would we need to
          * call CMSU with an error indicating that we are closing??
          */

         p_ar->alpmx[ix].alpmr_state = PF_ALPMR_STATE_W_START;
         /* pf_alarm_error_ind(net, &p_ar->apmx[ix], p_ar->err_cls,
          * PNET_ERROR_CODE_2_CMSU_AR_ALARM_IND); */
         break;
      }
   }

   return ret;
}

/**
 * @internal
 * Confirmation back to ALPMI on message sent via APMS.
 *
 * ALPMI: APMS_A_Data.cnf(+/-)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           InOut: The APMS instance sending the confirmation.
 * @param res              In:    0  positive confirmation. This is cnf(+)
 *                                -1 negative confirmation. This is cnf(-)
 */
static void pf_alarm_alpmi_apms_a_data_cnf (
   pnet_t * net,
   pf_apmx_t * p_apmx,
   int res)
{
   switch (p_apmx->p_alpmx->alpmi_state)
   {
   case PF_ALPMI_STATE_W_START:
   case PF_ALPMI_STATE_W_ALARM:
   case PF_ALPMI_STATE_W_ACK:
      if (res != 0)
      {
         /* Handle neg cnf */
         p_apmx->p_ar->err_cls = PNET_ERROR_CODE_1_ALPMI;
         p_apmx->p_ar->err_code = PNET_ERROR_CODE_2_ALPMI_INVALID;
         pf_alarm_error_ind (
            net,
            p_apmx,
            p_apmx->p_ar->err_cls,
            p_apmx->p_ar->err_code);
      }
      break;
   }
}

/**
 * @internal
 * The APMR has received an Alarm ACK DATA message over wire, and tells ALPMI
 * about the result.
 *
 * ALPMI: APMR_A_Data.ind  (Implements part of it)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           InOut: The APMS instance sending the confirmation.
 * @param p_pnio_status    In:    Detailed ACK information.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_alpmi_apmr_a_data_ind (
   pnet_t * net,
   pf_apmx_t * p_apmx,
   const pnet_pnio_status_t * p_pnio_status)
{
   int ret = -1;

   switch (p_apmx->p_alpmx->alpmi_state)
   {
   case PF_ALPMI_STATE_W_START:
   case PF_ALPMI_STATE_W_ALARM:
      /* Ignore */
      ret = 0;
      break;
   case PF_ALPMI_STATE_W_ACK:
      /* This function is only called for DATA = ACK */
      p_apmx->p_alpmx->alpmi_state = PF_ALPMI_STATE_W_ALARM;
      pf_fspm_alpmi_alarm_cnf (net, p_apmx->p_ar, p_pnio_status);
      ret = 0;
      break;
   }

   return ret;
}

/**
 * @internal
 * Confirmation back to ALPMR from APMS that a Transport ACK (TACK) has been
 * received.
 *
 * Calls user call-back \a pnet_alarm_ack_cnf() for some cases.
 *
 * ALPMR: APMS_A_Data.cnf(+/-)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           InOut: The APMS instance sending the confirmation.
 * @param res              In:    0  positive confirmation. This is conf(+)
 *                                -1 negative confirmation. This is conf(-)
 */
static void pf_alarm_alpmr_apms_a_data_cnf (
   pnet_t * net,
   pf_apmx_t * p_apmx,
   int res)
{
   switch (p_apmx->p_alpmx->alpmr_state)
   {
   case PF_ALPMR_STATE_W_START:
      /* Ignore */
      break;
   case PF_ALPMR_STATE_W_NOTIFY:
   case PF_ALPMR_STATE_W_USER_ACK:
      if (res != 0)
      {
         /* Handle neg cnf */
         p_apmx->p_ar->err_cls = PNET_ERROR_CODE_1_ALPMR;
         p_apmx->p_ar->err_code = PNET_ERROR_CODE_2_ALPMR_INVALID;
         pf_alarm_error_ind (
            net,
            p_apmx,
            p_apmx->p_ar->err_cls,
            p_apmx->p_ar->err_code);
      }
      else
      {
         /* Ignore pos cnf */
      }
      break;
   case PF_ALPMR_STATE_W_TACK:
      if (res != 0)
      {
         /* Handle neg cnf */
         p_apmx->p_ar->err_cls = PNET_ERROR_CODE_1_ALPMR;
         p_apmx->p_ar->err_code = PNET_ERROR_CODE_2_ALPMR_INVALID;
         pf_alarm_error_ind (
            net,
            p_apmx,
            p_apmx->p_ar->err_cls,
            p_apmx->p_ar->err_code);
      }
      else
      {
         /* Handle pos cnf */
         /* ALPMR: ALPMR_Alarm_Ack.cnf(+) */
         pf_fspm_alpmr_alarm_ack_cnf (net, p_apmx->p_ar, res);
         p_apmx->p_alpmx->alpmr_state = PF_ALPMR_STATE_W_NOTIFY;
      }
      break;
   }
}

/**
 * @internal
 * The APMR has received an Alarm Notification DATA message over wire, and tells
 * ALPMR about the result.
 *
 * We have already sent the transport acknowledge ("TACK") frame.
 *
 * Will trigger the user callback \a pnet_alarm_ind().
 *
 * ALPMR: APMR_A_Data.ind
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           InOut: The APMR instance that received the message.
 * @param p_fixed          In:    The Fixed part of the RTA-PDU. Not used.
 * @param p_alarm_data     In:    The AlarmData from the RTA-PDU. (Slot, subslot
 *                                etc)
 * @param data_len         In:    VarDataLen from the RTA-PDU.
 * @param data_usi         In:    The USI from the RTA-PDU.
 * @param p_data           In:    The variable part of the RTA-PDU.
 * @return
 */
static int pf_alarm_alpmr_apmr_a_data_ind (
   pnet_t * net,
   pf_apmx_t * p_apmx,
   const pf_alarm_fixed_t * p_fixed,
   const pf_alarm_data_t * p_alarm_data,
   uint16_t data_len,
   uint16_t data_usi,
   const uint8_t * p_data)
{
   pnet_alarm_argument_t alarm_arg = {0};
   int ret = -1;

   switch (p_apmx->p_alpmx->alpmr_state)
   {
   case PF_ALPMR_STATE_W_START:
      /* Ignore */
      ret = 0;
      break;
   case PF_ALPMR_STATE_W_NOTIFY:
      /* Only DATA: AlarmNotifications are sent to this function */
      alarm_arg.api_id = p_alarm_data->api_id;
      alarm_arg.slot_nbr = p_alarm_data->slot_nbr;
      alarm_arg.subslot_nbr = p_alarm_data->subslot_nbr;
      alarm_arg.alarm_type = p_alarm_data->alarm_type;
      alarm_arg.alarm_specifier = p_alarm_data->alarm_specifier;
      alarm_arg.sequence_number = p_alarm_data->sequence_number;

      p_apmx->p_alpmx->alpmr_state = PF_ALPMR_STATE_W_USER_ACK;

      /*
       * Indicate alarm to app. App must respond to callback
       * using pnet_alarm_send_ack()
       */
      (void)pf_fspm_alpmr_alarm_ind (
         net,
         p_apmx->p_ar,
         &alarm_arg,
         data_len,
         data_usi,
         p_data);
      ret = 0;
      break;
   case PF_ALPMR_STATE_W_USER_ACK:
      /* ToDo: "Combine data for the RTA error request */
      /* FALL-THRU */
   case PF_ALPMR_STATE_W_TACK:
      p_apmx->p_ar->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
      p_apmx->p_ar->err_code = PNET_ERROR_CODE_2_ABORT_AR_PROTOCOL_VIOLATION;
      pf_alarm_error_ind (
         net,
         p_apmx,
         p_apmx->p_ar->err_cls,
         p_apmx->p_ar->err_code);
      break;
   }

   return ret;
}

uint16_t pf_alarm_allocate_endpoint (pnet_t * net, pf_ar_t * ar)
{
   uint16_t i;

   for (i = 0; i < PNET_MAX_AR; ++i)
   {
      if (net->alarm_endpoint[i].ar == NULL)
      {
         net->alarm_endpoint[i].active = false;
         net->alarm_endpoint[i].ar = ar;
         return (i + 1);
      }
   }
   return 0;
}

void pf_alarm_free_endpoint (pnet_t * net, pf_ar_t * ar)
{
   uint16_t i;

   for (i = 0; i < PNET_MAX_AR; ++i)
   {
      if (net->alarm_endpoint[i].ar == ar)
      {
         net->alarm_endpoint[i].active = false;
         net->alarm_endpoint[i].ar = NULL;
         return;
      }
   }
}

/*********************** Frame handler callback ******************************/

/**
 * @internal
 * Handle an alarm frame from the IO-controller, and put it into the
 * correct alarm frame input queue.
 *
 * There are two instances of this handler, one for low prio alarm frames
 * and one for high prio alarm frames (delivering to separare queues).
 *
 * All incoming Profinet frames with ID = PF_FRAME_ID_ALARM_LOW and
 * ID = PF_FRAME_ID_ALARM_HIGH end up here.
 *
 * This is a callback for the frame handler. Arguments should fulfill
 * pf_eth_frame_handler_t
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:    The Ethernet frame id.
 * @param p_buf            In:    The Ethernet frame buffer.
 * @param frame_id_pos     In:    Position in the buffer of the frame id.
 *                                Depends on whether VLAN tagging is used.
 * @param p_arg            In:    Not used, should be NULL.
 * @return  0 if the frame was NOT handled by this function.
 *          1 if the frame was handled and the buffer was freed.
 */
static int pf_alarm_apmr_frame_handler (
   pnet_t * net,
   uint16_t frame_id,
   pnal_buf_t * p_buf,
   uint16_t frame_id_pos,
   void * p_arg)
{
   int priority;
   pf_apmr_msg_t tempmsg;
   pf_get_info_t get_info;
   pf_ar_t * ar;
   uint16_t alarm_endpoint;
   uint16_t pos;
   int ret;

   ret = 0; /* Failed to handle frame. The frame handler needs to free
                   the buffer. */
   priority = (frame_id == PF_FRAME_ID_ALARM_HIGH) ? 1 : 0;
   ar = NULL;

   if (p_buf != NULL)
   {
      tempmsg.p_buf = p_buf;
      tempmsg.frame_id_pos = frame_id_pos;

      /* Parse the destination endpoint of the alarm,
         to know where it should be queued. */

      get_info.result = PF_PARSE_OK;
      get_info.is_big_endian = true;
      get_info.p_buf = (uint8_t *)p_buf->payload;
      get_info.len = p_buf->len;

      pos = frame_id_pos + sizeof (uint16_t); /* Skip frame_id. */
      alarm_endpoint = pf_get_uint16(&get_info, &pos);

      if (
         (alarm_endpoint > 0) && (alarm_endpoint <= PNET_MAX_AR) &&
         net->alarm_endpoint[alarm_endpoint - 1].active)
      {
         ar = net->alarm_endpoint[alarm_endpoint - 1].ar;
      }

      if (
         (ar != NULL) && (pf_alarm_receive_queue_post (
                             &ar->apmx[priority].alarm_receive_q,
                             &tempmsg) == 0))
      {
         LOG_INFO (
            PF_ALARM_LOG,
            "Alarm(%d): Received %s prio alarm frame %p. Put in queue. \n",
            __LINE__,
            priority ? "high" : "low",
            p_buf);

         ret = 1; /* Means that calling function should not free buffer,
                     as that will be done when reading the queue */
      }
      else
      {
         LOG_ERROR (
            PF_ALARM_LOG,
            "Alarm(%d): Failed to put incoming %s prio alarm frame in "
            "queue. "
            "Framehandler will free %p\n",
            __LINE__,
            priority ? "high" : "low",
            p_buf);
      }
   }
   else
   {
      /* No need for the frame handler to free p_buf as it is NULL */
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Did not put incoming %s prio alarm frame in queue, "
         "as p_buf is NULL.\n",
         __LINE__,
         priority ? "high" : "low");
      ret = 1;
   }

   return ret;
}

/********************* Scheduler callback ************************************/

/**
 * @internal
 * Timeout while waiting for an alarm TACK from the controller.
 *
 * Re-send the alarm frame unless the number of re-transmits have been reached.
 * If an ACK is received then the APMS state is no longer WTACK and the
 * frame is not re-sent (and the timer stops).
 *
 * Might free the frame buffer.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * APMS: TimerExpired
 * APMS: A_Data_req   retransmission   (This is LMPM_A_Data.req via macro)
 *
 * @param net              InOut: The p-net stack instance
 * @param arg
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 *                                stored tasks.
 */
static void pf_alarm_apms_timeout (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{
   pf_apmx_t * p_apmx = (pf_apmx_t *)arg;
   pnal_buf_t * p_rta;

   pf_scheduler_reset_handle (&p_apmx->resend_timeout);

   if (p_apmx->apms_state == PF_APMS_STATE_WTACK)
   {
      if (p_apmx->p_rta == NULL)
      {
         LOG_ERROR (
            PF_ALARM_LOG,
            "Alarm(%d): No alarm frame available for resending.\n",
            __LINE__);
      }
      else if (p_apmx->resend_counter > 0)
      {
         p_apmx->resend_counter--;

         /* Retransmit */
         if (p_apmx->p_ar->alarm_cr_request.alarm_cr_properties.transport_udp == true)
         {
            /* ToDo: Send as UDP frame */
         }
         else
         {
            LOG_INFO (
               PF_ALARM_LOG,
               "Alarm(%d): Re-sending %s prio alarm frame in %p\n",
               __LINE__,
               p_apmx->high_priority ? "high" : "low",
               p_apmx->p_rta);
            (void)pf_eth_send_on_management_port (net, p_apmx->p_rta);
         }

         if (
            pf_scheduler_add (
               net,
               p_apmx->timeout_us,
               pf_alarm_apms_timeout,
               p_apmx,
               &p_apmx->resend_timeout) != 0)
         {
            LOG_ERROR (
               PF_ALARM_LOG,
               "Alarm(%d): Failed to schedule resending of alarm frame\n",
               __LINE__);
         }
      }
      else
      {
#if 0
         /*
          * After the call to pf_alarm_error_ind() above CMDEV closes the connection.
          * CMDEV also calls CMSU which calls pf_alarm_close.
          *
          * There is no need to do anything more here!!!
          */
         LOG_DEBUG(PF_AL_BUF_LOG, "Alarm(%d): Free saved RTA buffer %p\n", __LINE__, p_apmx->p_rta);
         if (p_apmx->p_rta != NULL)
         {
            p_rta = p_apmx->p_rta;
            p_apmx->p_rta = NULL;
            pnal_buf_free(p_rta);
         }

         pf_alarm_alpmi_apms_a_data_cnf(net, p_apmx, -1);
         pf_alarm_alpmr_apms_a_data_cnf(net, p_apmx, -1);

#endif

         /* Timeout */
         p_apmx->apms_state = PF_APMS_STATE_OPEN;

         p_apmx->p_ar->err_cls = PNET_ERROR_CODE_1_APMS;
         p_apmx->p_ar->err_code = PNET_ERROR_CODE_2_ABORT_AR_ALARM_SEND_CNF_NEG;
         pf_alarm_error_ind (
            net,
            p_apmx,
            p_apmx->p_ar->err_cls,
            p_apmx->p_ar->err_code);
         /* Standard also says APMS_A_Data.cnf(-) */
      }
   }
   else
   {
      LOG_DEBUG (
         PF_ALARM_LOG,
         "Alarm(%d): Skip scheduled alarm retransmission as we are in in state "
         "%s. Free saved %s prio alarm output buffer %p\n",
         __LINE__,
         pf_alarm_apms_state_to_string (p_apmx->apms_state),
         p_apmx->high_priority ? "high" : "low",
         p_apmx->p_rta);
      if (p_apmx->p_rta != NULL)
      {
         p_rta = p_apmx->p_rta;
         p_apmx->p_rta = NULL;
         pnal_buf_free (p_rta);
      }
   }
}

/****************************************************************************/

/**
 * @internal
 * Initialize and start an AMPX instance (an APMS/APMR pair).
 *
 * It registers handlers for incoming alarm frames.
 *
 * APMS: APMS_Activate_req
 * APMS: APMS_Activate_cnf(+/-) via return value and err_cls/err_code
 * APMR: APMR_Activate_req
 * APMR: APMR_Activate_cnf(+/-) via return value and err_cls/err_code
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmx_activate (pnet_t * net, pf_ar_t * p_ar)
{
   int ret = 0; /* Assume all goes well */
   uint16_t ix;

   LOG_DEBUG (
      PF_ALARM_LOG,
      "Alarm(%d): Activating ALPMX for AREP %u\n",
      __LINE__,
      p_ar->arep);

   for (ix = 0; ix < NELEMENTS (p_ar->apmx); ix++)
   {
      if (p_ar->apmx[ix].apms_state != PF_APMS_STATE_CLOSED)
      {
         p_ar->err_cls = PNET_ERROR_CODE_1_APMS;
         p_ar->err_code = PNET_ERROR_CODE_2_APMS_INVALID_STATE;

         ret = -1;
      }
      else if (p_ar->apmx[ix].apmr_state != PF_APMR_STATE_CLOSED)
      {
         p_ar->err_cls = PNET_ERROR_CODE_1_APMR;
         p_ar->err_code = PNET_ERROR_CODE_2_APMR_INVALID_STATE;

         ret = -1;
      }
      else
      {
         memcpy (
            &p_ar->apmx[ix].da,
            &p_ar->ar_param.cm_initiator_mac_add,
            sizeof (p_ar->apmx[ix].da));

         p_ar->apmx[ix].src_ref = p_ar->alarm_cr_result.local_alarm_reference;
         p_ar->apmx[ix].dst_ref = p_ar->alarm_cr_request.local_alarm_reference;

         /* APMS counters */
         p_ar->apmx[ix].send_seq_count = 0xffff;
         p_ar->apmx[ix].send_seq_count_o = 0xfffe;

         /* APMR counters */
         p_ar->apmx[ix].exp_seq_count = 0xffff;
         p_ar->apmx[ix].exp_seq_count_o = 0xfffe;

         if (ix == 0)
         {
            p_ar->apmx[ix].high_priority = false;
            p_ar->apmx[ix].vlan_prio = ALARM_VLAN_PRIO_LOW;
            p_ar->apmx[ix].block_type_alarm_notify =
               PF_BT_ALARM_NOTIFICATION_LOW;
            p_ar->apmx[ix].block_type_alarm_ack = PF_BT_ALARM_ACK_LOW;
            p_ar->apmx[ix].frame_id = PF_FRAME_ID_ALARM_LOW;
         }
         else
         {
            p_ar->apmx[ix].high_priority = true;
            p_ar->apmx[ix].vlan_prio = ALARM_VLAN_PRIO_HIGH;
            p_ar->apmx[ix].block_type_alarm_notify =
               PF_BT_ALARM_NOTIFICATION_HIGH;
            p_ar->apmx[ix].block_type_alarm_ack = PF_BT_ALARM_ACK_HIGH;
            p_ar->apmx[ix].frame_id = PF_FRAME_ID_ALARM_HIGH;
         }

         /* Input alarm frame queue */
         pf_alarm_queue_mutex_create (
            &p_ar->apmx[ix].alarm_receive_q.accountant);
         pf_alarm_receive_queue_reset (&p_ar->apmx[ix].alarm_receive_q);

         p_ar->apmx[ix].timeout_us =
            100 * 1000 * p_ar->alarm_cr_request.rta_timeout_factor;
         p_ar->apmx[ix].resend_counter = 0; /* Updated in
                                             * pf_alarm_apms_apms_a_data_req()
                                             */

         p_ar->apmx[ix].p_ar = p_ar;
         p_ar->apmx[ix].p_alpmx = &p_ar->alpmx[ix];

         pf_scheduler_init_handle (&p_ar->apmx[ix].resend_timeout, "apmx");

         p_ar->apmx[ix].apms_state = PF_APMS_STATE_OPEN;
         p_ar->apmx[ix].apmr_state = PF_APMR_STATE_OPEN;
      }
   }

   /* Add the frame handler, if no alarm endpoint is active at this point. */
   for (ix = 0; (ix < PNET_MAX_AR) && (net->alarm_endpoint[ix].active == false);
        ++ix)
      ;
   if (ix >= PNET_MAX_AR)
   {
      pf_eth_frame_id_map_add (
            net,
            PF_FRAME_ID_ALARM_LOW,
            pf_alarm_apmr_frame_handler,
            NULL);
      pf_eth_frame_id_map_add (
            net,
            PF_FRAME_ID_ALARM_HIGH,
            pf_alarm_apmr_frame_handler,
            NULL);
   }

   /* Mark the alarm endpoint as active. */
   for (ix = 0; ix < PNET_MAX_AR; ++ix)
   {
      if (net->alarm_endpoint[ix].ar == p_ar)
      {
         net->alarm_endpoint[ix].active = true;
         break;
      }
   }

   return ret;
}

/**
 * @internal
 * APMS is checking incoming TACK (the ack_seq_num)
 *
 * APMS: A_Data_ind  (Implements parts of it. This ia LMPM_A_Data.ind via macro)
 *
 * Does free the outgoing alarm frame buffer.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           InOut: The APMX instance.
 * @param p_fixed          In:    The fixed part of the ALARM message.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apms_a_data_ind (
   pnet_t * net,
   pf_apmx_t * p_apmx,
   const pf_alarm_fixed_t * p_fixed)
{
   int ret = -1;
   pnal_buf_t * p_rta;

   if (
      (p_fixed->pdu_type.type == PF_RTA_PDU_TYPE_ACK) ||
      (p_fixed->pdu_type.type == PF_RTA_PDU_TYPE_DATA))
   {
      if (p_apmx->apms_state == PF_APMS_STATE_WTACK)
      {
         if (p_fixed->ack_seq_nbr == p_apmx->send_seq_count)
         {
            p_apmx->send_seq_count_o = p_apmx->send_seq_count;
            p_apmx->send_seq_count = (p_apmx->send_seq_count + 1) & 0x7fff;

            LOG_DEBUG (
               PF_AL_BUF_LOG,
               "Alarm(%d): Valid TACK received via DATA or ACK frame. Free "
               "saved outgoing %s prio alarm buffer if necessary %p\n",
               __LINE__,
               p_apmx->high_priority ? "high" : "low",
               p_apmx->p_rta);
            if (p_apmx->p_rta != NULL)
            {
               p_rta = p_apmx->p_rta;
               p_apmx->p_rta = NULL;
               pnal_buf_free (p_rta);
            }

            /* This is APMS_A_Data.cnf(+) */
            pf_alarm_alpmi_apms_a_data_cnf (net, p_apmx, 0);

            pf_alarm_alpmr_apms_a_data_cnf (net, p_apmx, 0);

            ret = 0;

            p_apmx->apms_state = PF_APMS_STATE_OPEN;
         }
         else
         {
            /*
             * Ignore!
             * Wrong sequence number.
             * A timeout occurs and the frame will be re-sent.
             */
            LOG_WARNING (
               PF_ALARM_LOG,
               "Alarm(%d): Wrong ACK sequence number. Received %u but it "
               "should be %u. Ignore.\n",
               __LINE__,
               p_fixed->ack_seq_nbr,
               p_apmx->send_seq_count);
            ret = 0;
         }
      }
      else
      {
         /*
          * Ignore!
          * A timeout occurred and the frame has already been freed.
          */
         LOG_DEBUG (
            PF_ALARM_LOG,
            "Alarm(%d): We are not waiting for transport ack, so do not clean "
            "alarm frame output buffer.\n",
            __LINE__);
         ret = 0;
      }
   }
   else
   {
      /*
       * Ignore!
       * This frame is not interesting.
       */
      ret = 0;
   }

   return ret;
}

/**
 * @internal
 * Create and send a Profinet alarm frame to the IO-controller.
 *
 * Different frame variants:
 *  DATA with alarm notification
 *  DATA with ACK
 *  ERR
 *  NACK
 *  ACK (used as transport ACK = TACK)
 *
 * Messages with TACK == true are saved in the APMX instance in order
 * to handle re-transmissions.
 *
 * APMS: A_Data_req  (This is LMPM_A_Data.req via macro)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           InOut: The APMX instance.
 * @param p_fixed          In:    The fixed part of the RTA-PDU. PDU type,
 *                                TACK flag, sequence counters etc.
 * @param p_alarm_data     In:    Alarm type, slot, subslot etc.
 *                                Mandatory for DATA messages, otherwise NULL.
 *                                The payload part of it (if any) is not used.
 * @param maint_status     In:    The Maintenance status used for specific USI
 *                                values (inserted only if not zero).
 * @param payload_usi      In:    The payload USI. Only used for
 *                                block type = alarm notify.
 *                                Use 0 for no payload.
 * @param payload_len      In:    Number of bytes of manufaturer data.
 *                                Only used for manufacturer data ("USI format",
 *                                payload USI <= 0x7FFF).
 *                                May be 0 for manufacturer data.
 * @param p_payload        In:    pf_diag_item_t, manufacturer data or NULL.
 *                                Mandatory if payload_len > 0 or for USI
 *                                values for diagnosis in standard format
 *                                (which expect a pf_diag_item_t)
 * @param p_pnio_status    In:    Mandatory for ERROR messages.
 *                                For DATA messages a NULL value gives an Alarm
 *                                Notification DATA message, otherwise an
 *                                Alarm Ack DATA message.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apms_a_data_req (
   pnet_t * net,
   pf_apmx_t * p_apmx,
   const pf_alarm_fixed_t * p_fixed,
   const pf_alarm_data_t * p_alarm_data,
   uint32_t maint_status,
   uint16_t payload_usi,
   uint16_t payload_len,
   const uint8_t * p_payload,
   const pnet_pnio_status_t * p_pnio_status)
{
   int ret = -1;
   uint16_t var_part_len_pos;
   uint16_t var_part_len;
   pnal_buf_t * p_rta;
   uint8_t * p_buf = NULL;
   uint16_t pos = 0;
   const pnet_ethaddr_t * mac_address = pf_cmina_get_device_macaddr (net);
   uint16_t u16 = 0;

   if (p_apmx->p_ar->alarm_cr_request.alarm_cr_properties.transport_udp == true)
   {
      /* ToDo: Create UDP frames */
   }
   else
   {
      p_rta = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
      if (p_rta == NULL)
      {
         LOG_ERROR (
            PF_ALARM_LOG,
            "Alarm(%d): Failed to allocate output buffer for alarm",
            __LINE__);
      }
      else
      {
         p_buf = p_rta->payload;
         if (p_buf == NULL)
         {
            LOG_ERROR (
               PF_ALARM_LOG,
               "Alarm(%d): Failed to allocate payload output buffer for "
               "alarm.\n",
               __LINE__);
         }
         else
         {
            LOG_DEBUG (
               PF_AL_BUF_LOG,
               "Alarm(%d): Allocated alarm output buffer for %s %p\n",
               __LINE__,
               pf_alarm_pdu_type_to_string (p_fixed->pdu_type.type),
               p_rta);

            /* Insert destination MAC address */
            pf_put_mem (
               &p_apmx->da,
               sizeof (p_apmx->da),
               PF_FRAME_BUFFER_SIZE,
               p_buf,
               &pos);

            /* Insert source MAC address (our interface MAC address) */
            memcpy (&p_buf[pos], mac_address->addr, sizeof (pnet_ethaddr_t));
            pos += sizeof (pnet_ethaddr_t);

            /* Insert VLAN Tag protocol identifier (TPID) */
            pf_put_uint16 (
               true,
               PNAL_ETHTYPE_VLAN,
               PF_FRAME_BUFFER_SIZE,
               p_buf,
               &pos);

            /* Insert VLAN prio (and VLAN ID=0) */
            u16 = (p_apmx->vlan_prio & 0x0007) << 13; /* Three leftmost bits */
            pf_put_uint16 (true, u16, PF_FRAME_BUFFER_SIZE, p_buf, &pos);

            /* Insert EtherType */
            pf_put_uint16 (
               true,
               PNAL_ETHTYPE_PROFINET,
               PF_FRAME_BUFFER_SIZE,
               p_buf,
               &pos);

            /* Insert Profinet frame ID (first part of Ethernet frame payload)
             */
            pf_put_uint16 (
               true,
               p_apmx->frame_id,
               PF_FRAME_BUFFER_SIZE,
               p_buf,
               &pos);

            /* Insert alarm specific data */
            pf_put_alarm_fixed (
               true,
               p_fixed,
               PF_FRAME_BUFFER_SIZE,
               p_buf,
               &pos);

            var_part_len_pos = pos;
            /* var_part_len is not yet known */
            pf_put_uint16 (true, 0, PF_FRAME_BUFFER_SIZE, p_buf, &pos);

            /* Insert variable part of alarm payload for DATA and ERR */
            if (p_fixed->pdu_type.type == PF_RTA_PDU_TYPE_DATA)
            {
               if (p_pnio_status != NULL)
               {
                  /* Build an Alarm Ack DATA message */
                  pf_put_alarm_block (
                     true,
                     p_apmx->block_type_alarm_ack,
                     p_alarm_data,
                     maint_status,
                     0,    /* No payload */
                     0,    /* No payload */
                     NULL, /* No payload */
                     p_pnio_status,
                     PF_FRAME_BUFFER_SIZE,
                     p_buf,
                     &pos);
               }
               else
               {
                  /* Build an Alarm Notification DATA message */
                  pf_put_alarm_block (
                     true,
                     p_apmx->block_type_alarm_notify,
                     p_alarm_data,
                     maint_status,
                     payload_usi,
                     payload_len,
                     p_payload,
                     NULL, /* No PNIO status */
                     PF_FRAME_BUFFER_SIZE,
                     p_buf,
                     &pos);
               }
            }
            else if (p_fixed->pdu_type.type == PF_RTA_PDU_TYPE_ERR)
            {
               CC_ASSERT (p_pnio_status != NULL);
               pf_put_pnet_status (
                  true,
                  p_pnio_status,
                  PF_FRAME_BUFFER_SIZE,
                  p_buf,
                  &pos);
            }
            else
            {
               /* NACK or ACK does not take any var part. */
            }

            /* Finally insert the correct VarPartLen */
            var_part_len = pos - (var_part_len_pos + sizeof (var_part_len));
            pf_put_uint16 (
               true,
               var_part_len,
               PF_FRAME_BUFFER_SIZE,
               p_buf,
               &var_part_len_pos);

            p_rta->len = pos;

            LOG_INFO (
               PF_ALARM_LOG,
               "Alarm(%d): Send an alarm frame %s %p Frame ID: 0x%04X  "
               "Alarmtype: %u  Tack: %u  USI: 0x%04X  Payload len: %u  "
               "Send seq: %u  ACK seq: %u\n",
               __LINE__,
               pf_alarm_pdu_type_to_string (p_fixed->pdu_type.type),
               p_rta,
               p_apmx->frame_id,
               (p_alarm_data != NULL) ? p_alarm_data->alarm_type : 0,
               p_fixed->add_flags.tack,
               payload_usi,
               payload_len,
               p_fixed->send_seq_num,
               p_fixed->ack_seq_nbr);

            if (pf_eth_send_on_management_port (net, p_rta) <= 0)
            {
               LOG_ERROR (
                  PF_ALARM_LOG,
                  "Alarm(%d): Error from pnal_eth_send(rta)\n",
                  __LINE__);
            }
            else
            {
               ret = 0;
            }
         }

         if (p_fixed->add_flags.tack == true)
         {
            if (p_apmx->p_rta == NULL)
            {
               LOG_DEBUG (
                  PF_AL_BUF_LOG,
                  "Alarm(%d): Save alarm output buffer for later use. %p\n",
                  __LINE__,
                  p_rta);
               p_apmx->p_rta = p_rta;
            }
            else
            {
               LOG_ERROR (
                  PF_ALARM_LOG,
                  "Alarm(%d): RTA alarm output buffer with TACK lost! "
                  "Free %p  Existing buffer %p\n",
                  __LINE__,
                  p_rta,
                  p_apmx->p_rta);
               pnal_buf_free (p_rta);
            }
         }
         else
         {
            /* ACK, NAK and ERR buffers are not saved for retransmission. */
            LOG_DEBUG (
               PF_AL_BUF_LOG,
               "Alarm(%d): Free unsaved alarm output buffer %p\n",
               __LINE__,
               p_rta);
            pnal_buf_free (p_rta);
         }
      }
   }

   return ret;
}

/**
 * @internal
 * Create and send an DATA RTA-PDU
 *
 * This is done by APMS, and is triggered by ALPMI or ALPMR.
 *
 * The message will have TACK == true, and will be scheduled for retransmission.
 * It uses pf_alarm_apms_a_data_req() for the sending.
 *
 * APMS: APMS_A_Data.req
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           InOut: The APMS instance. Select high or low prio by
 *                                using the correct instance.
 * @param p_alarm_data     In:    Alarm details (Alarm type, slot, subslot,
 *                                possibly payload etc)
 * @param maint_status     In:    The Maintenance status used for specific USI
 *                                values (inserted only if not zero).
 * @param p_pnio_status    In:    Optional. Detailed error information (or NULL)
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apms_apms_a_data_req (
   pnet_t * net,
   pf_apmx_t * p_apmx,
   const pf_alarm_data_t * p_alarm_data,
   uint32_t maint_status,
   const pnet_pnio_status_t * p_pnio_status)
{
   int ret = -1;
   pf_alarm_fixed_t fixed;

   if (p_apmx->apms_state != PF_APMS_STATE_OPEN)
   {
      /* TODO: The standard says err_cls = PNET_ERROR_CODE_1_APMS
                                 err_code = PNET_ERROR_CODE_2_APMS_WRONG_STATE
       */
      pf_alarm_alpmi_apms_a_data_cnf (net, p_apmx, -1);
      pf_alarm_alpmr_apms_a_data_cnf (net, p_apmx, -1);
   }
   else
   {
      memset (&fixed, 0, sizeof (fixed));
      fixed.src_ref = p_apmx->src_ref;
      fixed.dst_ref = p_apmx->dst_ref;
      fixed.pdu_type.type = PF_RTA_PDU_TYPE_DATA;
      fixed.pdu_type.version = PF_ALARM_PDU_TYPE_VERSION_1;
      fixed.add_flags.window_size = 1;
      fixed.add_flags.tack = true;
      fixed.send_seq_num = p_apmx->send_seq_count;
      fixed.ack_seq_nbr = p_apmx->exp_seq_count_o;

      p_apmx->resend_counter = p_apmx->p_ar->alarm_cr_request.rta_retries;

      ret = pf_alarm_apms_a_data_req (
         net,
         p_apmx,
         &fixed,
         p_alarm_data,
         maint_status,
         p_alarm_data->payload.usi,
         p_alarm_data->payload.len,
         p_alarm_data->payload.data,
         p_pnio_status);

      p_apmx->apms_state = PF_APMS_STATE_WTACK;

      /* APMS: A_Data_cnf     (tells that incoming message is acyclic data)
         In state PF_APMS_STATE_WTACK with A_Data_cnf we should start
         retransmission timer */
      if (
         pf_scheduler_add (
            net,
            p_apmx->timeout_us,
            pf_alarm_apms_timeout,
            p_apmx,
            &p_apmx->resend_timeout) != 0)
      {
         LOG_ERROR (
            PF_ALARM_LOG,
            "Alarm(%d): Failed to schedule sending of alarm frame\\n",
            __LINE__);
         ret = -1;
      }
   }

   return ret;
}

/**
 * @internal
 * Close APMX (i.e. close APMS and APMR, for both high and low prio alarms)
 *
 * Sends a low prio alarm.
 * Unregisters frame handlers.
 *
 * APMS: APMS_Close_req
 * APMS: APMS_Close_cnf via return value and err_cls/err_code
 * APMR: APMR_Close_req
 * APMR: APMR_Close_cnf(+)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param err_code         In:    Error code. See PNET_ERROR_CODE_2_*
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmx_close (pnet_t * net, pf_ar_t * p_ar, uint8_t err_code)
{
   uint16_t ix;
   pnet_pnio_status_t pnio_status;
   pnal_buf_t * p_rta;
   pf_alarm_fixed_t fixed;

   /* Send CLOSE alarm first, with low prio by using p_ar->apmx[0] */
   if (p_ar->apmx[0].apms_state != PF_APMS_STATE_CLOSED)
   {
      pnio_status.error_code = PNET_ERROR_CODE_RTA_ERROR;
      pnio_status.error_decode = PNET_ERROR_DECODE_PNIO;
      pnio_status.error_code_1 = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
      pnio_status.error_code_2 = err_code;

      memset (&fixed, 0, sizeof (fixed));
      fixed.src_ref = p_ar->apmx[0].src_ref;
      fixed.dst_ref = p_ar->apmx[0].dst_ref;
      fixed.pdu_type.type = PF_RTA_PDU_TYPE_ERR;
      fixed.pdu_type.version = PF_ALARM_PDU_TYPE_VERSION_1;
      fixed.add_flags.window_size = 1;
      fixed.add_flags.tack = false;
      fixed.send_seq_num = p_ar->apmx[0].send_seq_count;
      fixed.ack_seq_nbr = p_ar->apmx[0].exp_seq_count_o;

      if (
         pf_alarm_apms_a_data_req (
            net,
            &p_ar->apmx[0],
            &fixed,
            NULL, /* No alarm_data */
            0,
            0,
            0,
            NULL, /* No payload */
            &pnio_status) != 0)
      {
         LOG_ERROR (
            PF_ALARM_LOG,
            "alarm(%d): Could not send close alarm\n",
            __LINE__);
      }
   }

   /* Mark the alarm endpoint as inactive. */
   for (ix = 0; ix < PNET_MAX_AR; ++ix)
   {
      if (net->alarm_endpoint[ix].ar == p_ar)
      {
         net->alarm_endpoint[ix].active = false;
         break;
      }
   }
   /* Remove the frame handler if no alarm endpoint is active at this point. */
   for (ix = 0; (ix < PNET_MAX_AR) && (net->alarm_endpoint[ix].active == false);
        ++ix)
      ;
   if (ix >= PNET_MAX_AR)
   {
      pf_eth_frame_id_map_remove (net, PF_FRAME_ID_ALARM_HIGH);
      pf_eth_frame_id_map_remove (net, PF_FRAME_ID_ALARM_LOW);
   }

   for (ix = 0; ix < NELEMENTS (p_ar->apmx); ix++)
   {
      /* Close APMS */
      if (p_ar->apmx[ix].apms_state != PF_APMS_STATE_CLOSED)
      {
         /* Free resources */
         /* StopTimer */
         pf_scheduler_remove_if_running (net, &p_ar->apmx[ix].resend_timeout);

         p_ar->apmx[ix].p_ar = NULL;
         p_ar->apmx[ix].apms_state = PF_APMS_STATE_CLOSED;
      }

      LOG_DEBUG (
         PF_AL_BUF_LOG,
         "Alarm(%d): Free saved %s prio alarm output buffer %p for AREP %u if "
         "necessary\n",
         __LINE__,
         p_ar->apmx[ix].high_priority ? "high" : "low",
         p_ar->apmx[ix].p_rta,
         p_ar->arep);
      if (p_ar->apmx[ix].p_rta != NULL)
      {
         p_rta = p_ar->apmx[ix].p_rta;
         p_ar->apmx[ix].p_rta = NULL;
         pnal_buf_free (p_rta);
      }

      /* Close APMR */
      if (p_ar->apmx[ix].apmr_state != PF_APMR_STATE_CLOSED)
      {
         p_ar->apmx[ix].apmr_state = PF_APMR_STATE_CLOSED;

         pf_alarm_receive_queue_reset (&p_ar->apmx[ix].alarm_receive_q);
         pf_alarm_queue_mutex_destroy (
            &p_ar->apmx[ix].alarm_receive_q.accountant);
      }
   }

   return 0;
}

/**
 * @internal
 * Send alarm APMR ACK (a TACK = transport acknowledge)
 *
 * It uses APMS  pf_alarm_apms_a_data_req() for the sending.
 *
 * APMR: A_Data_req  (Implements parts of it. This ia LMPM_A_Data.req via macro)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           InOut: The APMX instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmr_send_ack (pnet_t * net, pf_apmx_t * p_apmx)
{
   int ret = -1;
   pf_alarm_fixed_t fixed;

   memset (&fixed, 0, sizeof (fixed));
   fixed.src_ref = p_apmx->src_ref;
   fixed.dst_ref = p_apmx->dst_ref;
   fixed.pdu_type.type = PF_RTA_PDU_TYPE_ACK;
   fixed.pdu_type.version = PF_ALARM_PDU_TYPE_VERSION_1;
   fixed.add_flags.window_size = 1;
   fixed.add_flags.tack = false;
   fixed.send_seq_num = p_apmx->send_seq_count_o;
   fixed.ack_seq_nbr = p_apmx->exp_seq_count_o;

   ret = pf_alarm_apms_a_data_req (
      net,
      p_apmx,
      &fixed,
      NULL, /* No additional data for ACK message */
      0,
      0,
      0,
      NULL, /* No payload for ACK message */
      NULL);

   return ret;
}

/**
 * @internal
 * Send alarm APMR NACK
 *
 * It uses APMS  pf_alarm_apms_a_data_req() for the sending.
 *
 * APMR: A_Data_req  (Implements parts of it. This ia LMPM_A_Data.req via macro)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           InOut: The APMX instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmr_send_nack (pnet_t * net, pf_apmx_t * p_apmx)
{
   int ret = -1;
   pf_alarm_fixed_t fixed;

   memset (&fixed, 0, sizeof (fixed));
   fixed.src_ref = p_apmx->src_ref;
   fixed.dst_ref = p_apmx->dst_ref;
   fixed.pdu_type.type = PF_RTA_PDU_TYPE_NACK;
   fixed.pdu_type.version = PF_ALARM_PDU_TYPE_VERSION_1;
   fixed.add_flags.window_size = 1;
   fixed.add_flags.tack = false;
   fixed.send_seq_num = p_apmx->send_seq_count_o;
   fixed.ack_seq_nbr = p_apmx->exp_seq_count_o;

   ret = pf_alarm_apms_a_data_req (
      net,
      p_apmx,
      &fixed,
      NULL, /* No additional data for NACK message */
      0,
      0,
      0,
      NULL, /* No payload for NACK message */
      NULL);

   return ret;
}

/**
 * @internal
 * Handle reception of an RTA DATA PDU in APMR.
 *
 * Sends back ACK or NACK.
 * Uses APMS to handle ack_seq_num in incoming messages.
 *
 * APMR: A_Data_ind   (Implements part of it. This ia LMPM_A_Data.ind via macro)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           InOut: The APMX instance.
 * @param p_fixed          In:    The fixed part of the RTA-DPU.
 * @param data_len         In:    Length of the variable part of the RTA-PDU.
 * @param p_get_info       InOut: ?
 * @param pos              In:    Position of ?
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmr_a_data_ind (
   pnet_t * net,
   pf_apmx_t * p_apmx,
   const pf_alarm_fixed_t * p_fixed,
   uint16_t data_len,
   pf_get_info_t * p_get_info,
   uint16_t pos)
{
   int ret = -1;
   uint16_t start_pos = pos;
   pf_block_header_t block_header;
   pf_alarm_data_t alarm_data;
   pnet_pnio_status_t pnio_status;
   uint16_t data_usi;
   const uint8_t * p_data;

   switch (p_apmx->apmr_state)
   {
   case PF_APMR_STATE_CLOSED:
      /* Ignore */
      ret = 0;
      break;
   case PF_APMR_STATE_OPEN:
      if (p_fixed->add_flags.tack == false)
      {
         /* Ignore */
         ret = 0;
      }
      else
      {
         if (p_fixed->send_seq_num == p_apmx->exp_seq_count)
         {
            pf_get_block_header (p_get_info, &pos, &block_header);
            if (block_header.block_type == p_apmx->block_type_alarm_ack)
            {
               /* This message is an Alarm ACK PDU. Parse it and deliver to
                * ALPMI. */
               LOG_DEBUG (
                  PF_ALARM_LOG,
                  "Alarm(%d): The incoming Alarm DATA message is an ACK PDU. "
                  "Send seq: %u Trigger a TACK response.\n",
                  __LINE__,
                  p_fixed->send_seq_num);

               /* Tell APMS to handle ack_seq_num in message */
               (void)pf_alarm_apms_a_data_ind (net, p_apmx, p_fixed);

               p_apmx->exp_seq_count_o = p_apmx->exp_seq_count;
               p_apmx->exp_seq_count = (p_apmx->exp_seq_count + 1) & 0x7fff;

               ret = pf_alarm_apmr_send_ack (net, p_apmx);
               if (ret == 0)
               {
                  p_apmx->apmr_state = PF_APMR_STATE_WCNF;

                  /* APMR: A_Data_cnf is triggered here*/

                  pf_get_pnio_status (p_get_info, &pos, &pnio_status);
                  ret = pf_alarm_alpmi_apmr_a_data_ind (
                     net,
                     p_apmx,
                     &pnio_status); /* APMR: APMR_A_Data.ind */
                  if (ret == 0)
                  {
                     p_apmx->apmr_state = PF_APMR_STATE_OPEN;
                  }
               }
            }
            else if (block_header.block_type == p_apmx->block_type_alarm_notify)
            {
               /* This message is an Alarm Notification PDU. Parse it and
                * deliver to ALPMR. */
               LOG_DEBUG (
                  PF_ALARM_LOG,
                  "Alarm(%d): The incoming Alarm DATA message is an alarm "
                  "notification PDU. Send seq: %u Trigger a TACK response.\n",
                  __LINE__,
                  p_fixed->send_seq_num);

               /* Tell APMS to handle ack_seq_num in message */
               (void)pf_alarm_apms_a_data_ind (net, p_apmx, p_fixed);

               p_apmx->exp_seq_count_o = p_apmx->exp_seq_count;
               p_apmx->exp_seq_count = (p_apmx->exp_seq_count + 1) & 0x7fff;

               ret = pf_alarm_apmr_send_ack (net, p_apmx);
               if (ret == 0)
               {
                  p_apmx->apmr_state = PF_APMR_STATE_WCNF;

                  /* APMR: A_Data_cnf is triggered here*/

                  pf_get_alarm_data (p_get_info, &pos, &alarm_data);
                  data_usi = pf_get_uint16 (p_get_info, &pos);
                  if (p_get_info->result == PF_PARSE_OK)
                  {
                     data_len -= (pos - start_pos);
                     p_data = &p_get_info->p_buf[pos];
                     ret = pf_alarm_alpmr_apmr_a_data_ind (
                        net,
                        p_apmx,
                        p_fixed,
                        &alarm_data,
                        data_len,
                        data_usi,
                        p_data); /* APMR: APMR_A_Data.ind */
                  }
                  else
                  {
                     LOG_ERROR (
                        PF_ALARM_LOG,
                        "Alarm(%d): Error parsing Alarm notification\n",
                        __LINE__);
                     ret = -1;
                  }
                  if (ret == 0)
                  {
                     p_apmx->apmr_state = PF_APMR_STATE_OPEN;
                  }
               }
            }
            else
            {
               LOG_ERROR (
                  PF_ALARM_LOG,
                  "Alarm(%d): Unhandled incoming alarm block type 0x%04x\n",
                  __LINE__,
                  block_header.block_type);
               /* The standard says PNET_ERROR_CODE_1_ALPMR,
                * PNET_ERROR_CODE_2_ALPMR_WRONG_ALARM_PDU internally */
               p_apmx->p_ar->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
               p_apmx->p_ar->err_code =
                  PNET_ERROR_CODE_2_ABORT_AR_ALARM_IND_ERROR;
               pf_alarm_error_ind (
                  net,
                  p_apmx,
                  p_apmx->p_ar->err_cls,
                  p_apmx->p_ar->err_code);
               ret = -1;
            }
         }
         else if (p_fixed->send_seq_num == p_apmx->exp_seq_count_o)
         {
            /* Tell APMS to handle ack_seq_num in message */
            (void)pf_alarm_apms_a_data_ind (net, p_apmx, p_fixed);

            /* Re-send the ACK */
            (void)pf_alarm_apmr_send_ack (net, p_apmx);
            ret = 0;
         }
         else
         {
            p_apmx->p_ar->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
            p_apmx->p_ar->err_code = PNET_ERROR_CODE_2_ABORT_CODE_SEQ;
            pnio_status.error_code = PNET_ERROR_CODE_PNIO;
            pnio_status.error_decode = PNET_ERROR_DECODE_PNIO;
            pnio_status.error_code_1 = p_apmx->p_ar->err_cls;
            pnio_status.error_code_2 = p_apmx->p_ar->err_code;
            pf_fspm_create_log_book_entry (
               net,
               p_apmx->p_ar->arep,
               &pnio_status,
               __LINE__);

            (void)pf_alarm_apmr_send_nack (net, p_apmx);
            ret = 0;
         }
      }
      break;
   case PF_APMR_STATE_WCNF:
      /* WCNF is a virtual state in the case above */
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): This should never happen!\n",
         __LINE__);
      break;
   default:
      /* Ignore */
      ret = 0;
      break;
   }

   return ret;
}

/**
 * @internal
 * Return the alarm specifier and maintenance status for a specific sub-slot,
 * by looking at all diagnosis items found for that subslot.
 *
 * Also includes the "current diag item" in the analysis.
 *
 * Describes diagnosis alarms.
 *
 * This corresponds to the "BuildSummarizedAlarmSpecifier" and
 * "BuildSummarizedMaintenanceStatus" functions mentioned in the standard.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param subslot_nbr      In:    The sub-slot number.
 * @param p_diag_item      In:    The current diag item (may be NULL).
 * @param p_alarm_spec     Out:   The computed alarm specifier.
 * @param p_maint_status   Out:   The computed maintenance status.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_get_diag_summary (
   pnet_t * net,
   const pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   const pf_diag_item_t * p_diag_item,
   pnet_alarm_spec_t * p_alarm_spec,
   uint32_t * p_maint_status)
{
   pf_device_t * p_dev = NULL;
   pf_subslot_t * p_subslot = NULL;
   uint16_t item_ix;
   pf_diag_item_t * p_item = NULL;

   if (pf_cmdev_get_device (net, &p_dev) == 0)
   {
      memset (p_alarm_spec, 0, sizeof (*p_alarm_spec));
      *p_maint_status = 0;

      /* Consider only alarms on this sub-slot. */
      if (
         pf_cmdev_get_subslot_full (
            net,
            api_id,
            slot_nbr,
            subslot_nbr,
            &p_subslot) == 0)
      {
         /* Collect all diags into maintenanceStatus*/

         /* First handle the "current" (unreported) item. */
         if (p_diag_item != NULL)
         {
            pf_alarm_add_diag_item_to_summary (
               p_ar,
               p_subslot,
               p_diag_item,
               p_alarm_spec,
               p_maint_status);
         }

         /* Now handle all the already reported diag items. */
         item_ix = p_subslot->diag_list;
         pf_cmdev_get_diag_item (net, item_ix, &p_item);
         while (p_item != NULL)
         {
            /* Collect all diags into maintenanceStatus*/
            pf_alarm_add_diag_item_to_summary (
               p_ar,
               p_subslot,
               p_item,
               p_alarm_spec,
               p_maint_status);

            item_ix = p_item->next;
            pf_cmdev_get_diag_item (net, item_ix, &p_item);
         }
      }
   }

   return 0;
}

/**
 * @internal
 * Handle the reception of Alarm messages for one AR instance.
 *
 * This function reads alarm frames from both input queues of the AR.
 *
 * The message is normally processed in APMR/ALPMR, and indications may be sent
 * to the application. Repeated for both low and high prio queue.
 *
 * Incoming ACK is sent to APMS.
 *
 * APMR + APMS: Implementation detail
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmr_periodic (pnet_t * net, pf_ar_t * p_ar)
{
   int ret = 0;
   pf_apmx_t * p_apmx;
   uint16_t ix;
   pnal_buf_t * p_buf;
   pf_alarm_fixed_t fixed;
   uint16_t var_part_len;
   pnet_pnio_status_t pnio_status;
   pf_get_info_t get_info;
   uint16_t pos;
   pf_apmr_msg_t inputmsg;

   /*
    * Periodic is run cyclically. It may run for some time.
    * If anything happens to the APMR (e.g. it is closed) during this time
    * then we need to stop handling this APMR!
    * Otherwise there will be lots of NULL pointers.
    */
   for (ix = 0; ((ret == 0) && (ix < NELEMENTS (p_ar->apmx))); ix++)
   {
      p_apmx = &p_ar->apmx[ix];
      p_buf = NULL;
      while (
         (ret == 0) && (p_apmx->apmr_state != PF_APMR_STATE_CLOSED) &&
         (pf_alarm_receive_queue_fetch (&p_apmx->alarm_receive_q, &inputmsg) ==
          0))
      {
         /* Got something - extract! */
         p_buf = inputmsg.p_buf;

         /* Skip frame_id */
         pos = inputmsg.frame_id_pos + sizeof (uint16_t);

         get_info.result = PF_PARSE_OK;
         get_info.is_big_endian = true;
         get_info.p_buf = (uint8_t *)p_buf->payload;
         get_info.len = p_buf->len;

         /* Parse fixed part of incoming alarm frame */
         memset (&fixed, 0, sizeof (fixed));
         pf_get_alarm_fixed (&get_info, &pos, &fixed);
         var_part_len = pf_get_uint16 (&get_info, &pos);

         if (fixed.pdu_type.version == 1)
         {
            switch (fixed.pdu_type.type)
            {
            case PF_RTA_PDU_TYPE_ACK:
               LOG_DEBUG (
                  PF_ALARM_LOG,
                  "Alarm(%d): Got %s prio alarm ACK frame from queue. %p\n",
                  __LINE__,
                  p_apmx->high_priority ? "high" : "low",
                  p_buf);
               if (var_part_len == 0)
               {
                  /* Tell APMS to check for and handle ACK */
                  (void)pf_alarm_apms_a_data_ind (net, p_apmx, &fixed);
               }
               else
               {
                  LOG_ERROR (
                     PF_ALARM_LOG,
                     "Alarm(%d): Wrong var_part_len %u for incoming ACK "
                     "frame\n",
                     __LINE__,
                     (unsigned)var_part_len);
                  ret = 0; /* Just ignore */
               }
               break;
            case PF_RTA_PDU_TYPE_NACK:
               LOG_DEBUG (
                  PF_ALARM_LOG,
                  "Alarm(%d): Got %s prio alarm NACK frame from queue. %p\n",
                  __LINE__,
                  p_apmx->high_priority ? "high" : "low",
                  p_buf);
               /* APMS: A_Data_ind  (Implements parts of it, for NACK) */
               if (var_part_len == 0)
               {
                  /* Ignore */
                  ret = 0;
               }
               else
               {
                  LOG_ERROR (
                     PF_ALARM_LOG,
                     "Alarm(%d): Wrong var_part_len %u for incoming "
                     "NACK frame\n",
                     __LINE__,
                     (unsigned)var_part_len);
                  ret = 0; /* Just ignore */
               }
               break;
            case PF_RTA_PDU_TYPE_DATA:
               LOG_DEBUG (
                  PF_ALARM_LOG,
                  "Alarm(%d): Got incoming %s prio alarm DATA frame from "
                  "queue. %p. "
                  "Length on wire: %d  Var part len: %d\n",
                  __LINE__,
                  p_apmx->high_priority ? "high" : "low",
                  p_buf,
                  p_buf->len,
                  var_part_len);
               ret = pf_alarm_apmr_a_data_ind (
                  net,
                  p_apmx,
                  &fixed,
                  var_part_len,
                  &get_info,
                  pos);
               break;
            case PF_RTA_PDU_TYPE_ERR:
               if (var_part_len == 4) /* sizeof(pf_pnio_status_t) */
               {
                  /* APMR: A_Data_ind  (Implements parts of it, for ERR) */
                  /* APMS: A_Data_ind  (Implements parts of it, for ERR) */
                  pf_get_pnio_status (&get_info, &pos, &pnio_status);

                  /* Should we also be able to receive ERR frames to APMS?
                   * Should its state have an effect of the result? */
                  switch (p_apmx->apmr_state)
                  {
                  case PF_APMR_STATE_OPEN:
                     LOG_DEBUG (
                        PF_ALARM_LOG,
                        "Alarm(%d): Got incoming alarm ERROR frame from "
                        "queue. %p. Error code 1 (ERRCLS): %0x02x  "
                        "Error code 2 (ERRCODE): %0x02x\n",
                        __LINE__,
                        p_buf,
                        pnio_status.error_code_1,
                        pnio_status.error_code_2);
                     pf_alarm_error_ind (
                        net,
                        p_apmx,
                        pnio_status.error_code_1,
                        pnio_status.error_code_2);
                     break;
                  default:
                     LOG_ERROR (
                        PF_ALARM_LOG,
                        "Alarm(%d): Alarm received from IO-controller, but "
                        "the APMR state is %s\n",
                        __LINE__,
                        pf_alarm_apmr_state_to_string (p_apmx->apmr_state));
                     pf_alarm_error_ind (
                        net,
                        p_apmx,
                        PNET_ERROR_CODE_1_APMR,
                        PNET_ERROR_CODE_2_APMR_INVALID_STATE);
                     break;
                  }

                  ret = 0;
               }
               else
               {
                  LOG_ERROR (
                     PF_ALARM_LOG,
                     "Alarm(%d): Alarm ERROR frame received from "
                     "IO-controller, but it has wrong var_part_len %u\n",
                     __LINE__,
                     (unsigned)var_part_len);
                  /* Ignore */
                  ret = 0;
               }
               break;
            default:
               LOG_ERROR (
                  PF_ALARM_LOG,
                  "Alarm(%d): Alarm received from IO-controller, but it "
                  "has wrong PDU-Type.type %u\n",
                  __LINE__,
                  (unsigned)fixed.pdu_type.type);
               /* Ignore */
               ret = 0;
               break;
            }
         }
         else
         {
            LOG_ERROR (
               PF_ALARM_LOG,
               "Alarm(%d):  Alarm received from IO-controller, but it "
               "has wrong PDU-Type.version %u\n",
               __LINE__,
               (unsigned)fixed.pdu_type.version);
            /* Ignore */
            ret = 0;
         }

         LOG_DEBUG (
            PF_AL_BUF_LOG,
            "Alarm(%d): Free received buffer %p\n",
            __LINE__,
            p_buf);
         pnal_buf_free (p_buf);
         p_buf = NULL;
      }
   }

   return ret;
}

/**
 * Send alarm
 * - Update diag summary
 * - Update sequence numbers
 * - Hand over to alarm to APMS instance
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_apmx           InOut: The APMX instance.
 * @param alarm_data       In:    Alarm details (Alarm type, slot, subslot,
 *                                possibly payload etc)
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_send_internal (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_apmx_t * p_apmx,
   pf_alarm_data_t * alarm_data)
{
   int ret = -1;
   uint32_t maint_status = 0;

   CC_ASSERT (net->global_alarm_enable == true);
   CC_ASSERT (p_ar->alarm_enable == true);
   CC_ASSERT (p_apmx->p_alpmx->alpmi_state == PF_ALPMI_STATE_W_ALARM);

   (void)pf_alarm_get_diag_summary (
      net,
      p_ar,
      alarm_data->api_id,
      alarm_data->slot_nbr,
      alarm_data->subslot_nbr,
      NULL,
      &alarm_data->alarm_specifier,
      &maint_status);

   /* Calculate sequence numbers */
   alarm_data->sequence_number = p_apmx->p_alpmx->sequence_number;
   p_apmx->p_alpmx->prev_sequence_number = p_apmx->p_alpmx->sequence_number;
   p_apmx->p_alpmx->sequence_number++;
   p_apmx->p_alpmx->sequence_number %= 0x0800;

   /* Create and send an DATA RTA-PDU */
   ret = pf_alarm_apms_apms_a_data_req (
      net,
      p_apmx,
      alarm_data,
      maint_status,
      NULL);

   if (ret == 0)
   {
      p_apmx->p_alpmx->alpmi_state = PF_ALPMI_STATE_W_ACK;
   }
   else
   {
      LOG_ERROR (PF_ALARM_LOG, "Alarm(%d): Failed to send alarm\n", __LINE__);
   }

   return ret;
}

/************************ Queue handling ************************************/

/**
 * @internal
 * Create a mutex for a queue.
 *
 * Note that on first usage, the pf_queue_accountant_t must be fully cleared.
 *
 * @param p_accountant     InOut: Queue accountant
 */
void pf_alarm_queue_mutex_create (pf_queue_accountant_t * p_accountant)
{
   if (p_accountant->mutex == NULL)
   {
      p_accountant->mutex = os_mutex_create();
   }
}

/**
 * @internal
 * Destroy a mutex for a queue.
 *
 * @param p_accountant     InOut: Queue accountant
 */
void pf_alarm_queue_mutex_destroy (pf_queue_accountant_t * p_accountant)
{
   if (p_accountant->mutex != NULL)
   {
      os_mutex_destroy (p_accountant->mutex);
      p_accountant->mutex = NULL;
   }
}

/**
 * @internal
 * Check if the queue is available (initialized).
 *
 * @param p_accountant     InOut: Queue accountant
 * @return true if the queue is available
 *         false if the queue not is initialized
 */
bool pf_alarm_queue_is_available (pf_queue_accountant_t * p_accountant)
{
   return p_accountant->mutex != NULL;
}

/**
 * @internal
 * Lock a queue.
 *
 * @param p_accountant     InOut: Queue accountant
 */
static void pf_alarm_queue_lock (pf_queue_accountant_t * p_accountant)
{
   CC_ASSERT (p_accountant->mutex != NULL);
   os_mutex_lock (p_accountant->mutex);
}

/**
 * @internal
 * Unlock a queue.
 *
 * @param p_accountant     InOut: Queue accountant
 */
static void pf_alarm_queue_unlock (pf_queue_accountant_t * p_accountant)
{
   CC_ASSERT (p_accountant->mutex != NULL);
   os_mutex_unlock (p_accountant->mutex);
}

/**
 * @internal
 * Reset the queue indexes.
 *
 * NOTE: Remember to lock/unlock the queue before and after this operation.
 *
 * @param p_accountant     InOut: Queue accountant
 */
static void pf_alarm_queue_accountant_reset (
   pf_queue_accountant_t * p_accountant)
{
   p_accountant->count = 0;
   p_accountant->read_index = 0;
   p_accountant->write_index = 0;
}

/**
 * @internal
 * Get next write index for a queue.
 *
 * NOTE: Remember to lock/unlock the queue before and after this operation.
 *
 * @param p_accountant     InOut: Queue accountant
 * @param p_write_index    Out:   Write index to use
 * @return 0 if there is room in the queue
 *         -1 if the queue is full
 */
static int pf_alarm_queue_get_writeindex (
   pf_queue_accountant_t * p_accountant,
   uint16_t * p_write_index)
{
   if (p_accountant->count >= PNET_MAX_ALARMS)
   {
      return -1;
   }

   *p_write_index = p_accountant->write_index;

   p_accountant->write_index++;
   if (p_accountant->write_index >= PNET_MAX_ALARMS)
   {
      p_accountant->write_index = 0;
   }
   p_accountant->count++;

   return 0;
}

/**
 * @internal
 * Get next read index for a queue.
 *
 * NOTE: Remember to lock/unlock the queue before and after this operation.
 *
 * @param p_accountant     InOut: Queue accountant
 * @param p_read_index     Out:   Read index to use
 * @return 0 if the queue has an item to read
 *         -1 if the queue is empty
 */
static int pf_alarm_queue_get_readindex (
   pf_queue_accountant_t * p_accountant,
   uint16_t * p_read_index)
{
   if (p_accountant->count == 0)
   {
      return -1;
   }

   *p_read_index = p_accountant->read_index;

   p_accountant->read_index++;
   if (p_accountant->read_index >= PNET_MAX_ALARMS)
   {
      p_accountant->read_index = 0;
   }
   p_accountant->count--;

   return 0;
}

/**
 * Reset queue for incoming alarm frames. Will free corresponding buffers.
 *
 * Note: The mutex must have been created before.
 *       First time the pf_alarm_receive_queue_t is used it should be fully
 *       cleared. So if this function is used immediately thereafter on the
 *       queue it is a no-op.
 *
 * @param q                InOut: Alarm receive queue
 */
void pf_alarm_receive_queue_reset (pf_alarm_receive_queue_t * q)
{
   uint16_t ix;

   if (pf_alarm_queue_is_available (&q->accountant) == false)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Resetting non-initialized queue.\n",
         __LINE__);

      return;
   }

   pf_alarm_queue_lock (&q->accountant);
   pf_alarm_queue_accountant_reset (&q->accountant);
   for (ix = 0; ix < PNET_MAX_ALARMS; ix++)
   {
      if (q->items[ix].p_buf != NULL)
      {
         pnal_buf_free (q->items[ix].p_buf);
         q->items[ix].p_buf = NULL;
      }
      q->items[ix].frame_id_pos = 0;
   }
   pf_alarm_queue_unlock (&q->accountant);
}

/**
 * Post an alarm frame to the receive queue
 * @param q                InOut: Alarm receive queue
 * @param p_alarm_frame    In:    Alarm frame
 * @return 0  is returned if an alarm frame is posted onto the queue
 *         -1 is returned if queue is non-available or full (Logs an error)
 */
int pf_alarm_receive_queue_post (
   pf_alarm_receive_queue_t * q,
   const pf_apmr_msg_t * p_alarm_frame)
{
   uint16_t write_index;
   int ret = -1;

   if (pf_alarm_queue_is_available (&q->accountant) == false)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Receive queue is not available, alarm frame dropped.\n",
         __LINE__);

      return ret;
   }

   pf_alarm_queue_lock (&q->accountant);
   if (pf_alarm_queue_get_writeindex (&q->accountant, &write_index) == 0)
   {
      q->items[write_index].frame_id_pos = p_alarm_frame->frame_id_pos;
      q->items[write_index].p_buf = p_alarm_frame->p_buf;
      ret = 0;
   }
   pf_alarm_queue_unlock (&q->accountant);

   if (ret != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Receive queue is full, alarm frame dropped.\n",
         __LINE__);
   }

   return ret;
}

/**
 * Fetch an alarm frame from the receive queue
 * @param q                InOut: Alarm receive queue
 * @param p_alarm_frame    Out:   Alarm frame
 * @return 0  is returned if an alarm frame is fetched from the queue
 *         -1 is returned if queue is non-available or empty
 */
int pf_alarm_receive_queue_fetch (
   pf_alarm_receive_queue_t * q,
   pf_apmr_msg_t * p_alarm_frame)
{
   uint16_t read_index;
   int ret = -1;

   if (pf_alarm_queue_is_available (&q->accountant) == false)
   {
      return ret;
   }

   pf_alarm_queue_lock (&q->accountant);
   if (pf_alarm_queue_get_readindex (&q->accountant, &read_index) == 0)
   {
      p_alarm_frame->frame_id_pos = q->items[read_index].frame_id_pos;
      q->items[read_index].frame_id_pos = 0;

      p_alarm_frame->p_buf = q->items[read_index].p_buf;
      q->items[read_index].p_buf = NULL;

      ret = 0;
   }
   pf_alarm_queue_unlock (&q->accountant);

   return ret;
}

/**
 * Reset queue for outgoing alarms
 * @param q                InOut: Alarm send queue (High or low prio)
 */
void pf_alarm_send_queue_reset (pf_alarm_send_queue_t * q)
{
   if (pf_alarm_queue_is_available (&q->accountant) == false)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Resetting non-initialized queue.\n",
         __LINE__);

      return;
   }

   pf_alarm_queue_lock (&q->accountant);
   pf_alarm_queue_accountant_reset (&q->accountant);
   memset (q->items, 0, sizeof (q->items));
   pf_alarm_queue_unlock (&q->accountant);
}

/**
 * Post an alarm to the send queue
 * @param q                InOut: Alarm send queue (High or low prio)
 * @param p_alarm_data     In:    Alarm details (Alarm type, slot, subslot,
 *                                possibly payload etc)
 * @return 0  is returned if an alarm is posted onto the queue
 *         -1 is returned if queue is non-available or full (Logs an error)
 */
int pf_alarm_send_queue_post (
   pf_alarm_send_queue_t * q,
   const pf_alarm_data_t * p_alarm_data)
{
   uint16_t write_index;
   int ret = -1;
   if (pf_alarm_queue_is_available (&q->accountant) == false)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Send queue is not available, alarm message dropped.\n",
         __LINE__);

      return ret;
   }

   pf_alarm_queue_lock (&q->accountant);
   if (pf_alarm_queue_get_writeindex (&q->accountant, &write_index) == 0)
   {
      memcpy (&q->items[write_index], p_alarm_data, sizeof (*p_alarm_data));
      ret = 0;
   }
   pf_alarm_queue_unlock (&q->accountant);

   if (ret != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Send queue is full, alarm message dropped.\n",
         __LINE__);
   }

   return ret;
}

/**
 * Fetch an alarm from the send queue
 * @param q                InOut: Alarm send queue (High or low prio)
 * @param p_alarm_data     Out:   Alarm details (Alarm type, slot, subslot,
 *                                possibly payload etc)
 * @return 0  is returned if an alarm is fetched from the queue
 *         -1 is returned if queue is empty or non-available
 */
int pf_alarm_send_queue_fetch (
   pf_alarm_send_queue_t * q,
   pf_alarm_data_t * p_alarm_data)
{
   uint16_t read_index;
   int ret = -1;

   if (pf_alarm_queue_is_available (&q->accountant) == false)
   {
      return ret;
   }

   pf_alarm_queue_lock (&q->accountant);
   if (pf_alarm_queue_get_readindex (&q->accountant, &read_index) == 0)
   {
      memcpy (p_alarm_data, &q->items[read_index], sizeof (*p_alarm_data));
      ret = 0;
   }
   pf_alarm_queue_unlock (&q->accountant);

   return ret;
}

/****************************************************************************/

/**
 * @internal
 * Handle outgoing alarm messages for one AR instance.
 *
 * When called this function checks the ALMPI state for high and
 * low priority alarms. If state is PF_ALPMI_STATE_W_ALARM and a alarm
 * is found in the send queue this alarm is sent.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static void pf_alarm_almpi_periodic (pnet_t * net, pf_ar_t * p_ar)
{
   int res = 0;
   int16_t prio;
   pf_apmx_t * p_apmx;
   pf_alarm_send_queue_t * q;
   pf_alarm_data_t alarm_data;

   if (p_ar->alarm_enable == true)
   {
      for (prio = 1; prio >= 0; prio--)
      {
         p_apmx = &p_ar->apmx[prio];
         q = &p_ar->alarm_send_q[prio];
         if (p_apmx->p_alpmx->alpmi_state == PF_ALPMI_STATE_W_ALARM)
         {
            res = pf_alarm_send_queue_fetch (q, &alarm_data);
            if (res == 0)
            {
               (void)pf_alarm_send_internal (net, p_ar, p_apmx, &alarm_data);
               /*
                * If a high prio alarm is found don't check low prio
                * alarms until next call of this operation.
                */
               break;
            }
         }
      }
   }
}

/************************ Common alarm functions *****************************/

void pf_alarm_enable (pf_ar_t * p_ar)
{
   p_ar->alarm_enable = true;
}

void pf_alarm_disable (pf_ar_t * p_ar)
{
   p_ar->alarm_enable = false;
}

bool pf_alarm_pending (const pf_ar_t * p_ar)
{
   /* ToDo: Handle this call from cmpbe. */
   return false;
}

int pf_alarm_activate (pnet_t * net, pf_ar_t * p_ar)
{
   int ret = 0; /* Assume all goes well */
   int16_t i;
   pf_alarm_send_queue_t * q;

   for (i = 0; i < PF_ALARM_NUMBER_OF_PRIORITY_LEVELS; i++)
   {
      q = &p_ar->alarm_send_q[i];
      pf_alarm_queue_mutex_create (&q->accountant);
      pf_alarm_send_queue_reset (q);
   }

   if (pf_alarm_alpmx_activate (p_ar) != 0)
   {
      ret = -1;
   }

   if (pf_alarm_apmx_activate (net, p_ar) != 0)
   {
      ret = -1;
   }

   return ret;
}

int pf_alarm_close (pnet_t * net, pf_ar_t * p_ar)
{
   int ret = 0;
   int16_t i;
   pf_alarm_send_queue_t * q;

   for (i = 0; i < PF_ALARM_NUMBER_OF_PRIORITY_LEVELS; i++)
   {
      q = &p_ar->alarm_send_q[i];
      pf_alarm_send_queue_reset (q);
      pf_alarm_queue_mutex_destroy (&q->accountant);
   }

   LOG_DEBUG (
      PF_ALARM_LOG,
      "Alarm(%d): Closing alarm instance for AREP %u. Sending alarm with "
      "error_code2 = 0x%X\n",
      __LINE__,
      p_ar->arep,
      p_ar->err_code);
   if (pf_alarm_alpmx_close (p_ar) != 0)
   {
      ret = -1;
   }

   if (pf_alarm_apmx_close (net, p_ar, p_ar->err_code) != 0)
   {
      ret = -1;
   }

   return ret;
}

/**
 * @internal
 * Update the alarm specifier and maintenance status from a diagnosis item.
 *
 * Add one diag item to the alarm summary (this function is typically executed
 * several times on different diagnosis items to update the same alarm
 * specifier and maintenance status values).
 *
 * Looks at:
 *   p_diag_item->usi
 *   p_diag_item->fmt.std.ch_properties
 *   p_diag_item->fmt.std.qual_ch_qualifier
 *
 * @param p_ar             In:    The AR instance.
 * @param p_subslot        In:    The subslot instance.
 * @param p_diag_item      In:    The diag item.
 * @param p_alarm_spec     InOut: The updated alarm specifier.
 *                                Describes diagnosis alarms.
 * @param p_maint_status   InOut: The updated maintenance status.
 *                                Describes diagnosis alarms.
 */
void pf_alarm_add_diag_item_to_summary (
   const pf_ar_t * p_ar,
   const pf_subslot_t * p_subslot,
   const pf_diag_item_t * p_diag_item,
   pnet_alarm_spec_t * p_alarm_spec,
   uint32_t * p_maint_status)
{
   bool is_fault = false;
   bool is_maintenance_demanded = false;
   bool is_maintenance_required = false;
   bool is_appears = false;
   bool is_same_ar = false;
   uint32_t severity_qualifier = 0;

   /* Is the diagnosis on the same AR? */
   if (
      ((p_subslot->ownsm_state == PF_OWNSM_STATE_IOC) ||
       (p_subslot->ownsm_state == PF_OWNSM_STATE_IOS)) &&
      (p_subslot->owner == p_ar))
   {
      is_same_ar = true;
   }

   if (p_diag_item->usi < PF_USI_CHANNEL_DIAGNOSIS)
   {
      /* Diagnosis in manufacturer specific format (always FAULT) */

      p_alarm_spec->manufacturer_diagnosis = true;
      p_alarm_spec->submodule_diagnosis = true;
      if (is_same_ar == true)
      {
         p_alarm_spec->ar_diagnosis = true;
      }
   }
   else
   {
      /* Diagnosis in standard format */

      severity_qualifier =
         (p_diag_item->fmt.std.qual_ch_qualifier &
          PF_DIAG_QUALIFIED_SEVERITY_MASK);

      /* Severity "Maintenance required" */
      if (
         PF_DIAG_CH_PROP_MAINT_GET (p_diag_item->fmt.std.ch_properties) ==
         PNET_DIAG_CH_PROP_MAINT_REQUIRED)
      {
         is_maintenance_required = true;
      }
      else if (
         PF_DIAG_CH_PROP_MAINT_GET (p_diag_item->fmt.std.ch_properties) ==
         PNET_DIAG_CH_PROP_MAINT_QUALIFIED)
      {
         if ((severity_qualifier & PF_DIAG_QUALIFIER_MASK_REQUIRED) > 0)
         {
            is_maintenance_required = true;
         }
      }

      /* Severity "Maintenance demanded"  */
      if (
         PF_DIAG_CH_PROP_MAINT_GET (p_diag_item->fmt.std.ch_properties) ==
         PNET_DIAG_CH_PROP_MAINT_DEMANDED)
      {
         is_maintenance_demanded = true;
      }
      else if (
         PF_DIAG_CH_PROP_MAINT_GET (p_diag_item->fmt.std.ch_properties) ==
         PNET_DIAG_CH_PROP_MAINT_QUALIFIED)
      {
         if ((severity_qualifier & PF_DIAG_QUALIFIER_MASK_DEMANDED) > 0)
         {
            is_maintenance_demanded = true;
         }
      }

      /* Severity "Fault" */
      if (
         PF_DIAG_CH_PROP_MAINT_GET (p_diag_item->fmt.std.ch_properties) ==
         PNET_DIAG_CH_PROP_MAINT_FAULT)
      {
         is_fault = true;
      }
      else if (
         PF_DIAG_CH_PROP_MAINT_GET (p_diag_item->fmt.std.ch_properties) ==
         PNET_DIAG_CH_PROP_MAINT_QUALIFIED)
      {
         if ((severity_qualifier & PF_DIAG_QUALIFIER_MASK_FAULT) > 0)
         {
            is_fault = true;
         }
      }

      /* Is the diagnosis appearing */
      if (
         PF_DIAG_CH_PROP_SPEC_GET (p_diag_item->fmt.std.ch_properties) ==
         PF_DIAG_CH_PROP_SPEC_APPEARS)
      {
         is_appears = true;
      }

      /* Maintenance qualifier bits */
      *p_maint_status |= severity_qualifier;

      /* MaintenanceRequired and MaintenanceDemanded bits */
      if (is_maintenance_required == true)
      {
         *p_maint_status |= PF_DIAG_BIT_MAINTENANCE_REQUIRED;
      }
      if (is_maintenance_demanded == true)
      {
         *p_maint_status |= PF_DIAG_BIT_MAINTENANCE_DEMANDED;
      }

      /* Alarm specifiers for diagnosis in standard format */
      if (is_appears == true)
      {
         p_alarm_spec->channel_diagnosis = true;
      }
      if (is_appears == true && is_fault == true)
      {
         p_alarm_spec->submodule_diagnosis = true;
      }
      if (is_appears == true && is_fault == true && is_same_ar == true)
      {
         p_alarm_spec->ar_diagnosis = true;
      }
   }
}

int pf_alarm_periodic (pnet_t * net)
{
   uint16_t ix;
   pf_ar_t * p_ar;

   if (net->global_alarm_enable == true)
   {
      for (ix = 0; ix < PNET_MAX_AR; ix++)
      {
         p_ar = pf_ar_find_by_index (net, ix);
         if ((p_ar != NULL) && (p_ar->in_use == true))
         {
            /* Handle incoming alarm frames */
            (void)pf_alarm_apmr_periodic (net, p_ar);

            /* Handle outgoing alarm messages */
            (void)pf_alarm_almpi_periodic (net, p_ar);
         }
      }
   }

   return 0;
}

int pf_alarm_alpmr_alarm_ack (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pnet_alarm_argument_t * p_alarm_argument,
   const pnet_pnio_status_t * p_pnio_status)
{
   int ret = -1;
   pf_alarm_data_t alarm_data;
   pf_apmx_t * p_apmx = &p_ar->apmx[1]; /* Always use high prio. */
   pf_alpmx_t * p_alpmx = &p_ar->alpmx[1];

   CC_ASSERT (p_alarm_argument != NULL); /* From user code */
   CC_ASSERT (p_pnio_status != NULL);

   switch (p_alpmx->alpmr_state)
   {
   case PF_ALPMR_STATE_W_USER_ACK:
      memset (&alarm_data, 0, sizeof (alarm_data));
      alarm_data.alarm_type = p_alarm_argument->alarm_type;
      alarm_data.api_id = p_alarm_argument->api_id;
      alarm_data.slot_nbr = p_alarm_argument->slot_nbr;
      alarm_data.subslot_nbr = p_alarm_argument->subslot_nbr;
      alarm_data.alarm_specifier = p_alarm_argument->alarm_specifier;
      alarm_data.sequence_number = p_alarm_argument->sequence_number;

      p_alpmx->prev_sequence_number = p_alpmx->sequence_number;
      p_alpmx->sequence_number++;
      p_alpmx->sequence_number %= 0x0800;

      ret = pf_alarm_apms_apms_a_data_req (
         net,
         p_apmx,
         &alarm_data,
         0,
         p_pnio_status);

      p_alpmx->alpmr_state = PF_ALPMR_STATE_W_TACK;
      break;
   case PF_ALPMR_STATE_W_START:
   case PF_ALPMR_STATE_W_NOTIFY:
   case PF_ALPMR_STATE_W_TACK:
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): You tried to send an alarm ACK, but the ALPMR state is "
         "%s.\n",
         __LINE__,
         pf_alarm_alpmr_state_to_string (p_alpmx->alpmr_state));
      p_ar->err_cls = PNET_ERROR_CODE_1_ALPMI;
      p_ar->err_code = PNET_ERROR_CODE_2_ALPMI_WRONG_STATE;
      break;
   }

   return ret;
}

/**
 * @internal
 * Send an alarm to the controller using an RTA DATA PDU.
 *
 * It will always require TACK back.
 *
 * It uses pf_alarm_apms_apms_a_data_req() for sending (and retransmission).
 *
 * Also look in the diagnostics database to attach information whether
 * there are any relevant diagnosis items.
 *
 * Note that the PLC can set the max alarm payload length at startup. This
 * limit can be 200 to 1432 bytes.
 *
 * ALPMI: ALPMI_Alarm_Notification.req
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param alarm_type       In:    The alarm type.
 * @param high_prio        In:    true => Use high prio alarm.
 * @param api_id           In:    The API identifier.
 * @param slot_nbr         In:    The slot number.
 * @param subslot_nbr      In:    The sub-slot number.
 * @param p_diag_item      In:    Additional diagnosis item (may be NULL).
 *                                Used when calculating alarm specifier and
 *                                maintenance status.
 * @param module_ident     In:    The module ident number.
 * @param submodule_ident  In:    The sub-module ident number.
 * @param payload_usi      In:    The payload USI. Only used for
 *                                block type = alarm notify.
 *                                Use 0 for no payload (not even the USI value).
 * @param payload_len      In:    Number of bytes of manufaturer data,
 *                                or sizeof(pf_diag_item_t) for diagnosis in
 *                                standard format.
 *                                May be 0 also for manufacturer data.
 *                                Max PNET_MAX_ALARM_PAYLOAD_DATA_SIZE or
 *                                value from PLC.
 * @param p_payload        In:    pf_diag_item_t, manufacturer data or NULL.
 *                                Mandatory if payload_len > 0 or for USI
 *                                values for diagnosis in standard format
 *                                (which expect a pf_diag_item_t)
 * @return  0  if operation succeeded.
 *          -1 if an error occurred. Errors include:
 *             Payload oversize (process alarms only)
 *             Alarms not enabled for AR
 *             Queue for outgoing alarms is non-available or full
 */
static int pf_alarm_send_alarm (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_alarm_type_values_t alarm_type,
   bool high_prio,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   const pf_diag_item_t * p_diag_item,
   uint32_t module_ident,
   uint32_t submodule_ident,
   uint16_t payload_usi,
   uint16_t payload_len,
   const uint8_t * p_payload)
{
   pf_alarm_data_t alarm_data;

   if (net->global_alarm_enable == false || p_ar->alarm_enable == false)
   {
      LOG_INFO (
         PF_ALARM_LOG,
         "Alarm(%d): Alarm sending is disabled. Global enable: %u, AR enable: "
         "%u.\n",
         __LINE__,
         net->global_alarm_enable,
         p_ar->alarm_enable);

      return -1;
   }

   if (payload_len > sizeof (alarm_data.payload.data))
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): You provided too long alarm payload (%u bytes) but max "
         "is %zu. Increase PNET_MAX_ALARM_PAYLOAD_DATA_SIZE.\n",
         __LINE__,
         payload_len,
         sizeof (alarm_data.payload.data));

      return -1;
   }

   if (payload_len > p_ar->alarm_cr_request.max_alarm_data_length)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): You provided too long alarm payload (%u bytes) but PLC "
         "said max %u bytes.\n",
         __LINE__,
         payload_len,
         p_ar->alarm_cr_request.max_alarm_data_length);

      return -1;
   }

   if (payload_len > 0 && p_payload == NULL)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): payload_len is %u but no payload is given.\n",
         __LINE__,
         payload_len);

      return -1;
   }

   /* Prepare data */
   memset (&alarm_data, 0, sizeof (alarm_data));
   alarm_data.alarm_type = alarm_type;
   alarm_data.api_id = api_id;
   alarm_data.slot_nbr = slot_nbr;
   alarm_data.subslot_nbr = subslot_nbr;
   alarm_data.module_ident = module_ident;
   alarm_data.submodule_ident = submodule_ident;

   alarm_data.payload.usi = payload_usi;
   alarm_data.payload.len = payload_len;

   if (payload_len > 0)
   {
      memcpy (alarm_data.payload.data, p_payload, payload_len);
   }

   LOG_DEBUG (
      PF_ALARM_LOG,
      "Alarm(%d): Putting alarm data in %s prio output queue. Alarm type %u "
      "Payload %u bytes.\n",
      __LINE__,
      high_prio ? "high" : "low",
      alarm_type,
      payload_len);

   return pf_alarm_send_queue_post (
      &p_ar->alarm_send_q[high_prio ? 1 : 0],
      &alarm_data);
}

/************************ Send specific alarm types **************************/

int pf_alarm_send_process (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t payload_usi,
   uint16_t payload_len,
   const uint8_t * p_payload)
{
   uint32_t module_ident = 0;
   uint32_t submodule_ident = 0;

   LOG_INFO (
      PF_ALARM_LOG,
      "Alarm(%d): Sending process alarm. Slot %u, subslot %u, %u bytes, USI "
      "%u\n",
      __LINE__,
      slot_nbr,
      subslot_nbr,
      payload_len,
      payload_usi);

   if (payload_usi >= PF_USI_CHANNEL_DIAGNOSIS)
   {
      LOG_ERROR (
         PNET_LOG,
         "Alarm(%d): Wrong USI given for process alarm: %u Slot: %u "
         "Subslot %u\n",
         __LINE__,
         payload_usi,
         slot_nbr,
         subslot_nbr);
      return -1;
   }

   /* Find module and submodule identities */
   if (pf_cmdev_get_module_ident (net, api_id, slot_nbr, &module_ident) != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Failed to get module ident for slot: %u\n",
         __LINE__,
         slot_nbr);

      return -1;
   }
   if (
      pf_cmdev_get_submodule_ident (
         net,
         api_id,
         slot_nbr,
         subslot_nbr,
         &submodule_ident) != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Failed to get submodule ident for slot: %u, "
         "subslot: %u\n",
         __LINE__,
         slot_nbr,
         subslot_nbr);

      return -1;
   }

   return pf_alarm_send_alarm (
      net,
      p_ar,
      PF_ALARM_TYPE_PROCESS,
      true, /* High prio */
      api_id,
      slot_nbr,
      subslot_nbr,
      NULL, /* No p_diag_item */
      module_ident,
      submodule_ident,
      payload_usi,
      payload_len,
      p_payload);
}

int pf_alarm_send_diagnosis (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   const pf_diag_item_t * p_diag_item)
{
   pf_alarm_type_values_t alarm_type = PF_ALARM_TYPE_DIAGNOSIS;
   uint32_t module_ident = 0;
   uint32_t submodule_ident = 0;

   if (p_diag_item == NULL)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): The diagnosis item is NULL\n",
         __LINE__);

      return -1;
   }

   /* Calculate alarm type */
   if (p_diag_item->usi >= PF_USI_CHANNEL_DIAGNOSIS)
   {
      switch (p_diag_item->fmt.std.ch_error_type)
      {
      case PF_WRT_ERROR_REMOTE_MISMATCH:
         alarm_type = PF_ALARM_TYPE_PORT_DATA_CHANGE;
         break;
      default:
         break;
      }
   }

   /* Find module and submodule identities */
   if (pf_cmdev_get_module_ident (net, api_id, slot_nbr, &module_ident) != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Failed to get module ident for slot: %u\n",
         __LINE__,
         slot_nbr);

      return -1;
   }
   if (
      pf_cmdev_get_submodule_ident (
         net,
         api_id,
         slot_nbr,
         subslot_nbr,
         &submodule_ident) != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Failed to get submodule ident for slot: %u, "
         "subslot: %u\n",
         __LINE__,
         slot_nbr,
         subslot_nbr);

      return -1;
   }

   LOG_INFO (
      PF_ALARM_LOG,
      "Alarm(%d): Sending diagnosis alarm "
      "(type 0x%04X) Slot: %u  Subslot: 0x%04X  "
      "Module ident: %u Submodule ident: %u "
      "USI: 0x%04X\n",
      __LINE__,
      alarm_type,
      slot_nbr,
      subslot_nbr,
      (unsigned)module_ident,
      (unsigned)submodule_ident,
      p_diag_item->usi);

   if (p_diag_item->usi < PF_USI_CHANNEL_DIAGNOSIS)
   {
      /* USI format */
      return pf_alarm_send_alarm (
         net,
         p_ar,
         alarm_type,
         false, /* Low prio */
         api_id,
         slot_nbr,
         subslot_nbr,
         p_diag_item,
         module_ident,
         submodule_ident,
         p_diag_item->usi,
         p_diag_item->fmt.usi.len,
         (uint8_t *)&p_diag_item->fmt.usi.manuf_data);
   }

   /* Standard format */
   return pf_alarm_send_alarm (
      net,
      p_ar,
      alarm_type,
      false, /* Low prio */
      api_id,
      slot_nbr,
      subslot_nbr,
      p_diag_item,
      module_ident,
      submodule_ident,
      p_diag_item->usi,
      sizeof (*p_diag_item),
      (uint8_t *)p_diag_item);
}

int pf_alarm_send_usi_diagnosis_disappears (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t usi)
{
   uint32_t module_ident = 0;
   uint32_t submodule_ident = 0;

   CC_ASSERT (usi < PF_USI_CHANNEL_DIAGNOSIS);

   if (pf_cmdev_get_module_ident (net, api_id, slot_nbr, &module_ident) != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Failed to get module ident for slot: %u\n",
         __LINE__,
         slot_nbr);

      return -1;
   }

   if (
      pf_cmdev_get_submodule_ident (
         net,
         api_id,
         slot_nbr,
         subslot_nbr,
         &submodule_ident) != 0)
   {
      LOG_ERROR (
         PF_ALARM_LOG,
         "Alarm(%d): Failed to get submodule ident for slot: %u, "
         "subslot: %u\n",
         __LINE__,
         slot_nbr,
         subslot_nbr);

      return -1;
   }

   LOG_INFO (
      PF_ALARM_LOG,
      "Alarm(%d): Sending diagnosis alarm disappears "
      "Slot: %u  Subslot: 0x%04X  "
      "Module ident: %u Submodule ident: %u "
      "USI: 0x%04X\n",
      __LINE__,
      slot_nbr,
      subslot_nbr,
      (unsigned)module_ident,
      (unsigned)submodule_ident,
      usi);

   return pf_alarm_send_alarm (
      net,
      p_ar,
      PF_ALARM_TYPE_DIAGNOSIS_DISAPPEARS,
      false, /* Low prio */
      api_id,
      slot_nbr,
      subslot_nbr,
      NULL,
      module_ident,
      submodule_ident,
      usi,
      0,
      NULL);
}

int pf_alarm_send_pull (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr)
{
   uint16_t alarm_type = PF_ALARM_TYPE_PULL;

   LOG_INFO (
      PF_ALARM_LOG,
      "Alarm(%d): Sending pull alarm. Slot: %u  Subslot: 0x%04X\n",
      __LINE__,
      slot_nbr,
      subslot_nbr);
   if (subslot_nbr == 0)
   {
      if (p_ar->ar_param.ar_properties.pull_module_alarm_allowed == true)
      {
         alarm_type = PF_ALARM_TYPE_PULL_MODULE;
      }
   }

   (void)pf_alarm_send_alarm (
      net,
      p_ar,
      alarm_type,
      false, /* Low prio */
      api_id,
      slot_nbr,
      subslot_nbr,
      NULL,  /* No p_diag_item */
      0,     /* module_ident */
      0,     /* submodule_ident */
      0,     /* No payload */
      0,     /* No payload */
      NULL); /* No payload */

   return 0;
}

int pf_alarm_send_plug (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t module_ident,
   uint32_t submodule_ident)
{
   LOG_INFO (
      PF_ALARM_LOG,
      "Alarm(%d): Sending plug alarm. Slot: %u  Subslot: 0x%04X\n",
      __LINE__,
      slot_nbr,
      subslot_nbr);

   (void)pf_alarm_send_alarm (
      net,
      p_ar,
      PF_ALARM_TYPE_PLUG,
      false, /* Low prio */
      api_id,
      slot_nbr,
      subslot_nbr,
      NULL, /* No p_diag_item */
      module_ident,
      submodule_ident,
      0,     /* No payload */
      0,     /* No payload */
      NULL); /* No payload */

   return 0;
}

int pf_alarm_send_plug_wrong (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t module_ident,
   uint32_t submodule_ident)
{
   LOG_INFO (
      PF_ALARM_LOG,
      "Alarm(%d): Sending plug wrong alarm. Slot: %u  Subslot: 0x%04X\n",
      __LINE__,
      slot_nbr,
      subslot_nbr);

   (void)pf_alarm_send_alarm (
      net,
      p_ar,
      PF_ALARM_TYPE_PLUG_WRONG_MODULE,
      false, /* Low prio */
      api_id,
      slot_nbr,
      subslot_nbr,
      NULL, /* No p_diag_item */
      module_ident,
      submodule_ident,
      0,     /* No payload */
      0,     /* No payload */
      NULL); /* No payload */

   return 0;
}

int pf_alarm_send_released (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t module_ident,
   uint32_t submodule_ident)
{
   LOG_INFO (
      PF_ALARM_LOG,
      "Alarm(%d): Sending released alarm. Slot: %u  Subslot: 0x%04X\n",
      __LINE__,
      slot_nbr,
      subslot_nbr);

   (void)pf_alarm_send_alarm (
      net,
      p_ar,
      PF_ALARM_TYPE_RELEASED,
      false, /* Low prio */
      api_id,
      slot_nbr,
      subslot_nbr,
      NULL, /* No p_diag_item */
      module_ident,
      submodule_ident,
      0,     /* No payload */
      0,     /* No payload */
      NULL); /* No payload */

   return 0;
}
