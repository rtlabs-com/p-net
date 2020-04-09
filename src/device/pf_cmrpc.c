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
 * @brief Implements the Context Management RPC Device Protocol Machine (CMRPC)
 *
 * This handles RPC communication; for example connect, release, dcontrol,
 * ccontrol and parameter read and write.
 *
 * It opens a UDP socket and listens for connections.
 *
 * CMRPC has only one state - IDLE. I.e., it is state-less.
 *
 * An AR is created on each connect.
 * The ar:s are allocated from the array net->cmrpc_ar
 * ToDo: Move to CMDEV device structure.
 *
 * Sessions are used for sending notifications to the controller, when a
 * response is expected.
 * Each session uses one entry in net->cmrpc_session_info
 *
 * The socket net->cmrpc_rpcreq_socket is used for RPC requests (connects etc)
 *
 */

#ifdef UNIT_TEST
#define os_udp_socket mock_os_udp_socket
#define os_udp_sendto mock_os_udp_sendto
#define os_udp_open mock_os_udp_open
#define os_udp_close mock_os_udp_close
#define os_udp_recvfrom mock_os_udp_recvfrom
#endif

#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <inttypes.h>

#include "osal.h"

#include "pf_includes.h"
#include "pf_block_reader.h"
#include "pf_block_writer.h"

static const pf_uuid_t     implicit_ar = {0,0,0,{0,0,0,0,0,0,0,0}};

/**************** Diagnostic strings *****************************************/

/**
 * @internal
 * Return a string representation of the AR state.
 * @param state            In:   The AR state.
 * @return  A string representing the AR state.
 */
static const char *pf_ar_state_to_string(
   pf_ar_state_values_t    state)
{
   const char *s = "<unknown>";

   switch (state)
   {
   case PF_AR_STATE_PRIMARY:  s = "PF_AR_STATE_PRIMARY"; break;
   case PF_AR_STATE_FIRST:    s = "PF_AR_STATE_FIRST"; break;
   case PF_AR_STATE_BACKUP:   s = "PF_AR_STATE_BACKUP"; break;
   }

   return s;
}

/**
 * @internal
 * Return a string representation of the AR sync state.
 * @param state            In:   The AR sync state.
 * @return  A string representing the AR sync state.
 */
static const char *pf_ar_sync_state_to_string(
   pf_sync_state_values_t  state)
{
   const char *s = "<unknown>";

   switch (state)
   {
   case PF_SYNC_STATE_SYNCHRONIZED:  s = "PF_SYNC_STATE_SYNCHRONIZED"; break;
   case PF_SYNC_STATE_NOT_AVAILABLE: s = "PF_SYNC_STATE_NOT_AVAILABLE"; break;
   }

   return s;
}

/**
 * @internal
 * Return a string representation of an IOCR type.
 * @param type             In:   The IOCR type.
 * @return  A string representing the IOCR type.
 */
static const char *pf_iocr_type_to_string(
   pf_iocr_type_values_t   type)
{
   const char *s = "<unknown>";

   switch (type)
   {
   case PF_IOCR_TYPE_INPUT:         s = "INPUT"; break;
   case PF_IOCR_TYPE_OUTPUT:        s = "OUTPUT"; break;
   case PF_IOCR_TYPE_MC_CONSUMER:   s = "MC CONSUMER"; break;
   case PF_IOCR_TYPE_MC_PROVIDER:   s = "MC PROVIDER"; break;
   default:
      break;
   }

   return s;
}

/**
 * @internal
 * Return a string representation of the AR type.
 * @param type             In:   The AR type.
 * @return  A string representing the AR type.
 */
static const char *pf_ar_type_to_string(
   pf_ar_type_values_t     type)
{
   const char *s = "<unknown>";

   switch (type)
   {
   case PF_ART_IOCAR_SINGLE:        s = "PF_ART_IOCAR_SINGLE"; break;
   case PF_ART_IOSAR:               s = "PF_ART_IOSAR"; break;
   case PF_ART_IOCAR_SINGLE_RTC_3:  s = "PF_ART_IOCAR_SINGLE_RTC_3"; break;
   case PF_ART_IOCAR_SR:            s = "PF_ART_IOCAR_SR"; break;
   }

   return s;
}

void pf_cmrpc_show(
   pnet_t                  *net,
   unsigned                level)
{
   uint16_t                ix;
   uint16_t                iy;
   uint16_t                ar_ix;
   pf_ar_t                 *p_ar = NULL;
   pf_iocr_t               *p_iocr = NULL;
   pf_session_info_t       *p_sess = NULL;
   pf_iodata_object_t      *p_desc;



   if (level & 0x0800)
   {
      for (ix = 0; ix < PF_MAX_SESSION; ix++)
      {
         p_sess = &net->cmrpc_session_info[ix];
         printf("Session ID            = %u\n", (unsigned)p_sess->ix);
         printf("   in use             = %s\n", p_sess->in_use ? "YES" : "NO");
         printf("   release in progress= %s\n", p_sess->release_in_progress ? "YES" : "NO");

         printf("   socket             = %u\n", (unsigned)p_sess->socket);
         printf("   @AR                = %p\n", p_sess->p_ar);
         printf("   from me            = %s\n", p_sess->from_me ? "YES" : "NO");
         printf("   ip_addr            = %u.%u.%u.%u\n",
            ((unsigned)p_sess->ip_addr>>24) & 0xff,
            ((unsigned)p_sess->ip_addr>>16) & 0xff,
            ((unsigned)p_sess->ip_addr>>8) & 0xff,
            (unsigned)p_sess->ip_addr & 0xff);
         printf("   port               = %u\n", (unsigned)p_sess->port);
         printf("   sequence_nmb_send  = %u\n", (unsigned)p_sess->sequence_nmb_send);
         printf("   fragment_nbr       = %u\n", (unsigned)p_sess->fragment_nbr);
         printf("   dcontrol_sequence_nmb = %u\n", (unsigned)p_sess->dcontrol_sequence_nmb);
         printf("   dcontrol result    = %02x %02x %02x %02x\n",
            (unsigned)p_sess->dcontrol_result.pnio_status.error_code,
            (unsigned)p_sess->dcontrol_result.pnio_status.error_decode,
            (unsigned)p_sess->dcontrol_result.pnio_status.error_code_1,
            (unsigned)p_sess->dcontrol_result.pnio_status.error_code_2);
      }
   }

   if (level & 0x1000)
   {
      for (ar_ix = 0; ar_ix < PNET_MAX_AR; ar_ix++)
      {
         p_ar = pf_ar_find_by_index(net, ar_ix);
         printf("AR in use             = %s\n", p_ar->in_use ? "YES" : "NO");
         printf("AR arep               = %u\n", (unsigned)p_ar->arep);
         printf("AR state              = %s\n", pf_ar_state_to_string(p_ar->ar_state));
         printf("sync state            = %s\n", pf_ar_sync_state_to_string(p_ar->sync_state));
         printf("ready 4 data          = %s\n", p_ar->ready_4_data ? "TRUE" : "FALSE");
         pf_cmdev_show(p_ar);
         pf_cmpbe_show(p_ar);
         pf_cmio_show(p_ar);
         pf_cmsm_show(p_ar);
         pf_cmwrr_show(net, p_ar);
         printf("AR param valid        = %u\n", (unsigned)p_ar->ar_param.valid);
         printf("         type         = %s\n", pf_ar_type_to_string(p_ar->ar_param.ar_type));
         printf("         session key  = %u\n", (unsigned)p_ar->ar_param.session_key);
         printf("   prop  state        = %u\n", (unsigned)p_ar->ar_param.ar_properties.state);
         printf("         par server   = %s\n", p_ar->ar_param.ar_properties.parameterization_server == PF_PS_EXTERNAL_PARAMETER_SERVER? "Ext" : "CM initiator");
         printf("         dev access   = %u\n", (unsigned)p_ar->ar_param.ar_properties.device_access);
         printf("         sup allowed  = %u\n", (unsigned)p_ar->ar_param.ar_properties.supervisor_takeover_allowed);
         printf("         pull alarm OK= %u\n", (unsigned)p_ar->ar_param.ar_properties.pull_module_alarm_allowed);
         printf("         startup mode = %s\n", p_ar->ar_param.ar_properties.startup_mode ? "Advanced" : "Legacy");
         printf("   err_cls            = %x\n", (unsigned)p_ar->err_cls);
         printf("   err_code           = %x\n", (unsigned)p_ar->err_code);
         printf("   CM Initiator Activity Timeout Factor = %u\n", p_ar->ar_param.cm_initiator_activity_timeout_factor);

         pf_alarm_show(p_ar);

         printf("nbr_iocr              = %u\n", (unsigned)p_ar->nbr_iocrs);
         if (level & 0x03)
         {
            for (ix = 0; ix < p_ar->nbr_iocrs; ix++)
            {
               p_iocr = &p_ar->iocrs[ix];
               printf("\n-------------- IOCR %u ---------------\n", ix);
               printf("%3u    : IOCR type    = %s\n", (unsigned)ix, pf_iocr_type_to_string(p_iocr->param.iocr_type));
               printf("%3u    : in_length    = %u\n", (unsigned)ix, (unsigned)p_iocr->in_length);
               printf("%3u    : out_length   = %u\n", (unsigned)ix, (unsigned)p_iocr->out_length);
               printf("%3u    : nbr_data_desc= %u\n", (unsigned)ix, (unsigned)p_iocr->nbr_data_desc);
               if (level & 0x02)    /* More details */
               {
                  for (iy = 0; iy < p_iocr->nbr_data_desc; iy++)
                  {
                     p_desc = &p_iocr->data_desc[iy];
                     printf("%3u,%3u: in_use       = %u\n", (unsigned)ix, (unsigned)iy, (unsigned)p_desc->in_use);
                     printf("%3u,%3u: data_avail   = %u\n", (unsigned)ix, (unsigned)iy, (unsigned)p_desc->data_avail);
                     printf("%3u,%3u: api          = %u\n", (unsigned)ix, (unsigned)iy, (unsigned)p_desc->api_id);
                     printf("%3u,%3u: slot         = %u\n", (unsigned)ix, (unsigned)iy, (unsigned)p_desc->slot_nbr);
                     printf("%3u,%3u: subslot      = %u\n", (unsigned)ix, (unsigned)iy, (unsigned)p_desc->subslot_nbr);
                     printf("%3u,%3u: offset       = %u\n", (unsigned)ix, (unsigned)iy, (unsigned)p_desc->data_offset);
                     printf("%3u,%3u: len          = %u\n", (unsigned)ix, (unsigned)iy, (unsigned)p_desc->data_length);
                     printf("%3u,%3u: IOPS off     = %u\n", (unsigned)ix, (unsigned)iy, (unsigned)p_desc->iops_offset);
                     printf("%3u,%3u: IOPS len     = %u\n", (unsigned)ix, (unsigned)iy, (unsigned)p_desc->iops_length);
                     printf("%3u,%3u: IOCS off     = %u\n", (unsigned)ix, (unsigned)iy, (unsigned)p_desc->iocs_offset);
                     printf("%3u,%3u: IOCS len     = %u\n", (unsigned)ix, (unsigned)iy, (unsigned)p_desc->iocs_length);
                  }
                  if ((p_iocr->param.iocr_type == PF_IOCR_TYPE_INPUT) ||
                      (p_iocr->param.iocr_type == PF_IOCR_TYPE_MC_PROVIDER))
                  {
                     pf_ppm_show(&p_iocr->ppm);
                  }
                  else
                  {
                     pf_cpm_show(net, &p_iocr->cpm);
                  }
               }
            }
         }
      }
   }
}

/*********************** Sessions and ARs ************************************/

/**
 * @internal
 * Allocate a new session instance.
 * @param net              InOut: The p-net stack instance
 * @param pp_sess          Out:  A pointer to the new session instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_session_allocate(
   pnet_t                  *net,
   pf_session_info_t       **pp_sess)
{
   int                     ret = -1;
   uint16_t                ix = 0;
   pf_session_info_t       *p_sess = NULL;

   os_mutex_lock(net->p_cmrpc_rpc_mutex);
   while ((ix < NELEMENTS(net->cmrpc_session_info)) &&
         (net->cmrpc_session_info[ix].in_use == true))
   {
      ix++;
   }

   if (ix < NELEMENTS(net->cmrpc_session_info))
   {
      p_sess = &net->cmrpc_session_info[ix];
      memset(p_sess, 0, sizeof(*p_sess));
      p_sess->in_use = true;
      p_sess->eth_handle = net->eth_handle;
      p_sess->p_ar = NULL;
      p_sess->sequence_nmb_send = 0;
      p_sess->dcontrol_sequence_nmb = UINT32_MAX;

      p_sess->from_me = true;
      p_sess->activity_uuid.data1 = net->cmrpc_session_number++;
      p_sess->activity_uuid.data2 = 0x1234;
      p_sess->activity_uuid.data3 = 0x5678;
      p_sess->activity_uuid.data4[0] = 0x01;
      p_sess->activity_uuid.data4[1] = 0x02;
      p_sess->activity_uuid.data4[2] = 0x03;
      p_sess->activity_uuid.data4[3] = 0x04;
      p_sess->activity_uuid.data4[4] = 0x05;
      p_sess->activity_uuid.data4[5] = 0x06;
      p_sess->activity_uuid.data4[6] = 0x07;
      p_sess->activity_uuid.data4[7] = 0x08;

      *pp_sess = p_sess;

      p_sess->ix = ix;
      LOG_INFO(PF_RPC_LOG, "RPC(%d): Allocated session %u\n", __LINE__, (unsigned)p_sess->ix);

      ret = 0;
   }
   os_mutex_unlock(net->p_cmrpc_rpc_mutex);

   return ret;
}

/**
 * @internal
 * Free the session_info.
 * @param p_sess           In:   The session instance.
 */
static void pf_session_release(
   pf_session_info_t       *p_sess)
{
   if (p_sess != NULL)
   {
      if (p_sess->in_use == true)
      {
         LOG_INFO(PF_RPC_LOG, "RPC(%d): Released session ix %u\n", __LINE__, (unsigned)p_sess->ix);
         memset(p_sess, 0, sizeof(*p_sess));
         p_sess->in_use = false;
      }
      else
      {
         LOG_ERROR(PF_RPC_LOG, "RPC(%d): Session not in use\n", __LINE__);
      }
   }
   else
   {
      LOG_ERROR(PF_RPC_LOG, "RPC(%d): Session is NULL\n", __LINE__);
   }
}

/**
 * @internal
 * Find a session by its activity UUID.
 * @param net              InOut: The p-net stack instance
 * @param p_uuid           In:   The UUID to look for.
 * @param pp_sess          Out:  The session instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_session_locate_by_uuid(
   pnet_t                  *net,
   pf_uuid_t               *p_uuid,
   pf_session_info_t       **pp_sess)
{
   int      ret = -1;
   uint16_t ix;

   ix = 0;
   while ((ix < NELEMENTS(net->cmrpc_session_info)) &&
          ((net->cmrpc_session_info[ix].in_use == false) ||
           (memcmp(p_uuid, &net->cmrpc_session_info[ix].activity_uuid, sizeof(*p_uuid)) != 0)))
   {
      ix++;
   }
   if (ix < NELEMENTS(net->cmrpc_session_info))
   {
      *pp_sess = &net->cmrpc_session_info[ix];
      ret = 0;
   }

   return ret;
}

/**
 * @internal
 * Find a session by its AR.
 * Only find sessions that originate from the device.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param pp_sess          Out:  The session instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_session_locate_by_ar(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_session_info_t       **pp_sess)
{
   int      ret = -1;
   uint16_t ix;

   ix = 0;
   while ((ix < NELEMENTS(net->cmrpc_session_info)) &&
          ((net->cmrpc_session_info[ix].in_use == false) ||
           (net->cmrpc_session_info[ix].from_me == false) ||
           (net->cmrpc_session_info[ix].p_ar != p_ar)))
   {
      ix++;
   }
   if (ix < NELEMENTS(net->cmrpc_session_info))
   {
      *pp_sess = &net->cmrpc_session_info[ix];
      ret = 0;
   }

   return ret;
}

/**
 * @internal
 * Allocate and clear a new AR.
 * @param net              InOut: The p-net stack instance
 * @param pp_ar            Out:  The new AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_ar_allocate(
   pnet_t                  *net,
   pf_ar_t                 **pp_ar)
{
   int                     ret = -1;
   uint16_t                ix = 0;
   os_mutex_lock(net->p_cmrpc_rpc_mutex);
   while ((ix < NELEMENTS(net->cmrpc_ar)) &&
         (net->cmrpc_ar[ix].in_use == true))
   {
      ix++;
   }

   if (ix < NELEMENTS(net->cmrpc_ar))
   {
      memset(&net->cmrpc_ar[ix], 0, sizeof(net->cmrpc_ar[ix]));
      net->cmrpc_ar[ix].in_use = true;

      *pp_ar = &net->cmrpc_ar[ix];

      ret = 0;
   }
   os_mutex_unlock(net->p_cmrpc_rpc_mutex);

   if (ix < PNET_MAX_AR)
   {
      net->cmrpc_ar[ix].arep = ix + 1;      /* Avoid AREP == 0 */
      LOG_INFO(PF_RPC_LOG, "RPC(%d): Allocate AR %u\n", __LINE__, ix);
   }

   return ret;
}

/**
 * @internal
 * Free the AR.
 * @param p_ar             In:   The AR instance.
 */
static void pf_ar_release(
   pf_ar_t                 *p_ar)
{
   if (p_ar != NULL)
   {
      if (p_ar->in_use == true)
      {
         LOG_INFO(PF_RPC_LOG, "RPC(%d): Free AR %u\n", __LINE__, p_ar->arep - 1);
         memset(p_ar, 0, sizeof(*p_ar));
      }
      else
      {
         LOG_ERROR(PNET_LOG, "RPC(%d): AR already released\n", __LINE__);
      }
   }
   else
   {
      LOG_ERROR(PNET_LOG, "RPC(%d): AR is NULL\n", __LINE__);
   }
}

/**
 * @internal
 * Find an AR by its UUID. The AR must be in a "used" state (not POWER_ON).
 * @param net              InOut: The p-net stack instance
 * @param p_uuid           In:  The AR_UUID.
 * @param pp_ar            Out: The AR (or NULL).
 * @return  0  if AR is found and in "used" state.
 *         -1  if the AR_UUID is invalid.
 */
static int pf_ar_find_by_uuid(
   pnet_t                  *net,
   pf_uuid_t               *p_uuid,
   pf_ar_t                 **pp_ar)
{
   int                     ret = -1;
   uint16_t                ix;
   pf_cmdev_state_values_t cmdev_state;

   ix = 0;
   while ((ix < NELEMENTS(net->cmrpc_ar)) &&
          ((net->cmrpc_ar[ix].in_use == false) ||
           ((pf_cmdev_get_state(&net->cmrpc_ar[ix], &cmdev_state) == 0) &&
            (cmdev_state == PF_CMDEV_STATE_POWER_ON)) ||
           (memcmp(p_uuid, &net->cmrpc_ar[ix].ar_param.ar_uuid, sizeof(*p_uuid)) != 0)))
   {
      ix++;
   }
   if (ix < NELEMENTS(net->cmrpc_ar))
   {
      *pp_ar = &net->cmrpc_ar[ix];
      ret = 0;
   }

   return ret;
}

pf_ar_t *pf_ar_find_by_index(
   pnet_t                  *net,
   uint16_t                ix)
{
   if (ix < NELEMENTS(net->cmrpc_ar))
   {
      return &net->cmrpc_ar[ix];
   }

   return NULL;
}

int pf_ar_find_by_arep(
   pnet_t                  *net,
   uint32_t                arep,
   pf_ar_t                 **pp_ar)
{
   int                        ret = -1;
   uint16_t                   ix;

   if (arep > 0)
   {
      ix = arep - 1;    /* Convert to index */
      if ((ix < NELEMENTS(net->cmrpc_ar)) &&
          (net->cmrpc_ar[ix].in_use == true))
      {
         *pp_ar = &net->cmrpc_ar[ix];
         ret = 0;
      }
   }

   return ret;
}

/******************** Error handling *****************************************/

void pf_set_error(
   pnet_result_t           *p_stat,
   uint8_t                 code,
   uint8_t                 decode,
   uint8_t                 code_1,
   uint8_t                 code_2)
{
   LOG_INFO(PNET_LOG, "ERROR: %u, %u, %u, %u\n", (unsigned)code, (unsigned)decode, (unsigned)code_1, (unsigned)code_2);
   p_stat->pnio_status.error_code = code;
   p_stat->pnio_status.error_decode = decode;
   p_stat->pnio_status.error_code_1 = code_1;
   p_stat->pnio_status.error_code_2 = code_2;
}

/******************** RPC parsers *******************************************/

/**
 * @internal
 * Check if block header length and version is valid.
 * @param block_length     In:   The block length.
 * @param p_block_header   In:   The block header instance.
 * @param err_code_1       In:   error_code_1 to use if an error is found.
 * @param p_stat           Out:  Detailed error information.
 * @return  0  if no error was found
 *          -1 if an error was found.
 */
static int pf_check_block_header(
   uint16_t                block_length,
   pf_block_header_t       *p_block_header,
   uint8_t                 err_code_1,
   pnet_result_t           *p_stat)
{
   int   ret = 0;       /* OK until error found. */

   if (p_block_header->block_length != block_length)
   {
      pf_set_error(p_stat, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, err_code_1, 1);
      ret = -1;
   }
   else if (p_block_header->block_version_high != PNET_BLOCK_VERSION_HIGH)
   {
      pf_set_error(p_stat, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, err_code_1, 2);
      ret = -1;
   }
   else if (p_block_header->block_version_low != PNET_BLOCK_VERSION_LOW)
   {
      pf_set_error(p_stat, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, err_code_1, 3);
      ret = -1;
   }

   return ret;
}

/**
 * @internal
 * Parse all blocks in a connect RPC request message.
 * @param p_sess           In:   The RPC session instance.
 * @param p_pos            InOut:Position in the input buffer.
 * @param p_ar             InOut:The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rm_connect_interpret_ind(
   pf_session_info_t       *p_sess,
   uint16_t                *p_pos,
   pf_ar_t                 *p_ar)
{
   int                     ret = 0;       /* OK until error found. */
   uint16_t                start_pos;
   pf_block_header_t       block_header;
   uint16_t                data_pos;
   bool                    first_block = true;

   start_pos = *p_pos;
   while ((ret == 0) &&
         (p_sess->get_info.result == PF_PARSE_OK) &&
         ((*p_pos + sizeof(block_header)) <= p_sess->get_info.len))
   {
      pf_get_block_header(&p_sess->get_info, p_pos, &block_header);
      data_pos = *p_pos;
      if ((p_sess->get_info.result == PF_PARSE_OK) &&
          (((*p_pos + block_header.block_length + 4) - sizeof(block_header)) <= p_sess->get_info.len))
      {
         switch (block_header.block_type)
         {
         case PF_BT_AR_BLOCK_REQ:
            pf_get_ar_param(&p_sess->get_info, p_pos, p_ar);

            /* Must be first block */
            if (first_block == false)
            {
               pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ, 0x00);
               ret = -1;
            }
            else
            {
               ret = pf_check_block_header(54 + (uint16_t)strlen(p_ar->ar_param.cm_initiator_station_name),
                  &block_header, PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ, &p_sess->rpc_result);
               if (ret == 0)
               {
                  p_ar->nbr_ar_param++;
               }
            }
            break;
         case PF_BT_IOCR_BLOCK_REQ:
            pf_get_iocr_param(&p_sess->get_info, p_pos, p_ar->nbr_iocrs, p_ar);

            /* Count the types for error discovery */
            if (p_ar->iocrs[p_ar->nbr_iocrs].param.iocr_type == PF_IOCR_TYPE_INPUT)
            {
               p_ar->input_cr_cnt++;
            }
            if (p_ar->iocrs[p_ar->nbr_iocrs].param.iocr_type == PF_IOCR_TYPE_OUTPUT)
            {
               p_ar->output_cr_cnt++;
            }
            if (p_ar->iocrs[p_ar->nbr_iocrs].param.iocr_type == PF_IOCR_TYPE_MC_CONSUMER)
            {
               p_ar->mcr_cons_cnt++;
            }
            if (p_ar->iocrs[p_ar->nbr_iocrs].param.iocr_properties.rt_class == PF_RT_CLASS_3)
            {
               p_ar->rtc3_present = true;
            }

            if (p_ar->ar_param.ar_properties.device_access == true)
            {
               pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
               ret = -1;
            }
            else
            {
               ret = pf_check_block_header((*p_pos - data_pos) + 2, &block_header, PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ, &p_sess->rpc_result);
               if (ret == 0)
               {
                  p_ar->nbr_iocrs++;
               }
            }
            break;
         case PF_BT_EXPECTED_SUBMODULE_BLOCK:
            pf_get_exp_api_module(&p_sess->get_info, p_pos, p_ar);
            switch (p_sess->get_info.result)
            {
            case PF_PARSE_OK:
               if (p_ar->ar_param.ar_properties.device_access == true)
               {
                  pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
                  ret = -1;
               }
               else
               {
                  ret = pf_check_block_header((*p_pos - data_pos) + 2, &block_header, PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ, &p_sess->rpc_result);
                  /* nbr_exp_api is counted in pf_get_exp_api_module() */
               }
               break;
            case PF_PARSE_NULL_POINTER:
               ret = -1;
               break;
            case PF_PARSE_END_OF_INPUT:
               LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): Out of expected API resources\n", __LINE__);
               ret = -1;
               break;
            case PF_PARSE_OUT_OF_API_RESOURCES:
               LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): Out of expected API resources\n", __LINE__);
               ret = -1;
               break;
            case PF_PARSE_OUT_OF_EXP_SUBMODULE_RESOURCES:
               LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): Out of expected submodule resources\n", __LINE__);
               ret = -1;
               break;
            case PF_PARSE_ERROR:
               /* Already handled */
               ret = -1;
               break;
            default:
               LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): Unhandled parser error %d\n", __LINE__, ret);
               ret = -1;
               break;
            }
            break;
         case PF_BT_ALARM_CR_BLOCK_REQ:
            pf_get_alarm_cr_request(&p_sess->get_info, p_pos, p_ar);

            if (p_ar->ar_param.ar_properties.device_access == true)
            {
               pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
               ret = -1;
            }
            else
            {
               ret = pf_check_block_header((*p_pos - data_pos) + 2, &block_header, PNET_ERROR_CODE_1_CONN_FAULTY_ALARM_BLOCK_REQ, &p_sess->rpc_result);
               if (ret == 0)
               {
                  p_ar->nbr_alarm_cr++;
               }
            }
            break;

#if PNET_OPTION_PARAMETER_SERVER
         case PF_BT_PRM_SERVER_REQ:
            pf_get_ar_prm_server_request(&p_sess->get_info, p_pos, p_ar);

            if (p_ar->ar_param.ar_properties.device_access == true)
            {
               pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
               ret = -1;
            }
            else
            {
               ret = pf_check_block_header((*p_pos - data_pos) + 2, &block_header, PNET_ERROR_CODE_1_CONN_FAULTY_PRM_SERVER_BLOCK_REQ, &p_sess->rpc_result);
               if (ret == 0)
               {
                  p_ar->nbr_prm_server++;
               }
            }
            break;
#endif
#if PNET_OPTION_MC_CR
         case PF_BT_MCR_REQ:
            pf_get_mcr_request(&p_sess->get_info, p_pos, p_ar->nbr_mcr, p_ar);
            if (p_ar->ar_param.ar_properties.device_access == true)
            {
               pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
               ret = -1;
            }
            else
            {
               ret = pf_check_block_header((*p_pos - data_pos) + 2, &block_header, PNET_ERROR_CODE_1_CONN_FAULTY_MCR_BLOCK_REQ, &p_sess->rpc_result);
               if (ret == 0)
               {
                  p_ar->nbr_mcr++;
               }
            }
            break;
#endif
         case PF_BT_RPC_SERVER_REQ:
            pf_get_ar_rpc_request(&p_sess->get_info, p_pos, p_ar);

            if (p_ar->ar_param.ar_properties.device_access == true)
            {
               pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
               ret = -1;
            }
            else
            {
               ret = pf_check_block_header((*p_pos - data_pos) + 2, &block_header, PNET_ERROR_CODE_1_CONN_FAULTY_AR_RPC_BLOCK_REQ, &p_sess->rpc_result);
               if (ret == 0)
               {
                  p_ar->nbr_rpc_server++;
               }
            }
            break;
#if PNET_OPTION_AR_VENDOR_BLOCKS
         case PF_BT_AR_VENDOR_BLOCK_REQ:
            pf_get_ar_vendor_request(&p_sess->get_info, p_pos, p_ar->nbr_ar_vendor, p_ar);

            if (p_ar->ar_param.ar_properties.device_access == true)
            {
               pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
               ret = -1;
            }
            else
            {
               ret = pf_check_block_header((*p_pos - data_pos) + 2, &block_header, PNET_ERROR_CODE_1_CONN_FAULTY_FAULTY_RECORD, &p_sess->rpc_result);
               if (ret == 0)
               {
                  p_ar->nbr_ar_vendor++;
               }
            }
            break;
#endif
#if PNET_OPTION_IR
         case PF_BT_IR_INFO_BLOCK_REQ:
            pf_get_ir_info_request(&p_sess->get_info, p_pos, p_ar);

            if (p_ar->ar_param.ar_properties.device_access == true)
            {
               pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
               ret = -1;
            }
            else
            {
               ret = pf_check_block_header((*p_pos - data_pos) + 2, &block_header, PNET_ERROR_CODE_1_CONN_FAULTY_IR_INFO, &p_sess->rpc_result);
               if (ret == 0)
               {
                  p_ar->nbr_ir_info++;
               }
            }
            break;
#endif
         default:
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): Unknown block type %u\n", __LINE__, block_header.block_type);
            ret = -1;
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
            break;
         }
      }
      else
      {
         LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): Unexpected end of input\n", __LINE__);
         ret = -1;
      }
      first_block = false;
   }

   if (ret == 0)
   {
      /* Error discovery. */
      if (p_sess->get_info.result == PF_PARSE_OK)
      {
         if (*p_pos != p_sess->get_info.len)
         {
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
            ret = -1;
         }
         else if (((uint32_t)*p_pos - start_pos) != p_sess->ndr_data.args_length)
         {
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): args_length %" PRIu32 " != request length %" PRIu32 "\n", __LINE__,
               p_sess->ndr_data.args_length, (uint32_t)(*p_pos - start_pos));
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
            ret = -1;
         }
      }
      else
      {
         pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
         ret = -1;
      }
   }

   return ret;
}

/**
 * @internal
 * Create all blocks in a connect RPC response.
 * @param p_sess           In:   The RPC session instance.
 * @param ret              In:   result of the connect request.
 * @param p_ar             In:   The AR instance.
 * @param res_size         In:   Size of the response buffer.
 * @param p_res            InOut:Response buffer.
 * @param p_res_pos        InOut:Position in the response buffer.
 */
static void pf_cmrpc_rm_connect_rsp(
   pf_session_info_t       *p_sess,
   int                     ret,
   pf_ar_t                 *p_ar,
   uint16_t                res_size,
   uint8_t                 *p_res,
   uint16_t                *p_res_pos)
{
   uint16_t                ix;
   uint16_t                hdr_pos;
   uint16_t                start_pos;
   uint16_t                res_len = res_size;

   pf_put_pnet_status(p_sess->get_info.is_big_endian, &p_sess->rpc_result.pnio_status, res_len, p_res, p_res_pos);

   hdr_pos = *p_res_pos;    /* Save for last */
   /* Insert the response header with dummy length and actual_count */
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.args_length, res_len, p_res, p_res_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.maximum_count, res_len, p_res, p_res_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.offset, res_len, p_res, p_res_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.actual_count, res_len, p_res, p_res_pos);

   /* Limit our response to what the client can handle. */
   if (*p_res_pos + p_sess->ndr_data.args_maximum < res_size)
   {
      res_len = *p_res_pos + p_sess->ndr_data.args_maximum;
   }

   start_pos = *p_res_pos;    /* Start of blocks - save for last */

   /**
    * ARBlockRes, {[IOCRBlockRes*], [AlarmCRBlockRes], [ModuleDiffBlock],
    * [ARRPCBlockRes](a)}
    *
    * Special case "PF_ART_IOCAR_SINGLE" or "PF_ART_IOSAR":
    * ARBlockRes, {IOCRBlockRes*, AlarmCRBlockRes, [ModuleDiffBlock],
    * [ARRPCBlockRes](a)}
    *
    * Special case "PF_ART_IOSAR" with ar_properties.device_access = PF_DA_DEVICE_CONTEXT:
    * ARBlockRes
    *
    * (a) Only if request contains an ARRPCBlockReq
    */

   if ((ret == 0) && (p_ar != NULL))
   {
      pf_put_ar_result(p_sess->get_info.is_big_endian, p_ar, res_len, p_res, p_res_pos);

      for (ix = 0; ix < p_ar->nbr_iocrs; ix++)
      {
         pf_put_iocr_result(p_sess->get_info.is_big_endian, p_ar, ix, res_len, p_res, p_res_pos);
      }

      pf_put_alarm_cr_result(p_sess->get_info.is_big_endian, p_ar, res_len, p_res, p_res_pos);

      if (p_ar->nbr_api_diffs > 0)
      {
         pf_put_ar_diff(p_sess->get_info.is_big_endian, p_ar, res_len, p_res, p_res_pos);
      }

      /* Only if RPC server request */
      if (p_ar->ar_rpc_request.valid)
      {
         pf_put_ar_rpc_result(p_sess->get_info.is_big_endian, p_ar, res_len, p_res, p_res_pos);
      }

      if (p_ar->ar_param.ar_properties.startup_mode)
      {
         /**
          * Additional blocks in advanced start mode:
          * {ARServerBlockRes, [ARVendorBlockRes*](b)}
          *
          * (b) Equal to number of ARVendorBlockReq in request.
          */

         pf_put_ar_server_result(p_sess->get_info.is_big_endian, p_ar, res_len, p_res, p_res_pos);

#if PNET_OPTION_AR_VENDOR_BLOCKS
         for (ix = 0; ix < p_ar->nbr_ar_vendor; ix++)
         {
            pf_put_ar_vendor_result(p_sess->get_info.is_big_endian, p_ar, ix, res_len, p_res, p_res_pos);
         }
#endif
      }
   }

   /* Fixup the header with correct length info. */
   p_sess->ndr_data.args_length = *p_res_pos - start_pos;
   p_sess->ndr_data.array.actual_count = *p_res_pos - start_pos;

   LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): args_length = %" PRIu32 "\n", __LINE__, p_sess->ndr_data.args_length);

   /* Over-write the response header with correct length and actual_count. */
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.args_length, res_len, p_res, &hdr_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.maximum_count, res_len, p_res, &hdr_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.offset, res_len, p_res, &hdr_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.actual_count, res_len, p_res, &hdr_pos);
}

/**
 * @internal
 * Take a DCE RPC connect request and create a DCE RPC connect response.
 *
 * This will trigger these user callbacks:
 *
 *  * pnet_exp_module_ind()
 *  * pnet_exp_submodule_ind()
 *  * pnet_connect_ind()
 *  * pnet_state_ind() with PNET_EVENT_STARTUP
 *
 * @param net              InOut: The p-net stack instance
 * @param p_sess           In:   The session instance.
 * @param req_pos          In:   Position in the request buffer.
 * @param res_size         In:   The size of the response buffer.
 * @param p_res            Out:  The response buffer.
 * @param p_res_pos        InOut:Position within the response buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rm_connect_ind(
   pnet_t                  *net,
   pf_session_info_t       *p_sess,
   uint16_t                req_pos,
   uint16_t                res_size,
   uint8_t                 *p_res,
   uint16_t                *p_res_pos)
{
   int                     ret = -1;
   pf_ar_t                 *p_ar = NULL;
   pf_ar_t                 *p_ar_2 = NULL;

   if (p_sess->rpc_result.pnio_status.error_code != 0)
   {
      LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): RPC request has error\n", __LINE__);
      pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT);
   }
   /* CheckResource */
   else if (pf_ar_allocate(net, &p_ar) == 0)
   {
      /* Cross-reference */
      p_ar->p_sess = p_sess;
      p_sess->p_ar = p_ar;

      /* Check_RPC - No support for ArSet (yet) */
      if (pf_cmrpc_rm_connect_interpret_ind(p_sess, &req_pos, p_ar) == 0)
      {
         if (pf_ar_find_by_uuid(net, &p_ar->ar_param.ar_uuid, &p_ar_2) != 0)
         {
            /* Valid, unknown AR, NoArSet */
            p_ar->ar_state = PF_AR_STATE_PRIMARY;
            p_ar->sync_state = PF_SYNC_STATE_NOT_AVAILABLE;

            ret = pf_cmdev_rm_connect_ind(net, p_ar, &p_sess->rpc_result);
         }
         else
         {
            /* Valid, explicit AR - This could be a re-connect */
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT);
            (void)pf_cmdev_state_ind(net, p_ar, PNET_EVENT_ABORT);
         }
      }
      else
      {
         LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): Connect interpreter error\n", __LINE__);
         /* Invalid - ret already set */
      }
   }
   else
   {
      /* unavailable */
      LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): Out of resources (AR)\n", __LINE__);
      pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_NO_AR_RESOURCES);
   }

   LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): CONNECT: ret = %d   error_code=%" PRIu8 "  error_decode=%" PRIu8  "error_code_1=%" PRIu8 "  error_code_2=%" PRIu8 "\n", __LINE__,
      ret, p_sess->rpc_result.pnio_status.error_code, p_sess->rpc_result.pnio_status.error_decode, p_sess->rpc_result.pnio_status.error_code_1, p_sess->rpc_result.pnio_status.error_code_2);

   pf_cmrpc_rm_connect_rsp(p_sess, ret, p_ar, res_size, p_res, p_res_pos);

   if ((ret != 0) || (p_sess->rpc_result.pnio_status.error_code != 0))
   {
      /* Connect failed: Terminate session - free all resources */
      LOG_INFO(PF_RPC_LOG, "RPC(%d): Connect failed - Free all!\n", __LINE__);
      pf_session_release(p_sess);
      pf_ar_release(p_ar);
   }

   LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): Connect response %d\n", __LINE__, ret);
   /* RPC_connect_rsp */
   return ret;
}

/**
 * @internal
 * Parse all blocks in a release RPC request.
 * @param p_sess           In:   The RPC session instance.
 * @param req_pos          In:   Position in the request buffer.
 * @param p_release_io     In:   The release control block.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rm_release_interpret_req(
   pf_session_info_t       *p_sess,
   uint16_t                req_pos,
   pf_control_block_t      *p_release_io)
{
   int                     ret = 0;
   uint16_t                start_pos;
   pf_block_header_t       block_header;

   start_pos = req_pos;
   while (((req_pos + sizeof(block_header)) <= p_sess->get_info.len) &&
          (p_sess->get_info.result == PF_PARSE_OK))
   {
      pf_get_block_header(&p_sess->get_info, &req_pos, &block_header);
      if ((((req_pos + block_header.block_length + 4) - sizeof(block_header)) <= p_sess->get_info.len) &&
          (p_sess->get_info.result == PF_PARSE_OK))
      {
         switch (block_header.block_type)
         {
         case PF_BT_RELEASE_BLOCK_REQ:
            pf_get_control(&p_sess->get_info, &req_pos, p_release_io);
            break;
         default:
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): Unknown block type %u\n", __LINE__, block_header.block_type);
            ret = -1;
            break;
         }
      }
   }

   if (ret == 0)
   {
      if (p_sess->get_info.result == PF_PARSE_OK)
      {
         /* Error discovery. */
         if (req_pos != p_sess->get_info.len)
         {
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_RELEASE, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
            ret = -1;
         }
         else if ((uint32_t)(req_pos - start_pos) != p_sess->ndr_data.args_length)
         {
            LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): args_length %" PRIu32 " != request length %" PRIu32 "\n", __LINE__,
               p_sess->ndr_data.args_length, (uint32_t)(req_pos - start_pos));
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_RELEASE, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
            ret = -1;
         }
         else if (p_release_io->control_command != BIT(PF_CONTROL_COMMAND_BIT_RELEASE))
         {
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): ControlCommand has more than one bit set: %04x\n", __LINE__, p_release_io->control_command);
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_RELEASE, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT);
            ret = -1;
         }
      }
      else
      {
         pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_RELEASE, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
         ret = -1;
      }
   }

   return ret;
}

/**
 * @internal
 * Create all blocks in a release RPC response.
 * @param p_sess           In:   The RPC session instance.
 * @param p_release_io     In:   The release control block.
 * @param res_size         In:   The size of the response buffer.
 * @param p_res            Out:  The response buffer.
 * @param p_res_pos        InOut:Position within the response buffer.
 * @param p_status_pos     Out:  Position of the status within the response buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static void pf_cmrpc_rm_release_rsp(
   pf_session_info_t       *p_sess,
   pf_control_block_t      *p_release_io,
   uint16_t                res_size,
   uint8_t                 *p_res,
   uint16_t                *p_res_pos,
   uint16_t                *p_status_pos)
{
   uint16_t                hdr_pos;
   uint16_t                start_pos;
   uint16_t                res_len = res_size;

   *p_status_pos = *p_res_pos;
   pf_put_pnet_status(p_sess->get_info.is_big_endian, &p_sess->rpc_result.pnio_status, res_len, p_res, p_res_pos);

   hdr_pos = *p_res_pos;    /* Save for last. */
   /* Insert the response header with dummy length and actual_count. */
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.args_length, res_len, p_res, p_res_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.maximum_count, res_len, p_res, p_res_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.offset, res_len, p_res, p_res_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.actual_count, res_len, p_res, p_res_pos);

   /* Limit our response to what the client can handle. */
   if (*p_res_pos + p_sess->ndr_data.args_maximum < res_size)
   {
      res_len = *p_res_pos + p_sess->ndr_data.args_maximum;
   }

   start_pos = *p_res_pos;    /* Start of blocks - save for last */

   if (p_sess->rpc_result.pnio_status.error_code == 0)
   {
      p_release_io->control_command = BIT(PF_CONTROL_COMMAND_BIT_DONE);
      pf_put_control(p_sess->get_info.is_big_endian, PF_BT_RELEASE_BLOCK_RES, p_release_io, res_len, p_res, p_res_pos);
   }

   /* Fixup the header with correct length info. */
   p_sess->ndr_data.args_length = *p_res_pos - start_pos;
   p_sess->ndr_data.array.actual_count = *p_res_pos - start_pos;

   /* Over-write the response header with correct length and actual_count. */
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.args_length, res_len, p_res, &hdr_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.maximum_count, res_len, p_res, &hdr_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.offset, res_len, p_res, &hdr_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.actual_count, res_len, p_res, &hdr_pos);
}

/**
 * @internal
 * Take a DCE RPC release request and create a DCE RPC release response.
 * @param net              InOut: The p-net stack instance
 * @param p_sess           In:   The RPC session instance.
 * @param req_pos          In:   Position in the request buffer.
 * @param res_size         In:   The size of the response buffer.
 * @param p_res            Out:  The response buffer.
 * @param p_res_pos        InOut:Position within the response buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rm_release_ind(
   pnet_t                  *net,
   pf_session_info_t       *p_sess,
   uint16_t                req_pos,
   uint16_t                res_size,
   uint8_t                 *p_res,
   uint16_t                *p_res_pos)
{
   int                     ret = -1;
   pf_control_block_t      release_io;
   pf_ar_t                 *p_ar = NULL;
   uint16_t                status_pos;
   pnet_result_t           rpc_result;
   bool                    is_big_endian;

   memset(&release_io, 0, sizeof(release_io));
   memset(&rpc_result, 0, sizeof(rpc_result));

   /* Save things for creating the response (incl. status_pos, below...) */
   is_big_endian = p_sess->get_info.is_big_endian;
   /* Create a positive response in case all goes well. */
   pf_cmrpc_rm_release_rsp(p_sess, &release_io, res_size, p_res, p_res_pos, &status_pos);

   if (p_sess->rpc_result.pnio_status.error_code != 0)
   {
      LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): RPC request has error\n", __LINE__);
      pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_RELEASE, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT);
   }
   else
   {
      ret = pf_cmrpc_rm_release_interpret_req(p_sess, req_pos, &release_io);
   }

   rpc_result = p_sess->rpc_result;

   if (ret == 0)
   {
      /* Check_RPC */
      if ((pf_ar_find_by_uuid(net, &release_io.ar_uuid, &p_ar) == 0) &&
         (release_io.session_key == p_ar->ar_param.session_key))
      {
         (void)pf_cmdev_rm_release_ind(net, p_ar, &rpc_result);
      }
      else
      {
         pf_set_error(&rpc_result, PNET_ERROR_CODE_RELEASE, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_AR_UUID_UNKNOWN);
      }
   }

   /* Now insert the actual status */
   pf_put_pnet_status(is_big_endian, &rpc_result.pnio_status, res_size, p_res, &status_pos);

   return ret;
}

/**
 * @internal
 * Parse all blocks in a DControl RPC request message.
 * @param p_sess           In:   The RPC session instance.
 * @param req_pos          In:   Position in the request buffer.
 * @param p_control_io     Out:  The DControl control block.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rm_dcontrol_interpret_req(
   pf_session_info_t       *p_sess,
   uint16_t                req_pos,
   pf_control_block_t      *p_control_io)
{
   int                     ret = 0;
   uint16_t                start_pos;
   pf_block_header_t       block_header;

   start_pos = req_pos;
   while (((req_pos + sizeof(block_header)) <= p_sess->get_info.len) &&
          (p_sess->get_info.result == PF_PARSE_OK))
   {
      pf_get_block_header(&p_sess->get_info, &req_pos, &block_header);
      if ((((req_pos + block_header.block_length + 4) - sizeof(block_header)) <= p_sess->get_info.len) &&
          (p_sess->get_info.result == PF_PARSE_OK))
      {
         switch (block_header.block_type)
         {
         case PF_BT_PRMBEGIN_REQ:
         case PF_BT_PRMEND_REQ:
            pf_get_control(&p_sess->get_info, &req_pos, p_control_io);
            break;
         default:
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONTROL, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): Unknown block type %u\n", __LINE__, block_header.block_type);
            ret = -1;
            break;
         }
      }
   }

   if (ret == 0)
   {
      if (p_sess->get_info.result == PF_PARSE_OK)
      {
         /* Error discovery. */
         if (req_pos != p_sess->get_info.len)
         {
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONTROL, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
            ret = -1;
         }
         else if ((uint32_t)(req_pos - start_pos) != p_sess->ndr_data.args_length)
         {
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): args_length %" PRIu32 " != request length %" PRIu32 "\n", __LINE__,
               p_sess->ndr_data.args_length, (uint32_t)(req_pos - start_pos));
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONTROL, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
            ret = -1;
         }
         else if (p_control_io->control_command != BIT(PF_CONTROL_COMMAND_BIT_PRM_END))
         {
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): ControlCommand has more than one bit set: %04x\n", __LINE__, p_control_io->control_command);
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONTROL, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT);
            ret = -1;
         }
      }
      else
      {
         pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONTROL, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
         ret = -1;
      }
   }

   return ret;
}

/**
 * @internal
 * Create all blocks in a DControl RPC response.
 * @param p_sess           In:   The RPC session instance.
 * @param ret              In:   Result of the release operation.
 * @param p_control_io     In:   The DControl control block.
 * @param res_size         In:   The size of the response buffer.
 * @param p_res            Out:  The response buffer.
 * @param p_pos            InOut:Position within the response buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static void pf_cmrpc_rm_dcontrol_rsp(
   pf_session_info_t       *p_sess,
   int                     ret,
   pf_control_block_t      *p_control_io,
   uint16_t                res_size,
   uint8_t                 *p_res,
   uint16_t                *p_res_pos)
{
   uint16_t                hdr_pos;
   uint16_t                start_pos;
   uint16_t                res_len = res_size;

   pf_put_pnet_status(p_sess->get_info.is_big_endian, &p_sess->rpc_result.pnio_status, res_len, p_res, p_res_pos);

   hdr_pos = *p_res_pos;    /* Save for last */
   /* Insert the response header with dummy length and actual_count */
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.args_length, res_len, p_res, p_res_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.maximum_count, res_len, p_res, p_res_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.offset, res_len, p_res, p_res_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.actual_count, res_len, p_res, p_res_pos);

   /* Limit our response to what the client can handle. */
   if (*p_res_pos + p_sess->ndr_data.args_maximum < res_size)
   {
      res_len = *p_res_pos + p_sess->ndr_data.args_maximum;
   }

   start_pos = *p_res_pos;    /* Start of blocks - save for last */

   if (p_sess->rpc_result.pnio_status.error_code == 0)
   {
      p_control_io->control_command = BIT(PF_CONTROL_COMMAND_BIT_DONE);
      pf_put_control(p_sess->get_info.is_big_endian, PF_BT_PRMEND_RES, p_control_io, res_len, p_res, p_res_pos);
   }

   /* Fixup the header with correct length info. */
   p_sess->ndr_data.args_length = *p_res_pos - start_pos;
   p_sess->ndr_data.array.actual_count = *p_res_pos - start_pos;

   /* Over-write the response header with correct length and actual_count. */
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.args_length, res_len, p_res, &hdr_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.maximum_count, res_len, p_res, &hdr_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.offset, res_len, p_res, &hdr_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.actual_count, res_len, p_res, &hdr_pos);
}

/**
 * @internal
 * Take a DCE RPC DControl request and create a DCE RPC DControl response.
 * @param net              InOut: The p-net stack instance
 * @param p_sess           In:   The RPC session instance.
 * @param req_pos          In:   Position in the request buffer.
 * @param res_size         In:   The size of the response buffer.
 * @param p_res            Out:  The response buffer.
 * @param p_res_pos        InOut:Position within the response buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rm_dcontrol_ind(
   pnet_t                  *net,
   pf_session_info_t       *p_sess,
   pf_rpc_header_t         *p_rpc,
   uint16_t                req_pos,
   uint16_t                res_size,
   uint8_t                 *p_res,
   uint16_t                *p_res_pos)
{
   int                     ret = -1;
   pf_control_block_t      control_io;
   pf_ar_t                 *p_ar = NULL;

   memset(&control_io, 0, sizeof(control_io));

   if (pf_cmrpc_rm_dcontrol_interpret_req(p_sess, req_pos, &control_io) == 0)
   {
      /* Detect re-run */
      if (p_sess->dcontrol_sequence_nmb != p_rpc->sequence_nmb)
      {
         if (pf_ar_find_by_uuid(net, &control_io.ar_uuid, &p_ar) == 0)
         {
            if (pf_cmdev_rm_dcontrol_ind(net, p_ar, &control_io, &p_sess->rpc_result) == 0)
            {
               ret = pf_cmpbe_rm_dcontrol_ind(net, p_ar, &control_io, &p_sess->rpc_result);
            }
         }
         else
         {
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONTROL, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_AR_UUID_UNKNOWN);
         }

         /* Store dcontrol result in we later detect a re-run */
         p_sess->dcontrol_result = p_sess->rpc_result;
         p_sess->dcontrol_sequence_nmb = p_rpc->sequence_nmb;
      }
      else
      {
         /* Re-send the previous answer */
         p_sess->rpc_result = p_sess->dcontrol_result;
      }
   }

   LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): DCONTROL: ret = %d   error_code=%" PRIu8 "  error_decode=%" PRIu8  "error_code_1=%" PRIu8 "  error_code_2=%" PRIu8 "\n", __LINE__,
      ret, p_sess->rpc_result.pnio_status.error_code, p_sess->rpc_result.pnio_status.error_decode, p_sess->rpc_result.pnio_status.error_code_1, p_sess->rpc_result.pnio_status.error_code_2);

   pf_cmrpc_rm_dcontrol_rsp(p_sess, ret, &control_io, res_size, p_res, p_res_pos);

   return ret;
}

/**
 * @internal
 * Parse all blocks in a IODRead RPC request message.
 * @param p_sess           In:   The RPC session instance.
 * @param req_pos          In:   Position in the request buffer.
 * @param p_read_request   In:   The IODRead request block.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rm_read_interpret_ind(
   pf_session_info_t       *p_sess,
   uint16_t                req_pos,
   pf_iod_read_request_t   *p_read_request)
{
   int                     ret = -1;
   uint16_t                start_pos;
   pf_block_header_t       block_header;

   start_pos = req_pos;
   if (((req_pos + sizeof(block_header)) <= p_sess->get_info.len) &&
       (p_sess->get_info.result == PF_PARSE_OK))
   {
      pf_get_block_header(&p_sess->get_info, &req_pos, &block_header);
      if ((((req_pos + block_header.block_length + 4) - sizeof(block_header)) <= p_sess->get_info.len) &&
          (p_sess->get_info.result == PF_PARSE_OK))
      {
         switch (block_header.block_type)
         {
         case PF_BT_IOD_READ_REQ_HEADER:
            pf_get_read_request(&p_sess->get_info, &req_pos, p_read_request);
            ret = 0;
            break;
         default:
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_READ, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CONN_FAULTY_FAULTY_RECORD, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): Unknown block type %u\n", __LINE__, block_header.block_type);
            break;
         }
      }
   }

   if (ret == 0)
   {
      if (p_sess->get_info.result == PF_PARSE_OK)
      {
         /* Error discovery. */
         if (req_pos != p_sess->get_info.len)
         {
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_READ, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
            ret = -1;
         }
         else if ((uint32_t)(req_pos - start_pos) != p_sess->ndr_data.args_length)
         {
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): args_length %" PRIu32 " != request length %" PRIu32 "\n", __LINE__,
               p_sess->ndr_data.args_length, (uint32_t)(req_pos - start_pos));
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_READ, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
            ret = -1;
         }
      }
      else
      {
         pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_READ, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
         ret = -1;
      }
   }

   return ret;
}

/**
 * @internal
 * Take a DCE RPC IODRead request and create a DCE RPC IODRead response.
 *
 * This will trigger the \a pnet_read_ind() user callback for certain parameters.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_sess           In:   The session instance.
 * @param opnum            In:   The RPC operation number.
 * @param req_pos          In:   Position in the request buffer.
 * @param res_size         In:   The size of the response buffer.
 * @param p_res            Out:  The response buffer.
 * @param p_res_pos        InOut:Position within the response buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rm_read_ind(
   pnet_t                  *net,
   pf_session_info_t       *p_sess,
   uint16_t                opnum,
   uint16_t                req_pos,
   uint16_t                res_size,
   uint8_t                 *p_res,
   uint16_t                *p_res_pos)
{
   int                     ret = -1;
   pf_iod_read_request_t   read_request;
   pf_ar_t                 *p_ar = NULL;  /* Assume the implicit AR */
   uint16_t                res_len = res_size;
   uint16_t                status_pos;
   uint16_t                hdr_pos;
   uint16_t                start_pos;
   pf_api_t                *p_api = NULL;

   memset(&read_request, 0, sizeof(read_request));

   if (p_sess->rpc_result.pnio_status.error_code != 0)
   {
      LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): RPC request has error\n", __LINE__);
      pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_READ, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT);
   }
   else if (pf_cmrpc_rm_read_interpret_ind(p_sess, req_pos, &read_request) == 0)
   {
      if ((memcmp(&read_request.ar_uuid, &implicit_ar, sizeof(implicit_ar)) == 0) &&
          (opnum != PF_RPC_DEV_OPNUM_READ_IMPLICIT))
      {
         pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_READ, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CONN_FAULTY_FAULTY_RECORD, PNET_ERROR_CODE_2_CMRPC_AR_UUID_UNKNOWN);
      }
      else if ((pf_ar_find_by_uuid(net, &read_request.ar_uuid, &p_ar) == 0) &&
            (opnum != PF_RPC_DEV_OPNUM_READ))
      {
         pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_READ, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_AR_UUID_UNKNOWN);
      }
      else if ((read_request.api != 0) &&
               (pf_cmdev_get_api(net, read_request.api, &p_api) != 0))
      {
         pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_READ, PNET_ERROR_DECODE_PNIORW, PNET_ERROR_CODE_1_ACC_INVALID_AREA_API, 6);
      }
      else
      {
         /* Note that for "read implicit" we have no AR */
         if ((p_ar == NULL) && (opnum != PF_RPC_DEV_OPNUM_READ_IMPLICIT))
         {
            /* In case an AR is needed try to get the target AR */
            pf_ar_find_by_uuid(net, &read_request.target_ar_uuid, &p_ar);
         }
         /*
          * Read requests are special: There are so many different formats that
          * we will let the read functions read directly from the request buffer
          * and create their responses directly into the response buffer.
          */
         /* Limit our response to what the client can handle. */
         if (*p_res_pos + p_sess->ndr_data.args_maximum < res_size)
         {
            res_len = *p_res_pos + p_sess->ndr_data.args_maximum;
         }

         status_pos = *p_res_pos; /* Save for last. */
         pf_put_pnet_status(p_sess->get_info.is_big_endian, &p_sess->rpc_result.pnio_status, res_len, p_res, p_res_pos);

         hdr_pos = *p_res_pos;    /* Save for last. */
         /* Insert the response header with dummy length and actual_count. */
         pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.args_length, res_len, p_res, p_res_pos);
         pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.maximum_count, res_len, p_res, p_res_pos);
         pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.offset, res_len, p_res, p_res_pos);
         pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.actual_count, res_len, p_res, p_res_pos);

         start_pos = *p_res_pos;    /* Start of blocks - save for last */

         if (pf_cmrdr_rm_read_ind(net, p_ar, &read_request, &p_sess->rpc_result, res_size, p_res, p_res_pos) == 0)
         {
            ret = pf_cmsm_rm_read_ind(net, p_ar, &read_request);
         }
         else
         {
            LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): Error from pf_cmrdr_rm_read_ind\n", __LINE__);
         }

         /* Insert the actual operation result */
         pf_put_pnet_status(p_sess->get_info.is_big_endian, &p_sess->rpc_result.pnio_status, res_len, p_res, &status_pos);

         /* Fixup the header with correct length info. */
         p_sess->ndr_data.args_length = *p_res_pos - start_pos;
         p_sess->ndr_data.array.actual_count = *p_res_pos - start_pos;

         /* Over-write the response header with correct length and actual_count. */
         pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.args_length, res_len, p_res, &hdr_pos);
         pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.maximum_count, res_len, p_res, &hdr_pos);
         pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.offset, res_len, p_res, &hdr_pos);
         pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.actual_count, res_len, p_res, &hdr_pos);
      }
   }

   return ret;
}

/**
 * @internal
 * Parse one block in a IODWrite RPC request message.
 * @param p_get_info       In:   Parser data for the input buffer.
 * @param p_write_request  In:   The IODWrite request block.
 * @param p_pos            InOut:Position in the input buffer.
 * @param p_stat           Out:  Detailed error information.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rm_write_interpret_ind(
   pf_get_info_t           *p_get_info,
   pf_iod_write_request_t  *p_write_request,
   uint16_t                *p_pos,
   pnet_result_t           *p_stat)
{
   int                     ret = -1;
   pf_block_header_t       block_header;

   if ((*p_pos + sizeof(block_header)) <= p_get_info->len)
   {
      pf_get_block_header(p_get_info, p_pos, &block_header);
      if ((((*p_pos + block_header.block_length + 4) - sizeof(block_header)) <= p_get_info->len) &&
          (p_get_info->result == PF_PARSE_OK))
      {
         switch (block_header.block_type)
         {
         case PF_BT_IOD_WRITE_REQ_HEADER:
            pf_get_write_request(p_get_info, p_pos, p_write_request);
            ret = 0;
            break;
         default:
            pf_set_error(p_stat, PNET_ERROR_CODE_WRITE, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CONN_FAULTY_FAULTY_RECORD, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): Unknown block type %u\n", __LINE__, block_header.block_type);
            break;
         }
      }
   }

   return ret;
}

/**
 * @internal
 * Perform write of one data record.
 * @param net              InOut: The p-net stack instance
 * @param p_get_info       In:   Parser data for the input buffer.
 * @param p_write_request  In:   The IODWrite request block.
 * @param p_write_result   Out:  The IODWrite result block.
 * @param p_stat           Out:  Detailed error information.
 * @param p_req_pos        InOut:Position in the request buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_perform_one_write(
   pnet_t                  *net,
   pf_get_info_t           *p_get_info,
   pf_iod_write_request_t  *p_write_request,
   pf_iod_write_result_t   *p_write_result,
   pnet_result_t           *p_stat,
   uint16_t                *p_req_pos)
{
   int                     ret = -1;
   pf_ar_t                 *p_ar = NULL;
   pf_block_header_t       block_header;
   pf_api_t                *p_api = NULL;

   if (pf_ar_find_by_uuid(net, &p_write_request->ar_uuid, &p_ar) != 0)
   {
      pf_set_error(p_stat, PNET_ERROR_CODE_READ, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_AR_UUID_UNKNOWN);
   }
   else if ((p_write_request->api != 0) &&
            (pf_cmdev_get_api(net, p_write_request->api, &p_api) != 0))
   {
      pf_set_error(p_stat, PNET_ERROR_CODE_WRITE, PNET_ERROR_DECODE_PNIORW, PNET_ERROR_CODE_1_ACC_INVALID_AREA_API, 1);
   }
   else if ((*p_req_pos + p_write_request->record_data_length) <= p_get_info->len)
   {
      if (p_write_request->index < 0x8000)
      {
         /* This is a write of a GSDML param. No block header in this case. */
         if (pf_cmwrr_rm_write_ind(net, p_ar, p_write_request, p_write_result, p_stat,
            p_get_info->p_buf, p_write_request->record_data_length, p_req_pos) == 0)
         {
            ret = 0;
         }
         else
         {
            pf_set_error(p_stat, PNET_ERROR_CODE_WRITE, PNET_ERROR_DECODE_PNIORW, PNET_ERROR_CODE_1_ACC_INVALID_AREA_API, 2);
            ret = -1;
         }
      }
      else
      {
         /*
          * Write requests are special: There are so many different block types that
          * we will let CMWRR read data (sans block header) directly from the request buffer.
          *
          * Skip the block header - it is not needed in the stack.
          */
         pf_get_block_header(p_get_info, p_req_pos, &block_header);  /* Not needed by code!! */
         if (pf_cmwrr_rm_write_ind(net, p_ar, p_write_request, p_write_result, p_stat,
            p_get_info->p_buf, p_write_request->record_data_length - sizeof(pf_block_header_t), p_req_pos) == 0)
         {
            ret = 0;
         }
         else
         {
            pf_set_error(p_stat, PNET_ERROR_CODE_WRITE, PNET_ERROR_DECODE_PNIORW, PNET_ERROR_CODE_1_ACC_INVALID_AREA_API, 2);
            ret = -1;
         }
      }
   }
   else
   {
      pf_set_error(p_stat, PNET_ERROR_CODE_WRITE, PNET_ERROR_DECODE_PNIORW, PNET_ERROR_CODE_1_ACC_INVALID_AREA_API, 4);
      ret = -1;
   }

   return ret;
}

/**
 * @internal
 * Take a DCE RPC IODWrite request and create a DCE RPC IODWrite response.
 *
 * This will trigger the \a pnet_write_ind() user callback for certain parameters.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_sess           In:   The RPC session instance.
 * @param req_pos          In:   Position in the request buffer.
 * @param res_size         In:   The size of the response buffer.
 * @param p_res            Out:  The response buffer.
 * @param p_res_pos        InOut:Position within the response buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rm_write_ind(
   pnet_t                  *net,
   pf_session_info_t       *p_sess,
   uint16_t                req_pos,
   uint16_t                res_size,
   uint8_t                 *p_res,
   uint16_t                *p_res_pos)
{
   int                     ret = -1;
   pf_iod_write_request_t  write_request;
   pf_iod_write_result_t   write_result;
   pf_iod_write_request_t  write_request_multi;
   pf_iod_write_result_t   write_result_multi;
   pnet_result_t           write_stat_multi;
   uint16_t                res_len = res_size;
   uint16_t                req_start_pos;
   uint16_t                res_hdr_pos;
   uint16_t                res_start_pos;
   uint16_t                res_status_pos;

   memset(&write_request, 0, sizeof(write_request));
   memset(&write_result, 0, sizeof(write_result));
   memset(&write_request_multi, 0, sizeof(write_request_multi));
   memset(&write_result_multi, 0, sizeof(write_result_multi));
   memset(&write_stat_multi, 0, sizeof(write_stat_multi));

   /*
    * Read the first request block.
    * It might be the only one, but that will be dealt with below.
    */
   req_start_pos = req_pos;               /* Start of request blocks */
   ret = pf_cmrpc_rm_write_interpret_ind(&p_sess->get_info,
      &write_request, &req_pos, &p_sess->rpc_result);

   /*
    * Prepare the response buffer by writing dummy data and recording their positions.
    * This allows us to insert the actual values later.
    */

   res_status_pos = *p_res_pos;   /* Save for last. */
   pf_put_pnet_status(p_sess->get_info.is_big_endian, &p_sess->rpc_result.pnio_status, res_len, p_res, p_res_pos);

   res_hdr_pos = *p_res_pos;      /* Save for last. */
   /* Insert the response header with dummy length and dummy actual_count. */
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.args_length, res_len, p_res, p_res_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.maximum_count, res_len, p_res, p_res_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.offset, res_len, p_res, p_res_pos);
   pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.actual_count, res_len, p_res, p_res_pos);

   /* Limit our response to what the client can handle. */
   if (req_pos + p_sess->ndr_data.args_maximum < res_size)
   {
      res_len = req_pos + p_sess->ndr_data.args_maximum;
   }

   write_result.sequence_number = write_request.sequence_number;
   write_result.ar_uuid = write_request.ar_uuid;
   write_result.api = write_request.api;
   write_result.slot_number = write_request.slot_number;
   write_result.subslot_number = write_request.subslot_number;
   write_result.index = write_request.index;
   write_result.record_data_length = 0;
   write_result.pnio_status = p_sess->rpc_result.pnio_status;

   res_start_pos = *p_res_pos;   /* Save for last. */
   pf_put_write_result(p_sess->get_info.is_big_endian, &write_result, res_size, p_res, p_res_pos);

   if (p_sess->get_info.result == PF_PARSE_OK)
   {
      /*
       * get request header
       * if (index == PF_IDX_AR_WRITE_MULTIPLE)
       *    while (more data available)
       *       get request header
       *       perform write
       *       write record_data_write
       * else
       *    perform write
       *    write record_data_write
       */
      if (ret == 0)
      {
         if (write_request.index == PF_IDX_AR_WRITE_MULTIPLE)     /* Handle multi-write */
         {
            memset(&write_result_multi, 0, sizeof(write_result_multi));
            while (((req_pos + 58) < p_sess->get_info.len) &&      /* 58 == sizeof((PACKED)write_request)+2 */
                   (write_result_multi.pnio_status.error_code == 0) &&
                   (pf_cmrpc_rm_write_interpret_ind(&p_sess->get_info,
                    &write_request_multi, &req_pos, &write_stat_multi) == 0))
            {
               ret = pf_cmrpc_perform_one_write(net, &p_sess->get_info, &write_request_multi,
                  &write_result_multi, &write_stat_multi, &req_pos);
               pf_put_write_result(p_sess->get_info.is_big_endian, &write_result_multi, res_len, p_res, p_res_pos);

               /* Align on 32-bits to point to next write request */
               while ((req_pos % sizeof(uint32_t)) != 0)
               {
                  req_pos++;
               }
               /* Prepare for next bloc */
               memset(&write_stat_multi, 0, sizeof(write_stat_multi));
            }
            if (req_pos != p_sess->get_info.len)
            {
               ret = -1;
            }
         }
         else     /* single write */
         {
            ret = pf_cmrpc_perform_one_write(net, &p_sess->get_info, &write_request,
               &write_result, &p_sess->rpc_result, &req_pos);
            pf_put_write_result(p_sess->get_info.is_big_endian, &write_result, res_len, p_res, p_res_pos);
         }
      }

      if (ret == 0)
      {
         if (p_sess->get_info.result == PF_PARSE_OK)
         {
            if ((uint32_t)(req_pos - req_start_pos) != p_sess->ndr_data.args_length)
            {
               LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): args_length %u != request length %u\n", __LINE__,
                  (unsigned)p_sess->ndr_data.args_length, (unsigned)(req_pos - req_start_pos));
               pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_WRITE, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
               ret = -1;
            }
         }
         else
         {
            LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): parse result = %u\n", __LINE__, p_sess->get_info.result);
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_WRITE, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
            ret = -1;
         }
      }

      /* Fixup the header with correct length info. */
      p_sess->ndr_data.args_length = *p_res_pos - res_start_pos;
      p_sess->ndr_data.array.actual_count = *p_res_pos - res_start_pos;

      /* Over-write the response header with correct length and actual_count. */
      pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.args_length, res_len, p_res, &res_hdr_pos);
      pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.maximum_count, res_len, p_res, &res_hdr_pos);
      pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.offset, res_len, p_res, &res_hdr_pos);
      pf_put_uint32(p_sess->get_info.is_big_endian, p_sess->ndr_data.array.actual_count, res_len, p_res, &res_hdr_pos);

      /* Insert the actual result of the write operation into the first result block */
      pf_put_pnet_status(p_sess->get_info.is_big_endian, &p_sess->rpc_result.pnio_status, res_len, p_res, &res_status_pos);
   }

   return ret;
}

/**
 * @internal
 * Take a DCE RPC request and create a DCE RPC response.
 * @param net              InOut: The p-net stack instance
 * @param p_sess           In:   The session instance.
 * @param req_pos          In:   Position in the input buffer.
 * @param p_rpc            In:   The RPC header.
 * @param res_size         In:   The size of the response buffer.
 * @param p_res            Out:  The response buffer.
 * @param p_res_pos        InOut:Position in the response buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rpc_request(
   pnet_t                  *net,
   pf_session_info_t       *p_sess,
   uint16_t                req_pos,
   pf_rpc_header_t         *p_rpc,
   uint16_t                res_size,
   uint8_t                 *p_res,
   uint16_t                *p_res_pos)
{
   int                     ret = -1;

   switch (p_rpc->opnum)
   {
   case PF_RPC_DEV_OPNUM_CONNECT:
      LOG_INFO(PF_RPC_LOG, "CONNECT\n");
      ret = pf_cmrpc_rm_connect_ind(net, p_sess, req_pos, res_size, p_res, p_res_pos);
      break;
   case PF_RPC_DEV_OPNUM_RELEASE:
      LOG_INFO(PF_RPC_LOG, "RELEASE\n");
      ret = pf_cmrpc_rm_release_ind(net, p_sess, req_pos, res_size, p_res, p_res_pos);
      break;
   case PF_RPC_DEV_OPNUM_READ:
      LOG_INFO(PF_RPC_LOG, "READ\n");
      ret = pf_cmrpc_rm_read_ind(net, p_sess, p_rpc->opnum, req_pos, res_size, p_res, p_res_pos);
      break;
   case PF_RPC_DEV_OPNUM_WRITE:
      LOG_INFO(PF_RPC_LOG, "WRITE\n");
      ret = pf_cmrpc_rm_write_ind(net, p_sess, req_pos, res_size, p_res, p_res_pos);
      break;
   case PF_RPC_DEV_OPNUM_CONTROL:
      LOG_INFO(PF_RPC_LOG, "CONTROL\n");
      ret = pf_cmrpc_rm_dcontrol_ind(net, p_sess, p_rpc, req_pos, res_size, p_res, p_res_pos);
      break;
   case PF_RPC_DEV_OPNUM_READ_IMPLICIT:
      LOG_INFO(PF_RPC_LOG, "READ_IMPLICIT\n");
      ret = pf_cmrpc_rm_read_ind(net, p_sess, p_rpc->opnum, req_pos, res_size, p_res, p_res_pos);
      break;
   default:
      LOG_ERROR(PNET_LOG, "CMRPC(%d): : Unknown opnum %" PRIu16 "\n", __LINE__, p_rpc->opnum);
      break;
   }

   return ret;
}

int pf_cmrpc_rm_ccontrol_req(
   pnet_t                  *net,
   pf_ar_t                 *p_ar)
{
   int                     ret = -1;
   pf_rpc_header_t         rpc_req;
   pf_ndr_data_t           ndr_data;
   pf_control_block_t      control_io;
   uint16_t                pos = 0;
   uint16_t                start_pos = 0;
   uint16_t                control_pos = 0;
   uint16_t                hdr_pos = 0;
   uint16_t                length_of_body_pos = 0;
   pnet_cfg_t              *p_cfg = NULL;
   pf_session_info_t       *p_sess = NULL;

   memset(&rpc_req, 0, sizeof(rpc_req));
   memset(&ndr_data, 0, sizeof(ndr_data));
   memset(&control_io, 0, sizeof(control_io));

   pf_fspm_get_cfg(net, &p_cfg);
   if (pf_session_allocate(net, &p_sess) != 0)
   {
      LOG_ERROR(PF_RPC_LOG, "RPC(%d): Out of session reaources\n", __LINE__);
   }
   else
   {
      p_sess->p_ar = p_ar;
      p_sess->ip_addr = p_ar->p_sess->ip_addr;
      p_sess->port = OS_PF_RPC_SERVER_PORT;

      memset(&rpc_req, 0, sizeof(rpc_req));
      memset(&ndr_data, 0, sizeof(ndr_data));
      memset(&control_io, 0, sizeof(control_io));

      ndr_data.args_maximum = 1500;
      ndr_data.args_length = 0;             /* To be filled later */
      ndr_data.array.maximum_count = 1500;
      ndr_data.array.offset = 0;
      ndr_data.array.actual_count = 0;      /* To be filled later */

      rpc_req.version = 4;
      rpc_req.packet_type = PF_RPC_PT_REQUEST;
      rpc_req.flags.idempotent = true;
      rpc_req.is_big_endian = false;
      rpc_req.serial_high = 0;


      rpc_req.object_uuid = p_ar->ar_param.cm_initiator_object_uuid;

      /* Controller RPC id */
      rpc_req.interface_uuid.data1 = 0xdea00002;
      rpc_req.interface_uuid.data2 = 0x6c97;
      rpc_req.interface_uuid.data3 = 0x11d1;
      rpc_req.interface_uuid.data4[0] = 0x82;
      rpc_req.interface_uuid.data4[1] = 0x71;
      rpc_req.interface_uuid.data4[2] = 0x00;
      rpc_req.interface_uuid.data4[3] = 0xa0;
      rpc_req.interface_uuid.data4[4] = 0x24;
      rpc_req.interface_uuid.data4[5] = 0x42;
      rpc_req.interface_uuid.data4[6] = 0xdf;
      rpc_req.interface_uuid.data4[7] = 0x7d;

      rpc_req.activity_uuid = p_sess->activity_uuid;

      rpc_req.interface_version = 0x00000001;   /* 1.0 */
      rpc_req.sequence_nmb = p_sess->sequence_nmb_send++;
      rpc_req.opnum = PF_RPC_DEV_OPNUM_CONTROL;
      rpc_req.interface_hint = 0xffff;
      rpc_req.activity_hint = 0xffff;
      rpc_req.length_of_body = 0;         /* To be filled later */
      rpc_req.serial_low = 0;

      control_io.ar_uuid = p_ar->ar_param.ar_uuid;
      control_io.session_key = p_ar->ar_param.session_key;
      control_io.control_command = BIT(PF_CONTROL_COMMAND_BIT_APP_RDY);
      control_io.alarm_sequence_number = 0;  /* Reserved */
      control_io.control_block_properties = 0;

      memset(p_sess->buffer, 0, sizeof(p_sess->buffer));
      pos = 0;

      hdr_pos = 0;
      length_of_body_pos = 0;

      pf_put_dce_rpc_header(&rpc_req, sizeof(p_sess->buffer), p_sess->buffer, &pos, &length_of_body_pos);

      start_pos = pos;

      pf_put_uint32(rpc_req.is_big_endian, ndr_data.args_maximum, sizeof(p_sess->buffer), p_sess->buffer, &pos);
      hdr_pos = pos;
      pf_put_uint32(rpc_req.is_big_endian, ndr_data.args_length, sizeof(p_sess->buffer), p_sess->buffer, &pos);
      pf_put_uint32(rpc_req.is_big_endian, ndr_data.array.maximum_count, sizeof(p_sess->buffer), p_sess->buffer, &pos);
      pf_put_uint32(rpc_req.is_big_endian, ndr_data.array.offset, sizeof(p_sess->buffer), p_sess->buffer, &pos);
      pf_put_uint32(rpc_req.is_big_endian, ndr_data.array.actual_count, sizeof(p_sess->buffer), p_sess->buffer, &pos);

      /* Always little-endian on the wire */
      control_pos = pos;
      pf_put_control(true, PF_BT_APPRDY_REQ, &control_io, sizeof(p_sess->buffer), p_sess->buffer, &pos);

      pf_put_ar_diff(rpc_req.is_big_endian, p_ar, sizeof(p_sess->buffer), p_sess->buffer, &pos);

      /* Finalize */
      /* Insert the real value of length_of_body in the rpc header */
      pf_put_uint16(rpc_req.is_big_endian, pos - start_pos, sizeof(p_sess->buffer), p_sess->buffer, &length_of_body_pos);

      /* Fixup the header with correct length info. */
      ndr_data.args_length = pos - control_pos;
      ndr_data.array.actual_count = pos - control_pos;

      /* Over-write the response header with correct length and actual_count. */
      pf_put_uint32(rpc_req.is_big_endian, ndr_data.args_length, sizeof(p_sess->buffer), p_sess->buffer, &hdr_pos);
      pf_put_uint32(rpc_req.is_big_endian, ndr_data.array.maximum_count, sizeof(p_sess->buffer), p_sess->buffer, &hdr_pos);
      pf_put_uint32(rpc_req.is_big_endian, ndr_data.array.offset, sizeof(p_sess->buffer), p_sess->buffer, &hdr_pos);
      pf_put_uint32(rpc_req.is_big_endian, ndr_data.array.actual_count, sizeof(p_sess->buffer), p_sess->buffer, &hdr_pos);

      p_sess->socket = os_udp_socket();
      if (p_sess->socket > 0)
      {
         if (os_udp_sendto(p_sess->socket, p_sess->ip_addr, p_sess->port, p_sess->buffer, pos) == pos)
         {
            LOG_INFO(PF_RPC_LOG, "os_udp_sendto success!!\n");
            ret = 0;
         }
         else
         {
            LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): os_udp_sendto failed\n", __LINE__);
         }
      }
      else
      {
         LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): os_udp_socket failed: %d\n", __LINE__, (int)p_sess->socket);
      }
   }

   return ret;
}

/**
 * @internal
 * Parse all blocks in a CControl RPC response message.
 * @param p_sess           In:   The session instance.
 * @param req_pos          In:   The position in the input buffer.
 * @param p_control_io     In:   The CControl control block.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rm_ccontrol_interpret_cnf(
   pf_session_info_t       *p_sess,
   uint16_t                req_pos,
   pf_control_block_t      *p_control_io)
{
   int                     ret = 0;
   uint16_t                start_pos;
   pf_block_header_t       block_header;

   start_pos = req_pos;
   while (((req_pos + sizeof(block_header)) <= p_sess->get_info.len) &&
          (p_sess->get_info.result == PF_PARSE_OK))
   {
      pf_get_block_header(&p_sess->get_info, &req_pos, &block_header);
      if ((((req_pos + block_header.block_length + 4) - sizeof(block_header)) <= p_sess->get_info.len) &&
          (p_sess->get_info.result == PF_PARSE_OK))
      {
         switch (block_header.block_type)
         {
         case PF_BT_APPRDY_RES:
            pf_get_control(&p_sess->get_info, &req_pos, p_control_io);
            break;
         default:
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONTROL, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS);
            LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): Unknown block type %u\n", __LINE__, (unsigned)block_header.block_type);
            LOG_DEBUG(PNET_LOG, "CMRPC(%d): : Unknown block type %u\n", __LINE__, (unsigned)block_header.block_type);
            ret = -1;
            break;
         }
      }
      else
      {
         LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): : Parse error\n", __LINE__);
      }
   }

   if (ret == 0)
   {
      if (p_sess->get_info.result == PF_PARSE_OK)
      {
         /* Error discovery. */
         if (req_pos != p_sess->get_info.len)
         {
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONTROL, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
            ret = -1;
         }
         else if ((uint32_t)(req_pos - start_pos) != p_sess->ndr_data.args_length)
         {
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): args_length %" PRIu32 " != request length %" PRIu32 "\n", __LINE__,
               p_sess->ndr_data.args_length, (uint32_t)(req_pos - start_pos));
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONTROL, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
            ret = -1;
         }
         else if (p_control_io->control_command != BIT(PF_CONTROL_COMMAND_BIT_DONE))
         {
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): ControlCommand has more than one bit set: %04x\n", __LINE__, p_control_io->control_command);
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONTROL, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT);
            ret = -1;
         }
      }
      else
      {
         pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONTROL, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT);
         ret = -1;
      }
   }

   return ret;
}

/**
 * @internal
 * Handle the response from a CControl request
 * @param net              InOut: The p-net stack instance
 * @param p_sess           In:   The session instance.
 * @param req_pos          In:   Position in the input buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rm_ccontrol_cnf(
   pnet_t                  *net,
   pf_session_info_t       *p_sess,
   uint16_t                req_pos)
{
   int                     ret = -1;
   pf_control_block_t      ccontrol_io;
   pf_ar_t                 *p_ar = p_sess->p_ar;

   memset(&ccontrol_io, 0, sizeof(ccontrol_io));

   if (pf_cmrpc_rm_ccontrol_interpret_cnf(p_sess, req_pos, &ccontrol_io) == 0)
   {
      /*
       * The only expected response here is "done".
       */
      if (ccontrol_io.control_command == BIT(PF_CONTROL_COMMAND_BIT_DONE))
      {
         /* Send the result to CMDEV */
         if (pf_cmdev_rm_ccontrol_cnf(net, p_ar, &ccontrol_io, &p_sess->rpc_result) == 0)
         {
            /* Only if result is OK */
            if (pf_cmpbe_rm_ccontrol_cnf(p_ar, &ccontrol_io, &p_sess->rpc_result) == 0)
            {
               pf_session_release(p_sess);
               ret = 0;
            }
            else
            {
               LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): \n", __LINE__);
            }
         }
         else
         {
            LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): \n", __LINE__);
         }
      }
      else
      {
         LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): \n", __LINE__);
      }
   }
   else
   {
      LOG_DEBUG(PF_RPC_LOG, "CMRPC(%d): \n", __LINE__);
   }

   return ret;
}


/******************** State machine and DCE RPC protocol *********************/

/**
 * @internal
 * Handle one DCE RPC response type message.
 * @param net              InOut: The p-net stack instance
 * @param p_sess           In:   The session instance.
 * @param req_pos          In:   The response length
 * @param p_rpc            In:   The RPC header.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_rpc_response(
   pnet_t                  *net,
   pf_session_info_t       *p_sess,
   uint16_t                req_pos,
   pf_rpc_header_t         *p_rpc)
{
   int                     ret = -1;

   switch (p_rpc->opnum)
   {
   case PF_RPC_DEV_OPNUM_CONTROL:
      ret = pf_cmrpc_rm_ccontrol_cnf(net, p_sess, req_pos);
      break;
   default:
      LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): Unknown opnum %" PRIu16 "\n", __LINE__, p_rpc->opnum);
      break;
   }

   return ret;
}

/**
 * @internal
 * Handle one DCE RPC message.
 * @param net              InOut: The p-net stack instance
 * @param ip_addr          In:   The IP address of the originator
 * @param port             In:   The port of the originator.
 * @param p_req            In:   The input message buffer.
 * @param req_len          In:   The input message length.
 * @param p_res            Out:  The output buffer.
 * @param p_res_len        In:   The size of the output buffer.
 *                         Out:  The length of the output message.
 * @param p_is_release     Out:  Set to true if operation is a release op.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
static int pf_cmrpc_dce_packet(
   pnet_t                  *net,
   uint32_t                ip_addr,
   uint16_t                port,
   uint8_t                 *p_req,
   uint32_t                req_len,
   uint8_t                 *p_res,
   uint16_t                *p_res_len,
   bool                    *p_is_release)
{
   int                     ret = -1;
   pf_rpc_header_t         rpc_req;
   pf_rpc_header_t         rpc_res;
   uint16_t                start_pos = 0;
   uint16_t                length_of_body_pos = 0;
   uint16_t                req_pos = 0;
   uint16_t                res_pos = 0;
   pf_get_info_t           get_info;
   pf_session_info_t       *p_sess = NULL;

   get_info.result = PF_PARSE_OK;
   get_info.p_buf = p_req;
   get_info.len = req_len;

   /* This function also sets get_info.is_big_endian */
   pf_get_dce_rpc_header(&get_info, &req_pos, &rpc_req);

   /* Find the session or allocate a new one */
   pf_session_locate_by_uuid(net, &rpc_req.activity_uuid, &p_sess);
   if (p_sess == NULL)
   {
      (void)pf_session_allocate(net, &p_sess);
   }

   if (p_sess == NULL)
   {
      /* Unavailable */
      LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): Out of session resources.\n", __LINE__);
   }
   else
   {
      p_sess->get_info = get_info;
      memset(&p_sess->rpc_result, 0, sizeof(p_sess->rpc_result));

      /* Handle RPC fragments: */
      if (rpc_req.flags.fragment == false)
      {
         /* This is the normal path where the request is contained entirely in one frame */
         p_sess->buf_len = 0;
         p_sess->ip_addr = ip_addr;
         p_sess->port = port;

         p_sess->from_me = false;
         p_sess->activity_uuid = rpc_req.activity_uuid;

         p_sess->is_big_endian = p_sess->get_info.is_big_endian;
         p_sess->fragment_nbr = 0;
      }
      else
      {
         /*
          * It is a fragment of a request.
          * Collect all fragments into the session buffer and
          * proceed only when the last fragment has been received.
          */
         if (rpc_req.fragment_nmb == 0)
         {
            /* This is the first fragment. */
            /* Initialize the session */
            p_sess->buf_len = 0;
            p_sess->ip_addr = ip_addr;
            p_sess->port = port;

            p_sess->from_me = false;
            p_sess->activity_uuid = rpc_req.activity_uuid;

            p_sess->is_big_endian = p_sess->get_info.is_big_endian;
            p_sess->fragment_nbr = 0;
         }
         else
         {
            /* All fragments must have same endianness in this implementation */
            if (p_sess->is_big_endian != rpc_req.is_big_endian)
            {
               /*
                * Endianness differs.
                */
               LOG_ERROR(PF_RPC_LOG, "RPC(%d): Endianness differs in fragments\n", __LINE__);
               pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT);
            }
         }

         /* Enter here _even_if_ an error is already detected because we need to generate an error response. */
         if (p_sess->fragment_nbr != rpc_req.fragment_nmb)
         {
            LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): Unexpected fragment number %u != %u (exp)\n",
               __LINE__, (unsigned)rpc_req.fragment_nmb, (unsigned)p_sess->fragment_nbr + 1);
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT);
         }
         else  if ((p_sess->buf_len + rpc_req.length_of_body) > sizeof(p_sess->buffer))
         {
            LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): Fragments exceed max buffer\n", __LINE__);
            pf_set_error(&p_sess->rpc_result, PNET_ERROR_CODE_CONNECT, PNET_ERROR_DECODE_PNIO, PNET_ERROR_CODE_1_CMRPC, PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT);
         }
         else
         {
            /* Copy to session buffer */
            memcpy(&p_sess->buffer[p_sess->buf_len], &p_req[req_pos], rpc_req.length_of_body);
            p_sess->buf_len += rpc_req.length_of_body;
            p_sess->fragment_nbr++;

            if (rpc_req.flags.last_fragment == true)
            {
               /* Re-route the parser to use the session buffer */
               req_pos = 0;
               p_sess->get_info.p_buf = p_sess->buffer;
               p_sess->get_info.len = p_sess->buf_len;
            }
         }
      }

      /* Enter here _even_if_ an error is already detected because may we need to generate an error response. */
      if ((rpc_req.flags.fragment == false) ||
          (rpc_req.flags.last_fragment == true))
      {
         pf_get_ndr_data(&p_sess->get_info, &req_pos, &p_sess->ndr_data);
         /* From now on all is big-endian */
         p_sess->get_info.is_big_endian = true;

         switch (rpc_req.packet_type)
         {
         case PF_RPC_PT_REQUEST:
            /* Prepare the response */
            rpc_res = rpc_req;
            rpc_res.packet_type = PF_RPC_PT_RESPONSE;
            rpc_res.flags.last_fragment = false;
            rpc_res.flags.fragment = false;
            rpc_res.flags.no_fack = true;
            rpc_res.flags.maybe = false;
            rpc_res.flags.idempotent = true;
            rpc_res.flags.broadcast = false;
            rpc_res.flags2.cancel_pending = false;
            rpc_res.fragment_nmb = 0;
            rpc_res.is_big_endian = p_sess->get_info.is_big_endian;

            /* Insert the response header to get pos of rpc response body. */
            pf_put_dce_rpc_header(&rpc_res, *p_res_len, p_res, &res_pos, &length_of_body_pos);
            start_pos = res_pos;                /* Save for later */

            if (rpc_req.opnum == PF_RPC_DEV_OPNUM_RELEASE)
            {
               p_sess->release_in_progress = true; /* Tell everybody */
               *p_is_release = true;               /* Tell caller */
            }
            ret = pf_cmrpc_rpc_request(net, p_sess, req_pos, &rpc_req, *p_res_len, p_res, &res_pos);
            if (rpc_req.opnum == PF_RPC_DEV_OPNUM_RELEASE)
            {
               pf_session_release(p_sess);
            }

            /*
             * FROM HERE ON:
             * Do not use p_sess after this line.
             * The RPC may have been a release. In that case the session does no longer exist.
             */

            /* Insert the real value of length_of_body in the rpc header */
            pf_put_uint16(rpc_res.is_big_endian, (uint16_t)(res_pos - start_pos), *p_res_len, p_res, &length_of_body_pos);
            break;
         case PF_RPC_PT_RESPONSE:
            ret = pf_cmrpc_rpc_response(net, p_sess, req_pos, &rpc_req);
            break;
         default:
            LOG_ERROR(PF_RPC_LOG, "CMRPC(%d): Unknown packet_type %" PRIu8 "\n", __LINE__, rpc_req.packet_type);
            break;
         }
      }
      else
      {
         if ((rpc_req.flags.fragment == true) &&
             (rpc_req.flags.no_fack == false))
         {
            /* Create Fragment ACK */
            /* Send ACK */
            rpc_res = rpc_req;
            rpc_res.packet_type = PF_RPC_PT_FRAG_ACK;
            rpc_res.flags.last_fragment = false;
            rpc_res.flags.fragment = false;
            rpc_res.flags.no_fack = false;
            rpc_res.flags.maybe = false;
            rpc_res.flags.idempotent = true;
            rpc_res.flags.broadcast = false;
            rpc_res.flags2.cancel_pending = false;
            rpc_res.length_of_body = 0;
            rpc_res.is_big_endian = p_sess->is_big_endian;

            pf_put_dce_rpc_header(&rpc_res, *p_res_len, p_res, &res_pos, &length_of_body_pos);
         }
         else
         {
            /* Waiting for the next fragment. */
         }

         ret = 0;
      }
   }

   /* Size of result data */
   *p_res_len = res_pos;

   return ret;
}

void pf_cmrpc_periodic(
   pnet_t                  *net)
{
   uint32_t                dcerpc_addr;
   uint16_t                dcerpc_port;
   int                     dcerpc_req_len;
   uint16_t                dcerpc_resp_len = 0;
   uint16_t                ix;
   bool                    is_release = false;

   /* Poll for RPC session confirmations */
   for (ix = 0; ix < NELEMENTS(net->cmrpc_session_info); ix++)
   {
      if ((net->cmrpc_session_info[ix].in_use == true) && (net->cmrpc_session_info[ix].from_me == true))
      {
         dcerpc_req_len = os_udp_recvfrom(net->cmrpc_session_info[ix].socket, &dcerpc_addr, &dcerpc_port, net->cmrpc_dcerpc_req_frame, sizeof(net->cmrpc_dcerpc_req_frame));
         if (dcerpc_req_len > 0)
         {
            dcerpc_resp_len = sizeof(net->cmrpc_dcerpc_rsp_frame);
            (void)pf_cmrpc_dce_packet(net, dcerpc_addr, dcerpc_port, net->cmrpc_dcerpc_req_frame, dcerpc_req_len, net->cmrpc_dcerpc_rsp_frame, &dcerpc_resp_len, &is_release);
         }
      }
   }

   /* Poll RPC requests */
   dcerpc_req_len = os_udp_recvfrom(net->cmrpc_rpcreq_socket, &dcerpc_addr, &dcerpc_port, net->cmrpc_dcerpc_req_frame, sizeof(net->cmrpc_dcerpc_req_frame));
   if (dcerpc_req_len > 0)
   {
      dcerpc_resp_len = sizeof(net->cmrpc_dcerpc_rsp_frame);
      (void)pf_cmrpc_dce_packet(net, dcerpc_addr, dcerpc_port, net->cmrpc_dcerpc_req_frame, dcerpc_req_len, net->cmrpc_dcerpc_rsp_frame, &dcerpc_resp_len, &is_release);
      if (dcerpc_resp_len != 0)
      {
         os_udp_sendto(net->cmrpc_rpcreq_socket, dcerpc_addr, dcerpc_port, net->cmrpc_dcerpc_rsp_frame, dcerpc_resp_len);
      }
      else
      {
         LOG_INFO(PF_RPC_LOG, "CMRPC(%d): Nothing to send!\n", __LINE__);
      }
      if (is_release == true)
      {
         os_udp_close(net->cmrpc_rpcreq_socket);
         net->cmrpc_rpcreq_socket = os_udp_open(OS_IPADDR_ANY, OS_PF_RPC_SERVER_PORT);
      }
   }
}

void pf_cmrpc_init(
   pnet_t                  *net)
{
   if (net->p_cmrpc_rpc_mutex == NULL)
   {
      net->p_cmrpc_rpc_mutex = os_mutex_create();
      memset(net->cmrpc_ar, 0, sizeof(net->cmrpc_ar));
      memset(net->cmrpc_session_info, 0, sizeof(net->cmrpc_session_info));

      net->cmrpc_rpcreq_socket = os_udp_open(OS_IPADDR_ANY, OS_PF_RPC_SERVER_PORT);
   }

   /* Save for later (put it into each session */
   net->cmrpc_session_number = 0x12345678;     /* Starting number */
}


/* Not used */
void pf_cmrpc_exit(
   pnet_t                  *net)
{
   if (net->p_cmrpc_rpc_mutex != NULL)
   {
      os_mutex_destroy(net->p_cmrpc_rpc_mutex);
      memset(net->cmrpc_ar, 0, sizeof(net->cmrpc_ar));
      memset(net->cmrpc_session_info, 0, sizeof(net->cmrpc_session_info));
   }
}

/*********************** Local primitives ************************************/

int pf_cmrpc_cmdev_state_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_event_values_t     event)
{
   int                     res = -1;
   pf_session_info_t       *p_sess = NULL;

   switch (event)
   {
   case PNET_EVENT_ABORT:
      /* Close all RPC session (CCONTROL) sockets */
      while (pf_session_locate_by_ar(net, p_ar, &p_sess) == 0)
      {
         os_udp_close(p_sess->socket);
         pf_session_release(p_sess);
      }

      if (p_ar != NULL)    /* CheckAREP */
      {
         if (p_ar->p_sess != NULL)
         {
            if (p_ar->p_sess->release_in_progress == false)
            {
               pf_session_release(p_ar->p_sess);

               /* Re-open the global RPC socket. */
               os_udp_close(net->cmrpc_rpcreq_socket);
               net->cmrpc_rpcreq_socket = os_udp_open(OS_IPADDR_ANY, OS_PF_RPC_SERVER_PORT);
            }
         }
         else
         {
            LOG_ERROR(PF_RPC_LOG, "RPC(%d): Session is NULL\n", __LINE__);
         }
         pf_ar_release(p_ar);
      }

      res = 0;
      break;
   default:
      break;
   }

   return res;
}
