/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

/**
 * @file
 * @brief CPM driver for LAN9662
 *
 */

#include "pf_includes.h"
#include "pf_lan9662_mera.h"

#include <inttypes.h>
#include <string.h>

/**
 * @internal
 * Control interval timeout handler
 * This is a callback for the scheduler.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    The IOCR instance. pf_iocr_t
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 *                                stored tasks.
 */
static void pf_cpm_drv_lan9662_ci_timeout (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{
   pf_iocr_t * p_iocr = (pf_iocr_t *)arg;
   uint32_t start = os_get_current_time_us();
   uint32_t exec;
   uint64_t rx_count = 0;
   uint8_t data_status = 0;
   uint8_t changes = 0;
   pf_mera_event_t mera_event = {0};
   bool reset_dht = false;

   pf_scheduler_reset_handle (&p_iocr->cpm.ci_timeout);

   if (p_iocr->cpm.ci_running == true) /* Timer running */
   {
      if (pf_mera_cpm_read_rx_count (p_iocr->cpm.frame, &rx_count) != 0)
      {
         LOG_ERROR (
            PF_CPM_LOG,
            "CPM_DRV_MERA(%d): Failed to read frame rx counter\n",
            __LINE__);
      }
      p_iocr->cpm.recv_cnt += rx_count;

      if (pf_mera_cpm_read_data_status (p_iocr->cpm.frame, &data_status) != 0)
      {
         LOG_ERROR (
            PF_CPM_LOG,
            "CPM_DRV_MERA(%d): Failed to read data status\n",
            __LINE__);
      }

      changes = p_iocr->cpm.data_status ^ data_status;
      p_iocr->cpm.data_status = data_status;
      if (changes != 0)
      {
         LOG_INFO (
            PF_CPM_LOG,
            "CPM_DRV_MERA(%d): Data Status changed. New value 0x%x\n",
            __LINE__,
            data_status);
      }

      switch (p_iocr->cpm.state)
      {
      case PF_CPM_STATE_W_START:
      case PF_CPM_STATE_FRUN:
         if (
            (rx_count > 0) &&
            (data_status & BIT (PNET_DATA_STATUS_BIT_PROVIDER_STATE)))
         {
            LOG_DEBUG (
               PF_CPM_LOG,
               "CPM_DRV_MERA(%d): Data received and status ok\n",
               __LINE__);

            pf_cpm_state_ind (net, p_iocr->p_ar, p_iocr->crep, true);
            pf_cpm_set_state (&p_iocr->cpm, PF_CPM_STATE_RUN);

            /* Reconfigure RTE to activate DHT */
            pf_mera_cpm_activate_rte_dht (p_iocr->cpm.frame);
         }

         /* Always reset DHT while in the initial states. */

         reset_dht = true;
         break;

      case PF_CPM_STATE_RUN:

         if (changes != 0)
         {
            pf_fspm_data_status_changed (
               net,
               p_iocr->p_ar,
               p_iocr,
               changes,
               data_status);
         }

         if (rx_count > 0)
         {
            (void)pf_cmio_cpm_new_data_ind (p_iocr->p_ar, p_iocr->crep, true);
         }
         break;

      default:
         LOG_ERROR (
            PF_CPM_LOG,
            "CPM_DRV_MERA(%d): Unsupported CPM state\n",
            __LINE__);
         break;
      }

      if (pf_mera_poll_cpm_events (net, p_iocr->cpm.frame, &mera_event) != 0)
      {
         LOG_ERROR (
            PF_CPM_LOG,
            "CPM_DRV_MERA(%d): Failed to poll rtp events\n",
            __LINE__);
      }

      if (rx_count > 0 && !mera_event.data_status_mismatch)
      {
         reset_dht = true;
      }
      else
      {
         if ((BIT (PNET_DATA_STATUS_BIT_PROVIDER_STATE) & data_status) == 0)
         {
            reset_dht = true;
         }
         if (
            (BIT (PNET_DATA_STATUS_BIT_STATION_PROBLEM_INDICATOR) &
             data_status) == 0)
         {
            reset_dht = true;
         }
      }

      if (reset_dht)
      {
         p_iocr->cpm.dht = 0;
      }
      else
      {
         p_iocr->cpm.dht++;
      }

      if (p_iocr->cpm.dht >= p_iocr->cpm.data_hold_factor)
      {
         if (p_iocr->cpm.dht == p_iocr->cpm.data_hold_factor)
         {
            LOG_WARNING (
               PF_CPM_LOG,
               "CPM_DRV_MERA(%d): Data hold timer (DHT) expired\n",
               __LINE__);

            if (!mera_event.stopped)
            {
               /* Workaround for RTC testcase. */
               LOG_INFO (
                  PF_CPM_LOG,
                  "CPM_DRV_MERA(%d): Generate rtp stopped event\n",
                  __LINE__);

               mera_event.stopped = true;
            }
         }
      }

      if (mera_event.stopped)
      {
         p_iocr->p_ar->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
         p_iocr->p_ar->err_code =
            PNET_ERROR_CODE_2_ABORT_AR_CONSUMER_DHT_EXPIRED;

         p_iocr->cpm.dht = 0;
         p_iocr->cpm.ci_running = false; /* Stop timer */
         pf_cpm_state_ind (net, p_iocr->p_ar, p_iocr->crep, false);

         pf_cpm_set_state (&p_iocr->cpm, PF_CPM_STATE_W_START);
      }

      if (p_iocr->cpm.ci_running == true)
      {
         /* Timer auto-reload */
         if (
            pf_scheduler_add (
               net,
               p_iocr->cpm.control_interval,
               pf_cpm_drv_lan9662_ci_timeout,
               arg,
               &p_iocr->cpm.ci_timeout) != 0)
         {
            LOG_ERROR (
               PF_CPM_LOG,
               "CPM_DRV_MERA(%d): Timeout not started\n",
               __LINE__);
            p_iocr->p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
            p_iocr->p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID;
            pf_cmsu_cpm_error_ind (
               net,
               p_iocr->p_ar,
               p_iocr->p_ar->err_cls,
               p_iocr->p_ar->err_code);
         }
      }
   }

   exec = os_get_current_time_us() - start;
   if (exec > p_iocr->cpm.max_exec)
   {
      p_iocr->cpm.max_exec = exec;
   }
}

static int pf_cpm_drv_lan9662_create (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t crep)
{
   pf_cpm_t * p_cpm = &p_ar->iocrs[crep].cpm;

   pf_mera_frame_cfg_t mera_cpm_cfg = {0};

   LOG_DEBUG (
      PF_PPM_LOG,
      "CPM_DRV_MERA(%d): Allocate RTE frame (outbound data / rx frames) for "
      "AREP %u "
      "CREP "
      "%" PRIu32 "\n",
      __LINE__,
      p_ar->arep,
      crep);

   mera_cpm_cfg.p_iocr = &p_ar->iocrs[crep];
   mera_cpm_cfg.port = 0;

   pf_mera_get_port_from_remote_mac (
      &p_ar->ar_param.cm_initiator_mac_add,
      &mera_cpm_cfg.port);

   memcpy (
      &mera_cpm_cfg.main_mac_addr,
      &net->pf_interface.main_port.mac_address,
      sizeof (pnet_ethaddr_t));

   p_cpm->frame = pf_mera_cpm_alloc (net, &mera_cpm_cfg);

   if (p_cpm->frame == NULL)
   {
      return -1;
   }

   return 0;
}

/**
 * @internal
 * Close RTE resources used in CPM frame.
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    CPM frame
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 *                                stored tasks.
 */
static void pf_cpm_drv_lan9662_delayed_close (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{
   pf_drv_frame_t * frame = (pf_drv_frame_t *)arg;
   pf_mera_cpm_stop (net, frame);
   pf_mera_cpm_free (net, frame);
}

static int pf_cpm_drv_lan9662_close_req (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t crep)
{
   pf_cpm_t * p_cpm = &p_ar->iocrs[crep].cpm;

   p_cpm->ci_running = false;
   pf_scheduler_reset_handle (&p_cpm->ci_timeout);

   /* Delay close of pf_mera resources
    * Workaround for delayed alarm transmission.
    * Close RTE resources after alarm is sent.
    */
   pf_scheduler_init_handle (&p_cpm->ci_timeout, "cpm_close");
   pf_scheduler_add (
      net,
      2,
      pf_cpm_drv_lan9662_delayed_close,
      p_cpm->frame,
      &p_cpm->ci_timeout);

   return 0;
}

static int pf_cpm_drv_lan9662_activate_req (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t crep)
{
   int ret = -1;
   pf_cpm_t * p_cpm;
   pf_iocr_t * p_iocr;

   p_iocr = &p_ar->iocrs[crep];
   p_cpm = &p_iocr->cpm;

   LOG_DEBUG (
      PF_PPM_LOG,
      "CPM_DRV_MERA(%d): Activate RTE (outbound data / rx frames) for AREP %u "
      "CREP "
      "%" PRIu32 "\n",
      __LINE__,
      p_ar->arep,
      crep);

   ret = pf_mera_cpm_start (net, p_cpm->frame);
   if (ret == 0)
   {
      pf_scheduler_init_handle (&p_cpm->ci_timeout, "cpm");
      ret = pf_scheduler_add (
         net,
         p_cpm->control_interval,
         pf_cpm_drv_lan9662_ci_timeout,
         p_iocr,
         &p_cpm->ci_timeout);

      if (ret == 0)
      {
         p_cpm->ci_running = true;
      }
      else
      {
         LOG_ERROR (
            PF_CPM_LOG,
            "CPM_DRV_MERA(%d): Timeout not started\n",
            __LINE__);
      }
   }
   else
   {
      LOG_ERROR (
         PF_CPM_LOG,
         "CPM_DRV_MERA(%d): Failed to start RTE\n",
         __LINE__);
   }

   return ret;
}

/**
 * Read data and iops of an iodata object in the
 * active RTE frame.
 * TODO - behaviour for iodata mapped to QSPI is tbd
 */
static int pf_cpm_drv_lan9662_get_data_and_iops (
   pnet_t * net,
   pf_iocr_t * p_iocr,
   const pf_iodata_object_t * p_iodata,
   bool * p_new_flag,
   uint8_t * p_data,
   uint16_t data_len,
   uint8_t * p_iops,
   uint8_t iops_len)
{
   int ret = -1;

   ret = pf_mera_cpm_read_data_and_iops (
      net,
      p_iocr->cpm.frame,
      p_iodata->slot_nbr,
      p_iodata->subslot_nbr,
      p_new_flag,
      p_data,
      data_len,
      p_iops,
      iops_len);

   return ret;
}

/**
 * Read the consumer status for an iodata object
 * in the active RTE frame.
 * @return 0 on success, -1 on error
 */
static int pf_cpm_drv_lan9662_get_iocs (
   pnet_t * net,
   pf_iocr_t * p_iocr,
   const pf_iodata_object_t * p_iodata,
   uint8_t * p_iocs,
   uint8_t iocs_len)
{
   return pf_mera_cpm_read_iocs (
      net,
      p_iocr->cpm.frame,
      p_iodata->slot_nbr,
      p_iodata->subslot_nbr,
      p_iocs,
      iocs_len);
}

/**
 * Read data_status for active CPM frame.
 * @param p_cpm            In: CPM instance
 * @param p_data_status    InOut: Ref to data_status value
 * @return 0 on success, -1 on error
 */

static int pf_cpm_drv_lan9662_get_data_status (
   const pf_cpm_t * p_cpm,
   uint8_t * p_data_status)
{
   return pf_mera_cpm_read_data_status (p_cpm->frame, p_data_status);
}

/**
 * Show the current state of the driver
 * @param net     In: The p-net stack instance
 * @param p_ppm   In: CPM instance
 */
static void pf_cpm_drv_lan9662_show (const pnet_t * net, const pf_cpm_t * p_cpm)
{
   printf ("Not implemented:  pf_cpm_drv_mera_show()\n");
}

int pf_driver_cpm_init (pnet_t * net)
{
   static const pf_cpm_driver_t drv = {
      .create = pf_cpm_drv_lan9662_create,
      .activate_req = pf_cpm_drv_lan9662_activate_req,
      .close_req = pf_cpm_drv_lan9662_close_req,
      .get_data_and_iops = pf_cpm_drv_lan9662_get_data_and_iops,
      .get_iocs = pf_cpm_drv_lan9662_get_iocs,
      .get_data_status = pf_cpm_drv_lan9662_get_data_status,
      .show = pf_cpm_drv_lan9662_show};

   net->cpm_drv = &drv;

   LOG_INFO (
      PF_CPM_LOG,
      "CPM_DRV(%d): LAN9662 CPM driver installed\n",
      __LINE__);
   return 0;
}
