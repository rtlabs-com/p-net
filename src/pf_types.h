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
 * @brief Internal typedefs for pnet
 */

#ifndef PF_TYPES_H
#define PF_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

#if PNET_USE_ATOMICS
#include <stdatomic.h>
#else
#define atomic_int            uint32_t
#define ATOMIC_VAR_INIT(x)    x

#ifdef atomic_fetch_add
#undef atomic_fetch_add
#endif
static inline uint32_t atomic_fetch_add(atomic_int *p, uint32_t v)
{
   uint32_t prev = *p;
   *p += v;

   return prev;
}
#ifdef atomic_fetch_sub
#undef atomic_fetch_sub
#endif
static inline uint32_t atomic_fetch_sub(atomic_int *p, uint32_t v)
{
   uint32_t prev = *p;
   *p -= v;

   return prev;
}
#endif

/*********************** RPC header ******************************************/

/** Magic UUID values */
#define PNET_UUID_NIL_OBJECT              "00000000-0000-0000-0000-000000000000"

#define PNET_RPC_DATA_REPRESENTATION      "8A885D04-1CEB-11C9-9FE8-08002B104860" /* Used by the EPMAP service */
#define PNET_UUID_EPMAP_INTERFACE         "E1AF8308-5D1F-11C9-91A4-08002B14A0FA" /* RPC interface version 3.0 */
#define PNET_UUID_EPMAP_OBJECT            "00000000-0000-0000-0000-000000000000" /* RPC interface version 3.0 */

#define PROFINET_IO_CONSTANT_VALUE        "DEA00000-6C97-11D1-8271"
#define PNET_UUID_IO_DEVICE_INTERFACE     "DEA00001-6C97-11D1-8271-00A02442DF7D" /* RPC interface version 1.0 */
#define PNET_UUID_IO_CONTROLLER_INTERFACE "DEA00002-6C97-11D1-8271-00A02442DF7D" /* RPC interface version 1.0 */
#define PNET_UUID_IO_SUPERVISOR_INTERFACE "DEA00003-6C97-11D1-8271-00A02442DF7D" /* RPC interface version 1.0 */
#define PNET_UUID_IO_PRM_SERVER_INTERFACE "DEA00004-6C97-11D1-8271-00A02442DF7D" /* RPC interface version 1.0 */

typedef enum pf_rpc_packet_type_values
{
   PF_RPC_PT_REQUEST = 0,
   PF_RPC_PT_PING,
   PF_RPC_PT_RESPONSE,
   PF_RPC_PT_FAULT,
   PF_RPC_PT_WORKING,
   PF_RPC_PT_RESP_PING,                   /* No Call - response to ping */
   PF_RPC_PT_REJECT,
   PF_RPC_PT_ACK,
   PF_RPC_PT_CL_CANCEL,
   PF_RPC_PT_FRAG_ACK,
   PF_RPC_PT_CANCEL_ACK,
} pf_rpc_packet_type_values_t;

typedef enum pf_rpc_flags_bits
{
   PF_RPC_F_RESERVED_0 = 0,
   PF_RPC_F_LAST_FRAGMENT,
   PF_RPC_F_FRAGMENT,
   PF_RPC_F_NO_FACK,
   PF_RPC_F_MAYBE,
   PF_RPC_F_IDEMPOTENT,
   PF_RPC_F_BROADCAST,
   PF_RPC_F_RESERVED_7,
} pf_rpc_flags_bits_t;

typedef enum pf_rpc_flags2_bits
{
   PF_RPC_F2_CANCEL_PENDING = 1,
} pf_rpc_flags2_bits_t;

typedef enum pf_dev_opnum_values
{
   PF_RPC_DEV_OPNUM_CONNECT = 0,
   PF_RPC_DEV_OPNUM_RELEASE,
   PF_RPC_DEV_OPNUM_READ,
   PF_RPC_DEV_OPNUM_WRITE,
   PF_RPC_DEV_OPNUM_CONTROL,
   PF_RPC_DEV_OPNUM_READ_IMPLICIT,
} pf_dev_opnum_values_t;

/* ToDo: The EPM is not yet implemented */
typedef enum pf_epmapper_opnum_values
{
   PF_RPC_EPM_OPNUM_INSERT,
   PF_RPC_EPM_OPNUM_DELETE,
   PF_RPC_EPM_OPNUM_LOOKUP,               /* Mandatory to support */
   PF_RPC_EPM_OPNUM_MAP,
   PF_RPC_EPM_OPNUM_LOOKUP_HANDLE_FREE,   /* Mandatory */
   PF_RPC_EPM_OPNUM_INP_OBJECT,
   PF_RPC_EPM_OPNUM_MGMT_DELETE,
} pf_epmapper_opnum_values_t;

typedef struct pf_rpc_flags
{
   bool              last_fragment;
   bool              fragment;
   bool              no_fack;
   bool              maybe;
   bool              idempotent;
   bool              broadcast;
} pf_rpc_flags_t;

typedef struct pf_rpc_flags2_t
{
   bool              cancel_pending;
} pf_rpc_flags2_t;

typedef struct pf_uuid
{
   uint32_t                data1;
   uint16_t                data2;
   uint16_t                data3;
   uint8_t                 data4[8];
} pf_uuid_t;

typedef struct pf_rpc_header
{
   uint8_t           version;          /* Allowed: 0x04 */
   uint8_t           packet_type;      /* rpc_packet_type_values_t */
   pf_rpc_flags_t    flags;            /* rpc_flags_bits */
   pf_rpc_flags2_t   flags2;
   bool              is_big_endian;
   uint8_t           float_repr;       /* Allowed: IEEE (0) */
   uint8_t           reserved;
   uint8_t           serial_high;
   pf_uuid_t         object_uuid;      /* Identifies IO device, IO controller or IO supervisor */
   pf_uuid_t         interface_uuid;   /* Identify PNIO interface */
   pf_uuid_t         activity_uuid;
   uint32_t          server_boot_time;
   uint32_t          interface_version;
   uint32_t          sequence_nmb;
   uint16_t          opnum;            /* Dev: pf_dev_opnum_values_t/pf_epmapper_opnum_values */
   uint16_t          interface_hint;
   uint16_t          activity_hint;
   uint16_t          length_of_body;   /* AKA fragment_length */
   uint16_t          fragment_nmb;
   uint8_t           auth_protocol;    /* Allowed: 0 (zero) */
   uint8_t           serial_low;
} pf_rpc_header_t;


/************************** Block header *************************************/

typedef enum pf_block_type_values
{
   /* Reserved 0x0000 */
   PF_BT_ALARM_NOTIFICATION_HIGH       = 0x0001,
   PF_BT_ALARM_NOTIFICATION_LOW        = 0x0002,
   /* Reserved 0x0003.. 0x0007 */
   PF_BT_IOD_WRITE_REQ_HEADER          = 0x0008,
   PF_BT_IOD_READ_REQ_HEADER           = 0x0009,
   /* Reserved 0x000A..0x000F */
   PF_BT_DIAGNOSIS_DATA                = 0x0010,
   /* Reserved 0x0011 */
   PF_BT_EXPECTED_IDENTIFICATION_DATA  = 0x0012,
   PF_BT_REAL_IDENTIFICATION_DATA      = 0x0013,
   PF_BT_SUBSTITUTE_VALUE              = 0x0014,
   PF_BT_RECORD_INPUT_DATA_OBJECT      = 0x0015,
   PF_BT_RECORD_OUTPUT_DATA_OBJECT     = 0x0016,

   PF_BT_AR_DATA                       = 0x0018,
   PF_BT_LOG_BOOK_DATA                 = 0x0019,
   PF_BT_API_DATA                      = 0x001A,
   PF_BT_SRL_DATA                      = 0x001B,

   PF_BT_IM_0                          = 0x0020,
   PF_BT_IM_1                          = 0x0021,
   PF_BT_IM_2                          = 0x0022,
   PF_BT_IM_3                          = 0x0023,
   PF_BT_IM_4                          = 0x0024,
   PF_BT_IM_5                          = 0x0025,
   PF_BT_IM_6                          = 0x0026,
   PF_BT_IM_7                          = 0x0027,
   PF_BT_IM_8                          = 0x0028,
   PF_BT_IM_9                          = 0x0029,
   PF_BT_IM_10                         = 0x002a,
   PF_BT_IM_11                         = 0x002b,
   PF_BT_IM_12                         = 0x002c,
   PF_BT_IM_13                         = 0x002d,
   PF_BT_IM_14                         = 0x002e,
   PF_BT_IM_15                         = 0x002f,

   PF_BT_IM_0_FILTER_DATA_SUBMODULE    = 0x0030,
   PF_BT_IM_0_FILTER_DATA_MODULE       = 0x0031,
   PF_BT_IM_0_FILTER_DATA_DEVICE       = 0x0032,

   PF_BT_IM_5_DATA                     = 0x0034,

   PF_BT_AR_BLOCK_REQ                  = 0x0101,
   PF_BT_IOCR_BLOCK_REQ                = 0x0102,
   PF_BT_ALARM_CR_BLOCK_REQ            = 0x0103,
   PF_BT_EXPECTED_SUBMODULE_BLOCK      = 0x0104,
   PF_BT_PRM_SERVER_REQ                = 0x0105,
   PF_BT_MCR_REQ                       = 0x0106,
   PF_BT_RPC_SERVER_REQ                = 0x0107,
   PF_BT_AR_VENDOR_BLOCK_REQ           = 0x0108,
   PF_BT_IR_INFO_BLOCK_REQ             = 0x0109,
   PF_BT_SR_INFO_BLOCK_REQ             = 0x010A,
   PF_BT_AR_FSU_BLOCK_REQ              = 0x010B,
   PF_BT_RS_INFO_BLOCK_REQ             = 0x010C,
   /* Reserved 0x010D-0x0x010F */
   PF_BT_PRMEND_REQ                    = 0x0110,
   PF_BT_PRMEND_PLUG_ALARM_REQ         = 0x0111,
   PF_BT_APPRDY_REQ                    = 0x0112,
   PF_BT_APPRDY_PLUG_ALARM_REQ         = 0x0113,
   PF_BT_RELEASE_BLOCK_REQ             = 0x0114,
   /* Reserved 0x0115 */
   PF_BT_XCONTROL_RDYCOMP_REQ          = 0x0116,
   PF_BT_XCONTROL_RDYRTC3_REQ          = 0x0117,
   PF_BT_PRMBEGIN_REQ                  = 0x0118,
   PF_BT_SUBMODULE_PRMBEGIN_REQ        = 0x0119,

   PF_BT_MAINTENANCE_ITEM              = 0x0f00,

   /* Output from a PROFINET device */
   /* Reserved 0x8000 */
   PF_BT_ALARM_ACK_HIGH                = 0x8001,
   PF_BT_ALARM_ACK_LOW                 = 0x8002,
   PF_BT_WRITE_RES                     = 0x8008,
   PF_BT_READ_RES                      = 0x8009,
   /* Reserved 0x8010.. 0x8100 */
   PF_BT_AR_BLOCK_RES                  = 0x8101,
   PF_BT_IOCR_BLOCK_RES                = 0x8102,
   PF_BT_ALARM_CR_BLOCK_RES            = 0x8103,
   PF_BT_MODULE_DIFF_BLOCK             = 0x8104,
   PF_BT_PRM_SERVER_BLOCK_RES          = 0x8105,
   PF_BT_AR_SERVER_BLOCK               = 0x8106,
   PF_BT_AR_RPC_BLOCK_RES              = 0x8107,
   PF_BT_AR_VENDOR_BLOCK_RES           = 0x8108,
   /* Reserved 0x8109-0x0x810F */
   PF_BT_PRMEND_RES                    = 0x8110,
   PF_BT_PRMEND_PLUG_ALARM_RES         = 0x8111,
   PF_BT_APPRDY_RES                    = 0x8112,
   PF_BT_APPRDY_PLUG_ALARM_RES         = 0x8113,
   PF_BT_RELEASE_BLOCK_RES             = 0x8114,
} pf_block_type_values_t;

typedef enum pf_index_values
{
   PF_IDX_USER_MAX                     = 0x7fff,

   PF_IDX_SUB_EXP_ID_DATA              = 0x8000,
   PF_IDX_SUB_REAL_ID_DATA             = 0x8001,

   PF_IDX_SUB_DIAGNOSIS_CH             = 0x800a,
   PF_IDX_SUB_DIAGNOSIS_ALL            = 0x800b,
   PF_IDX_SUB_DIAGNOSIS_DMQS           = 0x800c,
   PF_IDX_SUB_DIAG_MAINT_REQ_CH        = 0x8010,
   PF_IDX_SUB_DIAG_MAINT_DEM_CH        = 0x8011,
   PF_IDX_SUB_DIAG_MAINT_REQ_ALL       = 0x8012,
   PF_IDX_SUB_DIAG_MAINT_DEM_ALL       = 0x8013,

   PF_IDX_SUB_SUBSTITUTE               = 0x801e,

   PF_IDX_SUB_PDIR                     = 0x8020,

   PF_IDX_SUB_PDPORTDATAREALEXT        = 0x8027,
   PF_IDX_SUB_INPUT_DATA               = 0x8028,
   PF_IDX_SUB_OUTPUT_DATA              = 0x8029,
   PF_IDX_SUB_PDPORT_DATA_REAL         = 0x802a,
   PF_IDX_SUB_PDPORT_DATA_CHECK        = 0x802b,
   PF_IDX_SUB_PDIR_DATA                = 0x802c,
   PF_IDX_SUB_PDSYNC_ID0               = 0x802d,

   PF_IDX_SUB_PDPORT_DATA_ADJ          = 0x802f,
   PF_IDX_SUB_ISOCHRONUOUS_DATA        = 0x8030,
   PF_IDX_SUB_PDTIME                   = 0x8031,

   PF_IDX_SUB_PDINTF_MRP_REAL          = 0x8050,
   PF_IDX_SUB_PDINTF_MRP_CHECK         = 0x8051,
   PF_IDX_SUB_PDINTF_MRP_ADJUST        = 0x8052,
   PF_IDX_SUB_PDPORT_MRP_ADJUST        = 0x8053,
   PF_IDX_SUB_PDPORT_MRP_REAL          = 0x8054,
   PF_IDX_SUB_PDPORT_MRPIC_REAL        = 0x8055,
   PF_IDX_SUB_PDPORT_MRPIC_CHECK       = 0x8056,
   PF_IDX_SUB_PDPORT_MRPIC_ADJUST      = 0x8057,

   PF_IDX_SUB_PDPORT_FO_REAL           = 0x8060,
   PF_IDX_SUB_PDPORT_FO_CHECK          = 0x8061,
   PF_IDX_SUB_PDPORT_FO_ADJUST         = 0x8062,

   PF_IDX_SUB_PDNC_CHECK               = 0x8070,
   PF_IDX_SUB_PDINTF_ADJUST            = 0x8071,
   PF_IDX_SUB_PDPORT_STATISTIC         = 0x8072,

   PF_IDX_SUB_PDINTF_REAL              = 0x8080,

   PF_IDX_SUB_PDINTF_FSU_ADJUST        = 0x8090,

   PF_IDX_SUB_PE_ENTITY_STATUS         = 0x80af,
   PF_IDX_SUB_COMBINED_OBJ_CONTAINER   = 0x80b0,

   PF_IDX_SUB_RS_ADJUST_OBSERVER       = 0x80cf,

   PF_IDX_SUB_IM_0                     = 0xaff0,
   PF_IDX_SUB_IM_1                     = 0xaff1,
   PF_IDX_SUB_IM_2                     = 0xaff2,
   PF_IDX_SUB_IM_3                     = 0xaff3,
   PF_IDX_SUB_IM_4                     = 0xaff4,
   PF_IDX_SUB_IM_5                     = 0xaff5,
   PF_IDX_SUB_IM_6                     = 0xaff6,
   PF_IDX_SUB_IM_7                     = 0xaff7,
   PF_IDX_SUB_IM_8                     = 0xaff8,
   PF_IDX_SUB_IM_9                     = 0xaff9,
   PF_IDX_SUB_IM_10                    = 0xaffa,
   PF_IDX_SUB_IM_11                    = 0xaffb,
   PF_IDX_SUB_IM_12                    = 0xaffc,
   PF_IDX_SUB_IM_13                    = 0xaffd,
   PF_IDX_SUB_IM_14                    = 0xaffe,
   PF_IDX_SUB_IM_15                    = 0xafff,

   PF_IDX_SLOT_EXP_ID_DATA             = 0xc000,
   PF_IDX_SLOT_REAL_ID_DATA            = 0xc001,

   PF_IDX_SLOT_DIAGNOSIS_CH            = 0xc00a,
   PF_IDX_SLOT_DIAGNOSIS_ALL           = 0xc00b,
   PF_IDX_SLOT_DIAGNOSIS_DMQS          = 0xc00c,

   PF_IDX_SLOT_DIAG_MAINT_REQ_CH       = 0xc010,
   PF_IDX_SLOT_DIAG_MAINT_DEM_CH       = 0xc011,
   PF_IDX_SLOT_DIAG_MAINT_REQ_ALL      = 0xc012,
   PF_IDX_SLOT_DIAG_MAINT_DEM_ALL      = 0xc013,

   PF_IDX_AR_EXP_ID_DATA               = 0xe000,
   PF_IDX_AR_REAL_ID_DATA              = 0xe001,
   PF_IDX_AR_MOD_DIFF                  = 0xe002,

   PF_IDX_AR_DIAGNOSIS_CH              = 0xe00a,
   PF_IDX_AR_DIAGNOSIS_ALL             = 0xe00b,
   PF_IDX_AR_DIAGNOSIS_DMQS            = 0xe00c,

   PF_IDX_AR_DIAG_MAINT_REQ_CH         = 0xe010,
   PF_IDX_AR_DIAG_MAINT_DEM_CH         = 0xe011,
   PF_IDX_AR_DIAG_MAINT_REQ_ALL        = 0xe012,
   PF_IDX_AR_DIAG_MAINT_DEM_ALL        = 0xe013,

   PF_IDX_AR_PE_ENTITY_FILTER_DATA     = 0xe030,
   PF_IDX_AR_PE_ENTITY_STATUS_DATA     = 0xe031,

   PF_IDX_AR_WRITE_MULTIPLE            = 0xe040,

   PF_IDX_AR_FSU_DATA_ADJUST           = 0xe050,

   PF_IDX_AR_RS_GET_EVENT              = 0xe060,
   PF_IDX_AR_RS_ACK_EVENT              = 0xe061,

   PF_IDX_API_REAL_ID_DATA             = 0xf000,

   PF_IDX_API_DIAGNOSIS_CH             = 0xf00a,
   PF_IDX_API_DIAGNOSIS_ALL            = 0xf00b,
   PF_IDX_API_DIAGNOSIS_DMQS           = 0xf00c,

   PF_IDX_API_DIAG_MAINT_REQ_CH        = 0xf010,
   PF_IDX_API_DIAG_MAINT_DEM_CH        = 0xf011,
   PF_IDX_API_DIAG_MAINT_REQ_ALL       = 0xf012,
   PF_IDX_API_DIAG_MAINT_DEM_ALL       = 0xf013,

   PF_IDX_API_AR_DATA                  = 0xf020,

   PF_IDX_DEV_DIAGNOSIS_DMQS           = 0xf80c,

   PF_IDX_DEV_AR_DATA                  = 0xf820,
   PF_IDX_DEV_API_DATA                 = 0xf821,

   PF_IDX_DEV_LOGBOOK_DATA             = 0xf830,
   PF_IDX_DEV_PDEV_DATA                = 0xf831,

   PF_IDX_DEV_IM_0_FILTER_DATA         = 0xf840,
   PF_IDX_DEV_PDREAL_DATA              = 0xf841,
   PF_IDX_DEV_PDEXP_DATA               = 0xf842,

   PF_IDX_DEV_AUTO_CONFIGURATION       = 0xf850,
   PF_IDX_DEV_GSD_UPLOAD               = 0xf860,

   PF_IDX_DEV_PE_ENTITY_FILTER_DATA    = 0xf870,
   PF_IDX_DEV_PE_ENTITY_STATUS_DATA    = 0xf871,
   PF_IDX_DEV_ASSET_MANAGEMENT_1       = 0xf880,
   PF_IDX_DEV_ASSET_MANAGEMENT_2       = 0xf881,
   PF_IDX_DEV_ASSET_MANAGEMENT_3       = 0xf882,
   PF_IDX_DEV_ASSET_MANAGEMENT_4       = 0xf883,
   PF_IDX_DEV_ASSET_MANAGEMENT_5       = 0xf884,
   PF_IDX_DEV_ASSET_MANAGEMENT_6       = 0xf885,
   PF_IDX_DEV_ASSET_MANAGEMENT_7       = 0xf886,
   PF_IDX_DEV_ASSET_MANAGEMENT_8       = 0xf887,
   PF_IDX_DEV_ASSET_MANAGEMENT_9       = 0xf888,
   PF_IDX_DEV_ASSET_MANAGEMENT_10      = 0xf889,

   PF_IDX_DEV_CONN_MON_TRIGGER         = 0xfbff,
} pf_index_values_t;

CC_PACKED_BEGIN
typedef struct CC_PACKED pf_block_header
{
   uint16_t                block_type;             /** pf_block_type_values_t */
   uint16_t                block_length;
   uint8_t                 block_version_high;
   uint8_t                 block_version_low;
} pf_block_header_t;
CC_PACKED_END
/* The following line guarantees that offsetof works as expected on received data. */
CC_STATIC_ASSERT(sizeof(pf_block_header_t) == (2+2+1+1));

/* <============================ Block header */

typedef struct pf_ndr_array
{
   uint32_t                maximum_count;
   uint32_t                offset;
   uint32_t                actual_count;
} pf_ndr_array_t;

typedef struct pf_ndr_data
{
   uint32_t                args_maximum;
   uint32_t                args_length;
   pf_ndr_array_t          array;
} pf_ndr_data_t;

typedef enum pf_control_command_bit_positions
{
   PF_CONTROL_COMMAND_BIT_PRM_END = 0,
   PF_CONTROL_COMMAND_BIT_APP_RDY = 1,
   PF_CONTROL_COMMAND_BIT_RELEASE = 2,
   PF_CONTROL_COMMAND_BIT_DONE = 3,
   PF_CONTROL_COMMAND_BIT_RDY_FOR_COMPANION = 4,
   PF_CONTROL_COMMAND_BIT_RDY_FOR_RTC3 = 5,
   PF_CONTROL_COMMAND_BIT_PRM_BEGIN = 6,
} pf_control_command_bit_positions_t;

/** Used for both RPC control/release requests and responses */
typedef struct pf_control_block
{
   pf_uuid_t               ar_uuid;
   uint16_t                session_key;
   uint16_t                control_command;
   uint16_t                control_block_properties;
   uint16_t                alarm_sequence_number;
} pf_control_block_t;

typedef struct pf_ip_suite
{
   /* Order is important!! */
   os_ipaddr_t             ip_addr;
   os_ipaddr_t             ip_mask;
   os_ipaddr_t             ip_gateway;
} pf_ip_suite_t;

typedef struct pf_full_ip_suite
{
   /* Order is important!! */
   pf_ip_suite_t           ip_suite;
   os_ipaddr_t             dns_addr[4];
} pf_full_ip_suite_t;

/*
 * ============= im_0_filter_data typedefs ==================
 */
typedef struct pf_im_0_filter_data_sub
{
   uint16_t                subslot_number;               /** Allowed 0x0000.0x7FFF */
   uint32_t                submodule_ident_number;
} pf_im_0_filter_data_sub_t;

typedef struct pf_im_0_filter_data_mod
{
   uint32_t                module_ident_number;
   uint16_t                slot_number;                  /** Allowed 0x0000.0x7FFF */
   uint16_t                nbr_submodules;
   pf_im_0_filter_data_sub_t submodules[1];
} pf_im_0_filter_data_mod_t;

typedef struct pf_im_0_filter_data_api
{
   uint32_t                api;
   uint16_t                nbr_modules;
   pf_im_0_filter_data_mod_t modules[1];
} pf_im_0_filter_data_api_t;

typedef struct pf_im_0_filter_data
{
   uint16_t                nbr_apis;
   pf_im_0_filter_data_api_t apis[1];
} pf_im_0_filter_data_t;

/*
 * ============= Alarm typedefs ==================
 */

#define PF_ALARM_PDU_TYPE_VERSION_1       0x01

typedef enum pf_alarm_pdu_type_values
{
   PF_RTA_PDU_TYPE_RESERVED = 0,
   PF_RTA_PDU_TYPE_DATA = 1,
   PF_RTA_PDU_TYPE_NACK = 2,
   PF_RTA_PDU_TYPE_ACK = 3,
   PF_RTA_PDU_TYPE_ERR = 4,
   /* 0x05..0x0f Reserved */
} pf_alarm_pdu_type_values_t;

typedef enum pf_alarm_type_values
{
   PF_ALARM_TYPE_DIAGNOSIS = 0x0001,
   PF_ALARM_TYPE_PROCESS = 0x0002,
   PF_ALARM_TYPE_PULL = 0x0003,
   PF_ALARM_TYPE_PLUG = 0x0004,
   PF_ALARM_TYPE_STATUS = 0x0005,
   PF_ALARM_TYPE_UPDATE = 0x0006,
   PF_ALARM_TYPE_MEDIA_REDUNDANCY = 0x0007,
   PF_ALARM_TYPE_CONTROLLED_BY_SUPERVISOR = 0x0008,
   PF_ALARM_TYPE_RELEASED = 0x0009,
   PF_ALARM_TYPE_PLUG_WRONG_MODULE = 0x000a,
   PF_ALARM_TYPE_RETURN_OF_SUBMODULE = 0x000b,
   PF_ALARM_TYPE_DIAGNOSIS_DISAPPEARS = 0x000c,
   PF_ALARM_TYPE_MC_COMM_MISMATCH = 0x000d,
   PF_ALARM_TYPE_PORT_DATA_CHANGE = 0x000e,
   PF_ALARM_TYPE_SYNC_DATA_CHANGE = 0x000f,
   PF_ALARM_TYPE_ISOCHRONUOUS_MODE_PROBLEM = 0x0010,
   PF_ALARM_TYPE_NETWORK_COMPONENT_PROBLEM = 0x0011,
   PF_ALARM_TYPE_TIME_DATA_CHANGE = 0x0012,
   PF_ALARM_TYPE_DYNAMIC_FRAME_PACKING_PROBLEM = 0x0013,
   PF_ALARM_TYPE_MRPD_PROBLEM = 0x0014,
   /* Reserved 0x0015 */
   PF_ALARM_TYPE_MILTIPLE_INTERFACE_MISMATCH = 0x0016,
   /* Reserved 0x0017..0x001d */
   PF_ALARM_TYPE_UPLOAD_AND_RETRIVAL = 0x001e,
   PF_ALARM_TYPE_PULL_MODULE = 0x001f,
   /* Manufaturer specific 0x0020..0x007f */
   /* Reserved for profiles 0x0080..0x00ff */
   /* Reserved 0x0100.. 0xffff */
} pf_alarm_type_values_t;

typedef struct pf_alarm_pdu_type
{
   uint8_t                 type;          /* pf_alarm_pdu_type_values_t */
   uint8_t                 version;
} pf_alarm_pdu_type_t;

typedef struct pf_alarm_add_flags
{
   uint8_t                 window_size;
   bool                    tack;
} pf_alarm_add_flags_t;

typedef struct pf_alarm_fixed
{
   uint16_t                dst_ref;
   uint16_t                src_ref;
   pf_alarm_pdu_type_t     pdu_type;
   pf_alarm_add_flags_t    add_flags;
   uint16_t                send_seq_num;
   uint16_t                ack_seq_nbr;
   /*
    * pf_alarm_fixed_t is followed by:
    * uint16_t             var_part_len;  (0..1432)
    *                     [block_header (pf_alarm_data_t/pf_alarm_err_t)]
    */
} pf_alarm_fixed_t;

typedef struct pf_alarm_data
{
   uint16_t                alarm_type;          /* pf_alarm_type_values_t */
   uint32_t                api_id;
   uint16_t                slot_nbr;
   uint16_t                subslot_nbr;
   uint32_t                module_ident;
   uint16_t                submodule_ident;
   pnet_alarm_spec_t       alarm_specifier;
   uint16_t                sequence_number;
   /*
    * pf_alarm_data_t may be followed by alarm_payload:
    *    RTA-SDU = alarm_noftification_pdu | alarm_ack_pdu
    */
} pf_alarm_data_t;


typedef struct pf_alarm_ack
{
   uint16_t                alarm_type;          /* pf_alarm_type_values_t */
   uint32_t                api_id;
   uint16_t                slot_nbr;
   uint16_t                subslot_nbr;
   pnet_alarm_spec_t       alarm_specifier;
   pnet_pnio_status_t      pnio_status;
} pf_alarm_ack_t;

typedef struct pf_alarm_err
{
   pnet_pnio_status_t      pnio_status;
   /*
    * pf_alarm_err_t may be followed by alarm_payload:
    *    vendor_device_error_info = VendorId, DeviceId, data*
    */
} pf_alarm_err_t;

#define PF_MAX_SESSION                    (2*(PNET_MAX_AR) + 1)               /* 2 per ar, and one spare. */

/*
 * Keep this value small as it define the number of entries in the
 * frame id map and all entries are probed until a match is found.
 *
 * Each input CR may have 2 frameIds (for RTC3)
 * Add space for DCP:     0xfefc..0xfeff.
 * Add space for alarms:  0xfc01, 0xfe01.
 */
#define PF_ETH_MAX_MAP                    ((PNET_MAX_API) * (PNET_MAX_AR) * (PNET_MAX_CR) * 2 + 4 + 2)

/**
 * The scheduler is used by both the CPM and PPM machines.
 * The DCP uses the scheduler for responding to multi-cast messages.
 * pf_cmsm uses it to supervise the startup sequence.
 */
#define PF_MAX_TIMEOUTS                   (2 * (PNET_MAX_AR) * (PNET_MAX_CR) + 10)

#define PF_CMINA_FS_HELLO_RETRY           3
#define PF_CMINA_FS_HELLO_INTERVAL        (3*1000)     /* ms => 3s. Default is 30ms */

typedef enum pf_cmina_state_values
{
   PF_CMINA_STATE_SETUP,
   PF_CMINA_STATE_SET_NAME,
   PF_CMINA_STATE_SET_IP,
   PF_CMINA_STATE_W_CONNECT,
} pf_cmina_state_values_t;

typedef enum pf_cmsu_state_values
{
   PF_CMSU_STATE_IDLE = 0,
   PF_CMSU_STATE_STARTUP,
   PF_CMSU_STATE_RUN,
   PF_CMSU_STATE_ABORT
} pf_cmsu_state_values_t;

typedef enum pf_cmwrr_state_values
{
   PF_CMWRR_STATE_IDLE,
   PF_CMWRR_STATE_STARTUP,
   PF_CMWRR_STATE_PRMEND,
   PF_CMWRR_STATE_DATA,
} pf_cmwrr_state_values_t;


/**
 * The prototype of the externally supplied call-back functions.
 * @param net              InOut: The p-net stack instance
 * @param arg              In:   User-defined (may be NULL).
 * @param current_time     In:   The current system time.
 */
typedef void (*pf_scheduler_timeout_ftn_t)(
   pnet_t                     *net,
   void                       *arg,
   uint32_t                   current_time);


typedef struct pf_scheduler_timeouts
{
   const char                    *p_name; /* For debugging only */
   bool                          in_use;  /* For debugging only */

   uint32_t                      when;    /* absolute time of timeout */
   uint32_t                      next;    /* Next in list */
   uint32_t                      prev;    /* Previous in list */

   pf_scheduler_timeout_ftn_t    cb;      /* Call-back to call on timeout */
   void                          *arg;    /* call-back argument */
} pf_scheduler_timeouts_t;

/**
 * This is the prototype for the Profinet frame handler.
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:   The frame ID.
 * @param p_buf            In:   The Ethernet frame.
 * @param frame_id_pos     In:   The position of the frame ID.
 * @param p_arg            In:   User-defined argument.
 */
typedef int (*pf_eth_frame_handler_t)(
   pnet_t                   *net,
   uint16_t                frame_id,
   os_buf_t                *p_buf,
   uint16_t                frame_id_pos,
   void                    *p_arg);


typedef struct pf_eth_frame_id_map
{
   bool                    in_use;
   uint16_t                frame_id;
   pf_eth_frame_handler_t  frame_handler;
   void                    *p_arg;
} pf_eth_frame_id_map_t;


/*
 * Each struct in pf_cmina_dcp_ase_t is carefully laid out in order to use
 * strncmp/memcmp in the DCP identity request and strncpy/memcpy in the
 * get/set requests.
 */
typedef struct pf_cmina_dcp_ase
{
   char                    name_of_station[240 + 1];  /* Terminated */
   char                    device_vendor[20+1];       /* Terminated */
   uint8_t                 device_role;               /* Only value "1" supported */
   uint16_t                device_initiative;

   struct
   {
      /* Order is important!! */
      uint8_t              high;
      uint8_t              low;
   } instance_id;

   pnet_cfg_device_id_t    device_id;
   pnet_cfg_device_id_t    oem_device_id;

   char                    port_name[14 + 1];         /* Terminated */

   char                    manufacturer_specific_string[240 + 1];
   pnet_ethaddr_t          mac_address;
   uint16_t                standard_gw_value;

   bool                    dhcp_enable;
   pf_full_ip_suite_t      full_ip_suite;

   struct
   {
      /* Nothing much yet */
      char                 hostname[240+1];
   } dhcp_info;
} pf_cmina_dcp_ase_t;


/* ====== AR typedefs ============== */

typedef enum pf_ppm_state_values
{
   PF_PPM_STATE_W_START = 0,
   PF_PPM_STATE_RUN,
} pf_ppm_state_values_t;

typedef enum pf_cpm_state_values
{
   PF_CPM_STATE_W_START = 0,
   PF_CPM_STATE_FRUN,
   PF_CPM_STATE_RUN,
} pf_cpm_state_values_t;

typedef enum pf_parameter_server_values
{
   PF_PS_EXTERNAL_PARAMETER_SERVER,
   PF_PS_CM_INITIATOR
} pf_parameter_server_values_t;

typedef enum pf_ar_type_values
{
   PF_ART_IOCAR_SINGLE = 0x0001,
   PF_ART_IOSAR = 0x0006,                              /** Supervisor AR */
   PF_ART_IOCAR_SINGLE_RTC_3 = 0x0010,
   PF_ART_IOCAR_SR = 0x0020,                           /** Set of AR */
} pf_ar_type_values_t;

typedef enum pf_iocr_type_values
{
   PF_IOCR_TYPE_RESERVED_0,
   PF_IOCR_TYPE_INPUT,
   PF_IOCR_TYPE_OUTPUT,
   PF_IOCR_TYPE_MC_PROVIDER,
   PF_IOCR_TYPE_MC_CONSUMER,
} pf_iocr_type_values_t;

typedef enum pf_companion_ar_values
{
   PF_CAR_SINGLE_AR,
   PF_CAR_FIRST_COMPANION_AR,                            /** For future use */
   PF_CAR_SECOND_COMPANION_AR                            /** For future use */
} pf_companion_ar_values_t;

typedef enum pf_rt_class_values
{
   PF_RT_CLASS_1 = 1,                                    /** Legacy: FrameID range 7 */
   PF_RT_CLASS_2,                                        /** FrameID range 6 */
   PF_RT_CLASS_3,                                        /** FrameID range 3 */
   PF_RT_CLASS_UDP                                       /** FrameID range 7 */
} pf_rt_class_values_t;

typedef enum pf_protocol_values
{
   PF_PROTOCOL_DNS,
   PF_PROTOCOL_DCP
} pf_protocol_values_t;

typedef enum pf_priority_values
{
   PF_PRIORITY_USER,
   PF_PRIORITY_FIXED
} pf_priority_values_t;

typedef enum pf_data_direction_values
{
   PF_DIRECTION_INPUT = 1,
   PF_DIRECTION_OUTPUT = 2,
} pf_data_direction_values_t;

typedef enum pf_dev_status_type
{
   PF_DEV_STATUS_TYPE_IOCS,
   PF_DEV_STATUS_TYPE_IOPS,
} pf_dev_status_type_t;

typedef enum pf_end_point_1_values
{
   PF_EP1_NEITHER_LEFT_NOR_ABOVE,
   PF_EP1_LEFT_OR_ABOVE
} pf_end_point_1_values_t;

typedef enum pf_end_point_2_values
{
   PF_EP2_NEITHER_RIGHT_NOR_BELOW,
   PF_EP2_RIGHT_OR_BELOW
} pf_end_point_2_values_t;

typedef enum pf_rs_alarm_transport_mode_values
{
   PF_RSA_TRANSPORT_MODE_0 = 0,
   PF_RSA_TRANSPORT_MODE_1 = 1,
} pf_rs_alarm_transport_mode_values_t;

typedef enum pf_fs_parameter_mode_values
{
   PF_FS_PARAMETER_MODE_ON,
   PF_FS_PARAMETER_MODE__OFF
} pf_fs_parameter_mode_values_t;

typedef enum pf_ar_state_values
{
   PF_AR_STATE_PRIMARY,                                  /** default value */
   PF_AR_STATE_FIRST,
   PF_AR_STATE_BACKUP
} pf_ar_state_values_t;

typedef enum pf_sync_state_values
{
   PF_SYNC_STATE_SYNCHRONIZED,
   PF_SYNC_STATE_NOT_AVAILABLE
} pf_sync_state_values_t;

typedef enum pf_alarm_cr_type_values
{
   PF_ALARM_CR = 0x0001,
} pf_alarm_cr_type_values_t;

/** According to the spec these are the states used by CMDEV/CMDEV_DA.
 *
 * The states for the AR is stored in cmdev_state, within the AR.
 *
 * An AR can be controlled by either a CMDEV or a CMDEV_DA state machine.
 * This is selected by looking at ar_parameters.device_access.
 * A CMDEV_DA state machine has only the states POWER_ON, W_CIND, W_CRES and DATA.
 * A CMDEV state machine uses all states listed below.
 */
typedef enum pf_cmdev_state_values
{
   PF_CMDEV_STATE_POWER_ON = 0,           /* Data initialization. (Must be first) */
   PF_CMDEV_STATE_W_CIND,                 /* Wait for connect indcation. */
   PF_CMDEV_STATE_W_CRES,                 /* Wait for connect response from app and CMSU startup. */
   PF_CMDEV_STATE_W_SUCNF,                /* Wait for CMSU confirmation. */
   PF_CMDEV_STATE_W_PEIND,                /* Wait for PrmEnd indication. */
   PF_CMDEV_STATE_W_PERES,                /* Wait for PrmEnd response from app. */
   PF_CMDEV_STATE_W_ARDY,                 /* Wait for app ready from app. */
   PF_CMDEV_STATE_W_ARDYCNF,              /* Wait for app ready confirmation from controller. */
   PF_CMDEV_STATE_WDATA,                  /* Wait for established cyclic data exchange. */
   PF_CMDEV_STATE_DATA,                   /* Data exchange and connection monitoring. */
   PF_CMDEV_STATE_ABORT                   /* Abort application relation. */
} pf_cmdev_state_values_t;

/* ====================================================================== */

typedef struct pf_ar_properties
{
   uint16_t                state;                        /** Allowed values: ACTIVE (=1) */
   bool                    supervisor_takeover_allowed;
   uint8_t                 parameterization_server;      /** pf_parameter_server_values_t */
   bool                    device_access;                /** See IEC61158-6-10 */
   uint16_t                companion_ar;                 /** pf_companion_ar_values_t */
   bool                    acknowledge_companion_ar;     /** whether companion_ar is needed */
   bool                    combined_object_container;    /** See IEC61158-6-10 */
   bool                    startup_mode;                 /** false: legacy (do not use), true: advanced */
   bool                    pull_module_alarm_allowed;    /** false: mandatory support, true: optional */
} pf_ar_properties_t;

typedef struct pf_ar_param
{
   bool                    valid;
   uint16_t                ar_type;                      /** pf_ar_type_values_t */
   pf_uuid_t               ar_uuid;
   uint16_t                session_key;
   pnet_ethaddr_t          cm_initiator_mac_add;
   pf_uuid_t               cm_initiator_object_uuid;
   pf_ar_properties_t      ar_properties;
   uint16_t                cm_initiator_activity_timeout_factor;        /** res: 100ms */
   uint16_t                cm_initiator_udp_rt_port;                    /** 0x8892 */
   uint16_t                cm_initiator_station_name_len;
   char                    cm_initiator_station_name[240 + 1];          /** Terminated string. */
} pf_ar_param_t;

typedef struct pf_ar_result
{
   pnet_ethaddr_t          cm_responder_mac_add;
   uint16_t                responder_udp_rt_port;
} pf_ar_result_t;

typedef struct pf_iocr_properties
{
   uint32_t                rt_class;                     /** pf_rt_class_values_t */
   uint16_t                reserved_1;
   uint16_t                reserved_2;
   uint16_t                reserved_3;
} pf_iocr_properties_t;

typedef struct pf_frame_descriptor
{
   uint16_t                slot_number;
   uint16_t                subslot_number;
   uint16_t                frame_offset;
} pf_frame_descriptor_t;

typedef struct pf_api_entry
{
   uint32_t                api;
   uint16_t                nbr_io_data;
   pf_frame_descriptor_t   io_data[PNET_MAX_MODULES * PNET_MAX_SUBMODULES];
   uint16_t                nbr_iocs;
   pf_frame_descriptor_t   iocs[PNET_MAX_MODULES * PNET_MAX_SUBMODULES];
} pf_api_entry_t;

typedef struct pf_iocr_tag_header
{
   uint16_t                vlan_id;                      /** Bits 0..11. Default: 0 */
   uint16_t                iocr_user_priority;           /** Bits 12..15. Allowed: IOCR_PRIORITY = 6 */
} pf_iocr_tag_header_t;

typedef struct pf_iocr_param
{
   bool                    valid;
   uint16_t                iocr_type;                    /** pf_iocr_type_values_t */
   uint16_t                iocr_reference;               /** Cross-ref from e.g. mcr_request and ir_info */
   uint16_t                lt_field;                     /** Allowed: 0x8892 */
   pf_iocr_properties_t    iocr_properties;
   uint16_t                c_sdu_length;                 /** Allowed: RT_CLASS_UDP: 12..1440, others: 40..1440 */
   uint16_t                frame_id;                     /** Depends on RT_class */
   uint16_t                send_clock_factor;            /** res: 31.25us, Allowed: 1..128, default: 32 */
   uint16_t                reduction_ratio;
   uint16_t                phase;                        /** 1..reduction_ratio */
   uint16_t                sequence;                     /** Allowed: RT_class_3: 0, others: 0 (mandatory) 1..0xffff (optional) */
   uint32_t                frame_send_offset;            /** res: 1ns, Allowed: (All:) BEST_EFFORT (0xffffffff), RT_class_3: 0x0..0x3D08FF (mandatory) */
   uint32_t                watchdog_factor;
   uint16_t                data_hold_factor;             /** Allowed: 0x0000..0x1e00, more restrictions apply */

   pf_iocr_tag_header_t    iocr_tag_header;

   pnet_ethaddr_t          iocr_multicast_mac_add;       /** See IEC61158-6-10 */

   uint16_t                nbr_apis;
   pf_api_entry_t          apis[PNET_MAX_API];
} pf_iocr_param_t;

typedef struct pf_iocr_result
{
   uint16_t                iocr_type;                    /** pf_iocr_type_values_t */
   uint16_t                iocr_reference;
   uint16_t                frame_id;
} pf_iocr_result_t;

typedef struct pf_data_descriptor
{
   uint16_t                data_direction;               /** pf_data_direction_values_t */
   uint16_t                submodule_data_length;        /** Allowed: 0..1439 */
   uint8_t                 length_iops;                  /** Allowed: 1 */
   uint8_t                 length_iocs;                  /** Allowed: 1 */
} pf_data_descriptor_t;

typedef struct pf_submodule_properties
{
   uint8_t                 type;                         /** Bits 0..1: pnet_submodule_dir_t */
   bool                    sharedInput;
   bool                    reduce_input_submodule_data_length;
   bool                    reduce_output_submodule_data_length;
   bool                    discard_ioxs;
} pf_submodule_properties_t;

typedef struct pf_exp_submodule
{
   uint16_t                subslot_number;               /** Allowed: 0x0000..0x9FFF */
   uint32_t                submodule_ident_number;
   pf_submodule_properties_t submodule_properties;
   uint16_t                nbr_data_descriptors;
   pf_data_descriptor_t    data_descriptor[2];           /** 2 used for PNET_DIR_IO */
} pf_exp_submodule_t;

typedef struct pf_exp_module
{
   uint16_t                slot_number;                  /** Allowed 0x0000.0x7FFF */
   uint32_t                module_ident_number;
   uint16_t                module_properties;            /** Reserved - currently unused */

   uint16_t                nbr_submodules;
   pf_exp_submodule_t      submodules[PNET_MAX_SUBMODULES];
} pf_exp_module_t;

typedef struct pf_exp_api
{
   bool                    valid;
   uint32_t                api;
   uint16_t                nbr_modules;
   pf_exp_module_t         modules[PNET_MAX_MODULES];
} pf_exp_api_t;

typedef struct pf_submodule_state
{
   uint8_t                 add_info;                     /** Bits 0..2: pf_submod_add_info_t */
   bool                    advice_avail;                 /** Bit 3 */
   bool                    maintenance_required;         /** Bit 4 */
   bool                    maintenance_demanded;         /** Bit 5 */
   bool                    fault;                        /** Bit 6 */
   uint8_t                 ar_info;                      /** Bits 7..10: pf_submod_ar_info_t */
   uint8_t                 ident_info;                   /** Bits 11..14: pf_ident_info_values_t */
   bool                    format_indicator;             /** Bit 15: Always 1 (true) */
} pf_submodule_state_t;

typedef struct pf_submodule_diff
{
   uint16_t                subslot_number;
   uint32_t                submodule_ident_number;
   pf_submodule_state_t    submodule_state;
} pf_submodule_diff_t;

typedef struct pf_module_diff
{
   uint16_t                slot_number;
   uint32_t                module_ident_number;
   uint16_t                module_state;                 /** pf_module_state_values_t */
   uint16_t                nbr_submodule_diffs;
   pf_submodule_diff_t     submodule_diffs[PNET_MAX_SUBMODULES];
} pf_module_diff_t;

typedef struct pf_api_diff
{
   uint32_t                api;
   uint16_t                nbr_module_diffs;
   pf_module_diff_t        module_diffs[PNET_MAX_MODULES];
} pf_api_diff_t;

typedef struct pf_parameter_server_properties
{
   uint32_t                reserved;                     /** Currently unused */
} pf_parameter_server_properties_t;

typedef struct pf_prm_server
{
   bool                                valid;
   pf_uuid_t                           parameter_server_object_uuid;
   pf_parameter_server_properties_t    parameter_server_properties;           /** Allowed: 0 */
   uint16_t                            cm_initiator_activity_timeout_factor;  /** res: 100ms, allowed 1..1000 */
   uint16_t                            parameter_server_station_name_len;
   char                                parameter_server_station_name[240 + 1];/** Terminated */
} pf_prm_server_t;

typedef struct pf_address_resolution_properties
{
   uint8_t                 protocol;                     /** pf_protocol_values_t */
   uint8_t                 resolution_factor;            /** range: 1..0x64 */
} pf_address_resolution_properties_t;

typedef struct pf_multicast_cr
{
   bool                    valid;
   uint16_t                iocr_reference;
   pf_address_resolution_properties_t address_resolution_properties;
   uint16_t                mci_timeout_factor;              /* MCI_timeout range 0x0001..0xffff */
   uint16_t                length_provider_station_name;
   char                    provider_station_name[240 + 1];  /** Terminated */
} pf_multicast_cr_t;

typedef struct pf_ar_rpc_request
{
   bool                    valid;                     /** Requested in connect.req */
   uint16_t                initiator_rpc_server_port; /** Allowed 0x0000..0x03ff (optional), 0x0400..0xbfff (usable), 0xc000..0xffff (recommended) */
} pf_ar_rpc_request_t;

typedef struct pf_ar_rpc_result
{
   uint16_t                responder_rpc_server_port; /** Allowed 0x0000..0x03ff (optional), 0x0400..0xbfff (usable), 0xc000..0xffff (recommended) */
} pf_ar_rpc_result_t;

typedef struct pf_subframe_data
{
   uint8_t                 subframe_position;
   uint8_t                 subframe_length;
} pf_subframe_data_t;

typedef struct pf_dfp_iocr
{
   uint16_t                iocr_reference;
   uint16_t                subframe_offset;              /** Range: 0..0x59B */
   pf_subframe_data_t      subframe_data;
} pf_dfp_iocr_t;

typedef struct pf_ir_info
{
   bool                    valid;
   pf_uuid_t               ir_data_uuid;
   uint16_t                nbr_iocrs;                    /** Allowed 0, 2 */
   pf_dfp_iocr_t           dfp_iocrs[PNET_MAX_DFP_IOCR];
} pf_ir_info_t;

typedef struct pf_sr_info
{
   bool                    valid;
   uint16_t                redundancy_data_hold_factor;  /** res: 1ms, range: 1..0xC8 (optional), 0xc9..0xffff (mandatory) */
   struct
   {
      uint8_t              input_valid_on_backup;        /** Range: 0..1 */
      uint8_t              mode;                         /** Range: 0..1 */
   } sr_properties;
} pf_sr_info_t;

typedef struct pf_rs_info
{
   bool                    valid;
   uint8_t                 rs_alarm_transport_mode;      /** Range 0 (read by record), 1 (using alarm reporting) */
} pf_rs_info_t;

typedef struct pf_ar_fsu
{
   bool                    valid;
   struct
   {
      uint32_t             fs_parameter_mode;            /** Range 0..1 (ON, OFF) */
      pf_uuid_t            fs_parameter_uuid;
   } fs_parameter_block;
#if PNET_MAX_MAN_SPECIFIC_FAST_STARTUP_DATA_LENGTH
   uint16_t                length_manufacturer_specific_fast_startup_data;
   uint8_t                 manufacturer_specific_fast_startup_data[PNET_MAX_MAN_SPECIFIC_FAST_STARTUP_DATA_LENGTH];
#endif
} pf_ar_fsu_t;

typedef struct pf_ar_server
{
   uint16_t                length_cm_responder_station_name;
   char                    cm_responder_station_name[240 + 1];  /** Terminated */
} pf_ar_server_t;

typedef struct pf_alarm_cr_properties
{
   bool                    priority;                     /** pf_priority_values_t */
   bool                    transport_udp;                /** false ==> PF_RT_CLASS_1, true ==> PF_RT_CLASS_UDP */
} pf_alarm_cr_properties_t;

typedef struct pf_alarm_cr_tag_header
{
   uint8_t                 vlan_id;                      /** Bits 0..11. Allowed: No VLAN (0) */
   uint8_t                 alarm_user_priority;          /** Bits 12..15. Allowed: USER_PRIORITY_LOW (5), USER_PRIORITY_HIGH (6) */
} pf_alarm_cr_tag_header_t;

typedef struct pf_alarm_cr_request
{
   bool                    valid;
   uint32_t                alarm_cr_type;                /** Allowed: 0x0001 */
   uint16_t                lt_field;                     /** Allowed: 0x8892 */
   pf_alarm_cr_properties_t alarm_cr_properties;
   uint16_t                rta_timeout_factor;           /** res: 100ms, allowed: 1..100 (mandatory), 101..0xffff (optional) */
   uint16_t                rta_retries;                  /** Allowed: 3..15 */
   uint16_t                local_alarm_reference;        /** Controller reference */
   uint16_t                max_alarm_data_length;        /** Allowed 200..1432 */
   pf_alarm_cr_tag_header_t alarm_cr_tag_header_high;    /** USER_PRIORITY_HIGH (6) */
   pf_alarm_cr_tag_header_t alarm_cr_tag_header_low;     /** USER_PRIORITY_LOW (5) */
} pf_alarm_cr_request_t;

typedef struct pf_alarm_cr_result
{
   uint16_t                alarm_cr_type;                /** pf_alarm_cr_type_values_t (0x0001) */
   uint16_t                remote_alarm_reference;       /** Our reference */
   uint16_t                max_alarm_data_length;        /** Range: 200..1432 */
} pf_alarm_cr_result_t;

#if PNET_OPTION_AR_VENDOR_BLOCKS
typedef struct pf_ar_vendor_request
{
   bool                    valid;
   uint32_t                ap_structure_identifier;      /** 0..7fff (vendor specific), 0x8000 (extended identification rules), 0x8001..0xffff (administrative number) */
   uint32_t                api;
   uint16_t                length_data_request;
   uint8_t                 data_request[PNET_MAX_AR_VENDOR_BLOCK_DATA_LENGTH]; /** Not terminated. */
} pf_ar_vendor_request_t;

typedef struct pf_ar_vendor_result_t
{
   uint16_t                length_data_response;
   uint8_t                 data_response[PNET_MAX_AR_VENDOR_BLOCK_DATA_LENGTH]; /** Not terminated. */
} pf_ar_vendor_result_t;
#endif

typedef struct pf_ppm
{
   pf_ppm_state_values_t   state;
   uint32_t                exec;

   int                     errline;
   uint32_t                errcnt;

   pnet_ethaddr_t          sa;                  /* Source MAC address */
   pnet_ethaddr_t          da;                  /* Destination MAC address (IO-controller) */

   bool                    first_transmit;

   void                    *p_send_buffer;
   bool                    new_buf;             /* New data to be sent */

   uint16_t                cycle;
   uint8_t                 transfer_status;
   uint8_t                 data_status;
   uint16_t                buffer_length;
   uint16_t                buffer_pos;          /* Start of PROFINET data in frame */
   uint16_t                cycle_counter_offset;
   uint16_t                data_status_offset;
   uint16_t                transfer_status_offset;

   uint8_t                 buffer_data[1500];   /* Max */

   uint32_t                trx_cnt;

   uint32_t                control_interval;
   bool                    ci_running;
   uint32_t                ci_timer;
} pf_ppm_t;

typedef struct pf_cpm
{
   pf_cpm_state_values_t   state;
   uint32_t                max_exec;

   int                     errline;
   uint32_t                errcnt;

   uint32_t                recv_cnt;
   uint32_t                free_cnt;

   pnet_ethaddr_t          sa;                  /* Mac of the controller */

   uint16_t                nbr_frame_id;        /* 1 or 2 */
   uint16_t                frame_id[2];         /* 2 needed for some instances of RT_CLASS_3 */
   uint16_t                data_hold_factor;

   void                    *p_buffer_app;       /* Owned by app */
   void                    *p_buffer_cpm;       /* owned by cpm */
   bool                    new_buf;             /* New data to be received */
   uint16_t                frame_id_pos;        /* Handles VLAN in ETH header */

   uint8_t                 data_status;
   uint16_t                buffer_length;
   uint16_t                buffer_pos;          /* Start of PROFINET data in frame */

   uint16_t                dht;
   bool                    new_data;
   uint32_t                rxa[PNET_MAX_PORT][2];  /* Max 2 frame_ids */
   int32_t                 cycle;               /* value -1 means "never" */

   uint32_t                control_interval;
   bool                    ci_running;
   uint32_t                ci_timer;

   /* CMIO data */
   bool                    cmio_start;         /* cmInstance.start/stop */
} pf_cpm_t;

typedef struct pf_iodata_object
{
   bool                    in_use;
   uint32_t                api_id;
   uint16_t                slot_nbr;
   uint16_t                subslot_nbr;

   /* Individual sub-slot data */
   uint16_t                data_offset;
   uint16_t                data_length;
   /* The provider status */
   uint16_t                iops_offset;
   uint16_t                iops_length;
   /* The consumer status */
   uint16_t                iocs_offset;
   uint16_t                iocs_length;

   bool                    data_avail;

#if PNET_MAX_DFP_IOCR > 0
   pf_subframe_data_t      subframe_data[PNET_MAX_DFP_IOCR];
#endif
} pf_iodata_object_t;

/* Forward declaration of some useful types */
struct pf_ar;
struct pf_alpmx;

/* IOCR = IO Communication Relation */
typedef struct pf_iocr
{
   struct pf_ar            *p_ar;
   uint32_t                crep;                         /* Index to self in pf_ar_t  ->iocr array  */

   pf_cpm_t                cpm;                          /* Handles output */
   pf_ppm_t                ppm;                          /* Handles input  */

   uint16_t                mppm;
   uint16_t                cmdmc;
   uint16_t                dfp;

   /* Total frame content (data, IOCS and IOPS) length after L2 header (buffer_pos) */
   uint16_t                in_length;
   uint16_t                out_length;

   uint16_t                nbr_data_desc;
   pf_iodata_object_t      data_desc[PNET_MAX_API * PNET_MAX_MODULES * PNET_MAX_SUBMODULES];

   pf_iocr_param_t         param;                        /* From connect.req */
   pf_iocr_result_t        result;                       /* From connect.ind */
} pf_iocr_t;

typedef enum pf_cmsm_state_values
{
   PF_CMSM_STATE_IDLE,
   PF_CMSM_STATE_RUN
} pf_cmsm_state_values_t;

typedef enum pf_cmio_state_values
{
   PF_CMIO_STATE_IDLE = 0,
   PF_CMIO_STATE_STARTUP,
   PF_CMIO_STATE_WDATA,
   PF_CMIO_STATE_DATA,
} pf_cmio_state_values_t;

typedef enum pf_cmpbe_state_values
{
   PF_CMPBE_STATE_IDLE,
   PF_CMPBE_STATE_WFIND,
   PF_CMPBE_STATE_WFRSP,
   PF_CMPBE_STATE_WFPEI,
   PF_CMPBE_STATE_WFPER,
   PF_CMPBE_STATE_WFREQ,
   PF_CMPBE_STATE_WFCNF
} pf_cmpbe_state_values_t;

typedef enum pf_apms_state_values
{
   PF_APMS_STATE_CLOSED,
   PF_APMS_STATE_OPEN,
   PF_APMS_STATE_WTACK
} pf_apms_state_values_t;

typedef enum pf_apmr_state_values
{
   PF_APMR_STATE_CLOSED,
   PF_APMR_STATE_OPEN,
   PF_APMR_STATE_WCNF
} pf_apmr_state_values_t;

typedef struct pf_apmr_msg
{
   uint16_t                         frame_id_pos;
   os_buf_t                         *p_buf;
} pf_apmr_msg_t;

/*
 * This type contains all the information needed for one
 * APMS/APMR pair.
 */
typedef struct pf_apmx
{
   struct pf_ar            *p_ar;
   struct pf_alpmx         *p_alpmx;

   pnet_ethaddr_t          da;                           /* Destination MAC address (IO-controller) */

   uint16_t                src_ref;                      /* Our ref */
   uint16_t                dst_ref;                      /* controller local_alarm_reference */

   pf_apms_state_values_t  apms_state;
   pf_apmr_state_values_t  apmr_state;

   uint16_t                send_seq_count;
   uint16_t                send_seq_count_o;

   uint16_t                exp_seq_count;
   uint16_t                exp_seq_count_o;

   /* The receive queue */
   os_mbox_t               *p_alarm_q;
   /* The messages sent via the mailbox */
   pf_apmr_msg_t           apmr_msg[PNET_MAX_ALARMS];
   uint16_t                apmr_msg_nbr;

   /* Latest sent alarm */
   os_buf_t                *p_rta;

   uint16_t                vlan_prio;                    /* 5 or 6 */
   uint16_t                block_type_alarm_notify;
   uint16_t                block_type_alarm_ack;
   uint16_t                frame_id;

   uint32_t                timeout_us;
   uint32_t                timeout_id;
   uint32_t                retry;
} pf_apmx_t;

typedef enum pf_alpmr_state_values
{
   PF_ALPMR_STATE_W_START,
   PF_ALPMR_STATE_W_NOTIFY,
   PF_ALPMR_STATE_W_USER_ACK,
   PF_ALPMR_STATE_W_TACK
} pf_alpmr_state_values_t;

typedef enum pf_alpmi_state_values
{
   PF_ALPMI_STATE_W_START,
   PF_ALPMI_STATE_W_ALARM,
   PF_ALPMI_STATE_W_ACK,
} pf_alpmi_state_values_t;

/*
 * This type contains all the information needed for one
 * ALPMI/ALPMR pair.
 */
typedef struct pf_alpmx
{
   pnet_ethaddr_t          da;               /* Destination MAC address (IO-controller) */

   /*
    * The sequence number is used for PULL/PLUG requests.
    * It is updated by the alarm component and checked by the CMDEV component.
    */
   uint16_t                prev_sequence_number;
   uint16_t                sequence_number;

   pf_alpmi_state_values_t alpmi_state;
   pf_alpmr_state_values_t alpmr_state;
} pf_alpmx_t;

typedef enum pf_get_result
{
   PF_PARSE_OK,
   PF_PARSE_NULL_POINTER,                    /* Input buffer is NULL */
   PF_PARSE_END_OF_INPUT,                    /* Unexpected */
   PF_PARSE_OUT_OF_API_RESOURCES,
   PF_PARSE_OUT_OF_EXP_SUBMODULE_RESOURCES,
   PF_PARSE_ERROR                            /* Other */
} pf_get_result_t;

typedef struct pf_get_info
{
   pf_get_result_t         result;
   bool                    is_big_endian;
   uint8_t                 *p_buf;
   uint16_t                len;
} pf_get_info_t;

/*
 * A session stores information used for supervision of connection activity.
 * A session is allocated for each connect in order to handle segmented RPC requests.
 *   In order to find this session
 * A session is also needed for each CCONTROL request. Use pf_session_locate_by_uuid.
 */
typedef struct pf_session_info
{
   uint16_t                ix;
   bool                    in_use;
   bool                    release_in_progress;
   uint32_t                socket;
   os_eth_handle_t         *eth_handle;
   struct pf_ar            *p_ar;
   bool                    from_me;
   pf_uuid_t               activity_uuid;
   uint32_t                ip_addr;
   os_ipport_t             port;
   uint32_t                sequence_nmb_send;      /* rm_ccontrol_req */

   /*
    * According to the services spec the maximum supported write record data size is 4068 bytes.
    * Allow for some overhead.
    */
   uint8_t                 buffer[4500];           /* Send/Receive buffer */
   uint16_t                buf_len;

   pf_get_info_t           get_info;
   bool                    is_big_endian;          /* From rpc_header_t in first fragment */
   pnet_result_t           rpc_result;
   pf_ndr_data_t           ndr_data;

   /* These are used while collecting the fragments */
   uint16_t                fragment_nbr;

   /* This item is used to handle dcontrol re-runs */
   uint32_t                dcontrol_sequence_nmb;      /* From dcontrol request */
   pnet_result_t           dcontrol_result;
} pf_session_info_t;

typedef struct pf_ar
{
   bool                    in_use;
   uint16_t                arep;

   pf_session_info_t       *p_sess;

   pf_cmdev_state_values_t cmdev_state;                  /* pf_cmdev_state_values_t */
   pnet_ethaddr_t          src_addr;                     /* Connect client address */

   uint16_t                nbr_ar_param;
   pf_ar_param_t           ar_param;                     /* From connect.req */
   pf_ar_result_t          ar_result;                    /* From connect.ind */

   uint16_t                nbr_ar_server;
   pf_ar_server_t          ar_server;                    /* From connect.ind */

   uint16_t                nbr_alarm_cr;                 /* 0 or 1 */
   pf_alarm_cr_request_t   alarm_cr_request;             /* From connect.req */
   pf_alarm_cr_result_t    alarm_cr_result;              /* From connect.ind */

   bool                    alarm_enable;
   /* Alarm handling ALPMI/ALPMR machines: one for LOW (0) prio and one for HIGH (1) prio. */
   pf_alpmx_t              alpmx[2];
   /* Alarm handling APMS/APMR machines: one for LOW (0) prio and one for HIGH (1) prio. */
   pf_apmx_t               apmx[2];

   uint16_t                nbr_ar_rpc;
   pf_ar_rpc_request_t     ar_rpc_request;               /* From connect.req */
   pf_ar_rpc_result_t      ar_rpc_result;                /* From connect.ind */

   uint16_t                nbr_iocrs;                    /* From connect.req */
   pf_iocr_t               iocrs[PNET_MAX_CR];

   uint16_t                nbr_exp_apis;
   pf_exp_api_t            exp_apis[PNET_MAX_API];       /* From connect.req */

   uint16_t                nbr_api_diffs;                /* From connect.ind */
   pf_api_diff_t           api_diffs[PNET_MAX_API];

   uint16_t                nbr_prm_server;
   uint16_t                nbr_rpc_server;
   uint16_t                nbr_ir_info;

   /* Used for error discovery in connect RPC */
   uint16_t                input_cr_cnt;
   uint16_t                output_cr_cnt;
   uint16_t                mcr_cons_cnt;
   bool                    rtc3_present;

   /* Internal state */
   uint16_t                remote_application_ready_timeout;   /** res: 1s, allowed: 300 */
   uint16_t                ar_state;                           /** pf_ar_state_values_t */
   uint16_t                sync_state;                         /** pf_sync_state_values_t */
   bool                    ready_4_data;

   /* Global error codes */
   uint8_t                 err_cls;
   uint8_t                 err_code;

   pf_cmsm_state_values_t  cmsm_state;
   uint32_t                cmsm_timer;

   pf_cmio_state_values_t  cmio_state;
   uint32_t                cmio_timer;
   bool                    cmio_timer_run;

   pf_cmpbe_state_values_t cmpbe_state;
   uint16_t                cmpbe_stored_command;

   /* Optional data */
#if PNET_OPTION_MC_CR
   uint16_t                nbr_mcr;
   pf_multicast_cr_t       mcr_request[PNET_MAX_MC_CR];
#endif
#if PNET_OPTION_PARAMETER_SERVER
   pf_prm_server_t         prm_server;
#endif
#if PNET_OPTION_IR
   pf_ir_info_t            ir_info;
#endif
#if PNET_OPTION_SR
   pf_sr_info_t            sr_info;
#endif
#if PNET_OPTION_REDUNDANCY
   struct
   {
      uint8_t              end_point_1;                  /** pf_end_point_1_values_t */
      uint8_t              end_point_2;                  /** pf_end_point_2_values_t */
   } redundancy_info;
#endif
#if PNET_OPTION_AR_VENDOR_BLOCKS
   uint16_t                nbr_ar_vendor;
   pf_ar_vendor_request_t  ar_vendor_request[PNET_MAX_AR_VENDOR_BLOCKS];
   pf_ar_vendor_result_t   ar_vendor_result[PNET_MAX_AR_VENDOR_BLOCKS];
#endif
#if PNET_OPTION_RS
   pf_rs_info_t            rs_info;
#endif
#if PNET_OPTION_FAST_STARTUP
   pf_ar_fsu_t             fast_startup_data;
#endif
} pf_ar_t;


/*
 * ============= Plugable typedefs ==================
 */

typedef enum pf_submod_plug_state
{
   PF_SUBMOD_PLUG_OK,
   PF_SUBMOD_PLUG_SUBSTITUTE,                            /** module_state == SUBSTITUTE */
   PF_SUBMOD_PLUG_WRONG,                                 /** module_state == WRONG_MODULE */
   PF_SUBMOD_PLUG_NO                                     /** module_state == WRONG_MODULE */
} pf_submod_plug_state_t;

typedef enum pf_submod_owner
{
   PF_SUBMOD_OWNER_FREE,
   PF_SUBMOD_OWNER_SOL,                                  /* Superordinator locked */
   PF_SUBMOD_OWNER_IOS,                                  /* Owned by IO supervisor */
   PF_SUBMOD_OWNER_IOC,                                  /* Owned by IO controller */
} pf_submod_owner;

typedef enum pf_mod_plug_state
{
   PF_MOD_PLUG_NO_MODULE,                                /** No module plugged into slot. */
   PF_MOD_PLUG_WRONG_MODULE,                             /** Module/submodule ident number(s) do not match cfg. */
   PF_MOD_PLUG_PROPER_MODULE,                            /** All OK. Submodule may have diff. */
   PF_MOD_PLUG_SUBSTITUTE                                /** There is another module plugged into the slot. */
} pf_mod_plug_state_t;

typedef enum pf_submod_add_info
{
   PNET_SUBMOD_ADD_INFO_NONE,
   PNET_SUBMOD_ADD_INFO_TAKEOVER_NOT_ALLOWED
} pf_submod_add_info_t;

typedef enum pf_submod_ar_info
{
   PF_SUBMOD_AR_INFO_OWN,                                /** The AR is owner */
   PF_SUBMOD_AR_INFO_APPLICATION_READY_PENDING,          /** Parameterization in progress */
   PF_SUBMOD_AR_INFO_SUPERORDINATED_LOCKED,              /** Locked by application (ToDo: API for that) */
   PF_SUBMOD_AR_INFO_LOCKED_BY_IO_CONTROLLER,            /** Locked by other IO controller */
   PF_SUBMOD_AR_INFO_LOCKED_BY_IO_SUPERVISOR             /** Locked by other IO Supervisor */
} pf_submod_ar_info_t;

typedef struct pf_submod_state
{
   pf_submod_add_info_t       add_info;                  /** Bits 0..2 */
   bool                       advice_avail;              /** Bit 3 */
   bool                       maintenance_required;      /** Bit 4 */
   bool                       maintenance_demanded;      /** Bit 5 */
   bool                       fault;                     /** Bit 6 */
   pf_submod_ar_info_t        ar_info;                   /** Bits 7..10 */
   pf_submod_plug_state_t     ident_info;                /** Bits 11..14 */
   bool                       format_indicator;          /** Bit 15: Always 1 (true) */
} pf_submod_state_t;

typedef enum pf_usi_values
{
   /* 0x0000..0x07ff    Manufacturer specific */
   PF_USI_CHANNEL_DIAGNOSIS               = 0x8000,
   PF_USI_ALARM_MULTIPLE                  = 0x8001,
   PF_USI_EXTENDED_CHANNEL_DIAGNOSIS      = 0x8002,
   PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS     = 0x8003,
   /* 0x8004..0x80ff    Reserved */
   PF_USI_MAINTENANCE                     = 0x8100,
   /* 0x8101..0x81ff    Reserved */
   PF_USI_UPLOAD_AND_RETRIEVAL            = 0x8200,
   PF_USI_IPARAMETER                      = 0x8201,
   /* 0x8202..0x82ff    Reserved */
   PF_USI_RS_LOW_WATERMARK                = 0x8300,
   PF_USI_RS_TIMEOUT                      = 0x8301,
   PF_USI_RS_OVERFLOW                     = 0x8302,
   PF_USI_RS_EVENT                        = 0x8303,
   /* 0x8304..0x8309    Reserved */
   PF_USI_PE_ENERGY_SAVING_STATUS         = 0x8310,
   /* 0x8311..0x8319    Reserved */
   PF_USI_PROCESS_ALARM_REASON            = 0x8320,
   /* 0x8321..0x8fff    Reserved */
   /* 0x9000..0x9fff    Reserved for profiles */
   /* 0xa000..0xffff    Reserved */
} pf_usi_values_t;

/*
 * The diag filter is used when reading record data in CMRDR.
 */
typedef enum pf_diag_filter_level
{
   PF_DIAG_FILTER_FAULT_STD,
   PF_DIAG_FILTER_FAULT_ALL,
   PF_DIAG_FILTER_ALL,
   PF_DIAG_FILTER_M_REQ,
   PF_DIAG_FILTER_M_DEM
} pf_diag_filter_level_t;

typedef struct pf_diag_std
{
   uint32_t                ext_ch_add_value;
   uint32_t                qual_ch_qualifier;
   uint16_t                ch_nbr;
   uint16_t                ch_properties;
   uint16_t                ch_error_type;
   uint16_t                ext_ch_error_type;
} pf_diag_std_t;

/* Make pf_diag_usi_t equal in size to pf_diag_std_t */
#define PF_DIAG_MANUF_DATA_LEN      sizeof(pf_diag_std_t)
typedef struct pf_diag_usi
{
   uint8_t                 manuf_data[PF_DIAG_MANUF_DATA_LEN];
} pf_diag_usi_t;

/* This is the end of list designator. */
#define PF_DIAG_IX_NULL    UINT16_MAX

typedef struct pf_diag_item
{
   /* The selector is the usi member (0x0000..0x7fff => usi, else std) */
   union fmt
   {
      pf_diag_std_t        std;
      pf_diag_usi_t        usi;
   } fmt;

   bool                    in_use;
   uint16_t                usi;        /* pf_usi_values_t */
   uint16_t                next;       /* Next in list */
} pf_diag_item_t;

typedef struct pf_subslot
{
   bool                    in_use;
   uint16_t                subslot_nbr;

   /* Submodule plug information */
   uint32_t                exp_submodule_ident_number;
   uint32_t                submodule_ident_number;
   uint16_t                length_input;
   uint16_t                length_output;
   pnet_submodule_dir_t    direction;

   /* Run-time information */
   pf_ar_t                 *p_ar;
   pf_submod_state_t       submodule_state;

   /* The following members shall be protected by the device.diag_mutex. */
   /*
    * This is an index into device.diag_items[].
    * It points to the list of reported diag alarms for this specific sub-slot.
    */
   uint16_t                diag_list;
} pf_subslot_t;

typedef struct pf_slot
{
   bool                    in_use;
   /* Module plug information */
   uint32_t                exp_module_ident_number;
   uint32_t                module_ident_number;
   pf_mod_plug_state_t     plug_state;
   pf_subslot_t            subslots[PNET_MAX_SUBMODULES];

   /* Run-time information */
   pf_ar_t                 *p_ar;
   uint16_t                slot_nbr;
} pf_slot_t;

typedef struct pf_api
{
   bool                    in_use;
   uint32_t                api_id;

   pf_slot_t               slots[PNET_MAX_MODULES];

   pf_ar_t                 *p_ar;
} pf_api_t;

/*
 * The device struct contains information about the configured API's.
 * The api member contains a hierarchy which may be traversed using
 * the function pf_cmdev_cfg_traverse().
 */
typedef struct pf_device
{
   /*
    * Record things that are needed for the read/write record data service.
    */
   pf_api_t                apis[PNET_MAX_API];

   /*
    * This is the pool of diag items.
    * It is used instead of dynamic memory to avoid fragmentation.
    */
   os_mutex_t              *diag_mutex;      /* Protect the diag items */
   pf_diag_item_t          diag_items[PNET_MAX_DIAG_ITEMS];
   uint16_t                diag_items_free;  /* Head of the unused list */
} pf_device_t;

/*
 * This enum is sometimes used to limit the depth when traversing
 * the pf_device_t structure.
 */
typedef enum pf_dev_filter_level
{
   /* Order is important. */
   PF_DEV_FILTER_LEVEL_DEVICE,
   PF_DEV_FILTER_LEVEL_API_ID,
   PF_DEV_FILTER_LEVEL_API,
   PF_DEV_FILTER_LEVEL_SLOT,
   PF_DEV_FILTER_LEVEL_SUBSLOT,
} pf_dev_filter_level_t;

/* =================== read/write data record structures ================= */

typedef struct pf_iod_read_request /* Read from dcontrol */
{
   uint16_t                sequence_number;
   pf_uuid_t               ar_uuid;
   uint32_t                api;
   uint16_t                slot_number;
   uint16_t                subslot_number;
   uint8_t                 padding[2];
   uint16_t                index;
   uint32_t                record_data_length;
   pf_uuid_t               target_ar_uuid;   /* Only used if implicit AR */
   uint8_t                 rw_padding[8];
} pf_iod_read_request_t;

typedef struct pf_iod_read_result
{
   uint16_t                sequence_number;
   pf_uuid_t               ar_uuid;
   uint32_t                api;
   uint16_t                slot_number;
   uint16_t                subslot_number;
   uint8_t                 padding[2];
   uint16_t                index;
   uint32_t                record_data_length;
   uint16_t                add_data_1;
   uint16_t                add_data_2;
   uint8_t                 rw_padding[20];
} pf_iod_read_result_t;

typedef struct pf_iod_write_request /* Write from dcontrol */
{
   uint16_t                sequence_number;
   pf_uuid_t               ar_uuid;
   uint32_t                api;
   uint16_t                slot_number;
   uint16_t                subslot_number;
   uint8_t                 padding[2];
   uint16_t                index;
   uint32_t                record_data_length;
   uint8_t                 rw_padding[24];
} pf_iod_write_request_t;

typedef struct pf_iod_write_result
{
   uint16_t                sequence_number;
   pf_uuid_t               ar_uuid;
   uint32_t                api;
   uint16_t                slot_number;
   uint16_t                subslot_number;
   uint8_t                 padding[2];
   uint16_t                index;
   uint32_t                record_data_length;
   uint16_t                add_data_1;
   uint16_t                add_data_2;
   pnet_pnio_status_t      pnio_status;
   uint8_t                 rw_padding[16];
} pf_iod_write_result_t;

/* ============= LogBook typedefs ================== */
/* block_version_low == 1 */

typedef enum pf_ts_status_values
{
   PF_TS_STATUS_GLOBAL = 0,
   PF_TS_STATUS_LOCAL = 1,
   PF_TS_STATUS_LOCAL_ARB = 2,         /* Arbitrary time scale */
} pf_ts_status_values_t;

typedef struct pf_log_book_ts
{
   uint16_t                status;     /* pf_ts_status_values_t */
   uint16_t                sec_hi;
   uint32_t                sec_lo;
   uint32_t                nano_sec;
} pf_log_book_ts_t;

typedef struct pf_log_book_entry
{
   pf_log_book_ts_t        time_ts;
   pf_uuid_t               ar_uuid;
   pnet_pnio_status_t      pnio_status;
   uint32_t                entry_detail;
} pf_log_book_entry_t;

typedef struct pf_log_book
{
   pf_log_book_ts_t        time_ts;    /* Used for what ?? */
   pf_log_book_entry_t     entries[PNET_MAX_LOG_BOOK_ENTRIES];
   uint16_t                put;        /* Points to oldest entry if wrap */
   bool                    wrap;       /* All entries valid */
} pf_log_book_t;


struct pnet
{
   uint32_t                            os_buf_alloc_cnt;
   bool                                global_alarm_enable;
   os_mutex_t                          *cpm_buf_lock;
   atomic_int                          cpm_instance_cnt;
   os_mutex_t                          *ppm_buf_lock;
   atomic_int                          ppm_instance_cnt;
   uint16_t                            dcp_global_block_qualifier;
   pnet_ethaddr_t                      dcp_sam;
   bool                                dcp_delayed_response_waiting;
   uint32_t                            dcp_timeout;
   uint32_t                            dcp_sam_timeout;
   os_eth_handle_t                     *eth_handle;
   pf_eth_frame_id_map_t               eth_id_map[PF_ETH_MAX_MAP];
   volatile pf_scheduler_timeouts_t    scheduler_timeouts[PF_MAX_TIMEOUTS];
   volatile uint32_t                   scheduler_timeout_first;
   volatile uint32_t                   scheduler_timeout_free;
   os_mutex_t                          *scheduler_timeout_mutex;
   uint32_t                            scheduler_tick_interval;
   bool                                cmdev_initialized;
   pf_device_t                         cmdev_device;
   pf_cmina_dcp_ase_t                  cmina_perm_dcp_ase;
   pf_cmina_dcp_ase_t                  cmina_temp_dcp_ase;
   pf_cmina_state_values_t             cmina_state;
   uint8_t                             cmina_error_decode;
   uint8_t                             cmina_error_code_1;
   uint16_t                            cmina_hello_count;
   uint32_t                            cmina_hello_timeout;
   bool                                cmina_commit_ip_suite;
   os_mutex_t                          *p_cmrpc_rpc_mutex;
   uint32_t                            cmrpc_session_number;
   pf_ar_t                             cmrpc_ar[PNET_MAX_AR];
   pf_session_info_t                   cmrpc_session_info[PF_MAX_SESSION];
   int                                 cmrpc_rpcreq_socket;
   uint8_t                             cmrpc_dcerpc_req_frame[1500];
   uint8_t                             cmrpc_dcerpc_rsp_frame[1500];
   pf_cmsu_state_values_t              cmsu_state;
   pf_cmwrr_state_values_t             cmwrr_state;
   const pnet_cfg_t                    *p_fspm_default_cfg;
   pnet_cfg_t                          fspm_cfg;
   pf_log_book_t                       fspm_log_book;
   os_mutex_t                          *fspm_log_book_mutex;
};


#ifdef __cplusplus
}
#endif

#endif /* PF_TYPES_H */
