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
 * @brief Implements the Cyclic Consumer Provider Protocol Machine (CPM)
 *
 * A global mutex is used instead of a per-instance mutex.
 * The locking time is very low so it should not be very congested.
 * The mutex is created on the first call to pf_cpm_create and deleted on
 * the last call to pf_cpm_close.
 * Keep track of how many instances exist and delete the mutex when the
 * number reaches 0 (zero).
 *
 */


#ifdef UNIT_TEST

#endif

#include <string.h>
#include "pf_includes.h"
#include "pf_block_reader.h"

static const char          *cpm_sync_name = "cpm";

/**
 * @internal
 * Return a string representation of the CPM state.
 * @param state            In:   The CPM state.
 * @return  A string representing the CPM state.
 */
static const char *pf_cpm_state_to_string(
   pf_cpm_state_values_t   state)
{
   const char *s = "<unknown>";

   switch (state)
   {
   case PF_CPM_STATE_W_START: s = "PF_CPM_STATE_W_START"; break;
   case PF_CPM_STATE_FRUN:    s = "PF_CPM_STATE_FRUN"; break;
   case PF_CPM_STATE_RUN:     s = "PF_CPM_STATE_RUN"; break;
   }

   return s;
}

void pf_cpm_init(
   pnet_t                  *net)
{
   net->cpm_instance_cnt = ATOMIC_VAR_INIT(0);
}

/**
 * @internal
 * Change the CPM state.
 * @param p_spm            In:   The CPM instance.
 * @param state            In:   The new CPM state.
 */
static void pf_cpm_set_state(
   pf_cpm_t                *p_cpm,
   pf_cpm_state_values_t   state)
{
   if (state != p_cpm->state)
   {
      LOG_INFO(PF_CPM_LOG, "CPM(%d): New state %s\n", __LINE__, pf_cpm_state_to_string(state));
      p_cpm->state = state;
   }
}

/**
 * @internal
 * Notify other components about CPM events.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param crep             In:   The IOCR index
 * @param start            In:   Start/Stop indicator
 */
static void pf_cpm_state_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                crep,
   bool                    start)
{
   pf_cmio_cpm_state_ind(net, p_ar, crep, start);
   pf_cmdmc_cpm_state_ind(p_ar, crep, start);
}

/**
 * @internal
 * The control_interval timer has expired.
 *
 * This is a callback for the scheduler. Arguments should fulfill pf_scheduler_timeout_ftn_t
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              In:   The IOCR instance.
 * @param current_time     In:   The current device time.
 */
static void pf_cpm_control_interval_expired(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                current_time)
{
   pf_iocr_t               *p_iocr = (pf_iocr_t *)arg;
   uint32_t                start = os_get_current_time_us();
   uint32_t                exec;

   p_iocr->cpm.ci_timer = UINT32_MAX;
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
            p_iocr->p_ar->err_code = PNET_ERROR_CODE_2_ABORT_AR_CMI_TIMEOUT;

            p_iocr->cpm.dht = 0;
            p_iocr->cpm.ci_running = false;    /* Stop timer */
            pf_cpm_state_ind(net, p_iocr->p_ar, p_iocr->crep, false);   /* stop */

            pf_cpm_set_state(&p_iocr->cpm, PF_CPM_STATE_W_START);
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
         if (pf_scheduler_add(net, p_iocr->cpm.control_interval, cpm_sync_name,
            pf_cpm_control_interval_expired, arg, &p_iocr->cpm.ci_timer) != 0)
         {
            p_iocr->cpm.ci_timer = UINT32_MAX;

            LOG_ERROR(PNET_LOG, "CPM(%d): Timeout not started\n", __LINE__);
            p_iocr->p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
            p_iocr->p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID;
            pf_cmsu_cpm_error_ind(net, p_iocr->p_ar, p_iocr->p_ar->err_cls, p_iocr->p_ar->err_code);
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
void pf_cpm_mrwdt_expired(       /* ToDo: Make static later */
   os_timer_t              *timer,
   void                    *arg)
{
   pf_iocr_t               *p_iocr = (pf_iocr_t *)arg;

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

int pf_cpm_create(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                crep)
{
   uint32_t                cnt;

   cnt = atomic_fetch_add(&net->cpm_instance_cnt, 1);
   if (cnt == 0)
   {
      net->cpm_buf_lock = os_mutex_create();
   }

   pf_cpm_set_state(&p_ar->iocrs[crep].cpm, PF_CPM_STATE_W_START);

   return 0;
}

int pf_cpm_close_req(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                crep)
{
   pf_cpm_t                *p_cpm = &p_ar->iocrs[crep].cpm;
   uint32_t                cnt;

   LOG_INFO(PF_CPM_LOG, "CPM: close\n");
   p_cpm->ci_running = false;    /* StopTimer */
   if (p_cpm->ci_timer != UINT32_MAX)
   {
      pf_scheduler_remove(net, cpm_sync_name, p_cpm->ci_timer);
      p_cpm->ci_timer = UINT32_MAX;
   }
   pf_cpm_set_state(p_cpm, PF_CPM_STATE_W_START);

   pf_eth_frame_id_map_remove(net, p_cpm->frame_id[0]);
   if (p_cpm->nbr_frame_id == 2)
   {
      pf_eth_frame_id_map_remove(net, p_cpm->frame_id[1]);
   }
   if (p_cpm->p_buffer_cpm != NULL)
   {
      os_buf_free(p_cpm->p_buffer_cpm);
   }
   if (p_cpm->p_buffer_app != NULL)
   {
      os_buf_free(p_cpm->p_buffer_app);
   }

   cnt = atomic_fetch_sub(&net->cpm_instance_cnt, 1);
   if (cnt == 1)
   {
      os_mutex_destroy(net->cpm_buf_lock);
      net->cpm_buf_lock = NULL;
   }

   return 0;
}

/**
 * @internal
 * Perform the cycle counter check of the received frame.
 * @param prev             In:   The previous cycle counter.
 * @param now              In:   The current cycle counter.
 * @return  0  If the cycle check is OK.
 *          -1 if the cycle check fails.
 */
static int pf_cpm_check_cycle(
  int32_t                  prev,
  uint16_t                 now)
{
   int ret = -1;
   uint16_t diff;

   diff = (int32_t)now - (uint16_t)prev;
   if ((diff >= 1) && (diff <= 61440))
   {
      ret = 0;
   }
   else
   {
      ret = 1;
   }

   return ret;
}

/**
 * @internal
 * Perform a check of the source address of the received frame.
 * @param p_cpm            In:   The CPM instance.
 * @param p_buf            In:   The frame buffer.
 * @return
 */
static int pf_cpm_check_src_addr(
   pf_cpm_t                *p_cpm,
   os_buf_t                *p_buf)
{
   int ret = -1;

   /* Payload contains [da], [sa], <etc> */
   if (memcmp(&p_cpm->sa,
      &((uint8_t*)p_buf->payload)[sizeof(pnet_ethaddr_t)],
      sizeof(p_cpm->sa)) == 0)
   {
      ret = 0;
   }

   return ret;
}

/**
 * @internal
 * Replace the current buffer with a newer one and set the new_buf flag.
 * @param net              InOut: The p-net stack instance
 * @param p_cpm            In:   The CPM instance.
 * @param pp_buf           In:   The new buffer.
 *                         Out:  The previous buffer.
 */
static void pf_cpm_put_buf(
   pnet_t                  *net,
   pf_cpm_t                *p_cpm,
   os_buf_t                **pp_buf)
{
   void *p;

   os_mutex_lock(net->cpm_buf_lock);
   p = p_cpm->p_buffer_cpm;
   p_cpm->p_buffer_cpm = *pp_buf;
   *pp_buf = p;
   p_cpm->new_buf = true;
   os_mutex_unlock(net->cpm_buf_lock);
}

/**
 * @internal
 * Make sure that p_buffer_app points to the newest received buffer.
 * @param net              InOut: The p-net stack instance
 * @param p_cpm            In:  The CPM instance.
 * @param p_new_flag       Out: true if new data has been received.
 * @param pp_buffer        Out: A pointer to the latest received data (or NULL).
 */
static void pf_cpm_get_buf(
   pnet_t                  *net,
   pf_cpm_t                *p_cpm,
   bool                    *p_new_flag,
   uint8_t                 **pp_buffer)
{
   void *p;

   os_mutex_lock(net->cpm_buf_lock);
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
   os_mutex_unlock(net->cpm_buf_lock);

   if (p_cpm->p_buffer_app != NULL)
   {
      *pp_buffer = &((uint8_t*)((os_buf_t *)p_cpm->p_buffer_app)->payload)[p_cpm->buffer_pos];
   }
   else
   {
      *pp_buffer = NULL;
   }
}

/**
 * @internal
 * Handle new frames on Ethernet.
 *
 * This is a callback for the frame handler. Arguments should fulfill pf_eth_frame_handler_t
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:   The frame id of the frame.
 * @param p_buf            In:   The received data.
 * @param frame_id_pos     In:   Position of the frame id in the buffer.
 * @param p_arg            In:   The IOCR instance.
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
static int pf_cpm_c_data_ind(
   pnet_t                  *net,
   uint16_t                frame_id,
   os_buf_t                *p_buf,
   uint16_t                frame_id_pos,
   void                    *p_arg)
{
   int                     ret = 0;    /* Means "Not handled" */
   pf_iocr_t               *p_iocr = p_arg;
   pf_cpm_t                *p_cpm = &p_iocr->cpm;
   uint16_t                pos;
   uint16_t                len;
   uint16_t                cycle;
   uint8_t                 transfer_status;
   uint8_t                 data_status;
   uint8_t                 changes;
   uint8_t                 *p_ind_buf = (uint8_t *)p_buf->payload;
   bool                    frame_structure;
   bool                    c_sdu_structure;
   bool                    dht_reload;
   bool                    data_valid;
   bool                    primary;
   bool                    backup;
   bool                    update_data;

   p_cpm->recv_cnt++;

   switch (p_cpm->state)
   {
   case PF_CPM_STATE_W_START:
      p_iocr->p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
      p_iocr->p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID_STATE;
      p_cpm->errline = __LINE__;
      p_cpm->errcnt++;
      ret = 1;    /* Means "handled" */
      break;
   case PF_CPM_STATE_FRUN:
      /* FALL-THRU */
   case PF_CPM_STATE_RUN:
      pos = p_buf->len - 4;   /* cycle counter is at the end of the data */

      /* Data from the APDU */
      cycle = (uint16_t)p_ind_buf[pos]*0x100 + p_ind_buf[pos + 1];
      data_status = p_ind_buf[pos + sizeof(uint16_t)];
      transfer_status = p_ind_buf[pos + sizeof(uint16_t) + 1];

      /* ToDo: ReceivedInRED
       * ToDo: LocalDataTransferControl
       */
      data_valid = (data_status & 0x04) != 0;
      backup = (data_status & 0x01) == 0;
      primary = (data_status & 0x01) != 0;

      /* Compute length of p_buf without VLAN tags */
      len = p_buf->len - frame_id_pos;
      len += 2*sizeof(pnet_ethaddr_t) + sizeof(uint16_t);

      frame_structure = ((transfer_status == 0) &&
            (pf_cpm_check_src_addr(p_cpm, p_buf) == 0) &&
            /* Frame_id is OK - or we would not be here. */
            (len == p_cpm->buffer_length));
      c_sdu_structure = (pf_cpm_check_cycle(p_cpm->cycle, cycle) == 0);

      dht_reload = (frame_structure && c_sdu_structure && (data_valid || backup));
      update_data = (frame_structure && c_sdu_structure && data_valid && (primary || backup));

      if (data_valid == false)
      {
         /* 19 */
         /* Ignore */
         LOG_DEBUG(PF_PPM_LOG, "CPM(%d): data_valid == false\n", __LINE__);
      }
      else if (dht_reload)
      {
         if (p_cpm->state == PF_CPM_STATE_FRUN)
         {
            pf_cpm_state_ind(net, p_iocr->p_ar, p_iocr->crep, true);   /* start */
         }

         if (update_data)
         {
            /* 20 */
            pf_cpm_put_buf(net, p_cpm, &p_buf);
            p_cpm->frame_id_pos = frame_id_pos; /* Save for consumer */
            p_cpm->buffer_pos = p_cpm->frame_id_pos + sizeof(uint16_t);
            (void)pf_cmio_cpm_new_data_ind(p_iocr->p_ar, p_iocr->crep, true);
         }
         else
         {
            /* 21 */
            (void)pf_cmio_cpm_new_data_ind(p_iocr->p_ar, p_iocr->crep, false);
         }

         /* 20, 21 */
         p_cpm->dht = 0;

         p_cpm->cycle = (int32_t)cycle;
         changes = p_cpm->data_status ^ data_status;
         p_cpm->data_status = data_status;

         if (changes != 0)
         {
            /* Notify the application about changes in the data_status */
            (void)pf_fspm_data_status_changed(net, p_iocr->p_ar, p_iocr, changes, data_status);
         }
         pf_cpm_set_state(p_cpm, PF_CPM_STATE_RUN);
      }
      else
      {
         /* Ignore */
         LOG_DEBUG(PF_PPM_LOG, "CPM(%d): data_valid != false && dht_reload == 0\n", __LINE__);
      }

      ret = 1;    /* Means "handled" */
      break;
   }

   if (ret != 0)
   {
      if (p_buf != NULL)
      {
         p_cpm->free_cnt++;
         os_buf_free(p_buf);
      }
   }

   return ret;
}

int pf_cpm_udp_c_data_ind(
   uint16_t                frame_id,
   os_buf_t                *p_buf)
{
   /* ToDo: Handle RT_CLASS_UDP */
   return 0;
}

int pf_cpm_activate_req(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                crep)
{
   int                     ret = -1;
   uint16_t                ix;
   pf_cpm_t                *p_cpm;
   pf_iocr_t               *p_iocr;

   p_iocr = &p_ar->iocrs[crep];
   p_cpm = &p_iocr->cpm;
   switch (p_cpm->state)
   {
   case PF_CPM_STATE_W_START:
      p_cpm->data_hold_factor = p_iocr->param.data_hold_factor;

      p_cpm->control_interval = ((uint32_t)p_iocr->param.send_clock_factor *
            (uint32_t)p_iocr->param.reduction_ratio * 1000U) / 32U;   /* us */

      for (ix = 0; ix < PNET_MAX_PORT; ix++)
      {
         p_cpm->rxa[ix][0] = -1;                                  /* "invalid" cycle counter */
         p_cpm->rxa[ix][1] = -1;
      }
      p_cpm->cycle = -1;                                          /* "invalid" */
      p_cpm->new_data = false;

      p_cpm->dht = 0;
      p_cpm->recv_cnt = 0;

      memcpy(&p_cpm->sa, &p_ar->ar_param.cm_initiator_mac_add, sizeof(p_cpm->sa));

      p_cpm->buffer_pos = 2*sizeof(pnet_ethaddr_t) +              /* ETH src and dest addr */
            sizeof(uint16_t) +                                    /* LT */
            sizeof(uint16_t);                                     /* FrameId */
      p_cpm->data_status = 0;
      p_cpm->buffer_length = p_cpm->buffer_pos +                  /* ETH frame header */
            p_iocr->param.c_sdu_length +                          /* Profinet data length */
            sizeof(uint16_t) +                                    /* cycle counter */
            1 +                                                   /* data status */
            1;                                                    /* transfer status */

      p_cpm->nbr_frame_id = 1;   /* ToDo: For now. More for RTC_3 */

      p_cpm->frame_id[0] = p_iocr->param.frame_id;
      pf_eth_frame_id_map_add(net, p_cpm->frame_id[0], pf_cpm_c_data_ind, p_iocr);

      if (p_cpm->nbr_frame_id == 2)
      {
         p_cpm->frame_id[1] = p_cpm->frame_id[0] + 1;
         pf_eth_frame_id_map_add(net, p_cpm->frame_id[1], pf_cpm_c_data_ind, &p_iocr);
      }

      /* ToDo: Shall be aligned with local send clock or PTCP (Does it matter for RTClass1/2?) */
      pf_cpm_set_state(p_cpm, PF_CPM_STATE_FRUN);
      p_cpm->ci_running = true;
      ret = pf_scheduler_add(net, p_cpm->control_interval, cpm_sync_name,
         pf_cpm_control_interval_expired, p_iocr, &p_cpm->ci_timer);
      if (ret != 0)
      {
         p_cpm->ci_timer = UINT32_MAX;

         LOG_ERROR(PNET_LOG, "CPM(%d): Timeout not started\n", __LINE__);
         p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
         p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID;
         pf_cmsu_cpm_error_ind(net, p_ar, p_ar->err_cls, p_ar->err_code);
      }
      /* pf_cpm_activate_cnf */
      break;
   case PF_CPM_STATE_FRUN:
   case PF_CPM_STATE_RUN:
      p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
      p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID_STATE;
      break;
   default:
      LOG_ERROR(PF_CPM_LOG, "CPM(%d): Illegal state in cpm[%d] %d\n", __LINE__, (int)crep, p_cpm->state);
      p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
      p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID_STATE;
      break;
   }

   return ret;
}

/**
 * @internal
 * Find the AR, input IOCR and IODATA object instances for the specified sub-slot.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API id.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param pp_ar            Out:  The AR instance.
 * @param pp_iocr          Out:  The IOCR instance.
 * @param pp_iodata        Out:  The IODATA object instance.
 * @return  0  If the information has been found.
 *          -1 If the information was not found.
 */
static int pf_cpm_get_ar_iocr_desc(
   pnet_t                  *net,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   pf_ar_t                 **pp_ar,
   pf_iocr_t               **pp_iocr,
   pf_iodata_object_t      **pp_iodata)
{
   int                     ret = -1;
   uint32_t                crep;
   uint16_t                iodata_ix;
   pf_subslot_t            *p_subslot = NULL;
   pf_ar_t                 *p_ar = NULL;
   pf_iocr_t               *p_iocr = NULL;
   bool                    found = false;

   if (pf_cmdev_get_subslot_full(net, api_id, slot_nbr, subslot_nbr, &p_subslot) == 0)
   {
      p_ar = p_subslot->p_ar;
   }

   if (p_ar == NULL)
   {
      LOG_DEBUG(PF_PPM_LOG, "CPM(%d): No AR set in sub-slot\n", __LINE__);
   }
   else
   {
      /*
       * Search the AR for an INPUT CR or an MC provider CR containing the
       * sub-slot.
       */
      for (crep = 0; ((found == false) && (crep < p_ar->nbr_iocrs)); crep++)
      {
         if ((p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_OUTPUT) ||
             (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_MC_CONSUMER))
         {
            p_iocr = &p_ar->iocrs[crep];
            for (iodata_ix = 0; ((found == false) && (iodata_ix < p_iocr->nbr_data_desc)); iodata_ix++)
            {
               if ((p_iocr->data_desc[iodata_ix].in_use == true) &&
                   (p_iocr->data_desc[iodata_ix].api_id == api_id) &&
                   (p_iocr->data_desc[iodata_ix].slot_nbr == slot_nbr) &&
                   (p_iocr->data_desc[iodata_ix].subslot_nbr == subslot_nbr))
               {
                  *pp_iodata = &p_iocr->data_desc[iodata_ix];
                  *pp_iocr = p_iocr;
                  *pp_ar = p_ar;
                  found = true;
               }
            }
         }
      }
   }

   if (found == true)
   {
      ret = 0;
   }

   return ret;
}

int pf_cpm_get_data_and_iops(
   pnet_t                  *net,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   bool                    *p_new_flag,
   uint8_t                 *p_data,
   uint16_t                *p_data_len,
   uint8_t                 *p_iops,
   uint8_t                 *p_iops_len)
{
   int                     ret = -1;
   pf_iocr_t               *p_iocr = NULL;
   pf_iodata_object_t      *p_iodata = NULL;
   pf_ar_t                 *p_ar = NULL;
   uint8_t                 *p_buffer = NULL;

   if (pf_cpm_get_ar_iocr_desc(net, api_id, slot_nbr, subslot_nbr, &p_ar, &p_iocr, &p_iodata) == 0)
   {
      switch (p_iocr->cpm.state)
      {
      case PF_CPM_STATE_W_START:
         p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
         p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID_STATE;
         LOG_DEBUG(PF_CPM_LOG, "CPM(%d): Get data in wrong state: %u\n", __LINE__, p_iocr->cpm.state);
         break;
      case PF_CPM_STATE_FRUN:
      case PF_CPM_STATE_RUN:
         if (*p_data_len < p_iodata->data_length)
         {
            *p_data_len = 0;
            *p_new_flag = false;
            LOG_ERROR(PF_CPM_LOG, "CPM(%d): Buffer too small in get data\n", __LINE__);
         }
         else
         {
            pf_cpm_get_buf(net, &p_iocr->cpm, p_new_flag, &p_buffer);

            if (p_buffer != NULL)
            {
               os_mutex_lock(net->cpm_buf_lock);
               if (p_iodata->data_length > 0)
               {
                  memcpy(p_data, &p_buffer[p_iodata->data_offset], p_iodata->data_length);
               }
               if (p_iodata->iops_length > 0)
               {
                  memcpy(p_iops, &p_buffer[p_iodata->data_offset + p_iodata->data_length], p_iodata->iops_length);
               }
               os_mutex_unlock(net->cpm_buf_lock);

               *p_data_len = p_iodata->data_length;
               *p_iops_len = (uint8_t)p_iodata->iops_length,
               ret = 0;
            }
            else
            {
               *p_data_len = 0;
               *p_new_flag = false;
               LOG_DEBUG(PF_CPM_LOG, "CPM(%d): No data received in get data\n", __LINE__);
            }
         }
         break;
      default:
         LOG_DEBUG(PF_CPM_LOG, "CPM(%d): Set data in wrong state: %u\n", __LINE__, p_iocr->cpm.state);
         break;
      }
   }
   else
   {
      /* May happen after an ABORT */
      LOG_DEBUG(PF_CPM_LOG, "CPM(%d): No data descriptor found in set data\n", __LINE__);
   }

   return ret;
}

int pf_cpm_get_iocs(
   pnet_t                  *net,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint8_t                 *p_iocs,
   uint8_t                 *p_iocs_len)
{
   int                     ret = -1;
   pf_iocr_t               *p_iocr = NULL;
   pf_iodata_object_t      *p_iodata = NULL;
   pf_ar_t                 *p_ar = NULL;
   uint8_t                 *p_buffer = NULL;
   bool                    new_flag = false;

   if (pf_cpm_get_ar_iocr_desc(net, api_id, slot_nbr, subslot_nbr, &p_ar, &p_iocr, &p_iodata) == 0)
   {
      switch (p_iocr->cpm.state)
      {
      case PF_CPM_STATE_W_START:
         p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
         p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID_STATE;
         LOG_DEBUG(PF_CPM_LOG, "CPM(%d): Get iocs in wrong state: %u\n", __LINE__, p_iocr->cpm.state);
         break;
      case PF_CPM_STATE_FRUN:
      case PF_CPM_STATE_RUN:
         if (p_iodata->iocs_length == 0)
         {
            LOG_DEBUG(PF_CPM_LOG, "CPM(%d): iocs_length is zero in get iocs\n", __LINE__);
         }
         else
         {
            pf_cpm_get_buf(net, &p_iocr->cpm, &new_flag, &p_buffer);

            if (p_buffer != NULL)
            {
               os_mutex_lock(net->cpm_buf_lock);
               memcpy(p_iocs, &p_buffer[p_iodata->iocs_offset], p_iodata->iocs_length);
               os_mutex_unlock(net->cpm_buf_lock);

               *p_iocs_len = (uint8_t)p_iodata->iocs_length,
               ret = 0;
            }
            else
            {
               LOG_DEBUG(PF_CPM_LOG, "CPM(%d): No data received in get iocs\n", __LINE__);
            }
         }
         break;
      default:
         LOG_DEBUG(PF_CPM_LOG, "CPM(%d): Get iocs in wrong state: %u\n", __LINE__, (unsigned)p_iocr->cpm.state);
         break;
      }
   }
   else
   {
      /* May happen after an ABORT */
      LOG_DEBUG(PF_CPM_LOG, "CPM(%d): No data descriptor found in get iocs\n", __LINE__);
   }

   return ret;
}

int pf_cpm_get_data_status(
   pf_cpm_t                *p_cpm,
   uint8_t                 *p_data_status)
{
   *p_data_status = p_cpm->data_status;

   return 0;
}

void pf_cpm_show(
   pnet_t                  *net,
   pf_cpm_t                *p_cpm)
{
   printf("cpm:\n");
   printf("   instance_cnt       = %u\n", (unsigned)net->cpm_instance_cnt);
   printf("   state              = %s\n", pf_cpm_state_to_string(p_cpm->state));
   printf("   max_exec           = %u\n", (unsigned)p_cpm->max_exec);
   printf("   errline            = %u\n", (unsigned)p_cpm->errline);
   printf("   errcnt             = %u\n", (unsigned)p_cpm->errcnt);
   printf("   frame_id           = %u\n", (unsigned)p_cpm->frame_id[0]);
   printf("   data_hold_factor   = %u\n", (unsigned)p_cpm->data_hold_factor);
   printf("   dHT                = %u\n", (unsigned)p_cpm->dht);
   printf("   control_interval   = %i\n", (int)p_cpm->control_interval);
   printf("   cycle              = %i\n", (int)p_cpm->cycle);
   printf("   recv_cnt           = %u\n", (unsigned)p_cpm->recv_cnt);
   printf("   free_cnt           = %u\n", (unsigned)p_cpm->free_cnt);
   printf("   p_buffer_app       = %p\n", p_cpm->p_buffer_app);
   printf("   p_buffer_cpm       = %p\n", p_cpm->p_buffer_cpm);
   printf("   p_buffer->len      = %u\n", p_cpm->p_buffer_cpm ? ((os_buf_t *)(p_cpm->p_buffer_cpm))->len : 0);
   printf("   new_buf            = %u\n", (unsigned)p_cpm->new_buf);
   printf("   ci_running         = %u\n", (unsigned)p_cpm->ci_running);
   printf("   ci_timer           = %u\n", (unsigned)p_cpm->ci_timer);
   printf("   buffer_status      = %x\n", (unsigned)p_cpm->data_status);
   printf("   buffer_length      = %u\n", (unsigned)p_cpm->buffer_length);
   printf("   buffer_pos         = %u\n", (unsigned)p_cpm->buffer_pos);
}
