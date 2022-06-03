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
 * @brief Public API for Profinet stack.
 *
 * # HW- and OS-specifics
 *
 * The Profinet stack depends on some OS and device-specific functions, which
 * must be implemented by the application.
 *
 * The API for these functions is defined in osal.h.
 */

#ifndef PNET_API_H
#define PNET_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pnet_export.h"
#include "pnet_options.h"
#include "pnet_version.h"
#include "pnal_config.h"
#if PNET_OPTION_DRIVER_ENABLE
#include "driver_config.h" /* Configuration options for enabled driver */
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define PNET_PRODUCT_NAME_MAX_LEN  25 /* Not including termination */
#define PNET_ORDER_ID_MAX_LEN      20 /* Not including termination */
#define PNET_SERIAL_NUMBER_MAX_LEN 16 /* Not including termination */

/* Including termination. Standard says 22 (without termination) */
#define PNET_LOCATION_MAX_SIZE 23

/** Including separator and one termination */
#define PNET_MAX_FILE_FULLPATH_SIZE                                            \
   (PNET_MAX_DIRECTORYPATH_SIZE + PNET_MAX_FILENAME_SIZE)

/** Including termination. Based on Linux IFNAMSIZ */
#define PNET_INTERFACE_NAME_MAX_SIZE 16

/** Supported block version by this implementation */
#define PNET_BLOCK_VERSION_HIGH 1
#define PNET_BLOCK_VERSION_LOW  0

/** Some blocks (e.g. logbook) uses the following lower version number. */
#define PNET_BLOCK_VERSION_LOW_1 1

/*
 * Module and submodule ident number for the DAP module.
 * The DAP module and submodules must be plugged by the application after the
 * call to pnet_init.
 */
#define PNET_SLOT_DAP_IDENT                       0x00000000
#define PNET_SUBSLOT_DAP_WHOLE_MODULE             0x00000000
#define PNET_SUBSLOT_DAP_IDENT                    0x00000001
#define PNET_SUBSLOT_DAP_INTERFACE_1_IDENT        0x00008000
#define PNET_SUBSLOT_DAP_INTERFACE_1_PORT_1_IDENT 0x00008001
#define PNET_SUBSLOT_DAP_INTERFACE_1_PORT_2_IDENT 0x00008002
#define PNET_SUBSLOT_DAP_INTERFACE_1_PORT_3_IDENT 0x00008003
#define PNET_SUBSLOT_DAP_INTERFACE_1_PORT_4_IDENT 0x00008004

#define PNET_MOD_DAP_IDENT                       0x00000001 /* For use in PNET_SLOT_DAP_IDENT */
#define PNET_SUBMOD_DAP_IDENT                    0x00000001
#define PNET_SUBMOD_DAP_INTERFACE_1_IDENT        0x00008000
#define PNET_SUBMOD_DAP_INTERFACE_1_PORT_1_IDENT 0x00008001
#define PNET_SUBMOD_DAP_INTERFACE_1_PORT_2_IDENT 0x00008002
#define PNET_SUBMOD_DAP_INTERFACE_1_PORT_3_IDENT 0x00008003
#define PNET_SUBMOD_DAP_INTERFACE_1_PORT_4_IDENT 0x00008004

#define PNET_API_NO_APPLICATION_PROFILE 0

#define PNET_PORT_1 1
#define PNET_PORT_2 2
#define PNET_PORT_3 3
#define PNET_PORT_4 4

/**************************** Error codes ***********************************/

/**
 * Error Codes
 *
 * The Profinet error code consists of 4 values. The data structure for this is
 * defined in the typedef pnet_pnio_status_t.
 *
 * The values here do not constitute an exhaustive list.
 */

/**
 * # Values used in error_code
 *
 * Reserved 0x00 (No error)
 * Reserved 0x01-0x1F
 * Manufacturer specific 0x20-0x3F (LogBookData) Reserved 0x40-0x80
 */
#define PNET_ERROR_CODE_NOERROR 0x00
#define PNET_ERROR_CODE_PNIO    0x81 /** All other errors */
/** Reserved 0x82-0xDE */
#define PNET_ERROR_CODE_RTA_ERROR 0xCF /** In ERR-RTA-PDU and ERR-UDP-PDU */
/** Reserved 0xD0-0xD9 */
#define PNET_ERROR_CODE_ALARM_ACK 0xDA /** In DATA-RTA-PDU and DATA-UDP-PDU */
#define PNET_ERROR_CODE_CONNECT   0xDB /** CL-RPC-PDU */
#define PNET_ERROR_CODE_RELEASE   0xDC /** CL-RPC-PDU */
#define PNET_ERROR_CODE_CONTROL   0xDD /** CL-RPC-PDU */
#define PNET_ERROR_CODE_READ      0xDE /** Only with PNIORW */
#define PNET_ERROR_CODE_WRITE     0xDF /** Only with PNIORW */
/** Reserved 0xE0-0xFF */

/**
 * # Values used in error_decode.
 *
 * Reserved 0x00 (No error)
 *
 * Reserved 0x01-0x7F
 */
#define PNET_ERROR_DECODE_NOERROR               0x00
#define PNET_ERROR_DECODE_PNIORW                0x80 /** Only Read/Write */
#define PNET_ERROR_DECODE_PNIO                  0x81
#define PNET_ERROR_DECODE_MANUFACTURER_SPECIFIC 0x82
/** Reserved 0x83-0xFF */

/**
 * # List of error_code_1 values, for PNET_ERROR_DECODE_PNIORW
 *
 * APP = application
 * ACC = access
 * RES = resource
 */
#define PNET_ERROR_CODE_1_APP_READ_ERROR           0xA0
#define PNET_ERROR_CODE_1_APP_WRITE_ERROR          0xA1
#define PNET_ERROR_CODE_1_APP_MODULE_FAILURE       0xA2
#define PNET_ERROR_CODE_1_APP_BUSY                 0xA7
#define PNET_ERROR_CODE_1_APP_VERSION_CONFLICT     0xA8
#define PNET_ERROR_CODE_1_APP_NOT_SUPPORTED        0xA9
#define PNET_ERROR_CODE_1_ACC_INVALID_INDEX        0xB0
#define PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR   0xB1
#define PNET_ERROR_CODE_1_ACC_INVALID_SLOT_SUBSLOT 0xB2
#define PNET_ERROR_CODE_1_ACC_TYPE_CONFLICT        0xB3
#define PNET_ERROR_CODE_1_ACC_INVALID_AREA_API     0xB4
#define PNET_ERROR_CODE_1_ACC_STATE_CONFLICT       0xB5
#define PNET_ERROR_CODE_1_ACC_ACCESS_DENIED        0xB6
#define PNET_ERROR_CODE_1_ACC_INVALID_RANGE        0xB7
#define PNET_ERROR_CODE_1_ACC_INVALID_PARAMETER    0xB8
#define PNET_ERROR_CODE_1_ACC_INVALID_TYPE         0xB9
#define PNET_ERROR_CODE_1_ACC_BACKUP               0xBA
#define PNET_ERROR_CODE_1_RES_READ_CONFLICT        0xC0
#define PNET_ERROR_CODE_1_RES_WRITE_CONFLICT       0xC1
#define PNET_ERROR_CODE_1_RES_RESOURCE_BUSY        0xC2
#define PNET_ERROR_CODE_1_RES_RESOURCE_UNAVAILABLE 0xC3

/**
 * # List of error_code_1 values, for PNET_ERROR_DECODE_PNIO
 */
#define PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ         0x01
#define PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ       0x02
#define PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ        0x03
#define PNET_ERROR_CODE_1_CONN_FAULTY_ALARM_BLOCK_REQ      0x04
#define PNET_ERROR_CODE_1_CONN_FAULTY_PRM_SERVER_BLOCK_REQ 0x05
#define PNET_ERROR_CODE_1_CONN_FAULTY_MCR_BLOCK_REQ        0x06
#define PNET_ERROR_CODE_1_CONN_FAULTY_AR_RPC_BLOCK_REQ     0x07
#define PNET_ERROR_CODE_1_CONN_FAULTY_FAULTY_RECORD        0x08
#define PNET_ERROR_CODE_1_CONN_FAULTY_IR_INFO              0x09
#define PNET_ERROR_CODE_1_CONN_FAULTY_SR_INFO              0x0A
#define PNET_ERROR_CODE_1_CONN_FAULTY_AR_FSU               0x0B
#define PNET_ERROR_CODE_1_CONN_FAULTY_VENDOR               0x0C
#define PNET_ERROR_CODE_1_CONN_FAULTY_RSINFO               0x0D
/** Reserved 0x0E-0x13 */
#define PNET_ERROR_CODE_1_DCTRL_FAULTY_CONNECT            0x14
#define PNET_ERROR_CODE_1_DCTRL_FAULTY_CONNECT_PLUG       0x15
#define PNET_ERROR_CODE_1_XCTRL_FAULTY_CONNECT            0x16
#define PNET_ERROR_CODE_1_XCTRL_FAULTY_CONNECT_PLUG       0x17
#define PNET_ERROR_CODE_1_DCTRL_FAULTY_CONNECT_PRMBEG     0x18
#define PNET_ERROR_CODE_1_DCTRL_FAULTY_CONNECT_SUBMODLIST 0x19
/** Reserved 0x1A-0x27 */
#define PNET_ERROR_CODE_1_RELS_FAULTY_BLOCK 0x28
/** Reserved 0x29-0x31 */
/** Reserved 0x39-0x3B */
#define PNET_ERROR_CODE_1_ALARM_ACK 0x3C
#define PNET_ERROR_CODE_1_CMDEV     0x3D
#define PNET_ERROR_CODE_1_CMCTL     0x3E
#define PNET_ERROR_CODE_1_CTLDINA   0x3F
#define PNET_ERROR_CODE_1_CMRPC     0x40
#define PNET_ERROR_CODE_1_ALPMI     0x41
#define PNET_ERROR_CODE_1_ALPMR     0x42
#define PNET_ERROR_CODE_1_LMPM      0x43
#define PNET_ERROR_CODE_1_MAC       0x44
#define PNET_ERROR_CODE_1_RPC       0x45
#define PNET_ERROR_CODE_1_APMR      0x46
#define PNET_ERROR_CODE_1_APMS      0x47
#define PNET_ERROR_CODE_1_CPM       0x48
#define PNET_ERROR_CODE_1_PPM       0x49
#define PNET_ERROR_CODE_1_DCPUCS    0x4A
#define PNET_ERROR_CODE_1_DCPUCR    0x4B
#define PNET_ERROR_CODE_1_DCPMCS    0x4C
#define PNET_ERROR_CODE_1_DCPMCR    0x4D
#define PNET_ERROR_CODE_1_FSPM      0x4E
/** Reserved 0x4F-0x63 */
/** CTRLxxx  0x64-0xC7 */
#define PNET_ERROR_CODE_1_CMSM  0xC8
#define PNET_ERROR_CODE_1_CMRDR 0xCA
#define PNET_ERROR_CODE_1_CMWRR 0xCC
#define PNET_ERROR_CODE_1_CMIO  0xCD
#define PNET_ERROR_CODE_1_CMSU  0xCE
#define PNET_ERROR_CODE_1_CMINA 0xD0
#define PNET_ERROR_CODE_1_CMPBE 0xD1
#define PNET_ERROR_CODE_1_CMSRL 0xD2
#define PNET_ERROR_CODE_1_CMDMC 0xD3
/** CTRLxxx  0xD4-0xFC */
#define PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL 0xFD
/** Reserved 0xFE */
#define PNET_ERROR_CODE_1_USER_SPECIFIC 0xFF

/**
 * # List of error_code_2 values (not exhaustive)
 */
#define PNET_ERROR_CODE_2_CMDEV_STATE_CONFLICT 0x00
#define PNET_ERROR_CODE_2_CMDEV_RESOURCE       0x01

#define PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID 0x00
#define PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS     0x01
#define PNET_ERROR_CODE_2_CMRPC_IOCR_MISSING       0x02
#define PNET_ERROR_CODE_2_CMRPC_WRONG_BLOCK_COUNT  0x03
#define PNET_ERROR_CODE_2_CMRPC_NO_AR_RESOURCES    0x04
#define PNET_ERROR_CODE_2_CMRPC_AR_UUID_UNKNOWN    0x05
#define PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT     0x06
#define PNET_ERROR_CODE_2_CMRPC_OUT_OF_PCA_RESOURCES                           \
   0x07 /** PCA = Provider, Consumer or Alarm */
#define PNET_ERROR_CODE_2_CMRPC_OUT_OF_MEMORY           0x08
#define PNET_ERROR_CODE_2_CMRPC_PDEV_ALREADY_OWNED      0x09
#define PNET_ERROR_CODE_2_CMRPC_ARSET_STATE_CONFLICT    0x0A
#define PNET_ERROR_CODE_2_CMRPC_ARSET_PARAM_CONFLICT    0x0B
#define PNET_ERROR_CODE_2_CMRPC_PDEV_PORTS_WO_INTERFACE 0x0C

#define PNET_ERROR_CODE_2_CTLDINA_ARP_MULTIPLE_IP_ADDRESSES 0x07

#define PNET_ERROR_CODE_2_CMSM_INVALID_STATE  0x00
#define PNET_ERROR_CODE_2_CMSM_SIGNALED_ERROR 0x01

#define PNET_ERROR_CODE_2_CMRDR_INVALID_STATE  0x00
#define PNET_ERROR_CODE_2_CMRDR_SIGNALED_ERROR 0x01

#define PNET_ERROR_CODE_2_CMWRR_INVALID_STATE        0x00
#define PNET_ERROR_CODE_2_CMWRR_AR_STATE_NOT_PRIMARY 0x01
#define PNET_ERROR_CODE_2_CMWRR_SIGNALED_ERROR       0x02

#define PNET_ERROR_CODE_2_CMIO_INVALID_STATE  0x00
#define PNET_ERROR_CODE_2_CMIO_SIGNALED_ERROR 0x01

#define PNET_ERROR_CODE_2_CMSU_INVALID_STATE           0x00
#define PNET_ERROR_CODE_2_CMSU_AR_ADD_PROV_CONS_FAILED 0x01
#define PNET_ERROR_CODE_2_CMSU_AR_ALARM_OPEN_FAILED    0x02
#define PNET_ERROR_CODE_2_CMSU_AR_ALARM_SEND           0x03
#define PNET_ERROR_CODE_2_CMSU_AR_ALARM_ACK_SEND       0x04
#define PNET_ERROR_CODE_2_CMSU_AR_ALARM_IND            0x05

#define PNET_ERROR_CODE_2_CMINA_INVALID_STATE  0x00
#define PNET_ERROR_CODE_2_CMINA_SIGNALED_ERROR 0x01

#define PNET_ERROR_CODE_2_CMPBE_INVALID_STATE  0x00
#define PNET_ERROR_CODE_2_CMPBE_SIGNALED_ERROR 0x01

#define PNET_ERROR_CODE_2_CMSRL_INVALID_STATE  0x00
#define PNET_ERROR_CODE_2_CMSRL_SIGNALED_ERROR 0x01

#define PNET_ERROR_CODE_2_CMDMC_INVALID_STATE  0x00
#define PNET_ERROR_CODE_2_CMDMC_SIGNALED_ERROR 0x01

#define PNET_ERROR_CODE_2_CPM_INVALID_STATE 0x00
#define PNET_ERROR_CODE_2_CPM_INVALID       0x01

#define PNET_ERROR_CODE_2_PPM_INVALID_STATE 0x00
#define PNET_ERROR_CODE_2_PPM_INVALID       0x01

#define PNET_ERROR_CODE_2_APMS_INVALID_STATE 0x00
#define PNET_ERROR_CODE_2_APMS_LMPM_ERROR    0x01
#define PNET_ERROR_CODE_2_APMS_TIMEOUT       0x02
/* Reserved 0x03..0xff */

#define PNET_ERROR_CODE_2_APMR_INVALID_STATE 0x00
#define PNET_ERROR_CODE_2_APMR_LMPM_ERROR    0x01
/* Reserved 0x02..0xff */

#define PNET_ERROR_CODE_2_ALPMI_INVALID_STATE 0x00
#define PNET_ERROR_CODE_2_ALPMI_WRONG_ACK_PDU 0x01
#define PNET_ERROR_CODE_2_ALPMI_INVALID       0x02
#define PNET_ERROR_CODE_2_ALPMI_WRONG_STATE   0x03
/* Reserved 0x04..0xff */

#define PNET_ERROR_CODE_2_ALPMR_INVALID_STATE   0x00
#define PNET_ERROR_CODE_2_ALPMR_WRONG_ALARM_PDU 0x01
#define PNET_ERROR_CODE_2_ALPMR_INVALID         0x02
#define PNET_ERROR_CODE_2_ALPMR_WRONG_STATE     0x03
/* Reserved 0x04..0xff */

#define PNET_ERROR_CODE_2_INVALID_BLOCK_LEN          0x01
#define PNET_ERROR_CODE_2_INVALID_BLOCK_VERSION_HIGH 0x02
#define PNET_ERROR_CODE_2_INVALID_BLOCK_VERSION_LOW  0x03

/**
 * # List of error_code_2 values, for
 * PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL (not exhaustive).
 */
#define PNET_ERROR_CODE_2_ABORT_CODE_SEQ                   0x01
#define PNET_ERROR_CODE_2_ABORT_INSTANCE_CLOSED            0x02
#define PNET_ERROR_CODE_2_ABORT_AR_OUT_OF_MEMORY           0x03
#define PNET_ERROR_CODE_2_ABORT_AR_ADD_CPM_PPM_FAILED      0x04
#define PNET_ERROR_CODE_2_ABORT_AR_CONSUMER_DHT_EXPIRED    0x05
#define PNET_ERROR_CODE_2_ABORT_AR_CMI_TIMEOUT             0x06
#define PNET_ERROR_CODE_2_ABORT_AR_ALARM_OPEN_FAILED       0x07
#define PNET_ERROR_CODE_2_ABORT_AR_ALARM_SEND_CNF_NEG      0x08
#define PNET_ERROR_CODE_2_ABORT_AR_ALARM_ACK_SEND_CNF_NEG  0x09
#define PNET_ERROR_CODE_2_ABORT_AR_ALARM_DATA_TOO_LONG     0x0a
#define PNET_ERROR_CODE_2_ABORT_AR_ALARM_IND_ERROR         0x0b
#define PNET_ERROR_CODE_2_ABORT_AR_RPC_CLIENT_CALL_CNF_NEG 0x0c
#define PNET_ERROR_CODE_2_ABORT_AR_ABORT_REQ               0x0d
#define PNET_ERROR_CODE_2_ABORT_AR_RE_RUN_ABORTS_EXISTING  0x0e
#define PNET_ERROR_CODE_2_ABORT_AR_RELEASE_IND_RECEIVED    0x0f
#define PNET_ERROR_CODE_2_ABORT_AR_DEVICE_DEACTIVATED      0x10
#define PNET_ERROR_CODE_2_ABORT_AR_REMOVED                 0x11
#define PNET_ERROR_CODE_2_ABORT_AR_PROTOCOL_VIOLATION      0x12
#define PNET_ERROR_CODE_2_ABORT_AR_NAME_RESOLUTION_ERROR   0x13
#define PNET_ERROR_CODE_2_ABORT_AR_RPC_BIND_ERROR          0x14
#define PNET_ERROR_CODE_2_ABORT_AR_RPC_CONNECT_ERROR       0x15
#define PNET_ERROR_CODE_2_ABORT_AR_RPC_READ_ERROR          0x16
#define PNET_ERROR_CODE_2_ABORT_AR_RPC_WRITE_ERROR         0x17
#define PNET_ERROR_CODE_2_ABORT_AR_RPC_CONTROL_ERROR       0x18
#define PNET_ERROR_CODE_2_ABORT_AR_FORBIDDEN_PULL_OR_PLUG  0x19
#define PNET_ERROR_CODE_2_ABORT_AR_AP_REMOVED              0x1a
#define PNET_ERROR_CODE_2_ABORT_AR_LINK_DOWN               0x1b
#define PNET_ERROR_CODE_2_ABORT_AR_MC_REGISTER_FAILED      0x1c
#define PNET_ERROR_CODE_2_ABORT_NOT_SYNCHRONIZED           0x1d
#define PNET_ERROR_CODE_2_ABORT_WRONG_TOPOLOGY             0x1e
#define PNET_ERROR_CODE_2_ABORT_DCP_STATION_NAME_CHANGED   0x1f
#define PNET_ERROR_CODE_2_ABORT_DCP_RESET_TO_FACTORY       0x20
#define PNET_ERROR_CODE_2_ABORT_PDEV_CHECK_FAILED          0x24

/**
 * # List of error_code_2 values, for
 * PNET_ERROR_CODE_1_DCTRL_FAULTY_CONNECT (not exhaustive).
 */
#define PNET_ERROR_CODE_2_DCTRL_FAULTY_CONNECT_CONTROLCOMMAND 0x08

/******************************** Events ******************************/

/**
 * The events are sent from CMDEV to the application using the
 * \a pnet_state_ind() call-back function.
 */
typedef enum pnet_event_values
{
   /** The AR has been closed. */
   PNET_EVENT_ABORT,

   /** A connection has been initiated. */
   PNET_EVENT_STARTUP,

   /** Controller has sent all write records. */
   PNET_EVENT_PRMEND,

   /** Controller has acknowledged the APPL_RDY message */
   PNET_EVENT_APPLRDY,

   /** The cyclic data transmission is now properly set up */
   PNET_EVENT_DATA
} pnet_event_values_t;

/**
 * Values used for IOCS and IOPS. The actual values are important, as they are
 * sent on the wire.
 */
typedef enum pnet_ioxs_values
{
   PNET_IOXS_BAD = 0x00,
   PNET_IOXS_GOOD = 0x80
} pnet_ioxs_values_t;

/**
 * Values used when plugging a sub-module using \a pnet_plug_submodule().
 * The actual values are important, as they are sent on the wire.
 */
typedef enum pnet_submodule_dir
{
   PNET_DIR_NO_IO = 0x00,
   PNET_DIR_INPUT = 0x01,
   PNET_DIR_OUTPUT = 0x02,
   PNET_DIR_IO = 0x03
} pnet_submodule_dir_t;

/*
 * Data configuration for one submodule.
 *
 * Used when indicating an expected submodule.
 */
typedef struct pnet_data_cfg
{
   pnet_submodule_dir_t data_dir;
   uint16_t insize;
   uint16_t outsize;
} pnet_data_cfg_t;

/**
 * CControl command codes used in the \a pnet_dcontrol_ind() call-back function.
 */
typedef enum pnet_control_command
{
   PNET_CONTROL_COMMAND_PRM_BEGIN,
   PNET_CONTROL_COMMAND_PRM_END,
   PNET_CONTROL_COMMAND_APP_RDY,
   PNET_CONTROL_COMMAND_RELEASE,

   /** Not yet implemented */
   PNET_CONTROL_COMMAND_RDY_FOR_COMPANION,

   /** Not yet implemented */
   PNET_CONTROL_COMMAND_RDY_FOR_RTC3,
} pnet_control_command_t;

/**
 * Data status bits used in the \a pnet_new_data_status_ind() call-back
 * function.
 */
typedef enum pnet_data_status_bits
{
   /** 0 => BACKUP, 1 => PRIMARY */
   PNET_DATA_STATUS_BIT_STATE = 0,

   /** Meaning depends on STATE bit */
   PNET_DATA_STATUS_BIT_REDUNDANCY,

   /** 0 => INVALID, 1 => VALID */
   PNET_DATA_STATUS_BIT_DATA_VALID,

   PNET_DATA_STATUS_BIT_RESERVED_1,

   /** 0 => STOP, 1 => RUN */
   PNET_DATA_STATUS_BIT_PROVIDER_STATE,

   /** 0 => Problem detected, 1 => Normal operation */
   PNET_DATA_STATUS_BIT_STATION_PROBLEM_INDICATOR,

   PNET_DATA_STATUS_BIT_RESERVED_2,

   /** 0 => Evaluate data status,
    *  1 => Ignore the data status (typically used on a frame with subframes) */
   PNET_DATA_STATUS_BIT_IGNORE
} pnet_data_status_bits_t;

/** Network handle */
typedef struct pnet pnet_t;

/**
 * Profinet stack detailed error information.
 */
typedef struct pnet_pnio_status
{
   uint8_t error_code;
   uint8_t error_decode;
   uint8_t error_code_1;
   uint8_t error_code_2;
} pnet_pnio_status_t;

/************************ Alarm and Diagnosis ********************************/

/** Summary of all diagnosis items, sent in an alarm.
   Only valid for alarms attached to the Diagnosis ASE
   See Profinet 2.4 Services, section 7.3.1.6.5.1 Alarm Notification */
typedef struct pnet_alarm_spec
{
   /** Diagnosis in standard format (any severity, appear)
    *  on this subslot */
   bool channel_diagnosis;

   /** Diagnosis in USI format (which always is FAULT) on this subslot,
    * (USI format does not have appears/disappears) */
   bool manufacturer_diagnosis;

   /** Fault or Qual27-Qual31 (standard or USI format, appear) on
    *  this subslot */
   bool submodule_diagnosis;

   /** Fault or Qual27-Qual31 (standard or USI format, appear) on this AR */
   bool ar_diagnosis;
} pnet_alarm_spec_t;

/**
 * Shows location and summary of diagnosis items in an alarm
 */
typedef struct pnet_alarm_argument
{
   uint32_t api_id;
   uint16_t slot_nbr;
   uint16_t subslot_nbr;
   uint16_t alarm_type;
   uint16_t sequence_number;
   pnet_alarm_spec_t alarm_specifier;
} pnet_alarm_argument_t;

/**
 * Sent to controller on negative result.
 */
typedef struct pnet_result
{
   /** Profinet-specific error information. */
   pnet_pnio_status_t pnio_status;

   /** Application-specific error information. */
   uint16_t add_data_1;

   /** Application-specific error information. */
   uint16_t add_data_2;
} pnet_result_t;

/******************************* Callbacks ************************************/

/*
 * Application call-back functions
 *
 * The application should define call-back functions which are called by
 * the stack when specific events occurs within the stack.
 *
 * Note that most of these functions are mandatory in the sense that they must
 * exist and return 0 (zero) for a functioning stack. Some functions are
 * required to perform specific functionality.
 *
 * The call-back functions should return 0 (zero) for a successful call and
 * -1 for an unsuccessful call.
 */

/**
 * Indication to the application that a Connect request was received from the
 * controller.
 *
 * This application call-back function is called by the Profinet stack on every
 * Connect request from the Profinet controller.
 *
 * The connection will be opened if this function returns 0 (zero) and the stack
 * is otherwise able to establish a connection.
 *
 * If this function returns something other than 0 (zero) then the Connect
 * request is refused by the device. In case of error the application should
 * provide error information in \a p_result.
 *
 * It is optional to implement this callback (assumes success if not
 * implemented).
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:    The AREP.
 * @param p_result         Out:   Detailed error information if return != 0.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_connect_ind) (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result);

/**
 * Indication to the application that a Release request was received from the
 * controller.
 *
 * This application call-back function is called by the Profinet stack on every
 * Release request from the Profinet controller.
 *
 * The connection will be closed regardless of the return value from this
 * function. In case of error the application should provide error information
 * in \a p_result.
 *
 * It is optional to implement this callback (assumes success if not
 * implemented).
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:    The AREP.
 * @param p_result         Out:   Detailed error information if return != 0.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_release_ind) (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result);

/**
 * Indication to the application that a DControl request was received from the
 * controller. Typically this means that the controller is done writing
 * parameters.
 *
 * This application call-back function is called by the Profinet stack on every
 * DControl request from the Profinet controller.
 *
 * The application is not required to take any action but the function must
 * return 0 (zero) for proper function of the stack. If this function returns
 * something other than 0 (zero) then the DControl request is refused by the
 * device. In case of error the application should provide error information in
 * \a p_result.
 *
 * It is optional to implement this callback (assumes success if not
 * implemented).
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:    The AREP.
 * @param control_command  In:    The DControl command code.
 * @param p_result         Out:   Detailed error information if return != 0.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_dcontrol_ind) (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_control_command_t control_command,
   pnet_result_t * p_result);

/**
 * Indication to the application that a CControl confirmation was received from
 * the controller. Typically this means that the controller has received our
 * "Application ready" message.
 *
 * This application call-back function is called by the Profinet stack on every
 * CControl confirmation from the Profinet controller.
 *
 * The application is not required to take any action.
 * The return value from this call-back function is ignored by the Profinet
 * stack. In case of error the application should provide error information in
 * \a p_result.
 *
 * It is optional to implement this callback (assumes success?).
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:    The AREP.
 * @param p_result         Out:   Detailed error information.
 * @return 0 on success. Other values are ignored.
 */
typedef int (*pnet_ccontrol_cnf) (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_result_t * p_result);

/**
 * Indication to the application that a state transition has occurred within the
 * Profinet stack.
 *
 * This application call-back function is called by the Profinet stack on
 * specific state transitions within the Profinet stack.
 *
 * At the very least the application must react to the PNET_EVENT_PRMEND state
 * transition. After this event the application must call \a
 * pnet_application_ready(), when it has finished its setup and it is ready to
 * exchange data.
 *
 * The return value from this call-back function is ignored by the Profinet
 * stack.
 *
 * It is optional to implement this callback (but then it would be difficult
 * to know when to call the \a pnet_application_ready() function).
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:    The AREP.
 * @param state            In:    The state transition event. See
 *                                pnet_event_values_t.
 * @return 0 on success. Other values are ignored.
 */
typedef int (*pnet_state_ind) (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   pnet_event_values_t state);

/**
 * Indication to the application that an IODRead request was received from the
 * controller.
 *
 * This application call-back function is called by the Profinet stack on every
 * IODRead request from the Profinet controller which specify an
 * application-specific value of \a idx (0x0000 - 0x7fff). All other values of
 * \a idx are handled internally by the Profinet stack.
 *
 * The application must verify the value of \a idx, and that \a p_read_length is
 * large enough. Further, the application must provide a
 * pointer to the binary value in \a pp_read_data and the size, in bytes, of the
 * binary value in \a p_read_length.
 *
 * The Profinet stack does not perform any endianness conversion on the binary
 * value.
 *
 * In case of error the application should provide error information in \a
 * p_result.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:    The AREP.
 * @param api              In:    The AP identifier.
 * @param slot             In:    The slot number.
 * @param subslot          In:    The sub-slot number.
 * @param idx              In:    The data record index.
 * @param sequence_number  In:    The sequence number.
 * @param pp_read_data     Out:   A pointer to the binary value.
 * @param p_read_length    InOut: The maximum (in) and actual (out) length in
 *                                bytes of the binary value.
 * @param p_result         Out:   Detailed error information if returning != 0
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_read_ind) (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint8_t ** pp_read_data,
   uint16_t * p_read_length,
   pnet_result_t * p_result);

/**
 * Indication to the application that an IODWrite request was received from the
 * controller.
 *
 * This application call-back function is called by the Profinet stack on every
 * IODWrite request from the Profinet controller which specify an
 * application-specific value of \a idx (0x0000 - 0x7fff). All other values of
 * \a idx are handled internally by the Profinet stack.
 *
 * The application must verify the values of \a idx and \a write_length and save
 * (copy) the binary value in \a p_write_data. A future IODRead must return the
 * latest written value.
 *
 * The Profinet stack does not perform any endianness conversion on the binary
 * value.
 *
 * In case of error the application should provide error information in \a
 * p_result.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:    The AREP.
 * @param api              In:    The API identifier.
 * @param slot             In:    The slot number.
 * @param subslot          In:    The sub-slot number.
 * @param idx              In:    The data record index.
 * @param sequence_number  In:    The sequence number.
 * @param write_length     In:    The length in bytes of the binary value.
 * @param p_write_data     In:    A pointer to the binary value.
 * @param p_result         Out:   Detailed error information if returning != 0
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_write_ind) (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t idx,
   uint16_t sequence_number,
   uint16_t write_length,
   const uint8_t * p_write_data,
   pnet_result_t * p_result);

/**
 * Indication to the application that a module is requested by the controller in
 * a specific slot.
 *
 * This application call-back function is called by the Profinet stack to
 * indicate that the controller has requested the presence of a specific module,
 * ident number \a module_ident, in the slot number \a slot.
 *
 * The application must react to this by configuring itself accordingly (if
 * possible) and call function pnet_plug_module() to configure the stack for
 * this module.
 *
 * If the wrong module ident number is plugged then the stack will accept this,
 * but signal to the controller that a substitute module is fitted.
 *
 * This function should return 0 (zero) if a valid module was plugged. Or return
 * -1 if the application cannot handle this request.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param api              In:    The AP identifier.
 * @param slot             In:    The slot number.
 * @param module_ident     In:    The module ident number.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_exp_module_ind) (
   pnet_t * net,
   void * arg,
   uint32_t api,
   uint16_t slot,
   uint32_t module_ident);

/**
 * Indication to the application that a sub-module is requested by the
 * controller in a specific sub-slot.
 *
 * This application call-back function is called by the Profinet stack to
 * indicate that the controller has requested the presence of a specific
 * sub-module with ident number \a submodule_ident, in the sub-slot number
 * \a subslot, with module ident number \a module_ident in slot \a slot.
 *
 * If a module has not been plugged in the slot \a slot then an automatic plug
 * request is issued internally by the stack.
 *
 * The application must react to this by configuring itself accordingly (if
 * possible) and call function \a pnet_plug_submodule() to configure the stack
 * with the correct input/output data sizes.
 *
 * If the wrong sub-module ident number is plugged then the stack will accept
 * this, but signal to the controller that a substitute sub-module is fitted.
 *
 * This function should return 0 (zero) if a valid sub-module was plugged,
 * or return -1 if the application cannot handle this request.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param api              In:    The AP identifier.
 * @param slot             In:    The slot number.
 * @param subslot          In:    The sub-slot number.
 * @param module_ident     In:    The module ident number.
 * @param submodule_ident  In:    The sub-module ident number.
 * @param p_exp_data       In:    The expected data configuration (sizes and
 *                                direction)
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_exp_submodule_ind) (
   pnet_t * net,
   void * arg,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint32_t module_ident,
   uint32_t submodule_ident,
   const pnet_data_cfg_t * p_exp_data);

/**
 * Indication to the application that the data status received from the
 * controller has changed.
 *
 * This application call-back function is called by the Profinet stack to
 * indicate that the received data status has changed.
 *
 * The application is not required by the Profinet stack to take any action. It
 * may use this information as it wishes. The return value from this call-back
 * function is ignored by the Profinet stack.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:    The AREP.
 * @param crep             In:    The CREP.
 * @param changes          In:    The changed bits in the received data status.
 *                                See pnet_data_status_bits_t
 * @param data_status      In:    Current received data status (after changes).
 *                                See pnet_data_status_bits_t
 * @return 0 on success. Other values are ignored.
 */
typedef int (*pnet_new_data_status_ind) (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   uint32_t crep,
   uint8_t changes,
   uint8_t data_status);

/**
 * The IO-controller has sent an alarm to the device.
 *
 * This functionality is used for alarms triggered by the IO-controller.
 *
 * When receiving this indication, the application shall
 * respond with \a pnet_alarm_send_ack(), which
 * may be called in the context of this callback.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:    The AREP.
 * @param p_alarm_argument In:    The alarm argument (with slot, subslot,
 *                                alarm_type etc)
 * @param data_len         In:    Data length
 * @param data_usi         In:    Alarm USI
 * @param p_data           In:    Alarm data
 * @return  0  on success.
 *          Other values are ignored.
 */
typedef int (*pnet_alarm_ind) (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   const pnet_alarm_argument_t * p_alarm_argument,
   uint16_t data_len,
   uint16_t data_usi,
   const uint8_t * p_data);

/**
 * The controller acknowledges the alarm sent previously.
 * It is now possible to send another alarm.
 *
 * This functionality is used for alarms triggered by the IO-device.
 *
 * It is optional to implement this callback.
 * The return value from this call-back function is ignored by the Profinet
 * stack.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:    The AREP.
 * @param p_pnio_status    In:    Detailed ACK information.
 * @return 0 on success. Other values are ignored.
 */
typedef int (*pnet_alarm_cnf) (
   pnet_t * net,
   void * arg,
   uint32_t arep,
   const pnet_pnio_status_t * p_pnio_status);

/**
 * The controller acknowledges the alarm ACK sent previously.
 *
 * This functionality is used for alarms triggered by the IO-controller.
 *
 * It is optional to implement this callback.
 * The return value from this call-back function is ignored by the Profinet
 * stack.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:    The AREP.
 * @param res              In:    0 if ACK was received by the remote side.
 *                                  This is cnf(+).
 *                                -1 if ACK was not received by the remote
 *                                  side. This is cnf(-).
 * @return 0 on success. Other values are ignored.
 */
typedef int (
   *pnet_alarm_ack_cnf) (pnet_t * net, void * arg, uint32_t arep, int res);

/**
 * Indication to the application that a reset request was received from the
 * IO-controller.
 *
 * The IO-controller can ask for communication parameters or application
 * data to be reset, or to do a factory reset.
 *
 * This application call-back function is called by the Profinet stack on every
 * reset request (via the DCP "Set" command) from the Profinet controller.
 *
 * The application should reset the application data if
 * \a should_reset_application is true. For other cases this callback is
 * triggered for diagnostic reasons.
 *
 * The return value from this call-back function is ignored by the Profinet
 * stack.
 *
 * It is optional to implement this callback (if you do not have any application
 * data that could be reset).
 *
 * Reset modes:
 *  * 0:  (Power-on reset, not from IO-controller. Will not trigger this.)
 *  * 1:  Reset application data
 *  * 2:  Reset communication parameters (done by the stack)
 *  * 99: Reset all (factory reset).
 *
 * The reset modes 1-9 (out of which 1 and 2 are supported here) are defined
 * by the Profinet standard. Value 99 is used here to indicate that the
 * IO-controller has requested a factory reset via another mechanism.
 *
 * In order to remain responsive to DCP communication and Ethernet switching,
 * the device should not do a hard or soft reset for reset mode 1 or 2. It is
 * allowed for the factory reset case (but not mandatory).
 *
 * No \a arep information is available, as this callback typically is triggered
 * when there is no active connection.
 *
 * @param net                      InOut: The p-net stack instance
 * @param arg                      InOut: User-defined data (not used by p-net)
 * @param should_reset_application In:    True if the user should reset the
 *                                        application data.
 * @param reset_mode               In:    Detailed reset information.
 * @return 0 on success. Other values are ignored.
 */
typedef int (*pnet_reset_ind) (
   pnet_t * net,
   void * arg,
   bool should_reset_application,
   uint16_t reset_mode);

/**
 * Indication to the application that the Profinet signal LED should change
 * state.
 *
 * Use this callback to implement control of the LED.
 *
 * It is optional to implement this callback (but a compliant Profinet device
 * must have a signal LED)
 *
 * No \a arep information is available, as this callback typically is triggered
 * when there is no active connection.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param led_state        In:    True if the signal LED should be on.
 * @return  0  on success.
 *          -1 if an error occurred. Will trigger a log message.
 */
typedef int (*pnet_signal_led_ind) (pnet_t * net, void * arg, bool led_state);

/*
 * Network and device configuration.
 *
 * Configuration of the stack is performed by transferring a structure
 * in the call to pnet_init().
 *
 * Along with the configuration the initial (default) values of the
 * I&M data records are conveyed as well as the values used for
 * sending LLDP frames.
 *
 * Configuration values are taken as is. No validation is performed.
 */

/**
 * Describe which I&M values are supported.
 *
 * I&M0 is always supported.
 *
 */
typedef enum pnet_im_supported_values
{
   PNET_SUPPORTED_IM1 = 0x0002,
   PNET_SUPPORTED_IM2 = 0x0004,
   PNET_SUPPORTED_IM3 = 0x0008,

   /** Should only be used together with functional safety */
   PNET_SUPPORTED_IM4 = 0x0010,
} pnet_im_supported_values_t;

/**
 * The I&M0 data record is read-only by the controller.
 *
 * This data record is mandatory.
 */
typedef struct pnet_im_0
{
   uint8_t im_vendor_id_hi;
   uint8_t im_vendor_id_lo;

   /** Terminated string */
   char im_order_id[PNET_ORDER_ID_MAX_LEN + 1];

   /** Terminated string */
   char im_serial_number[PNET_SERIAL_NUMBER_MAX_LEN + 1];

   uint16_t im_hardware_revision;
   char im_sw_revision_prefix;
   uint8_t im_sw_revision_functional_enhancement;
   uint8_t im_sw_revision_bug_fix;
   uint8_t im_sw_revision_internal_change;
   uint16_t im_revision_counter;
   uint16_t im_profile_id;
   uint16_t im_profile_specific_type;

   /** Always 1 */
   uint8_t im_version_major;

   /** Always 1 */
   uint8_t im_version_minor;

   /** One bit for each supported I&M1..15. I&M0 is always supported.
       Use pnet_im_supported_values_t. */
   uint16_t im_supported;
} pnet_im_0_t;

/**
 * The I&M1 data record is read-write by the controller.
 *
 * This data record is optional. If this data record is supported
 * by the application then bit 1 in the im_supported member of I&M0 shall be
 * set.
 */
typedef struct pnet_im_1
{
   /** Terminated string */
   char im_tag_function[32 + 1];

   /** Terminated string */
   char im_tag_location[PNET_LOCATION_MAX_SIZE];
} pnet_im_1_t;

/**
 * The I&M2 data record is read-write by the controller.
 *
 * This data record is optional. If this data record is supported
 * by the application then bit 2 in the im_supported member of I&M0 shall be
 * set.
 */
typedef struct pnet_im_2
{
   /** Terminated string in the format "YYYY-MM-DD HH:MM"  */
   char im_date[16 + 1];
} pnet_im_2_t;

/**
 * The I&M3 data record is read-write by the controller.
 *
 * This data record is optional. If this data record is supported
 * by the application then bit 3 in the im_supported member of I&M0 shall be
 * set.
 */
typedef struct pnet_im_3
{
   /** Terminated string padded with spaces */
   char im_descriptor[54 + 1];
} pnet_im_3_t;

/**
 * The I&M4 data record is read-write by the controller.
 *
 * Used for functional safety.
 *
 * This data record is optional. If this data record is supported
 * by the application then bit 4 in the im_supported member of I&M0 shall be
 * set.
 */
typedef struct pnet_im_4
{
   /** Non-terminated binary string, padded with zeroes */
   char im_signature[54];
} pnet_im_4_t;

/**
 * The device-specific vendor and device identities.
 *
 * The vendor id is obtained from Profibus International.
 *
 * The device id is assigned by the vendor.
 */
typedef struct pnet_cfg_device_id
{
   uint8_t vendor_id_hi;
   uint8_t vendor_id_lo;
   uint8_t device_id_hi;
   uint8_t device_id_lo;
} pnet_cfg_device_id_t;

/**
 * Used for assigning a static IPv4 address to the device.
 *
 * The Profinet stack also supports assigning an IP address, mask and gateway
 * address via DCP Set commands based on the Ethernet MAC address.
 *
 * An IP address of 0.0.0.1 has the member d=1, and the rest of the members
 * set to 0.
 *
 */
typedef struct pnet_ip_addr_t
{
   uint8_t a;
   uint8_t b;
   uint8_t c;
   uint8_t d;
} pnet_cfg_ip_addr_t;

/**
 * The Ethernet MAC address.
 */
typedef struct pnet_ethaddr
{
   uint8_t addr[6];
} pnet_ethaddr_t;

/* Including termination. Standard says 240 (without termination) */
#define PNET_CHASSIS_ID_MAX_SIZE (241)

/* Including termination. Standard says 240 (without termination) */
#define PNET_STATION_NAME_MAX_SIZE (241)

/* Including termination. Standard says 14 (without termination) */
#define PNET_PORT_NAME_MAX_SIZE (15)

/** Including termination */
#define PNET_LLDP_CHASSIS_ID_MAX_SIZE (PNET_CHASSIS_ID_MAX_SIZE)

/** Including termination */
#define PNET_LLDP_PORT_ID_MAX_SIZE                                             \
   (PNET_STATION_NAME_MAX_SIZE + PNET_PORT_NAME_MAX_SIZE)

/**
 * Physical Port Configuration
 */
typedef struct pnet_port_cfg
{
   const char * netif_name;

   /** Ethernet MAU type to use when it not can be read from hardware.
    *  For example
    *     0x0010 = Copper 100 Mbit/s Full duplex.
    *     0x001E = Copper 1000 Mbit/s Full duplex.
    *  See \a pnal_eth_mau_t for more examples. */
   uint16_t default_mau_type;
} pnet_port_cfg_t;

/**
 * IP Configuration
 */
typedef struct pnet_ip_cfg
{
   /** Not yet supported by stack. */
   bool dhcp_enable;

   pnet_cfg_ip_addr_t ip_addr;
   pnet_cfg_ip_addr_t ip_mask;
   pnet_cfg_ip_addr_t ip_gateway;
} pnet_ip_cfg_t;

/**
 * Interface Configuration
 *
 * Configuration of network interfaces used by the stack.
 * The \a main_netif_name defines the network interface used by a controller/PLC
 * to access the device (called Management Port in Profinet).
 * The \a physical_ports array defines the physical ports connected to the
 * main_port.
 *
 * In the case one network interface is used, \a main_netif_name and
 * \a physical_ports[0].netif_name will refer to the same network interface.
 */
typedef struct pnet_if_cfg
{
   /** Main (DAP) network interface */
   const char * main_netif_name;

   /** IP Settings for main network interface */
   pnet_ip_cfg_t ip_cfg;

   pnet_port_cfg_t physical_ports[PNET_MAX_PHYSICAL_PORTS];
} pnet_if_cfg_t;

/**
 * This is all the configuration needed to use the Profinet stack.
 *
 * The application must supply the values in the call to
 * function \a pnet_init().
 */
typedef struct pnet_cfg
{
   /** Tick interval in us.
    *  Specifies the time between calls to \a pnet_handle_periodic().
    */
   uint32_t tick_us;

   pnet_state_ind state_cb;
   pnet_connect_ind connect_cb;
   pnet_release_ind release_cb;
   pnet_dcontrol_ind dcontrol_cb;
   pnet_ccontrol_cnf ccontrol_cb;
   pnet_write_ind write_cb;
   pnet_read_ind read_cb;
   pnet_exp_module_ind exp_module_cb;
   pnet_exp_submodule_ind exp_submodule_cb;
   pnet_new_data_status_ind new_data_status_cb;
   pnet_alarm_ind alarm_ind_cb;
   pnet_alarm_cnf alarm_cnf_cb;
   pnet_alarm_ack_cnf alarm_ack_cnf_cb;
   pnet_reset_ind reset_cb;
   pnet_signal_led_ind signal_led_cb;

   /** Userdata passed to callbacks, not used by stack */
   void * cb_arg;

   pnet_im_0_t im_0_data;
   pnet_im_1_t im_1_data;
   pnet_im_2_t im_2_data;
   pnet_im_3_t im_3_data;
   pnet_im_4_t im_4_data;

   pnet_cfg_device_id_t device_id;
   pnet_cfg_device_id_t oem_device_id;

   /** Default station name. Terminated string */
   char station_name[PNET_STATION_NAME_MAX_SIZE];

   /**
    * Product name
    *
    * This is known as DeviceVendorValue and DeviceType in the Profinet
    * specification. It constitutes the first part of SystemIdentification
    * (sysDescr in SNMP). It may also be used to construct the Chassis ID.
    * See IEC CDV 61158-6-10 ch. 4.10.3.3.1.
    *
    * Terminated string.
    */
   char product_name[PNET_PRODUCT_NAME_MAX_LEN + 1];

   /** Smallest allowed data exchange interval, in units of 31.25 us.
    *  Used for triggering error messages to the PLC. Should match GSDML file.
    *  Typically 32, which corresponds to 1 ms. Max 0x1000 (128 ms) */
   uint16_t min_device_interval;

   /** Operating system dependent settings */
   pnal_cfg_t pnal_cfg;

   /** Send DCP HELLO message on startup if true. */
   bool send_hello;

   /** Number of physical ports. Should respect PNET_MAX_PHYSICAL_PORTS.
       This parameter is useful when shipping a single compiled version of the
       library, but there are several applications with different number of
       ports. */
   uint8_t num_physical_ports; // TODO int or uint16_t ?

   /** Send diagnosis in the qualified format (otherwise extended format) */
   bool use_qualified_diagnosis;

   pnet_if_cfg_t if_cfg;

#if PNET_OPTION_DRIVER_ENABLE
   bool driver_enable;
   pnet_driver_config_t driver_config;
#endif

   /** Storage between runs
    *  Terminated string with absolute path.
    *  Use NULL or empty string for current directory. */
   char file_directory[PNET_MAX_DIRECTORYPATH_SIZE];

} pnet_cfg_t;

/*
 * API function return values
 *
 * All functions return either (zero) 0 for a successful call or
 * -1 for an unsuccessful call.
 */

/**
 * Initialize the Profinet stack.
 *
 * This function must be called to initialize the Profinet stack.
 * @param p_cfg            In:    Profinet configuration. These values are used
 *                                at first startup and at factory reset.
 * @return a handle to the stack instance, or NULL if an error occurred.
 */
PNET_EXPORT pnet_t * pnet_init (const pnet_cfg_t * p_cfg);

/**
 * Execute all periodic functions within the ProfiNet stack.
 *
 * This function shall be called periodically by the application.
 * The period is specified by the tick_us parameter, part of pnet_cfg_t
 * configuration.
 * The period should match the expected I/O data rate to and from the device.
 * @param net              InOut: The p-net stack instance
 */
PNET_EXPORT void pnet_handle_periodic (pnet_t * net);

/**
 * Application signals ready to exchange data.
 *
 * Sends a CControl request to the IO-controller.
 *
 * This function must be called _after_ the application has received
 * the \a pnet_state_ind() user callback with PNET_EVENT_PRMEND,
 * in order for a connection to be established.
 *
 * If this CControl request ("Application ready") is sent (to the PLC) before
 * the DControl response ("Param end") is sent (to the PLC) there will
 * be problems at startup.
 *
 * If this function does not see all PPM data and IOPS areas are set (by the
 * app) then it returns error and the application must try again - otherwise the
 * startup will hang.
 *
 * Triggers the \a pnet_state_ind() user callback with PNET_EVENT_APPLRDY.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:    The AREP
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_application_ready (pnet_t * net, uint32_t arep);

/**
 * Plug a module into a slot.
 *
 * This function is used to plug a specific module into a specific slot.
 *
 * This function may be called from the \a pnet_exp_module_ind() call-back
 * function of the application.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:    The API identifier.
 * @param slot             In:    The slot number.
 * @param module_ident     In:    The module ident number.
 * @return  0  if a module could be plugged into the slot.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_plug_module (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint32_t module_ident);

/**
 * Plug a sub-module into a sub-slot.
 *
 * This function is used to plug a specific sub-module into a specific sub-slot.
 *
 * This function may be called from the \a pnet_exp_submodule_ind call-back
 * function of the application.
 *
 * If a module has not been plugged into the designated slot then it will be
 * plugged automatically by this function.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:    The API identifier.
 * @param slot             In:    The slot number.
 * @param subslot          In:    The sub-slot number.
 * @param module_ident     In:    The module ident number.
 * @param submodule_ident  In:    The sub-module ident number.
 * @param direction        In:    The direction of data.
 * @param length_input     In:    The size in bytes of the input data.
 * @param length_output    In:    The size in bytes of the output data.
 * @return  0  if the sub-module could be plugged into the sub-slot.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_plug_submodule (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint32_t module_ident,
   uint32_t submodule_ident,
   pnet_submodule_dir_t direction,
   uint16_t length_input,
   uint16_t length_output);

/**
 * Pull a module from a slot.
 *
 * This function may be used to unplug a module from a specific slot.
 *
 * This function internally calls pnet_pull_submodule() on any plugged
 * sub-modules before pulling the module itself.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:    The API identifier.
 * @param slot             In:    The slot number.
 * @return  0  if a module could be pulled from the slot.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_pull_module (pnet_t * net, uint32_t api, uint16_t slot);

/**
 * Pull a sub-module from a sub-slot.
 *
 * This function may be used to unplug a sub-module from a specific sub-slot.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:    The API identifier.
 * @param slot             In:    The slot number.
 * @param subslot          In:    The sub-slot number.
 * @return  0  if the sub-module could be pulled from the sub-slot.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_pull_submodule (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot);

/**
 * Updates the IOPS and data of one sub-slot to send to the controller.
 *
 * This function may be called to set new data and IOPS values of a sub-slot to
 * send to the controller.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:    The API.
 * @param slot             In:    The slot.
 * @param subslot          In:    The sub-slot.
 * @param p_data           In:    Data buffer. If NULL the data will not
 *                                be updated.
 * @param data_len         In:    Bytes in data buffer.
 * @param iops             In:    The device provider status.
 *                                See pnet_ioxs_values_t
 * @return  0  if a sub-module data and IOPS was set.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_input_set_data_and_iops (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   const uint8_t * p_data,
   uint16_t data_len,
   uint8_t iops);

/**
 * Fetch the controller consumer status of one sub-slot.
 *
 * This function is used to retrieve the consumer status (IOCS) value of
 * an input sub-slot (data sent to the controller) back from the controller.
 *
 * Note that this function will reset the \a p_new_flag flag that is available
 * in the \a pnet_output_get_data_and_iops() function. See that documentation
 * entry for details.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:    The API.
 * @param slot             In:    The slot.
 * @param subslot          In:    The sub-slot.
 * @param p_iocs           Out:   The controller consumer status.
 *                                See pnet_ioxs_values_t
 * @return  0  if a sub-module IOCS was set.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_input_get_iocs (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint8_t * p_iocs);

/**
 * Retrieve latest sub-slot data and IOPS received from the controller.
 *
 * This function may be called to retrieve the latest data and IOPS values
 * of a sub-slot sent from the controller.
 *
 * Note that this function retrieves output data from the controller. The
 * function \a pnet_input_set_data_and_iops() sends input data to the
 * controller. Those are different data streams, so you can not expect to read
 * back data that has been set by \a pnet_input_set_data_and_iops() using this
 * function.
 *
 * If a valid new data (and IOPS) frame has arrived from the IO-controller since
 * your last call to this function (regardless of the slot/subslot arguments)
 * then the flag \a p_new_flag is set to true, else it is set to false.
 * Note that this does not check whether the data content has changed from
 * any previous frame. This flag will be reset also by using
 * \a pnet_input_get_iocs(), so it is recommended to execute
 * \a pnet_output_get_data_and_iops() first if you are to execute both
 * (and the value of the flag is important to your application).
 *
 * Note that the latest data and IOPS values are copied to the application
 * buffers regardless of the value of \a p_new_flag.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:    The API.
 * @param slot             In:    The slot.
 * @param subslot          In:    The sub-slot.
 * @param p_new_flag       Out:   true if new data.
 * @param p_data           Out:   The received data.
 * @param p_data_len       In:    Size of receive buffer.
 *                         Out:   Received number of data bytes.
 * @param p_iops           Out:   The controller provider status (IOPS).
 *                                See pnet_ioxs_values_t
 * @return  0  if a sub-module data and IOPS is retrieved.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_output_get_data_and_iops (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   bool * p_new_flag,
   uint8_t * p_data,
   uint16_t * p_data_len,
   uint8_t * p_iops);

/**
 * Set the device consumer status for one sub-slot.
 *
 * This function is used to set the device consumer status (IOCS)
 * of a specific sub-slot. It is used for output sub-slots (data from the
 * controller) to send consumer status back to the controller.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:    The API.
 * @param slot             In:    The slot.
 * @param subslot          In:    The sub-slot.
 * @param iocs             In:    The device consumer status.
 *                                See pnet_ioxs_values_t
 * @return  0  if a sub-module IOCS was set.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_output_set_iocs (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint8_t iocs);

/**
 * Set the state to "Primary" or "Backup" in the cyclic data sent to the
 * IO-Controller.
 *
 * Implements the "Local Set State" primitive.
 *
 * By default the cyclic communication is "Primary".
 *
 * @param net              InOut: The p-net stack instance
 * @param primary          In:    true to set state to "Primary".
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_set_primary_state (pnet_t * net, bool primary);

/**
 * Set the state to redundant in the cyclic data sent to the IO-Controller.
 *
 * Implements the "Local Set Redundancy State" primitive.
 *
 * By default the cyclic communication is not in redundant state.
 *
 * @param net              InOut: The p-net stack instance
 * @param redundant        In:    true to set state to "redundant".
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_set_redundancy_state (pnet_t * net, bool redundant);

/**
 * Implements the "Local set Provider State" primitive.
 *
 * The application should call this function at least once during startup to set
 * the provider status of frames sent to the controller.
 *
 * The application may call this function at any time, e.g., to signal a
 * temporary inability to produce new application data to the controller.
 *
 * Its effect is similar to setting IOPS to PNET_IOXS_BAD for all sub-slots.
 *
 * @param net              InOut: The p-net stack instance
 * @param run              In:    true to set state to "run".
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_set_provider_state (pnet_t * net, bool run);

/**
 * Application requests abortion of the connection.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:    The AREP
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_ar_abort (pnet_t * net, uint32_t arep);

/**
 * Application requests factory reset of the device.
 *
 * Use this for example when a local hardware switch is used to do a
 * factory reset.
 *
 * Also closes any open connections.
 *
 * @param net              InOut: The p-net stack instance
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_factory_reset (pnet_t * net);

/**
 * Delete data files.
 *
 * Mainly intended for device development and testing.
 * Applications should typically not use this function.
 *
 * Use \a pnet_factory_reset() instead, that uses this functionality internally.
 *
 * @param file_directory   In:    File directory
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_remove_data_files (const char * file_directory);

/**
 * Fetch error information from the AREP.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:    The AREP.
 * @param p_err_cls        Out:   The error class. See PNET_ERROR_CODE_1_*
 * @param p_err_code       Out:   The error code. See PNET_ERROR_CODE_2_*
 * @return  0  If the AREP is valid.
 *          -1 if the AREP is not valid.
 */
PNET_EXPORT int pnet_get_ar_error_codes (
   pnet_t * net,
   uint32_t arep,
   uint16_t * p_err_cls,
   uint16_t * p_err_code);

/**
 * Application creates an entry in the log book.
 *
 * This function is used to insert an entry into the Profinet log-book.
 * Controllers may read the entire log-book using IODRead requests.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:    The AREP.
 * @param p_pnio_status    In:    The PNIO status.
 * @param entry_detail     In:    Additional application information.
 *                                Manufacturer specific.
 */
PNET_EXPORT void pnet_create_log_book_entry (
   pnet_t * net,
   uint32_t arep,
   const pnet_pnio_status_t * p_pnio_status,
   uint32_t entry_detail);

/**
 * Application issues a process alarm.
 *
 * This function may be used by the application to issue a process alarm.
 * Such alarms are sent to the controller using high-priority alarm messages.
 *
 * Optional payload may be attached to the alarm. If \a payload_usi is != 0
 * and \a payload_len > 0 then
 * the \a payload_usi and the payload at \a p_payload is attached to the alarm.
 *
 * After calling this function the application must wait for
 * the \a pnet_alarm_cnf() callback before sending another alarm.
 * This function fails if the application does not wait for the
 * \a pnet_alarm_cnf() between sending two alarms.
 *
 * Note that the PLC can set the max alarm payload length at startup. This
 * limit can be 200 to 1432 bytes.
 *
 * This functionality is used for alarms triggered by the IO-device.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:    The AREP.
 * @param api              In:    The API.
 * @param slot             In:    The slot.
 * @param subslot          In:    The sub-slot.
 * @param payload_usi      In:    The USI for the payload. Max 0x7fff
 * @param payload_len      In:    Length in bytes of the payload. May be 0.
 *                                Max PNET_MAX_ALARM_PAYLOAD_DATA_SIZE or
 *                                value from PLC.
 * @param p_payload        In:    The alarm payload (USI specific format).
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try
 * later).
 */
PNET_EXPORT int pnet_alarm_send_process_alarm (
   pnet_t * net,
   uint32_t arep,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t payload_usi,
   uint16_t payload_len,
   const uint8_t * p_payload);

/**
 * Application acknowledges the reception of an alarm from the controller.
 *
 * This function sends an ACK to the controller.
 * This function must be called by the application after receiving an alarm
 * in the \a pnet_alarm_ind() call-back. Failure to do so within the timeout
 * specified in the connect of the controller will make the controller
 * re-send the alarm.
 *
 * This functionality is used for alarms triggered by the IO-controller.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:    The AREP.
 * @param p_alarm_argument In:    The alarm argument (with slot, subslot,
 *                                alarm_type etc)
 * @param p_pnio_status    In:    Detailed ACK status.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_alarm_send_ack (
   pnet_t * net,
   uint32_t arep,
   const pnet_alarm_argument_t * p_alarm_argument,
   const pnet_pnio_status_t * p_pnio_status);

/* ****************************** Diagnosis ****************************** */

#define PNET_CHANNEL_WHOLE_SUBMODULE 0x8000

/**
 * Channel size in bits. Values 8..255 are reserved.
 */
typedef enum pnet_diag_ch_prop_type_values
{
   PNET_DIAG_CH_PROP_TYPE_UNSPECIFIED = 0,
   PNET_DIAG_CH_PROP_TYPE_1_BIT = 1,
   PNET_DIAG_CH_PROP_TYPE_2_BIT = 2,
   PNET_DIAG_CH_PROP_TYPE_4_BIT = 3,
   PNET_DIAG_CH_PROP_TYPE_8_BIT = 4,
   PNET_DIAG_CH_PROP_TYPE_16_BIT = 5,
   PNET_DIAG_CH_PROP_TYPE_32_BIT = 6,
   PNET_DIAG_CH_PROP_TYPE_64_BIT = 7,
} pnet_diag_ch_prop_type_values_t;

/** Channel group or individual channel. Also known as "Accumulative" */
typedef enum pnet_diag_ch_group_values
{
   PNET_DIAG_CH_INDIVIDUAL_CHANNEL = 0,
   PNET_DIAG_CH_CHANNEL_GROUP = 1
} pnet_diag_ch_group_values_t;

/**
 * Channel direction.
 */
typedef enum pnet_diag_ch_prop_dir_values
{
   PNET_DIAG_CH_PROP_DIR_MANUF_SPEC = 0,
   PNET_DIAG_CH_PROP_DIR_INPUT = 1,
   PNET_DIAG_CH_PROP_DIR_OUTPUT = 2,
   PNET_DIAG_CH_PROP_DIR_INOUT = 3,
} pnet_diag_ch_prop_dir_values_t;

/**
 * Diagnosis severity.
 */
typedef enum pnet_diag_ch_prop_maint_values
{
   PNET_DIAG_CH_PROP_MAINT_FAULT = 0,
   PNET_DIAG_CH_PROP_MAINT_REQUIRED = 1,
   PNET_DIAG_CH_PROP_MAINT_DEMANDED = 2,
   PNET_DIAG_CH_PROP_MAINT_QUALIFIED = 3,
} pnet_diag_ch_prop_maint_values_t;

/**
 * Diagnosis source.
 */
typedef struct pnet_diag_source
{
   uint32_t api;
   uint16_t slot;
   uint16_t subslot;

   /** 0 - 0x7FFF manufacturer specific, 0x8000 whole submodule */
   uint16_t ch;

   pnet_diag_ch_group_values_t ch_grouping;
   pnet_diag_ch_prop_dir_values_t ch_direction;
} pnet_diag_source_t;

/**
 * Add a diagnosis entry, in the standard format.
 *
 * The channel error types and the extended channel error types are
 * defined in Profinet 2.4 Services Annex F.
 *
 * This sends a diagnosis alarm.
 *
 * Uses the "Qualified channel diagnosis" format on the wire if the
 * configuration parameter use_qualified_diagnosis is true, otherwise
 * "Extended channel diagnosis".
 *
 * @param net                 InOut: The p-net stack instance.
 * @param p_diag_source       In:    Slot, subslot, channel, direction etc.
 * @param ch_bits             In:    Number of bits in the channel.
 * @param severity            In:    Diagnosis severity.
 * @param ch_error_type       In:    The channel error type.
 * @param ext_ch_error_type   In:    The extended channel error type
 *                                   (more details on the channel error type).
 *                                   Use 0 if not given.
 * @param ext_ch_add_value    In:    The extended channel error additional
 *                                   value. Typically a measurement value (for
 *                                   example number of dropped packets) related
 *                                   to the error.
 * @param qual_ch_qualifier   In:    More detailed severity information. Used
 *                                   when severity =
 *                                   PNET_DIAG_CH_PROP_MAINT_QUALIFIED and the
 *                                   use_qualified_diagnosis is enabled.
 *                                   Max one bit should be set.
 *                                   Use 0 otherwise.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_diag_std_add (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   pnet_diag_ch_prop_type_values_t ch_bits,
   pnet_diag_ch_prop_maint_values_t severity,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value,
   uint32_t qual_ch_qualifier);

/**
 * Update the "extended channel error additional value", for a diagnosis entry
 * in the standard format.
 *
 * The Profinet standard says that diagnosis updates should not be done at a
 * higher frequency than 1 Hz.
 *
 * This sends a diagnosis alarm.
 *
 * @param net               InOut: The p-net stack instance.
 * @param p_diag_source     In:    Slot, subslot, channel, direction etc.
 * @param ch_error_type     In:    The channel error type.
 * @param ext_ch_error_type In:    The extended channel error type
 *                                 (more details on the channel error type).
 *                                 Use 0 if not given.
 * @param ext_ch_add_value  In:    New extended channel error additional value.
 *                                 Typically a measurement value (for
 *                                 example number of dropped packets) related
 *                                 to the error.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_diag_std_update (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value);

/**
 * Remove a diagnosis entry in the standard format.
 *
 * An error is returned if the diagnosis doesn't exist.
 *
 * This sends a diagnosis alarm.
 *
 * @param net               InOut: The p-net stack instance.
 * @param p_diag_source     In:    Slot, subslot, channel, direction etc.
 * @param ch_error_type     In:    The channel error type.
 * @param ext_ch_error_type In:    The extended channel error type
 *                                 (more details on the channel error type).
 *                                 Use 0 if not given.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_diag_std_remove (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type);

/**
 * Add a diagnosis entry in manufacturer-specified ("USI") format.
 *
 * If the diagnosis already exists, it is updated.
 * Use \a pnet_diag_usi_update() instead if you would like to have an error
 * if the diagnosis is missing when trying to update it.
 *
 * A diagnosis in USI format is always assigned to the channel “whole submodule”
 * (not individual channels). The severity is always “Fault”, the accumulative
 * flag is false and the direction is "Manufacturer specific".
 *
 * It is recommended to use the standard diagnosis format in most cases.
 * Use the manufacturer specific format ("USI format") only if it's not
 * possible to use the standard format.
 *
 * Note that the PLC can set the max alarm payload length at startup, and
 * that affects how large diagnosis entries can be sent via alarms.
 * This payload limit can be 200 to 1432 bytes.
 *
 * This sends a diagnosis alarm.
 *
 * @param net              InOut: The p-net stack instance.
 * @param api              In:    The API.
 * @param slot             In:    The slot.
 * @param subslot          In:    The sub-slot.
 * @param usi              In:    The USI. Range 0..0x7fff
 * @param manuf_data_len   In:    Length in bytes of the
 *                                manufacturer specific diagnosis data.
 *                                A value 0 is allowed.
 *                                Max PNET_MAX_DIAG_MANUF_DATA_SIZE or value
 *                                from PLC.
 * @param p_manuf_data     In:    The manufacturer specific diagnosis data.
 *                                Mandatory if manuf_data_len > 0, otherwise
 *                                NULL.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_diag_usi_add (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t usi,
   uint16_t manuf_data_len,
   const uint8_t * p_manuf_data);

/**
 * Update the manufacturer specific data, for a diagnosis entry in
 * manufacturer-specified ("USI") format.
 *
 * An error is returned if the diagnosis doesn't exist.
 * Use \a pnet_diag_usi_add() instead if you would like to create the
 * missing diagnosis when trying to update it.
 *
 * Note that the PLC can set the max alarm payload length at startup, and
 * that affects how large diagnosis entries can be sent via alarms.
 * This payload limit can be 200 to 1432 bytes.
 *
 * The Profinet standard says that diagnosis updates should not be done at a
 * higher frequency than 1 Hz.
 *
 * This sends a diagnosis alarm.
 *
 * @param net              InOut: The p-net stack instance.
 * @param api              In:    The API.
 * @param slot             In:    The slot.
 * @param subslot          In:    The sub-slot.
 * @param usi              In:    The USI. Range 0..0x7fff
 * @param manuf_data_len   In:    Length in bytes of the
 *                                manufacturer specific diagnosis data.
 *                                A value 0 is allowed.
 *                                Max PNET_MAX_DIAG_MANUF_DATA_SIZE or
 *                                value from PLC.
 * @param p_manuf_data     In:    New manufacturer specific diagnosis data.
 *                                Mandatory if manuf_data_len > 0, otherwise
 *                                NULL.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_diag_usi_update (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t usi,
   uint16_t manuf_data_len,
   const uint8_t * p_manuf_data);

/**
 * Remove a diagnosis entry in manufacturer-specified ("USI") format.
 *
 * An error is returned if the diagnosis doesn't exist.
 *
 * This sends a diagnosis alarm.
 *
 * @param net              InOut: The p-net stack instance.
 * @param api              In:    The API.
 * @param slot             In:    The slot.
 * @param subslot          In:    The sub-slot.
 * @param usi              In:    The USI. Range 0..0x7fff
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_diag_usi_remove (
   pnet_t * net,
   uint32_t api,
   uint16_t slot,
   uint16_t subslot,
   uint16_t usi);

/**
 * Add a diagnosis entry, in standard or USI format.
 *
 * This is a low-level function. Typically you should use one of these instead
 * (which uses this function internally):
 *  - pnet_diag_std_add()
 *  - pnet_diag_usi_add()
 *
 * Note that the PLC can set the max alarm payload length at startup, and
 * that affects how large diagnosis entries can be sent via alarms.
 * This payload limit can be 200 to 1432 bytes.
 *
 * This sends a diagnosis alarm.
 *
 * @param net                 InOut: The p-net stack instance.
 * @param p_diag_source       In:    Slot, subslot, channel, direction etc.
 * @param ch_bits             In:    Number of bits in the channel.
 * @param severity            In:    Diagnosis severity.
 * @param ch_error_type       In:    The channel error type.
 * @param ext_ch_error_type   In:    The extended channel error type.
 * @param ext_ch_add_value    In:    The extended channel error additional
 *                                   value.
 * @param qual_ch_qualifier   In:    The qualified channel qualifier.
 * @param usi                 In:    The USI.
 * @param manuf_data_len      In:    Length in bytes of the
 *                                   manufacturer specific diagnosis data.
 *                                   Max PNET_MAX_DIAG_MANUF_DATA_SIZE or
 *                                   value from PLC.
 *                                   (Only needed if USI <= 0x7fff,
 *                                    and may still be 0).
 * @param p_manuf_data        In:    The manufacturer specific diagnosis data.
 *                                   (Only needed if USI <= 0x7fff).
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_diag_add (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   pnet_diag_ch_prop_type_values_t ch_bits,
   pnet_diag_ch_prop_maint_values_t severity,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value,
   uint32_t qual_ch_qualifier,
   uint16_t usi,
   uint16_t manuf_data_len,
   const uint8_t * p_manuf_data);

/**
 * Update a diagnosis entry, in standard or USI format.
 *
 * This is a low-level function. Typically you should use one of these instead
 * (which uses this function internally):
 *  - pnet_diag_std_update()
 *  - pnet_diag_usi_update()
 *
 * If the USI of the item is in the manufacturer-specified range then
 * the USI is used to identify the item.
 * Otherwise the other parameters are used (all must match):
 *  - p_diag_source (all fields)
 *  - Channel error type.
 *  - Extended error type.
 *
 * When the item is found either the manufacturer data is updated (for
 * USI in manufacturer-specific range) or the extended channel additional
 * value is updated.
 *
 * Note that the PLC can set the max alarm payload length at startup, and
 * that affects how large diagnosis entries can be sent via alarms.
 * This payload limit can be 200 to 1432 bytes.
 *
 * This sends a diagnosis alarm.
 *
 * @param net               InOut: The p-net stack instance.
 * @param p_diag_source     In:    Slot, subslot, channel, direction etc.
 * @param ch_error_type     In:    The channel error type.
 * @param ext_ch_error_type In:    The extended channel error type, or 0.
 * @param ext_ch_add_value  In:    New extended channel error additional value.
 * @param usi               In:    The USI.
 * @param manuf_data_len    In:    Length in bytes of the
 *                                 manufacturer specific diagnosis data.
 *                                 Max PNET_MAX_DIAG_MANUF_DATA_SIZE or
 *                                 value from PLC.
 *                                 (Only needed if USI <= 0x7fff,
 *                                  and may still be 0).
 * @param p_manuf_data      In:    New manufacturer specific diagnosis data.
 *                                 (Only needed if USI <= 0x7fff).
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_diag_update (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value,
   uint16_t usi,
   uint16_t manuf_data_len,
   const uint8_t * p_manuf_data);

/**
 * Remove a diagnosis entry, in standard or USI format.
 *
 * This is a low-level function. Typically you should use one of these instead
 * (which uses this function internally):
 *  - pnet_diag_std_remove()
 *  - pnet_diag_usi_remove()
 *
 * If the USI of the item is in the manufacturer-specified range then
 * the USI is used to identify the item.
 * Otherwise the other parameters are used (all must match):
 *  - p_diag_source (all fields)
 *  - Channel error type.
 *  - Extended error type.
 *
 * This sends a diagnosis alarm.
 *
 * @param net               InOut: The p-net stack instance.
 * @param p_diag_source     In:    Slot, subslot, channel, direction etc.
 * @param ch_error_type     In:    The channel error type.
 * @param ext_ch_error_type In:    The extended channel error type, or 0.
 * @param usi               In:    The USI.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_diag_remove (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint16_t usi);

/******************** Show Profinet stack info ********************************/

/**
 * Show information from the Profinet stack.
 *
 * @param net              InOut: The p-net stack instance
 * @param level            In:   The amount of detail to show.
 *
 *     0x0010              | Show compile time options
 *     0x0020              | Show CMDEV
 *     0x0080              | Show SNMP
 *     0x0100              | Show Ports
 *     0x0200              | Show diagnosis
 *     0x0400              | Show logbook
 *     0x0800              | Show all sessions.
 *     0x1000              | Show all ARs.
 *     0x1001              |     include IOCR.
 *     0x1002              |     include data_descriptors.
 *     0x1003              |     include IOCR and data_descriptors.
 *     0x2000              | Show config/CMINA information.
 *     0x4000              | Show scheduler information.
 *     0x8000              | Show I&M data.
 *
 *     Bit in the level parameter:
 *     15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
 *      1                                     I&M
 *         1                                  Scheduler
 *            1                               CMINA
 *               1                            AR
 *                  1                         Sessions
 *                     1                      Logbook
 *                       1                    Diagnosis
 *                         1                  Ports
 *                           1                SNMP
 *                               1            CMDEV
 *                                 1          Options
 *                                       1    More IOCR info on AR
 *                                         1  More IOCR info on AR
 */
PNET_EXPORT void pnet_show (pnet_t * net, unsigned level);

#ifdef __cplusplus
}
#endif

#endif /* PNET_API_H */
