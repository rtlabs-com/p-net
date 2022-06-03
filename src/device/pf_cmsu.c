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
 * @brief Implements Context Management Startup Protocol Machine
 * Device (CMSU) The CMSU of an IO device arranges the start of the
 * underlying protocol machines.
 * The startup of the CPM and PPM state machines shall be done between
 * the reception of the connect request service and of the Application
 * ready service. The startup shall be finished before issuing the
 * Application ready request service.
 */

#ifdef UNIT_TEST

#endif

#include <string.h>
#include "pf_includes.h"

void pf_cmsu_init (pnet_t * net, pf_ar_t * p_ar)
{
   p_ar->cmsu_state = PF_CMSU_STATE_IDLE;
}

/**
 * @internal
 * Return a string representation of the CMSU state.
 * @param state            In:   The CMSU state
 * @return  A string representing the CMSU state.
 */
static const char * pf_cmsu_state_to_string (pf_cmsu_state_values_t state)
{
   const char * s = "<unknown>";

   switch (state)
   {
   case PF_CMSU_STATE_IDLE:
      s = "PF_CMSU_STATE_IDLE";
      break;
   case PF_CMSU_STATE_STARTUP:
      s = "PF_CMSU_STATE_STARTUP";
      break;
   case PF_CMSU_STATE_RUN:
      s = "PF_CMSU_STATE_RUN";
      break;
   case PF_CMSU_STATE_ABORT:
      s = "PF_CMSU_STATE_ABORT";
      break;
   }

   return s;
}

/**
 * @internal
 * Handle state changes in the CMSU component.
 * @param net              InOut: The p-net stack instance
 * @param state            In:   The new CMSU state.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmsu_set_state (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_cmsu_state_values_t state)
{
   LOG_DEBUG (
      PNET_LOG,
      "CMSU(%d): New state %s for AREP %u (was %s)\n",
      __LINE__,
      pf_cmsu_state_to_string (state),
      p_ar->arep,
      pf_cmsu_state_to_string (p_ar->cmsu_state));

   p_ar->cmsu_state = state;

   return 0;
}

int pf_cmsu_cmdev_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_event_values_t event)
{
   uint32_t crep;

   LOG_DEBUG (
      PNET_LOG,
      "CMSU(%d): Received event %s from CMDEV. Initial state %s for AREP %u.\n",
      __LINE__,
      pf_cmdev_event_to_string (event),
      pf_cmsu_state_to_string (p_ar->cmsu_state),
      p_ar->arep);

   switch (p_ar->cmsu_state)
   {
   case PF_CMSU_STATE_IDLE:
      break;
   case PF_CMSU_STATE_STARTUP:
      if (event == PNET_EVENT_PRMEND)
      {
         for (crep = 0; crep < p_ar->nbr_iocrs; crep++)
         {
            if (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_INPUT)
            {
               if (pf_ppm_activate_req (net, p_ar, crep) != 0)
               {
                  LOG_ERROR (
                     PNET_LOG,
                     "CMSU(%d): Failed to activate_ppm for AREP %u.\n",
                     __LINE__,
                     p_ar->arep);
               }
            }
         }
      }
      /* fallthrough */
   case PF_CMSU_STATE_RUN:
      if (event == PNET_EVENT_ABORT)
      {
         LOG_DEBUG (
            PNET_LOG,
            "CMSU(%d): Received event PNET_EVENT_ABORT from CMDEV. Closing "
            "PPM, CPM and alarm instances for AREP %u. Sending alarm with "
            "error_code2 = 0x%X\n",
            __LINE__,
            p_ar->arep,
            p_ar->err_code);
         for (crep = 0; crep < p_ar->nbr_iocrs; crep++)
         {
            if (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_INPUT)
            {
               pf_ppm_close_req (net, p_ar, crep);
            }
            if (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_OUTPUT)
            {
               pf_cpm_close_req (net, p_ar, crep);
            }
         }

         pf_alarm_close (net, p_ar);

         pf_cmdmc_close_req (p_ar);

         /* ToDo: Cleanup static ARP cache entry */
         /* ACCM_req(Command: REMOVE, ip_address) */
         pf_cmsu_set_state (net, p_ar, PF_CMSU_STATE_ABORT);
         /* All *_cnf have been received */
         pf_cmsu_set_state (net, p_ar, PF_CMSU_STATE_IDLE);
      }
      break;
   case PF_CMSU_STATE_ABORT:
   default:
      LOG_ERROR (
         PNET_LOG,
         "CMSU(%d): Illegal state in cmsu %d\n",
         __LINE__,
         p_ar->cmsu_state);
      break;
   }
   return 0;
}

int pf_cmsu_start_req (pnet_t * net, pf_ar_t * p_ar, pnet_result_t * p_stat)
{
   int ret = -1;
   uint32_t crep;

   switch (p_ar->cmsu_state)
   {
   case PF_CMSU_STATE_IDLE:
      ret = 0; /* Assume all goes well */
      for (crep = 0; crep < p_ar->nbr_iocrs; crep++)
      {
         if (ret == 0)
         {
            if (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_OUTPUT)
            {
               if (pf_cpm_create (net, p_ar, crep) != 0)
               {
                  p_stat->pnio_status.error_code_2 =
                     PNET_ERROR_CODE_2_CMSU_AR_ADD_PROV_CONS_FAILED;
                  ret = -1;
               }
               else if (pf_cpm_activate_req (net, p_ar, crep) != 0)
               {
                  p_stat->pnio_status.error_code_2 =
                     PNET_ERROR_CODE_2_CMSU_AR_ADD_PROV_CONS_FAILED;
                  ret = -1;
               }
            }
         }
         if (ret == 0)
         {
            if (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_INPUT)
            {
               if (pf_ppm_create (net, p_ar, crep) != 0)
               {
                  p_stat->pnio_status.error_code_2 =
                     PNET_ERROR_CODE_2_CMSU_AR_ADD_PROV_CONS_FAILED;
                  ret = -1;
               }
            }
         }
      }
      if ((ret == 0) && (pf_alarm_activate (net, p_ar) != 0))
      {
         p_stat->pnio_status.error_code_2 =
            PNET_ERROR_CODE_2_CMSU_AR_ALARM_OPEN_FAILED;
         ret = -1;
      }
      if ((ret == 0) && (pf_cmdmc_activate_req (p_ar) != 0))
      {
         p_stat->pnio_status.error_code_2 =
            PNET_ERROR_CODE_2_CMDEV_STATE_CONFLICT;
         ret = -1;
      }
      if ((ret == 0) && (pf_cmsm_activate (p_ar) != 0))
      {
         p_stat->pnio_status.error_code_2 =
            PNET_ERROR_CODE_2_CMSM_SIGNALED_ERROR;
         ret = -1;
      }

#if 0
      if ((ret == 0) && (pf_dfp_activate_req(p_ar) != 0))
      {
         p_stat->pnio_status.error_code_2 = PNET_ERROR_CODE_2_CMSU_AR_ADD_PROV_CONS_FAILED;
         ret = -1;
      }
#endif

      /* ToDo: Create static ARP cache entry */
      /* ACCM_req(Command: ADD, ip_address, mac_address) */

      pf_cmsu_set_state (net, p_ar, PF_CMSU_STATE_STARTUP);
      /* CMSU_Start.cnf */
      break;
   case PF_CMSU_STATE_STARTUP:
      p_stat->pnio_status.error_code_1 = PNET_ERROR_CODE_1_CMDEV;
      p_stat->pnio_status.error_code_2 = PNET_ERROR_CODE_2_CMDEV_STATE_CONFLICT;
      ret = -1;
      break;
   case PF_CMSU_STATE_RUN:
      p_stat->pnio_status.error_code_1 = PNET_ERROR_CODE_1_CMDEV;
      p_stat->pnio_status.error_code_2 = PNET_ERROR_CODE_2_CMDEV_STATE_CONFLICT;
      ret = -1;
      break;
   default:
      LOG_ERROR (
         PNET_LOG,
         "CMSU(%d): Illegal state in cmsu %d\n",
         __LINE__,
         p_ar->cmsu_state);
      break;
   }

   return ret;
}

int pf_cmsu_cmdmc_error_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint8_t err_cls,
   uint8_t err_code)
{
   p_ar->err_cls = err_cls;
   p_ar->err_code = err_code;
   return pf_cmdev_state_ind (net, p_ar, PNET_EVENT_ABORT);
}

int pf_cmsu_cpm_error_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint8_t err_cls,
   uint8_t err_code)
{
   p_ar->err_cls = err_cls;
   p_ar->err_code = err_code;
   return pf_cmdev_state_ind (net, p_ar, PNET_EVENT_ABORT);
}

int pf_cmsu_ppm_error_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint8_t err_cls,
   uint8_t err_code)
{
   p_ar->err_cls = err_cls;
   p_ar->err_code = err_code;
   return pf_cmdev_state_ind (net, p_ar, PNET_EVENT_ABORT);
}

int pf_cmsu_alarm_error_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint8_t err_cls,
   uint8_t err_code)
{
   p_ar->err_cls = err_cls;
   p_ar->err_code = err_code;
   return pf_cmdev_state_ind (net, p_ar, PNET_EVENT_ABORT);
}
