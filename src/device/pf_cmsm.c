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

#endif


/**
 * @file
 * @brief Implements the Context Management Surveillance Protocol Machine Device (CMSM)
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

#include "pf_includes.h"

static const char          *cmsm_sync_name = "cmsm";

int pf_cmsm_activate(
   pf_ar_t                 *p_ar)
{
   if (p_ar == NULL)
   {
      return -1;
   }

   p_ar->cmsm_timer = UINT32_MAX;
   return 0;
}

/**
 * @internal
 * Return a string representation of the CMSM state.
 * @param state            In:   The CMSM state
 * @return  A string representing the CMSM state.
 */
static const char *pf_cmsm_state_to_string(
   pf_cmsm_state_values_t  state)
{
   const char              *s = "<unknown>";

   switch (state)
   {
   case PF_CMSM_STATE_IDLE:   s = "PF_CMSM_STATE_IDLE"; break;
   case PF_CMSM_STATE_RUN:    s = "PF_CMSM_STATE_RUN"; break;
   }

   return s;
}

void pf_cmsm_show(
   pf_ar_t                 *p_ar)
{
   if (p_ar == NULL)
   {
      printf("CMSM: The AR is null\n");
      return;
   }

   printf("CMSM state            = %s\n", pf_cmsm_state_to_string(p_ar->cmsm_state));
   printf("     timer            = %u\n", (unsigned)p_ar->cmsm_timer);
}

/**
 * @internal
 * Set the state of the CMSM component.
 * @param p_ar             In:   The AR instance (not NULL)
 * @param state            In:   The new state.
 */
static void pf_cmsm_set_state(
   pf_ar_t                 *p_ar,
   pf_cmsm_state_values_t  state)
{
   assert(p_ar != NULL);

   if (state != p_ar->cmsm_state)
   {
      LOG_INFO(PNET_LOG, "CMSM(%d): New state %s\n", __LINE__, pf_cmsm_state_to_string(state));
      p_ar->cmsm_state = state;
   }
}

/**
 * @internal
 * Handle timeouts in the connection monitoring.
 * Aborts the connection start-up procedure if timing out.
 *
 * This is a callback for the scheduler. Arguments should fulfill pf_scheduler_timeout_ftn_t
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:   The AR instance (not NULL). Void pointer converted to pf_ar_t
 * @param current_time     In:   The current time.
 */
static void pf_cmsm_timeout(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                current_time)
{
   assert(arg != NULL);
   pf_ar_t                 *p_ar = (pf_ar_t *)arg;

   p_ar->cmsm_timer = UINT32_MAX;
   switch (p_ar->cmsm_state)
   {
   case PF_CMSM_STATE_IDLE:
      /* Ignore */
      break;
   case PF_CMSM_STATE_RUN:
      LOG_DEBUG(PNET_LOG, "CMSM(%d): TIMEOUT!!\n", __LINE__);
      pf_cmdev_state_ind(net, p_ar, PNET_EVENT_ABORT);
      pf_cmsm_set_state(p_ar, PF_CMSM_STATE_IDLE);
      break;
   }
}

/********************** Public functions affecting the timer *****************/

int pf_cmsm_cmdev_state_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_event_values_t     event)
{
   int                     ret = -1;

   if (p_ar == NULL)
   {
      return 0;
   }

   switch (p_ar->cmsm_state)
   {
   case PF_CMSM_STATE_IDLE:
      switch (event)
      {
      case PNET_EVENT_STARTUP:
         pf_cmsm_set_state(p_ar, PF_CMSM_STATE_RUN);
         LOG_INFO(PNET_LOG, "CMSM(%d): p_ar->ar_param.cm_initiator_activity_timeout_factor = %u\n",
            __LINE__, (unsigned)p_ar->ar_param.cm_initiator_activity_timeout_factor);
         if (p_ar->cmsm_timer != UINT32_MAX)
         {
            pf_scheduler_remove(net, cmsm_sync_name, p_ar->cmsm_timer);
            p_ar->cmsm_timer = UINT32_MAX;   /* unused */
         }
         pf_scheduler_add(net, p_ar->ar_param.cm_initiator_activity_timeout_factor*100*1000,   /* time in us */
            cmsm_sync_name, pf_cmsm_timeout, (void *)p_ar, &p_ar->cmsm_timer);
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
         if (p_ar->cmsm_timer != UINT32_MAX)
         {
            pf_scheduler_remove(net, cmsm_sync_name, p_ar->cmsm_timer);
            p_ar->cmsm_timer = UINT32_MAX;   /* unused */
         }
         pf_cmsm_set_state(p_ar, PF_CMSM_STATE_IDLE);
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

int pf_cmsm_rm_read_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iod_read_request_t   *p_read_request)
{
   int                     ret = -1;

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
         if (p_ar->cmsm_timer != UINT32_MAX)
         {
            pf_scheduler_remove(net, cmsm_sync_name, p_ar->cmsm_timer);
            p_ar->cmsm_timer = UINT32_MAX;   /* unused */
         }
         if (pf_scheduler_add(net, p_ar->ar_param.cm_initiator_activity_timeout_factor*100*1000,   /* time in us */
            cmsm_sync_name, pf_cmsm_timeout, (void *)p_ar, &p_ar->cmsm_timer) != 0)
         {
            p_ar->cmsm_timer = UINT32_MAX;   /* unused */
         }
         ret = 0;
      }
      else
      {
         ret = 0;
      }
      break;
   }

   return ret;
}

int pf_cmsm_cm_read_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iod_read_request_t   *p_read_request)
{
   int                     ret = -1;

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
      if (p_ar->cmsm_timer != UINT32_MAX)
      {
         pf_scheduler_remove(net, cmsm_sync_name, p_ar->cmsm_timer);
         p_ar->cmsm_timer = UINT32_MAX;   /* unused */
      }
      if (pf_scheduler_add(net, p_ar->ar_param.cm_initiator_activity_timeout_factor*100*1000,   /* time in us */
         cmsm_sync_name, pf_cmsm_timeout, (void *)p_ar, &p_ar->cmsm_timer) != 0)
      {
         p_ar->cmsm_timer = UINT32_MAX;   /* unused */
      }
      ret = 0;
      break;
   }

   return ret;
}

int pf_cmsm_cm_write_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iod_write_request_t   *p_write_request)
{
   int                     ret = -1;

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
      if (p_ar->cmsm_timer != UINT32_MAX)
      {
         pf_scheduler_remove(net, cmsm_sync_name, p_ar->cmsm_timer);
         p_ar->cmsm_timer = UINT32_MAX;   /* unused */
      }
      if (pf_scheduler_add(net, p_ar->ar_param.cm_initiator_activity_timeout_factor*100*1000,   /* time in us */
         cmsm_sync_name, pf_cmsm_timeout, (void *)p_ar, &p_ar->cmsm_timer) != 0)
      {
         p_ar->cmsm_timer = UINT32_MAX;   /* unused */
      }
      ret = 0;
      break;
   }

   if (ret != 0)
   {
      LOG_ERROR(PNET_LOG, "CMSM(%d): Wrong state\n", __LINE__);
   }

   return ret;
}
