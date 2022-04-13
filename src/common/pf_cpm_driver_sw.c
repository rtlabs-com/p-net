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
 * @brief Implements the CPM default software (SW) driver
 * The driver handles receiving cyclic data. Registers a handler for incoming
 * real time data frames. Maintains a timer to monitor incoming frames.
 *
 */

#ifdef UNIT_TEST
#define os_get_current_time_us mock_os_get_current_time_us
#endif

#include "pf_includes.h"

#include <inttypes.h>
#include <string.h>

/**
 * @internal
 * The control_interval timer has expired.
 *
 * This is a callback for the scheduler. Arguments should fulfill
 * pf_scheduler_timeout_ftn_t
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    The IOCR instance. pf_iocr_t
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 *                                stored tasks.
 */
static void pf_cpm_control_interval_expired (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{
   pf_iocr_t * p_iocr = (pf_iocr_t *)arg;
   uint32_t start = os_get_current_time_us();
   uint32_t exec;

   pf_scheduler_reset_handle (&p_iocr->cpm.ci_timeout);

   if (p_iocr->cpm.ci_running == true) /* Timer running */
   {
      switch (p_iocr->cpm.state)
      {
      case PF_CPM_STATE_W_START:
      case PF_CPM_STATE_FRUN:
         break;
      case PF_CPM_STATE_RUN:
         if (p_iocr->cpm.dht >= p_iocr->cpm.data_hold_factor)
         {
            /* dht expired */
            p_iocr->p_ar->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
            p_iocr->p_ar->err_code =
               PNET_ERROR_CODE_2_ABORT_AR_CONSUMER_DHT_EXPIRED;

            p_iocr->cpm.dht = 0;
            p_iocr->cpm.ci_running = false; /* Stop timer */
            pf_cpm_state_ind (net, p_iocr->p_ar, p_iocr->crep, false); /* stop
                                                                        */

            pf_cpm_set_state (&p_iocr->cpm, PF_CPM_STATE_W_START);
         }
         else
         {
            p_iocr->cpm.dht++;
         }
         break;
      }

      if (p_iocr->cpm.ci_running == true)
      {
         /* Timer auto-reload */
         if (
            pf_scheduler_add (
               net,
               p_iocr->cpm.control_interval,
               pf_cpm_control_interval_expired,
               arg,
               &p_iocr->cpm.ci_timeout) != 0)
         {
            LOG_ERROR (
               PF_CPM_LOG,
               "CPM_DRV_SW(%d): Timeout not started\n",
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

#if PNET_OPTION_REDUNDANCY
/**
 * @internal
 * Handle that the MRWDT timer expires.
 * @param timer            In:   The timer instance.
 * @param arg              In:   The IOCR instance.
 */
void pf_cpm_mrwdt_expired (/* ToDo: Make static later */
                           os_timer_t * timer,
                           void * arg)
{
   pf_iocr_t * p_iocr = (pf_iocr_t *)arg;

   switch (p_iocr->cpm.state)
   {
   case PF_CPM_STATE_W_START:
   case PF_CPM_STATE_FRUN:
      /* Ignore */
      break;
   case PF_CPM_STATE_RUN:
      /* ToDo: Handle */
      break;
   }
}
#endif

static int pf_cpm_driver_sw_create (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   return 0;
}

static int pf_cpm_driver_sw_close_req (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t crep)
{
   pf_cpm_t * p_cpm = &p_ar->iocrs[crep].cpm;

   p_cpm->ci_running = false; /* StopTimer */
   pf_scheduler_remove_if_running (net, &p_cpm->ci_timeout);

   pf_eth_frame_id_map_remove (net, p_cpm->frame_id[0]);
   if (p_cpm->nbr_frame_id == 2)
   {
      pf_eth_frame_id_map_remove (net, p_cpm->frame_id[1]);
   }
   if (p_cpm->p_buffer_cpm != NULL)
   {
      pnal_buf_free (p_cpm->p_buffer_cpm);
   }
   if (p_cpm->p_buffer_app != NULL)
   {
      pnal_buf_free (p_cpm->p_buffer_app);
   }

   return 0;
}

/**
 * @internal
 * Replace the current buffer with a newer one and set the new_buf flag.
 * @param net              InOut: The p-net stack instance
 * @param p_cpm            InOut: The CPM instance.
 * @param pp_buf           In:    The new buffer.
 *                         Out:   The previous buffer.
 */
static void pf_cpm_put_buf (pnet_t * net, pf_cpm_t * p_cpm, pnal_buf_t ** pp_buf)
{
   void * p;

   os_mutex_lock (net->cpm_buf_lock);
   p = p_cpm->p_buffer_cpm;
   p_cpm->p_buffer_cpm = *pp_buf;
   *pp_buf = p;
   p_cpm->new_buf = true;
   os_mutex_unlock (net->cpm_buf_lock);
}

/**
 * @internal
 * Make sure that p_buffer_app points to the newest received buffer.
 * @param net              InOut: The p-net stack instance
 * @param p_cpm            InOut: The CPM instance.
 * @param p_new_flag       Out:   true if a new valid data frame has been
 *                         received.
 * @param pp_buffer        Out: A pointer to the latest received data (or NULL).
 */
static void pf_cpm_get_buf (
   pnet_t * net,
   pf_cpm_t * p_cpm,
   bool * p_new_flag,
   uint8_t ** pp_buffer)
{
   void * p;

   os_mutex_lock (net->cpm_buf_lock);
   if (p_cpm->new_buf == true)
   {
      *p_new_flag = true;
      p = p_cpm->p_buffer_app;
      p_cpm->p_buffer_app = p_cpm->p_buffer_cpm;
      p_cpm->p_buffer_cpm = p;
      p_cpm->new_buf = false;
   }
   else
   {
      *p_new_flag = false;
   }
   os_mutex_unlock (net->cpm_buf_lock);

   if (p_cpm->p_buffer_app != NULL)
   {
      *pp_buffer = &((uint8_t *)((pnal_buf_t *)p_cpm->p_buffer_app)
                        ->payload)[p_cpm->buffer_pos];
   }
   else
   {
      *pp_buffer = NULL;
   }
}

/**
 * @internal
 * Handle new incoming cyclic data frames on Ethernet.
 *
 * This is a callback for the frame handler, and is intended to be registered by
 * \a pf_eth_frame_id_map_add(). Arguments should fulfill pf_eth_frame_handler_t
 *
 * Triggers the \a pnet_new_data_status_ind() user callback on data
 * status changes.
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:   The frame id of the frame.
 * @param p_buf            In:   The received data.
 * @param frame_id_pos     In:   Position of the frame id in the buffer.
 * @param p_arg            In:   The IOCR instance. pf_iocr_t
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
static int pf_cpm_c_data_ind (
   pnet_t * net,
   uint16_t frame_id,
   pnal_buf_t * p_buf,
   uint16_t frame_id_pos,
   void * p_arg)
{
   int ret = 0; /* Means "Not handled" */
   pf_iocr_t * p_iocr = p_arg;
   pf_cpm_t * p_cpm = &p_iocr->cpm;
   uint16_t pos;
   uint16_t len;
   uint16_t cycle;
   uint8_t transfer_status;
   uint8_t data_status;
   uint8_t changes;
   uint8_t * p_ind_buf = (uint8_t *)p_buf->payload;
   bool frame_structure;
   bool c_sdu_structure;
   bool dht_reload;
   bool data_valid;
   bool primary;
   bool backup;
   bool update_data;

   p_cpm->recv_cnt++;

   switch (p_cpm->state)
   {
   case PF_CPM_STATE_W_START:
      p_iocr->p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
      p_iocr->p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID_STATE;
      p_cpm->errline = __LINE__;
      p_cpm->errcnt++;
      ret = 1; /* Means "handled" */
      break;
   case PF_CPM_STATE_FRUN:
      /* FALL-THRU */
   case PF_CPM_STATE_RUN:
      pos = p_buf->len - 4; /* cycle counter is at the end of the data */

      /* Data from the APDU */
      cycle = (uint16_t)p_ind_buf[pos] * 0x100 + p_ind_buf[pos + 1];
      data_status = p_ind_buf[pos + sizeof (uint16_t)];
      transfer_status = p_ind_buf[pos + sizeof (uint16_t) + 1];

      /* ToDo: ReceivedInRED
       * ToDo: LocalDataTransferControl
       */
      data_valid = (data_status & 0x04) != 0;
      backup = (data_status & 0x01) == 0;
      primary = (data_status & 0x01) != 0;

      /* Compute length of p_buf without VLAN tags */
      len = p_buf->len - frame_id_pos;
      len += 2 * sizeof (pnet_ethaddr_t) + sizeof (uint16_t);

      frame_structure =
         ((transfer_status == 0) &&
          (pf_cpm_check_src_addr (p_cpm, p_buf) == 0) &&
          /* Frame_id is OK - or we would not be here. */
          (len == p_cpm->buffer_length));
      c_sdu_structure = (pf_cpm_check_cycle (p_cpm->cycle, cycle) == 0);

      dht_reload =
         (frame_structure && c_sdu_structure && (data_valid || backup));
      update_data =
         (frame_structure && c_sdu_structure && data_valid &&
          (primary || backup));

      if (data_valid == false)
      {
         /* 19 */
         /* Ignore */
         LOG_DEBUG (
            PF_CPM_LOG,
            "CPM_DRV_SW(%d): data_valid == false\n",
            __LINE__);
      }
      else if (dht_reload)
      {
         if (p_cpm->state == PF_CPM_STATE_FRUN)
         {
            pf_cpm_state_ind (net, p_iocr->p_ar, p_iocr->crep, true); /* start
                                                                       */
         }

         if (update_data)
         {
            /* 20 */
            pf_cpm_put_buf (net, p_cpm, &p_buf);
            p_cpm->frame_id_pos = frame_id_pos; /* Save for consumer */
            p_cpm->buffer_pos = p_cpm->frame_id_pos + sizeof (uint16_t);
            (void)pf_cmio_cpm_new_data_ind (p_iocr->p_ar, p_iocr->crep, true);
         }
         else
         {
            /* 21 */
            (void)pf_cmio_cpm_new_data_ind (p_iocr->p_ar, p_iocr->crep, false);
         }

         /* 20, 21 */
         p_cpm->dht = 0;

         p_cpm->cycle = (int32_t)cycle;
         changes = p_cpm->data_status ^ data_status;
         p_cpm->data_status = data_status;

         if (changes != 0)
         {
            /* Notify the application about changes in the data_status */
            pf_fspm_data_status_changed (
               net,
               p_iocr->p_ar,
               p_iocr,
               changes,
               data_status);
         }
         pf_cpm_set_state (p_cpm, PF_CPM_STATE_RUN);
      }
      else
      {
         /* Ignore */
         LOG_DEBUG (
            PF_CPM_LOG,
            "CPM_DRV_SW(%d): data_valid != false && dht_reload == 0\n",
            __LINE__);
      }

      ret = 1; /* Means "handled" */
      break;
   }

   if (ret != 0)
   {
      if (p_buf != NULL)
      {
         p_cpm->free_cnt++;
         pnal_buf_free (p_buf);
      }
   }

   return ret;
}

static int pf_cpm_driver_sw_activate_req (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t crep)
{
   int ret = -1;
   pf_cpm_t * p_cpm;
   pf_iocr_t * p_iocr;

   p_iocr = &p_ar->iocrs[crep];
   p_cpm = &p_iocr->cpm;

   pf_eth_frame_id_map_add (net, p_cpm->frame_id[0], pf_cpm_c_data_ind, p_iocr);

   if (p_cpm->nbr_frame_id == 2)
   {
      p_cpm->frame_id[1] = p_cpm->frame_id[0] + 1;
      pf_eth_frame_id_map_add (
         net,
         p_cpm->frame_id[1],
         pf_cpm_c_data_ind,
         p_iocr);
   }
   pf_scheduler_init_handle (&p_cpm->ci_timeout, "cpm");
   ret = pf_scheduler_add (
      net,
      p_cpm->control_interval,
      pf_cpm_control_interval_expired,
      p_iocr,
      &p_cpm->ci_timeout);
   if (ret != 0)
   {
      LOG_ERROR (PF_CPM_LOG, "CPM_DRV_SW(%d): Timeout not started\n", __LINE__);
   }
   else
   {
      p_cpm->ci_running = true;
   }

   return ret;
}

static int pf_cpm_driver_sw_get_data_and_iops (
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

   uint8_t * p_buffer = NULL;

   /* Get the latest frame buffer */
   pf_cpm_get_buf (net, &p_iocr->cpm, p_new_flag, &p_buffer);

   if (p_buffer != NULL)
   {
      os_mutex_lock (net->cpm_buf_lock);
      if (p_iodata->data_length > 0)
      {
         memcpy (
            p_data,
            &p_buffer[p_iodata->data_offset],
            p_iodata->data_length);
      }
      if (p_iodata->iops_length > 0)
      {
         memcpy (
            p_iops,
            &p_buffer[p_iodata->iops_offset],
            p_iodata->iops_length);
      }
      os_mutex_unlock (net->cpm_buf_lock);
      ret = 0;
   }
   else
   {
      *p_new_flag = false;
      LOG_DEBUG (
         PF_CPM_LOG,
         "CPM_DRV_SW(%d): No data received in get data\n",
         __LINE__);
   }

   return ret;
}

static int pf_cpm_driver_sw_get_iocs (
   pnet_t * net,
   pf_iocr_t * p_iocr,
   const pf_iodata_object_t * p_iodata,
   uint8_t * p_iocs,
   uint8_t iocs_len)
{
   int ret = -1;
   uint8_t * p_buffer = NULL;
   bool new_flag;

   pf_cpm_get_buf (net, &p_iocr->cpm, &new_flag, &p_buffer);

   if (p_buffer != NULL)
   {
      os_mutex_lock (net->cpm_buf_lock);
      memcpy (p_iocs, &p_buffer[p_iodata->iocs_offset], p_iodata->iocs_length);
      os_mutex_unlock (net->cpm_buf_lock);
      ret = 0;
   }

   return ret;
}

static int pf_cpm_driver_sw_get_data_status (
   const pf_cpm_t * p_cpm,
   uint8_t * p_data_status)
{
   *p_data_status = p_cpm->data_status;
   return 0;
}

static void pf_cpm_driver_sw_show (const pnet_t * net, const pf_cpm_t * p_cpm)
{
   printf ("pf_cpm_driver_sw_show not implemented\n");
}

void pf_cpm_driver_sw_init (pnet_t * net)
{
   static const pf_cpm_driver_t drv = {
      .create = pf_cpm_driver_sw_create,
      .activate_req = pf_cpm_driver_sw_activate_req,
      .close_req = pf_cpm_driver_sw_close_req,
      .get_data_and_iops = pf_cpm_driver_sw_get_data_and_iops,
      .get_iocs = pf_cpm_driver_sw_get_iocs,
      .get_data_status = pf_cpm_driver_sw_get_data_status,
      .show = pf_cpm_driver_sw_show};

   net->cpm_drv = &drv;

   LOG_INFO (
      PF_CPM_LOG,
      "CPM_DRIVER_SW(%d): Default CPM driver installed\n",
      __LINE__);
}
