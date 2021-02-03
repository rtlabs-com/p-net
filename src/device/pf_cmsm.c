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
 * @brief Implements the Context Management Surveillance Protocol Machine Device
 * (CMSM)
 *
 * The CMSM component monitors the establishment of a connection.
 * Once the device enters the DATA state this component is done.
 *
 * An incoming CMDEV event PNET_EVENT_STARTUP starts the timer,
 * and PNET_EVENT_DATA or PNET_EVENT_ABORT stops the timer.
 *
 * In case the timer times out, the CMDEV state is set to PNET_EVENT_ABORT.
 *
 * The time is extended (timer is restarted) for incoming RPC read and write
 * requests, if the timer is running.
 *
 * If there is no ongoing connection (p_ar == NULL), then the timer is not
 * affected by the rest of the stack.
 */

#ifdef UNIT_TEST

#endif

#include "pf_includes.h"

int pf_cmsm_activate (pf_ar_t * p_ar)
{
   if (p_ar == NULL)
   {
      return -1;
   }

   pf_scheduler_init_handle (&p_ar->cmsm_timeout, "cmsm");
   return 0;
}

/**
 * @internal
 * Return a string representation of the CMSM state.
 * @param state            In:   The CMSM state
 * @return  A string representing the CMSM state.
 */
static const char * pf_cmsm_state_to_string (pf_cmsm_state_values_t state)
{
   const char * s = "<unknown>";

   switch (state)
   {
   case PF_CMSM_STATE_IDLE:
      s = "PF_CMSM_STATE_IDLE";
      break;
   case PF_CMSM_STATE_RUN:
      s = "PF_CMSM_STATE_RUN";
      break;
   }

   return s;
}

void pf_cmsm_show (const pf_ar_t * p_ar)
{
   if (p_ar == NULL)
   {
      printf ("CMSM: The AR is null\n");
      return;
   }

   printf (
      "CMSM state            = %s\n",
      pf_cmsm_state_to_string (p_ar->cmsm_state));
   printf (
      "     timer            = %u\n",
      (unsigned)pf_scheduler_get_value (&p_ar->cmsm_timeout));
}

/**
 * @internal
 * Set the state of the CMSM component.
 * @param p_ar             InOut: The AR instance (not NULL)
 * @param state            In:    The new state.
 */
static void pf_cmsm_set_state (pf_ar_t * p_ar, pf_cmsm_state_values_t state)
{
   CC_ASSERT (p_ar != NULL);

   if (state != p_ar->cmsm_state)
   {
      LOG_DEBUG (
         PNET_LOG,
         "CMSM(%d): New state %s (was %s)\n",
         __LINE__,
         pf_cmsm_state_to_string (state),
         pf_cmsm_state_to_string (p_ar->cmsm_state));
      p_ar->cmsm_state = state;
   }
}

/**
 * @internal
 * Handle timeouts in the connection monitoring.
 * Aborts the connection start-up procedure if timing out.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: The AR instance (not NULL). Void pointer
 *                                converted to pf_ar_t
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 *                                stored tasks. Not used here.
 */
static void pf_cmsm_timeout (pnet_t * net, void * arg, uint32_t current_time)
{
   CC_ASSERT (arg != NULL);
   pf_ar_t * p_ar = (pf_ar_t *)arg;

   pf_scheduler_reset_handle (&p_ar->cmsm_timeout);
   switch (p_ar->cmsm_state)
   {
   case PF_CMSM_STATE_IDLE:
      /* Ignore */
      break;
   case PF_CMSM_STATE_RUN:
      LOG_ERROR (
         PNET_LOG,
         "CMSM(%d): Timeout for communication start up. CMDEV state: %s \n",
         __LINE__,
         pf_cmdev_state_to_string (p_ar->cmdev_state));

      p_ar->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
      p_ar->err_code = PNET_ERROR_CODE_2_ABORT_AR_CMI_TIMEOUT;
      pf_cmdev_state_ind (net, p_ar, PNET_EVENT_ABORT);
      pf_cmsm_set_state (p_ar, PF_CMSM_STATE_IDLE);
      break;
   }
}

/********************** Public functions affecting the timer *****************/

int pf_cmsm_cmdev_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_event_values_t event)
{
   int ret = -1;

   if (p_ar == NULL)
   {
      return 0;
   }

   LOG_DEBUG (
      PNET_LOG,
      "CMSM(%d): Received event %s from CMDEV. Initial state %s for AREP %u.\n",
      __LINE__,
      pf_cmdev_event_to_string (event),
      pf_cmsm_state_to_string (p_ar->cmsm_state),
      p_ar->arep);

   switch (p_ar->cmsm_state)
   {
   case PF_CMSM_STATE_IDLE:
      switch (event)
      {
      case PNET_EVENT_STARTUP:
         pf_cmsm_set_state (p_ar, PF_CMSM_STATE_RUN);
         LOG_DEBUG (
            PNET_LOG,
            "CMSM(%d): Starting timer. cm_initiator_activity_timeout_factor %u "
            "x 100 ms\n",
            __LINE__,
            (unsigned)p_ar->ar_param.cm_initiator_activity_timeout_factor);
         (void)pf_scheduler_restart (
            net,
            p_ar->ar_param.cm_initiator_activity_timeout_factor * 100 *
               1000, /* time in us */
            pf_cmsm_timeout,
            (void *)p_ar,
            &p_ar->cmsm_timeout);
         ret = 0;
         break;
      default:
         /* Ignore */
         ret = 0;
         break;
      }
      break;
   case PF_CMSM_STATE_RUN:
      switch (event)
      {
      case PNET_EVENT_DATA:
      case PNET_EVENT_ABORT:
         if (pf_scheduler_is_running (&p_ar->cmsm_timeout))
         {
            LOG_DEBUG (
               PNET_LOG,
               "CMSM(%d): Stopping communication start-up timer. AREP %u\n",
               __LINE__,
               p_ar->arep);
         }
         pf_scheduler_remove_if_running (net, &p_ar->cmsm_timeout);
         pf_cmsm_set_state (p_ar, PF_CMSM_STATE_IDLE);
         ret = 0;
         break;
      default:
         /* Ignore */
         ret = 0;
         break;
      }
      break;
   }

   return ret;
}

int pf_cmsm_rm_read_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_iod_read_request_t * p_read_request)
{
   int ret = -1;

   if (p_ar == NULL)
   {
      return 0;
   }

   switch (p_ar->cmsm_state)
   {
   case PF_CMSM_STATE_IDLE:
      /* Ignore or positive rsp */
      ret = 0;
      break;
   case PF_CMSM_STATE_RUN:
      if (p_read_request->index == PF_IDX_DEV_CONN_MON_TRIGGER)
      {
         /* Restart timeout period */
         (void)pf_scheduler_restart (
            net,
            p_ar->ar_param.cm_initiator_activity_timeout_factor * 100 *
               1000, /* time in us */
            pf_cmsm_timeout,
            (void *)p_ar,
            &p_ar->cmsm_timeout);
      }
      ret = 0;
      break;
   }

   return ret;
}

int pf_cmsm_cm_read_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_read_request_t * p_read_request)
{
   int ret = -1;

   if (p_ar == NULL)
   {
      return 0;
   }

   switch (p_ar->cmsm_state)
   {
   case PF_CMSM_STATE_IDLE:
      /* Ignore */
      ret = 0;
      break;
   case PF_CMSM_STATE_RUN:
      /* Restart timeout period */
      (void)pf_scheduler_restart (
         net,
         p_ar->ar_param.cm_initiator_activity_timeout_factor * 100 * 1000,
         pf_cmsm_timeout,
         (void *)p_ar,
         &p_ar->cmsm_timeout);
      ret = 0;
      break;
   }

   return ret;
}

int pf_cmsm_cm_write_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_request)
{
   int ret = -1;

   if (p_ar == NULL)
   {
      return 0;
   }

   switch (p_ar->cmsm_state)
   {
   case PF_CMSM_STATE_IDLE:
      /* Ignore */
      ret = 0;
      break;
   case PF_CMSM_STATE_RUN:
      /* Restart timeout period */
      (void)pf_scheduler_restart (
         net,
         p_ar->ar_param.cm_initiator_activity_timeout_factor * 100 * 1000,
         pf_cmsm_timeout,
         (void *)p_ar,
         &p_ar->cmsm_timeout);
      ret = 0;
      break;
   }

   if (ret != 0)
   {
      LOG_ERROR (PNET_LOG, "CMSM(%d): Wrong state\n", __LINE__);
   }

   return ret;
}
