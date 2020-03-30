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

#include <string.h>
#include "pf_includes.h"


void pf_cmsu_init(
   pnet_t                  *net)
{
   net->cmsu_state = PF_CMSU_STATE_IDLE;
}

/**
 * @internal
 * Handle state changes in the CMSU component.
 * @param net              InOut: The p-net stack instance
 * @param state            In:   The new CMSU state.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmsu_set_state(
   pnet_t                  *net,
   pf_cmsu_state_values_t  state)
{
   net->cmsu_state = state;

   return 0;
}

int pf_cmsu_cmdev_state_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_event_values_t     event)
{
   uint32_t    crep;

   switch (net->cmsu_state)
   {
   case PF_CMSU_STATE_IDLE:
      break;
   case PF_CMSU_STATE_STARTUP:
   case PF_CMSU_STATE_RUN:
      if (event == PNET_EVENT_ABORT)
      {
         for (crep = 0; crep < p_ar->nbr_iocrs; crep++)
         {
            if (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_INPUT)
            {
               pf_ppm_close_req(net, p_ar, crep);
            }
            if (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_OUTPUT)
            {
               pf_cpm_close_req(net, p_ar, crep);
            }
         }

         pf_alarm_close(net, p_ar);

         pf_cmdmc_close_req(p_ar);

         /* ToDo: Cleanup static ARP cache entry */
         /* ACCM_req(Command: REMOVE, ip_address) */
         pf_cmsu_set_state(net, PF_CMSU_STATE_ABORT);
         /* All *_cnf have been received */
         pf_cmsu_set_state(net, PF_CMSU_STATE_IDLE);
      }
      break;
   case PF_CMSU_STATE_ABORT:
   default:
      LOG_ERROR(PNET_LOG, "CMSU(%d): Illegal state in cmsu %d\n", __LINE__, net->cmsu_state);
      break;
   }
   return 0;
}

int pf_cmsu_start_req(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_result_t           *p_stat)
{
   int         ret = -1;
   uint32_t    crep;

   switch (net->cmsu_state)
   {
   case PF_CMSU_STATE_IDLE:
      ret = 0;                /* Assume all goes well */
      for (crep = 0; crep < p_ar->nbr_iocrs; crep++)
      {
         if (ret == 0)
         {
            if (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_OUTPUT)
            {
                if (pf_cpm_create(net, p_ar, crep) != 0)
               {
                  p_stat->pnio_status.error_code_2 = PNET_ERROR_CODE_2_CMSU_AR_ADD_PROV_CONS_FAILED;
                  ret = -1;
               }
               else if (pf_cpm_activate_req(net, p_ar, crep) != 0)
               {
                  p_stat->pnio_status.error_code_2 = PNET_ERROR_CODE_2_CMSU_AR_ADD_PROV_CONS_FAILED;
                  ret = -1;
               }
            }
         }
         if (ret == 0)
         {
            if (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_INPUT)
            {
               if (pf_ppm_activate_req(net, p_ar, crep) != 0)
               {
                  p_stat->pnio_status.error_code_2 = PNET_ERROR_CODE_2_CMSU_AR_ADD_PROV_CONS_FAILED;
                  ret = -1;
               }
            }
         }

      }
      if ((ret == 0) && (pf_alarm_activate(net, p_ar) != 0))
      {
         p_stat->pnio_status.error_code_2 = PNET_ERROR_CODE_2_CMSU_AR_ALARM_OPEN_FAILED;
         ret = -1;
      }
      if ((ret == 0) && (pf_cmdmc_activate_req(p_ar) != 0))
      {
         p_stat->pnio_status.error_code_2 = PNET_ERROR_CODE_2_CMDEV_STATE_CONFLICT;
         ret = -1;
      }
      if ((ret == 0) && (pf_cmsm_activate(p_ar) != 0))
      {
         p_stat->pnio_status.error_code_2 = PNET_ERROR_CODE_2_CMSM_SIGNALED_ERROR;
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

      pf_cmsu_set_state(net, PF_CMSU_STATE_STARTUP);
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
      LOG_ERROR(PNET_LOG, "CMSU(%d): Illegal state in cmsu %d\n", __LINE__, net->cmsu_state);
      break;
   }

   return ret;
}

int pf_cmsu_cmdmc_error_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint8_t                 err_cls,
   uint8_t                 err_code)
{
   p_ar->err_cls = err_cls;
   p_ar->err_code = err_code;
   return pf_cmdev_state_ind(net, p_ar, PNET_EVENT_ABORT);
}

int pf_cmsu_cpm_error_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint8_t                 err_cls,
   uint8_t                 err_code)
{
   p_ar->err_cls = err_cls;
   p_ar->err_code = err_code;
   return pf_cmdev_state_ind(net, p_ar, PNET_EVENT_ABORT);
}

int pf_cmsu_ppm_error_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint8_t                 err_cls,
   uint8_t                 err_code)
{
   p_ar->err_cls = err_cls;
   p_ar->err_code = err_code;
   return pf_cmdev_state_ind(net, p_ar, PNET_EVENT_ABORT);
}

int pf_cmsu_alarm_error_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint8_t                 err_cls,
   uint8_t                 err_code)
{
   p_ar->err_cls = err_cls;
   p_ar->err_code = err_code;
   return pf_cmdev_state_ind(net, p_ar, PNET_EVENT_ABORT);
}
