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

#include "pf_includes.h"
#include "pf_lan9662_mera.h"

#include <string.h>
#include <inttypes.h>

/**
 * @file
 * @brief PPM driver for LAN9662
 *
 */

int pf_ppm_drv_lan9662_create (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   pf_mera_frame_cfg_t mera_ppm_cfg = {0};

   LOG_DEBUG (
      PF_PPM_LOG,
      "PPM_DRV_MERA(%d): Allocate RTE frame (inbound data / tx frames) for "
      "AREP %u "
      "CREP "
      "%" PRIu32 "\n",
      __LINE__,
      p_ar->arep,
      crep);

   mera_ppm_cfg.p_iocr = &p_ar->iocrs[crep];

   pf_mera_get_port_from_remote_mac (
      &p_ar->ar_param.cm_initiator_mac_add,
      &mera_ppm_cfg.port);

   memcpy (
      &mera_ppm_cfg.main_mac_addr,
      &net->pf_interface.main_port.mac_address,
      sizeof (pnet_ethaddr_t));

   p_ar->iocrs[crep].ppm.frame = pf_mera_ppm_alloc (net, &mera_ppm_cfg);

   if (p_ar->iocrs[crep].ppm.frame == NULL)
   {
      LOG_ERROR (
         PF_PPM_LOG,
         "PPM_DRV_MERA(%d): Frame allocation failed\n",
         __LINE__);
      return -1;
   }
   return 0;
}

int pf_ppm_drv_lan9662_activate_req (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   LOG_DEBUG (
      PF_PPM_LOG,
      "PPM_DRV_MERA(%d): Activate RTE (inbound data / tx frames) for AREP %u "
      "CREP "
      "%" PRIu32 "\n",
      __LINE__,
      p_ar->arep,
      crep);

   if (p_ar->iocrs[crep].ppm.frame == NULL)
   {
      LOG_ERROR (
         PF_PPM_LOG,
         "PPM_DRV_MERA(%d): No mera frame allocated\n",
         __LINE__);
      return -1;
   }

   return pf_mera_ppm_start (net, p_ar->iocrs[crep].ppm.frame);
}

/**
 * @internal
 * Close RTE resources used in PPM frame.
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    PPM frame
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 *                                stored tasks.
 */
static void pf_ppm_drv_lan9662_delayed_close (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{
   pf_drv_frame_t * frame = (pf_drv_frame_t *)arg;
   pf_mera_ppm_stop (net, frame);
   pf_mera_ppm_free (net, frame);
}

int pf_ppm_drv_lan9662_close_req (pnet_t * net, pf_ar_t * p_ar, uint32_t crep)
{
   LOG_DEBUG (
      PF_PPM_LOG,
      "PPM(%d): Stop RTE for AREP %u CREP "
      "%" PRIu32 "\n",
      __LINE__,
      p_ar->arep,
      crep);

   /* Delay close of pf_mera resources
    * Workaround for delayed alarm transmission.
    * Close RTE resources after alarm is sent.
    */
   pf_scheduler_init_handle (&p_ar->iocrs[crep].ppm.ci_timeout, "ppm_close");
   pf_scheduler_add (
      net,
      2,
      pf_ppm_drv_lan9662_delayed_close,
      p_ar->iocrs[crep].ppm.frame,
      &p_ar->iocrs[crep].ppm.ci_timeout);

   return 0;
}

/**
 * Write the ppm frame buffer
 * @param iocr    InOut: Communication relation
 * @param offset  In: Buffer offset to write
 * @param data    In: Data source
 * @param len     In: Data length
 * @return 0 on success, -1 on error
 */
static int pf_ppm_drv_lan9662_write_frame_buffer (
   pf_iocr_t * iocr,
   uint32_t offset,
   const uint8_t * data,
   uint16_t len)
{
   memcpy (&iocr->ppm.buffer_data[offset], data, len);
   return 0;
}
/**
 * Read the ppm frame buffer
 * @param iocr    InOut: Communication relation
 * @param offset  In: Buffer offset to read
 * @param data    In: Data destination
 * @param len     In: Data length
 * @return 0 on success, -1 on error
 */
static int pf_ppm_drv_lan9662_read_frame_buffer (
   pf_iocr_t * iocr,
   uint32_t offset,
   uint8_t * data,
   uint16_t len)
{
   memcpy (data, &iocr->ppm.buffer_data[offset], len);
   return 0;
}

/**
 * Update the PPM frame buffer and write
 * data and iops to the active RTE frame.
 */
static int pf_ppm_drv_lan9662_write_data_and_iops (
   pnet_t * net,
   pf_iocr_t * iocr,
   const pf_iodata_object_t * p_iodata,
   const uint8_t * data,
   uint16_t len,
   const uint8_t * iops,
   uint8_t iops_len)
{
   int ret = 0;

   if (data != NULL)
   {
      pf_ppm_drv_lan9662_write_frame_buffer (
         iocr,
         p_iodata->data_offset,
         data,
         len);
   }
   pf_ppm_drv_lan9662_write_frame_buffer (
      iocr,
      p_iodata->iops_offset,
      iops,
      iops_len);

   if (pf_mera_is_active_frame (iocr->ppm.frame))
   {
      /* Only initiate write to RTE if frame is active.
       * Initialization values are still written to
       * frame buffer and are used as initial SRAM values.
       */

      if (data != NULL)
      {
         ret =
            pf_mera_ppm_write_input (net, iocr->ppm.frame, p_iodata, data, len);
      }

      if (ret == 0)
      {
         ret = pf_mera_ppm_write_iops (
            net,
            iocr->ppm.frame,
            p_iodata,
            iops,
            iops_len);
      }
   }

   return ret;
}

/**
 * Read data and iops from SRAM
 */
static int pf_ppm_drv_lan9662_read_data_and_iops (
   pnet_t * net,
   pf_iocr_t * iocr,
   const pf_iodata_object_t * p_iodata,
   uint8_t * data,
   uint16_t data_len,
   uint8_t * iops,
   uint8_t iops_len)
{
   return pf_mera_ppm_read_data_and_iops (
      net,
      iocr->ppm.frame,
      p_iodata,
      data,
      data_len,
      iops,
      iops_len);
}

/**
 * Write consumer status for a subslot.
 * Write the PPM frame buffer and the active RTE frame.
 */
static int pf_ppm_drv_lan9662_write_iocs (
   pnet_t * net,
   pf_iocr_t * iocr,
   const pf_iodata_object_t * p_iodata,
   const uint8_t * iocs,
   uint8_t iocs_len)
{
   return pf_ppm_drv_lan9662_write_frame_buffer (
      iocr,
      p_iodata->iocs_offset,
      iocs,
      iocs_len);
}

/**
 * Read consumer status previously written
 * using pf_ppm_drv_lan9662_write_iocs().
 * Value is fetched from the PPM frame buffer,
 * not from the RTE.
 */
static int pf_ppm_drv_lan9662_read_iocs (
   pnet_t * net,
   pf_iocr_t * iocr,
   const pf_iodata_object_t * p_iodata,
   uint8_t * iocs,
   uint8_t iocs_len)
{

   return pf_ppm_drv_lan9662_read_frame_buffer (
      iocr,
      p_iodata->iocs_offset,
      iocs,
      iocs_len);
}

static int pf_ppm_drv_lan9662_write_data_status (
   pnet_t * net,
   pf_iocr_t * iocr,
   uint8_t data_status)
{
   pf_ppm_drv_lan9662_write_frame_buffer (
      iocr,
      iocr->ppm.data_status_offset,
      &data_status,
      sizeof (data_status));

   return pf_mera_ppm_write_data_status (net, iocr->ppm.frame, data_status);
}

/**
 * Show the current state of the driver
 * @param p_ppm   In: PPM instance
 */
void pf_ppm_drv_lan9662_show (const pf_ppm_t * p_ppm)
{
   printf ("Not implemented:  pf_ppm_drv_lan9662_show()\n");
}

int pf_driver_ppm_init (pnet_t * net)
{
   static const pf_ppm_driver_t drv = {
      .create = pf_ppm_drv_lan9662_create,
      .activate_req = pf_ppm_drv_lan9662_activate_req,
      .close_req = pf_ppm_drv_lan9662_close_req,
      .write_data_and_iops = pf_ppm_drv_lan9662_write_data_and_iops,
      .read_data_and_iops = pf_ppm_drv_lan9662_read_data_and_iops,
      .write_iocs = pf_ppm_drv_lan9662_write_iocs,
      .read_iocs = pf_ppm_drv_lan9662_read_iocs,
      .write_data_status = pf_ppm_drv_lan9662_write_data_status,
      .show = pf_ppm_drv_lan9662_show};

   net->ppm_drv = &drv;

   LOG_INFO (
      PF_PPM_LOG,
      "PPM_DRV(%d): LAN9662 PPM driver installed\n",
      __LINE__);
   return 0;
}
