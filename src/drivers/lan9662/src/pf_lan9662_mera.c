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

#include "microchip/ethernet/rte/api.h"

#include "pf_includes.h"
#include "pnet_driver_options.h"
#include "pnet_lan9662_api.h"
#include "pf_lan9662_mera.h"
#include "pf_rte_uio.h"
#include "pf_sram_uio.h"
#include "pf_mera_trace.h"
#include "pnal_filetools.h"

#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

#define PF_MERA_GROUP_ID_NONE 0
#define PF_MERA_GROUP_ID_PPM  1
#define PF_MERA_GROUP_ID_CPM  2

#define PF_MERA_INVALID_ID            0xFFFF
#define PF_MERA_DATA_STATUS_MASK      0x35
#define PF_MERA_SCRIPT_ARG_LEN        24
#define PF_MERA_RTE_INFO_PERIOD_IN_US 30000000
#define PF_MERA_POLL_INTERVAL_IN_US   1000000
#define PF_MERA_PPM_IODATA_OFFSET     6   /* frame ID + vlan ID */
#define PF_MERA_CPM_IODATA_OFFSET     2   /* frame ID + vlan ID */
#define PF_MERA_RAL_TIME_OFFSET       100 /* Time in ns */

/* Default behaviour is that pf_mera assigns SRAM address for iodata/subslot
 * Enabling this define allows the application to set the SRAM address using
 * rte configuration functions in P-Net API.
 */
#define PF_MERA_ALLOW_APPLICATION_SRAM 0

/* Log enable / disable of features shared between PPM and CPM side */
#define PF_MERA_LOG (PF_PPM_LOG | PF_CPM_LOG)

typedef struct pf_mera_rte_cfg
{
   bool valid;
   pnet_mera_rte_data_cfg_t cfg;
} pf_mera_rte_cfg_t;

/**
 * RTE mapping of an iodata object.
 * Same type is used for both inbound and outbound
 * data.
 */
typedef struct pf_rte_object
{
   bool in_use;
   pf_iodata_object_t * iodata;
   mera_io_intf_t io_interface_type;
   uint16_t sram_address;
   uint16_t qspi_address;
   const uint8_t * default_data;
   mera_ob_dg_id_t ob_dg_id;
   mera_ib_ra_id_t ib_ra_id;
   mera_ib_ral_id_t ib_ral_id;
   mera_buf_t ib_ral_buf;
} pf_rte_object_t;

/**
 * RTE configuration for a complete frame.
 * The frame can be either a PPM or a CPM frame.
 * Includes information of the RTE configuration as
 * well as an array with the configuration of each
 * iodata object / subslot and their RTE configurations.
 */
typedef struct pf_mera_frame
{
   pf_drv_frame_t drv_frame;
   bool used;   /* true when allocated */
   bool active; /* true when RTE is started */
   bool is_ppm; /* true if ppm / inbound RTE frame */
   bool is_cpm; /* true if cpm / outbound RTE frame */

   /* Configuration passed on allocation */
   pf_mera_frame_cfg_t config;

   /* General RTE configuration */
   uint16_t port;
   uint32_t interval; /* Frame read/write interval [ns] */
   uint16_t vcam_id;
   uint16_t vlan_idx;
   mera_rtp_id_t rtp_id;

   uint16_t sram_frame_address; /* Start address of SRAM frame */
   uint16_t rte_data_offset;    /* RTE io-data offset. Different for inbound and
                                   outbound configurations */

   /* RTE configuration for individual iodata objects */
   pf_rte_object_t rte_object[PNET_MAX_SLOTS][PNET_MAX_SUBSLOTS];

   /* Inbound configuration */
   uint16_t ib_ral_id_count;
   uint16_t ib_ra_id_count;
   /* Internal transfer QSPI -> SRAM */
   mera_ob_wal_id_t internal_wal_id;

   /* Outbound configuration */
   mera_ob_wal_id_t ob_wal_id;
   uint16_t ob_wa_count;
   uint16_t ob_dg_count;

   uint64_t rx_count;

   /* A ref to the mera lib should not be part
    * of the frame. But it is added to avoid updating
    * a call chain including the block writer.
    * Todo: Fix by adding net ref in CPM interface function
    * get_data_status()
    */
   struct mera_inst * mera_lib;

} pf_mera_frame_t;

typedef struct pf_mera
{
   pf_drv_t hwo_drv;

   struct mera_inst * mera_lib; /* MERA library ref */
   pf_mera_frame_t frame[PNET_LAN9662_MAX_FRAMES];
   pf_scheduler_handle_t sched_handle; /* Debug info timeout handle */
   pf_scheduler_handle_t poll_handle;  /* Mera lib poll timeout handle */

   /* Submodule RTE configuration set by calls to
    * pnet_mera_input_set_rte_config and pnet_mera_out_set_rte_config.
    * Should be part of frame, but application has to set configuration
    * before cpm frame is created and config is stored until ppm/cpm
    * are activated.
    */
   pf_mera_rte_cfg_t rte_data_cfg[PNET_MAX_SLOTS][PNET_MAX_SUBSLOTS];

   /* RTE resource management */
   struct
   {
      uint16_t base_id;
      bool used[PNET_LAN9662_MAX_IDS];
   } vcam_id;

   struct
   {
      uint16_t base_id;
      bool used[PNET_LAN9662_MAX_IDS];
   } vlan_index;

   struct
   {
      uint16_t base_id;
      bool used[PNET_LAN9662_MAX_IDS];
   } rtp;

   struct
   {
      uint16_t base_id;
      bool used[PNET_LAN9662_MAX_IDS];
   } ral;

   struct
   {
      uint16_t base_id;
      bool used[PNET_LAN9662_MAX_IDS];
   } wal;

} pf_mera_t;

/**
 * Get string representation of RTE interface type
 * @param t             In: RTE interface type
 * @return String representation
 */
static const char * pf_mera_rte_data_type_to_str (pnet_mera_rte_data_type_t t)
{
   switch (t)
   {
   case PNET_MERA_DATA_TYPE_SRAM:
      return "SRAM";
   case PNET_MERA_DATA_TYPE_QSPI:
      return "QSPI";
   default:
      return "UNSUPPORTED";
   }
}

static const char * pf_mera_interface_type_to_str (mera_io_intf_t if_type)
{
   switch (if_type)
   {
   case MERA_IO_INTF_QSPI:
      return "QSPI";
   case MERA_IO_INTF_PI:
      return "PI";
   case MERA_IO_INTF_SRAM:
      return "SRAM";
   case MERA_IO_INTF_PCIE:
      return "PCIE";
   default:
      return "UNKNOWN";
   }
}

/**
 * Get the subslot index
 * The subslots numbers starts at 1 and DAP subslots for
 * identity and port information at 0x8000. This
 * function converts the subslot number to corresponding
 * index in the management data structures.
 *
 * subslot   number      id
 * slot 0
 *   subslot 1           0
 *   subslot 0x8000      1
 *   subslot 0x8001      2
 *   subslot 0x8002      3
 * slot 1
 *   subslot 1           0
 *   subslot 2           1
 * @param subslot       In: (Real) Subslot number
 * @return Subslot index
 */
static uint16_t get_subslot_index (uint16_t subslot)
{
   uint16_t subslot_index;
   if (subslot < PNET_SUBSLOT_DAP_INTERFACE_1_IDENT)
   {
      subslot_index = subslot - 1;
   }
   else
   {
      subslot_index = subslot - PNET_SUBSLOT_DAP_INTERFACE_1_IDENT + 1;
   }

   CC_ASSERT (subslot_index < PNET_MAX_SUBSLOTS);

   return subslot_index;
}

pf_rte_object_t * get_rte_object (
   pf_mera_frame_t * frame,
   uint16_t slot,
   uint16_t subslot)
{
   uint16_t subslot_index = get_subslot_index (subslot);

   CC_ASSERT (frame != NULL);
   CC_ASSERT (subslot < PNET_MAX_SLOTS);

   return &frame->rte_object[slot][subslot_index];
}

/**
 * Callback for mera library debug info
 * Prints debug information using vprintf()
 * @param fmt     In: Format string
 * @param ...     In: argument list
 * @return Always returns 0
 */
static int pf_mera_printf_cb (const char * fmt, ...)
{
   va_list args;
   va_start (args, fmt);
   vprintf (fmt, args);
   va_end (args);
   return 0;
}

#if PNET_OPTION_LAN9662_SHOW_RTE_INFO
/**
 * Display RTE status using printf
 * @param mera    In: mera lib reference
 */
static void pf_mera_rte_show (struct mera_inst * mera)
{
   mera_debug_info_t info;

   info.group = MERA_DEBUG_GROUP_ALL;
   info.rtp_id = 0;   /* All RTP IDs */
   info.full = false; /* Only active IDs */
   info.clear = true;

   printf ("RTE Status:\n");
   mera_debug_info_print (mera, pf_mera_printf_cb, &info);
}
#endif

/**
 * @internal
 * Show current RTE configuration
 * This is a callback for the scheduler.
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    Handle to the LAN9662 driver.
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 *                                stored tasks.
 */
#if PNET_OPTION_LAN9662_SHOW_RTE_INFO
static void pf_mera_rte_show_periodic (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{

   pf_mera_t * handle = (pf_mera_t *)arg;
   pf_mera_rte_show (handle->mera_lib);

   pf_scheduler_add (
      net,
      PF_MERA_RTE_INFO_PERIOD_IN_US,
      pf_mera_rte_show_periodic,
      handle,
      &handle->sched_handle);
}
#endif

/**
 * @internal
 * Poll mera lib / RTE
 * This is a callback for the scheduler.
 * @param net              InOut: The p-net stack instance
 * @param arg              In:    Handle to the LAN9662 driver.
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 *                                stored tasks.
 */
static void pf_mera_poll (pnet_t * net, void * arg, uint32_t current_time)
{
   pf_mera_t * handle = (pf_mera_t *)arg;

   mera_poll (handle->mera_lib);

   pf_scheduler_add (
      net,
      PF_MERA_POLL_INTERVAL_IN_US,
      pf_mera_poll,
      handle,
      &handle->poll_handle);
}

/**
 * Add vcam rule for an inbound RTE connection
 * Calls the add_inbound_vcap_rule.sh script
 *
 * @param vcam_handle      In: VCAM handle
 * @param vlan_id          In: VLAN ID
 * @param mac_addr         In: MAC of management port
 * @param rt_vlan_idx      In: RTE VLAN index
 * @param frame_id         In: Profinet frame ID
 * @param rtp_id           In: RTP ID
 * @return 0 on success, -1 on error
 */
static int pf_mera_ib_add_vcam_rule (
   int vcam_handle,
   uint16_t vlan_id,
   pnet_ethaddr_t * mac_addr,
   int rt_vlan_idx,
   int frame_id,
   int rtp_id)
{
   const char * argv[8];
   char vlan_id_str[PF_MERA_SCRIPT_ARG_LEN];
   char mac_addr_str[PF_MERA_SCRIPT_ARG_LEN];
   char rt_vlan_idx_str[PF_MERA_SCRIPT_ARG_LEN];
   char vcam_handle_str[PF_MERA_SCRIPT_ARG_LEN];
   char frame_id_str[PF_MERA_SCRIPT_ARG_LEN];
   char rtp_id_str[PF_MERA_SCRIPT_ARG_LEN];

   snprintf (vlan_id_str, sizeof (vlan_id_str), "0x%x", vlan_id);
   snprintf (
      mac_addr_str,
      sizeof (mac_addr_str),
      "%02X:%02X:%02X:%02X:%02X:%02X",
      mac_addr->addr[0],
      mac_addr->addr[1],
      mac_addr->addr[2],
      mac_addr->addr[3],
      mac_addr->addr[4],
      mac_addr->addr[5]);

   snprintf (rt_vlan_idx_str, sizeof (rt_vlan_idx_str), "%d", rt_vlan_idx);
   snprintf (vcam_handle_str, sizeof (vcam_handle_str), "%d", vcam_handle);
   snprintf (frame_id_str, sizeof (frame_id_str), "%d", frame_id);
   snprintf (rtp_id_str, sizeof (rtp_id_str), "%d", rtp_id);

   argv[0] = "add_inbound_vcap_rule.sh";
   argv[1] = vcam_handle_str;
   argv[2] = vlan_id_str;
   argv[3] = mac_addr_str;
   argv[4] = rt_vlan_idx_str;
   argv[5] = frame_id_str;
   argv[6] = rtp_id_str;
   argv[7] = NULL;

   return pnal_execute_script (argv);
}

/**
 * Add vcam rule for an outbound RTE connection
 * Calls the add_outbound_vcap_rule.sh script
 *
 * @param vcam_handle      In: VCAM handle
 * @param vlan_id          In: VLAN ID
 * @param mac_addr         In: MAC of management port
 * @param rt_vlan_idx      In: RTE VLAN index
 * @param frame_id         In: Profinet frame ID
 * @param rtp_id           In: RTP ID
 * @return 0 on success, -1 on error
 */
int ob_add_vcam_rule (
   int vcam_handle,
   uint16_t vlan_id,
   pnet_ethaddr_t * mac_addr,
   int rt_vlan_idx,
   int frame_id,
   int rtp_id)
{
   const char * argv[7];
   char vlan_id_str[PF_MERA_SCRIPT_ARG_LEN];
   char mac_addr_str[PF_MERA_SCRIPT_ARG_LEN];
   char rt_vlan_idx_str[PF_MERA_SCRIPT_ARG_LEN];
   char vcam_handle_str[PF_MERA_SCRIPT_ARG_LEN];
   char frame_id_str[PF_MERA_SCRIPT_ARG_LEN];
   char rtp_id_str[PF_MERA_SCRIPT_ARG_LEN];

   snprintf (vlan_id_str, sizeof (vlan_id_str), "0x%x", vlan_id);
   snprintf (
      mac_addr_str,
      sizeof (mac_addr_str),
      "%02X:%02X:%02X:%02X:%02X:%02X",
      mac_addr->addr[0],
      mac_addr->addr[1],
      mac_addr->addr[2],
      mac_addr->addr[3],
      mac_addr->addr[4],
      mac_addr->addr[5]);
   snprintf (rt_vlan_idx_str, sizeof (rt_vlan_idx_str), "%d", rt_vlan_idx);
   snprintf (vcam_handle_str, sizeof (vcam_handle_str), "%d", vcam_handle);
   snprintf (frame_id_str, sizeof (frame_id_str), "%d", frame_id);
   snprintf (rtp_id_str, sizeof (rtp_id_str), "%d", rtp_id);

   argv[0] = "add_outbound_vcap_rule.sh";
   argv[1] = vcam_handle_str;
   argv[2] = vlan_id_str;
   argv[3] = rt_vlan_idx_str;
   argv[4] = frame_id_str;
   argv[5] = rtp_id_str;
   argv[6] = mac_addr_str;
   argv[7] = NULL;

   return pnal_execute_script (argv);
}

/**
 * Delete vcam rule
 * Calls the delete_vcap_rule.sh script
 * @param vcam_handle      In: VCAM handle
 * @param vlan_id          In: VLAN ID
 * @param rt_vlan_idx      In: RTE VLAN index
 * @param frame_id         In: Profinet frame ID
 * @param rtp_id           In: RTP ID
 * @return 0 on success, -1 on error
 */
static int pf_mera_delete_vcam_rule (int vcam_handle)
{
   const char * argv[3];
   char vcam_handle_str[16];

   snprintf (vcam_handle_str, sizeof (vcam_handle_str), "%d", vcam_handle);

   argv[0] = "del_vcap_rule.sh";
   argv[1] = vcam_handle_str;
   argv[2] = NULL;

   return pnal_execute_script (argv);
}

/**
 * Allocate a vcam ID for a RTE configuration
 * @param handle     InOut: Ref to mera instance
 * @return vcam ID, PF_MERA_INVALID_ID on error
 */
static uint16_t pf_mera_vcam_id_alloc (pf_mera_t * handle)
{
   uint16_t i;
   CC_ASSERT (handle != NULL);
   for (i = 0; i < NELEMENTS (handle->vcam_id.used); i++)
   {
      if (handle->vcam_id.used[i] == false)
      {
         handle->vcam_id.used[i] = true;
         return handle->vcam_id.base_id + i;
      }
   }
   LOG_ERROR (PF_MERA_LOG, "MERA(%d): Failed to alloc VCAM ID \n", __LINE__);
   return PF_MERA_INVALID_ID;
}

/**
 * Free a vcam ID
 * @param handle     InOut: Ref to mera instance
 * @param id         In: vcam ID
 */
static void pf_mera_vcam_id_free (pf_mera_t * handle, uint16_t id)
{
   CC_ASSERT (handle != NULL);
   if (id != PF_MERA_INVALID_ID)
   {
      handle->vcam_id.used[id - handle->vcam_id.base_id] = false;
   }
}

/**
 * Allocate a vlan index for a RTE configuration
 * @param handle     InOut: Ref to mera instance
 * @return vlan index, PF_MERA_INVALID_ID on error
 */
static uint16_t pf_mera_vlan_index_alloc (pf_mera_t * handle)
{
   uint16_t i;
   CC_ASSERT (handle != NULL);
   for (i = 0; i < NELEMENTS (handle->vlan_index.used); i++)
   {
      if (handle->vlan_index.used[i] == false)
      {
         handle->vlan_index.used[i] = true;
         return handle->vlan_index.base_id + i;
      }
   }
   LOG_ERROR (PF_MERA_LOG, "MERA(%d): Failed to alloc vlan index\n", __LINE__);
   return PF_MERA_INVALID_ID;
}

/**
 * Free a vlan index
 * @param handle     InOut: Ref to mera instance
 * @param id         In: vlan index
 */
static void pf_mera_vlan_index_free (pf_mera_t * handle, uint16_t id)
{
   CC_ASSERT (handle != NULL);
   if (id != PF_MERA_INVALID_ID)
   {
      handle->vlan_index.used[id - handle->vlan_index.base_id] = false;
   }
}

/**
 * Allocate a RTE ID
 * @param handle     InOut: Ref to mera instance
 * @return RTP ID, PF_MERA_INVALID_ID on error
 */
static uint16_t pf_mera_rtp_id_alloc (pf_mera_t * handle)
{
   uint16_t i;
   CC_ASSERT (handle != NULL);

   for (i = 0; i < NELEMENTS (handle->rtp.used); i++)
   {
      if (handle->rtp.used[i] == false)
      {
         handle->rtp.used[i] = true;
         return handle->rtp.base_id + i;
      }
   }
   LOG_ERROR (PF_MERA_LOG, "MERA(%d): Failed to allocate RTP ID\n", __LINE__);
   return PF_MERA_INVALID_ID;
}

/**
 * Free a RTP ID
 * @param handle     InOut: Ref to mera instance
 * @param id         In: RTP ID
 */
static void pf_mera_rtp_id_free (pf_mera_t * handle, uint16_t id)
{
   CC_ASSERT (handle != NULL);
   if (id != PF_MERA_INVALID_ID)
   {
      handle->rtp.used[id - handle->rtp.base_id] = false;
   }
}

/**
 * Allocate a Read Action List (RAL) ID
 * @param handle     InOut: Ref to mera instance
 * @return ral ID, PF_MERA_INVALID_ID on error
 */
static uint16_t pf_mera_ral_id_alloc (pf_mera_t * handle)
{
   uint16_t i;
   CC_ASSERT (handle != NULL);

   for (i = 0; i < NELEMENTS (handle->ral.used); i++)
   {
      if (handle->ral.used[i] == false)
      {
         handle->ral.used[i] = true;
         return handle->ral.base_id + i;
      }
   }
   LOG_ERROR (PF_PPM_LOG, "MERA(%d): Failed to alloc RAL ID \n", __LINE__);
   return PF_MERA_INVALID_ID;
}

/**
 * Free a Read Action List (RAL) ID
 * @param handle     InOut: Ref to mera instance
 * @param id         In: ral ID
 */
static void pf_mera_ral_id_free (pf_mera_t * handle, uint16_t id)
{
   CC_ASSERT (handle != NULL);
   if (id != PF_MERA_INVALID_ID)
   {
      handle->ral.used[id - handle->ral.base_id] = false;
   }
}

/**
 * Allocate a Write Action List (WAL) ID
 * @param handle     InOut: Ref to mera instance
 * @return wal ID, PF_MERA_INVALID_ID on error
 */
static uint16_t pf_mera_wal_id_alloc (pf_mera_t * handle)
{
   uint16_t i;
   CC_ASSERT (handle != NULL);

   for (i = 0; i < NELEMENTS (handle->wal.used); i++)
   {
      if (handle->wal.used[i] == false)
      {
         handle->wal.used[i] = true;
         return handle->wal.base_id + i;
      }
   }
   LOG_ERROR (PF_PPM_LOG, "MERA(%d): Failed to alloc WAL ID \n", __LINE__);
   return PF_MERA_INVALID_ID;
}

/**
 * Free a Write Action List (WAL) ID
 * @param handle     InOut: Ref to mera instance
 * @param id         In: wal ID
 */
static void pf_mera_wal_id_free (pf_mera_t * handle, uint16_t id)
{
   CC_ASSERT (handle != NULL);
   if (id != PF_MERA_INVALID_ID)
   {
      handle->wal.used[id - handle->wal.base_id] = false;
   }
}

/**
 * Add RTE data group for an iodata object.
 * Maps the data source to frame/pdu offset.
 * @param mera_lib   In: Ref to mera lib instance
 * @param frame      In: Ref to frame
 * @param entry      In: Iodata object configuration
 * @return 0 on success, -1 on error
 */
static int pf_mera_ib_add_dg (
   struct mera_inst * mera_lib,
   pf_mera_frame_t * frame,
   pf_rte_object_t * entry)
{
   int err;
   mera_ib_dg_conf_t ib_dg_cfg;

   err = mera_ib_dg_init (&ib_dg_cfg);
   if (err)
   {
      LOG_ERROR (
         PF_MERA_LOG,
         "MERA(%d): Failed to init data group (dg)\n",
         __LINE__);
      return -1;
   }

   ib_dg_cfg.rtp_id = frame->rtp_id;
   ib_dg_cfg.pdu_offset = entry->iodata->data_offset + frame->rte_data_offset;
   ib_dg_cfg.valid_offset = entry->iodata->iops_offset + frame->rte_data_offset;
   if (entry->io_interface_type == MERA_IO_INTF_SRAM)
   {
      /* In SRAM mode the IOPS is handled by SW */
      ib_dg_cfg.valid_update = false;
   }
   else
   {
      ib_dg_cfg.valid_update = true;
   }

   ib_dg_cfg.opc_seq_update = false;
   ib_dg_cfg.opc_code_update = false;

   LOG_DEBUG (
      PF_PPM_LOG,
      "MERA(%d): mera_ib_dg_add "
      "ral_id:%d "
      "ra_id:%d "
      "cfg.rtp_id:%d "
      "cfg.pdu_offset:%d "
      "cfg.valid_offset:%d\n",
      __LINE__,
      entry->ib_ral_id,
      entry->ib_ra_id,
      ib_dg_cfg.rtp_id,
      ib_dg_cfg.pdu_offset,
      ib_dg_cfg.valid_offset);

   err =
      mera_ib_dg_add (mera_lib, entry->ib_ral_id, entry->ib_ra_id, &ib_dg_cfg);
   if (err)
   {
      LOG_ERROR (
         PF_PPM_LOG,
         "MERA(%d): Failed to add inbound data group\n",
         __LINE__);
      return err;
   }

   return err;
}
/**
 * Convert P-Net interface data type to mera lib type.
 * @param pnet_if_type     In: P-Net interface type
 * @return mera lib interface type
 */
static mera_io_intf_t pf_mera_interface_type (
   pnet_mera_rte_data_type_t pnet_if_type)
{
   switch (pnet_if_type)
   {
   case PNET_MERA_DATA_TYPE_SRAM:
      return MERA_IO_INTF_SRAM;
   case PNET_MERA_DATA_TYPE_QSPI:
      return MERA_IO_INTF_QSPI;
   default:
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Unsupported interface type\n",
         __LINE__);
      exit (EXIT_FAILURE);
   }
}

/**
 * Add RTE read action for an iodata object.
 * Configures the data source
 * @param mera_lib   In: Ref to mera lib instance
 * @param entry      In: Iodata object configuration
 * @return 0 on success, -1 on error
 */
static int pf_mera_ib_add_ra (
   struct mera_inst * mera_lib,
   pf_rte_object_t * entry)
{
   mera_ib_ra_conf_t ib_ra_cfg;
   mera_ib_ra_ctrl_t ctrl;

   if (mera_ib_ra_init (&ib_ra_cfg) != 0)
   {
      LOG_ERROR (PF_PPM_LOG, "MERA(%d): Failed to init read action\n", __LINE__);
      return -1;
   }

   ib_ra_cfg.ra_id = entry->ib_ra_id;
   ib_ra_cfg.rd_addr.intf = entry->io_interface_type;
   ib_ra_cfg.rd_addr.addr = entry->sram_address;
   if (entry->io_interface_type == MERA_IO_INTF_QSPI)
   {
      ib_ra_cfg.rd_addr.addr = entry->qspi_address;
   }
   ib_ra_cfg.length = entry->iodata->data_length;

   LOG_DEBUG (
      PF_PPM_LOG,
      "MERA(%d): mera_ib_ra_add "
      "ral_id:%d "
      "cfg.ra_id:%d "
      "cfg.rd_addr.intf:%d-%s "
      "cfg.rd_addr.addr: 0x%x "
      "cfg.length:%d\n",
      __LINE__,
      entry->ib_ral_id,
      entry->ib_ra_id,
      ib_ra_cfg.rd_addr.intf,
      pf_mera_interface_type_to_str (ib_ra_cfg.rd_addr.intf),
      ib_ra_cfg.rd_addr.addr,
      ib_ra_cfg.length);

   if (mera_ib_ra_add (mera_lib, entry->ib_ral_id, &ib_ra_cfg) != 0)
   {
      LOG_ERROR (PF_MERA_LOG, "MERA(%d): Failed to add read action\n", __LINE__);
      return -1;
   }

   if (ib_ra_cfg.rd_addr.intf == MERA_IO_INTF_QSPI)
   {
      ctrl.enable = true;
      if (
         mera_ib_ra_ctrl_set (
            mera_lib,
            entry->ib_ral_id,
            entry->ib_ra_id,
            &ctrl) != 0)
      {
         LOG_ERROR (
            PF_PPM_LOG,
            "MERA(%d): mera_ib_ra_ctrl_set() error\n",
            __LINE__);
         return -1;
      }
   }

   return 0;
}

/**
 * Add RTE read action list for an iodata object.
 * Configures when data source shall be read.
 * @param mera_lib       In: Ref to mera lib instance
 * @param entry          In: Iodata object configuration
 * @param interval_in_ns In: Read interval in nanoseconds
 * @return 0 on success, -1 on error
 */
static int pf_mera_ib_add_ral (
   struct mera_inst * mera_lib,
   pf_rte_object_t * entry,
   uint32_t interval_in_ns)
{
   mera_ib_ral_conf_t ral_cfg;

   ral_cfg.time.offset = PF_MERA_RAL_TIME_OFFSET * entry->ib_ral_id;
   ral_cfg.time.interval = interval_in_ns;

   LOG_DEBUG (
      PF_PPM_LOG,
      "MERA(%d): mera_ib_ral_conf_set "
      "ral_id:%d "
      "cfg.time.offset:%d(ns) "
      "cfg.time.interval:%d(ns)\n",
      __LINE__,
      entry->ib_ral_id,
      ral_cfg.time.offset,
      ral_cfg.time.interval);

   if (mera_ib_ral_conf_set (mera_lib, entry->ib_ral_id, &ral_cfg) != 0)
   {
      LOG_ERROR (
         PF_MERA_LOG,
         "MERA(%d): Failed to set read action configuration\n",
         __LINE__);
      return -1;
   }

   return 0;
}

/**
 * Add RTE configuration for an inbound iodata object.
 * @param handle     In: Ref to mera instance
 * @param frame      In: Ref to frame
 * @param entry      In: Iodata object configuration
 * @return 0 on success, -1 on error
 */
static int pf_mera_add_ib_entry (
   pf_mera_t * handle,
   pf_mera_frame_t * frame,
   pf_rte_object_t * entry)
{
   mera_ob_wa_conf_t wa_cfg;
   entry->in_use = true;

   LOG_DEBUG (
      PF_PPM_LOG,
      "MERA(%d): [%u,%u] Configure inbound data entry\n",
      __LINE__,
      entry->iodata->slot_nbr,
      entry->iodata->subslot_nbr);

   entry->ib_ral_id = pf_mera_ral_id_alloc (handle);

   if (pf_mera_ib_add_ra (handle->mera_lib, entry) != 0)
   {
      LOG_ERROR (PF_MERA_LOG, "MERA(%d): Failed ta add read action\n", __LINE__);
      return -1;
   }

   if (pf_mera_ib_add_dg (handle->mera_lib, frame, entry) != 0)
   {
      LOG_ERROR (PF_MERA_LOG, "MERA(%d): Failed ta add data group\n", __LINE__);
      return -1;
   }

   if (pf_mera_ib_add_ral (handle->mera_lib, entry, frame->interval) != 0)
   {
      LOG_ERROR (
         PF_MERA_LOG,
         "MERA(%d): Failed ta add read action list\n",
         __LINE__);
      return -1;
   }

   if (entry->io_interface_type == MERA_IO_INTF_QSPI)
   {
      LOG_DEBUG (
         PF_MERA_LOG,
         "MERA(%d): [%u,%u] Add internal transfer QSPI 0x%x -> SRAM 0x%x\n",
         __LINE__,
         entry->iodata->slot_nbr,
         entry->iodata->subslot_nbr,
         entry->qspi_address,
         entry->sram_address);

      if (mera_ob_wa_init (&wa_cfg) != 0)
      {
         LOG_ERROR (
            PF_MERA_LOG,
            "MERA(%d): Failed init internal write action\n",
            __LINE__);
         return 0;
      }

      wa_cfg.internal = true;
      /* wa_cfg.rtp_id not used for internal transfers */
      /* wa_cfg.grp_id not used for internal transfers */
      /* wa_cfg.dg_id  not used for internal transfers */

      /* Read address */
      wa_cfg.rd_addr.intf = MERA_IO_INTF_QSPI;
      wa_cfg.rd_addr.addr = entry->qspi_address;

      /* Write address */
      wa_cfg.wr_addr.intf = MERA_IO_INTF_SRAM;
      wa_cfg.wr_addr.addr = entry->sram_address;

      wa_cfg.length = entry->iodata->data_length;

      if (mera_ob_wa_add (handle->mera_lib, frame->internal_wal_id, &wa_cfg) != 0)
      {
         LOG_ERROR (
            PF_MERA_LOG,
            "MERA(%d): Failed to add internal write action\n",
            __LINE__);
         return -1;
      }
   }

   return 0;
}

/**
 * Set the general RTE configuration for a PPM frame.
 * @param handle     In: Ref to mera instance
 * @param frame      In: Ref to frame
 * @param entry      In: Iodata object configuration
 * @return 0 on success, -1 on error
 */
static int pf_mera_ib_rtp_config (pf_mera_t * handle, pf_mera_frame_t * frame)
{
   int err = 0;
   mera_ib_rtp_conf_t ib_rtp_config = {0};
   pf_ppm_t * p_ppm = &frame->config.p_iocr->ppm;

   ib_rtp_config.type = MERA_RTP_TYPE_PN;
   ib_rtp_config.grp_id = PF_MERA_GROUP_ID_NONE;
   ib_rtp_config.mode = MERA_RTP_IB_MODE_INJ;
   ib_rtp_config.time.offset = MERA_TIME_OFFSET_NONE;
   ib_rtp_config.time.interval = frame->interval;
   ib_rtp_config.port = frame->port;
   ib_rtp_config.length = ((pnal_buf_t *)p_ppm->p_send_buffer)->len;

   memcpy (
      ib_rtp_config.data,
      ((pnal_buf_t *)p_ppm->p_send_buffer)->payload,
      ((pnal_buf_t *)p_ppm->p_send_buffer)->len);

   LOG_DEBUG (
      PF_PPM_LOG,
      "MERA(%d): mera_ib_rtp_conf_set "
      "rtp_id:%d "
      "type: %d "
      "grp_id:%d "
      "mode:%d "
      "time.offset:%d "
      "time.interval:%d "
      "port:%d "
      "length:%d\n",
      __LINE__,
      frame->rtp_id,
      ib_rtp_config.type,
      ib_rtp_config.grp_id,
      ib_rtp_config.mode,
      ib_rtp_config.time.offset,
      ib_rtp_config.time.interval,
      ib_rtp_config.port,
      ib_rtp_config.length);

   err = mera_ib_rtp_conf_set (handle->mera_lib, frame->rtp_id, &ib_rtp_config);
   if (err)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed to set inbound RTP configuration\n",
         __LINE__);
   }
   return err;
}

/**
 * Configure and start a RTE configuration for a PPM frame.
 * Sets the general RTE configuration and the configuration
 * for each iodata object in the communication relationship.
 * @param handle     In: Ref to mera instance
 * @param frame      In: Ref to frame
 * @return 0 on success, -1 on error
 */
static int pf_mera_ppm_start_rte (pf_mera_t * handle, pf_mera_frame_t * frame)
{
   int err = 0;
   pf_iocr_t * p_iocr = frame->config.p_iocr;
   uint16_t slot, subslot;
   mera_ob_wal_conf_t wal_conf;

   err = pf_mera_ib_add_vcam_rule (
      frame->vcam_id,
      p_iocr->param.iocr_tag_header.vlan_id,
      &frame->config.main_mac_addr, /* MAC of sending interface */
      frame->vlan_idx,
      p_iocr->param.frame_id,
      frame->rtp_id);

   if (err != 0)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed to add inbound vcam rule\n",
         __LINE__);
      return err;
   }

   err = pf_mera_ib_rtp_config (handle, frame);
   if (err != 0)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed to set inbound RTP configuration\n",
         __LINE__);
      return err;
   }

   /*
    * Add WAL used for internal write actions QSPI -> SRAM
    * The names of the called functions is a bit misleading
    * since the WAL is for internal writes and has no data group (dg)
    * or rtp id associated with it and no relation to outbound data.
    */
   mera_ob_wal_conf_get (handle->mera_lib, frame->internal_wal_id, &wal_conf);
   wal_conf.time.offset = 100000;
   wal_conf.time.interval = frame->interval;

   if (
      mera_ob_wal_conf_set (
         handle->mera_lib,
         frame->internal_wal_id,
         &wal_conf) != 0)
   {
      LOG_ERROR (
         PF_MERA_LOG,
         "MERA(%d): Failed to set WAL for internal transfers\n",
         __LINE__);
   }

   for (slot = 0; slot < PNET_MAX_SLOTS; slot++)
   {
      for (subslot = 0; subslot < PNET_MAX_SUBSLOTS; subslot++)
      {
         if (
            frame->rte_object[slot][subslot].iodata != NULL &&
            frame->rte_object[slot][subslot].iodata->data_length > 0)
         {
            err = pf_mera_add_ib_entry (
               handle,
               frame,
               &frame->rte_object[slot][subslot]);

            if (err != 0)
            {
               return err;
            }
         }
      }
   }

   frame->active = true;

   return err;
}

int pf_driver_init (pnet_t * net)
{
   pf_mera_t * handle;
   mera_cb_t mera_lib_cb = {
      .reg_rd = pf_rte_uio_reg_read,
      .reg_wr = pf_rte_uio_reg_write,
      .lock = NULL,
      .unlock = NULL,
      .trace_printf = pf_mera_callout_trace_printf,
      .trace_hex_dump = NULL};

   LOG_DEBUG (PF_MERA_LOG, "MERA(%d): Init RTE UIO driver\n", __LINE__);

   if (pf_rte_uio_init() != 0)
   {
      LOG_FATAL (
         PF_PPM_LOG,
         "MERA(%d): RTE UIO driver initialization error\n",
         __LINE__);
      exit (EXIT_FAILURE);
   }

   LOG_DEBUG (PF_MERA_LOG, "MERA(%d): Init SRAM UIO driver\n", __LINE__);
   if (pf_sram_init() != 0)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): SRAM UIO driver initialization error\n",
         __LINE__);
      exit (EXIT_FAILURE);
   }

   handle = (pf_mera_t *)malloc (sizeof (pf_mera_t));
   CC_ASSERT (handle != NULL);
   memset (handle, 0, sizeof (pf_mera_t));

   handle->vcam_id.base_id =
      net->p_fspm_default_cfg->driver_config.mera.vcam_base_id;
   handle->rtp.base_id =
      net->p_fspm_default_cfg->driver_config.mera.rtp_base_id;
   handle->ral.base_id =
      net->p_fspm_default_cfg->driver_config.mera.ral_base_id;
   handle->wal.base_id =
      net->p_fspm_default_cfg->driver_config.mera.wal_base_id;

   handle->mera_lib = mera_create (&mera_lib_cb);
   if (handle->mera_lib == NULL)
   {
      LOG_FATAL (PF_MERA_LOG, "MERA(%d): Failed to init MERA lib\n", __LINE__);
      exit (EXIT_FAILURE);
   }

   /* Start mera poll timeout */
   pf_scheduler_init_handle (&handle->poll_handle, "mera_poll");
   if (
      pf_scheduler_add (
         net,
         PF_MERA_POLL_INTERVAL_IN_US,
         pf_mera_poll,
         handle,
         &handle->poll_handle) != 0)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed to start poll timer\n",
         __LINE__);
      exit (EXIT_FAILURE);
   }

   handle->hwo_drv.initialized = true;
   net->hwo_drv = (pf_drv_t *)handle;

   return 0;
};

/**
 * Allocate a mera frame
 * The frame contains all required RTE configurations for a PPM or CPM.
 * Iodata interface type is initialized to SRAM.
 * @param handle     In: Ref to mera instance
 * @return Allocated frame, NULL on error
 */
static pf_mera_frame_t * pf_mera_alloc_frame (pf_mera_t * handle)
{
   uint16_t i, j, k;
   pf_mera_frame_t * frame = NULL;

   for (i = 0; i < PNET_LAN9662_MAX_FRAMES; i++)
   {
      if (handle->frame[i].used == false)
      {
         frame = &handle->frame[i];
         memset (frame, 0, sizeof (*frame));
         frame->drv_frame.initialized = true;
         frame->used = true;
         frame->active = false;
         frame->ob_dg_count = 1;

         for (j = 0; j < PNET_MAX_SLOTS; j++)
         {
            for (k = 0; k < PNET_MAX_SUBSLOTS; k++)
            {
               frame->rte_object[j][k].ib_ral_id = PF_MERA_INVALID_ID;
               frame->rte_object[j][k].io_interface_type = MERA_IO_INTF_SRAM;
               frame->rte_object[j][k].sram_address = PF_SRAM_INVALID_ADDRESS;
               frame->mera_lib = handle->mera_lib;
            }
         }
         LOG_DEBUG (
            PF_MERA_LOG,
            "MERA(%d): Frame allocated (index=%d)\n",
            __LINE__,
            i);
         return frame;
      }
   }

   LOG_FATAL (PF_MERA_LOG, "MERA(%d): Failed to allocate frame\n", __LINE__);
   exit (EXIT_FAILURE);
   return NULL;
}

bool pf_mera_is_active_frame (pf_drv_frame_t * frame)
{
   return ((pf_mera_frame_t *)frame)->active;
}

/**
 * Free a mera frame
 * @param handle     InOut: Ref to mera instance
 * @param frame      In: The frame to free
 */
static void pf_mera_free_frame (pf_mera_t * handle, pf_mera_frame_t * frame)
{
   if (frame != NULL)
   {
      frame->used = false;
      return;
   }
   LOG_ERROR (PF_MERA_LOG, "MERA(%d): Failed to free frame\n", __LINE__);
}

pf_drv_frame_t * pf_mera_ppm_alloc (
   pnet_t * net,
   const pf_mera_frame_cfg_t * cfg)
{
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   pf_mera_frame_t * frame = NULL;
   pf_iodata_object_t * p_iodata;
   pf_rte_object_t * entry;
   uint16_t i;
   pf_ppm_t * ppm;

   if (cfg == NULL || cfg->p_iocr == NULL || handle == NULL)
   {
      return NULL;
   }

   frame = pf_mera_alloc_frame (handle);
   if (frame == NULL)
   {
      return NULL;
   }

   frame->is_ppm = true;

   frame->sram_frame_address = pf_sram_frame_alloc();
   if (frame->sram_frame_address == 0)
   {
      LOG_ERROR (
         PF_MERA_LOG,
         "MERA(%d): Failed to allocate SRAM frame\n",
         __LINE__);
      pf_mera_free_frame (handle, frame);
      return NULL;
   }

   LOG_DEBUG (
      PF_MERA_LOG,
      "MERA(%d): Allocated PPM SRAM frame at 0x%x\n",
      __LINE__,
      frame->sram_frame_address);

   frame->config = *cfg;
   frame->port = cfg->port;

   ppm = &frame->config.p_iocr->ppm;
   frame->interval =
      (1000000 * ppm->send_clock_factor * ppm->reduction_ratio) / 32;

   for (i = 0; i < frame->config.p_iocr->nbr_data_desc; i++)
   {
      p_iodata = &frame->config.p_iocr->data_desc[i];
      if (p_iodata->data_length > 0)
      {
         entry =
            get_rte_object (frame, p_iodata->slot_nbr, p_iodata->subslot_nbr);
         entry->iodata = p_iodata;
      }
   }

   frame->vcam_id = pf_mera_vcam_id_alloc (handle);
   frame->rtp_id = pf_mera_rtp_id_alloc (handle);
   frame->internal_wal_id = pf_mera_wal_id_alloc (handle);
   frame->vlan_idx = pf_mera_vlan_index_alloc (handle);
   frame->rte_data_offset = PF_MERA_PPM_IODATA_OFFSET;

   LOG_DEBUG (PF_MERA_LOG, "MERA(%d): PPM frame allocated\n", __LINE__);
   return (pf_drv_frame_t *)frame;
}

int pf_mera_ppm_free (pnet_t * net, pf_drv_frame_t * drv_frame)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   LOG_DEBUG (PF_PPM_LOG, "MERA(%d): Free PPM frame\n", __LINE__);
   if (frame != NULL)
   {
      pf_mera_vcam_id_free (handle, frame->vcam_id);
      pf_mera_vlan_index_free (handle, frame->vlan_idx);
      pf_mera_wal_id_free (handle, frame->internal_wal_id);
      pf_mera_rtp_id_free (handle, frame->rtp_id);
      pf_sram_frame_free (frame->sram_frame_address);
      pf_mera_free_frame (handle, frame);
      return 0;
   }
   LOG_ERROR (PF_MERA_LOG, "MERA(%d): Failed to free PPM frame\n", __LINE__);
   return -1;
}

/**
 * Generate RTE configurations for all iodata objects in a PPM type
 * of communication relation.
 * @param handle     InOut: Ref to mera instance
 * @param frame      InOut: The PPM frame
 * @return 0 on success, -1 on error
 */
static int pf_mera_ppm_config (pf_mera_t * handle, pf_mera_frame_t * frame)
{
   int ret = 0;
   uint16_t slot, subslot;
   pf_rte_object_t * entry;
   const pf_mera_rte_cfg_t * entry_rte_data_cfg; /* qspi address */

   LOG_DEBUG (PF_PPM_LOG, "MERA(%d): Apply RTE configuration (PPM)\n", __LINE__);

   for (slot = 0; slot < PNET_MAX_SLOTS; slot++)
   {
      for (subslot = 0; subslot < PNET_MAX_SUBSLOTS; subslot++)
      {
         if (
            frame->rte_object[slot][subslot].iodata != NULL &&
            frame->rte_object[slot][subslot].iodata->data_length > 0)
         {
            entry = &frame->rte_object[slot][subslot];
            entry_rte_data_cfg = &handle->rte_data_cfg[slot][subslot];

            entry->ib_ra_id = frame->ib_ra_id_count++;

            if (entry_rte_data_cfg->valid)
            {
               entry->io_interface_type =
                  pf_mera_interface_type (entry_rte_data_cfg->cfg.type);

#if PF_MERA_ALLOW_APPLICATION_SRAM
               if (entry_rte_data_cfg->cfg.type == PNET_MERA_DATA_TYPE_SRAM)
               {
                  entry->sram_address = entry_rte_data_cfg->cfg.address;
               }
#endif
               entry->qspi_address = entry_rte_data_cfg->cfg.address;
               entry->default_data = entry_rte_data_cfg->cfg.default_data;
            }

            if (entry->sram_address == PF_SRAM_INVALID_ADDRESS)
            {
               entry->sram_address = frame->sram_frame_address +
                                     frame->rte_data_offset +
                                     entry->iodata->data_offset;
            }

            LOG_DEBUG (
               PF_PPM_LOG,
               "MERA(%d): [%u,%u] Interface: %s Start address: 0x%x Size: %d\n",
               __LINE__,
               entry->iodata->slot_nbr,
               entry->iodata->subslot_nbr,
               pf_mera_interface_type_to_str (entry->io_interface_type),
               entry->io_interface_type == MERA_IO_INTF_QSPI
                  ? entry->qspi_address
                  : entry->sram_address,
               entry->iodata->data_length);
         }
      }
   }

   return ret;
}

int pf_mera_ppm_start (pnet_t * net, pf_drv_frame_t * drv_frame)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   if (handle == NULL || frame == NULL || frame->config.p_iocr == NULL)
   {
      LOG_ERROR (
         PF_PPM_LOG,
         "MERA(%d): Failed to start RTE - parameter error\n",
         __LINE__);
      return -1;
   }

   if (pf_mera_ppm_config (handle, frame) != 0)
   {
      LOG_ERROR (PF_PPM_LOG, "MERA(%d): Failed to configure RTE\n", __LINE__);
      return -1;
   }

   if (pf_mera_ppm_start_rte (handle, frame) != 0)
   {
      LOG_ERROR (PF_PPM_LOG, "MERA(%d): Failed to start RTE\n", __LINE__);
      return -1;
   }

#if PNET_OPTION_LAN9662_SHOW_RTE_INFO
   pf_scheduler_init_handle (&handle->sched_handle, "mera_show");
   if (
      pf_scheduler_add (
         net,
         2000000,
         pf_mera_rte_show_periodic,
         handle,
         &handle->sched_handle) != 0)
   {
      LOG_FATAL (PF_MERA_LOG, "MERA(%d): Failed to add timer\n", __LINE__);
      exit (EXIT_FAILURE);
   }
#endif

   return 0;
}

int pf_mera_ppm_write (
   pnet_t * net,
   pf_drv_frame_t * drv_frame,
   uint32_t offset,
   const uint8_t * data,
   uint32_t len)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   uint16_t i;
   mera_ib_rtp_data_t write_req;
   int ret;

   if (frame->active)
   {
      for (i = 0; i < len; i++)
      {
         write_req.value = data[i];
         write_req.offset = offset + i;
         ret =
            mera_ib_rtp_data_set (handle->mera_lib, frame->rtp_id, &write_req);

         if (ret != 0)
         {
            LOG_ERROR (
               PF_MERA_LOG,
               "MERA(%d): Failed to write frame RTP frame buffer\n",
               __LINE__);
            return -1;
         }
      }
   }

   return 0;
}

int pf_mera_ppm_write_input (
   pnet_t * net,
   pf_drv_frame_t * drv_frame,
   const pf_iodata_object_t * p_iodata,
   const uint8_t * data,
   uint32_t len)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   int ret = -1;
   pf_rte_object_t * entry;

   if (!frame->active)
   {
      return -1;
   }

   entry = get_rte_object (frame, p_iodata->slot_nbr, p_iodata->subslot_nbr);

   switch (entry->io_interface_type)
   {
   case MERA_IO_INTF_SRAM:
      if (
         mera_ib_ral_req (
            handle->mera_lib,
            entry->ib_ral_id,
            &entry->ib_ral_buf) == 0)
      {
         ret = pf_sram_write (
            entry->sram_address + entry->ib_ral_buf.addr,
            data,
            len);

         if (mera_ib_ral_rel (handle->mera_lib, entry->ib_ral_id) != 0)
         {
            ret = -1;
            LOG_ERROR (
               PF_MERA_LOG,
               "MERA(%d): Releasing RAL buffer failed\n",
               __LINE__);
         }
      }
      else
      {
         ret = -1;
         LOG_ERROR (
            PF_MERA_LOG,
            "MERA(%d): Request for RAL buffer failed\n",
            __LINE__);
      }

      break;
   case PNET_MERA_DATA_TYPE_QSPI:
   default:
      /* Write not supported */
      return 0;
   }
   return ret;
}

int pf_mera_ppm_read_data_and_iops (
   pnet_t * net,
   pf_drv_frame_t * drv_frame,
   const pf_iodata_object_t * p_iodata,
   uint8_t * p_data,
   uint16_t data_len,
   uint8_t * p_iops,
   uint8_t iops_len)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   int err = 0;
   pf_rte_object_t * entry;
   mera_buf_t internal_wal_buf;

   *p_iops = PNET_IOXS_GOOD;

   if (!frame->active)
   {
      return -1;
   }

   if (p_iodata->slot_nbr == PNET_SLOT_DAP_IDENT)
   {
      return 0;
   }

   if (data_len == 0 || iops_len == 0)
   {
      return 0;
   }

   entry = get_rte_object (frame, p_iodata->slot_nbr, p_iodata->subslot_nbr);

   switch (entry->io_interface_type)
   {
   case MERA_IO_INTF_SRAM:
      if (pf_sram_read (entry->sram_address, p_data, data_len) != 0)
      {
         LOG_ERROR (
            PF_MERA_LOG,
            "MERA(%d): [%u,%u] Failed to read SRAM (input) \n",
            __LINE__,
            p_iodata->slot_nbr,
            p_iodata->subslot_nbr);

         return -1;
      }
      break;

   case MERA_IO_INTF_QSPI:
      err = mera_ob_wal_req (
         handle->mera_lib,
         frame->internal_wal_id,
         &internal_wal_buf);
      if (err != 0)
      {
         LOG_ERROR (
            PF_MERA_LOG,
            "MERA(%d): Failed to req wal buffer for internal transfer\n",
            __LINE__);
         return err;
      }
      if (
         pf_sram_read (
            entry->sram_address + internal_wal_buf.addr,
            p_data,
            data_len) != 0)
      {
         mera_ob_wal_rel (handle->mera_lib, frame->internal_wal_id);
         LOG_ERROR (
            PF_MERA_LOG,
            "MERA(%d): [%u,%u] Failed to read SRAM (input) \n",
            __LINE__,
            p_iodata->slot_nbr,
            p_iodata->subslot_nbr);

         return -1;
      }
      mera_ob_wal_rel (handle->mera_lib, frame->internal_wal_id);
      break;

   default:
      LOG_ERROR (
         PF_MERA_LOG,
         "MERA(%d): [%u,%u] unsupported interface type\n",
         __LINE__,
         p_iodata->slot_nbr,
         p_iodata->subslot_nbr);
      return -1;
   }
   return 0;
}

int pf_mera_ppm_write_iops (
   pnet_t * net,
   pf_drv_frame_t * drv_frame,
   const pf_iodata_object_t * p_iodata,
   const uint8_t * iops,
   uint8_t len)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   pf_rte_object_t * entry;

   if (p_iodata->slot_nbr == PNET_SLOT_DAP_IDENT)
   {
      /* Setting only supported on data submodules
       * DAP iops set during initial RTE configuration
       * and never changed.
       */
      return -1;
   }

   if (!frame->active)
   {
      return -1;
   }

   entry = get_rte_object (frame, p_iodata->slot_nbr, p_iodata->subslot_nbr);

   switch (entry->io_interface_type)
   {
   case MERA_IO_INTF_SRAM:
      return pf_mera_ppm_write (
         net,
         (pf_drv_frame_t *)frame,
         frame->config.p_iocr->ppm.buffer_pos + p_iodata->iops_offset,
         iops,
         len);

   case MERA_IO_INTF_QSPI:
   default:
      /* Writing IOPS is not supported
       * Automatically updated by RTE
       */
      return -1;
   }
   return 0;
}

int pf_mera_ppm_write_iocs (
   pnet_t * net,
   pf_drv_frame_t * drv_frame,
   const pf_iodata_object_t * p_iodata,
   const uint8_t * iops,
   uint8_t len)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;

   if (!frame->active)
   {
      return -1;
   }

   return pf_mera_ppm_write (
      net,
      drv_frame,
      frame->config.p_iocr->ppm.buffer_pos + p_iodata->iocs_offset,
      iops,
      len);
}

int pf_mera_ppm_write_data_status (
   pnet_t * net,
   pf_drv_frame_t * drv_frame,
   uint8_t data_status)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   if (frame == NULL || !frame->active)
   {
      /* Ignore writes during ppm initialization */
      return 0;
   }

   return pf_mera_ppm_write (
      net,
      drv_frame,
      frame->config.p_iocr->ppm.data_status_offset,
      &data_status,
      1);
}

int pf_mera_ppm_stop (pnet_t * net, pf_drv_frame_t * drv_frame)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   uint16_t i, j;

   LOG_DEBUG (PF_PPM_LOG, "MERA(%d): Stop PPM\n", __LINE__);
   if (frame->active)
   {
      mera_ib_flush (handle->mera_lib);
      pf_mera_delete_vcam_rule (frame->vcam_id);

      for (i = 0; i < PNET_MAX_SLOTS; i++)
      {
         for (j = 0; j < PNET_MAX_SUBSLOTS; j++)
         {
            pf_mera_ral_id_free (handle, frame->rte_object[i][j].ib_ral_id);
         }
      }
   }
   else
   {
      LOG_DEBUG (
         PF_PPM_LOG,
         "MERA(%d): No action - PPM is not started\n",
         __LINE__);
   }

   return 0;
}

/**
 * Set the general RTE configuration for a CPM frame.
 * CPM timing configuration is applied by the function
 * pf_mera_cpm_activate_rte_dht() at a later point
 * in the AR establishment procedure.
 * @param mera_lib   In: Ref to mera lib instance
 * @param frame      In: Ref to frame
 * @return 0 on success, -1 on error
 */
static int pf_mera_ob_rtp_config (
   struct mera_inst * mera_lib,
   pf_mera_frame_t * frame)
{
   int err = 0;

   mera_ob_rtp_conf_t ob_rtp_config = {0};

   ob_rtp_config.type = MERA_RTP_TYPE_PN;
   ob_rtp_config.grp_id = PF_MERA_GROUP_ID_NONE;
   ob_rtp_config.length = sizeof (uint16_t) + /* frame_id */
                          frame->config.p_iocr->param.c_sdu_length + /* Profinet
                                                                         data
                                                                         length
                                                                       */
                          sizeof (uint16_t) + /* cycle counter */
                          1 +                 /* data status */
                          1;                  /* transfer status */

   ob_rtp_config.pn_ds = PF_MERA_DATA_STATUS_MASK;
   ob_rtp_config.pn_discard = false;
   ob_rtp_config.opc_grp_ver = 0;
   ob_rtp_config.wal_enable = true;
   ob_rtp_config.wal_id = frame->ob_wal_id;

   ob_rtp_config.time.offset = MERA_TIME_OFFSET_NONE;
   ob_rtp_config.time.interval = 0;
   ob_rtp_config.time_cnt = 0;

   LOG_DEBUG (
      PF_CPM_LOG,
      "MERA(%d): mera_ob_rtp_conf_set "
      "type:%d "
      "grp_id:%d "
      "length:%d "
      "pn_ds:%d "
      "pn_discard:%d "
      "opc_grp_ver:%d "
      "wal_enable:%d "
      "wal_id:%d "
      "time.offset:%d "
      "time.interval:%d "
      "time_cnt:%d\n",
      __LINE__,
      ob_rtp_config.type,
      ob_rtp_config.grp_id,
      ob_rtp_config.length,
      ob_rtp_config.pn_ds,
      ob_rtp_config.pn_discard,
      ob_rtp_config.opc_grp_ver,
      ob_rtp_config.wal_enable,
      ob_rtp_config.wal_id,
      ob_rtp_config.time.offset,
      ob_rtp_config.time.interval,
      ob_rtp_config.time_cnt);

   err = mera_ob_rtp_conf_set (mera_lib, frame->rtp_id, &ob_rtp_config);
   if (err)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed to set RTP configuration\n",
         __LINE__);
   }
   return err;
}

/**
 * Set the data group (DG) configuration for an outbound iodata object
 * part of a CPM frame.
 * Sets the frame data offset and length.
 * @param mera_lib   In: Ref to mera lib instance
 * @param frame      In: Ref to frame
 * @param entry      In: Iodata object configuration
 * @return 0 on success, -1 on error
 */
static int pf_mera_ob_add_dg (
   struct mera_inst * mera_lib,
   pf_mera_frame_t * frame,
   pf_rte_object_t * entry)
{
   mera_ob_dg_conf_t dg_cfg;
   mera_ob_dg_ctrl_t dg_ctrl;

   if (mera_ob_dg_init (&dg_cfg) != 0)
   {
      LOG_ERROR (
         PF_MERA_LOG,
         "MERA(%d): Failed to init outbound data group\n",
         __LINE__);
      return -1;
   }

   dg_cfg.dg_id = entry->ob_dg_id;
   dg_cfg.pdu_offset = entry->iodata->data_offset + frame->rte_data_offset;
   dg_cfg.length = entry->iodata->data_length;
   dg_cfg.valid_offset = entry->iodata->iops_offset + frame->rte_data_offset;

   dg_cfg.valid_chk = true;
   dg_cfg.opc_seq_chk = false;
   dg_cfg.opc_code_chk = false;
   dg_cfg.invalid_default = true;
   if (entry->default_data != NULL)
   {
      memcpy (dg_cfg.data, entry->default_data, entry->iodata->data_length);
   }
   else
   {
      memset (dg_cfg.data, 0, entry->iodata->data_length);
   }

   LOG_DEBUG (
      PF_CPM_LOG,
      "MERA(%d): mera_ob_dg_add "
      "rtp_id:%d "
      "cfg.dg_id:%d "
      "cfg.pdu_offset:%d "
      "cfg.length:%d "
      "cfg.valid_offset:%d "
      "cfg.valid_chk:%d "
      "cfg.opc_seq_chk:%d "
      "cfg.opc_code_chk:%d "
      "cfg.invalid_default:%d\n",
      __LINE__,
      frame->rtp_id,
      dg_cfg.dg_id,
      dg_cfg.pdu_offset,
      dg_cfg.length,
      dg_cfg.valid_offset,
      dg_cfg.valid_chk,
      dg_cfg.opc_seq_chk,
      dg_cfg.opc_code_chk,
      dg_cfg.invalid_default);

   if (mera_ob_dg_add (mera_lib, frame->rtp_id, &dg_cfg) != 0)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed to add outbound data group\n",
         __LINE__);
      return -1;
   }

   dg_ctrl.enable = true;

   if (mera_ob_dg_ctrl_set (mera_lib, frame->rtp_id, entry->ob_dg_id, &dg_ctrl) != 0)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed to enable outbound data group\n",
         __LINE__);
      return -1;
   }

   return 0;
}

/**
 * Set the Write Action (WA) for an outbound iodata object
 * part of a CPM frame.
 * Sets the data sink (SRAM/QSPI address).
 * @param mera_lib   In: Ref to mera instance
 * @param frame      In: Ref to frame
 * @param entry      In: Iodata object configuration
 * @return 0 on success, -1 on error
 */
static int pf_mera_ob_add_wa (
   struct mera_inst * mera_lib,
   pf_mera_frame_t * frame,
   pf_rte_object_t * entry)
{
   mera_ob_wa_conf_t wa_cfg;
   if (mera_ob_wa_init (&wa_cfg) != 0)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed init outbound write action\n",
         __LINE__);
      return 0;
   }

   wa_cfg.internal = false;
   wa_cfg.rtp_id = frame->rtp_id;
   wa_cfg.grp_id = PF_MERA_GROUP_ID_NONE;
   wa_cfg.dg_id = entry->ob_dg_id;
   wa_cfg.wr_addr.intf = MERA_IO_INTF_SRAM;
   wa_cfg.wr_addr.addr = entry->sram_address;

   wa_cfg.length = entry->iodata->data_length;

   LOG_DEBUG (
      PF_CPM_LOG,
      "MERA(%d): mera_ob_wa_add "
      "wal_id:%d "
      "cfg.internal:%d "
      "cfg.rtp_id:%d "
      "cfg.grp_id:%d "
      "cfg.dg_id:%d "
      "cfg.length:%d "
      "cfg.wr_addr.intf:%d-%s "
      "cfg.wr_addr.addr:0x%x\n",
      __LINE__,
      frame->ob_wal_id,
      wa_cfg.internal,
      wa_cfg.rtp_id,
      wa_cfg.grp_id,
      wa_cfg.dg_id,
      wa_cfg.length,
      wa_cfg.wr_addr.intf,
      pf_mera_interface_type_to_str (wa_cfg.wr_addr.intf),
      wa_cfg.wr_addr.addr);

   if (mera_ob_wa_add (mera_lib, frame->ob_wal_id, &wa_cfg) != 0)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed to add outbound read action\n",
         __LINE__);
      return -1;
   }

   if (entry->io_interface_type == MERA_IO_INTF_QSPI)
   {
      wa_cfg.wr_addr.addr = entry->qspi_address;
      wa_cfg.wr_addr.intf = MERA_IO_INTF_QSPI;

      LOG_DEBUG (
         PF_CPM_LOG,
         "MERA(%d): mera_ob_wa_add "
         "wal_id:%d "
         "cfg.internal:%d "
         "cfg.rtp_id:%d "
         "cfg.grp_id:%d "
         "cfg.dg_id:%d "
         "cfg.length:%d "
         "cfg.wr_addr.intf:%d-%s "
         "cfg.wr_addr.addr:0x%x\n",
         __LINE__,
         frame->ob_wal_id,
         wa_cfg.internal,
         wa_cfg.rtp_id,
         wa_cfg.grp_id,
         wa_cfg.dg_id,
         wa_cfg.length,
         wa_cfg.wr_addr.intf,
         pf_mera_interface_type_to_str (wa_cfg.wr_addr.intf),
         wa_cfg.wr_addr.addr);

      if (mera_ob_wa_add (mera_lib, frame->ob_wal_id, &wa_cfg) != 0)
      {
         LOG_FATAL (
            PF_MERA_LOG,
            "MERA(%d): Failed to add outbound read action\n",
            __LINE__);
         return -1;
      }
   }

   return 0;
}

/**
 * Set the RTE configuration for an outbound iodata object
 * part of a CPM frame. Sets datagroup and write action.
 * @param mera_lib   In: Ref to mera lib instance
 * @param frame      In: Ref to frame
 * @param entry      In: Iodata object configuration
 * @return 0 on success, -1 on error
 */
static int pf_mera_add_ob_entry (
   struct mera_inst * mera_lib,
   pf_mera_frame_t * frame,
   pf_rte_object_t * entry)
{
   LOG_DEBUG (
      PF_CPM_LOG,
      "MERA(%d): [%u,%u] Configure outbound data entry\n",
      __LINE__,
      entry->iodata->slot_nbr,
      entry->iodata->subslot_nbr);

   entry->in_use = true;

   if (pf_mera_ob_add_dg (mera_lib, frame, entry) != 0)
   {
      return -1;
   }

   if (pf_mera_ob_add_wa (mera_lib, frame, entry) != 0)
   {
      return -1;
   }

   return 0;
}

/**
 * Generate RTE configurations for all iodata objects in a CPM type
 * of communication relation.
 * @param handle     InOut: Ref to mera instance
 * @param frame      InOut: The PPM frame
 * @return 0 on success, -1 on error
 */
static int pf_mera_cpm_config (pf_mera_t * handle, pf_mera_frame_t * frame)
{
   int ret = 0;
   uint16_t slot, subslot;
   pf_rte_object_t * entry;
   const pf_mera_rte_cfg_t * entry_rte_data_cfg; /* qspi address */

   LOG_DEBUG (PF_CPM_LOG, "MERA(%d): Apply RTE configuration (CMP)\n", __LINE__);

   for (slot = 0; slot < PNET_MAX_SLOTS; slot++)
   {
      for (subslot = 0; subslot < PNET_MAX_SUBSLOTS; subslot++)
      {
         if (
            frame->rte_object[slot][subslot].iodata != NULL &&
            frame->rte_object[slot][subslot].iodata->data_length > 0)
         {
            entry = &frame->rte_object[slot][subslot];
            entry_rte_data_cfg = &handle->rte_data_cfg[slot][subslot];
            entry->ob_dg_id = frame->ob_dg_count++;

            if (entry_rte_data_cfg->valid)
            {
               entry->io_interface_type =
                  pf_mera_interface_type (entry_rte_data_cfg->cfg.type);

#if PF_MERA_ALLOW_APPLICATION_SRAM
               if (entry_rte_data_cfg->cfg.type == PNET_MERA_DATA_TYPE_SRAM)
               {
                  entry->sram_address = entry_rte_data_cfg->cfg.address;
               }
#endif
               entry->qspi_address = entry_rte_data_cfg->cfg.address;
               entry->default_data = entry_rte_data_cfg->cfg.default_data;
            }

            if (entry->sram_address == PF_SRAM_INVALID_ADDRESS)
            {
               /* SRAM address has not been set by application
                * use address from SRAM frame
                */
               entry->sram_address = frame->sram_frame_address +
                                     frame->rte_data_offset +
                                     entry->iodata->data_offset;
            }

            LOG_DEBUG (
               PF_PPM_LOG,
               "MERA(%d): [%u,%u]: Interface: %s Start address: 0x%x Size: "
               "%d\n",
               __LINE__,
               entry->iodata->slot_nbr,
               entry->iodata->subslot_nbr,
               pf_mera_interface_type_to_str (entry->io_interface_type),
               entry->io_interface_type == MERA_IO_INTF_QSPI
                  ? entry->qspi_address
                  : entry->sram_address,
               entry->iodata->data_length);
         }
      }
   }

   return ret;
}

/**
 * Configure and start a RTE configuration for a CPM frame.
 * Sets the general RTE configuration and the configuration
 * for each iodata object in the communication relationship.
 * @param handle     In: Ref to mera instance
 * @param frame      In: Ref to frame
 * @return 0 on success, -1 on error
 */
static int pf_mera_cpm_start_rte (pf_mera_t * handle, pf_mera_frame_t * frame)
{
   int err = 0;
   pf_iocr_t * p_iocr = frame->config.p_iocr;
   uint16_t slot, subslot;
   mera_ob_wal_conf_t wal_conf;
   mera_ob_rtp_state_t state;

   err = ob_add_vcam_rule (
      frame->vcam_id,
      p_iocr->param.iocr_tag_header.vlan_id,
      &frame->config.main_mac_addr,
      frame->vlan_idx,
      p_iocr->param.frame_id,
      frame->rtp_id);

   if (err != 0)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed to add outbound vcam rule\n",
         __LINE__);
      return err;
   }

   err = pf_mera_ob_rtp_config (handle->mera_lib, frame);
   if (err != 0)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed to set outbound RTP configuration\n",
         __LINE__);
      return err;
   }

   for (slot = 0; slot < PNET_MAX_SLOTS; slot++)
   {
      for (subslot = 0; subslot < PNET_MAX_SUBSLOTS; subslot++)
      {
         if (
            frame->rte_object[slot][subslot].iodata != NULL &&
            frame->rte_object[slot][subslot].iodata->data_length > 0)
         {
            if (
               pf_mera_add_ob_entry (
                  handle->mera_lib,
                  frame,
                  &frame->rte_object[slot][subslot]) != 0)
            {
               return -1;
            }
         }
      }
   }

   /* Add WAL to trigger reset to default value on DHT watchdog timeout. */
   mera_ob_wal_conf_get (handle->mera_lib, frame->ob_wal_id, &wal_conf);
   wal_conf.time.offset = 10000;
   wal_conf.time.interval = frame->config.p_iocr->cpm.control_interval * 1000;

   if (mera_ob_wal_conf_set (handle->mera_lib, frame->ob_wal_id, &wal_conf) != 0)
   {
      LOG_ERROR (
         PF_MERA_LOG,
         "MERA(%d): Failed to set outbound RTP WAL \n",
         __LINE__);
   }

   /* Activate rtp id. Needed if DHT has expired in previous connection. */
   state.active = true;
   if (mera_ob_rtp_state_set (handle->mera_lib, frame->rtp_id, &state) != 0)
   {
      LOG_ERROR (PF_MERA_LOG, "MERA(%d): Failed to set RTP state \n", __LINE__);
   }

   frame->active = true;

   if (
      mera_event_enable (
         handle->mera_lib,
         (MERA_EVENT_RTP_STATE_STOPPED | MERA_EVENT_PN_DS_MISMATCH |
          MERA_EVENT_DG_INVALID),
         true) != 0)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed to enable outbound events\n",
         __LINE__);
      return -1;
   }

   return 0;
}

pf_drv_frame_t * pf_mera_cpm_alloc (
   pnet_t * net,
   const pf_mera_frame_cfg_t * cfg)
{
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   pf_mera_frame_t * frame = NULL;
   pf_iodata_object_t * p_iodata;
   pf_rte_object_t * entry;
   uint16_t i;

   if (cfg == NULL || cfg->p_iocr == NULL || handle == NULL)
   {
      return NULL;
   }

   frame = pf_mera_alloc_frame (handle);
   if (frame == NULL)
   {
      return NULL;
   }

   frame->is_cpm = true;

   frame->sram_frame_address = pf_sram_frame_alloc();
   if (frame->sram_frame_address == 0)
   {
      LOG_FATAL (
         PF_MERA_LOG,
         "MERA(%d): Failed to alloc SRAM for CPM frame\n",
         __LINE__);
      pf_mera_free_frame (handle, frame);
      return NULL;
   }

   LOG_DEBUG (
      PF_MERA_LOG,
      "MERA(%d): Allocated CPM SRAM frame at 0x%x\n",
      __LINE__,
      frame->sram_frame_address);

   frame->config = *cfg;
   frame->port = cfg->port;

   for (i = 0; i < frame->config.p_iocr->nbr_data_desc; i++)
   {
      p_iodata = &frame->config.p_iocr->data_desc[i];
      if (p_iodata->data_length > 0)
      {
         entry =
            get_rte_object (frame, p_iodata->slot_nbr, p_iodata->subslot_nbr);
         entry->iodata = p_iodata;
      }
   }

   frame->vcam_id = pf_mera_vcam_id_alloc (handle);
   frame->vlan_idx = pf_mera_vlan_index_alloc (handle);
   frame->rtp_id = pf_mera_rtp_id_alloc (handle);
   frame->ob_wal_id = pf_mera_wal_id_alloc (handle);
   frame->rte_data_offset = PF_MERA_CPM_IODATA_OFFSET;

   LOG_DEBUG (PF_CPM_LOG, "MERA(%d): CPM frame allocated\n", __LINE__);

   return (pf_drv_frame_t *)frame;
}

int pf_mera_cpm_start (pnet_t * net, pf_drv_frame_t * drv_frame)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;

   if (handle == NULL || frame == NULL || frame->config.p_iocr == NULL)
   {
      LOG_DEBUG (
         PF_CPM_LOG,
         "MERA(%d): Failed to start RTE - parameter error\n",
         __LINE__);
      return -1;
   }

   if (pf_mera_cpm_config (handle, frame) != 0)
   {
      LOG_DEBUG (
         PF_CPM_LOG,
         "MERA(%d): Failed to start RTE - data config error\n",
         __LINE__);
      return -1;
   }

   return pf_mera_cpm_start_rte (handle, frame);
}

int pf_mera_cpm_activate_rte_dht (pf_drv_frame_t * drv_frame)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   mera_ob_rtp_conf_t ob_rtp_config;

   if (mera_ob_rtp_conf_get (frame->mera_lib, frame->rtp_id, &ob_rtp_config) != 0)
   {
      LOG_ERROR (PF_CPM_LOG, "MERA(%d): Failed get CPM RTE config\n", __LINE__);
      return -1;
   }

   /* Configure CPM data hold timeout.
    * RTP and group will stop when configured timeout expires.
    * Default values will be applied to all DGs on
    * writes triggered when group is stopped.
    */

   ob_rtp_config.time.offset = MERA_TIME_OFFSET_NONE;
   ob_rtp_config.time.interval =
      frame->config.p_iocr->cpm.control_interval * 1000;
   ob_rtp_config.time_cnt = frame->config.p_iocr->cpm.data_hold_factor;

   LOG_DEBUG (
      PF_CPM_LOG,
      "MERA(%d): Reconfigure ob rtp dht timing =%uns cnt=%d\n",
      __LINE__,
      ob_rtp_config.time.interval,
      ob_rtp_config.time_cnt);

   if (mera_ob_rtp_conf_set (frame->mera_lib, frame->rtp_id, &ob_rtp_config) != 0)
   {
      LOG_ERROR (PF_CPM_LOG, "MERA(%d): Failed set CPM RTE config\n", __LINE__);
      return -1;
   }

   return 0;
}

int pf_mera_cpm_free (pnet_t * net, pf_drv_frame_t * drv_frame)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   LOG_DEBUG (PF_MERA_LOG, "MERA(%d): Free CPM frame\n", __LINE__);
   if (frame != NULL)
   {
      pf_mera_vcam_id_free (handle, frame->vcam_id);
      pf_mera_vlan_index_free (handle, frame->vlan_idx);
      pf_mera_rtp_id_free (handle, frame->rtp_id);
      pf_mera_wal_id_free (handle, frame->ob_wal_id);
      pf_sram_frame_free (frame->sram_frame_address);
      pf_mera_free_frame (handle, frame);
      return 0;
   }
   LOG_ERROR (PF_MERA_LOG, "MERA(%d): Failed to free CPM frame\n", __LINE__);
   return -1;
}

int pf_mera_cpm_stop (pnet_t * net, pf_drv_frame_t * drv_frame)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;

   LOG_DEBUG (PF_PPM_LOG, "MERA(%d): Stop CPM\n", __LINE__);

   mera_ob_wal_rel (handle->mera_lib, frame->ob_wal_id);

   if (mera_ob_flush (handle->mera_lib) != 0)
   {
      LOG_ERROR (PF_CPM_LOG, "MERA(%d): Failed to flush rtp\n", __LINE__);
   }
   if (pf_mera_delete_vcam_rule (frame->vcam_id) != 0)
   {
      LOG_ERROR (PF_CPM_LOG, "MERA(%d): Failed to delete VCAM rule\n", __LINE__);
   }

   /* Stop periodic debug info */
   pf_scheduler_remove_if_running (net, &handle->sched_handle);
   return 0;
}

/**
 * Disable RTE write operations for an iodata object.
 * Intended to be used while reading SRAM data groups.
 * @param mera    InOut: Ref to mera library instance
 * @param frame   In: CPM frame
 * @param entry   In: Iodata object configuration
 */
static void pf_mera_disable_ob_entry (
   struct mera_inst * mera,
   pf_mera_frame_t * frame,
   pf_rte_object_t * entry)
{
   mera_ob_dg_ctrl_t dg_ctrl;

   dg_ctrl.enable = false;

   if (mera_ob_dg_ctrl_set (mera, frame->rtp_id, entry->ob_dg_id, &dg_ctrl) != 0)
   {
      LOG_ERROR (
         PF_MERA_LOG,
         "MERA(%d): Failed to disable outbound data group\n",
         __LINE__);
   }
}

/**
 * Enable RTE write operations for an iodata object.
 * @param mera    InOut: Ref to mera library instance
 * @param frame   In: CPM frame
 * @param entry   In: Iodata object configuration
 */
static void pf_mera_enable_ob_entry (
   struct mera_inst * mera,
   pf_mera_frame_t * frame,
   pf_rte_object_t * entry)
{
   mera_ob_dg_ctrl_t dg_ctrl;

   dg_ctrl.enable = true;

   if (mera_ob_dg_ctrl_set (mera, frame->rtp_id, entry->ob_dg_id, &dg_ctrl) != 0)
   {
      LOG_ERROR (
         PF_MERA_LOG,
         "MERA(%d): Failed to enable outbound data group\n",
         __LINE__);
   }
}

int pf_mera_cpm_read_data_and_iops (
   pnet_t * net,
   pf_drv_frame_t * drv_frame,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   bool * p_new_flag,
   uint8_t * p_data,
   uint16_t data_len,
   uint8_t * p_iops,
   uint8_t iops_len)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   mera_ob_dg_status_t status;
   pf_rte_object_t * entry;

   *p_iops = PNET_IOXS_BAD;

   if (!frame->active)
   {
      return -1;
   }

   if (slot_nbr == PNET_SLOT_DAP_IDENT)
   {
      return 0;
   }

   if (data_len == 0 || iops_len == 0)
   {
      return 0;
   }

   entry = get_rte_object (frame, slot_nbr, subslot_nbr);

   switch (entry->io_interface_type)
   {
   case MERA_IO_INTF_SRAM:
   case MERA_IO_INTF_QSPI:
      if (
         mera_ob_dg_status_get (
            handle->mera_lib,
            frame->rtp_id,
            entry->ob_dg_id,
            &status) != 0)
      {
         LOG_ERROR (
            PF_MERA_LOG,
            "MERA(%d): [%u,%u] Failed to read data group status "
            "(output)\n",
            __LINE__,
            slot_nbr,
            subslot_nbr);
         return -1;
      }

      if (status.valid_chk)
      {
         LOG_DEBUG (
            PF_MERA_LOG,
            "MERA(%d): [%u,%u] IOPS BAD, Invalid output data\n",
            __LINE__,
            slot_nbr,
            subslot_nbr);
      }
      else
      {
         *p_iops = PNET_IOXS_GOOD;
      }

      pf_mera_disable_ob_entry (handle->mera_lib, frame, entry);

      if (pf_sram_read (entry->sram_address, p_data, data_len) != 0)
      {
         pf_mera_enable_ob_entry (handle->mera_lib, frame, entry);

         LOG_ERROR (
            PF_MERA_LOG,
            "MERA(%d): [%u,%u] Failed to read SRAM (output) \n",
            __LINE__,
            slot_nbr,
            subslot_nbr);
         return -1;
      }

      pf_mera_enable_ob_entry (handle->mera_lib, frame, entry);

      return 0;
      break;

   default:
      LOG_ERROR (
         PF_MERA_LOG,
         "MERA(%d): [%u,%u] unsupported interface type\n",
         __LINE__,
         slot_nbr,
         subslot_nbr);
      return -1;
   }
   return 0;
}

int pf_mera_cpm_read_iocs (
   pnet_t * net,
   pf_drv_frame_t * frame,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint8_t * p_iocs,
   uint8_t iocs_len)
{
   /* TODO - Figure out how this can be read from frame memory */
   /* this is implemented in pf_mera_cpm_read_data_and_iops.
    review this and figure out best implementation */
   if (iocs_len > 0)
   {
      *p_iocs = PNET_IOXS_GOOD;
   }
   return 0;
}

int pf_mera_poll_cpm_events (
   pnet_t * net,
   pf_drv_frame_t * frame,
   pf_mera_event_t * evt)
{
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   mera_event_t mera_event;

   if (net == NULL || frame == NULL || evt == NULL)
   {
      return 0;
   }

   memset (evt, 0, sizeof (pf_mera_event_t));

   if (mera_event_poll (handle->mera_lib, (mera_event_t *)&mera_event) == 0)
   {
      if ((MERA_EVENT_RTP_STATE_STOPPED & mera_event) != 0)
      {
         LOG_INFO (PF_MERA_LOG, "MERA(%d): CPM RTP stopped event\n", __LINE__);
         evt->stopped = true;
      }
      if ((MERA_EVENT_PN_DS_MISMATCH & mera_event) != 0)
      {
         evt->data_status_mismatch = true;
      }
      if ((MERA_EVENT_DG_INVALID & mera_event) != 0)
      {
         evt->invalid_dg = true;
      }
      return 0;
   }
   return -1;
}

int pf_mera_cpm_read_data_status (
   pf_drv_frame_t * drv_frame,
   uint8_t * p_data_status)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   mera_ob_rtp_status_t status;

   if (mera_ob_rtp_status_get (frame->mera_lib, frame->rtp_id, &status) == 0)
   {
      if (status.pn_ds_chk == 1)
      {
         /* RTE has a detected a data status mismatch.
            Report RTE data status
          */
         *p_data_status = status.pn_ds;
      }
      else
      {
         /* Data status ok - report checked data status */
         *p_data_status = PF_MERA_DATA_STATUS_MASK;
      }

      return 0;
   }

   return -1;
}

int pf_mera_cpm_read_rx_count (pf_drv_frame_t * drv_frame, uint64_t * rx_count)
{
   pf_mera_frame_t * frame = (pf_mera_frame_t *)drv_frame;
   mera_ob_rtp_counters_t counters;

   if (mera_ob_rtp_counters_get (frame->mera_lib, frame->rtp_id, &counters) == 0)
   {
      if (counters.rx_0 >= frame->rx_count)
      {
         *rx_count = counters.rx_0 - frame->rx_count;
      }
      else
      {
         /* Wrap around */
         *rx_count = counters.rx_0 + (UINT64_MAX - counters.rx_0);
      }
      frame->rx_count = counters.rx_0;
      return 0;
   }

   *rx_count = 0;
   return -1;
}

void pf_mera_show (pnet_t * net, bool cpm, bool ppm)
{
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   mera_debug_info_t info;

   info.group = MERA_DEBUG_GROUP_GEN;
   info.rtp_id = 0;   /* All RTP IDs */
   info.full = false; /* Only active IDs */
   info.clear = true;

   if (cpm && ppm)
   {
      info.group = MERA_DEBUG_GROUP_ALL;
   }
   else if (cpm)
   {
      info.group = MERA_DEBUG_GROUP_OB;
   }
   else if (ppm)
   {
      info.group = MERA_DEBUG_GROUP_IB;
   }

   printf ("RTE Status:\n");
   mera_debug_info_print (handle->mera_lib, pf_mera_printf_cb, &info);
}

int pf_mera_get_port_from_remote_mac (
   pnet_ethaddr_t * mac_addr,
   uint16_t * p_port)
{
   int ret = -1;
   char cmd[128] = {0};
   char result[100] = {0};
   FILE * fd = NULL;
   char mac_addr_str[PF_MERA_SCRIPT_ARG_LEN] = {0};

   *p_port = 0;

   snprintf (
      mac_addr_str,
      sizeof (mac_addr_str),
      "%02x:%02x:%02x:%02x:%02x:%02x",
      mac_addr->addr[0],
      mac_addr->addr[1],
      mac_addr->addr[2],
      mac_addr->addr[3],
      mac_addr->addr[4],
      mac_addr->addr[5]);

   snprintf (
      cmd,
      sizeof (cmd),
      "brctl showmacs br0 | grep %s | xargs | cut -c1-1",
      mac_addr_str);

   LOG_DEBUG (
      PF_CPM_LOG,
      "MERA(%d): Use brctl command to get active port: \"%s\"\n",
      __LINE__,
      cmd);

   fd = popen (cmd, "r");
   if (fd != NULL)
   {
      if (fgets (result, sizeof (result), fd) != NULL)
      {
         *p_port = atoi (result);
         *p_port = *p_port - 1;

         LOG_DEBUG (
            PF_CPM_LOG,
            "MERA(%d): Found mac on bridge interface %u, using port index "
            "%u\n",
            __LINE__,
            *p_port + 1,
            *p_port);
         ret = 0;
      }
      else
      {
         LOG_FATAL (
            PF_CPM_LOG,
            "MERA(%d): Failed to find mac on bridge, using port index %u\n",
            __LINE__,
            *p_port);
      }
      pclose (fd);
   }

   return ret;
}

/* Public function in P-Net API
 * For iodata objects not mapped to QSPI this operation
 * is not used. Instead the iodata is automatically mapped
 * to SRAM by the P-Net stack (this file).
 *
 * This config should be part of frame, but there is
 * no suitable application event at which the configuration
 * can be passed when CPM/PPM are created but not yet started.
 */
int pnet_mera_input_set_rte_config (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   const pnet_mera_rte_data_cfg_t * rte_data_cfg)
{
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;
   LOG_DEBUG (
      PF_PPM_LOG,
      "MERA(%d): [%u,%u] Adding RTE configuration "
      "%s at address 0x%x (inbound / iodata input)\n",
      __LINE__,
      slot,
      subslot,
      pf_mera_rte_data_type_to_str (rte_data_cfg->type),
      rte_data_cfg->address);

   handle->rte_data_cfg[slot][get_subslot_index (subslot)].valid = true;
   handle->rte_data_cfg[slot][get_subslot_index (subslot)].cfg = *rte_data_cfg;

   return 0;
}

int pnet_mera_output_set_rte_config (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   const pnet_mera_rte_data_cfg_t * rte_data_cfg)
{
   pf_mera_t * handle = (pf_mera_t *)net->hwo_drv;

   LOG_DEBUG (
      PF_CPM_LOG,
      "MERA(%d): [%u,%u] Adding RTE configuration "
      "%s at address 0x%x (outbound / iodata output)\n",
      __LINE__,
      slot,
      subslot,
      pf_mera_rte_data_type_to_str (rte_data_cfg->type),
      rte_data_cfg->address);

   handle->rte_data_cfg[slot][get_subslot_index (subslot)].valid = true;
   handle->rte_data_cfg[slot][get_subslot_index (subslot)].cfg = *rte_data_cfg;

   return 0;
}
