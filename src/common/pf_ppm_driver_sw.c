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

#include "pf_includes.h"

#include <string.h>
#include <inttypes.h>

/**
 * @internal
 * Send the process data frame.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * If the PPM has not been stopped during the wait, then a data message
 * is sent and the function is rescheduled.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    The IOCR instance.
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 *                                stored tasks.
 */
static void pf_ppm_drv_sw_send (pnet_t * net, void * arg, uint32_t current_time)
{
   pf_iocr_t * p_arg = (pf_iocr_t *)arg;
   uint32_t delay = 0;

   pf_scheduler_reset_handle (&p_arg->ppm.ci_timeout);
   if (p_arg->ppm.ci_running == true)
   {
      /* Insert data, status etc. The in_length is the size of input to the
       * controller */
      pf_ppm_finish_buffer (net, &p_arg->ppm, p_arg->in_length);

      /* Now send it */
      if (pf_eth_send_on_management_port (net, p_arg->ppm.p_send_buffer) > 0)
      {
         /* Schedule next execution */
         p_arg->ppm.next_exec += p_arg->ppm.control_interval;
         delay = p_arg->ppm.next_exec - current_time;
         if (
            pf_scheduler_add (
               net,
               delay,
               pf_ppm_drv_sw_send,
               arg,
               &p_arg->ppm.ci_timeout) == 0)
         {
            p_arg->ppm.trx_cnt++;
            if (p_arg->ppm.first_transmit == false)
            {
               pf_ppm_state_ind (net, p_arg->p_ar, &p_arg->ppm, false);
               p_arg->ppm.first_transmit = true;
            }
         }
         else
         {
            /* Indidate error */
            pf_ppm_state_ind (net, p_arg->p_ar, &p_arg->ppm, true);
         }
      }
   }
}

int pf_ppm_drv_sw_create (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   return 0;
}

int pf_ppm_drv_sw_activate_req (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   int ret = -1;
   pf_iocr_t * p_iocr = &p_ar->iocrs[crep];
   pf_ppm_t * p_ppm = &p_ar->iocrs[crep].ppm;

   LOG_DEBUG (
      PF_PPM_LOG,
      "PPM(%d): Start scheduling of process data frames for AREP %u CREP "
      "%" PRIu32 "\n",
      __LINE__,
      p_ar->arep,
      crep);

   pf_scheduler_init_handle (&p_ppm->ci_timeout, "ppm");
   ret = pf_scheduler_add (
      net,
      p_ppm->control_interval,
      pf_ppm_drv_sw_send,
      p_iocr,
      &p_ppm->ci_timeout);

   return ret;
}

int pf_ppm_drv_sw_close_req (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   pf_ppm_t * p_ppm = &p_ar->iocrs[crep].ppm;

   LOG_DEBUG (
      PF_PPM_LOG,
      "PPM(%d): Stop scheduling of process data frames for AREP %u CREP "
      "%" PRIu32 "\n",
      __LINE__,
      p_ar->arep,
      crep);

   pf_scheduler_remove_if_running (net, &p_ppm->ci_timeout);

   return 0;
}

static int pf_ppm_drv_sw_write_frame_buffer (
   pnet_t * net,
   pf_iocr_t * iocr,
   uint32_t offset,
   const uint8_t * data,
   uint16_t len)
{
   memcpy (&iocr->ppm.buffer_data[offset], data, len);
   return 0;
}

static int pf_ppm_drv_sw_read_frame_buffer (
   pnet_t * net,
   pf_iocr_t * iocr,
   uint32_t offset,
   uint8_t * data,
   uint16_t len)
{
   memcpy (data, &iocr->ppm.buffer_data[offset], len);
   return 0;
}

int pf_ppm_drv_sw_write_data_and_iops (
   pnet_t * net,
   pf_iocr_t * iocr,
   const pf_iodata_object_t * p_iodata,
   const uint8_t * data,
   uint16_t len,
   const uint8_t * iops,
   uint8_t iops_len)
{
   int ret = 0;
   os_mutex_lock (net->ppm_buf_lock);

   if (data != NULL)
   {
      ret = pf_ppm_drv_sw_write_frame_buffer (
         net,
         iocr,
         p_iodata->data_offset,
         data,
         len);
   }

   if (ret == 0)
   {
      ret = pf_ppm_drv_sw_write_frame_buffer (
         net,
         iocr,
         p_iodata->iops_offset,
         iops,
         iops_len);
   }
   os_mutex_unlock (net->ppm_buf_lock);

   return ret;
}

int pf_ppm_drv_sw_read_data_and_iops (
   pnet_t * net,
   pf_iocr_t * iocr,
   const pf_iodata_object_t * p_iodata,
   uint8_t * data,
   uint16_t data_len,
   uint8_t * iops,
   uint8_t iops_len)
{
   int ret;

   os_mutex_lock (net->ppm_buf_lock);

   ret = pf_ppm_drv_sw_read_frame_buffer (
      net,
      iocr,
      p_iodata->data_offset,
      data,
      data_len);
   if (ret == 0)
   {
      ret = pf_ppm_drv_sw_read_frame_buffer (
         net,
         iocr,
         p_iodata->iops_offset,
         iops,
         iops_len);
   }

   os_mutex_unlock (net->ppm_buf_lock);

   return ret;
}

int pf_ppm_drv_sw_write_iocs (
   pnet_t * net,
   pf_iocr_t * iocr,
   const pf_iodata_object_t * p_iodata,
   const uint8_t * iocs,
   uint8_t len)
{
   int ret;

   os_mutex_lock (net->ppm_buf_lock);
   ret = pf_ppm_drv_sw_write_frame_buffer (
      net,
      iocr,
      p_iodata->iocs_offset,
      iocs,
      len);
   os_mutex_unlock (net->ppm_buf_lock);

   return ret;
}

int pf_ppm_drv_sw_read_iocs (
   pnet_t * net,
   pf_iocr_t * iocr,
   const pf_iodata_object_t * p_iodata,
   uint8_t * iocs,
   uint8_t iocs_len)
{
   int ret;

   os_mutex_lock (net->ppm_buf_lock);
   ret = pf_ppm_drv_sw_read_frame_buffer (
      net,
      iocr,
      p_iodata->iocs_offset,
      iocs,
      iocs_len);
   os_mutex_unlock (net->ppm_buf_lock);

   return ret;
}

int pf_ppm_drv_sw_write_data_status (
   pnet_t * net,
   pf_iocr_t * iocr,
   uint8_t data_status)
{
   return 0;
}

void pf_ppm_drv_sw_show (const pf_ppm_t * p_ppm)
{
   printf ("pf_ppm_drv_sw_show not implemented\n");
}

void pf_ppm_driver_sw_init (pnet_t * net)
{
   static const pf_ppm_driver_t drv = {
      .create = pf_ppm_drv_sw_create,
      .activate_req = pf_ppm_drv_sw_activate_req,
      .close_req = pf_ppm_drv_sw_close_req,
      .write_data_and_iops = pf_ppm_drv_sw_write_data_and_iops,
      .read_data_and_iops = pf_ppm_drv_sw_read_data_and_iops,
      .write_iocs = pf_ppm_drv_sw_write_iocs,
      .read_iocs = pf_ppm_drv_sw_read_iocs,
      .write_data_status = pf_ppm_drv_sw_write_data_status,
      /*.read_data_status = pf_ppm_drv_sw_read_data_status, */
      .show = pf_ppm_drv_sw_show};

   net->ppm_drv = &drv;

   LOG_INFO (
      PF_PPM_LOG,
      "PPM_DRIVER_SW(%d): Default PPM driver installed\n",
      __LINE__);
}
