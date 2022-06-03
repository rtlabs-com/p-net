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
 * @brief Implements the Cyclic Provider Protocol Machine (PPM)
 *
 * This handles cyclic sending of data (and IOPS). Initializes transmit buffers.
 *
 * One instance of PPM exist per CR (together with a CPM).
 *
 * The states are W_START and RUN.
 *
 * There are functions used by the application (via pnet_api.c) to set and get
 * data, IOCS and IOPS.
 *
 * A global mutex is used instead of a per-instance mutex.
 * The locking time is very low so it should not be very congested.
 * The mutex is created on the first call to pf_ppm_create and deleted on
 * the last call to pf_ppm_close.
 * Keep track of how many instances exist and delete the mutex when the
 * number reaches 0 (zero).
 *
 */

#ifdef UNIT_TEST
#define os_get_current_time_us mock_os_get_current_time_us
#endif

#include "pf_includes.h"

#include <string.h>
#include <inttypes.h>

void pf_ppm_init (pnet_t * net)
{
   net->ppm_instance_cnt = ATOMIC_VAR_INIT (0);

   LOG_DEBUG (PF_PPM_LOG, "PPM(%d): Init driver\n", __LINE__);

#if PNET_OPTION_DRIVER_ENABLE
   if (net->fspm_cfg.driver_enable)
   {
      pf_driver_ppm_init (net);
   }
   else
   {
      pf_ppm_driver_sw_init (net);
   }
#else
   pf_ppm_driver_sw_init (net);
#endif
}

/**
 * Return a string representation of the PPM state.
 * @param state            In:   The PPM state.
 * @return  A string representing the PPM state.
 */
const char * pf_ppm_state_to_string (pf_ppm_state_values_t state)
{
   const char * s = "<unknown>";

   switch (state)
   {
   case PF_PPM_STATE_W_START:
      s = "PF_PPM_STATE_W_START";
      break;
   case PF_PPM_STATE_RUN:
      s = "PF_PPM_STATE_RUN";
      break;
   }

   return s;
}

int pf_ppm_state_ind (pnet_t * net, pf_ar_t * p_ar, pf_ppm_t * p_ppm, bool error)
{
   if (error == true)
   {
      p_ar->err_cls = PNET_ERROR_CODE_1_PPM;
      p_ar->err_code = PNET_ERROR_CODE_2_PPM_INVALID;
      pf_cmsu_ppm_error_ind (net, p_ar, p_ar->err_cls, p_ar->err_code);
   }

   return 0;
}

/**
 * Handle state changes in the PPM instance.
 * @param p_ppm            InOut: The PPM instance.
 * @param state            In:   The new PPM state.
 */
void pf_ppm_set_state (pf_ppm_t * p_ppm, pf_ppm_state_values_t state)
{
   if (state != p_ppm->state)
   {
      LOG_DEBUG (
         PF_PPM_LOG,
         "PPM(%d): New state %s (was %s)\n",
         __LINE__,
         pf_ppm_state_to_string (state),
         pf_ppm_state_to_string (p_ppm->state));
   }
   p_ppm->state = state;
}

/**
 * @internal
 * Initialize a transmit buffer of a PPM instance.
 *
 * Insert destination and source MAC addresses, VLAN tag, Ethertype and
 * Profinet frame ID.
 *
 * Initialize the rest of the buffer to zero.
 *
 * @param p_ppm            In:   The PPM instance.
 * @param p_buf            InOut:The buffer to be initialized.
 * @param frame_id         In:   The frame_id.
 * @param p_header         In:   The VLAN tag header IOCR parameter.
 */
static void pf_ppm_init_buf (
   pf_ppm_t * p_ppm,
   pnal_buf_t * p_buf,
   uint16_t frame_id,
   pf_iocr_tag_header_t * p_header)
{
   uint16_t pos;
   uint8_t * p_payload = (uint8_t *)p_buf->payload;
   uint16_t u16 = 0;

   p_buf->len = p_ppm->buffer_length;
   memset (p_payload, 0, p_buf->len);
   pos = 0;

   /* Insert destination MAC address */
   memcpy (&p_payload[pos], &p_ppm->da, sizeof (p_ppm->da));
   pos += sizeof (p_ppm->da);

   /* Insert source MAC address */
   memcpy (&p_payload[pos], &p_ppm->sa, sizeof (p_ppm->sa));
   pos += sizeof (p_ppm->sa);

   /* Insert VLAN Tag protocol identifier (TPID) */
   u16 = PNAL_ETHTYPE_VLAN;
   u16 = htons (u16);
   memcpy (&p_payload[pos], &u16, sizeof (u16));
   pos += sizeof (u16);

   /* Insert VLAN ID (VID) and priority (Priority Code Point = PCP) */
   u16 = p_header->vlan_id & 0x0FFF;
   u16 |= (p_header->iocr_user_priority & 0x0007) << 13; /* Three leftmost bits
                                                          */
   u16 = htons (u16);
   memcpy (&p_payload[pos], &u16, sizeof (u16));
   pos += sizeof (u16);

   /* Insert EtherType */
   u16 = PNAL_ETHTYPE_PROFINET;
   u16 = htons (u16);
   memcpy (&p_payload[pos], &u16, sizeof (u16));
   pos += sizeof (u16);

   /* Insert Profinet frame ID (first part of Ethernet frame payload) */
   u16 = htons (frame_id);
   memcpy (&p_payload[pos], &u16, sizeof (u16));
   /* No further pos advancement, to suppress clang warning */
}

void pf_ppm_finish_buffer (pnet_t * net, pf_ppm_t * p_ppm, uint16_t data_length)
{
   uint8_t * p_payload = ((pnal_buf_t *)p_ppm->p_send_buffer)->payload;
   uint16_t u16;

   p_ppm->cycle = pf_ppm_calculate_next_cyclecounter (
      p_ppm->cycle,
      p_ppm->send_clock_factor,
      p_ppm->reduction_ratio);

   /* Insert data */
   os_mutex_lock (net->ppm_buf_lock);
   memcpy (&p_payload[p_ppm->buffer_pos], p_ppm->buffer_data, data_length);
   os_mutex_unlock (net->ppm_buf_lock);

   /* Insert cycle counter */
   u16 = htons (p_ppm->cycle);
   memcpy (&p_payload[p_ppm->cycle_counter_offset], &u16, sizeof (u16));

   /* Insert data status */
   memcpy (
      &p_payload[p_ppm->data_status_offset],
      &p_ppm->data_status,
      sizeof (p_ppm->data_status));

   /* Insert transfer status */
   memcpy (
      &p_payload[p_ppm->transfer_status_offset],
      &p_ppm->transfer_status,
      sizeof (p_ppm->transfer_status));
}

/**
 * Initialize ppm configuration from AR data.
 *  - Compute and set frame offsets to
 *    cycle counter, status and more
 *  - sender address (sa)
 *  - destination address (da)
 *  - send interval config
 *  - status
 *
 * @param net              InOut: The NET instance.
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR instance.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
static int pf_ppm_init_cr_config (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   const uint16_t vlan_size = 4;
   pf_iocr_t * p_iocr = &p_ar->iocrs[crep];
   pf_ppm_t * p_ppm = &p_iocr->ppm;

   p_ppm->first_transmit = false;

   memcpy (
      &p_ppm->sa,
      &p_ar->ar_result.cm_responder_mac_add,
      sizeof (p_ppm->sa));

   memcpy (&p_ppm->da, &p_ar->ar_param.cm_initiator_mac_add, sizeof (p_ppm->da));

   p_ppm->buffer_pos = 2 * sizeof (pnet_ethaddr_t) + vlan_size +
                       sizeof (uint16_t) + sizeof (uint16_t);
   p_ppm->cycle = 0;
   p_ppm->transfer_status = 0;

   /* Pre-compute send buffer offsets*/
   p_ppm->cycle_counter_offset = p_ppm->buffer_pos + /* ETH frame header */
                                 p_iocr->param.c_sdu_length; /* Profinet data
                                                                length */

   p_ppm->data_status_offset = p_ppm->buffer_pos + /* ETH frame header */
                               p_iocr->param.c_sdu_length + /* Profinet data
                                                               length */
                               sizeof (uint16_t);           /* cycle counter */

   p_ppm->transfer_status_offset = p_ppm->buffer_pos + /* ETH frame header */
                                   p_iocr->param.c_sdu_length + /* Profinet
                                                                   data
                                                                   length */
                                   sizeof (uint16_t) + /* cycle counter */
                                   1;                  /* data status */

   p_ppm->buffer_length = p_ppm->buffer_pos +          /* ETH frame header */
                          p_iocr->param.c_sdu_length + /* Profinet data
                                                          length */
                          sizeof (uint16_t) +          /* cycle counter */
                          1 +                          /* data status */
                          1;                           /* transfer status */

   p_ppm->data_status =
      BIT (PNET_DATA_STATUS_BIT_STATE) | /* PRIMARY */
      BIT (PNET_DATA_STATUS_BIT_DATA_VALID) |
      BIT (PNET_DATA_STATUS_BIT_STATION_PROBLEM_INDICATOR); /* Normal */

   p_ppm->control_interval = ((uint32_t)p_iocr->param.send_clock_factor *
                              (uint32_t)p_iocr->param.reduction_ratio * 1000U) /
                             32U; /* us */

   p_ppm->send_clock_factor = p_iocr->param.send_clock_factor;
   p_ppm->reduction_ratio = p_iocr->param.reduction_ratio;

   return 0;
}

int pf_ppm_create (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   pf_iocr_t * p_iocr = &p_ar->iocrs[crep];
   pf_ppm_t * p_ppm;
   uint32_t cnt;

   CC_ASSERT (net && net->ppm_drv && net->ppm_drv->activate_req && p_ar);

   LOG_DEBUG (
      PF_PPM_LOG,
      "PPM(%d): Create PPM instance. AREP %u "
      "CREP %" PRIu32 "\n",
      __LINE__,
      p_ar->arep,
      crep);

   cnt = atomic_fetch_add (&net->ppm_instance_cnt, 1);
   if (cnt == 0)
   {
      net->ppm_buf_lock = os_mutex_create();
      CC_ASSERT (net->ppm_buf_lock != NULL);
   }

   p_ppm = &p_iocr->ppm;

   if (p_ppm->state != PF_PPM_STATE_W_START)
   {
      p_ar->err_cls = PNET_ERROR_CODE_1_PPM;
      p_ar->err_code = PNET_ERROR_CODE_2_PPM_INVALID_STATE;
      return -1;
   }

   if (pf_ppm_init_cr_config (net, p_ar, crep) != 0)
   {
      p_ar->err_cls = PNET_ERROR_CODE_1_PPM;
      p_ar->err_code = PNET_ERROR_CODE_2_PPM_INVALID_STATE;
      return -1;
   }

   /* Get the buffer to store the outgoing frame into. */
   p_ppm->p_send_buffer = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);

   if (p_ppm->p_send_buffer == NULL)
   {
      p_ar->err_cls = PNET_ERROR_CODE_1_PPM;
      p_ar->err_code = PNET_ERROR_CODE_2_PPM_INVALID_STATE;
      return -1;
   }

   /* Default_values: Set buffer to zero and IOxS to BAD (=0) */
   /* Default_status: Set cycle_counter to invalid, transfer_status = 0,
    * data_status = 0 */
   pf_ppm_init_buf (
      p_ppm,
      p_ppm->p_send_buffer,
      p_iocr->param.frame_id,
      &p_iocr->param.iocr_tag_header);

   return net->ppm_drv->create (net, p_ar, crep);
}

int pf_ppm_activate_req (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   int ret = -1;
   pf_iocr_t * p_iocr = &p_ar->iocrs[crep];
   pf_ppm_t * p_ppm;

   p_ppm = &p_iocr->ppm;

   if (p_ppm->state == PF_PPM_STATE_RUN)
   {
      p_ar->err_cls = PNET_ERROR_CODE_1_PPM;
      p_ar->err_code = PNET_ERROR_CODE_2_PPM_INVALID_STATE;
      return ret;
   }

   p_ppm->next_exec = os_get_current_time_us();
   p_ppm->next_exec += p_ppm->control_interval;

   LOG_DEBUG (
      PF_PPM_LOG,
      "PPM(%d): Activate PPM. AREP %u "
      "CREP %" PRIu32 ", period %" PRIu32 " microseconds. FrameID 0x%04x\n",
      __LINE__,
      p_ar->arep,
      crep,
      p_ppm->control_interval,
      p_iocr->param.frame_id);

   pf_ppm_set_state (p_ppm, PF_PPM_STATE_RUN);

   p_ppm->ci_running = true;

   /* Initialize input data and iops */
   pf_ppm_finish_buffer (net, p_ppm, p_iocr->in_length);

   ret = net->ppm_drv->activate_req (net, p_ar, crep);

   if (ret != 0)
   {
      pf_ppm_state_ind (net, p_ar, p_ppm, true); /* Error */
   }

   return ret;
}

int pf_ppm_close_req (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{

   pf_ppm_t * p_ppm = &p_ar->iocrs[crep].ppm;
   uint32_t cnt;

   LOG_DEBUG (
      PF_PPM_LOG,
      "PPM(%d): Closing PPM for AREP %u CREP %" PRIu32 "\n",
      __LINE__,
      p_ar->arep,
      crep);

   p_ppm->ci_running = false;

   /* Stop driver handling cyclic transmits */
   net->ppm_drv->close_req (net, p_ar, crep);

   pnal_buf_free (p_ppm->p_send_buffer);
   // TODO - set p_send_buffer to NULL ??

   pf_ppm_set_state (p_ppm, PF_PPM_STATE_W_START);
   p_ppm->data_status = 0;
   cnt = atomic_fetch_sub (&net->ppm_instance_cnt, 1);
   if (cnt == 1)
   {
      os_mutex_destroy (net->ppm_buf_lock);
      net->ppm_buf_lock = NULL;
   }

   return 0;
}

int pf_ppm_get_ar_iocr_desc (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   pf_ar_t ** pp_ar,
   pf_iocr_t ** pp_iocr,
   pf_iodata_object_t ** pp_iodata,
   uint32_t * p_crep)
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
      LOG_DEBUG (PF_PPM_LOG, "PPM(%d): No AR set in sub-slot\n", __LINE__);
   }
   else
   {
      /*
       * Search the AR for an INPUT CR or an MC provider CR containing the
       * sub-slot.
       */
      for (crep = 0; ((found == false) && (crep < p_ar->nbr_iocrs)); crep++)
      {
         if (
            (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_INPUT) ||
            (p_ar->iocrs[crep].param.iocr_type == PF_IOCR_TYPE_MC_PROVIDER))
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
                  *p_crep = crep;
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

int pf_ppm_set_data_and_iops (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   const uint8_t * p_data,
   uint16_t data_len,
   const uint8_t * p_iops,
   uint8_t iops_len)
{
   int ret = -1;
   pf_iocr_t * p_iocr = NULL;
   pf_iodata_object_t * p_iodata = NULL;
   pf_ar_t * p_ar = NULL;
   uint32_t crep;

   if (
      pf_ppm_get_ar_iocr_desc (
         net,
         api_id,
         slot_nbr,
         subslot_nbr,
         &p_ar,
         &p_iocr,
         &p_iodata,
         &crep) == 0)
   {
      switch (p_iocr->ppm.state)
      {
      case PF_PPM_STATE_W_START:
      case PF_PPM_STATE_RUN:
         if (
            (data_len == p_iodata->data_length) &&
            (iops_len == p_iodata->iops_length))
         {
            ret = net->ppm_drv->write_data_and_iops (
               net,
               p_iocr,
               p_iodata,
               p_data,
               data_len,
               p_iops,
               iops_len);

            p_iodata->data_avail = true;
         }
         else
         {
            LOG_ERROR (
               PF_PPM_LOG,
               "PPM(%d): Given data size %u and IOPS size %u, "
               "but PLC expects sizes %u and %u for slot %u subslot 0x%04x\n",
               __LINE__,
               data_len,
               iops_len,
               p_iodata->data_length,
               p_iodata->iops_length,
               slot_nbr,
               subslot_nbr);
         }
         break;
      default:
         LOG_ERROR (
            PF_PPM_LOG,
            "PPM(%d): Set data in wrong state: %u for AREP %u\n",
            __LINE__,
            p_iocr->ppm.state,
            p_ar->arep);
         break;
      }
   }
   else
   {
      /* May happen after an ABORT */
      LOG_DEBUG (
         PF_PPM_LOG,
         "PPM(%d): No data descriptor found for set data\n",
         __LINE__);
   }

   return ret;
}

int pf_ppm_set_iocs (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   const uint8_t * p_iocs,
   uint8_t iocs_len)
{
   int ret = -1;
   pf_iocr_t * p_iocr = NULL;
   pf_iodata_object_t * p_iodata = NULL;
   pf_ar_t * p_ar = NULL;
   uint32_t crep;

   if (
      pf_ppm_get_ar_iocr_desc (
         net,
         api_id,
         slot_nbr,
         subslot_nbr,
         &p_ar,
         &p_iocr,
         &p_iodata,
         &crep) == 0)
   {
      switch (p_iocr->ppm.state)
      {
      case PF_PPM_STATE_W_START:
      case PF_PPM_STATE_RUN:
         if (iocs_len == p_iodata->iocs_length)
         {
            ret = net->ppm_drv
                     ->write_iocs (net, p_iocr, p_iodata, p_iocs, iocs_len);
         }
         else if (p_iodata->iocs_length == 0)
         {
            /* ToDo: What does the spec say about this case? */
            LOG_DEBUG (PF_PPM_LOG, "PPM(%d): iocs_len is zero\n", __LINE__);
            ret = 0;
         }
         else
         {
            LOG_ERROR (
               PF_PPM_LOG,
               "PPM(%d): Given IOCS size %u, but PLC expects size %u "
               "for slot %u subslot 0x%04x\n",
               __LINE__,
               iocs_len,
               (unsigned)p_iodata->iocs_length,
               slot_nbr,
               subslot_nbr);
         }
         break;
      default:
         LOG_ERROR (
            PF_PPM_LOG,
            "PPM(%d): Set data in wrong state: %u for AREP %u\n",
            __LINE__,
            (unsigned)p_iocr->ppm.state,
            p_ar->arep);
         break;
      }
   }
   else
   {
      /* May happen after an ABORT */
      LOG_ERROR (
         PF_PPM_LOG,
         "PPM(%d): No data descriptor found for set iocs\n",
         __LINE__);
   }

   return ret;
}

int pf_ppm_get_data_and_iops (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint8_t * p_data,
   uint16_t * p_data_len,
   uint8_t * p_iops,
   uint8_t * p_iops_len)
{
   int ret = -1;
   pf_iocr_t * p_iocr = NULL;
   pf_iodata_object_t * p_iodata = NULL;
   pf_ar_t * p_ar = NULL;
   uint32_t crep;

   if (
      pf_ppm_get_ar_iocr_desc (
         net,
         api_id,
         slot_nbr,
         subslot_nbr,
         &p_ar,
         &p_iocr,
         &p_iodata,
         &crep) == 0)
   {
      switch (p_iocr->ppm.state)
      {
      case PF_PPM_STATE_W_START:
         p_ar->err_cls = PNET_ERROR_CODE_1_PPM;
         p_ar->err_code = PNET_ERROR_CODE_2_PPM_INVALID_STATE;
         LOG_DEBUG (
            PF_PPM_LOG,
            "PPM(%d): Get data in wrong state: %u for AREP %u\n",
            __LINE__,
            p_iocr->ppm.state,
            p_ar->arep);
         break;
      case PF_PPM_STATE_RUN:
         if (
            (*p_data_len >= p_iodata->data_length) &&
            (*p_iops_len >= p_iodata->iops_length))
         {
            *p_data_len = p_iodata->data_length;
            *p_iops_len = p_iodata->iops_length;
            ret = net->ppm_drv->read_data_and_iops (
               net,
               p_iocr,
               p_iodata,
               p_data,
               *p_data_len,
               p_iops,
               *p_iops_len);
         }
         else
         {
            LOG_ERROR (
               PF_PPM_LOG,
               "PPM(%d): Given data buffer size %u and IOPS buffer size "
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
         break;
      default:
         LOG_ERROR (
            PF_PPM_LOG,
            "PPM(%d): Get data in wrong state: %u for AREP %u\n",
            __LINE__,
            (unsigned)p_iocr->ppm.state,
            p_ar->arep);
         break;
      }
   }
   else
   {
      /* May happen after an ABORT */
      LOG_ERROR (
         PF_PPM_LOG,
         "PPM(%d): No data descriptor found for get data\n",
         __LINE__);
   }

   return ret;
}

int pf_ppm_get_iocs (
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
   uint32_t crep;

   if (
      pf_ppm_get_ar_iocr_desc (
         net,
         api_id,
         slot_nbr,
         subslot_nbr,
         &p_ar,
         &p_iocr,
         &p_iodata,
         &crep) == 0)
   {
      switch (p_iocr->ppm.state)
      {
      case PF_PPM_STATE_W_START:
         p_ar->err_cls = PNET_ERROR_CODE_1_PPM;
         p_ar->err_code = PNET_ERROR_CODE_2_PPM_INVALID_STATE;
         LOG_DEBUG (
            PF_PPM_LOG,
            "PPM(%d): Get iocs in wrong state: %u for AREP %u\n",
            __LINE__,
            p_iocr->ppm.state,
            p_ar->arep);
         break;
      case PF_PPM_STATE_RUN:
         if (*p_iocs_len >= p_iodata->iocs_length)
         {
            *p_iocs_len = p_iodata->iocs_length;
            ret = net->ppm_drv
                     ->read_iocs (net, p_iocr, p_iodata, p_iocs, *p_iocs_len);
         }
         else
         {
            LOG_ERROR (
               PF_PPM_LOG,
               "PPM(%d): Given IOCS buffer size %u, but minimum size is %u "
               "for slot %u subslot 0x%04x\n",
               __LINE__,
               (unsigned)*p_iocs_len,
               (unsigned)p_iodata->iocs_length,
               slot_nbr,
               subslot_nbr);
         }
         break;
      default:
         LOG_ERROR (
            PF_PPM_LOG,
            "PPM(%d): Get iocs in wrong state: %u for AREP %u\n",
            __LINE__,
            (unsigned)p_iocr->ppm.state,
            p_ar->arep);
         break;
      }
   }
   else
   {
      /* May happen after an ABORT */
      LOG_ERROR (
         PF_PPM_LOG,
         "PPM(%d): No data descriptor found for get iocs\n",
         __LINE__);
   }

   return ret;
}

int pf_ppm_set_data_status_state (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t crep,
   bool primary)
{
   pf_ppm_t * p_ppm;

   p_ppm = &p_ar->iocrs[crep].ppm;

   if (primary == true)
   {
      p_ppm->data_status |= BIT (PNET_DATA_STATUS_BIT_STATE);
   }
   else
   {
      p_ppm->data_status &= ~BIT (PNET_DATA_STATUS_BIT_STATE);
   }

   return net->ppm_drv->write_data_status (
      net,
      &p_ar->iocrs[crep],
      p_ppm->data_status);
}

int pf_ppm_set_data_status_redundancy (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t crep,
   bool redundant)
{
   pf_ppm_t * p_ppm;

   p_ppm = &p_ar->iocrs[crep].ppm;

   if (redundant == true)
   {
      p_ppm->data_status |= BIT (PNET_DATA_STATUS_BIT_REDUNDANCY);
   }
   else
   {
      p_ppm->data_status &= ~BIT (PNET_DATA_STATUS_BIT_REDUNDANCY);
   }

   return net->ppm_drv->write_data_status (
      net,
      &p_ar->iocrs[crep],
      p_ppm->data_status);
}

int pf_ppm_set_data_status_provider (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t crep,
   bool run)
{
   pf_ppm_t * p_ppm;

   p_ppm = &p_ar->iocrs[crep].ppm;

   if (run == true)
   {
      p_ppm->data_status |= BIT (PNET_DATA_STATUS_BIT_PROVIDER_STATE);
   }
   else
   {
      p_ppm->data_status &= ~BIT (PNET_DATA_STATUS_BIT_PROVIDER_STATE);
   }

   return net->ppm_drv->write_data_status (
      net,
      &p_ar->iocrs[crep],
      p_ppm->data_status);
}

int pf_ppm_get_data_status (const pf_ppm_t * p_ppm, uint8_t * p_data_status)
{
   *p_data_status = p_ppm->data_status;
   return 0;
}

void pf_ppm_set_problem_indicator (
   pnet_t * net,
   pf_ar_t * p_ar,
   bool problem_indicator)
{
   uint16_t ix;

   for (ix = 0; ix < NELEMENTS (p_ar->iocrs); ix++)
   {
      if (
         (p_ar->iocrs[ix].param.iocr_type == PF_IOCR_TYPE_INPUT) ||
         (p_ar->iocrs[ix].param.iocr_type == PF_IOCR_TYPE_MC_PROVIDER))
      {
         if (problem_indicator == true)
         {
            p_ar->iocrs[ix].ppm.data_status &=
               ~BIT (PNET_DATA_STATUS_BIT_STATION_PROBLEM_INDICATOR);
         }
         else
         {
            p_ar->iocrs[ix].ppm.data_status |=
               BIT (PNET_DATA_STATUS_BIT_STATION_PROBLEM_INDICATOR);
         }

         net->ppm_drv->write_data_status (
            net,
            &p_ar->iocrs[ix],
            p_ar->iocrs[ix].ppm.data_status);
      }
   }
}

/************************** Utilities ****************************************/

/**
 * Calculate the current cyclecounter from the current time.
 *
 * The timebase is 1/32 of a millisecond (31.25 us).
 * Note that 31.25 us is 1000/32 us.
 * This is the cyclecounter value, which is the background counter used for
 * cycles.
 *
 * One cycle length is send_clock_factor x 31.25 us.
 * The cyclenumber value is keeping track of which cycle we are in now.
 *
 * The reduction_ratio describes if transmit and receive should
 * be done every cycle. A value of 1 corresponds to every cycle,
 * while a value of 4 corresponds to transmit and receive every fourth cycle.
 *
 * For example, send_clock_factor=2 and reduction_ratio=4 gives a
 * cycle time of 62.5 us, and a frame is sent every 250 us.
 * In the first frame the cyclecounter value should be 0, and in the next should
 * the value be 8.
 *
 * We need to normalize the cyclecounter to multiples of
 * send_clock_factor x reduction_ratio.
 *
 * Described in Profinet protocol 2.4 sections
 *  - 4.5.3
 *  - 4.7.2.1.2 Coding of the field CycleCounter
 *  - 4.12.4.5
 *  - 5.2.5.59 and .62 and .63
 *
 * @param timestamp         In:   Current time in microseconds.
 * @param send_clock_factor In:   Defines the length of a cycle, in units
 *                                of 31.25 us. Allowed 1 .. 128.
 *                                Typically powers of two.
 * @param reduction_ratio   In:   Reduction ratio. If transmission should be
 *                                done every cycle. Allowed 1 .. 512.
 *                                Typically powers of two.
 * @return                  The current cyclecounter
 */
uint16_t pf_ppm_calculate_cyclecounter (
   uint32_t timestamp,
   uint16_t send_clock_factor,
   uint16_t reduction_ratio)
{
   uint64_t raw_cyclecounter;
   uint32_t number_of_transmissions;
   const uint32_t transmission_interval_in_timebases =
      send_clock_factor * reduction_ratio;

   raw_cyclecounter = (32UL * timestamp) / 1000UL;

   /* Integer division for truncation */
   number_of_transmissions =
      raw_cyclecounter / transmission_interval_in_timebases;

   /* Now a multiple of transmission intervals */
   return number_of_transmissions * transmission_interval_in_timebases;
}

/**
 * Calculate the next cyclecounter value from the previous one.
 *
 * The timebase is 1/32 of a millisecond (31.25 us).
 *
 * One cycle length is send_clock_factor x 31.25 us.
 *
 * The reduction_ratio describes if transmit and receive should
 * be done every cycle. A value of 1 corresponds to every cycle,
 * while a value of 4 corresponds to transmit and receive every fourth cycle.
 *
 * For example, send_clock_factor=2 and reduction_ratio=4 gives a
 * cycle time of 62.5 us, and a frame is sent every 250 us.
 * In the first frame the cyclecounter value should be 0, and in the next should
 * the value be 8.
 *
 * @param previous_cyclecounter In:   Previous cyclecounter value
 * @param send_clock_factor     In:   Defines the length of a cycle, in units
 *                                    of 31.25 us. Allowed 1 .. 128.
 *                                    Typically powers of two.
 * @param reduction_ratio       In:   Reduction ratio. If transmission should be
 *                                    done every cycle. Allowed 1 .. 512.
 *                                    Typically powers of two.
 * @return Next cyclecounter value
 */
uint16_t pf_ppm_calculate_next_cyclecounter (
   uint16_t previous_cyclecounter,
   uint16_t send_clock_factor,
   uint16_t reduction_ratio)
{
   uint16_t number_of_previous_transmissions;
   const uint32_t transmission_interval_in_timebases =
      send_clock_factor * reduction_ratio;

   /* Integer division for truncation */
   number_of_previous_transmissions =
      previous_cyclecounter / transmission_interval_in_timebases;

   /* Make sure we return a multiple of transmission intervals, regardless
      of the previous_cyclecounter value */
   return (number_of_previous_transmissions + 1) *
          transmission_interval_in_timebases;
}

/**************** Diagnostic strings *****************************************/

void pf_ppm_show (const pf_ppm_t * p_ppm)
{
   printf ("ppm:\n");
   printf (
      "   state                        = %s\n",
      pf_ppm_state_to_string (p_ppm->state));
   printf ("   errline                      = %u\n", (unsigned)p_ppm->errline);
   printf ("   errcnt                       = %u\n", (unsigned)p_ppm->errcnt);
   printf (
      "   first_transmit               = %u\n",
      (unsigned)p_ppm->first_transmit);
   printf ("   trx_cnt                      = %u\n", (unsigned)p_ppm->trx_cnt);
   printf ("   p_send_buffer                = %p\n", p_ppm->p_send_buffer);
   printf (
      "   p_send_buffer->len           = %u\n",
      p_ppm->p_send_buffer ? ((pnal_buf_t *)(p_ppm->p_send_buffer))->len : 0);
   printf ("   new_buf                      = %u\n", (unsigned)p_ppm->new_buf);
   printf (
      "   control_interval             = %u\n",
      (unsigned)p_ppm->control_interval);
   printf ("   next_exec                    = %" PRIu32 "\n", p_ppm->next_exec);
   printf ("   cycle                        = %u\n", (unsigned)p_ppm->cycle);
   printf (
      "   cycle_counter_off            = %u\n",
      (unsigned)p_ppm->cycle_counter_offset);
   printf (
      "   data_status_offset           = %u\n",
      (unsigned)p_ppm->data_status_offset);
   printf (
      "   transfer_status_of           = %u\n",
      (unsigned)p_ppm->transfer_status_offset);
   printf (
      "   ci_running                   = %u\n",
      (unsigned)p_ppm->ci_running);
   printf (
      "   ci_timer                     = %u\n",
      (unsigned)pf_scheduler_get_value (&p_ppm->ci_timeout));
   printf (
      "   transfer_status              = %u\n",
      (unsigned)p_ppm->transfer_status);
   printf (
      "   data_status                  = %x\n",
      (unsigned)p_ppm->data_status);
   printf (
      "   buffer_length                = %u\n",
      (unsigned)p_ppm->buffer_length);
   printf (
      "   buffer_pos                   = %u\n",
      (unsigned)p_ppm->buffer_pos);
}
