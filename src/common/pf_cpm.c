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
 * This handles receiving cyclic data. Registers a handler for incoming
 * real time data frames. Maintains a timer to monitor incoming frames.
 *
 * States are W_START, FRUN and RUN.
 *
 * A global mutex is used instead of a per-instance mutex.
 * The locking time is very low so it should not be very congested.
 * The mutex is created on the first call to pf_cpm_create and deleted on
 * the last call to pf_cpm_close.
 * Keep track of how many instances exist and delete the mutex when the
 * number reaches 0 (zero).
 *
 * The the received frames are handled by a CPM driver. The driver
 * configures the reception for the specific frame IDs, checks incoming
 * frames and implements timeouts. The driver also provides functions
 * for accessing cyclic data and data status.
 * The driver is installed in the CPM init function.
 *
 */

#ifdef UNIT_TEST
#endif

#include "pf_includes.h"

#include <inttypes.h>
#include <string.h>

#include "pf_includes.h"
#include "pf_block_reader.h"

/**
 * @internal
 * Return a string representation of the CPM state.
 * @param state            In:   The CPM state.
 * @return  A string representing the CPM state.
 */
static const char * pf_cpm_state_to_string (pf_cpm_state_values_t state)
{
   const char * s = "<unknown>";

   switch (state)
   {
   case PF_CPM_STATE_W_START:
      s = "PF_CPM_STATE_W_START";
      break;
   case PF_CPM_STATE_FRUN:
      s = "PF_CPM_STATE_FRUN";
      break;
   case PF_CPM_STATE_RUN:
      s = "PF_CPM_STATE_RUN";
      break;
   }

   return s;
}

void pf_cpm_init (pnet_t * net)
{
   net->cpm_instance_cnt = ATOMIC_VAR_INIT (0);

   LOG_DEBUG (PF_CPM_LOG, "CPM(%d): Init driver\n", __LINE__);

#if PNET_OPTION_DRIVER_ENABLE
   if (net->fspm_cfg.driver_enable)
   {
      pf_driver_cpm_init (net);
   }
   else
   {
      pf_cpm_driver_sw_init (net);
   }
#else
   pf_cpm_driver_sw_init (net);
#endif
}

void pf_cpm_set_state (pf_cpm_t * p_cpm, pf_cpm_state_values_t state)
{
   if (state != p_cpm->state)
   {
      LOG_DEBUG (
         PF_CPM_LOG,
         "CPM(%d): New state %s (was %s)\n",
         __LINE__,
         pf_cpm_state_to_string (state),
         pf_cpm_state_to_string (p_cpm->state));
      p_cpm->state = state;
   }
}

void pf_cpm_state_ind (pnet_t * net, pf_ar_t * p_ar, uint32_t crep, bool start)
{
   pf_cmio_cpm_state_ind (net, p_ar, crep, start);
   pf_cmdmc_cpm_state_ind (p_ar, crep, start);
}

int pf_cpm_create (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   int ret;
   uint32_t cnt;

   LOG_DEBUG (
      PF_CPM_LOG,
      "CPM(%d): Create CPM instance. AREP %u "
      "CREP %" PRIu32 "\n",
      __LINE__,
      p_ar->arep,
      crep);

   cnt = atomic_fetch_add (&net->cpm_instance_cnt, 1);
   if (cnt == 0)
   {
      net->cpm_buf_lock = os_mutex_create();
   }

   pf_cpm_set_state (&p_ar->iocrs[crep].cpm, PF_CPM_STATE_W_START);
   ret = net->cpm_drv->create (net, p_ar, crep);
   if (ret != 0)
   {
      LOG_FATAL (
         PF_CPM_LOG,
         "CPM(%d): Driver create request failed\n",
         __LINE__);
   }

   return ret;
}

int pf_cpm_close_req (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   int ret;
   uint32_t cnt;

   LOG_DEBUG (
      PF_CPM_LOG,
      "CPM(%d): Closing CPM for AREP %u CREP %" PRIu32 "\n",
      __LINE__,
      p_ar->arep,
      crep);

   pf_cpm_set_state (&p_ar->iocrs[crep].cpm, PF_CPM_STATE_W_START);

   ret = net->cpm_drv->close_req (net, p_ar, crep);
   if (ret == 0)
   {
      cnt = atomic_fetch_sub (&net->cpm_instance_cnt, 1);
      if (cnt == 1)
      {
         os_mutex_destroy (net->cpm_buf_lock);
         net->cpm_buf_lock = NULL;
      }
   }
   else
   {
      LOG_FATAL (PF_CPM_LOG, "CPM(%d): Driver close request failed\n", __LINE__);
   }

   return ret;
}

int pf_cpm_check_cycle (int32_t prev, uint16_t now)
{
   int ret = -1;
   uint16_t diff;

   diff = (int32_t)now - (uint16_t)prev;
   if ((diff >= 1) && (diff <= 61440))
   {
      ret = 0;
   }

   return ret;
}

int pf_cpm_check_src_addr (const pf_cpm_t * p_cpm, const pnal_buf_t * p_buf)
{
   int ret = -1;

   /* Payload contains [da], [sa], <etc> */
   if (
      memcmp (
         &p_cpm->sa,
         &((uint8_t *)p_buf->payload)[sizeof (pnet_ethaddr_t)],
         sizeof (p_cpm->sa)) == 0)
   {
      ret = 0;
   }

   return ret;
}

int pf_cpm_activate_req (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   int ret = -1;
   uint16_t ix;
   pf_cpm_t * p_cpm;
   pf_iocr_t * p_iocr;

   p_iocr = &p_ar->iocrs[crep];
   p_cpm = &p_iocr->cpm;

   LOG_DEBUG (
      PF_CPM_LOG,
      "CPM(%d): Activating CPM for output data reception. "
      "AREP %u CREP %" PRIu32 " FrameID: 0x%04x\n",
      __LINE__,
      p_ar->arep,
      crep,
      p_iocr->param.frame_id);

   switch (p_cpm->state)
   {
   case PF_CPM_STATE_W_START:
      p_cpm->data_hold_factor = p_iocr->param.data_hold_factor;

      p_cpm->control_interval =
         ((uint32_t)p_iocr->param.send_clock_factor *
          (uint32_t)p_iocr->param.reduction_ratio * 1000U) /
         32U; /* us */

      for (ix = 0; ix < pf_port_get_number_of_ports (net); ix++)
      {
         p_cpm->rxa[ix][0] = -1; /* "invalid" cycle counter */
         p_cpm->rxa[ix][1] = -1;
      }
      p_cpm->cycle = -1; /* "invalid" */
      p_cpm->new_data = false;

      p_cpm->dht = 0;
      p_cpm->recv_cnt = 0;

      memcpy (
         &p_cpm->sa,
         &p_ar->ar_param.cm_initiator_mac_add,
         sizeof (p_cpm->sa));

      p_cpm->buffer_pos = 2 * sizeof (pnet_ethaddr_t) + /* ETH src and dest addr
                                                         */
                          sizeof (uint16_t) +           /* LT */
                          sizeof (uint16_t);            /* FrameId */
      p_cpm->data_status = 0;
      p_cpm->buffer_length = p_cpm->buffer_pos +          /* ETH frame header */
                             p_iocr->param.c_sdu_length + /* Profinet data
                                                             length */
                             sizeof (uint16_t) +          /* cycle counter */
                             1 +                          /* data status */
                             1;                           /* transfer status */

      p_cpm->nbr_frame_id = 1; /* ToDo: For now. More for RTC_3 */
      p_cpm->frame_id[0] = p_iocr->param.frame_id;

      ret = net->cpm_drv->activate_req (net, p_ar, crep);
      if (ret == 0)
      {
         pf_cpm_set_state (p_cpm, PF_CPM_STATE_FRUN);
      }
      else
      {
         LOG_ERROR (
            PF_CPM_LOG,
            "CPM(%d): Driver activate request failed\n",
            __LINE__);

         p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
         p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID;
         pf_cmsu_cpm_error_ind (net, p_ar, p_ar->err_cls, p_ar->err_code);
      }
      /* pf_cpm_activate_cnf */
      break;
   case PF_CPM_STATE_FRUN:
   case PF_CPM_STATE_RUN:
      p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
      p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID_STATE;
      break;
   default:
      LOG_ERROR (
         PF_CPM_LOG,
         "CPM(%d): Illegal state in cpm[%d] %d\n",
         __LINE__,
         (int)crep,
         p_cpm->state);
      p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
      p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID_STATE;
      break;
   }

   return ret;
}

/**
 * @internal
 * Find the AR, output IOCR and IODATA object instances for the specified
 * sub-slot.
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
static int pf_cpm_get_ar_iocr_desc (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   pf_ar_t ** pp_ar,
   pf_iocr_t ** pp_iocr,
   pf_iodata_object_t ** pp_iodata)
{
   int ret = -1;
   uint32_t crep;
   uint16_t iodata_ix;
   pf_subslot_t * p_subslot = NULL;
   pf_ar_t * p_ar = NULL;
   pf_iocr_t * p_iocr = NULL;
   bool found = false;

   if (
      pf_cmdev_get_subslot_full (net, api_id, slot_nbr, subslot_nbr, &p_subslot) ==
      0)
   {
      p_ar = p_subslot->p_ar;
   }

   if (p_ar == NULL)
   {
      LOG_DEBUG (PF_CPM_LOG, "CPM(%d): No AR set in sub-slot\n", __LINE__);
   }
   else
   {
      /*
       * Search the AR for an OUTPUT CR or an MC consumer CR containing the
       * sub-slot.
       */
      for (crep = 0; ((found == false) && (crep < p_ar->nbr_iocrs)); crep++)
      {
         if (
            (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_OUTPUT) ||
            (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_MC_CONSUMER))
         {
            p_iocr = &p_ar->iocrs[crep];
            for (iodata_ix = 0;
                 ((found == false) && (iodata_ix < p_iocr->nbr_data_desc));
                 iodata_ix++)
            {
               if (
                  (p_iocr->data_desc[iodata_ix].in_use == true) &&
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

int pf_cpm_get_data_and_iops (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   bool * p_new_flag,
   uint8_t * p_data,
   uint16_t * p_data_len,
   uint8_t * p_iops,
   uint8_t * p_iops_len)
{
   int ret = -1;
   pf_iocr_t * p_iocr = NULL;
   pf_iodata_object_t * p_iodata = NULL;
   pf_ar_t * p_ar = NULL;

   if (
      pf_cpm_get_ar_iocr_desc (
         net,
         api_id,
         slot_nbr,
         subslot_nbr,
         &p_ar,
         &p_iocr,
         &p_iodata) == 0)
   {
      switch (p_iocr->cpm.state)
      {
      case PF_CPM_STATE_W_START:
         p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
         p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID_STATE;
         LOG_DEBUG (
            PF_CPM_LOG,
            "CPM(%d): Get data in wrong state: %u for AREP %u\n",
            __LINE__,
            p_iocr->cpm.state,
            p_ar->arep);
         break;
      case PF_CPM_STATE_FRUN:
      case PF_CPM_STATE_RUN:
         if (
            (*p_data_len < p_iodata->data_length) ||
            (*p_iops_len < p_iodata->iops_length))
         {
            *p_data_len = 0;
            *p_new_flag = false;
            LOG_ERROR (
               PF_CPM_LOG,
               "CPM(%d): Given data buffer size %u and IOPS buffer size "
               "%u, but minimum sizes are %u and %u for slot %u subslot "
               "0x%04x\n",
               __LINE__,
               (unsigned)*p_data_len,
               (unsigned)*p_iops_len,
               (unsigned)p_iodata->data_length,
               (unsigned)p_iodata->iops_length,
               slot_nbr,
               subslot_nbr);
         }
         else
         {
            *p_data_len = p_iodata->data_length;
            *p_iops_len = p_iodata->iops_length;

            ret = net->cpm_drv->get_data_and_iops (
               net,
               p_iocr,
               p_iodata,
               p_new_flag,
               p_data,
               *p_data_len,
               p_iops,
               *p_iops_len);

            if (ret != 0)
            {
               *p_data_len = 0;
               *p_iops_len = 0;
            }
         }
         break;
      default:
         LOG_DEBUG (
            PF_CPM_LOG,
            "CPM(%d): Set data in wrong state: %u for AREP %u\n",
            __LINE__,
            p_iocr->cpm.state,
            p_ar->arep);
         break;
      }
   }
   else
   {
      /* May happen after an ABORT */
      LOG_DEBUG (
         PF_CPM_LOG,
         "CPM(%d): No data descriptor found in set data\n",
         __LINE__);
   }

   return ret;
}

int pf_cpm_get_iocs (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint8_t * p_iocs,
   uint8_t * p_iocs_len)
{
   int ret = -1;
   pf_iocr_t * p_iocr = NULL;
   pf_iodata_object_t * p_iodata = NULL;
   pf_ar_t * p_ar = NULL;

   if (
      pf_cpm_get_ar_iocr_desc (
         net,
         api_id,
         slot_nbr,
         subslot_nbr,
         &p_ar,
         &p_iocr,
         &p_iodata) == 0)
   {
      switch (p_iocr->cpm.state)
      {
      case PF_CPM_STATE_W_START:
         p_ar->err_cls = PNET_ERROR_CODE_1_CPM;
         p_ar->err_code = PNET_ERROR_CODE_2_CPM_INVALID_STATE;
         LOG_DEBUG (
            PF_CPM_LOG,
            "CPM(%d): Get iocs in wrong state: %u for AREP %u\n",
            __LINE__,
            p_iocr->cpm.state,
            p_ar->arep);
         break;
      case PF_CPM_STATE_FRUN:
      case PF_CPM_STATE_RUN:
         if (*p_iocs_len < p_iodata->iocs_length)
         {
            LOG_ERROR (
               PF_CPM_LOG,
               "CPM(%d): Given IOCS buffer size %u, but minimum size is %u "
               "for slot %u subslot 0x%04x\n",
               __LINE__,
               (unsigned)*p_iocs_len,
               (unsigned)p_iodata->iocs_length,
               slot_nbr,
               subslot_nbr);
         }
         else if (p_iodata->iocs_length == 0)
         {
            LOG_DEBUG (
               PF_CPM_LOG,
               "CPM(%d): iocs_length is zero in get iocs\n",
               __LINE__);
         }
         else
         {
            *p_iocs_len = p_iodata->iocs_length;
            ret = net->cpm_drv
                     ->get_iocs (net, p_iocr, p_iodata, p_iocs, *p_iocs_len);
         }
         break;
      default:
         LOG_DEBUG (
            PF_CPM_LOG,
            "CPM(%d): Get iocs in wrong state: %u for AREP %u\n",
            __LINE__,
            (unsigned)p_iocr->cpm.state,
            p_ar->arep);
         break;
      }
   }
   else
   {
      /* May happen after an ABORT */
      LOG_DEBUG (
         PF_CPM_LOG,
         "CPM(%d): No data descriptor found in get iocs\n",
         __LINE__);
   }

   return ret;
}

int pf_cpm_get_data_status (const pf_cpm_t * p_cpm, uint8_t * p_data_status)
{
   *p_data_status = p_cpm->data_status;

   return 0;
}

void pf_cpm_show (const pnet_t * net, const pf_cpm_t * p_cpm)
{
   printf ("cpm:\n");
   printf ("   instance_cnt       = %u\n", (unsigned)net->cpm_instance_cnt);
   printf (
      "   state              = %s\n",
      pf_cpm_state_to_string (p_cpm->state));
   printf ("   max_exec           = %u\n", (unsigned)p_cpm->max_exec);
   printf ("   errline            = %u\n", (unsigned)p_cpm->errline);
   printf ("   errcnt             = %u\n", (unsigned)p_cpm->errcnt);
   printf ("   frame_id           = %u\n", (unsigned)p_cpm->frame_id[0]);
   printf ("   data_hold_factor   = %u\n", (unsigned)p_cpm->data_hold_factor);
   printf ("   dHT                = %u\n", (unsigned)p_cpm->dht);
   printf ("   control_interval   = %i\n", (int)p_cpm->control_interval);
   printf ("   cycle              = %i\n", (int)p_cpm->cycle);
   printf ("   recv_cnt           = %u\n", (unsigned)p_cpm->recv_cnt);
   printf ("   free_cnt           = %u\n", (unsigned)p_cpm->free_cnt);
   printf ("   p_buffer_app       = %p\n", p_cpm->p_buffer_app);
   printf ("   p_buffer_cpm       = %p\n", p_cpm->p_buffer_cpm);
   printf (
      "   p_buffer->len      = %u\n",
      p_cpm->p_buffer_cpm ? ((pnal_buf_t *)(p_cpm->p_buffer_cpm))->len : 0);
   printf ("   new_buf            = %u\n", (unsigned)p_cpm->new_buf);
   printf ("   ci_running         = %u\n", (unsigned)p_cpm->ci_running);
   printf (
      "   ci_timer           = %u\n",
      (unsigned)pf_scheduler_get_value (&p_cpm->ci_timeout));
   printf ("   buffer_status      = %x\n", (unsigned)p_cpm->data_status);
   printf ("   buffer_length      = %u\n", (unsigned)p_cpm->buffer_length);
   printf ("   buffer_pos         = %u\n", (unsigned)p_cpm->buffer_pos);
}
