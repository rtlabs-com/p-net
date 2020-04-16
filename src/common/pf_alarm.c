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
#define os_eth_send                 mock_os_eth_send
#endif

/*
 * ToDo:
 * 1) Send UDP frames.
 */
#include <string.h>

#include "pf_includes.h"
#include "pf_block_reader.h"
#include "pf_block_writer.h"

#define PF_FRAME_ID_ALARM_HIGH      0xfc01
#define PF_FRAME_ID_ALARM_LOW       0xfe01

#define ALARM_VLAN_PRIO_LOW         5
#define ALARM_VLAN_PRIO_HIGH        6


/* The scheduler identifier */
static const char *apmx_sync_name = "apmx";

/*************** Diagnostic strings *****************************************/

/**
 * @internal
 * Return a string representation of the given ALPMI state.
 * @param state            In:   The ALPMI state.
 * @return  A string representing the ALPMI state.
 */
static const char *pf_alarm_alpmi_state_to_string(
   pf_alpmi_state_values_t state)
{
   const char *s = "<error>";

   switch (state)
   {
   case PF_ALPMI_STATE_W_START:  s = "PF_ALPMI_STATE_W_START"; break;
   case PF_ALPMI_STATE_W_ALARM:  s = "PF_ALPMI_STATE_W_ALARM"; break;
   case PF_ALPMI_STATE_W_ACK:    s = "PF_ALPMI_STATE_W_ACK"; break;
   }

   return s;
}

/**
 * @internal
 * Return a string representation of the given ALPMR state.
 * @param state            In:   The ALPMR state.
 * @return  A string representing the ALPMR state.
 */
static const char *pf_alarm_alpmr_state_to_string(
   pf_alpmr_state_values_t state)
{
   const char *s = "<error>";

   switch (state)
   {
   case PF_ALPMR_STATE_W_START:    s = "PF_ALPMR_STATE_W_START"; break;
   case PF_ALPMR_STATE_W_NOTIFY:   s = "PF_ALPMR_STATE_W_NOTIFY"; break;
   case PF_ALPMR_STATE_W_USER_ACK: s = "PF_ALPMR_STATE_W_USER_ACK"; break;
   case PF_ALPMR_STATE_W_TACK:     s = "PF_ALPMR_STATE_W_TACK"; break;
   }

   return s;
}

/**
 * @internal
 * Return a string representation of the given APMS state.
 * @param state            In:   The APMS state.
 * @return  A string representing the APMS state.
 */
static const char *pf_alarm_apms_state_to_string(
   pf_apms_state_values_t  state)
{
   const char *s = "<error>";

   switch (state)
   {
   case PF_APMS_STATE_CLOSED: s = "PF_APMS_STATE_CLOSED"; break;
   case PF_APMS_STATE_OPEN:   s = "PF_APMS_STATE_OPEN"; break;
   case PF_APMS_STATE_WTACK:  s = "PF_APMS_STATE_WTACK"; break;
   }

   return s;
}

/**
 * @internal
 * Return a string representation of the given APMR state.
 * @param state            In:   The APMR state.
 * @return  A string representing the APMR state.
 */
static const char *pf_alarm_apmr_state_to_string(
   pf_apmr_state_values_t  state)
{
   const char *s = "<error>";

   switch (state)
   {
   case PF_APMR_STATE_CLOSED: s = "PF_APMR_STATE_CLOSED"; break;
   case PF_APMR_STATE_OPEN:   s = "PF_APMR_STATE_OPEN"; break;
   case PF_APMR_STATE_WCNF:   s = "PF_APMR_STATE_WCNF"; break;
   }

   return s;
}

void pf_alarm_show(
   pf_ar_t                 *p_ar)
{
   printf("Alarms   (low)\n");
   printf("  alpmi_state         = %s\n", pf_alarm_alpmi_state_to_string(p_ar->alpmx[0].alpmi_state));
   printf("  alpmr_state         = %s\n", pf_alarm_alpmr_state_to_string(p_ar->alpmx[0].alpmr_state));
   printf("  apms_state          = %s\n", pf_alarm_apms_state_to_string(p_ar->apmx[0].apms_state));
   printf("  apmr_state          = %s\n", pf_alarm_apmr_state_to_string(p_ar->apmx[0].apmr_state));
   printf("  exp_seq_count       = 0x%x\n", (unsigned)p_ar->apmx[0].exp_seq_count);
   printf("  exp_seq_count_o     = 0x%x\n", (unsigned)p_ar->apmx[0].exp_seq_count_o);
   printf("  send_seq_count      = 0x%x\n", (unsigned)p_ar->apmx[0].send_seq_count);
   printf("  send_seq_count_o    = 0x%x\n", (unsigned)p_ar->apmx[0].send_seq_count_o);
   printf("  sequence_number     = %u\n", (unsigned)p_ar->alpmx[0].sequence_number);
   printf("\n");
   printf("Alarms   (high)\n");
   printf("  alpmi_state         = %s\n", pf_alarm_alpmi_state_to_string(p_ar->alpmx[1].alpmi_state));
   printf("  alpmr_state         = %s\n", pf_alarm_alpmr_state_to_string(p_ar->alpmx[1].alpmr_state));
   printf("  apms_state          = %s\n", pf_alarm_apms_state_to_string(p_ar->apmx[1].apms_state));
   printf("  apmr_state          = %s\n", pf_alarm_apmr_state_to_string(p_ar->apmx[1].apmr_state));
   printf("  exp_seq_count       = 0x%x\n", (unsigned)p_ar->apmx[1].exp_seq_count);
   printf("  exp_seq_count_o     = 0x%x\n", (unsigned)p_ar->apmx[1].exp_seq_count_o);
   printf("  send_seq_count      = 0x%x\n", (unsigned)p_ar->apmx[1].send_seq_count);
   printf("  send_seq_count_o    = 0x%x\n", (unsigned)p_ar->apmx[1].send_seq_count_o);
   printf("  sequence_number     = %u\n", (unsigned)p_ar->alpmx[1].sequence_number);
   printf("\n");
}

/*****************************************************************************/

void pf_alarm_init(
   pnet_t                  *net)
{
   /* Enable alarms from the start */
   net->global_alarm_enable = true;
}

/**
 * @internal
 * Send an ALARM error indication to CMSU.
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           In:   The APMX instance.
 * @param err_cls          In:   The ERRCLS variable.
 * @param err_code         In:   The ERRCODE variable.
 */
static void pf_alarm_error_ind(
   pnet_t                  *net,
   pf_apmx_t               *p_apmx,
   uint8_t                 err_cls,
   uint8_t                 err_code)
{
   /*
    * ALPMI: PNET_ERROR_CODE_2_CMSU_AR_ALARM_SEND;
    * ALPMI: PNET_ERROR_CODE_2_CMSU_AR_ALARM_ACK_SEND;
    *
    * ALPMR: PNET_ERROR_CODE_2_CMSU_AR_ALARM_IND
    *
    * APMR:  PNET_ERROR_CODE_2_CMSU_AR_ALARM_IND
    */
   LOG_INFO(PF_ALARM_LOG, "Alarm(%d): err_cls 0x%02x  err_code 0x%02x\n", __LINE__, err_cls, err_code);
   (void)pf_cmsu_alarm_error_ind(net, p_apmx->p_ar, err_cls, err_code);
}

/* ===== ALPMI - Alarm initiator ===== */

/**
 * @internal
 * Activate the ALPMI and ALPMR instances.
 *
 * @param p_ar             In:   The AR instance.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_alpmx_activate(
   pf_ar_t                 *p_ar)
{
   int                     ret = 0; /* Assume all goes well */
   uint16_t                ix;

   for (ix = 0; ix < NELEMENTS(p_ar->alpmx); ix++)
   {
      if (p_ar->alpmx[ix].alpmi_state == PF_ALPMI_STATE_W_START)
      {
         /* Save the remote side MAC address for easy retrieval. */
         memcpy(&p_ar->alpmx[ix].da, &p_ar->ar_param.cm_initiator_mac_add, sizeof(p_ar->alpmx[ix].da));

         p_ar->alpmx[ix].prev_sequence_number = 0xffff;
         p_ar->alpmx[ix].sequence_number = 0;

         p_ar->alpmx[ix].alpmr_state = PF_ALPMR_STATE_W_NOTIFY;
         p_ar->alpmx[ix].alpmi_state = PF_ALPMI_STATE_W_ALARM;
      }
      else
      {
         p_ar->err_cls = PNET_ERROR_CODE_1_ALPMI;
         p_ar->err_code = PNET_ERROR_CODE_2_ALPMI_WRONG_STATE;

         ret = -1;
      }
   }

   return ret;
}

/**
 * @internal
 * Close the ALPMI and ALPMR instances.
 *
 * @param p_ar             In:   The AR instance.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_alpmx_close(
   pf_ar_t                 *p_ar)
{
   int                     ret = 0; /* Assume all goes well */
   uint16_t                ix;

   for (ix = 0; ix < NELEMENTS(p_ar->alpmx); ix++)
   {
      p_ar->alpmx[ix].alpmi_state = PF_ALPMI_STATE_W_START;
   }

   for (ix = 0; ix < NELEMENTS(p_ar->alpmx); ix++)
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
          * According to the spec we should call pf_alarm_error_ind here with ERRCLS and ERRCODE.
          * Since this avalanches into CMDEV which calls pf_cmsu_cmdev_state_ind which calls this
          * function we would create an eternal loop. So dont call here.
          *
          * CMSU is the only caller of pf_alarm_close - why would we need to call CMSU with an
          * error indicating that we are closing??
          */

         p_ar->alpmx[ix].alpmr_state = PF_ALPMR_STATE_W_START;
         /* pf_alarm_error_ind(net, &p_ar->apmx[ix], p_ar->err_cls, PNET_ERROR_CODE_2_CMSU_AR_ALARM_IND); */
         break;
      }
   }

   return ret;
}

/**
 * @internal
 * ALPMI: APMS_Data.cnf(+/-)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           In:   The APMS instance sending the confirmation.
 * @param res              In:   The positive (true) or negative (false) confirmation.
 */
static void pf_alarm_alpmi_apms_a_data_cnf(
   pnet_t                  *net,
   pf_apmx_t               *p_apmx,
   int                     res)
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
         pf_alarm_error_ind(net, p_apmx, p_apmx->p_ar->err_cls, p_apmx->p_ar->err_code);
      }
      break;
   }
}

/**
 * @internal
 * ALPMI: APMR_Data.cnf(+/-)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           In:   The APMS instance sending the confirmation.
 * @param p_pnio_status    In:   The positive (true) or negative (false) confirmation.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_alpmi_apmr_a_data_ind(
   pnet_t                  *net,
   pf_apmx_t               *p_apmx,
   pnet_pnio_status_t      *p_pnio_status)
{
   int                     ret = -1;

   /* ALPMI: APMR_A_Data.ind */
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
      (void)pf_fspm_aplmi_alarm_cnf(net, p_apmx->p_ar, p_pnio_status);
      ret = 0;
      break;
   }

   return ret;
}

/**
 * @internal
 * ALPMR: APMS_Data.cnf(+/-)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           In:   The APMS instance sending the confirmation.
 * @param res              In:   The positive (true) or negative (false) confirmation.
 */
static void pf_alarm_alpmr_apms_a_data_cnf(
   pnet_t                  *net,
   pf_apmx_t              *p_apmx,
   int                     res)
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
         pf_alarm_error_ind(net, p_apmx, p_apmx->p_ar->err_cls, p_apmx->p_ar->err_code);
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
         pf_alarm_error_ind(net, p_apmx, p_apmx->p_ar->err_cls, p_apmx->p_ar->err_code);
      }
      else
      {
         /* ALPMR_Alarm_Ack.cnf(+) */
         (void)pf_fspm_aplmr_alarm_ack_cnf(net, p_apmx->p_ar, res);
         p_apmx->p_alpmx->alpmr_state = PF_ALPMR_STATE_W_NOTIFY;
      }
      break;
   }
}

/**
 * @internal
 * Handle APMR DATA messages in ALPMR.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           In:   The APMR instance that received the message.
 * @param p_fixed          In:   The Fixed part of the RTA-PDU.
 * @param p_alarm_data     In:   The AlarmData from the RTA-PDU.
 * @param data_len         In:   VarDataLen from the RTA-PDU.
 * @param data_usi         In:   The USI from the RTA-PDU.
 * @param p_data           In:   The variable part of the RTA-PDU.
 * @return
 */
static int pf_alarm_alpmr_apmr_a_data_ind(
   pnet_t                  *net,
   pf_apmx_t               *p_apmx,
   pf_alarm_fixed_t        *p_fixed,
   pf_alarm_data_t         *p_alarm_data,
   uint16_t                data_len,
   uint16_t                data_usi,
   uint8_t                 *p_data)
{
   int                     ret = -1;

   switch (p_apmx->p_alpmx->alpmr_state)
   {
   case PF_ALPMR_STATE_W_START:
      /* Ignore */
      ret = 0;
      break;
   case PF_ALPMR_STATE_W_NOTIFY:
      /* Only DATA: AlarmNotifications are sent to this function */
      ret = pf_fspm_aplmr_alarm_ind(net, p_apmx->p_ar,
         p_alarm_data->api_id, p_alarm_data->slot_nbr, p_alarm_data->subslot_nbr,
         data_usi, data_len, p_data);

      /* App must now send an ACK or a NACK */
      p_apmx->p_alpmx->alpmr_state = PF_ALPMR_STATE_W_USER_ACK;
      break;
   case PF_ALPMR_STATE_W_USER_ACK:
      /* ToDo: "Combine data for the RTA error request */
      /* FALL-THRU */
   case PF_ALPMR_STATE_W_TACK:
      p_apmx->p_ar->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
      p_apmx->p_ar->err_code = PNET_ERROR_CODE_2_ABORT_AR_PROTOCOL_VIOLATION;
      pf_alarm_error_ind(net, p_apmx, p_apmx->p_ar->err_cls, p_apmx->p_ar->err_code);
      break;
   }

   return ret;
}

/* ===== APMS - Acyclic sender ===== */

/**
 * @internal
 * Handle high-priority alarm PDUs from the controller.
 *
 * This is a callback for the frame handler. Arguments should fulfill pf_eth_frame_handler_t
 *
 * All incoming Profinet frames with ID = PF_FRAME_ID_ALARM_HIGH end up here.
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:   The Ethernet frame id.
 * @param p_buf            In:   The Ethernet frame buffer.
 * @param frame_id_pos     In:   Position in the buffer of the frame id.
 * @param p_arg            In:   The AR.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmr_high_handler(
   pnet_t                  *net,
   uint16_t                frame_id,
   os_buf_t                *p_buf,
   uint16_t                frame_id_pos,
   void                    *p_arg)
{
   int                     ret = 1;       /* Means "handled" */
   pf_apmx_t               *p_apmx = (pf_apmx_t *)p_arg;
   pf_apmr_msg_t           *p_apmr_msg;
   uint16_t                nbr;

   if (ret != 0)
   {
      if (p_buf != NULL)
      {
         nbr = p_apmx->apmr_msg_nbr++;    /* ToDo: Make atomic */
         if (p_apmx->apmr_msg_nbr >= NELEMENTS(p_apmx->apmr_msg))
         {
            p_apmx->apmr_msg_nbr = 0;
         }
         p_apmr_msg = &p_apmx->apmr_msg[nbr];
         p_apmr_msg->p_buf = p_buf;
         p_apmr_msg->frame_id_pos = frame_id_pos;
         if (os_mbox_post(p_apmx->p_alarm_q, (void *)p_apmr_msg, 0) != 0)
         {
            LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): Lost one alarm\n", __LINE__);
         }
      }
   }

   return ret;
}

/**
 * @internal
 * Handle low-priority alarm PDUs from the controller.
 *
 * This is a callback for the frame handler. Arguments should fulfill pf_eth_frame_handler_t
 *
 * All incoming Profinet frames with ID = PF_FRAME_ID_ALARM_LOW end up here.
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:   The Ethernet frame id.
 * @param p_buf            In:   The Ethernet frame buffer.
 * @param frame_id_pos     In:   Position in the buffer of the frame id.
 * @param p_arg            In:   The AR.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmr_low_handler(
   pnet_t                  *net,
   uint16_t                frame_id,
   os_buf_t                *p_buf,
   uint16_t                frame_id_pos,
   void                    *p_arg)
{
   int                     ret = 1;       /* Means "handled" */
   pf_apmx_t               *p_apmx = (pf_apmx_t *)p_arg;
   pf_apmr_msg_t           *p_apmr_msg;
   uint16_t                nbr;

   if (ret != 0)
   {
      if (p_buf != NULL)
      {
         nbr = p_apmx->apmr_msg_nbr++;    /* ToDo: Make atomic */
         if (p_apmx->apmr_msg_nbr >= NELEMENTS(p_apmx->apmr_msg))
         {
            p_apmx->apmr_msg_nbr = 0;
         }
         p_apmr_msg = &p_apmx->apmr_msg[nbr];
         p_apmr_msg->p_buf = p_buf;
         p_apmr_msg->frame_id_pos = frame_id_pos;
         if (os_mbox_post(p_apmx->p_alarm_q, (void *)p_apmr_msg, 0) != 0)
         {
            LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): Lost one alarm\n", __LINE__);
         }
      }
   }

   return ret;
}

/**
 * @internal
 * Timeout while waiting for an ACK from the controller.
 *
 * Re-send the frame unless the number of re-transmits have been reached.
 * If an ACK is received then the APMS state is no longer WTACK and the
 * frame is not re-sent (and the timer stops).
 *
 * This is a callback for the scheduler. Arguments should fulfill pf_scheduler_timeout_ftn_t
 *
 * @param net              InOut: The p-net stack instance
 * @param arg
 * @param current_time
 */
static void pf_alarm_apms_timeout(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                current_time)
{
   pf_apmx_t               *p_apmx = (pf_apmx_t *)arg;
   os_buf_t                *p_rta;

   p_apmx->timeout_id = UINT32_MAX;
   if (p_apmx->apms_state == PF_APMS_STATE_WTACK)
   {
      if (p_apmx->retry > 0)
      {
         p_apmx->retry--;

         /* Retransmit */
         if (p_apmx->p_ar->alarm_cr_request.alarm_cr_properties.transport_udp == true)
         {
            /* ToDo: Send as UDP frame */
         }
         else
         {
            if (os_eth_send(p_apmx->p_ar->p_sess->eth_handle, p_apmx->p_rta) <= 0)
            {
               LOG_ERROR(PF_ALARM_LOG, "pf_alarm(%d): Error from os_eth_send(rta)\n", __LINE__);
            }
         }

         if (pf_scheduler_add(net, p_apmx->timeout_us,
            apmx_sync_name, pf_alarm_apms_timeout, p_apmx, &p_apmx->timeout_id) != 0)
         {
            LOG_ERROR(PF_ALARM_LOG, "pf_alarm(%d): Error from pf_scheduler_add\n", __LINE__);
         }
         else
         {
            p_apmx->timeout_id = UINT32_MAX;
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
         LOG_DEBUG(PF_AL_BUF_LOG, "pf_alarm(%d): Free saved RTA buffer %p\n", __LINE__, p_apmx->p_rta);
         if (p_apmx->p_rta != NULL)
         {
            p_rta = p_apmx->p_rta;
            p_apmx->p_rta = NULL;
            os_buf_free(p_rta);
         }

         pf_alarm_alpmi_apms_a_data_cnf(net, p_apmx, -1);
         pf_alarm_alpmr_apms_a_data_cnf(net, p_apmx, -1);

#endif
         p_apmx->apms_state = PF_APMS_STATE_OPEN;

         p_apmx->p_ar->err_cls = PNET_ERROR_CODE_1_APMS;
         p_apmx->p_ar->err_code = PNET_ERROR_CODE_2_APMS_TIMEOUT;
         pf_alarm_error_ind(net, p_apmx, p_apmx->p_ar->err_cls, p_apmx->p_ar->err_code);
      }
   }
   else
   {
      LOG_DEBUG(PF_ALARM_LOG, "pf_alarm(%d): Timeout in state %s\n", __LINE__, pf_alarm_apms_state_to_string(p_apmx->apms_state));
      LOG_DEBUG(PF_AL_BUF_LOG, "pf_alarm(%d): Free saved RTA buffer %p\n", __LINE__, p_apmx->p_rta);
      if (p_apmx->p_rta != NULL)
      {
         p_rta = p_apmx->p_rta;
         p_apmx->p_rta = NULL;
         os_buf_free(p_rta);
      }
   }
}

/**
 * @internal
 * Initialize and start an AMPX instance.
 *
 * This function activates and starts an APMS/APMR pair.
 *
 * It registers handlers for incoming alarm frames.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmx_activate(
   pnet_t                  *net,
   pf_ar_t                 *p_ar)
{
   int                     ret = 0; /* Assume all goes well */
   uint16_t                ix;

   for (ix = 0; ix < NELEMENTS(p_ar->apmx); ix++)
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
         memcpy(&p_ar->apmx[ix].da, &p_ar->ar_param.cm_initiator_mac_add, sizeof(p_ar->apmx[ix].da));

         p_ar->apmx[ix].src_ref = p_ar->alarm_cr_result.remote_alarm_reference;
         p_ar->apmx[ix].dst_ref = p_ar->alarm_cr_request.local_alarm_reference;

         /* APMS counters */
         p_ar->apmx[ix].send_seq_count = 0xffff;
         p_ar->apmx[ix].send_seq_count_o = 0xfffe;

         /* APMR counters */
         p_ar->apmx[ix].exp_seq_count = 0xffff;
         p_ar->apmx[ix].exp_seq_count_o = 0xfffe;

         if (ix == 0)
         {
            p_ar->apmx[ix].vlan_prio = ALARM_VLAN_PRIO_LOW;
            p_ar->apmx[ix].block_type_alarm_notify = PF_BT_ALARM_NOTIFICATION_LOW;
            p_ar->apmx[ix].block_type_alarm_ack = PF_BT_ALARM_ACK_LOW;
            p_ar->apmx[ix].frame_id = PF_FRAME_ID_ALARM_LOW;
         }
         else
         {
            p_ar->apmx[ix].vlan_prio = ALARM_VLAN_PRIO_HIGH;
            p_ar->apmx[ix].block_type_alarm_notify = PF_BT_ALARM_NOTIFICATION_HIGH;
            p_ar->apmx[ix].block_type_alarm_ack = PF_BT_ALARM_ACK_HIGH;
            p_ar->apmx[ix].frame_id = PF_FRAME_ID_ALARM_HIGH;
         }

         if (p_ar->apmx[ix].p_alarm_q == NULL)
         {
            p_ar->apmx[ix].p_alarm_q = os_mbox_create(PNET_MAX_ALARMS);
         }
         memset(p_ar->apmx[ix].apmr_msg, 0, sizeof(p_ar->apmx[ix].apmr_msg));
         p_ar->apmx[ix].apmr_msg_nbr = 0;

         p_ar->apmx[ix].timeout_us = 100*1000*p_ar->alarm_cr_request.rta_timeout_factor;

         p_ar->apmx[ix].p_ar = p_ar;
         p_ar->apmx[ix].p_alpmx = &p_ar->alpmx[ix];

         p_ar->apmx[ix].timeout_id = UINT32_MAX;

         p_ar->apmx[ix].apms_state = PF_APMS_STATE_OPEN;
         p_ar->apmx[ix].apmr_state = PF_APMR_STATE_OPEN;
      }
   }

   pf_eth_frame_id_map_add(net, PF_FRAME_ID_ALARM_LOW, pf_alarm_apmr_low_handler, &p_ar->apmx[0]);
   pf_eth_frame_id_map_add(net, PF_FRAME_ID_ALARM_HIGH, pf_alarm_apmr_high_handler, &p_ar->apmx[1]);

   return ret;
}

/**
 * @internal
 * APMS: a_data_ind
 *
 * Handle incoming alarm data
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           In:   The APMX instance.
 * @param p_fixed          In:   The fixed part of the ALARM message.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apms_a_data_ind(
   pnet_t                  *net,
   pf_apmx_t               *p_apmx,
   pf_alarm_fixed_t        *p_fixed)
{
   int                     ret = -1;
   os_buf_t                *p_rta;

   if ((p_fixed->pdu_type.type == PF_RTA_PDU_TYPE_ACK) ||
       (p_fixed->pdu_type.type == PF_RTA_PDU_TYPE_DATA))
   {
      /* APMS: a_data_ind */
      if (p_apmx->apms_state == PF_APMS_STATE_WTACK)
      {
         if (p_fixed->ack_seq_nbr == p_apmx->send_seq_count)
         {
            p_apmx->send_seq_count_o = p_apmx->send_seq_count;
            p_apmx->send_seq_count = (p_apmx->send_seq_count + 1) & 0x7fff;

            LOG_DEBUG(PF_AL_BUF_LOG, "pf_alarm(%d): Free saved RTA buffer %p\n", __LINE__, p_apmx->p_rta);
            if (p_apmx->p_rta != NULL)
            {
               p_rta = p_apmx->p_rta;
               p_apmx->p_rta = NULL;
               os_buf_free(p_rta);
            }

            pf_alarm_alpmi_apms_a_data_cnf(net, p_apmx, 0);
            pf_alarm_alpmr_apms_a_data_cnf(net, p_apmx, 0);

            ret = 0;

            p_apmx->apms_state = PF_APMS_STATE_OPEN;
         }
         else
         {
            /*
             * Ignore!
             * A timeout occurs and the frame will be re-sent.
             */
            ret = 0;
         }
      }
      else
      {
         /*
          * Ignore!
          * A timeout occurred and the frame has already been freed.
          */
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
 * APMS: A_Data_Req
 *
 * Create and send a Profinet alarm frame to the IO-controller.
 *
 * Messages with TACK == true are saved in the APMX instance in order
 * to handle re-transmissions.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           In:   The APMX instance.
 * @param p_fixed          In:   The fixed part of the RTA-PDU.
 * @param p_alarm_data     In:   More fixed data for DATA messages.
 * @param maint_status     In:   Added if > 0.
 * @param payload_usi      In:   The payload USI (may be 0)..
 * @param payload_len      In:   Mandatory if payload_usi > 0.
 * @param p_payload        In:   Mandatory if payload_len > 0.
 * @param p_pnio_status    In:   Mandatory for ERROR messages.
 *                               Must be NULL for DATA messages.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apms_a_data_req(
   pnet_t                  *net,
   pf_apmx_t               *p_apmx,
   pf_alarm_fixed_t        *p_fixed,
   pf_alarm_data_t         *p_alarm_data,
   uint32_t                maint_status,
   uint16_t                payload_usi,
   uint16_t                payload_len,
   uint8_t                 *p_payload,
   pnet_pnio_status_t      *p_pnio_status)
{
   int                     ret = -1;
   uint16_t                var_part_len_pos;
   uint16_t                var_part_len;
   os_buf_t                *p_rta;
   uint8_t                 *p_buf = NULL;
   uint16_t                pos = 0;
   const pnet_cfg_t        *p_cfg = NULL;
   uint16_t                u16 = 0;

   pf_fspm_get_default_cfg(net, &p_cfg);

   if (p_apmx->p_ar->alarm_cr_request.alarm_cr_properties.transport_udp == true)
   {
      /* ToDo: Create UDP frames */
   }
   else
   {
      LOG_DEBUG(PF_AL_BUF_LOG, "Alarm(%d): Allocate RTA buffer\n", __LINE__);
      p_rta = os_buf_alloc(1500);
      if (p_rta == NULL)
      {
         LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): No buffer for alarm notification\n", __LINE__);
      }
      else
      {
         p_buf = p_rta->payload;
         if (p_buf == NULL)
         {
            LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): No payload buffer for alarm notification\n", __LINE__);
         }
         else
         {
            /* Insert destination MAC address */
            pf_put_mem(&p_apmx->da, sizeof(p_apmx->da), 1500, p_buf, &pos);

            /* Insert source MAC address (our interface MAC address) */
            memcpy(&p_buf[pos], p_cfg->eth_addr.addr, sizeof(pnet_ethaddr_t));
            pos += sizeof(pnet_ethaddr_t);

            /* Insert VLAN Tag protocol identifier (TPID) */
            pf_put_uint16(true, OS_ETHTYPE_VLAN, 1500, p_buf, &pos);

            /* Insert VLAN prio (and VLAN ID=0) */
            u16 = (p_apmx->vlan_prio & 0x0007) << 13;  /* Three leftmost bits */
            pf_put_uint16(true, u16, 1500, p_buf, &pos);

            /* Insert EtherType */
            pf_put_uint16(true, OS_ETHTYPE_PROFINET, 1500, p_buf, &pos);

            /* Insert Profinet frame ID (first part of Ethernet frame payload) */
            pf_put_uint16(true, p_apmx->frame_id, 1500, p_buf, &pos);

            /* Insert alarm specific data */
            pf_put_alarm_fixed(true, p_fixed, 1500, p_buf, &pos);

            var_part_len_pos = pos;
            pf_put_uint16(true, 0, 1500, p_buf, &pos);                     /* var_part_len is unknown here */

            if (p_fixed->pdu_type.type == PF_RTA_PDU_TYPE_DATA)
            {
               if (p_pnio_status != NULL)
               {
                  /* Send an AlarmAck DATA message */
                  pf_put_alarm_block(true, p_apmx->block_type_alarm_ack,
                     p_alarm_data, maint_status,
                     0, 0, NULL, 1500, p_buf, &pos);

                  /* Append the PNIO status */
                  pf_put_pnet_status(true, p_pnio_status, 1500, p_buf, &pos);
               }
               else
               {
                  /* Send an AlarmNotification DATA message */
                  pf_put_alarm_block(true, p_apmx->block_type_alarm_notify,
                     p_alarm_data, maint_status,
                     payload_usi, payload_len, p_payload, 1500, p_buf, &pos);
               }
            }
            else if (p_fixed->pdu_type.type == PF_RTA_PDU_TYPE_ERR)
            {
               pf_put_pnet_status(true, p_pnio_status, 1500, p_buf, &pos);
            }
            else
            {
               /* NACK or ACk does not take any var part. */
            }

            /* Finally insert the correct VarPartLen */
            var_part_len = pos - (var_part_len_pos + sizeof(var_part_len));
            pf_put_uint16(true, var_part_len, 1500, p_buf, &var_part_len_pos);

            p_rta->len = pos;
            if (os_eth_send(p_apmx->p_ar->p_sess->eth_handle, p_rta) <= 0)
            {
               LOG_ERROR(PF_ALARM_LOG, "pf_alarm(%d): Error from os_eth_send(rta)\n", __LINE__);
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
               LOG_DEBUG(PF_AL_BUF_LOG, "pf_alarm(%d): Save RTA buffer\n", __LINE__);
               p_apmx->p_rta = p_rta;
            }
            else
            {
               LOG_ERROR(PF_ALARM_LOG, "pf_alarm(%d): RTA buffer with TACK lost!!\n", __LINE__);
               os_buf_free(p_rta);
            }
         }
         else
         {
            /* ACK, NAK and ERR buffers are not saved for retransmission. */
            LOG_DEBUG(PF_AL_BUF_LOG, "pf_alarm(%d): Free unsaved RTA buffer\n", __LINE__);
            os_buf_free(p_rta);
         }
      }
   }

   return ret;
}

/**
 * @internal
 * APMS: APMS_A_Data_req
 *
 * This function creates and sends an RTA-PDU.
 *
 * It uses pf_alarm_apms_a_data_req() for the sending.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           In:   The APMS instance.
 * @param pdu_type         In:   Mandatory. The PDU type.
 * @param tack             In:   Mandatory. TACK.
 * @param p_alarm_data     In:   Mandatory if pdu_type == DATA.
 * @param maint_status     In:   Inserted if > 0.
 * @param payload_usi      In:   May be 0.
 * @param payload_len      In:   Payload length. Mandatory id payload_usi != 0.
 * @param p_payload        In:   Mandatory if payload_len > 0.
 * @param p_pnio_status    Out:  Mandatory. Detailed error information.
 * @return
 */
static int pf_alarm_apms_apms_a_data_req(
   pnet_t                  *net,
   pf_apmx_t               *p_apmx,
   pf_alarm_pdu_type_values_t pdu_type,
   bool                    tack,
   pf_alarm_data_t         *p_alarm_data,
   uint32_t                maint_status,
   uint16_t                payload_usi,
   uint16_t                payload_len,
   uint8_t                 *p_payload,
   pnet_pnio_status_t      *p_pnio_status)
{
   int                     ret = -1;
   pf_alarm_fixed_t        fixed;

   if (p_apmx->apms_state != PF_APMS_STATE_OPEN)
   {
      p_pnio_status->error_code_1 = PNET_ERROR_CODE_1_APMS;
      p_pnio_status->error_code_2 = PNET_ERROR_CODE_2_APMS_INVALID_STATE;
      pf_alarm_alpmi_apms_a_data_cnf(net, p_apmx, -1);
      pf_alarm_alpmr_apms_a_data_cnf(net, p_apmx, -1);
   }
   else
   {
      fixed.src_ref = p_apmx->src_ref;
      fixed.dst_ref = p_apmx->dst_ref;
      fixed.pdu_type.type = pdu_type;
      fixed.pdu_type.version = PF_ALARM_PDU_TYPE_VERSION_1;
      fixed.add_flags.window_size = 1;
      fixed.add_flags.tack = tack;
      fixed.send_seq_num = p_apmx->send_seq_count;
      fixed.ack_seq_nbr = p_apmx->exp_seq_count_o;

      p_apmx->retry = p_apmx->p_ar->alarm_cr_request.rta_retries;

      ret = pf_alarm_apms_a_data_req(net, p_apmx, &fixed,
         p_alarm_data, maint_status,
         payload_usi, payload_len, p_payload, p_pnio_status);

      p_apmx->apms_state = PF_APMS_STATE_WTACK;

      /* APMS: a_data_cnf */
      ret = pf_scheduler_add(net, p_apmx->timeout_us, apmx_sync_name,
         pf_alarm_apms_timeout, p_apmx, &p_apmx->timeout_id);
      if (ret != 0)
      {
         p_apmx->timeout_id = UINT32_MAX;
         LOG_ERROR(PF_ALARM_LOG, "pf_alarm(%d): Error from pf_scheduler_add\n", __LINE__);
      }
   }

   return ret;
}

/**
 * @internal
 * Close APMX.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param err_code         In:   Error code
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmx_close(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint8_t                 err_code)
{
   uint16_t                ix;
   pnet_pnio_status_t      pnio_status;
   os_buf_t                *p_rta;

   /* Send low prio CLOSE alarm first */
   if (p_ar->apmx[0].apms_state != PF_APMS_STATE_CLOSED)
   {
      pnio_status.error_code = PNET_ERROR_CODE_RTA_ERROR;
      pnio_status.error_decode = PNET_ERROR_DECODE_PNIO;
      pnio_status.error_code_1 = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
      pnio_status.error_code_2 = err_code;

      if (pf_alarm_apms_apms_a_data_req(net, &p_ar->apmx[0], PF_RTA_PDU_TYPE_ERR, false,
         NULL,
         0,
         0, 0, NULL,
         &pnio_status) != 0)
      {
         LOG_ERROR(PF_ALARM_LOG, "alarm(%d): Could not send close alarm\n", __LINE__);
      }
   }

   pf_eth_frame_id_map_remove(net, PF_FRAME_ID_ALARM_HIGH);
   pf_eth_frame_id_map_remove(net, PF_FRAME_ID_ALARM_LOW);

   for (ix = 0; ix < NELEMENTS(p_ar->apmx); ix++)
   {
      if (p_ar->apmx[ix].apms_state != PF_APMS_STATE_CLOSED)
      {
         /* Free resources */
         /* StopTimer */
         if (p_ar->apmx[ix].timeout_id != UINT32_MAX)
         {
            pf_scheduler_remove(net, apmx_sync_name, p_ar->apmx[ix].timeout_id);
            p_ar->apmx[ix].timeout_id = UINT32_MAX;
         }

         p_ar->apmx[ix].p_ar = NULL;
         p_ar->apmx[ix].apms_state = PF_APMS_STATE_CLOSED;
      }
      LOG_DEBUG(PF_AL_BUF_LOG, "pf_alarm(%d): Free saved RTA buffer %p\n", __LINE__, p_ar->apmx[ix].p_rta);
      if (p_ar->apmx[ix].p_rta != NULL)
      {
         p_rta = p_ar->apmx[ix].p_rta;
         p_ar->apmx[ix].p_rta = NULL;
         os_buf_free(p_rta);
      }

      if (p_ar->apmx[ix].apmr_state != PF_APMR_STATE_CLOSED)
      {
         p_ar->apmx[ix].apmr_state = PF_APMR_STATE_CLOSED;

         if (p_ar->apmx[ix].p_alarm_q != NULL)
         {
            os_mbox_destroy(p_ar->apmx[ix].p_alarm_q);
         }
      }
   }

   return 0;
}


/**
 * @internal
 * Send APMR ACK
 *
 * It uses pf_alarm_apms_a_data_req() for the sending.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           In:   The APMX instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmr_send_ack(
   pnet_t                  *net,
   pf_apmx_t               *p_apmx)
{
   int                     ret = -1;
   pf_alarm_fixed_t        fixed;

   memset(&fixed, 0, sizeof(fixed));
   fixed.src_ref = p_apmx->src_ref;
   fixed.dst_ref = p_apmx->dst_ref;
   fixed.pdu_type.type = PF_RTA_PDU_TYPE_ACK;
   fixed.pdu_type.version = PF_ALARM_PDU_TYPE_VERSION_1;
   fixed.add_flags.window_size = 1;
   fixed.add_flags.tack = false;
   fixed.send_seq_num = p_apmx->send_seq_count_o;
   fixed.ack_seq_nbr = p_apmx->exp_seq_count_o;

   ret = pf_alarm_apms_a_data_req(net, p_apmx, &fixed,
      NULL,
      0,
      0, 0, NULL,
      NULL);

   return ret;
}


/**
 * @internal
 * Send APMR NACK
 *
 * It uses pf_alarm_apms_a_data_req() for the sending.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           In:   The APMX instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmr_send_nack(
   pnet_t                  *net,
   pf_apmx_t               *p_apmx)
{
   int                     ret = -1;
   pf_alarm_fixed_t        fixed;

   memset(&fixed, 0, sizeof(fixed));
   fixed.src_ref = p_apmx->src_ref;
   fixed.dst_ref = p_apmx->dst_ref;
   fixed.pdu_type.type = PF_RTA_PDU_TYPE_NACK;
   fixed.pdu_type.version = PF_ALARM_PDU_TYPE_VERSION_1;
   fixed.add_flags.window_size = 1;
   fixed.add_flags.tack = false;
   fixed.send_seq_num = p_apmx->send_seq_count_o;
   fixed.ack_seq_nbr = p_apmx->exp_seq_count_o;

   ret = pf_alarm_apms_a_data_req(net, p_apmx, &fixed,
      NULL,
      0,
      0, 0, NULL,
      NULL);

   return ret;
}

/**
 * @internal
 * Handle reception of an RTA DATA PDU.
 *
 * This function handles reception of an RTA-DATA-PDU in APMR.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_apmx           In:   The APMX instance.
 * @param p_fixed          In:   The fixed part of the RTA-DPU.
 * @param data_len         In:   Length of the variable part of the RTA-PDU.
 * @param p_data           In:   The variable part of the RTA-PDU.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmr_a_data_ind(
   pnet_t                  *net,
   pf_apmx_t               *p_apmx,
   pf_alarm_fixed_t        *p_fixed,
   uint16_t                data_len,
   pf_get_info_t           *p_get_info,
   uint16_t                pos)
{
   int                     ret = -1;
   uint16_t                start_pos = pos;
   pf_block_header_t       block_header;
   pf_alarm_data_t         alarm_data;
   pnet_pnio_status_t      pnio_status;
   uint16_t                data_usi;
   uint8_t                 *p_data;

   /* APMR: a_data_ind */
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
         /* Interpret the RTA-SDU ::= Alarm-Notification-PDU || Alarm-Ack-PDU */
         if (p_fixed->send_seq_num == p_apmx->exp_seq_count)
         {
            pf_get_block_header(p_get_info, &pos, &block_header);
            if (block_header.block_type == p_apmx->block_type_alarm_ack)
            {
               /* Tell APMS to handle ack_seq_num in message */
               (void)pf_alarm_apms_a_data_ind(net, p_apmx, p_fixed);

               p_apmx->exp_seq_count_o = p_apmx->exp_seq_count;
               p_apmx->exp_seq_count = (p_apmx->exp_seq_count + 1) & 0x7fff;

               ret = pf_alarm_apmr_send_ack(net, p_apmx);
               if (ret == 0)
               {
                  p_apmx->apmr_state = PF_APMR_STATE_WCNF;

                  /* APMR: A_Data_Cnf */
                  /* AlarmAck */
                  /* APMR: APMR_A_Data.ind */
                  pf_get_alarm_ack(p_get_info, &pos, &alarm_data);
                  pf_get_pnio_status(p_get_info, &pos, &pnio_status);

                  ret = pf_alarm_alpmi_apmr_a_data_ind(net, p_apmx, &pnio_status);
                  if (ret == 0)
                  {
                     p_apmx->apmr_state = PF_APMR_STATE_OPEN;
                  }
               }
            }
            else if (block_header.block_type == p_apmx->block_type_alarm_notify)
            {
               /* Tell APMS to handle ack_seq_num in message */
               (void)pf_alarm_apms_a_data_ind(net, p_apmx, p_fixed);

               p_apmx->exp_seq_count_o = p_apmx->exp_seq_count;
               p_apmx->exp_seq_count = (p_apmx->exp_seq_count + 1) & 0x7fff;

               ret = pf_alarm_apmr_send_ack(net, p_apmx);
               if (ret == 0)
               {
                  p_apmx->apmr_state = PF_APMR_STATE_WCNF;

                  /* APMR: A_Data_Cnf */
                  /* AlarmNotification */
                  /* APMR: APMR_A_Data.ind */
                  pf_get_alarm_data(p_get_info, &pos, &alarm_data);
                  /* Check for USI */
                  data_usi = pf_get_uint16(p_get_info, &pos);
                  if (p_get_info->result == PF_PARSE_OK)
                  {
                     data_len -= (pos - start_pos);
                     p_data = &p_get_info->p_buf[pos];
                     ret = pf_alarm_alpmr_apmr_a_data_ind(net, p_apmx, p_fixed, &alarm_data, data_len, data_usi, p_data);
                  }
                  else
                  {
                     LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): Error parsing Alarm notification\n", __LINE__);
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
               LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): Unhandled block type %#x\n", __LINE__, block_header.block_type);
               p_apmx->p_ar->err_cls = PNET_ERROR_CODE_1_ALPMR;
               p_apmx->p_ar->err_code = PNET_ERROR_CODE_2_ALPMR_WRONG_ALARM_PDU;
               pf_alarm_error_ind(net, p_apmx, p_apmx->p_ar->err_cls, p_apmx->p_ar->err_code);
               ret = -1;
            }
         }
         else if (p_fixed->send_seq_num == p_apmx->exp_seq_count_o)
         {
            /* Tell APMS to handle ack_seq_num in message */
            (void)pf_alarm_apms_a_data_ind(net, p_apmx, p_fixed);

            /* Re-send the ACK */
            (void)pf_alarm_apmr_send_ack(net, p_apmx);
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
            pf_fspm_create_log_book_entry(net, p_apmx->p_ar->arep, &pnio_status, __LINE__);

            (void)pf_alarm_apmr_send_nack(net, p_apmx);
            ret = 0;
         }
      }
      break;
   case PF_APMR_STATE_WCNF:
      /* WCNF is a virtual state in the case above */
      LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): This should never happen!\n", __LINE__);
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
 * Handle the reception of ALARM messages for one AR instance.
 *
 * When called this function reads max 1 alarm message from its input queue.
 * The message is processed in APMR/ALPMR and indications may be sent to the
 * application.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_apmr_periodic(
   pnet_t                  *net,
   pf_ar_t                 *p_ar)
{
   int                     ret = 0;
   pf_apmx_t               *p_apmx;
   uint16_t                ix;
   pf_apmr_msg_t           *p_alarm_msg;
   os_buf_t                *p_buf;
   pf_alarm_fixed_t        fixed;
   uint16_t                var_part_len;
   pnet_pnio_status_t      pnio_status;
   pf_get_info_t           get_info;
   uint16_t                pos;

   /*
    * Periodic is run cyclically. It may run for some time.
    * If anything happens to the APMR (e.g. it is closed) during this time
    * then we need to stop handling this APMR!
    * Otherwise there will be lots of NULL pointers.
    */
   for (ix = 0; ((ret == 0) && (ix < NELEMENTS(p_ar->apmx))); ix++)
   {
      p_apmx = &p_ar->apmx[ix];
      p_buf = NULL;
      while ((ret == 0) &&
             (p_apmx->apmr_state != PF_APMR_STATE_CLOSED) &&
             (os_mbox_fetch(p_apmx->p_alarm_q, (void **)&p_alarm_msg, 0) == 0))
      {
         if (p_alarm_msg != NULL)
         {
            /* Got something - extract! */
            p_buf = p_alarm_msg->p_buf;
            pos = p_alarm_msg->frame_id_pos + sizeof(uint16_t); /* Skip frame_id */

            get_info.result = PF_PARSE_OK;
            get_info.is_big_endian = true;
            get_info.p_buf = (uint8_t *)p_buf->payload;
            get_info.len = p_buf->len;

            memset(&fixed, 0, sizeof(fixed));
            pf_get_alarm_fixed(&get_info, &pos, &fixed);
            var_part_len = pf_get_uint16(&get_info, &pos);

            /* APMR: A_data.ind */
            if (fixed.pdu_type.version == 1)
            {
               switch (fixed.pdu_type.type)
               {
               case PF_RTA_PDU_TYPE_ACK:
                  LOG_DEBUG(PF_ALARM_LOG, "Alarm(%d): ACK received\n", __LINE__);
                  if (var_part_len == 0)
                  {
                     /* Tell APMS to check for and handle ACK */
                     (void)pf_alarm_apms_a_data_ind(net, p_apmx, &fixed);
                  }
                  else
                  {
                     LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): var_part_len %u\n", __LINE__, (unsigned)var_part_len);
                     ret = 0; /* Just ignore */
                  }
                  break;
               case PF_RTA_PDU_TYPE_NACK:
                  LOG_DEBUG(PF_ALARM_LOG, "Alarm(%d): NACK received\n", __LINE__);
                  /* APMS: A_Data_Ind (NAK) */
                  if (var_part_len == 0)
                  {
                     /* Ignore */
                     ret = 0;
                  }
                  else
                  {
                     LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): var_part_len %u\n", __LINE__, (unsigned)var_part_len);
                     ret = 0; /* Just ignore */
                  }
                  break;
               case PF_RTA_PDU_TYPE_DATA:
                  LOG_DEBUG(PF_ALARM_LOG, "Alarm(%d): DATA received\n", __LINE__);
                  ret = pf_alarm_apmr_a_data_ind(net, p_apmx, &fixed, var_part_len, &get_info, pos);
                  break;
               case PF_RTA_PDU_TYPE_ERR:
                  if (var_part_len == 4)     /* sizeof(pf_pnio_status_t) */
                  {
                     /* APMS: A_Data_Ind(ERR) */
                     pf_get_pnio_status(&get_info, &pos, &pnio_status);
                     switch (p_apmx->apmr_state)
                     {
                     case PF_APMR_STATE_OPEN:
                        pf_alarm_error_ind(net, p_apmx, pnio_status.error_code_1, pnio_status.error_code_2);
                        break;
                     default:
                        pf_alarm_error_ind(net, p_apmx, PNET_ERROR_CODE_1_APMR, PNET_ERROR_CODE_2_APMR_INVALID_STATE);
                        break;
                     }

                     ret = 0;
                  }
                  else
                  {
                     LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): var_part_len %u\n", __LINE__, (unsigned)var_part_len);
                     /* Ignore */
                     ret = 0;
                  }
                  break;
               default:
                  LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): PDU-Type.type %u\n", __LINE__, (unsigned)fixed.pdu_type.type);
                  /* Ignore */
                  ret = 0;
                  break;
               }
            }
            else
            {
               LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): PDU-Type.version %u\n", __LINE__, (unsigned)fixed.pdu_type.version);
               /* Ignore */
               ret = 0;
            }

            LOG_DEBUG(PF_AL_BUF_LOG, "pf_alarm(%d): Free received buffer\n", __LINE__);
            os_buf_free(p_buf);
            p_buf = NULL;
         }
         else
         {
            LOG_ERROR(PF_ALARM_LOG, "Alarm(%d): expected buf from mbox, but got NULL\n", __LINE__);
            ret = -1;
         }
      }
   }

   return ret;
}

/* ===== Common alarm functions ===== */
void pf_alarm_enable(
   pf_ar_t                 *p_ar)
{
   p_ar->alarm_enable = true;
}

void pf_alarm_disable(
   pf_ar_t                 *p_ar)
{
   p_ar->alarm_enable = false;
}

bool pf_alarm_pending(
   pf_ar_t                 *p_ar)
{
   /* ToDo: Handle this call from cmpbe. */
   return false;
}

int pf_alarm_activate(
   pnet_t                  *net,
   pf_ar_t                 *p_ar)
{
   int                     ret = 0; /* Assume all goes well */

   if (pf_alarm_alpmx_activate(p_ar) != 0)
   {
      ret = -1;
   }

   if (pf_alarm_apmx_activate(net, p_ar) != 0)
   {
      ret = -1;
   }

   return ret;
}

int pf_alarm_close(
   pnet_t                  *net,
   pf_ar_t                 *p_ar)
{
   int                     ret = 0;

   if (pf_alarm_alpmx_close(p_ar) != 0)
   {
      ret = -1;
   }

   if (pf_alarm_apmx_close(net, p_ar, p_ar->err_code) != 0)
   {
      ret = -1;
   }

   return ret;
}

/**
 * @internal
 * Add one diag item to the alarm digest.
 * @param p_ar             In:   The AR instance.
 * @param p_subslot        In:   The subslot instance.
 * @param p_diag_item      In:   The diag item.
 * @param p_alarm_spec     Out:  The computed alarm specifier.
 * @param p_maint_status   Out:  The computed maintenance status.
 */
static void pf_alarm_add_item_to_digest(
   pf_ar_t                 *p_ar,
   pf_subslot_t            *p_subslot,
   pf_diag_item_t          *p_diag_item,
   pnet_alarm_spec_t       *p_alarm_spec,
   uint32_t                *p_maint_status)
{
   if (p_diag_item->usi < PF_USI_CHANNEL_DIAGNOSIS)
   {
      p_alarm_spec->manufacturer_diagnosis = true;
   }
   else
   {
      *p_maint_status |= p_diag_item->fmt.std.qual_ch_qualifier;
      if (PNET_DIAG_CH_PROP_MAINT_GET(p_diag_item->fmt.std.ch_properties) == PNET_DIAG_CH_PROP_MAINT_REQUIRED)
      {
         *p_maint_status |= BIT(0);  /* Set MaintenanceRequired bit */
      }
      if (PNET_DIAG_CH_PROP_MAINT_GET(p_diag_item->fmt.std.ch_properties) == PNET_DIAG_CH_PROP_MAINT_DEMANDED)
      {
         *p_maint_status |= BIT(1);  /* Set MaintenanceDemanded bit */
      }

      if (PNET_DIAG_CH_PROP_MAINT_GET(p_diag_item->fmt.std.ch_properties) == PNET_DIAG_CH_PROP_MAINT_FAULT)
      {
         p_alarm_spec->submodule_diagnosis = true;
         if (p_subslot->p_ar == p_ar)
         {
            /* We found a FAULT that belongs to a specific AR */
            p_alarm_spec->ar_diagnosis = true;
         }
      }
      p_alarm_spec->channel_diagnosis = true;
   }
}

/**
 * @internal
 * Return the alarm specifier for a specific sub-slot.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param p_diag_item      In:   The current diag item (may be NULL).
 * @param p_alarm_spec     Out:  The computed alarm specifier.
 * @param p_maint_status   Out:  The computed maintenance status.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_alarm_get_diag_digest(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   pf_diag_item_t          *p_diag_item,
   pnet_alarm_spec_t       *p_alarm_spec,
   uint32_t                *p_maint_status)
{
   pf_device_t             *p_dev = NULL;
   pf_subslot_t            *p_subslot = NULL;
   uint16_t                item_ix;
   pf_diag_item_t          *p_item = NULL;

   if (pf_cmdev_get_device(net, &p_dev) == 0)
   {
      memset(p_alarm_spec, 0, sizeof(*p_alarm_spec));
      *p_maint_status = 0;

      /* Consider only alarms on this sub-slot. */
      if (pf_cmdev_get_subslot_full(net, api_id, slot_nbr, subslot_nbr, &p_subslot) == 0)
      {
         /* Collect all diags into maintenanceStatus*/

         /* First handle the "current" (unreported) item. */
         if (p_diag_item != NULL)
         {
            pf_alarm_add_item_to_digest(p_ar, p_subslot, p_diag_item, p_alarm_spec, p_maint_status);
         }

         /* Now handle all the already reported diag items. */
         item_ix = p_subslot->diag_list;
         pf_cmdev_get_diag_item(net, item_ix, &p_item);
         while (p_item != NULL)
         {
            /* Collect all diags into maintenanceStatus*/
            pf_alarm_add_item_to_digest(p_ar, p_subslot, p_item, p_alarm_spec, p_maint_status);

            item_ix = p_item->next;
            pf_cmdev_get_diag_item(net, item_ix, &p_item);
         }
      }
   }

   return 0;
}

int pf_alarm_periodic(
   pnet_t                  *net)
{
   uint16_t                ix;
   pf_ar_t                 *p_ar;

   if (net->global_alarm_enable == true)
   {
      for (ix = 0; ix < PNET_MAX_AR; ix++)
      {
         p_ar = pf_ar_find_by_index(net, ix);
         if ((p_ar != NULL) && (p_ar->in_use == true))
         {
            (void)pf_alarm_apmr_periodic(net, p_ar);
         }
      }
   }

   return 0;
}

int pf_alarm_alpmr_alarm_ack(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_pnio_status_t      *p_pnio_status)
{
   int                     ret = -1;
   pf_alarm_fixed_t        fixed;
   pf_apmx_t               *p_apmx = &p_ar->apmx[1];  /* Always use high prio. */
   pf_alpmx_t              *p_alpmx = &p_ar->alpmx[1];

   /* ALPMR_alarm_ack_req */
   switch (p_alpmx->alpmr_state)
   {
   case PF_ALPMR_STATE_W_USER_ACK:
      fixed.src_ref = p_apmx->src_ref;
      fixed.dst_ref = p_apmx->dst_ref;
      fixed.pdu_type.type = PF_RTA_PDU_TYPE_DATA;
      fixed.pdu_type.version = PF_ALARM_PDU_TYPE_VERSION_1;
      fixed.add_flags.window_size = 1;
      fixed.add_flags.tack = true;
      fixed.send_seq_num = p_apmx->send_seq_count;
      fixed.ack_seq_nbr = p_apmx->exp_seq_count_o;

      ret = pf_alarm_apms_a_data_req(net, p_apmx, &fixed,
         NULL,
         0,
         0, 0, NULL,
         p_pnio_status);

      /* ALPMR: APMS_A_Data.cnf */
      if (ret != 0)
      {
         p_ar->err_cls = PNET_ERROR_CODE_1_ALPMI;
         p_ar->err_code = PNET_ERROR_CODE_2_ALPMI_INVALID;
         pf_alarm_error_ind(net, p_apmx, p_ar->err_cls, p_ar->err_code);
      }
      p_alpmx->alpmr_state = PF_ALPMR_STATE_W_NOTIFY;
      break;
   case PF_ALPMR_STATE_W_START:
   case PF_ALPMR_STATE_W_NOTIFY:
   case PF_ALPMR_STATE_W_TACK:
      p_ar->err_cls = PNET_ERROR_CODE_1_ALPMI;
      p_ar->err_code = PNET_ERROR_CODE_2_ALPMI_WRONG_STATE;
      break;
   }

   return ret;
}

/**
 * @internal
 * Send an alarm.
 *
 * This function sends an alarm to the controller using an RTA DATA PDU.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param alarm_type       In:   The alarm type.
 * @param high_prio        In:   true => use high prio alarm.
 * @param tack             In:   true => require transfer ACK.
 * @param api_id           In:   The API identifier.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param p_diag_item      In:   The diagnosis item (may be NULL).
 * @param module_ident     In:   The module ident number.
 * @param submodule_ident  In:   The sub-module ident number.
 * @param payload_usi      In:   The USI of the payload.
 * @param payload_len      In:   The length of the payload (is usi < 0x8000).
 * @param p_payload        In:   The payload data (if usi < 0x8000).
 * @param p_result         Out:  Detailed error information (may be NULL).
 * @return  0  if operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try later).
 */
static int pf_alarm_send_alarm(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_alarm_type_values_t  alarm_type,
   bool                    high_prio,
   bool                    tack,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   pf_diag_item_t          *p_diag_item,
   uint32_t                module_ident,
   uint32_t                submodule_ident,
   uint16_t                payload_usi,
   uint16_t                payload_len,
   uint8_t                 *p_payload,
   pnet_result_t           *p_result)
{
   int                     ret = -1;
   pf_alarm_data_t         alarm_data;
   uint32_t                maint_status = 0;
   pf_apmx_t               *p_apmx = &p_ar->apmx[high_prio ? 1 : 0];
   pf_alpmx_t              *p_alpmx = &p_ar->alpmx[high_prio ? 1 : 0];

   if (net->global_alarm_enable == true)
   {
      if (p_alpmx->alpmi_state == PF_ALPMI_STATE_W_ALARM)
      {
         /* Manufacturer Data */
         memset(&alarm_data, 0, sizeof(alarm_data));
         alarm_data.alarm_type = alarm_type;
         alarm_data.api_id = api_id;
         alarm_data.slot_nbr = slot_nbr;
         alarm_data.subslot_nbr = subslot_nbr;
         alarm_data.module_ident = module_ident;
         alarm_data.submodule_ident = submodule_ident;
         (void)pf_alarm_get_diag_digest(net, p_ar, api_id, slot_nbr, subslot_nbr,
            p_diag_item, &alarm_data.alarm_specifier, &maint_status);

         alarm_data.sequence_number = p_alpmx->sequence_number;
         p_alpmx->prev_sequence_number = p_alpmx->sequence_number;
         p_alpmx->sequence_number++;
         p_alpmx->sequence_number %= 0x0800;

         ret = pf_alarm_apms_apms_a_data_req(net, p_apmx, PF_RTA_PDU_TYPE_DATA, tack,
            &alarm_data,
            maint_status,
            payload_usi, payload_len, p_payload,
            (p_result != NULL) ? &p_result->pnio_status : NULL);

         p_alpmx->alpmi_state = PF_ALPMI_STATE_W_ACK;
      }
      else
      {
         LOG_INFO(PF_ALARM_LOG, "Alarm(%d): ALPMI state %s (dropped)\n",
            __LINE__, pf_alarm_alpmi_state_to_string(p_alpmx->alpmi_state));
      }
   }
   else
   {
      ret = 0; /* ToDo: Is this right? */
   }

   return ret;
}

int pf_alarm_send_process(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint16_t                payload_usi,
   uint16_t                payload_len,
   uint8_t                 *p_payload)
{
   LOG_INFO(PF_ALARM_LOG, "Alarm(%d): process alarm\n", __LINE__);

   /* High prio, require TACK */
   return pf_alarm_send_alarm(net, p_ar, PF_ALARM_TYPE_PROCESS, true, true,
      api_id, slot_nbr, subslot_nbr,
      NULL,       /* p_diag_item */
      0, 0,       /* module_ident, submodule_ident */
      payload_usi, payload_len, p_payload,
      NULL);      /* p_result */
}

int pf_alarm_send_diagnosis(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   pf_diag_item_t          *p_diag_item)
{
   int                     ret = -1;

   LOG_INFO(PF_ALARM_LOG, "Alarm(%d): diagnosis alarm\n", __LINE__);
   if (p_diag_item != NULL)
   {
      /* Low prio, require TACK */
      ret = pf_alarm_send_alarm(net, p_ar, PF_ALARM_TYPE_DIAGNOSIS, false, true,
         api_id, slot_nbr, subslot_nbr,
         p_diag_item,
         0, 0,       /* module_ident, submodule_ident */
         p_diag_item->usi, sizeof(*p_diag_item), (uint8_t *)p_diag_item,
         NULL);      /* p_result */
   }

   return ret;
}

int pf_alarm_send_pull(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr)
{
   uint16_t                alarm_type;

   LOG_INFO(PF_ALARM_LOG, "Alarm(%d): pull alarm\n", __LINE__);
   if (subslot_nbr == 0)
   {
      if (p_ar->ar_param.ar_properties.pull_module_alarm_allowed == true)
      {
         alarm_type = PF_ALARM_TYPE_PULL_MODULE;
      }
      else
      {
         alarm_type = PF_ALARM_TYPE_PULL;
      }
   }
   else
   {
      alarm_type = PF_ALARM_TYPE_PULL;
   }

   /* High prio, no TACK */
   (void)pf_alarm_send_alarm(net, p_ar, alarm_type, false, false,
      api_id, slot_nbr, subslot_nbr,
      NULL,       /* p_diag_item */
      0, 0,       /* module_ident, submodule_ident */
      0, 0, NULL, /* payload_usi, payload_len, p_payload, */
      NULL);      /* p_result */

   return 0;
}

int pf_alarm_send_plug(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint32_t                module_ident,
   uint32_t                submodule_ident)
{
   LOG_INFO(PF_ALARM_LOG, "Alarm(%d): plug alarm\n", __LINE__);

   /* High prio, no TACK */
   (void)pf_alarm_send_alarm(net, p_ar, PF_ALARM_TYPE_PLUG, false, false,
      api_id, slot_nbr, subslot_nbr,
      NULL,       /* p_diag_item */
      module_ident, submodule_ident,
      0, 0, NULL, /* payload_usi, payload_len, p_payload, */
      NULL);      /* p_result */

   return 0;
}

int pf_alarm_send_plug_wrong(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint32_t                module_ident,
   uint32_t                submodule_ident)
{
   LOG_INFO(PF_ALARM_LOG, "Alarm(%d): plug wrong alarm\n", __LINE__);

   /* High prio, no TACK */
   (void)pf_alarm_send_alarm(net, p_ar, PF_ALARM_TYPE_PLUG_WRONG_MODULE, false, false,
      api_id, slot_nbr, subslot_nbr,
      NULL,       /* p_diag_item */
      module_ident, submodule_ident,
      0, 0, NULL, /* payload_usi, payload_len, p_payload, */
      NULL);      /* p_result */

   return 0;
}
