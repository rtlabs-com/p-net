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
extern "C" {
#endif

#if PNET_USE_ATOMICS
#include <stdatomic.h>
#else
#define atomic_int         uint32_t
#define ATOMIC_VAR_INIT(x) x

#ifdef atomic_fetch_add
#undef atomic_fetch_add
#endif
static inline uint32_t atomic_fetch_add (atomic_int * p, uint32_t v)
{
   uint32_t prev = *p;
   *p += v;

   return prev;
}
#ifdef atomic_fetch_sub
#undef atomic_fetch_sub
#endif
static inline uint32_t atomic_fetch_sub (atomic_int * p, uint32_t v)
{
   uint32_t prev = *p;
   *p -= v;

   return prev;
}
#endif

#define PF_RPC_SERVER_PORT             0x8894 /* PROFInet Context Manager */
#define PF_UDP_UNICAST_PORT            0x8892
#define PF_RPC_CCONTROL_EPHEMERAL_PORT 0xc001

#define PF_FRAME_BUFFER_SIZE 1500

/** This should be smaller than PF_FRAME_BUFFER_SIZE with the maximum size of
 * IP- and UDP headers, and some margin. Linux will fragment frames if this is
 * larger than 1464. */
#define PF_MAX_UDP_PAYLOAD_SIZE 1440

/**
 * Timeout in milliseconds after which the CControl request is re-transmitted.
 */
#define PF_CCONTROL_TIMEOUT 2000
#define PF_FRAG_TIMEOUT     2000

/** One neighbour shall be checked for PDPortDataCheck
 *  See Profinet 2.4 protocol, section 5.2.13.3
 */
#define PF_CHECK_PEERS_PER_PORT 1

/************************* Diagnosis ******************************************/

#define PF_DIAG_QUALIFIED_SEVERITY_MASK  ~0x00000007
#define PF_DIAG_BIT_MAINTENANCE_REQUIRED BIT (0)
#define PF_DIAG_BIT_MAINTENANCE_DEMANDED BIT (1)

/** Qualifier 31..27 */
#define PF_DIAG_QUALIFIER_MASK_FAULT 0xF8000000
/** Qualifier 26..17 */
#define PF_DIAG_QUALIFIER_MASK_DEMANDED 0x07FE0000
/** Qualifier 16..7 */
#define PF_DIAG_QUALIFIER_MASK_REQUIRED 0x0001FF80
/** Qualifier 6..3 */
#define PF_DIAG_QUALIFIER_MASK_ADVICE 0x00000078

/* ch_properties channel size in bits
   See also pnet_diag_ch_prop_type_values_t */
#define PF_DIAG_CH_PROP_TYPE_MASK 0x00ff
#define PF_DIAG_CH_PROP_TYPE_POS  0
#define PF_DIAG_CH_PROP_TYPE_GET(x)                                            \
   (((X)&PF_DIAG_CH_PROP_TYPE_MASK) >> PF_DIAG_CH_PROP_TYPE_POS)
#define PF_DIAG_CH_PROP_TYPE_SET(x, v)                                         \
   do                                                                          \
   {                                                                           \
      (x) &= ~PF_DIAG_CH_PROP_TYPE_MASK;                                       \
      (x) |= (v) << PF_DIAG_CH_PROP_TYPE_POS;                                  \
   } while (0)

/* ch_properties channel group or individual channel
   See also pnet_diag_ch_group_values_t */
#define PF_DIAG_CH_PROP_ACC_MASK 0x0100
#define PF_DIAG_CH_PROP_ACC_POS  8
#define PF_DIAG_CH_PROP_ACC_GET(x)                                             \
   (((x)&PF_DIAG_CH_PROP_ACC_MASK) >> PF_DIAG_CH_PROP_ACC_POS)
#define PF_DIAG_CH_PROP_ACC_SET(x, v)                                          \
   do                                                                          \
   {                                                                           \
      (x) &= ~PF_DIAG_CH_PROP_ACC_MASK;                                        \
      (x) |= (v) << PF_DIAG_CH_PROP_ACC_POS;                                   \
   } while (0)

/* ch_properties diagnosis severity
   See also pnet_diag_ch_prop_maint_values_t */
#define PF_DIAG_CH_PROP_MAINT_MASK 0x0600
#define PF_DIAG_CH_PROP_MAINT_POS  9
#define PF_DIAG_CH_PROP_MAINT_GET(x)                                           \
   (((x)&PF_DIAG_CH_PROP_MAINT_MASK) >> PF_DIAG_CH_PROP_MAINT_POS)
#define PF_DIAG_CH_PROP_MAINT_SET(x, v)                                        \
   do                                                                          \
   {                                                                           \
      (x) &= ~PF_DIAG_CH_PROP_MAINT_MASK;                                      \
      (x) |= (v) << PF_DIAG_CH_PROP_MAINT_POS;                                 \
   } while (0)

/* ch_properties appears or disappers */
#define PF_DIAG_CH_PROP_SPEC_MASK 0x1800
#define PF_DIAG_CH_PROP_SPEC_POS  11
#define PF_DIAG_CH_PROP_SPEC_GET(x)                                            \
   (((x)&PF_DIAG_CH_PROP_SPEC_MASK) >> PF_DIAG_CH_PROP_SPEC_POS)
#define PF_DIAG_CH_PROP_SPEC_SET(x, v)                                         \
   do                                                                          \
   {                                                                           \
      (x) &= ~PF_DIAG_CH_PROP_SPEC_MASK;                                       \
      (x) |= (v) << PF_DIAG_CH_PROP_SPEC_POS;                                  \
   } while (0)

typedef enum pf_diag_ch_prop_spec_values
{
   PF_DIAG_CH_PROP_SPEC_ALL_DISAPPEARS = 0,
   PF_DIAG_CH_PROP_SPEC_APPEARS = 1,
   PF_DIAG_CH_PROP_SPEC_DISAPPEARS = 2,
   PF_DIAG_CH_PROP_SPEC_DIS_OTHERS_REMAIN = 3,
} pf_diag_ch_prop_spec_values_t;

/* ch_properties direction
   See also pnet_diag_ch_prop_dir_values_t */
#define PF_DIAG_CH_PROP_DIR_MASK 0xe000
#define PF_DIAG_CH_PROP_DIR_POS  13
#define PF_DIAG_CH_PROP_DIR_GET(x)                                             \
   (((x)&PF_DIAG_CH_PROP_DIR_MASK) >> PF_DIAG_CH_PROP_DIR_POS)
#define PF_DIAG_CH_PROP_DIR_SET(x, v)                                          \
   do                                                                          \
   {                                                                           \
      (x) &= ~PF_DIAG_CH_PROP_DIR_MASK;                                        \
      (x) |= (v) << PF_DIAG_CH_PROP_DIR_POS;                                   \
   } while (0)

/*********************** RPC header ******************************************/

typedef enum pf_rpc_packet_type_values
{
   PF_RPC_PT_REQUEST = 0, //!< PF_RPC_PT_REQUEST
   PF_RPC_PT_PING,        //!< PF_RPC_PT_PING
   PF_RPC_PT_RESPONSE,    //!< PF_RPC_PT_RESPONSE
   PF_RPC_PT_FAULT,       //!< PF_RPC_PT_FAULT
   PF_RPC_PT_WORKING,     //!< PF_RPC_PT_WORKING
   PF_RPC_PT_RESP_PING,
   /* No Call - response to ping */ //!< PF_RPC_PT_RESP_PING
   PF_RPC_PT_REJECT,                //!< PF_RPC_PT_REJECT
   PF_RPC_PT_ACK,                   //!< PF_RPC_PT_ACK
   PF_RPC_PT_CL_CANCEL,             //!< PF_RPC_PT_CL_CANCEL
   PF_RPC_PT_FRAG_ACK,              //!< PF_RPC_PT_FRAG_ACK
   PF_RPC_PT_CANCEL_ACK,            //!< PF_RPC_PT_CANCEL_ACK
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
   PF_RPC_EPM_OPNUM_LOOKUP, /* Mandatory to support */
   PF_RPC_EPM_OPNUM_MAP,
   PF_RPC_EPM_OPNUM_LOOKUP_HANDLE_FREE, /* Mandatory */
   PF_RPC_EPM_OPNUM_INP_OBJECT,
   PF_RPC_EPM_OPNUM_MGMT_DELETE,
} pf_epmapper_opnum_values_t;

typedef enum pf_mediatype_values
{
   PF_PD_MEDIATYPE_UNKNOWN = 0,
   PF_PD_MEDIATYPE_COPPER,
   PF_PD_MEDIATYPE_FIBER,
   PF_PD_MEDIATYPE_RADIO
} pf_mediatype_values_t;

typedef struct pf_rpc_flags
{
   bool last_fragment;
   bool fragment;
   bool no_fack;
   bool maybe;
   bool idempotent;
   bool broadcast;
} pf_rpc_flags_t;

typedef struct pf_rpc_flags2_t
{
   bool cancel_pending;
} pf_rpc_flags2_t;

typedef struct pf_uuid
{
   uint32_t data1;
   uint16_t data2;
   uint16_t data3;
   uint8_t data4[8];
} pf_uuid_t;

typedef struct pf_rpc_header
{
   uint8_t version;      /* Allowed: 0x04 */
   uint8_t packet_type;  /* rpc_packet_type_values_t */
   pf_rpc_flags_t flags; /* rpc_flags_bits */
   pf_rpc_flags2_t flags2;
   bool is_big_endian;
   uint8_t float_repr; /* Allowed: IEEE (0) */
   uint8_t reserved;
   uint8_t serial_high;
   pf_uuid_t object_uuid;    /* Identifies IO device, IO controller or IO
                                supervisor */
   pf_uuid_t interface_uuid; /* Identify PNIO interface */
   pf_uuid_t activity_uuid;
   uint32_t server_boot_time;
   uint32_t interface_version;
   uint32_t sequence_nmb;
   uint16_t opnum; /* Dev: pf_dev_opnum_values_t/pf_epmapper_opnum_values */
   uint16_t interface_hint;
   uint16_t activity_hint;
   uint16_t length_of_body; /* AKA fragment_length */
   uint16_t fragment_nmb;
   uint8_t auth_protocol; /* Allowed: 0 (zero) */
   uint8_t serial_low;
} pf_rpc_header_t;

typedef enum pf_write_req_error_type_values
{
   PF_WRT_ERROR_REMOTE_MISMATCH = 0x8001
} pf_write_req_error_type_t;

typedef enum pf_write_req_ext_error_type_values
{
   PF_WRT_ERROR_PEER_STATIONNAME_MISMATCH = 0x8000,
   PF_WRT_ERROR_PEER_PORTNAME_MISMATCH = 0x8001,
   PF_WRT_ERROR_NO_PEER_DETECTED = 0x8005
} pf_write_req_ext_error_type_t;

/*
 * RPC and EPM implementation
 * PN-AL-protocol (Mar20) Section 4.10.3
 */

#define PF_RPC_TOWER_REFERENTID  3
#define PF_RPC_TOWER_FLOOR_COUNT 5

#define PF_RPC_FLOOR_VERSION_EPMv4 3
#define PF_RPC_FLOOR_VERSION_NPR   2
#define PF_RPC_FLOOR_VERSION_PNIO  1
#define PF_RPC_FLOOR_VERSION_MINOR 0

#define PF_RPC_EPM_ANNOTATION_OFFSET 0
#define PF_RPC_EPM_ANNOTATION_LENGTH 64
#define PF_RPC_EPM_FLOOR_LENGTH      75

typedef enum pf_rpc_protocol_type
{
   PF_RPC_PROTOCOL_DOD_UDP = 0x08,
   PF_RPC_PROTOCOL_DOD_IP = 0x09,
   PF_RPC_PROTOCOL_CONNECTIONLESS = 0x0a,
   PF_RPC_PROTOCOL_UUID = 0x0d
} pf_rpc_protocol_type_t;

/* PN-AL-protocol (Mar20) Table 328 */
typedef enum pf_rpc_inquiry_type
{
   PF_RPC_INQUIRY_READ_ALL_REGISTERED_INTERFACES = 0,        /*mandatory*/
   PF_RPC_INQUIRY_READ_ALL_OBJECTS_FOR_ONE_INTERFACE = 1,    /*optional*/
   PF_RPC_INQUIRY_READ_ALL_INTERFACES_INCLUDING_OBJECTS = 2, /*optional*/
   PF_RPC_INQUIRY_READ_ONE_INTERFACE_WITH_ONE_OBJECT = 3     /*optional*/
} pf_rpc_inquiry_type_t;

/* PN-AL-protocol (Mar20) Table 329 */
typedef enum pf_rpc_error_value
{
   PF_RPC_OK = 0,
   PF_RPC_NOT_REGISTERED = 0x16c9a0d6, /* Endpoint not registered */
} pf_rpc_error_value_t;

typedef enum
{
   PF_EPM_TYPE_NONE = 0,
   PF_EPM_TYPE_EPMV4,
   PF_EPM_TYPE_PNIO
} pf_rpc_epm_type_t;

/* Described in DCP 1.1 Appendix A */
typedef struct pf_rpc_uuid_type
{
   uint32_t time_low;
   uint16_t time_mid;
   uint16_t time_hi_and_version;
   uint8_t clock_hi_and_reserved;
   uint8_t clock_low;
   uint8_t node[6];
} pf_rpc_uuid_type_t;

/* Used for definition of epm protocol tower floors 1 and 2. */
typedef struct pf_floor_uuid_t
{
   uint8_t protocol_id;
   pf_rpc_uuid_type_t uuid;
   uint16_t version_major;
   uint16_t version_minor;
} pf_floor_uuid_t;

typedef struct pf_floor_3_t
{
   uint8_t protocol_id;
   uint16_t version_minor;
} pf_floor_3_t;

typedef struct pf_floor_4_t
{
   uint8_t protocol_id;
   uint16_t port;
} pf_floor_4_t;

typedef struct pf_floor_5_t
{
   uint8_t protocol_id;
   uint32_t ip_address;
} pf_floor_5_t;

typedef struct pf_rpc_tower
{
   pf_floor_uuid_t floor_1;
   pf_floor_uuid_t floor_2;
   pf_floor_3_t floor_3;
   pf_floor_4_t floor_4;
   pf_floor_5_t floor_5;
   pnet_cfg_t * p_cfg; /* Used for generation of annotation string*/
} pf_rpc_tower_t;

typedef struct pf_rpc_entry
{
   pf_rpc_epm_type_t epm_type;
   uint32_t max_count;
   uint32_t offset;
   uint32_t actual_count;
   pf_rpc_uuid_type_t object_uuid;
   pf_rpc_tower_t tower_entry;
} pf_rpc_entry_t;

typedef struct pf_rpc_handle
{
   uint32_t rpc_entry_handle;
   pf_rpc_uuid_type_t handle_uuid;
} pf_rpc_handle_t;

/*
 * See PN-AL-protocol (Mar20) Table 310 RPC substitutions
 * for definitions of rpc lookup response
 */
typedef struct pf_rpc_lookup_rsp
{
   pf_rpc_handle_t rpc_handle;
   uint32_t num_entry;
   pf_rpc_entry_t rpc_entry;
   uint32_t return_code;
} pf_rpc_lookup_rsp_t;

typedef struct pf_rpc_lookup_req
{
   uint32_t inquiry_type;
   uint32_t object_id;
   pf_uuid_t object_uuid;
   uint32_t interface_id;
   pf_uuid_t interface_uuid;
   uint16_t interface_ver_major;
   uint16_t interface_ver_minor;
   uint32_t version_option;
   pf_rpc_handle_t rpc_handle;
   uint32_t max_entries;
   uint16_t udpPort;
} pf_rpc_lookup_req_t;

/************************** Block header *************************************/

typedef enum pf_block_type_values
{
   /* Reserved 0x0000 */
   PF_BT_ALARM_NOTIFICATION_HIGH = 0x0001,
   PF_BT_ALARM_NOTIFICATION_LOW = 0x0002,
   /* Reserved 0x0003.. 0x0007 */
   PF_BT_IOD_WRITE_REQ_HEADER = 0x0008,
   PF_BT_IOD_READ_REQ_HEADER = 0x0009,
   /* Reserved 0x000A..0x000F */
   PF_BT_DIAGNOSIS_DATA = 0x0010,
   /* Reserved 0x0011 */
   PF_BT_EXPECTED_IDENTIFICATION_DATA = 0x0012,
   PF_BT_REAL_IDENTIFICATION_DATA = 0x0013,
   PF_BT_SUBSTITUTE_VALUE = 0x0014,
   PF_BT_RECORD_INPUT_DATA_OBJECT = 0x0015,
   PF_BT_RECORD_OUTPUT_DATA_OBJECT = 0x0016,

   PF_BT_AR_DATA = 0x0018,
   PF_BT_LOG_BOOK_DATA = 0x0019,
   PF_BT_API_DATA = 0x001A,
   PF_BT_SRL_DATA = 0x001B,

   PF_BT_IM_0 = 0x0020,
   PF_BT_IM_1 = 0x0021,
   PF_BT_IM_2 = 0x0022,
   PF_BT_IM_3 = 0x0023,
   PF_BT_IM_4 = 0x0024,
   PF_BT_IM_5 = 0x0025,
   PF_BT_IM_6 = 0x0026,
   PF_BT_IM_7 = 0x0027,
   PF_BT_IM_8 = 0x0028,
   PF_BT_IM_9 = 0x0029,
   PF_BT_IM_10 = 0x002a,
   PF_BT_IM_11 = 0x002b,
   PF_BT_IM_12 = 0x002c,
   PF_BT_IM_13 = 0x002d,
   PF_BT_IM_14 = 0x002e,
   PF_BT_IM_15 = 0x002f,

   PF_BT_IM_0_FILTER_DATA_SUBMODULE = 0x0030,
   PF_BT_IM_0_FILTER_DATA_MODULE = 0x0031,
   PF_BT_IM_0_FILTER_DATA_DEVICE = 0x0032,

   PF_BT_IM_5_DATA = 0x0034,

   PF_BT_AR_BLOCK_REQ = 0x0101,
   PF_BT_IOCR_BLOCK_REQ = 0x0102,
   PF_BT_ALARM_CR_BLOCK_REQ = 0x0103,
   PF_BT_EXPECTED_SUBMODULE_BLOCK = 0x0104,
   PF_BT_PRM_SERVER_REQ = 0x0105,
   PF_BT_MCR_REQ = 0x0106,
   PF_BT_RPC_SERVER_REQ = 0x0107,
   PF_BT_AR_VENDOR_BLOCK_REQ = 0x0108,
   PF_BT_IR_INFO_BLOCK_REQ = 0x0109,
   PF_BT_SR_INFO_BLOCK_REQ = 0x010A,
   PF_BT_AR_FSU_BLOCK_REQ = 0x010B,
   PF_BT_RS_INFO_BLOCK_REQ = 0x010C,
   /* Reserved 0x010D-0x0x010F */
   PF_BT_PRMEND_REQ = 0x0110,
   PF_BT_PRMEND_PLUG_ALARM_REQ = 0x0111,
   PF_BT_APPRDY_REQ = 0x0112,
   PF_BT_APPRDY_PLUG_ALARM_REQ = 0x0113,
   PF_BT_RELEASE_BLOCK_REQ = 0x0114,
   /* Reserved 0x0115 */
   PF_BT_XCONTROL_RDYCOMP_REQ = 0x0116,
   PF_BT_XCONTROL_RDYRTC3_REQ = 0x0117,
   PF_BT_PRMBEGIN_REQ = 0x0118,
   PF_BT_SUBMODULE_PRMBEGIN_REQ = 0x0119,

   PF_BT_PDPORTCHECK = 0x0200,
   PF_BT_CHECKPEERS = 0x020a,
   PF_BT_PDPORTDATAREAL = 0x020f,
   PF_BT_BOUNDARY_ADJUST = 0x0202,
   PF_BT_PEER_TO_PEER_BOUNDARY = 0x0224,
   PF_BT_INTERFACE_REAL_DATA = 0x0240,
   PF_BT_INTERFACE_ADJUST = 0x0250,
   PF_BT_PORT_STATISTICS = 0x0251,

   PF_BT_MULTIPLEBLOCK_HEADER = 0x0400,

   PF_BT_MAINTENANCE_ITEM = 0x0f00,

   /* Output from a PROFINET device */
   /* Reserved 0x8000 */
   PF_BT_ALARM_ACK_HIGH = 0x8001,
   PF_BT_ALARM_ACK_LOW = 0x8002,
   PF_BT_WRITE_RES = 0x8008,
   PF_BT_READ_RES = 0x8009,
   /* Reserved 0x8010.. 0x8100 */
   PF_BT_AR_BLOCK_RES = 0x8101,
   PF_BT_IOCR_BLOCK_RES = 0x8102,
   PF_BT_ALARM_CR_BLOCK_RES = 0x8103,
   PF_BT_MODULE_DIFF_BLOCK = 0x8104,
   PF_BT_PRM_SERVER_BLOCK_RES = 0x8105,
   PF_BT_AR_SERVER_BLOCK = 0x8106,
   PF_BT_AR_RPC_BLOCK_RES = 0x8107,
   PF_BT_AR_VENDOR_BLOCK_RES = 0x8108,
   /* Reserved 0x8109-0x0x810F */
   PF_BT_PRMEND_RES = 0x8110,
   PF_BT_PRMEND_PLUG_ALARM_RES = 0x8111,
   PF_BT_APPRDY_RES = 0x8112,
   PF_BT_APPRDY_PLUG_ALARM_RES = 0x8113,
   PF_BT_RELEASE_BLOCK_RES = 0x8114,
} pf_block_type_values_t;

typedef enum pf_index_values
{
   PF_IDX_USER_MAX = 0x7fff,

   PF_IDX_SUB_EXP_ID_DATA = 0x8000,
   PF_IDX_SUB_REAL_ID_DATA = 0x8001,

   PF_IDX_SUB_DIAGNOSIS_CH = 0x800a,
   PF_IDX_SUB_DIAGNOSIS_ALL = 0x800b,
   PF_IDX_SUB_DIAGNOSIS_DMQS = 0x800c,
   PF_IDX_SUB_DIAG_MAINT_REQ_CH = 0x8010,
   PF_IDX_SUB_DIAG_MAINT_DEM_CH = 0x8011,
   PF_IDX_SUB_DIAG_MAINT_REQ_ALL = 0x8012,
   PF_IDX_SUB_DIAG_MAINT_DEM_ALL = 0x8013,

   PF_IDX_SUB_SUBSTITUTE = 0x801e,

   PF_IDX_SUB_PDIR = 0x8020,

   PF_IDX_SUB_PDPORTDATAREALEXT = 0x8027,
   PF_IDX_SUB_INPUT_DATA = 0x8028,
   PF_IDX_SUB_OUTPUT_DATA = 0x8029,
   PF_IDX_SUB_PDPORT_DATA_REAL = 0x802a,
   PF_IDX_SUB_PDPORT_DATA_CHECK = 0x802b,
   PF_IDX_SUB_PDIR_DATA = 0x802c,
   PF_IDX_SUB_PDSYNC_ID0 = 0x802d,

   PF_IDX_SUB_PDPORT_DATA_ADJ = 0x802f,
   PF_IDX_SUB_ISOCHRONUOUS_DATA = 0x8030,
   PF_IDX_SUB_PDTIME = 0x8031,

   PF_IDX_SUB_PDINTF_MRP_REAL = 0x8050,
   PF_IDX_SUB_PDINTF_MRP_CHECK = 0x8051,
   PF_IDX_SUB_PDINTF_MRP_ADJUST = 0x8052,
   PF_IDX_SUB_PDPORT_MRP_ADJUST = 0x8053,
   PF_IDX_SUB_PDPORT_MRP_REAL = 0x8054,
   PF_IDX_SUB_PDPORT_MRPIC_REAL = 0x8055,
   PF_IDX_SUB_PDPORT_MRPIC_CHECK = 0x8056,
   PF_IDX_SUB_PDPORT_MRPIC_ADJUST = 0x8057,

   PF_IDX_SUB_PDPORT_FO_REAL = 0x8060,
   PF_IDX_SUB_PDPORT_FO_CHECK = 0x8061,
   PF_IDX_SUB_PDPORT_FO_ADJUST = 0x8062,

   PF_IDX_SUB_PDNC_CHECK = 0x8070,
   PF_IDX_SUB_PDINTF_ADJUST = 0x8071,
   PF_IDX_SUB_PDPORT_STATISTIC = 0x8072,

   PF_IDX_SUB_PDINTF_REAL = 0x8080,

   PF_IDX_SUB_PDINTF_FSU_ADJUST = 0x8090,

   PF_IDX_SUB_PE_ENTITY_STATUS = 0x80af,
   PF_IDX_SUB_COMBINED_OBJ_CONTAINER = 0x80b0,

   PF_IDX_SUB_RS_ADJUST_OBSERVER = 0x80cf,

   PF_IDX_SUB_IM_0 = 0xaff0,
   PF_IDX_SUB_IM_1 = 0xaff1,
   PF_IDX_SUB_IM_2 = 0xaff2,
   PF_IDX_SUB_IM_3 = 0xaff3,
   PF_IDX_SUB_IM_4 = 0xaff4,
   PF_IDX_SUB_IM_5 = 0xaff5,
   PF_IDX_SUB_IM_6 = 0xaff6,
   PF_IDX_SUB_IM_7 = 0xaff7,
   PF_IDX_SUB_IM_8 = 0xaff8,
   PF_IDX_SUB_IM_9 = 0xaff9,
   PF_IDX_SUB_IM_10 = 0xaffa,
   PF_IDX_SUB_IM_11 = 0xaffb,
   PF_IDX_SUB_IM_12 = 0xaffc,
   PF_IDX_SUB_IM_13 = 0xaffd,
   PF_IDX_SUB_IM_14 = 0xaffe,
   PF_IDX_SUB_IM_15 = 0xafff,

   PF_IDX_SLOT_EXP_ID_DATA = 0xc000,
   PF_IDX_SLOT_REAL_ID_DATA = 0xc001,

   PF_IDX_SLOT_DIAGNOSIS_CH = 0xc00a,
   PF_IDX_SLOT_DIAGNOSIS_ALL = 0xc00b,
   PF_IDX_SLOT_DIAGNOSIS_DMQS = 0xc00c,

   PF_IDX_SLOT_DIAG_MAINT_REQ_CH = 0xc010,
   PF_IDX_SLOT_DIAG_MAINT_DEM_CH = 0xc011,
   PF_IDX_SLOT_DIAG_MAINT_REQ_ALL = 0xc012,
   PF_IDX_SLOT_DIAG_MAINT_DEM_ALL = 0xc013,

   PF_IDX_AR_EXP_ID_DATA = 0xe000,
   PF_IDX_AR_REAL_ID_DATA = 0xe001,
   PF_IDX_AR_MOD_DIFF = 0xe002,

   PF_IDX_AR_DIAGNOSIS_CH = 0xe00a,
   PF_IDX_AR_DIAGNOSIS_ALL = 0xe00b,
   PF_IDX_AR_DIAGNOSIS_DMQS = 0xe00c,

   PF_IDX_AR_DIAG_MAINT_REQ_CH = 0xe010,
   PF_IDX_AR_DIAG_MAINT_DEM_CH = 0xe011,
   PF_IDX_AR_DIAG_MAINT_REQ_ALL = 0xe012,
   PF_IDX_AR_DIAG_MAINT_DEM_ALL = 0xe013,

   PF_IDX_AR_PE_ENTITY_FILTER_DATA = 0xe030,
   PF_IDX_AR_PE_ENTITY_STATUS_DATA = 0xe031,

   PF_IDX_AR_WRITE_MULTIPLE = 0xe040,

   PF_IDX_AR_FSU_DATA_ADJUST = 0xe050,

   PF_IDX_AR_RS_GET_EVENT = 0xe060,
   PF_IDX_AR_RS_ACK_EVENT = 0xe061,

   PF_IDX_API_REAL_ID_DATA = 0xf000,

   PF_IDX_API_DIAGNOSIS_CH = 0xf00a,
   PF_IDX_API_DIAGNOSIS_ALL = 0xf00b,
   PF_IDX_API_DIAGNOSIS_DMQS = 0xf00c,

   PF_IDX_API_DIAG_MAINT_REQ_CH = 0xf010,
   PF_IDX_API_DIAG_MAINT_DEM_CH = 0xf011,
   PF_IDX_API_DIAG_MAINT_REQ_ALL = 0xf012,
   PF_IDX_API_DIAG_MAINT_DEM_ALL = 0xf013,

   PF_IDX_API_AR_DATA = 0xf020,

   PF_IDX_DEV_DIAGNOSIS_DMQS = 0xf80c,

   PF_IDX_DEV_AR_DATA = 0xf820,
   PF_IDX_DEV_API_DATA = 0xf821,

   PF_IDX_DEV_LOGBOOK_DATA = 0xf830,
   PF_IDX_DEV_PDEV_DATA = 0xf831,

   PF_IDX_DEV_IM_0_FILTER_DATA = 0xf840,
   PF_IDX_DEV_PDREAL_DATA = 0xf841,
   PF_IDX_DEV_PDEXP_DATA = 0xf842,

   PF_IDX_DEV_AUTO_CONFIGURATION = 0xf850,
   PF_IDX_DEV_GSD_UPLOAD = 0xf860,

   PF_IDX_DEV_PE_ENTITY_FILTER_DATA = 0xf870,
   PF_IDX_DEV_PE_ENTITY_STATUS_DATA = 0xf871,
   PF_IDX_DEV_ASSET_MANAGEMENT_1 = 0xf880,
   PF_IDX_DEV_ASSET_MANAGEMENT_2 = 0xf881,
   PF_IDX_DEV_ASSET_MANAGEMENT_3 = 0xf882,
   PF_IDX_DEV_ASSET_MANAGEMENT_4 = 0xf883,
   PF_IDX_DEV_ASSET_MANAGEMENT_5 = 0xf884,
   PF_IDX_DEV_ASSET_MANAGEMENT_6 = 0xf885,
   PF_IDX_DEV_ASSET_MANAGEMENT_7 = 0xf886,
   PF_IDX_DEV_ASSET_MANAGEMENT_8 = 0xf887,
   PF_IDX_DEV_ASSET_MANAGEMENT_9 = 0xf888,
   PF_IDX_DEV_ASSET_MANAGEMENT_10 = 0xf889,

   PF_IDX_DEV_CONN_MON_TRIGGER = 0xfbff,
} pf_index_values_t;

CC_PACKED_BEGIN
typedef struct CC_PACKED pf_block_header
{
   uint16_t block_type; /** pf_block_type_values_t */
   uint16_t block_length;
   uint8_t block_version_high;
   uint8_t block_version_low;
} pf_block_header_t;
CC_PACKED_END
/* The following line guarantees that offsetof works as expected on received
 * data. */
CC_STATIC_ASSERT (sizeof (pf_block_header_t) == (2 + 2 + 1 + 1));

/* <============================ Block header */

typedef struct pf_ndr_array
{
   uint32_t maximum_count;
   uint32_t offset;
   uint32_t actual_count;
} pf_ndr_array_t;

typedef struct pf_ndr_data
{
   uint32_t args_maximum;
   uint32_t args_length;
   pf_ndr_array_t array;
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
   pf_uuid_t ar_uuid;
   uint16_t session_key;
   uint16_t control_command;
   uint16_t control_block_properties;
   uint16_t alarm_sequence_number;
} pf_control_block_t;

typedef struct pf_ip_suite
{
   /* Order is important!! */
   pnal_ipaddr_t ip_addr;
   pnal_ipaddr_t ip_mask;
   pnal_ipaddr_t ip_gateway;
} pf_ip_suite_t;

typedef struct pf_full_ip_suite
{
   /* Order is important!! */
   pf_ip_suite_t ip_suite;
   pnal_ipaddr_t dns_addr[4];
} pf_full_ip_suite_t;

/*
 * ============= im_0_filter_data typedefs ==================
 */
typedef struct pf_im_0_filter_data_sub
{
   uint16_t subslot_number; /** Allowed 0x0000.0x7FFF */
   uint32_t submodule_ident_number;
} pf_im_0_filter_data_sub_t;

typedef struct pf_im_0_filter_data_mod
{
   uint32_t module_ident_number;
   uint16_t slot_number; /** Allowed 0x0000.0x7FFF */
   uint16_t nbr_submodules;
   pf_im_0_filter_data_sub_t submodules[1];
} pf_im_0_filter_data_mod_t;

typedef struct pf_im_0_filter_data_api
{
   uint32_t api;
   uint16_t nbr_modules;
   pf_im_0_filter_data_mod_t modules[1];
} pf_im_0_filter_data_api_t;

typedef struct pf_im_0_filter_data
{
   uint16_t nbr_apis;
   pf_im_0_filter_data_api_t apis[1];
} pf_im_0_filter_data_t;

/*
 * ============= Alarm typedefs ==================
 */

#define PF_ALARM_PDU_TYPE_VERSION_1 0x01

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
   uint8_t type; /* pf_alarm_pdu_type_values_t */
   uint8_t version;
} pf_alarm_pdu_type_t;

typedef struct pf_alarm_add_flags
{
   uint8_t window_size;
   bool tack;
} pf_alarm_add_flags_t;

typedef struct pf_alarm_fixed
{
   uint16_t dst_ref;
   uint16_t src_ref;
   pf_alarm_pdu_type_t pdu_type;
   pf_alarm_add_flags_t add_flags;
   uint16_t send_seq_num;
   uint16_t ack_seq_nbr;
   /*
    * pf_alarm_fixed_t is followed by:
    * uint16_t             var_part_len;  (0..1432)
    *                     [block_header (pf_alarm_data_t/pf_alarm_err_t)]
    */
} pf_alarm_fixed_t;

typedef struct pf_alarm_err
{
   pnet_pnio_status_t pnio_status;
   /*
    * pf_alarm_err_t may be followed by alarm_payload:
    *    vendor_device_error_info = VendorId, DeviceId, data*
    */
} pf_alarm_err_t;

typedef struct pf_diag_std
{
   uint32_t ext_ch_add_value;
   uint32_t qual_ch_qualifier;
   uint16_t ch_nbr;
   uint16_t ch_properties; /* Appears, direction, channelgroup etc */
   uint16_t ch_error_type;
   uint16_t ext_ch_error_type;
} pf_diag_std_t;

/* Make pf_diag_usi_t equal in size to pf_diag_std_t */
#define PF_DIAG_MANUF_DATA_SIZE sizeof (pf_diag_std_t)
typedef struct pf_diag_usi
{
   uint8_t manuf_data[PF_DIAG_MANUF_DATA_SIZE];
} pf_diag_usi_t;

/* This is the end of list designator. */
#define PF_DIAG_IX_NULL UINT16_MAX

typedef struct pf_diag_item
{
   /* The selector is the usi member (0x0000..0x7fff => usi, else std) */
   union fmt
   {
      pf_diag_std_t std;
      pf_diag_usi_t usi;
   } fmt;

   bool in_use;
   uint16_t usi;  /* pf_usi_values_t */
   uint16_t next; /* Next in list (array index) */
} pf_diag_item_t;

#define PF_ALARM_MAX_PAYLOAD_DATA_SIZE sizeof (pf_diag_item_t)

typedef struct pf_alarm_payload
{
   uint16_t usi;
   uint16_t len;
   uint8_t data[PF_ALARM_MAX_PAYLOAD_DATA_SIZE];
} pf_alarm_payload_t;

/* See also pnet_alarm_argument_t for a subset */
typedef struct pf_alarm_data
{
   uint16_t alarm_type; /* pf_alarm_type_values_t */
   uint32_t api_id;
   uint16_t slot_nbr;
   uint16_t subslot_nbr;
   uint32_t module_ident;
   uint16_t submodule_ident;
   pnet_alarm_spec_t alarm_specifier; /* Booleans for diagnosis alarms. */
   uint16_t sequence_number;
   /*
    * pf_alarm_data_t may be followed by alarm_payload:
    *    RTA-SDU = alarm_noftification_pdu | alarm_ack_pdu
    */
   pf_alarm_payload_t payload;
} pf_alarm_data_t;

typedef struct pf_alarm_queue
{
   pf_alarm_data_t items[PNET_MAX_ALARMS];
   uint16_t write_index;
   uint16_t read_index;
   uint16_t count;
} pf_alarm_queue_t;

#define PF_MAX_SESSION (2 * (PNET_MAX_AR) + 1) /* 2 per ar, and one spare. */

/*
 * Keep this value small as it define the number of entries in the
 * frame id map and all entries are probed until a match is found.
 *
 * Each input CR may have 2 frameIds (for RTC3)
 * Add space for DCP:     0xfefc..0xfeff.
 * Add space for alarms:  0xfc01, 0xfe01.
 */
#define PF_ETH_MAX_MAP                                                         \
   ((PNET_MAX_API) * (PNET_MAX_AR) * (PNET_MAX_CR)*2 + 4 + 2)

/**
 * The scheduler is used by both the CPM and PPM machines.
 * The DCP uses the scheduler for responding to multi-cast messages.
 * pf_cmsm uses it to supervise the startup sequence.
 */
#define PF_MAX_TIMEOUTS (2 * (PNET_MAX_AR) * (PNET_MAX_CR) + 10)

#define PF_CMINA_FS_HELLO_RETRY 3
#define PF_CMINA_FS_HELLO_INTERVAL                                             \
   (3 * 1000)                            /* milliseconds. Default is 30 ms */
#define PF_LLDP_SEND_INTERVAL (5 * 1000) /* milliseconds */
#define PF_LLDP_INITIAL_PEER_TIMEOUT                                           \
   ((2 * PF_LLDP_SEND_INTERVAL) / 1000) /* seconds */
#define PF_LLDP_TTL 20                  /* seconds */

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
 * @param arg              In:    User-defined (may be NULL).
 * @param current_time     In:    The current system time, in microseconds,
 *                                when the scheduler is started to execute
 * stored tasks.
 */
typedef void (*pf_scheduler_timeout_ftn_t) (
   pnet_t * net,
   void * arg,
   uint32_t current_time);

typedef struct pf_scheduler_timeouts
{
   const char * p_name; /* For debugging only */
   bool in_use;         /* For debugging only */

   uint32_t when; /* absolute time of timeout */
   uint32_t next; /* Next in list */
   uint32_t prev; /* Previous in list */

   pf_scheduler_timeout_ftn_t cb; /* Call-back to call on timeout */
   void * arg;                    /* call-back argument */
} pf_scheduler_timeouts_t;

/**
 * This is the prototype for the Profinet frame handler.
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:    The frame ID.
 * @param p_buf            In:    The Ethernet frame.
 * @param frame_id_pos     In:    The position of the frame ID.
 * @param p_arg            In:    User-defined argument.
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
typedef int (*pf_eth_frame_handler_t) (
   pnet_t * net,
   uint16_t frame_id,
   pnal_buf_t * p_buf,
   uint16_t frame_id_pos,
   void * p_arg);

typedef struct pf_eth_frame_id_map
{
   bool in_use;
   uint16_t frame_id;
   pf_eth_frame_handler_t frame_handler;
   void * p_arg;
} pf_eth_frame_id_map_t;

/*
 * Each struct in pf_cmina_dcp_ase_t is carefully laid out in order to use
 * strncmp/memcmp in the DCP identity request and strncpy/memcpy in the
 * get/set requests.
 */
typedef struct pf_cmina_dcp_ase
{
   char station_name[PNET_STATION_NAME_MAX_SIZE]; /* Terminated string */

   /*
    * DCP DeviceVendorValue is set to configured product_name
    * See PN-AL-protocol (Mar20) ch 4.3.1.4.26
    * product_name is also known as DeviceType
    */
   char product_name[PNET_PRODUCT_NAME_MAX_LEN + 1]; /* Terminated string */
   uint8_t device_role;        /* Only value "1" supported */
   uint16_t device_initiative; /* 1: Should send hello. 0: No sending of hello
                                */

   char alias_name
      [PNET_STATION_NAME_MAX_SIZE + PNET_PORT_ID_MAX_SIZE]; /** Terminated */
   struct
   {
      /* Order is important!! */
      uint8_t high;
      uint8_t low;
   } instance_id;

   pnet_cfg_device_id_t device_id;
   pnet_cfg_device_id_t oem_device_id;

   char port_name[PNET_PORT_ID_MAX_SIZE]; /* Terminated. Not used. */

   pnet_ethaddr_t mac_address;
   uint16_t standard_gw_value;

   bool dhcp_enable;
   pf_full_ip_suite_t full_ip_suite;

   struct
   {
      /* Not yet used */
      char hostname[240 + 1];
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
   PF_ART_IOSAR = 0x0006, /** Supervisor AR */
   PF_ART_IOCAR_SINGLE_RTC_3 = 0x0010,
   PF_ART_IOCAR_SR = 0x0020, /** Set of AR */
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
   PF_CAR_FIRST_COMPANION_AR, /** For future use */
   PF_CAR_SECOND_COMPANION_AR /** For future use */
} pf_companion_ar_values_t;

typedef enum pf_rt_class_values
{
   PF_RT_CLASS_1 = 1, /** Legacy: FrameID range 7 */
   PF_RT_CLASS_2,     /** FrameID range 6 */
   PF_RT_CLASS_3,     /** FrameID range 3 */
   PF_RT_CLASS_UDP    /** FrameID range 7 */
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

/* See also pnet_submodule_dir_t */
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
   PF_AR_STATE_PRIMARY, /** default value */
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
 * A CMDEV_DA state machine has only the states POWER_ON, W_CIND, W_CRES and
 * DATA. A CMDEV state machine uses all states listed below.
 */
typedef enum pf_cmdev_state_values
{
   PF_CMDEV_STATE_POWER_ON = 0, /* Data initialization. (Must be first) */
   PF_CMDEV_STATE_W_CIND,       /* Wait for connect indcation. */
   PF_CMDEV_STATE_W_CRES,       /* Wait for connect response from app and CMSU
                                   startup. */
   PF_CMDEV_STATE_W_SUCNF,      /* Wait for CMSU confirmation. */
   PF_CMDEV_STATE_W_PEIND,      /* Wait for PrmEnd indication. */
   PF_CMDEV_STATE_W_PERES,      /* Wait for PrmEnd response from app. */
   PF_CMDEV_STATE_W_ARDY,       /* Wait for app ready from app. */
   PF_CMDEV_STATE_W_ARDYCNF, /* Wait for app ready confirmation from controller.
                              */
   PF_CMDEV_STATE_WDATA,     /* Wait for established cyclic data exchange. */
   PF_CMDEV_STATE_DATA,      /* Data exchange and connection monitoring. */
   PF_CMDEV_STATE_ABORT      /* Abort application relation. */
} pf_cmdev_state_values_t;

/* ====================================================================== */

typedef struct pf_ar_properties
{
   uint16_t state; /** Allowed values: ACTIVE (=1) */
   bool supervisor_takeover_allowed;
   uint8_t parameterization_server; /** pf_parameter_server_values_t */
   bool device_access;              /** See IEC61158-6-10 */
   uint16_t companion_ar;           /** pf_companion_ar_values_t */
   bool acknowledge_companion_ar;   /** whether companion_ar is needed */
   bool combined_object_container;  /** See IEC61158-6-10 */
   bool startup_mode; /** false: legacy (do not use), true: advanced */
   bool pull_module_alarm_allowed; /** false: mandatory support, true: optional
                                    */
} pf_ar_properties_t;

typedef struct pf_ar_param
{
   bool valid;
   uint16_t ar_type; /** pf_ar_type_values_t */
   pf_uuid_t ar_uuid;
   uint16_t session_key;
   pnet_ethaddr_t cm_initiator_mac_add;
   pf_uuid_t cm_initiator_object_uuid;
   pf_ar_properties_t ar_properties;
   uint16_t cm_initiator_activity_timeout_factor; /** res: 100ms */
   uint16_t cm_initiator_udp_rt_port;             /** 0x8892 */
   uint16_t cm_initiator_station_name_len;
   char cm_initiator_station_name[PNET_STATION_NAME_MAX_SIZE]; /** Terminated
                                                                     string. */
} pf_ar_param_t;

typedef struct pf_ar_result
{
   pnet_ethaddr_t cm_responder_mac_add;
   uint16_t responder_udp_rt_port;
} pf_ar_result_t;

typedef struct pf_iocr_properties
{
   uint32_t rt_class; /** pf_rt_class_values_t */
   uint16_t reserved_1;
   uint16_t reserved_2;
   uint16_t reserved_3;
} pf_iocr_properties_t;

typedef struct pf_frame_descriptor
{
   uint16_t slot_number;
   uint16_t subslot_number;
   uint16_t frame_offset;
} pf_frame_descriptor_t;

typedef struct pf_api_entry
{
   uint32_t api;
   uint16_t nbr_io_data;
   pf_frame_descriptor_t io_data[PNET_MAX_SLOTS * PNET_MAX_SUBSLOTS];
   uint16_t nbr_iocs;
   pf_frame_descriptor_t iocs[PNET_MAX_SLOTS * PNET_MAX_SUBSLOTS];
} pf_api_entry_t;

typedef struct pf_iocr_tag_header
{
   uint16_t vlan_id;            /** Bits 0..11. Default: 0 */
   uint16_t iocr_user_priority; /** Bits 12..15. Allowed: IOCR_PRIORITY = 6 */
} pf_iocr_tag_header_t;

typedef struct pf_iocr_param
{
   bool valid;
   uint16_t iocr_type;      /** pf_iocr_type_values_t */
   uint16_t iocr_reference; /** Cross-ref from e.g. mcr_request and ir_info */
   uint16_t lt_field;       /** Allowed: 0x8892 */
   pf_iocr_properties_t iocr_properties;
   uint16_t c_sdu_length; /** Allowed: RT_CLASS_UDP: 12..1440, others: 40..1440
                           */
   uint16_t frame_id;     /** Depends on RT_class */
   uint16_t send_clock_factor; /** res: 31.25us, Allowed: 1..128, default: 32 */
   uint16_t reduction_ratio;   /** Allowed: 1..512 */
   uint16_t phase;             /** 1..reduction_ratio */
   uint16_t sequence;          /** Allowed: RT_class_3: 0, others: 0
                                  (mandatory) 1..0xffff (optional) */
   uint32_t frame_send_offset; /** res: 1ns, Allowed: (All:) BEST_EFFORT
                                  (0xffffffff), RT_class_3: 0x0..0x3D08FF
                                  (mandatory) */
   uint32_t watchdog_factor;
   uint16_t data_hold_factor; /** Allowed: 0x0000..0x1e00, more restrictions
                                 apply */

   pf_iocr_tag_header_t iocr_tag_header;

   pnet_ethaddr_t iocr_multicast_mac_add; /** See IEC61158-6-10 */

   uint16_t nbr_apis;
   pf_api_entry_t apis[PNET_MAX_API];
} pf_iocr_param_t;

typedef struct pf_iocr_result
{
   uint16_t iocr_type; /** pf_iocr_type_values_t */
   uint16_t iocr_reference;
   uint16_t frame_id;
} pf_iocr_result_t;

typedef struct pf_data_descriptor
{
   uint16_t data_direction;        /** pf_data_direction_values_t */
   uint16_t submodule_data_length; /** Allowed: 0..1439 */
   uint8_t length_iops;            /** Allowed: 1 */
   uint8_t length_iocs;            /** Allowed: 1 */
} pf_data_descriptor_t;

typedef struct pf_submodule_properties
{
   uint8_t type; /** Bits 0..1: pnet_submodule_dir_t */
   bool sharedInput;
   bool reduce_input_submodule_data_length;
   bool reduce_output_submodule_data_length;
   bool discard_ioxs;
} pf_submodule_properties_t;

typedef struct pf_exp_submodule
{
   uint16_t subslot_number; /** Allowed: 0x0000..0x9FFF */
   uint32_t submodule_ident_number;
   pf_submodule_properties_t submodule_properties;
   uint16_t nbr_data_descriptors;
   pf_data_descriptor_t data_descriptor[2]; /** 2 used for PNET_DIR_IO */
} pf_exp_submodule_t;

typedef struct pf_exp_module
{
   uint16_t slot_number; /** Allowed 0x0000.0x7FFF */
   uint32_t module_ident_number;
   uint16_t module_properties; /** Reserved - currently unused */

   uint16_t nbr_submodules;
   pf_exp_submodule_t submodules[PNET_MAX_SUBSLOTS];
} pf_exp_module_t;

typedef struct pf_exp_api
{
   bool valid;
   uint32_t api;
   uint16_t nbr_modules;
   pf_exp_module_t modules[PNET_MAX_SLOTS];
} pf_exp_api_t;

typedef struct pf_submodule_state
{
   uint8_t add_info;          /** Bits 0..2: pf_submod_add_info_t */
   bool advice_avail;         /** Bit 3 */
   bool maintenance_required; /** Bit 4 */
   bool maintenance_demanded; /** Bit 5 */
   bool fault;                /** Bit 6 */
   uint8_t ar_info;           /** Bits 7..10: pf_submod_ar_info_t */
   uint8_t ident_info;        /** Bits 11..14: pf_ident_info_values_t */
   bool format_indicator;     /** Bit 15: Always 1 (true) */
} pf_submodule_state_t;

typedef struct pf_submodule_diff
{
   uint16_t subslot_number;
   uint32_t submodule_ident_number;
   pf_submodule_state_t submodule_state;
} pf_submodule_diff_t;

typedef struct pf_module_diff
{
   uint16_t slot_number;
   uint32_t module_ident_number;
   uint16_t module_state; /** pf_module_state_values_t */
   uint16_t nbr_submodule_diffs;
   pf_submodule_diff_t submodule_diffs[PNET_MAX_SUBSLOTS];
} pf_module_diff_t;

typedef struct pf_api_diff
{
   uint32_t api;
   uint16_t nbr_module_diffs;
   pf_module_diff_t module_diffs[PNET_MAX_SLOTS];
} pf_api_diff_t;

typedef struct pf_parameter_server_properties
{
   uint32_t reserved; /** Currently unused */
} pf_parameter_server_properties_t;

typedef struct pf_prm_server
{
   bool valid;
   pf_uuid_t parameter_server_object_uuid;
   pf_parameter_server_properties_t parameter_server_properties; /** Allowed: 0
                                                                  */
   uint16_t cm_initiator_activity_timeout_factor;                /** res: 100ms,
                                                                    allowed 1..1000 */
   uint16_t parameter_server_station_name_len;
   char parameter_server_station_name[PNET_STATION_NAME_MAX_SIZE]; /** Terminated
                                                                      string */
} pf_prm_server_t;

typedef struct pf_address_resolution_properties
{
   uint8_t protocol;          /** pf_protocol_values_t */
   uint8_t resolution_factor; /** range: 1..0x64 */
} pf_address_resolution_properties_t;

typedef struct pf_multicast_cr
{
   bool valid;
   uint16_t iocr_reference;
   pf_address_resolution_properties_t address_resolution_properties;
   uint16_t mci_timeout_factor; /* MCI_timeout range 0x0001..0xffff */
   uint16_t length_provider_station_name;
   char provider_station_name[PNET_STATION_NAME_MAX_SIZE]; /** Terminated */
} pf_multicast_cr_t;

typedef struct pf_ar_rpc_request
{
   bool valid;                         /** Requested in connect.req */
   uint16_t initiator_rpc_server_port; /** Allowed 0x0000..0x03ff (optional),
                                          0x0400..0xbfff (usable),
                                          0xc000..0xffff (recommended) */
} pf_ar_rpc_request_t;

typedef struct pf_ar_rpc_result
{
   uint16_t responder_rpc_server_port; /** Allowed 0x0000..0x03ff (optional),
                                          0x0400..0xbfff (usable),
                                          0xc000..0xffff (recommended) */
} pf_ar_rpc_result_t;

typedef struct pf_subframe_data
{
   uint8_t subframe_position;
   uint8_t subframe_length;
} pf_subframe_data_t;

typedef struct pf_dfp_iocr
{
   uint16_t iocr_reference;
   uint16_t subframe_offset; /** Range: 0..0x59B */
   pf_subframe_data_t subframe_data;
} pf_dfp_iocr_t;

typedef struct pf_ir_info
{
   bool valid;
   pf_uuid_t ir_data_uuid;
   uint16_t nbr_iocrs; /** Allowed 0, 2 */
   pf_dfp_iocr_t dfp_iocrs[PNET_MAX_DFP_IOCR];
} pf_ir_info_t;

typedef struct pf_sr_info
{
   bool valid;
   uint16_t redundancy_data_hold_factor; /** res: 1ms, range: 1..0xC8
                                            (optional), 0xc9..0xffff (mandatory)
                                          */
   struct
   {
      uint8_t input_valid_on_backup; /** Range: 0..1 */
      uint8_t mode;                  /** Range: 0..1 */
   } sr_properties;
} pf_sr_info_t;

typedef struct pf_rs_info
{
   bool valid;
   uint8_t rs_alarm_transport_mode; /** Range 0 (read by record), 1 (using alarm
                                       reporting) */
} pf_rs_info_t;

typedef struct pf_ar_fsu
{
   bool valid;
   struct
   {
      uint32_t fs_parameter_mode; /** Range 0..1 (ON, OFF) */
      pf_uuid_t fs_parameter_uuid;
   } fs_parameter_block;
#if PNET_MAX_MAN_SPECIFIC_FAST_STARTUP_DATA_LENGTH
   uint16_t length_manufacturer_specific_fast_startup_data;
   uint8_t manufacturer_specific_fast_startup_data
      [PNET_MAX_MAN_SPECIFIC_FAST_STARTUP_DATA_LENGTH];
#endif
} pf_ar_fsu_t;

typedef struct pf_ar_server
{
   uint16_t length_cm_responder_station_name;
   char cm_responder_station_name[PNET_STATION_NAME_MAX_SIZE]; /** Terminated */
} pf_ar_server_t;

typedef struct pf_alarm_cr_properties
{
   bool priority;      /** pf_priority_values_t */
   bool transport_udp; /** false ==> PF_RT_CLASS_1, true ==> PF_RT_CLASS_UDP */
} pf_alarm_cr_properties_t;

typedef struct pf_alarm_cr_tag_header
{
   uint8_t vlan_id;             /** Bits 0..11. Allowed: No VLAN (0) */
   uint8_t alarm_user_priority; /** Bits 12..15. Allowed: USER_PRIORITY_LOW (5),
                                   USER_PRIORITY_HIGH (6) */
} pf_alarm_cr_tag_header_t;

typedef struct pf_alarm_cr_request
{
   bool valid;
   uint32_t alarm_cr_type; /** Allowed: 0x0001 */
   uint16_t lt_field;      /** Allowed: 0x8892 */
   pf_alarm_cr_properties_t alarm_cr_properties;
   uint16_t rta_timeout_factor;    /** res: 100ms, allowed: 1..100 (mandatory),
                                      101..0xffff (optional) */
   uint16_t rta_retries;           /** Allowed: 3..15 */
   uint16_t local_alarm_reference; /** Controller reference */
   uint16_t max_alarm_data_length; /** Allowed 200..1432 */
   pf_alarm_cr_tag_header_t alarm_cr_tag_header_high; /** USER_PRIORITY_HIGH (6)
                                                       */
   pf_alarm_cr_tag_header_t alarm_cr_tag_header_low;  /** USER_PRIORITY_LOW (5)
                                                       */
} pf_alarm_cr_request_t;

typedef struct pf_alarm_cr_result
{
   uint16_t alarm_cr_type;          /** pf_alarm_cr_type_values_t (0x0001) */
   uint16_t remote_alarm_reference; /** Our reference */
   uint16_t max_alarm_data_length;  /** Range: 200..1432 */
} pf_alarm_cr_result_t;

#if PNET_OPTION_AR_VENDOR_BLOCKS
typedef struct pf_ar_vendor_request
{
   bool valid;
   uint32_t ap_structure_identifier; /** 0..7fff (vendor specific), 0x8000
                                        (extended identification rules),
                                        0x8001..0xffff (administrative number)
                                      */
   uint32_t api;
   uint16_t length_data_request;
   uint8_t data_request[PNET_MAX_AR_VENDOR_BLOCK_DATA_LENGTH]; /** Not
                                                                  terminated. */
} pf_ar_vendor_request_t;

typedef struct pf_ar_vendor_result_t
{
   uint16_t length_data_response;
   uint8_t data_response[PNET_MAX_AR_VENDOR_BLOCK_DATA_LENGTH]; /** Not
                                                                   terminated.
                                                                 */
} pf_ar_vendor_result_t;
#endif

typedef struct pf_ppm
{
   pf_ppm_state_values_t state;
   uint32_t next_exec; /* Next timestamp when a frame should be sent, in
                          microseconds */

   int errline;     /* Not yet used */
   uint32_t errcnt; /* Not yet used */

   pnet_ethaddr_t sa; /* Source MAC address */
   pnet_ethaddr_t da; /* Destination MAC address (IO-controller) */

   bool first_transmit; /* True if first transmission has been done */

   void * p_send_buffer; /* Output buffer with Ethernet header etc */
   bool new_buf;         /* Not yet used */

   uint16_t cycle; /* Cycle counter, in tics each 31.25 us (thus 16 tics per
                      ms). */
   uint8_t transfer_status;
   uint8_t data_status; /* Valid or invalid, run or stop, redundancy, problem
                           indicator etc */
   uint16_t buffer_length;
   uint16_t buffer_pos;           /* Start position of data in frame */
   uint16_t cycle_counter_offset; /* Start position of cycle counter in frame */
   uint16_t data_status_offset;   /* Start position of data status in frame */
   uint16_t transfer_status_offset; /* Start position of transfer status in
                                       frame */

   uint8_t buffer_data[PF_FRAME_BUFFER_SIZE]; /* Max */

   uint32_t trx_cnt; /* Number of frames sent */

   uint16_t send_clock_factor; /* Resolution: 31.25us, Allowed: 1..128, Default:
                                  32 */
   uint16_t reduction_ratio;   /* Allowed: 1..512 */
   uint32_t control_interval;  /* Period in microseconds between frames */
   bool ci_running;   /* True if the timer is running. Used for stopping
                         transmission before next scheduled sending.  */
   uint32_t ci_timer; /* Scheduler timeout instance. UINT32_MAX when stopped */
} pf_ppm_t;

typedef struct pf_cpm
{
   pf_cpm_state_values_t state;
   uint32_t max_exec;

   int errline;
   uint32_t errcnt;

   uint32_t recv_cnt;
   uint32_t free_cnt;

   pnet_ethaddr_t sa; /* Mac of the controller */

   uint16_t nbr_frame_id; /* 1 or 2 */
   uint16_t frame_id[2];  /* 2 needed for some instances of RT_CLASS_3 */
   uint16_t data_hold_factor;

   void * p_buffer_app;   /* Owned by app */
   void * p_buffer_cpm;   /* owned by cpm */
   bool new_buf;          /* New data to be received */
   uint16_t frame_id_pos; /* Handles VLAN in ETH header */

   uint8_t data_status;
   uint16_t buffer_length;
   uint16_t buffer_pos; /* Start of PROFINET data in frame */

   uint16_t dht; /* Set to zero at incoming cyclic frame, increased by
                    pf_cpm_control_interval_expired() */
   bool new_data;
   uint32_t rxa[PNET_MAX_PORT][2]; /* Max 2 frame_ids */
   int32_t cycle;                  /* value -1 means "never" */

   uint32_t control_interval;
   bool ci_running;
   uint32_t ci_timer;

   /* CMIO data */
   bool cmio_start; /* cmInstance.start/stop */
} pf_cpm_t;

typedef struct pf_iodata_object
{
   bool in_use;
   uint32_t api_id;
   uint16_t slot_nbr;
   uint16_t subslot_nbr;

   /* Individual sub-slot data */
   uint16_t data_offset;
   uint16_t data_length;
   /* The provider status */
   uint16_t iops_offset;
   uint16_t iops_length;
   /* The consumer status */
   uint16_t iocs_offset;
   uint16_t iocs_length;

   bool data_avail;

#if PNET_MAX_DFP_IOCR > 0
   pf_subframe_data_t subframe_data[PNET_MAX_DFP_IOCR];
#endif
} pf_iodata_object_t;

/* Forward declaration of some useful types */
struct pf_ar;
struct pf_alpmx;

/* IOCR = IO Communication Relation */
typedef struct pf_iocr
{
   struct pf_ar * p_ar;
   uint32_t crep; /* Index to self in pf_ar_t  ->iocr array  */

   pf_cpm_t cpm; /* Handles output */
   pf_ppm_t ppm; /* Handles input  */

   uint16_t mppm;
   uint16_t cmdmc;
   uint16_t dfp;

   /* Total frame content (data, IOCS and IOPS) length after L2 header
    * (buffer_pos) */
   uint16_t in_length;
   uint16_t out_length;

   uint16_t nbr_data_desc;
   pf_iodata_object_t
      data_desc[PNET_MAX_API * PNET_MAX_SLOTS * PNET_MAX_SUBSLOTS];

   pf_iocr_param_t param;   /* From connect.req */
   pf_iocr_result_t result; /* From connect.ind */
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
   uint16_t frame_id_pos;
   pnal_buf_t * p_buf;
} pf_apmr_msg_t;

/*
 * This type contains all the information needed for one
 * APMS/APMR pair.
 */
typedef struct pf_apmx
{
   struct pf_ar * p_ar;
   struct pf_alpmx * p_alpmx;

   pnet_ethaddr_t da; /* Destination MAC address (IO-controller) */

   uint16_t src_ref; /* Our ref */
   uint16_t dst_ref; /* Controller local_alarm_reference */

   pf_apms_state_values_t apms_state;
   pf_apmr_state_values_t apmr_state;

   uint16_t send_seq_count;
   uint16_t send_seq_count_o;

   uint16_t exp_seq_count;
   uint16_t exp_seq_count_o;

   /* The receive queue */
   os_mbox_t * p_alarm_q;
   /* The messages sent via the mailbox */
   pf_apmr_msg_t apmr_msg[PNET_MAX_ALARMS];
   uint16_t apmr_msg_nbr;

   /* Latest sent alarm */
   pnal_buf_t * p_rta;

   bool high_priority; /* True for high priority APMX. For printouts. */
   uint16_t vlan_prio; /* 5 or 6 */
   uint16_t block_type_alarm_notify;
   uint16_t block_type_alarm_ack;
   uint16_t frame_id;

   uint32_t timeout_us;
   uint32_t timeout_id; /* Scheduler handle for Alarm retransmission */
   uint32_t retry;
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
   pnet_ethaddr_t da; /* Destination MAC address (IO-controller) */

   /*
    * The sequence number is used for PULL/PLUG requests.
    * It is updated by the alarm component and checked by the CMDEV component.
    */
   uint16_t prev_sequence_number;
   uint16_t sequence_number;

   pf_alpmi_state_values_t alpmi_state;
   pf_alpmr_state_values_t alpmr_state;
} pf_alpmx_t;

typedef enum pf_get_result
{
   PF_PARSE_OK,
   PF_PARSE_NULL_POINTER, /* Input buffer is NULL */
   PF_PARSE_END_OF_INPUT, /* Unexpected */
   PF_PARSE_OUT_OF_API_RESOURCES,
   PF_PARSE_OUT_OF_EXP_SUBMODULE_RESOURCES,
   PF_PARSE_ERROR /* Other */
} pf_get_result_t;

typedef struct pf_get_info
{
   pf_get_result_t result;
   bool is_big_endian;
   const uint8_t * p_buf;
   uint16_t len;
} pf_get_info_t;

/*
 * A session stores information used for supervision of connection activity.
 * A session is allocated for each connect in order to handle fragmented RPC
 * requests. In order to find this session A session is also needed for each
 * CCONTROL request. Use pf_session_locate_by_uuid.
 */
typedef struct pf_session_info
{
   uint16_t ix;
   bool in_use;
   bool release_in_progress; /* The session handles an incoming release request
                              */
   bool kill_session; /* On error or when done. This will kill the session at
                         the end of handling the incoming RPC frame. */
   int socket;
   pnal_eth_handle_t * eth_handle;
   struct pf_ar * p_ar; /* Parent AR */
   bool from_me;        /* True if the session originates from the device (i.e.
                           CControl requests and responses). */
   pf_uuid_t activity_uuid;
   uint32_t ip_addr;
   pnal_ipport_t port; /* Source port on incoming message, destination port on
                        outgoing message */
   uint32_t sequence_nmb_send; /* rm_ccontrol_req */

   /*
    * Small devices should be able to cope with a single Ethernet frame.
    * Large devices may however require considerable longer Connect
    * Requests/responses, Reads responses and Write requests may also require
    * longer buffers. These are sent/received via fragmented RPC
    * requests/responses. Allocate buffers to handle these large request here.
    */
   uint8_t in_buffer[PNET_MAX_SESSION_BUFFER_SIZE]; /* Typically request buffer
                                                     */
   uint16_t in_buf_len;
   uint16_t in_fragment_nbr;

   uint8_t out_buffer[PNET_MAX_SESSION_BUFFER_SIZE]; /* Typically response
                                                        buffer */
   uint16_t out_buf_len;
   uint16_t out_buf_sent_pos; /* Number of bytes sent so far */
   uint16_t out_buf_send_len; /* Size of current packet to send */
   uint16_t out_fragment_nbr;

   pf_get_info_t get_info;
   bool is_big_endian; /* From rpc_header_t in first fragment */
   pnet_result_t rpc_result;
   pf_ndr_data_t ndr_data;

   /* This item is used to handle dcontrol re-runs */
   uint32_t dcontrol_sequence_nmb; /* From dcontrol request */
   pnet_result_t dcontrol_result;

   /* This timer is used to handle ccontrol and fragment re-transmissions */
   uint32_t resend_timeout; /* Handle for removing scheduled event */
   uint32_t resend_timeout_ctr;
} pf_session_info_t;

typedef struct pf_ar
{
   bool in_use;
   uint16_t arep;

   pf_session_info_t * p_sess; /* Incoming session, not the outgoing CControl
                                  session. Use pf_session_locate_by_ar() to find
                                  the CControl session. */

   pf_cmdev_state_values_t cmdev_state; /* pf_cmdev_state_values_t */
   pnet_ethaddr_t src_addr;             /* Connect client MAC address */

   uint16_t nbr_ar_param;
   pf_ar_param_t ar_param;   /* From connect.req */
   pf_ar_result_t ar_result; /* From connect.ind */

   uint16_t nbr_ar_server;
   pf_ar_server_t ar_server; /* From connect.ind */

   uint16_t nbr_alarm_cr;                  /* 0 or 1 */
   pf_alarm_cr_request_t alarm_cr_request; /* From connect.req */
   pf_alarm_cr_result_t alarm_cr_result;   /* From connect.ind */

   bool alarm_enable;
   /* Alarm handling ALPMI/ALPMR machines: one for LOW (0) prio and one for HIGH
    * (1) prio. */
   pf_alpmx_t alpmx[2];
   /* Alarm handling APMS/APMR machines: one for LOW (0) prio and one for HIGH
    * (1) prio. */
   pf_apmx_t apmx[2];

   /* Alarm queues for outgoing alarms: one for LOW (0) prio and one for HIGH
    * (1) prio. */
   pf_alarm_queue_t alarm_send_q[2];

   uint16_t nbr_ar_rpc;
   pf_ar_rpc_request_t ar_rpc_request; /* From connect.req */
   pf_ar_rpc_result_t ar_rpc_result;   /* From connect.ind */

   uint16_t nbr_iocrs; /* From connect.req */
   pf_iocr_t iocrs[PNET_MAX_CR];

   uint16_t nbr_exp_apis;
   pf_exp_api_t exp_apis[PNET_MAX_API]; /* From connect.req */

   uint16_t nbr_api_diffs; /* From connect.ind */
   pf_api_diff_t api_diffs[PNET_MAX_API];

   uint16_t nbr_prm_server;
   uint16_t nbr_rpc_server;
   uint16_t nbr_ir_info;

   /* Used for error discovery in connect RPC */
   uint16_t input_cr_cnt;
   uint16_t output_cr_cnt;
   uint16_t mcr_cons_cnt;
   bool rtc3_present;

   /* Internal state */
   uint16_t remote_application_ready_timeout; /** res: 1s, allowed: 300 */
   uint16_t ar_state;                         /** pf_ar_state_values_t */
   uint16_t sync_state;                       /** pf_sync_state_values_t */
   bool ready_4_data;

   /* Global error codes */
   uint8_t err_cls;
   uint8_t err_code;

   pf_cmsm_state_values_t cmsm_state;
   uint32_t cmsm_timer;

   pf_cmio_state_values_t cmio_state;
   uint32_t cmio_timer;
   bool cmio_timer_run;

   pf_cmpbe_state_values_t cmpbe_state;
   uint16_t cmpbe_stored_command;

   /* Optional data */
#if PNET_OPTION_MC_CR
   uint16_t nbr_mcr;
   pf_multicast_cr_t mcr_request[PNET_MAX_MC_CR];
#endif
#if PNET_OPTION_PARAMETER_SERVER
   pf_prm_server_t prm_server;
#endif
#if PNET_OPTION_IR
   pf_ir_info_t ir_info;
#endif
#if PNET_OPTION_SR
   pf_sr_info_t sr_info;
#endif
#if PNET_OPTION_REDUNDANCY
   struct
   {
      uint8_t end_point_1; /** pf_end_point_1_values_t */
      uint8_t end_point_2; /** pf_end_point_2_values_t */
   } redundancy_info;
#endif
#if PNET_OPTION_AR_VENDOR_BLOCKS
   uint16_t nbr_ar_vendor;
   pf_ar_vendor_request_t ar_vendor_request[PNET_MAX_AR_VENDOR_BLOCKS];
   pf_ar_vendor_result_t ar_vendor_result[PNET_MAX_AR_VENDOR_BLOCKS];
#endif
#if PNET_OPTION_RS
   pf_rs_info_t rs_info;
#endif
#if PNET_OPTION_FAST_STARTUP
   pf_ar_fsu_t fast_startup_data;
#endif
} pf_ar_t;

/*
 * ============= Plugable typedefs ==================
 */

typedef enum pf_submod_plug_state
{
   PF_SUBMOD_PLUG_OK,
   PF_SUBMOD_PLUG_SUBSTITUTE, /** module_state == SUBSTITUTE */
   PF_SUBMOD_PLUG_WRONG,      /** module_state == WRONG_MODULE */
   PF_SUBMOD_PLUG_NO          /** module_state == WRONG_MODULE */
} pf_submod_plug_state_t;

typedef enum pf_submod_owner
{
   PF_SUBMOD_OWNER_FREE,
   PF_SUBMOD_OWNER_SOL, /* Superordinator locked */
   PF_SUBMOD_OWNER_IOS, /* Owned by IO supervisor */
   PF_SUBMOD_OWNER_IOC, /* Owned by IO controller */
} pf_submod_owner;

typedef enum pf_mod_plug_state
{
   PF_MOD_PLUG_NO_MODULE,     /** No module plugged into slot. */
   PF_MOD_PLUG_WRONG_MODULE,  /** Module/submodule ident number(s) do not match
                                 cfg. */
   PF_MOD_PLUG_PROPER_MODULE, /** All OK. Submodule may have diff. */
   PF_MOD_PLUG_SUBSTITUTE /** There is another module plugged into the slot. */
} pf_mod_plug_state_t;

typedef enum pf_submod_add_info
{
   PF_SUBMOD_ADD_INFO_NONE,
   PF_SUBMOD_ADD_INFO_TAKEOVER_NOT_ALLOWED
} pf_submod_add_info_t;

typedef enum pf_submod_ar_info
{
   PF_SUBMOD_AR_INFO_OWN,                       /** The AR is owner */
   PF_SUBMOD_AR_INFO_APPLICATION_READY_PENDING, /** Parameterization in progress
                                                 */
   PF_SUBMOD_AR_INFO_SUPERORDINATED_LOCKED, /** Locked by application (ToDo: API
                                               for that) */
   PF_SUBMOD_AR_INFO_LOCKED_BY_IO_CONTROLLER, /** Locked by other IO controller
                                               */
   PF_SUBMOD_AR_INFO_LOCKED_BY_IO_SUPERVISOR  /** Locked by other IO Supervisor
                                               */
} pf_submod_ar_info_t;

typedef struct pf_submod_state
{
   pf_submod_add_info_t add_info;     /** Bits 0..2 */
   bool advice_avail;                 /** Bit 3 */
   bool maintenance_required;         /** Bit 4 */
   bool maintenance_demanded;         /** Bit 5 */
   bool fault;                        /** Bit 6 */
   pf_submod_ar_info_t ar_info;       /** Bits 7..10 */
   pf_submod_plug_state_t ident_info; /** Bits 11..14 */
   bool format_indicator;             /** Bit 15: Always 1 (true) */
} pf_submod_state_t;

typedef enum pf_usi_values
{
   /* 0x0000..0x7fff    Manufacturer specific */
   PF_USI_CHANNEL_DIAGNOSIS = 0x8000,
   PF_USI_ALARM_MULTIPLE = 0x8001,
   PF_USI_EXTENDED_CHANNEL_DIAGNOSIS = 0x8002,
   PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS = 0x8003,
   /* 0x8004..0x80ff    Reserved */
   PF_USI_MAINTENANCE = 0x8100,
   /* 0x8101..0x81ff    Reserved */
   PF_USI_UPLOAD_AND_RETRIEVAL = 0x8200,
   PF_USI_IPARAMETER = 0x8201,
   /* 0x8202..0x82ff    Reserved */
   PF_USI_RS_LOW_WATERMARK = 0x8300,
   PF_USI_RS_TIMEOUT = 0x8301,
   PF_USI_RS_OVERFLOW = 0x8302,
   PF_USI_RS_EVENT = 0x8303,
   /* 0x8304..0x8309    Reserved */
   PF_USI_PE_ENERGY_SAVING_STATUS = 0x8310,
   /* 0x8311..0x8319    Reserved */
   PF_USI_PROCESS_ALARM_REASON = 0x8320,
   /* 0x8321..0x8fff    Reserved */
   /* 0x9000..0x9fff    Reserved for profiles */
   /* 0xa000..0xffff    Reserved */
} pf_usi_values_t;

/*
 * The diag filter is used when reading record data in CMRDR.
 */
typedef enum pf_diag_filter_level
{
   PF_DIAG_FILTER_FAULT_STD, /* Insert all non-manufacturer specific */
   PF_DIAG_FILTER_FAULT_ALL, /* Manufacturer specific or fault */
   PF_DIAG_FILTER_ALL,       /* Insert all diagnosis */
   PF_DIAG_FILTER_M_REQ,     /* Manufacturer specific or maintenance required */
   PF_DIAG_FILTER_M_DEM      /* Manufacturer specific or maintenance demanded */
} pf_diag_filter_level_t;

typedef struct pf_subslot
{
   bool in_use;
   uint16_t subslot_nbr;

   /* Submodule plug information */
   uint32_t exp_submodule_ident_number;
   uint32_t submodule_ident_number;
   uint16_t length_input;
   uint16_t length_output;
   pnet_submodule_dir_t direction;

   /* Run-time information */
   pf_ar_t * p_ar;
   pf_submod_state_t submodule_state;

   /* The following members shall be protected by the device.diag_mutex. */
   /*
    * This is an index into device.diag_items[].
    * It points to the list of reported diag alarms for this specific sub-slot.
    *
    * Each subslot has its own list of diagnosis items.
    */
   uint16_t diag_list;
} pf_subslot_t;

typedef struct pf_slot
{
   bool in_use;
   /* Module plug information */
   uint32_t exp_module_ident_number;
   uint32_t module_ident_number;
   pf_mod_plug_state_t plug_state;
   pf_subslot_t subslots[PNET_MAX_SUBSLOTS];

   /* Run-time information */
   pf_ar_t * p_ar;
   uint16_t slot_nbr;
} pf_slot_t;

typedef struct pf_api
{
   bool in_use;
   uint32_t api_id;

   pf_slot_t slots[PNET_MAX_SLOTS];

   pf_ar_t * p_ar;
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
   pf_api_t apis[PNET_MAX_API];

   /*
    * This is the pool of diag items.
    * It is used instead of dynamic memory to avoid fragmentation.
    *
    * Each subslot uses its own list of diag items, and stores the index to
    * the head of its list.
    */
   os_mutex_t * diag_mutex; /* Protect the diag items */
   pf_diag_item_t diag_items[PNET_MAX_DIAG_ITEMS];
   uint16_t diag_items_free; /* Head of the unused list */
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
   uint16_t sequence_number;
   pf_uuid_t ar_uuid;
   uint32_t api;
   uint16_t slot_number;
   uint16_t subslot_number;
   uint8_t padding[2];
   uint16_t index;
   uint32_t record_data_length;
   pf_uuid_t target_ar_uuid; /* Only used if implicit AR */
   uint8_t rw_padding[8];
} pf_iod_read_request_t;

typedef struct pf_iod_read_result
{
   uint16_t sequence_number;
   pf_uuid_t ar_uuid;
   uint32_t api;
   uint16_t slot_number;
   uint16_t subslot_number;
   uint8_t padding[2];
   uint16_t index;
   uint32_t record_data_length;
   uint16_t add_data_1;
   uint16_t add_data_2;
   uint8_t rw_padding[20];
} pf_iod_read_result_t;

typedef struct pf_iod_write_request /* Write from dcontrol */
{
   uint16_t sequence_number;
   pf_uuid_t ar_uuid;
   uint32_t api;
   uint16_t slot_number;
   uint16_t subslot_number;
   uint8_t padding[2];
   uint16_t index;
   uint32_t record_data_length;
   uint8_t rw_padding[24];
} pf_iod_write_request_t;

typedef struct pf_iod_write_result
{
   uint16_t sequence_number;
   pf_uuid_t ar_uuid;
   uint32_t api;
   uint16_t slot_number;
   uint16_t subslot_number;
   uint8_t padding[2];
   uint16_t index;
   uint32_t record_data_length;
   uint16_t add_data_1;
   uint16_t add_data_2;
   pnet_pnio_status_t pnio_status;
   uint8_t rw_padding[16];
} pf_iod_write_result_t;

/* ============= LogBook typedefs ================== */
/* block_version_low == 1 */

typedef enum pf_ts_status_values
{
   PF_TS_STATUS_GLOBAL = 0,
   PF_TS_STATUS_LOCAL = 1,
   PF_TS_STATUS_LOCAL_ARB = 2, /* Arbitrary time scale */
} pf_ts_status_values_t;

typedef struct pf_log_book_ts
{
   uint16_t status; /* pf_ts_status_values_t */
   uint16_t sec_hi;
   uint32_t sec_lo;
   uint32_t nano_sec;
} pf_log_book_ts_t;

/*
 * p-net stack specific log book entry detail values.
 * Values are not defined by Profinet specification
 * but device/stack specific.
 * See PN-AL-protocol (Mar20) 7.3.6 LogBook ASE
 */
typedef enum
{
   PF_LOG_BOOK_AR_CONNECT = 1,
   PF_LOG_BOOK_AR_RELEASE,
   PF_LOG_BOOK_DCONTROL
} pf_log_book_entry_detail_values_t;

typedef struct pf_log_book_entry
{
   pf_log_book_ts_t time_ts;
   pf_uuid_t ar_uuid;
   pnet_pnio_status_t pnio_status;
   uint32_t entry_detail;
} pf_log_book_entry_t;

typedef struct pf_log_book
{
   pf_log_book_ts_t time_ts; /* Local timestamp when reading the logbook */
   pf_log_book_entry_t entries[PNET_MAX_LOG_BOOK_ENTRIES];
   uint16_t put; /* Points to oldest entry if wrap */
   bool wrap;    /* All entries valid */
} pf_log_book_t;

/**
 * System location (sysLocation).
 *
 * "The physical location of this node (e.g.,
 * 'telephone closet, 3rd floor'). If the location is unknown,
 *  the value is the zero-length string."
 * - IETF RFC 3418 (SNMP MIB-II).
 *
 * The value is supplied by network manager. By default, it is
 * the zero-length string.
 *
 * This should have the same value as "IM_Tag_Location" in I&M1.
 * See PN-Topology ch. 11.5.2: "Consistency".
 *
 * Note: According to the SNMP specification, the string could be up
 * to 255 characters. The p-net stack limits it to the length of
 * IM_Tag_Location, which is 22.
 * An extra byte is added as to ensure null-termination.
 *
 * This is a writable variable. As such, it is stored in persistent memory.
 * Only writable variables (using SNMP Set) need to be stored
 * in persistent memory.
 * See IEC CDV 61158-5-10 (PN-AL-Services) ch. 7.3.3.3.6.2: "Persistency".
 */
typedef struct pf_snmp_system_location
{
   char string[PNET_LOCATION_MAX_SIZE]; /* Terminated string */
} pf_snmp_system_location_t;

/**
 *
 * Substitution name: PDPortDataCheck
 * BlockHeader, Padding, Padding, SlotNumber, SubslotNumber, { [CheckPeers],
 * [CheckLineDelay], [CheckMAUType a], [CheckLinkState], [CheckSyncDifference],
 * [CheckMAUTypeDifference], [CheckMAUTypeExtension] b }
 *
 */
typedef struct pf_port_data_check
{
   uint16_t slot_number;
   uint16_t subslot_number;
   pf_block_header_t block_header;
} pf_port_data_check_t;

/**
 * Substitution name : CheckPeers
 * BlockHeader, NumberOfPeers, (LengthPeerPortName, PeerPortName,
 * LengthPeerStationName, PeerStationName)*, [Padding*]
 *
 * CheckPeers contains an array of peers to check
 */
typedef struct pf_check_peer
{
   uint8_t length_peer_port_name;
   uint8_t peer_port_name[PNET_LLDP_PORT_ID_MAX_SIZE]; /** Terminated */
   uint8_t length_peer_station_name;
   uint8_t peer_station_name[PNET_STATION_NAME_MAX_SIZE]; /** Terminated */
} pf_check_peer_t;

typedef struct pf_check_peers
{
   uint8_t number_of_peers;
   pf_check_peer_t peers[PF_CHECK_PEERS_PER_PORT];
} pf_check_peers_t;

/**
 * Substitution name: PDPortDataAdjust
 * BlockHeader, Padding, Padding, SlotNumber, SubslotNumber, {
 * [AdjustDomainBoundary], [AdjustMulticastBoundary], [AdjustMAUType ^
 * AdjustLinkState], [AdjustPeerToPeerBoundary], [AdjustDCPBoundary],
 * [AdjustPreambleLength], [AdjustMAUTypeExtension] a }
 */
typedef struct pf_port_data_adjust
{
   uint16_t slot_number;
   uint16_t subslot_number;
   pf_block_header_t block_header;
} pf_port_data_adjust_t;

/**
 * Substitution name: PeerToPeerBoundary
 * Table 722
 */
typedef struct pf_peer_to_peer_boundary
{
   uint32_t do_not_send_LLDP_frames : 1; /* 1: The LLDP agent shall not send
                                            LLDP frames */
   uint32_t do_not_send_pctp_delay_request_frames : 1; /* 1: The PTCP ASE shall
                                                          not send PTCP_DELAY
                                                          request frames */
   uint32_t do_not_send_path_delay_request_frames : 1; /* 1: The Time ASE shall
                                                          not send PATH_DELAY
                                                          request frames */
   uint32_t reserved : 29;
} pf_peer_to_peer_boundary_t;

/**
 * Substitution name: AdjustPeerToPeerBoundary
 * BlockHeader, Padding, Padding, PeerToPeerBoundary, AdjustProperties,
 * [Padding*]
 */
typedef struct pf_port_data_adjust_peer_to_peer_boundary
{
   pf_peer_to_peer_boundary_t peer_to_peer_boundary;
   uint16_t adjust_properties; /* Always 0, See PN-AL-protocol (Mar20)
                                  Ch.5.2.13.14 */
} pf_adjust_peer_to_peer_boundary_t;

/* See Profinet 2.4 section 5.2.28 and appendix W */
typedef struct pf_interface_stats
{
   uint32_t if_in_octets;
   uint32_t if_out_octets;
   uint32_t if_in_discards;
   uint32_t if_out_discards;
   uint32_t if_in_errors;
   uint32_t if_out_errors;
} pf_interface_stats_t;

/**
 * Link status
 *
 * See IEEE 802.1AB-2005 (LLDPv1) Annex G.2 "MAC/PHY Configuration/Status TLV".
 */
typedef struct pf_lldp_link_status
{
   bool is_autonegotiation_supported;
   bool is_autonegotiation_enabled;
   uint16_t autonegotiation_advertised_capabilities;
   uint16_t operational_mau_type;
   bool is_valid;
} pf_lldp_link_status_t;

/**
 * Port description
 *
 * "The port description field shall contain an alpha-numeric string
 * that indicates the port's description. If RFC 2863
 * is implemented, the ifDescr object should be used for this field."
 * - IEEE 802.1AB (LLDP) ch. 9.5.5.2 "port description".
 *
 * Note: According to the SNMP specification, the string could be up
 * to 255 characters (excluding termination). The p-net stack limits it to
 * PNET_MAX_PORT_DESCRIPTION_SIZE (including termination).
 */
typedef struct pf_lldp_port_description
{
   char string[PNET_MAX_PORT_DESCRIPTION_SIZE]; /* Terminated string */
   bool is_valid;
   size_t len;
} pf_lldp_port_description_t;

/**
 * Port list
 *
 * "Each octet within this value specifies a set of eight ports,
 * with the first octet specifying ports 1 through 8, the second
 * octet specifying ports 9 through 16, etc. Within each octet,
 * the most significant bit represents the lowest numbered port,
 * and the least significant bit represents the highest numbered
 * port. Thus, each port of the system is represented by a
 * single bit within the value of this object. If that bit has
 * a value of '1' then that port is included in the set of ports;
 * the port is not included if its bit has a value of '0'."
 * - IEEE 802.1AB (LLDP) ch. 12.2 "LLDP MIB module" (lldpPortList).
 *
 * Se also section about lldpConfigManAddrTable in PN-Topology.
 */
typedef struct pf_lldp_port_list_t
{
   uint8_t ports[2];
} pf_lldp_port_list_t;

/**
 * Port iterator
 *
 * This iterator may be used for iterating over all physical ports
 * on the local interface. The management port is not included.
 */
typedef struct pf_port_iterator
{
   int next_port;
} pf_port_iterator_t;

/**
 * Chassis ID
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.2.3 "chassis ID".
 */
typedef struct pf_lldp_chassis_id
{
   char string[PNET_LLDP_CHASSIS_ID_MAX_SIZE]; /**< Terminated string */
   uint8_t subtype;                            /* PF_LLDP_SUBTYPE_xxx */
   bool is_valid;
   size_t len;
} pf_lldp_chassis_id_t;

/**
 * Port ID
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.3 "Port ID TLV".
 */
typedef struct pf_lldp_port_id
{
   char string[PNET_LLDP_PORT_ID_MAX_SIZE]; /**< Terminated string */
   uint8_t subtype;
   bool is_valid;
   size_t len;
} pf_lldp_port_id_t;

/**
 * Interface number
 *
 * "The interface number field shall contain the assigned number within
 * the system that identifies the specific interface associated with this
 * management address. If the value of the interface subtype is unknown,
 * this field shall be set to zero."
 * - IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.9 "Management Address TLV".
 *
 * Also see PN-Topology ch. 6.5.1 "Mapping of Ports and PROFINET Interfaces
 * in LLDP MIB and MIB-II".
 */
typedef struct pf_lldp_interface_number
{
   int32_t value;
   uint8_t subtype; /* 1 = unknown, 2 = ifIndex, 3 = systemPortNumber */
} pf_lldp_interface_number_t;

/**
 * Management address
 *
 * Usually, this is an IPv4 address. It may also be a MAC address
 * or other kinds of addresses, such as IPv6.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.9 "Management Address TLV".
 */
typedef struct pf_lldp_management_address
{
   uint8_t value[31];
   uint8_t subtype;
   size_t len;
   pf_lldp_interface_number_t interface_number;
   bool is_valid;
} pf_lldp_management_address_t;

/**
 * Station name
 *
 * This is the name of an interface. It is called NameOfStation in
 * the Profinet specification. It is usually a string, but may also
 * be a MAC address.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol) ch. 4.3.1.4.16.
 */
typedef struct pf_lldp_station_name
{
   char string[PNET_STATION_NAME_MAX_SIZE]; /** Terminated string */
   size_t len;
} pf_lldp_station_name_t;

/**
 * Measured signal delays in nanoseconds
 *
 * Valid range is 0x1 - 0xFFF.
 * If a signal delay was not measured, its value is zero.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol) ch. 4.11.2.2: "LLDP APDU
 * abstract syntax", element "LLDP_PNIO_DELAY".
 *
 * See also pf_snmp_signal_delay_t.
 */
typedef struct pf_lldp_signal_delay
{
   uint32_t rx_delay_local;
   uint32_t rx_delay_remote;
   uint32_t tx_delay_local;
   uint32_t tx_delay_remote;
   uint32_t cable_delay_local;
   bool is_valid;
} pf_lldp_signal_delay_t;

typedef struct pf_lldp_boundary
{
   bool not_send_LLDP_Frames;
   bool send_PTCP_Delay;
   bool send_PATH_Delay;
   bool reserved_bit4;
   uint8_t reserved_8;
   uint16_t rserved_16;
} pf_lldp_boundary_t;

typedef struct pf_lldp_peer_to_peer_boundary
{
   pf_lldp_boundary_t boundary;
   uint16_t properites;
} pf_lldp_peer_to_peer_boundary_t;

/**
 * LLDP Peer information used by the Profinet stack.
 */
typedef struct pf_lldp_peer_info
{
   /* LLDP TLVs */
   pf_lldp_chassis_id_t chassis_id;
   pf_lldp_port_id_t port_id;
   uint16_t ttl;
   pf_lldp_port_description_t port_description;
   pf_lldp_management_address_t management_address;
   /* PROFIBUS TLVs */
   pf_lldp_signal_delay_t port_delay;
   uint8_t port_status[4];
   pnet_ethaddr_t mac_address;
   uint16_t media_type;
   uint32_t line_delay;
   uint32_t domain_boundary;
   uint32_t multicast_boundary;
   uint8_t link_state_port;
   pf_lldp_link_status_t phy_config;
   pf_lldp_peer_to_peer_boundary_t peer_boundary;
} pf_lldp_peer_info_t;

typedef struct pf_pdport
{
   bool lldp_peer_info_updated;
   bool lldp_peer_timeout;
   struct
   {
      bool active; /* Todo maybe a bitmask for different checks*/
      pf_check_peer_t peer;
   } check;
   struct
   {
      bool active; /* Todo maybe a bitmask for different checks*/
      pf_adjust_peer_to_peer_boundary_t peer_to_peer_boundary;
   } adjust;
} pf_pdport_t;

typedef struct pf_lldp_port
{
   /* LLDP peer information
    *
    * The information may be changed anytime as incoming LLDP packet arrives.
    *
    * Protected by LLDP mutex.
    */
   pf_lldp_peer_info_t peer_info;

   /* Timestamp for when LLDP packet with new content was received.
    *
    * Value is system uptime, in units of 10 milliseconds.
    *
    * Units are the same as sysUptime in SNMP.
    * Protected by LLDP mutex.
    */
   uint32_t timestamp_for_last_peer_change;

   /* Scheduler handle for LLDP receive timeout */
   uint32_t rx_timeout;

   /* Scheduler handle for periodic LLDP sending */
   uint32_t tx_timeout;

   /* Is information about peer device received?
    *
    * Information is received in LLDP packets.
    * Protected by LLDP mutex.
    */
   bool is_peer_info_received;
} pf_lldp_port_t;

/**
 * Port runtime data
 * Note that physical port configuration
 * is part of the configuration (pnet_port_cfg_t)
 */
typedef struct pf_port
{
   uint8_t port_num;
   pf_pdport_t pdport;
   pf_lldp_port_t lldp;
} pf_port_t;

struct pnet
{
   uint32_t pnal_buf_alloc_cnt;
   bool global_alarm_enable;
   os_mutex_t * cpm_buf_lock;
   atomic_int cpm_instance_cnt;
   os_mutex_t * ppm_buf_lock;
   atomic_int ppm_instance_cnt;
   uint16_t dcp_global_block_qualifier;
   pnet_ethaddr_t dcp_sam; /* Source address (MAC) of current DCP remote peer */
   bool dcp_delayed_response_waiting; /* A response to DCP IDENTIFY is waiting
                                         to be sent */
   uint32_t dcp_led_timeout;          /* Handle to the LED timeout instance */
   uint32_t dcp_sam_timeout;          /* Handle to the SAM timeout instance */
   uint32_t dcp_identresp_timeout;    /* Handle to the DCP identify timeout
                                         instance */
   pnal_eth_handle_t * eth_handle;
   pf_eth_frame_id_map_t eth_id_map[PF_ETH_MAX_MAP];
   volatile pf_scheduler_timeouts_t scheduler_timeouts[PF_MAX_TIMEOUTS];
   volatile uint32_t scheduler_timeout_first;
   volatile uint32_t scheduler_timeout_free;
   os_mutex_t * scheduler_timeout_mutex;
   uint32_t scheduler_tick_interval; /* microseconds */
   bool cmdev_initialized;
   pf_device_t cmdev_device;                     /* APIs and diag items */
   pf_cmina_dcp_ase_t cmina_nonvolatile_dcp_ase; /* Reflects what is/should be
                                                    stored in nvm */
   pf_cmina_dcp_ase_t cmina_current_dcp_ase;     /* Reflects current settings
                                                    (possibly not yet commited) */
   pf_cmina_state_values_t cmina_state;
   uint8_t cmina_error_decode;
   uint8_t cmina_error_code_1;
   uint16_t cmina_hello_count;
   uint32_t cmina_hello_timeout;
   bool cmina_commit_ip_suite;
   os_mutex_t * p_cmrpc_rpc_mutex;
   uint32_t cmrpc_session_number;
   pf_ar_t cmrpc_ar[PNET_MAX_AR];                        /* ARs */
   pf_session_info_t cmrpc_session_info[PF_MAX_SESSION]; /* Sessions */
   int cmrpc_rpcreq_socket; /* Main socket for incoming requests */
   uint8_t cmrpc_dcerpc_input_frame[PF_FRAME_BUFFER_SIZE];
   uint8_t cmrpc_dcerpc_output_frame[PF_FRAME_BUFFER_SIZE];
   pf_cmsu_state_values_t cmsu_state;
   pf_cmwrr_state_values_t cmwrr_state;
   const pnet_cfg_t * p_fspm_default_cfg; /* Default configuration from user.
                                             Used at factory reset */
   pnet_cfg_t fspm_cfg; /* Configuration from user. Might be updated by stack
                           during runtime */
   pf_log_book_t fspm_log_book;
   os_mutex_t * fspm_log_book_mutex;

   /* Mutex for protecting access to writable I&M data.
    *
    * Note I&M may be both read and written by SNMP, which executes from
    * its own thread context.
    */
   os_mutex_t * fspm_im_mutex;

   pf_interface_stats_t interface_statistics; /* Keeps track of number of sent
                                                   and discarded packets */

   /* LLDP mutex
    *
    * This mutex protects information about the peer device.
    */
   os_mutex_t * lldp_mutex;

   pf_port_t port[PNET_MAX_PORT];
};

/**
 * @internal
 * Initialise a pnet_t structure into already allocated memory.
 *
 * @param net              InOut: The p-net stack instance to be initialised.
 * @param tick_us          In:    Periodic interval in us. Specify the interval
 *                                between calls to pnet_handle_periodic().
 * @param p_cfg            In:    Profinet configuration. These values are used
 *                                at first startup and at factory reset.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
int pnet_init_only (
   pnet_t * net,
   uint32_t tick_us,
   const pnet_cfg_t * p_cfg);

#ifdef __cplusplus
}
#endif

#endif /* PF_TYPES_H */
