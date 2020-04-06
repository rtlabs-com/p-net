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
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <pnet_export.h>

/**
 * # Profinet Stack Options
 *
 * The defines named `PNET_OPTION_*` may be used to extend or reduce the functionality
 * of the stack. Setting these values to 0 excludes the specific function and may also
 * reduce the memory usage of the Profinet stack.
 *
 * Note that none of these options are currently supported (even if enabled by setting
 * the value to 1), except for parsing the RPC Connect request message and generating the
 * connect RPC Connect response message.
 */
#define PNET_OPTION_FAST_STARTUP                               1
#define PNET_OPTION_PARAMETER_SERVER                           1
#define PNET_OPTION_IR                                         1
#define PNET_OPTION_SR                                         1
#define PNET_OPTION_REDUNDANCY                                 1
#define PNET_OPTION_AR_VENDOR_BLOCKS                           1
#define PNET_OPTION_RS                                         1
#define PNET_OPTION_MC_CR                                      1
#define PNET_OPTION_SRL                                        1

/**
 * Disable use of atomic operations (stdatomic.h).
 * If the compiler supports it then set this define to 1.
 */
#define PNET_USE_ATOMICS                                       0

/**
 * # Memory Usage
 *
 * Memory usage is controlled by the `PNET_MAX_*` defines.
 * Define the required number of supported items.
 *
 * These values directly affect the memory usage of the implementation.
 * Sometimes in complicated ways.
 *
 * Please note that some defines have minimum requirements.
 * These values are used as is by the stack. No validation is performed.
 */
#define PNET_MAX_AR                                            1     /**< Number of connections. Must be > 0. */
#define PNET_MAX_API                                           1     /**< Number of Application Processes. Must be > 0. */
#define PNET_MAX_CR                                            2     /**< Per AR. 1 input and 1 output. */
#define PNET_MAX_MODULES                                       5     /**< Per API. Should be > 1 to allow at least one I/O module. */
#define PNET_MAX_SUBMODULES                                    3     /**< Per module (3 needed for DAP). */
#define PNET_MAX_CHANNELS                                      1     /**< Per sub-slot. Used for diagnosis. */
#define PNET_MAX_DFP_IOCR                                      2     /**< Allowed values are 0 (zero) or 2. */
#define PNET_MAX_PORT                                          1     /**< 2 for media redundancy. Currently only 1 is supported. */
#define PNET_MAX_LOG_BOOK_ENTRIES                              16
#define PNET_MAX_ALARMS                                        3     /**< Per AR and queue. One queue for hi and one for lo alarms. */
#define PNET_MAX_DIAG_ITEMS                                    200   /**< Total, per device. Max is 65534 items. */

#if PNET_OPTION_MC_CR
#define PNET_MAX_MC_CR                                         1     /**< Par AR. */
#endif

#if PNET_OPTION_AR_VENDOR_BLOCKS
#define PNET_MAX_AR_VENDOR_BLOCKS                              1     /**< Must be > 0 */
#define PNET_MAX_AR_VENDOR_BLOCK_DATA_LENGTH                   512
#endif

#define PNET_MAX_MAN_SPECIFIC_FAST_STARTUP_DATA_LENGTH         0     /**< or 512 (bytes) */

/**
 * # GSDML
 * The following values are application-specific and should match what
 * is specified in the GSDML file.
 */

/**
 * A value of 32 allows data exchange every 1 ms (resolution is 32.25us).
 */
#define PNET_MIN_DEVICE_INTERVAL                               32

/* ======================================================================= */
/*                 Do not modify anything below this text!                 */
/* ======================================================================= */

/** Supported block version by this implementation */
#define PNET_BLOCK_VERSION_HIGH                                1
#define PNET_BLOCK_VERSION_LOW                                 0

/** Some blocks (e.g. logbook) uses the following lower version number. */
#define PNET_BLOCK_VERSION_LOW_1                               1

/**
 * # Error Codes
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
 * Manufacturer specific 0x20-0x3F                                  (LogBookData)
 * Reserved 0x40-0x80
 */
#define PNET_ERROR_CODE_PNIO                                   0x81     /** All other errors */
/** Reserved 0x82-0xDE */
#define PNET_ERROR_CODE_RTA_ERROR                              0xCF     /** In ERR-RTA-PDU and ERR-UDP-PDU */
/** Reserved 0xD0-0xD9 */
#define PNET_ERROR_CODE_ALARM_ACK                              0xDA     /** In DATA-RTA-PDU and DATA-UDP-PDU */
#define PNET_ERROR_CODE_CONNECT                                0xDB     /** CL-RPC-PDU */
#define PNET_ERROR_CODE_RELEASE                                0xDC     /** CL-RPC-PDU */
#define PNET_ERROR_CODE_CONTROL                                0xDD     /** CL-RPC-PDU */
#define PNET_ERROR_CODE_READ                                   0xDE     /** Only with PNIORW */
#define PNET_ERROR_CODE_WRITE                                  0xDF     /** Only with PNIORW */
/** Reserved 0xE0-0xFF */

/**
 * # Values used in error_decode.
 *
 * Reserved 0x00 (No error)
 *
 * Reserved 0x01-0x7F
 */
#define PNET_ERROR_DECODE_PNIORW                               0x80     /** Only Read/Write */
#define PNET_ERROR_DECODE_PNIO                                 0x81
#define PNET_ERROR_DECODE_MANUFACTURER_SPECIFIC                0x82
/** Reserved 0x83-0xFF */

/**
 * # List of error_code_1 values, bits 4..7, for PNET_ERROR_DECODE_PNIORW
 */
#define PNET_ERROR_CODE_1_PNIORW_APP                           0xa0
#define PNET_ERROR_CODE_1_PNIORW_ACC                           0xb0
#define PNET_ERROR_CODE_1_PNIORW_RES                           0xc0

/**
 * # List of error_code_1 values, bits 0..3, for PNET_ERROR_CODE_1_PNIORW_APP
 */
#define PNET_ERROR_CODE_1_APP_READ_ERROR                       (0x00 + PNET_ERROR_CODE_1_PNIORW_APP)
#define PNET_ERROR_CODE_1_APP_WRITE_ERROR                      (0x01 + PNET_ERROR_CODE_1_PNIORW_APP)
#define PNET_ERROR_CODE_1_APP_MODULE_FAILURE                   (0x02 + PNET_ERROR_CODE_1_PNIORW_APP)
#define PNET_ERROR_CODE_1_APP_BUSY                             (0x07 + PNET_ERROR_CODE_1_PNIORW_APP)
#define PNET_ERROR_CODE_1_APP_VERSION_CONFLICT                 (0x08 + PNET_ERROR_CODE_1_PNIORW_APP)
#define PNET_ERROR_CODE_1_APP_NOT_SUPPORTED                    (0x09 + PNET_ERROR_CODE_1_PNIORW_APP)

/**
 * # List of error_code_1 values, bits 0..3, for PNET_ERROR_CODE_1_PNIORW_ACC
 */
#define PNET_ERROR_CODE_1_ACC_INVALID_INDEX                    (0x00 + PNET_ERROR_CODE_1_PNIORW_ACC)
#define PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR               (0x01 + PNET_ERROR_CODE_1_PNIORW_ACC)
#define PNET_ERROR_CODE_1_ACC_INVALID_SLOT_SUBSLOT             (0x02 + PNET_ERROR_CODE_1_PNIORW_ACC)
#define PNET_ERROR_CODE_1_ACC_TYPE_CONFLICT                    (0x03 + PNET_ERROR_CODE_1_PNIORW_ACC)
#define PNET_ERROR_CODE_1_ACC_INVALID_AREA_API                 (0x04 + PNET_ERROR_CODE_1_PNIORW_ACC)
#define PNET_ERROR_CODE_1_ACC_STATE_CONFLICT                   (0x05 + PNET_ERROR_CODE_1_PNIORW_ACC)
#define PNET_ERROR_CODE_1_ACC_ACCESS_DENIED                    (0x06 + PNET_ERROR_CODE_1_PNIORW_ACC)
#define PNET_ERROR_CODE_1_ACC_INVALID_RANGE                    (0x07 + PNET_ERROR_CODE_1_PNIORW_ACC)
#define PNET_ERROR_CODE_1_ACC_INVALID_PARAMETER                (0x08 + PNET_ERROR_CODE_1_PNIORW_ACC)
#define PNET_ERROR_CODE_1_ACC_INVALID_TYPE                     (0x09 + PNET_ERROR_CODE_1_PNIORW_ACC)
#define PNET_ERROR_CODE_1_ACC_BACKUP                           (0x0a + PNET_ERROR_CODE_1_PNIORW_ACC)

/**
 * # List of error_code_1 values, bits 0..3, for PNET_ERROR_CODE_1_PNIORW_RES
 */
#define PNET_ERROR_CODE_1_RES_READ_CONFLICT                    (0x00 + PNET_ERROR_CODE_1_PNIORW_RES)
#define PNET_ERROR_CODE_1_RES_WRITE_CONFLICT                   (0x01 + PNET_ERROR_CODE_1_PNIORW_RES)
#define PNET_ERROR_CODE_1_RES_RESOURCE_BUSY                    (0x02 + PNET_ERROR_CODE_1_PNIORW_RES)
#define PNET_ERROR_CODE_1_RES_RESOURCE_UNAVAILABLE             (0x03 + PNET_ERROR_CODE_1_PNIORW_RES)

/**
 * # List of error_code_1 values, for PNET_ERROR_DECODE_PNIO
 */
#define PNET_ERROR_CODE_1_CONN_FAULTY_AR_BLOCK_REQ             0x01
#define PNET_ERROR_CODE_1_CONN_FAULTY_IOCR_BLOCK_REQ           0x02
#define PNET_ERROR_CODE_1_CONN_FAULTY_EXP_BLOCK_REQ            0x03
#define PNET_ERROR_CODE_1_CONN_FAULTY_ALARM_BLOCK_REQ          0x04
#define PNET_ERROR_CODE_1_CONN_FAULTY_PRM_SERVER_BLOCK_REQ     0x05
#define PNET_ERROR_CODE_1_CONN_FAULTY_MCR_BLOCK_REQ            0x06
#define PNET_ERROR_CODE_1_CONN_FAULTY_AR_RPC_BLOCK_REQ         0x07
#define PNET_ERROR_CODE_1_CONN_FAULTY_FAULTY_RECORD            0x08
#define PNET_ERROR_CODE_1_CONN_FAULTY_IR_INFO                  0x09
#define PNET_ERROR_CODE_1_CONN_FAULTY_SR_INFO                  0x0A
#define PNET_ERROR_CODE_1_CONN_FAULTY_AR_FSU                   0x0B
#define PNET_ERROR_CODE_1_CONN_FAULTY_VENDOR                   0x0C
#define PNET_ERROR_CODE_1_CONN_FAULTY_RSINFO                   0x0D
/** Reserved 0x0E-0x13 */
#define PNET_ERROR_CODE_1_DCTRL_FAULTY_CONNECT                 0x14
#define PNET_ERROR_CODE_1_DCTRL_FAULTY_CONNECT_PLUG            0x15
#define PNET_ERROR_CODE_1_XCTRL_FAULTY_CONNECT                 0x16
#define PNET_ERROR_CODE_1_XCTRL_FAULTY_CONNECT_PLUG            0x17
#define PNET_ERROR_CODE_1_DCTRL_FAULTY_CONNECT_PRMBEG          0x18
#define PNET_ERROR_CODE_1_DCTRL_FAULTY_CONNECT_SUBMODLIST      0x19
/** Reserved 0x1A-0x27 */
#define PNET_ERROR_CODE_1_RELS_FAULTY_BLOCK                    0x28
/** Reserved 0x29-0x31 */
/** Reserved 0x39-0x3B */
#define PNET_ERROR_CODE_1_ALARM_ACK                            0x3C
#define PNET_ERROR_CODE_1_CMDEV                                0x3D
#define PNET_ERROR_CODE_1_CMCTL                                0x3E
#define PNET_ERROR_CODE_1_CTLDINA                              0x3F
#define PNET_ERROR_CODE_1_CMRPC                                0x40
#define PNET_ERROR_CODE_1_ALPMI                                0x41
#define PNET_ERROR_CODE_1_ALPMR                                0x42
#define PNET_ERROR_CODE_1_LMPM                                 0x43
#define PNET_ERROR_CODE_1_MAC                                  0x44
#define PNET_ERROR_CODE_1_RPC                                  0x45
#define PNET_ERROR_CODE_1_APMR                                 0x46
#define PNET_ERROR_CODE_1_APMS                                 0x47
#define PNET_ERROR_CODE_1_CPM                                  0x48
#define PNET_ERROR_CODE_1_PPM                                  0x49
#define PNET_ERROR_CODE_1_DCPUCS                               0x4A
#define PNET_ERROR_CODE_1_DCPUCR                               0x4B
#define PNET_ERROR_CODE_1_DCPMCS                               0x4C
#define PNET_ERROR_CODE_1_DCPMCR                               0x4D
#define PNET_ERROR_CODE_1_FSPM                                 0x4E
/** Reserved 0x4F-0x63 */
/** CTRLxxx  0x64-0xC7 */
#define PNET_ERROR_CODE_1_CMSM                                 0xC8
#define PNET_ERROR_CODE_1_CMRDR                                0xCA
#define PNET_ERROR_CODE_1_CMWRR                                0xCC
#define PNET_ERROR_CODE_1_CMIO                                 0xCD
#define PNET_ERROR_CODE_1_CMSU                                 0xCE
#define PNET_ERROR_CODE_1_CMINA                                0xD0
#define PNET_ERROR_CODE_1_CMPBE                                0xD1
#define PNET_ERROR_CODE_1_CMSRL                                0xD2
#define PNET_ERROR_CODE_1_CMDMC                                0xD3
/** CTRLxxx  0xD4-0xFC */
#define PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL                 0xFD
/** Reserved 0xFE */
#define PNET_ERROR_CODE_1_USER_SPECIFIC                        0xFF

/**
 * # List of error_code_2 values (not exhaustive)
 */
#define PNET_ERROR_CODE_2_CMDEV_STATE_CONFLICT                 0x00
#define PNET_ERROR_CODE_2_CMDEV_RESOURCE                       0x01

#define PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID             0x00
#define PNET_ERROR_CODE_2_CMRPC_UNKNOWN_BLOCKS                 0x01
#define PNET_ERROR_CODE_2_CMRPC_IOCR_MISSING                   0x02
#define PNET_ERROR_CODE_2_CMRPC_WRONG_BLOCK_COUNT              0x03
#define PNET_ERROR_CODE_2_CMRPC_NO_AR_RESOURCES                0x04
#define PNET_ERROR_CODE_2_CMRPC_AR_UUID_UNKNOWN                0x05
#define PNET_ERROR_CODE_2_CMRPC_STATE_CONFLICT                 0x06
#define PNET_ERROR_CODE_2_CMRPC_OUT_OF_PCA_RESOURCES           0x07 /** PCA = Provider, Consumer or Alarm */
#define PNET_ERROR_CODE_2_CMRPC_OUT_OF_MEMORY                  0x08
#define PNET_ERROR_CODE_2_CMRPC_PDEV_ALREADY_OWNED             0x09
#define PNET_ERROR_CODE_2_CMRPC_ARSET_STATE_CONFLICT           0x0A
#define PNET_ERROR_CODE_2_CMRPC_ARSET_PARAM_CONFLICT           0x0B
#define PNET_ERROR_CODE_2_CMRPC_PDEV_PORTS_WO_INTERFACE        0x0C

#define PNET_ERROR_CODE_2_CMSM_INVALID_STATE                   0x00
#define PNET_ERROR_CODE_2_CMSM_SIGNALED_ERROR                  0x01

#define PNET_ERROR_CODE_2_CMRDR_INVALID_STATE                  0x00
#define PNET_ERROR_CODE_2_CMRDR_SIGNALED_ERROR                 0x01

#define PNET_ERROR_CODE_2_CMWRR_INVALID_STATE                  0x00
#define PNET_ERROR_CODE_2_CMWRR_AR_STATE_NOT_PRIMARY           0x01
#define PNET_ERROR_CODE_2_CMWRR_SIGNALED_ERROR                 0x02

#define PNET_ERROR_CODE_2_CMIO_INVALID_STATE                   0x00
#define PNET_ERROR_CODE_2_CMIO_SIGNALED_ERROR                  0x01

#define PNET_ERROR_CODE_2_CMSU_INVALID_STATE                   0x00
#define PNET_ERROR_CODE_2_CMSU_AR_ADD_PROV_CONS_FAILED         0x01
#define PNET_ERROR_CODE_2_CMSU_AR_ALARM_OPEN_FAILED            0x02
#define PNET_ERROR_CODE_2_CMSU_AR_ALARM_SEND                   0x03
#define PNET_ERROR_CODE_2_CMSU_AR_ALARM_ACK_SEND               0x04
#define PNET_ERROR_CODE_2_CMSU_AR_ALARM_IND                    0x05

#define PNET_ERROR_CODE_2_CMINA_INVALID_STATE                  0x00
#define PNET_ERROR_CODE_2_CMINA_SIGNALED_ERROR                 0x01

#define PNET_ERROR_CODE_2_CMPBE_INVALID_STATE                  0x00
#define PNET_ERROR_CODE_2_CMPBE_SIGNALED_ERROR                 0x01

#define PNET_ERROR_CODE_2_CMSRL_INVALID_STATE                  0x00
#define PNET_ERROR_CODE_2_CMSRL_SIGNALED_ERROR                 0x01

#define PNET_ERROR_CODE_2_CMDMC_INVALID_STATE                  0x00
#define PNET_ERROR_CODE_2_CMDMC_SIGNALED_ERROR                 0x01

#define PNET_ERROR_CODE_2_CPM_INVALID_STATE                    0x00
#define PNET_ERROR_CODE_2_CPM_INVALID                          0x01

#define PNET_ERROR_CODE_2_PPM_INVALID_STATE                    0x00
#define PNET_ERROR_CODE_2_PPM_INVALID                          0x01

#define PNET_ERROR_CODE_2_APMS_INVALID_STATE                   0x00
#define PNET_ERROR_CODE_2_APMS_LMPM_ERROR                      0x01
#define PNET_ERROR_CODE_2_APMS_TIMEOUT                         0x02
/* Reserved 0x03..0xff */

#define PNET_ERROR_CODE_2_APMR_INVALID_STATE                   0x00
#define PNET_ERROR_CODE_2_APMR_LMPM_ERROR                      0x01
/* Reserved 0x02..0xff */

#define PNET_ERROR_CODE_2_ALPMI_INVALID_STATE                  0x00
#define PNET_ERROR_CODE_2_ALPMI_WRONG_ACK_PDU                  0x01
#define PNET_ERROR_CODE_2_ALPMI_INVALID                        0x02
#define PNET_ERROR_CODE_2_ALPMI_WRONG_STATE                    0x03
/* Reserved 0x04..0xff */

#define PNET_ERROR_CODE_2_ALPMR_INVALID_STATE                  0x00
#define PNET_ERROR_CODE_2_ALPMR_WRONG_ALARM_PDU                0x01
#define PNET_ERROR_CODE_2_ALPMR_INVALID                        0x02
#define PNET_ERROR_CODE_2_ALPMR_WRONG_STATE                    0x03
/* Reserved 0x04..0xff */

/**
 * # List of error_code_2 values, for
 * PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL (not exhaustive).
 */
#define PNET_ERROR_CODE_2_ABORT_CODE_SEQ                       0x01
#define PNET_ERROR_CODE_2_ABORT_INSTANCE_CLOSED                0x02
#define PNET_ERROR_CODE_2_ABORT_AR_OUT_OF_MEMORY               0x03
#define PNET_ERROR_CODE_2_ABORT_AR_ADD_CPM_PPM_FAILED          0x04
#define PNET_ERROR_CODE_2_ABORT_AR_CONSUMER_DHT_EXPIRED        0x05
#define PNET_ERROR_CODE_2_ABORT_AR_CMI_TIMEOUT                 0x06
#define PNET_ERROR_CODE_2_ABORT_AR_ALARM_OPEN_FAILED           0x07
#define PNET_ERROR_CODE_2_ABORT_AR_ALARM_SEND_CNF_NEG          0x08
#define PNET_ERROR_CODE_2_ABORT_AR_ALARM_ACK_SEND_CNF_NEG      0x09
#define PNET_ERROR_CODE_2_ABORT_AR_ALARM_DATA_TOO_LONG         0x0a
#define PNET_ERROR_CODE_2_ABORT_AR_ALARM_IND_ERROR             0x0b
#define PNET_ERROR_CODE_2_ABORT_AR_RPC_CLIENT_CALL_CNF_NEG     0x0c
#define PNET_ERROR_CODE_2_ABORT_AR_ABORT_REQ                   0x0d
#define PNET_ERROR_CODE_2_ABORT_AR_RE_RUN_ABORTS_EXISTING      0x0e
#define PNET_ERROR_CODE_2_ABORT_AR_RELEASE_IND_RECEIVED        0x0f
#define PNET_ERROR_CODE_2_ABORT_AR_DEVICE_DEACTIVATED          0x10
#define PNET_ERROR_CODE_2_ABORT_AR_REMOVED                     0x11
#define PNET_ERROR_CODE_2_ABORT_AR_PROTOCOL_VIOLATION          0x12
#define PNET_ERROR_CODE_2_ABORT_AR_NAME_RESOLUTION_ERROR       0x13
#define PNET_ERROR_CODE_2_ABORT_AR_RPC_BIND_ERROR              0x14
#define PNET_ERROR_CODE_2_ABORT_AR_RPC_CONNECT_ERROR           0x15
#define PNET_ERROR_CODE_2_ABORT_AR_RPC_READ_ERROR              0x16
#define PNET_ERROR_CODE_2_ABORT_AR_RPC_WRITE_ERROR             0x17
#define PNET_ERROR_CODE_2_ABORT_AR_RPC_CONTROL_ERROR           0x18
#define PNET_ERROR_CODE_2_ABORT_AR_FORBIDDEN_PULL_OR_PLUG      0x19
#define PNET_ERROR_CODE_2_ABORT_AR_AP_REMOVED                  0x1a
#define PNET_ERROR_CODE_2_ABORT_AR_LINK_DOWN                   0x1b
#define PNET_ERROR_CODE_2_ABORT_AR_MC_REGISTER_FAILED          0x1c
#define PNET_ERROR_CODE_2_ABORT_NOT_SYNCHRONIZED               0x1d
#define PNET_ERROR_CODE_2_ABORT_WRONG_TOPOLOGY                 0x1e
#define PNET_ERROR_CODE_2_ABORT_DCP_STATION_NAME_CHANGED       0x1f
#define PNET_ERROR_CODE_2_ABORT_DCP_RESET_TO_FACTORY           0x20
#define PNET_ERROR_CODE_2_ABORT_PDEV_CHECK_FAILED              0x24

/**
 * The events are sent from CMDEV to the application using the state_cb call-back function.
 */
typedef enum pnet_event_values
{
   PNET_EVENT_ABORT,          /**< The AR has been closed. */
   PNET_EVENT_STARTUP,        /**< A connection has been initiated. */
   PNET_EVENT_PRMEND,         /**< Controller has sent all write records. */
   PNET_EVENT_APPLRDY,        /**< Controller has acknowledged the APPL_RDY message */
   PNET_EVENT_DATA
} pnet_event_values_t;

/**
 * Values used for IOCS and IOPS.
 */
typedef enum pnet_ioxs_values
{
   PNET_IOXS_BAD = 0x00,
   PNET_IOXS_GOOD = 0x80
} pnet_ioxs_values_t;

/**
 * Values used when plugging a sub-module.
 */
typedef enum pnet_submodule_dir
{
   PNET_DIR_NO_IO,
   PNET_DIR_INPUT,
   PNET_DIR_OUTPUT,
   PNET_DIR_IO
} pnet_submodule_dir_t;

/**
 * CControl command codes used in the dcontrol_cb call-back function.
 */
typedef enum pnet_control_command
{
   PNET_CONTROL_COMMAND_PRM_BEGIN,
   PNET_CONTROL_COMMAND_PRM_END,
   PNET_CONTROL_COMMAND_APP_RDY,
   PNET_CONTROL_COMMAND_RELEASE,
   /* ToDo: These are not currently implemented */
   PNET_CONTROL_COMMAND_RDY_FOR_COMPANION,
   PNET_CONTROL_COMMAND_RDY_FOR_RTC3,
} pnet_control_command_t;

/**
 * Data status bits used in the new_data_status_cb call-back function.
 */
typedef enum pnet_data_status_bits
{
   PNET_DATA_STATUS_BIT_STATE = 0,                   /**< 0 => BACKUP, 1 => PRIMARY */
   PNET_DATA_STATUS_BIT_REDUNDANCY,
   PNET_DATA_STATUS_BIT_DATA_VALID,
   PNET_DATA_STATUS_BIT_RESERVED_1,
   PNET_DATA_STATUS_BIT_PROVIDER_STATE,
   PNET_DATA_STATUS_BIT_STATION_PROBLEM_INDICATOR,
   PNET_DATA_STATUS_BIT_RESERVED_2,
   PNET_DATA_STATUS_BIT_IGNORE
} pnet_data_status_bits_t;

 /** Network handle */
typedef struct pnet pnet_t;

/**
 * Profinet stack detailed error information.
 */
typedef struct pnet_pnio_status
{
   uint8_t                 error_code;
   uint8_t                 error_decode;
   uint8_t                 error_code_1;
   uint8_t                 error_code_2;
} pnet_pnio_status_t;

/**
 * Sent to controller on negative result.
 */
typedef struct pnet_result
{
   pnet_pnio_status_t      pnio_status;   /**< Profinet-specific error information. */
   uint16_t                add_data_1;    /**< Application-specific error information. */
   uint16_t                add_data_2;    /**< Application-specific error information. */
} pnet_result_t;

/*
 * Application call-back functions
 *
 * The application should define call-back functions which are called by
 * the stack when specific events occurs within the stack.
 *
 * Note that most of these functions are mandatory in the sense that they must exist
 * and return 0 (zero) for a functioning stack. Some functions are required to
 * perform specific functionality.
 *
 * The call-back functions should return 0 (zero) for a successful call and
 * -1 for an unsuccessful call.
 */

/**
 * Indication to the application that a Connect request was received from the controller.
 *
 * This application call-back function is called by the Profinet stack on every
 * Connect request from the Profinet controller.
 *
 * The connection will be opened if this function returns 0 (zero) and the stack
 * is otherwise able to establish a connection.
 *
 * If this function returns something other than 0 (zero) then the Connect request is
 * refused by the device.
 * In case of error the application should provide error information in \a p_result.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:   The AREP.
 * @param p_result         Out:  Detailed error information.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_connect_ind)(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_result_t           *p_result);

/**
 * Indication to the application that a Release request was received from the controller.
 *
 * This application call-back function is called by the Profinet stack on every
 * Release request from the Profinet controller.
 *
 * The connection will be closed regardless of the return value from this function.
 * In case of error the application should provide error information in \a p_result.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:   The AREP.
 * @param p_result         Out:  Detailed error information.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_release_ind)(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_result_t           *p_result);

/**
 * Indication to the application that a DControl request was received from the controller.
 *
 * This application call-back function is called by the Profinet stack on every
 * DControl request from the Profinet controller.
 *
 * The application is not required to take any action but the function must return 0 (zero)
 * for proper function of the stack.
 * If this function returns something other than 0 (zero) then the DControl request is
 * refused by the device.
 * In case of error the application should provide error information in \a p_result.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:   The AREP.
 * @param control_command  In:   The DControl command code.
 * @param p_result         Out:  Detailed error information.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_dcontrol_ind)(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_control_command_t  control_command,
   pnet_result_t           *p_result);

/**
 * Indication to the application that a CControl confirmation was received from the controller.
 *
 * This application call-back function is called by the Profinet stack on every
 * CControl confirmation from the Profinet controller.
 *
 * The application is not required to take any action.
 * In case of error the application should provide error information in \a p_result.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:   The AREP.
 * @param p_result         Out:  Detailed error information.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_ccontrol_cnf)(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_result_t           *p_result);

/**
 * Indication to the application that a state transition has occurred within the Profinet stack.
 *
 * This application call-back function is called by the Profinet stack on specific
 * state transitions within the Profinet stack.
 *
 * At the very least the application must react to the PNET_EVENT_PRMEND state transition.
 * After this event the application must call pnet_application_ready(), when it has finished
 * its setup and it is ready to exchange data.
 *
 * The return value from this call-back function is ignored by the Profinet stack.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:   The AREP.
 * @param state            In:   The state transition event. See pnet_event_values_t.
 * @return  0  on success.
 *          Other values are ignored.
 */
typedef int (*pnet_state_ind)(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_event_values_t     state);

/**
 * Indication to the application that an IODRead request was received from the controller.
 *
 * This application call-back function is called by the Profinet stack on every
 * IODRead request from the Profinet controller which specify an application-specific
 * value of \a idx (0x0000 - 0x7fff). All other values of \a idx are handled internally
 * by the Profinet stack.
 *
 * The application must verify the value of \a idx, and that \a p_read_length is
 * large enough. Further, the application must provide a
 * pointer to the binary value in \a pp_read_data and the size, in bytes, of the
 * binary value in \a p_read_length.
 *
 * The Profinet stack does not perform any endianess conversion on the binary value.
 *
 * In case of error the application should provide error information in \a p_result.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:   The AREP.
 * @param api              In:   The AP identifier.
 * @param slot             In:   The slot number.
 * @param subslot          In:   The sub-slot number.
 * @param idx              In:   The data record index.
 * @param sequence_number  In:   The sequence number.
 * @param pp_read_data     Out:  A pointer to the binary value.
 * @param p_read_length    InOut: The maximum (in) and actual (out) length in bytes of the binary value.
 * @param p_result         Out:  Detailed error information.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_read_ind)(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   uint16_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                idx,
   uint16_t                sequence_number,
   uint8_t                 **pp_read_data,   /**< Out: A pointer to the data */
   uint16_t                *p_read_length,   /**< Out: Size of data */
   pnet_result_t           *p_result);       /**< Error status if returning != 0 */

/**
 * Indication to the application that an IODWrite request was received from the controller.
 *
 * This application call-back function is called by the Profinet stack on every
 * IODWrite request from the Profinet controller which specify an application-specific
 * value of \a idx (0x0000 - 0x7fff). All other values of \a idx are handled internally
 * by the Profinet stack.
 *
 * The application must verify the values of \a idx and \a write_length and save (copy)
 * the binary value in \a p_write_data. A future IODRead must return the latest written
 * value.
 *
 * The Profinet stack does not perform any endianess conversion on the binary value.
 *
 * In case of error the application should provide error information in \a p_result.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:   The AREP.
 * @param api              In:   The AP identifier.
 * @param slot             In:   The slot number.
 * @param subslot          In:   The sub-slot number.
 * @param idx              In:   The data record index.
 * @param sequence_number  In:   The sequence number.
 * @param write_length     In:   The length in bytes of the binary value.
 * @param p_write_data     In:   A pointer to the binary value.
 * @param p_result         Out:  Detailed error information.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_write_ind)(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   uint16_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                idx,
   uint16_t                sequence_number,
   uint16_t                write_length,
   uint8_t                 *p_write_data,
   pnet_result_t           *p_result);

/**
 * Indication to the application that a module is requested by the controller in a specific slot.
 *
 * This application call-back function is called by the Profinet stack to indicate that the
 * controller has requested the presence of a specific module, ident number \a module_ident,
 * in the slot number \a slot.
 *
 * The application must react to this by configuring itself accordingly (if possible) and
 * call function pnet_plug_module() to configure the stack for this module.
 *
 * If the wrong module ident number is plugged then the stack will accept this, but signal
 * to the controller that a substitute module is fitted.
 *
 * This function should return 0 (zero) if a valid module was plugged. Or return -1 if the
 * application cannot handle this request.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param api              In:   The AP identifier.
 * @param slot             In:   The slot number.
 * @param module_ident     In:   The module ident number.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_exp_module_ind)(
   pnet_t                  *net,
   void                    *arg,
   uint16_t                api,
   uint16_t                slot,
   uint32_t                module_ident);

/**
 * Indication to the application that a sub-module is requested by the controller in a specific sub-slot.
 *
 * This application call-back function is called by the Profinet stack to indicate that the
 * controller has requested the presence of a specific sub-module, ident number \a submodule_ident,
 * in the sub-slot number \a subslot, with module ident number \a module_ident in slot \a slot.
 *
 * If a module has not been plugged in the slot \a slot then an automatic plug request is issued
 * internally by the stack.
 *
 * The application must react to this by configuring itself accordingly (if possible) and
 * call function pnet_plug_submodule() to configure the stack with the correct input/output data
 * sizes.
 *
 * If the wrong sub-module ident number is plugged then the stack will accept this, but signal
 * to the controller that a substitute sub-module is fitted.
 *
 * This function should return 0 (zero) if a valid sub-module was plugged. Or return -1 if the
 * application cannot handle this request.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param api              In:   The AP identifier.
 * @param slot             In:   The slot number.
 * @param subslot          In:   The sub-slot number.
 * @param module_ident     In:   The module ident number.
 * @param submodule_ident  In:   The sub-module ident number.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_exp_submodule_ind)(
   pnet_t                  *net,
   void                    *arg,
   uint16_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint32_t                module_ident,
   uint32_t                submodule_ident);

/**
 * Indication to the application that the data status received from the controller has changed.
 *
 * This application call-back function is called by the Profinet stack to indicate
 * that the received data status has changed.
 *
 * The application is not required by the Profinet stack to take any action. It may
 * use this information as it wishes.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:   The AREP.
 * @param crep             In:   The CREP.
 * @param changes          In:   The changed bits in the received data status.
 * @param data_status      In:   Current received data status (after changes).
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_new_data_status_ind)(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   uint32_t                crep,
   uint8_t                 changes, /**< Only modified bits from pnet_data_status_bits_t */
   uint8_t                 data_status);

/**
 * The controller has sent an alarm to the device.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:   The AREP.
 * @param api              In:   The AP identifier.
 * @param slot             In:   The slot number.
 * @param subslot          In:   The sub-slot number.
 * @param data_len         In:   Data length
 * @param data_usi         In:   Alarm USI
 * @param p_data           InOut: Alarm data
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_alarm_ind)(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                data_len,
   uint16_t                data_usi,
   uint8_t                 *p_data);

/**
 * The controller acknowledges the alarm sent previously.
 * It is now possible to send another alarm.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:   The AREP.
 * @param p_pnio_status    In:   Detailed ACK information.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_alarm_cnf)(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   pnet_pnio_status_t      *p_pnio_status);

/**
 * The controller acknowledges the alarm ACK sent previously.
 *
 * @param net              InOut: The p-net stack instance
 * @param arg              InOut: User-defined data (not used by p-net)
 * @param arep             In:   The AREP.
 * @param res              In:   0  if ACK was received by the remote side.
 *                               -1 if ACK was not received by the remote side.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_alarm_ack_cnf)(
   pnet_t                  *net,
   void                    *arg,
   uint32_t                arep,
   int                     res);

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
 * The return value from this call-back function is ignored by the Profinet stack.
 *
 * It is optional to implement this callback (if you do not have any application
 * data that could be reset).
 *
 * Reset modes:
 *
 * 0:  (Power-on reset, not from IO-controller. Will not trigger this callback.)
 * 1:  Reset application data
 * 2:  Reset communication parameters (done by the stack)
 * 99: Reset all (factory reset).
 *
 * The reset modes 1-9 (out of which 1 and 2 are supported here) are defined
 * by the Profinet standard. Value 99 is used here to indicate that the
 * IO-controller has requested a factory reset via another mechanism.
 *
 * In order to remain responsive to DCP communication and Ethernet switching,
 * the device should not do a hard or soft reset for reset mode 1 or 2. It is
 * allowed for the factory reset case (but not mandatory).
 *
 * @param net                       InOut: The p-net stack instance
 * @param arg                       InOut: User-defined data (not used by p-net)
 * @param should_reset_application  In:    True if the user should reset the application data.
 * @param reset_mode                In:    Detailed reset information.
 * @return  0  on success.
 *          -1 if an error occurred.
 */
typedef int (*pnet_reset_ind)(
   pnet_t                  *net,
   void                    *arg,
   bool                    should_reset_application,
   uint16_t                reset_mode);

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
 * The I&M0 data record is read-only by the controller.
 *
 * This data record is mandatory.
 */
typedef struct pnet_im_0
{
   uint8_t                 vendor_id_hi;
   uint8_t                 vendor_id_lo;
   char                    order_id[20+1];         /**< Terminated string */
   char                    im_serial_number[16+1]; /**< Terminated string */
   uint16_t                im_hardware_revision;
   char                    sw_revision_prefix;
   uint8_t                 im_sw_revision_functional_enhancement;
   uint8_t                 im_sw_revision_bug_fix;
   uint8_t                 im_sw_revision_internal_change;
   uint16_t                im_revision_counter;
   uint16_t                im_profile_id;
   uint16_t                im_profile_specific_type;
   uint8_t                 im_version_major;       /**< Always 1 */
   uint8_t                 im_version_minor;       /**< Always 1 */
   uint16_t                im_supported;           /**< One bit for each supported I&M1..15 (I&M0 always supported) */
} pnet_im_0_t;

/**
 * The I&M1 data record is read-write by the controller.
 *
 * This data record is optional. If this data record is supported
 * by the application then bit 0 in the im_supported member of I&M0 shall be set.
 */
typedef struct pnet_im_1
{
   char                    im_tag_function[32+1];  /**< Terminated string */
   char                    im_tag_location[22+1];  /**< Terminated string */
} pnet_im_1_t;

/**
 * The I&M2 data record is read-write by the controller.
 *
 * This data record is optional. If this data record is supported
 * by the application then bit 1 in the im_supported member of I&M0 shall be set.
 */
typedef struct pnet_im_2
{
   char                    im_date[16+1];          /**< "YYYY-MM-DD HH:MM" terminated string */
} pnet_im_2_t;

/**
 * The I&M3 data record is read-write by the controller.
 *
 * This data record is optional. If this data record is supported
 * by the application then bit 2 in the im_supported member of I&M0 shall be set.
 */
typedef struct pnet_im_3
{
   char                    im_descriptor[54+1];    /**< Terminated string padded with spaces */
} pnet_im_3_t;

/**
 * The I&M4 data record is read-write by the controller.
 *
 * This data record is optional. If this data record is supported
 * by the application then bit 3 in the im_supported member of I&M0 shall be set.
 */
typedef struct pnet_im_4
{
   char                    im_signature[54];       /**< Non-terminated binary string, padded with zeroes */
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
   uint8_t                 vendor_id_hi;
   uint8_t                 vendor_id_lo;
   uint8_t                 device_id_hi;
   uint8_t                 device_id_lo;
} pnet_cfg_device_id_t;

/**
 * Used for assigning a static IP address to the device.
 *
 * The Profinet stack also supports assigning an IP address, mask and gateway address
 * via DCP Set commands based on the Ethernet MAC address.
 */
typedef struct pnet_ip_addr_t
{
   uint8_t                 a;
   uint8_t                 b;
   uint8_t                 c;
   uint8_t                 d;
} pnet_cfg_ip_addr_t;

/**
 * The Ethernet MAC address.
 */
typedef struct pnet_ethaddr
{
  uint8_t addr[6];
} pnet_ethaddr_t;

/**
 * LLDP information used by the Profinet stack.
 */
typedef struct pnet_lldp_cfg
{
   char                    chassis_id[240 + 1];    /**< Terminated string */
   char                    port_id[240 + 1];       /**< Terminated string */
   pnet_ethaddr_t          port_addr;
   uint16_t                ttl;
   uint16_t                rtclass_2_status;
   uint16_t                rtclass_3_status;
   uint8_t                 cap_aneg;
   uint16_t                cap_phy;
   uint16_t                mau_type;
} pnet_lldp_cfg_t;

/**
 * This is all the configuration needed to use the Profinet stack.
 *
 * The application must supply the values in the call to function pnet_init().
 */
typedef struct pnet_cfg
{
   /** Application call-backs */
   pnet_state_ind          state_cb;
   pnet_connect_ind        connect_cb;
   pnet_release_ind        release_cb;
   pnet_dcontrol_ind       dcontrol_cb;
   pnet_ccontrol_cnf       ccontrol_cb;
   pnet_write_ind          write_cb;
   pnet_read_ind           read_cb;
   pnet_exp_module_ind     exp_module_cb;
   pnet_exp_submodule_ind  exp_submodule_cb;
   pnet_new_data_status_ind new_data_status_cb;
   pnet_alarm_ind          alarm_ind_cb;
   pnet_alarm_cnf          alarm_cnf_cb;
   pnet_alarm_ack_cnf      alarm_ack_cnf_cb;
   pnet_reset_ind          reset_cb;
   void                    *cb_arg;    /* Userdata passed to callbacks, not used by stack */

   /** I&M initial data */
   pnet_im_0_t             im_0_data;
   pnet_im_1_t             im_1_data;
   pnet_im_2_t             im_2_data;
   pnet_im_3_t             im_3_data;
   pnet_im_4_t             im_4_data;

   /** Identities */
   pnet_cfg_device_id_t    device_id;
   pnet_cfg_device_id_t    oem_device_id;
   char                    station_name[240+1];                   /**< Terminated string */
   char                    device_vendor[20+1];                   /**< Terminated string */
   char                    manufacturer_specific_string[240+1];   /**< Terminated string */

   /** LLDP */
   pnet_lldp_cfg_t         lldp_cfg;

   /** Capabilities */
   bool                    send_hello;             /**< Send DCP HELLO message on startup if true. */

   /** IP configuration */
   bool                    dhcp_enable;            /**< Not supported by stack. */
   pnet_cfg_ip_addr_t      ip_addr;
   pnet_cfg_ip_addr_t      ip_mask;
   pnet_cfg_ip_addr_t      ip_gateway;
   pnet_ethaddr_t          eth_addr;
} pnet_cfg_t;

/**
 * # Alarm and Diagnosis
 *
 */
typedef struct pnet_alarm_spec
{
   bool                    channel_diagnosis;
   bool                    manufacturer_diagnosis;
   bool                    submodule_diagnosis;
   bool                    ar_diagnosis;
} pnet_alarm_spec_t;

/*
* API function return values
*
* All functions return either (zero) 0 for a successful call or
* -1 for an unsuccessful call.
*/

/**
 * Initialize the ProfiNet stack.
 *
 * This function must be called to initialize the profinet stack.
 *
 * @param netif            In:   Name of the network interface.
 * @param tick_us          In:   Periodic interval in us. Specify the interval
 *                               between calls to pnet_handle_periodic().
 * @param p_cfg            In:   Profinet configuration.
 * @return a handle to the stack instance, or NULL if an error occured.
 */
PNET_EXPORT pnet_t* pnet_init(
   const char              *netif,
   uint32_t                tick_us,
   const pnet_cfg_t        *p_cfg);

/**
 * Execute all periodic functions within the ProfiNet stack.
 *
 * This function should be called periodically by the application.
 * The period is specified by the application in the tick_us argument
 * to pnet_init.
 * The period should match the expected I/O data rate to and from the device.
 * @param net              InOut: The p-net stack instance
 */
PNET_EXPORT void pnet_handle_periodic(
   pnet_t                  *net);

/**
 * Application signals ready to exchange data.
 *
 * Sends a ccontrol request to the IO-controller.
 *
 * This function must be called _after_ the application has received
 * state_cb(PNET_EVENT_PRMEND) in order for a connection to be established.
 *
 * If this function does not see all PPM data and IOPS areas are set (by the app)
 * then it returns error and the application must try again - otherwise the
 * startup will hang.
 *
 * Triggers the \a pnet_state_ind() user callback with PNET_EVENT_APPLRDY.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:   The AREP
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_application_ready(
   pnet_t                  *net,
   uint32_t                arep);

/**
 * Plug a module into a slot.
 *
 * This function is used to plug a specific module into a specific slot.
 *
 * This function may be called from the exp_module_cb call-back function
 * of the application.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:   The API identifier.
 * @param slot             In:   The slot number.
 * @param module_ident     In:   The module ident number.
 * @return  0  if a module could be plugged into the slot.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_plug_module(
   pnet_t                  *net,
   uint32_t                api,
   uint16_t                slot,
   uint32_t                module_ident);

/**
 * Plug a sub-module into a sub-slot.
 *
 * This function is used to plug a specific sub-module into a specific sub-slot.
 *
 * This function may be called from the exp_submodule_cb call-back function
 * of the application.
 *
 * If a module has not been plugged into the designated slot then it will be
 * plugged automatically by this function.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:   The API identifier.
 * @param slot             In:   The slot number.
 * @param subslot          In:   The sub-slot number.
 * @param module_ident     In:   The module ident number.
 * @param submodule_ident  In:   The sub-module ident number.
 * @param direction        In:   The direction of data.
 * @param length_input     In:   The size in bytes of the input data.
 * @param length_output    In:   The size in bytes of the output data.
 * @return  0  if the sub-module could be plugged into the sub-slot.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_plug_submodule(
   pnet_t                  *net,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint32_t                module_ident,
   uint32_t                submodule_ident,
   pnet_submodule_dir_t    direction,
   uint16_t                length_input,
   uint16_t                length_output);

/**
 * Pull a module from a slot.
 *
 * This function may be used to unplug a module from a specific slot.
 *
 * This function internally calls pnet_pull_submodule() on any plugged
 * sub-modules before pulling the module itself.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:   The API identifier.
 * @param slot             In:   The slot number.
 * @return  0  if a module could be pulled from the slot.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_pull_module(
   pnet_t                  *net,
   uint32_t                api,
   uint16_t                slot);

/**
 * Pull a sub-module from a sub-slot.
 *
 * This function may be used to unplug a sub-module from a specific sub-slot.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:   The API identifier.
 * @param slot             In:   The slot number.
 * @param subslot          In:   The sub-slot number.
 * @return  0  if the sub-module could be pulled from the sub-slot.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_pull_submodule(
   pnet_t                  *net,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot);

/**
 * Updates the IOPS and data of one sub-slot to send to the controller.
 *
 * This function may be called to set new data and IOPS values of a sub-slot to
 * send to the controller.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:  The API.
 * @param slot             In:  The slot.
 * @param subslot          In:  The sub-slot.
 * @param p_data           In:  Data buffer.
 * @param data_len         In:  Bytes in data buffer.
 * @param iops             In:  The device provider status.
 * @return  0  if a sub-module data and IOPS was set.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_input_set_data_and_iops(
   pnet_t                  *net,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint8_t                 *p_data,
   uint16_t                data_len,
   uint8_t                 iops);

/**
 * Fetch the controller consumer status of one sub-slot.
 *
 * This function may be called to retrieve the IOCS value of a sub-slot
 * sent from the controller.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:  The API.
 * @param slot             In:  The slot.
 * @param subslot          In:  The sub-slot.
 * @param p_iocs           Out: The controller consumer status.
 * @return  0  if a sub-module IOCS was set.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_input_get_iocs(
   pnet_t                  *net,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint8_t                 *p_iocs);

/**
 * Retrieve latest sub-slot data and IOPS received from the controller.
 *
 * This function may be called to retrieve the latest data and IOPS values
 * of a sub-slot sent from the controller.
 *
 * If new data and IOCS has arrived from the controller since the last call
 * then the flag \a p_new_flag is set to true, else it is set to false.
 *
 * Note that the latest data and IOPS values are copied to the application
 * buffers regardless of the value of \a p_new_flag.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:  The API.
 * @param slot             In:  The slot.
 * @param subslot          In:  The sub-slot.
 * @param p_new_flag       Out: true if new data.
 * @param p_data           Out: The received data.
 * @param p_data_len       In:  Size of receive buffer.
 *                         Out: Received number of data bytes.
 * @param p_iops           Out: The controller provider status (IOPS).
 * @return  0  if a sub-module data and IOPS is retrieved.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_output_get_data_and_iops(
   pnet_t                  *net,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   bool                    *p_new_flag,
   uint8_t                 *p_data,
   uint16_t                *p_data_len,
   uint8_t                 *p_iops);

/**
 * Set the device consumer status for one sub-slot.
 *
 * This function is use to set the device consumer status (IOCS)
 * of a specific sub-slot.
 *
 * @param net              InOut: The p-net stack instance
 * @param api              In:  The API.
 * @param slot             In:  The slot.
 * @param subslot          In:  The sub-slot.
 * @param iocs             In:  The device consumer status.
 * @return  0  if a sub-module IOCS was set.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_output_set_iocs(
   pnet_t                  *net,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint8_t                 iocs);

/**
 * Implements the "Local Set State" primitive.
 *
 * @param net              InOut: The p-net stack instance
 * @param primary          In:   true to set state to "primary".
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_set_state(
   pnet_t                  *net,
   bool                    primary);

/**
 * Implements the "Local Set Redundancy State" primitive.
 *
 * @param net              InOut: The p-net stack instance
 * @param redundant        In:   true to set state to "redundant".
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_set_redundancy_state(
   pnet_t                  *net,
   bool                    redundant);

/**
 * Implements the "Local set Provider State" primitive.
 *
 * The application should call this function at least once during startup to set
 * the provider status of frames sent to the controller.
 *
 * The application may call this function at any time, e.g., to signal a temporary
 * inability to produce new application data to the controller.
 *
 * Its effect is similar to setting IOPS to PNET_IOXS_BAD for all sub-slots.
 *
 * @param net              InOut: The p-net stack instance
 * @param run              In:   true to set state to "run".
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_set_provider_state(
   pnet_t                  *net,
   bool                    run);

/**
 * Application requests abortion of the connection.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:   The AREP
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_ar_abort(
   pnet_t                  *net,
   uint32_t                arep);

/**
 * Fetch error information from the AREP.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:   The AREP.
 * @param p_err_cls        Out:  The error class.
 * @param p_err_code       Out:  The Error code.
 * @return  0  If the AREP is valid.
 *          -1 if the AREP is not valid.
 */
PNET_EXPORT int pnet_get_ar_error_codes(
   pnet_t                  *net,
   uint32_t                arep,
   uint16_t                *p_err_cls,
   uint16_t                *p_err_code);

/**
 * Application creates an entry in the log book.
 *
 * This function is used to insert an entry into the Profinet log-book.
 * Controllers may read the log-book using IODRead requests.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:   The AREP.
 * @param p_pnio_status    In:   The PNIO status.
 * @param entry_detail     In:   Additional application information.
 */
PNET_EXPORT void pnet_create_log_book_entry(
   pnet_t                  *net,
   uint32_t                 arep,
   const pnet_pnio_status_t *p_pnio_status,
   uint32_t                 entry_detail);

/**
 * Application issues a process alarm.
 *
 * This function may be used by the application to issue a process alarm.
 * Such alarms are sent to the controller using high-priority alarm messages.
 * Optional payload may be attached to the alarm. If payload_usi is != 0 then
 * the payload_usi and the payload at p_payload is attached to the alarm.
 * After calling this function the application must wait for the alarm_cnf_cb
 * before sending another alarm.
 * This function fails if the application does not wait for the alarm_cnf_cb
 * between sending two alarms.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:   The AREP.
 * @param api              In:   The API.
 * @param slot             In:   The slot.
 * @param subslot          In:   The sub-slot.
 * @param payload_usi      In:   The USI for the payload.
 * @param payload_len      In:   Length in bytes of the payload.
 * @param p_payload        In:   The alarm payload (USI specific format).
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred (or waiting for ACK from controller: re-try later).
 */
PNET_EXPORT int pnet_alarm_send_process_alarm(
   pnet_t                  *net,
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                payload_usi,
   uint16_t                payload_len,
   uint8_t                 *p_payload);

/**
 * Application acknowledges the reception of an alarm from the controller.
 *
 * This function sends an ACk to the controller.
 * This function must be called by the application after receiving an alarm
 * in the pnet_alarm_ind call-back. Failure to do so within the timeout
 * specified in the connect of the controller will make the controller
 * re-send the alarm.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:   The AREP.
 * @param p_pnio_status    In:   Detailed ACK status.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_alarm_send_ack(
   pnet_t                  *net,
   uint32_t                arep,
   pnet_pnio_status_t      *p_pnio_status);

/* ****************************** Diagnosis ****************************** */
/* Mask and position of bit fields and values within ch_properties         */

#define PNET_DIAG_CH_PROP_TYPE_MASK          0x00ff
#define PNET_DIAG_CH_PROP_TYPE_POS           0
#define PNET_DIAG_CH_PROP_TYPE_GET(x)        (((X) & PNET_DIAG_CH_PROP_TYPE_MASK)>>PNET_DIAG_CH_PROP_TYPE_POS)
#define PNET_DIAG_CH_PROP_TYPE_SET(x, v)     do { (x) &= ~PNET_DIAG_CH_PROP_TYPE_MASK; (x) |= (v)<<PNET_DIAG_CH_PROP_TYPE_POS; } while (0)
typedef enum pnet_diag_ch_prop_type_values
{
   PNET_DIAG_CH_PROP_TYPE_UNSPECIFIED        = 0,
   PNET_DIAG_CH_PROP_TYPE_1_BIT              = 1,
   PNET_DIAG_CH_PROP_TYPE_2_BIT              = 2,
   PNET_DIAG_CH_PROP_TYPE_4_BIT              = 3,
   PNET_DIAG_CH_PROP_TYPE_8_BIT              = 4,
   PNET_DIAG_CH_PROP_TYPE_16_BIT             = 5,
   PNET_DIAG_CH_PROP_TYPE_32_BIT             = 6,
   PNET_DIAG_CH_PROP_TYPE_64_BIT             = 7,
   /* 8..255 Reserved */
} pnet_diag_ch_prop_type_values_t;

#define PNET_DIAG_CH_PROP_ACC_MASK           0x0100
#define PNET_DIAG_CH_PROP_ACC_POS            8
#define PNET_DIAG_CH_PROP_ACC_GET(x)         (((x) & PNET_DIAG_CH_PROP_ACC_MASK)>>PNET_DIAG_CH_PROP_ACC_POS)
#define PNET_DIAG_CH_PROP_ACC_SET(x, v)      do { (x) &= ~PNET_DIAG_CH_PROP_ACC_MASK; (x) |= (v)<<PNET_DIAG_CH_PROP_ACC_POS; } while (0)

#define PNET_DIAG_CH_PROP_MAINT_MASK         0x0600
#define PNET_DIAG_CH_PROP_MAINT_POS          9
#define PNET_DIAG_CH_PROP_MAINT_GET(x)       (((x) & PNET_DIAG_CH_PROP_MAINT_MASK)>>PNET_DIAG_CH_PROP_MAINT_POS)
#define PNET_DIAG_CH_PROP_MAINT_SET(x, v)    do { (x) &= ~PNET_DIAG_CH_PROP_MAINT_MASK; (x) |= (v)<<PNET_DIAG_CH_PROP_MAINT_POS; } while (0)
typedef enum pnet_diag_ch_prop_maint_values
{
   PNET_DIAG_CH_PROP_MAINT_FAULT             = 0,
   PNET_DIAG_CH_PROP_MAINT_REQUIRED          = 1,
   PNET_DIAG_CH_PROP_MAINT_DEMANDED          = 2,
   PNET_DIAG_CH_PROP_MAINT_QUALIFIED         = 3,
} pnet_diag_ch_prop_maint_values_t;

#define PNET_DIAG_CH_PROP_SPEC_MASK          0x1800
#define PNET_DIAG_CH_PROP_SPEC_POS           11
#define PNET_DIAG_CH_PROP_SPEC_GET(x)        (((x) & PNET_DIAG_CH_PROP_SPEC_MASK)>>PNET_DIAG_CH_PROP_SPEC_POS)
#define PNET_DIAG_CH_PROP_SPEC_SET(x, v)     do { (x) &= ~PNET_DIAG_CH_PROP_SPEC_MASK; (x) |= (v)<<PNET_DIAG_CH_PROP_SPEC_POS; } while (0)
typedef enum pnet_diag_ch_prop_spec_values
{
   PNET_DIAG_CH_PROP_SPEC_ALL_DISAPPEARS     = 0,
   PNET_DIAG_CH_PROP_SPEC_APPEARS            = 1,
   PNET_DIAG_CH_PROP_SPEC_DISAPPEARS         = 2,
   PNET_DIAG_CH_PROP_SPEC_DIS_OTHERS_REMAIN  = 3,
} pnet_diag_ch_prop_spec_values_t;

#define PNET_DIAG_CH_PROP_DIR_MASK           0xe000
#define PNET_DIAG_CH_PROP_DIR_POS            13
#define PNET_DIAG_CH_PROP_DIR_GET(x)         (((x) & PNET_DIAG_CH_PROP_DIR_MASK)>>PNET_DIAG_CH_PROP_DIR_POS)
#define PNET_DIAG_CH_PROP_DIR_SET(x, v)      do { (x) &= ~PNET_DIAG_CH_PROP_DIR_MASK; (x) |= (v)<<PNET_DIAG_CH_PROP_DIR_POS; } while (0)
typedef enum pnet_diag_ch_prop_dir_values
{
   PNET_DIAG_CH_PROP_DIR_MANUF_SPEC          = 0,
   PNET_DIAG_CH_PROP_DIR_INPUT               = 1,
   PNET_DIAG_CH_PROP_DIR_OUTPUT              = 2,
   PNET_DIAG_CH_PROP_DIR_INOUT               = 3,
} pnet_diag_ch_prop_dir_values_t;

#define PNET_DIAG_QUALIFIER_POS_FAULT        27
#define PNET_DIAG_QUALIFIER_POS_DEMANDED     17
#define PNET_DIAG_QUALIFIER_POS_REQUIRED     7
#define PNET_DIAG_QUALIFIER_POS_ADVICE       3

#define PNET_DIAG_USI_STD                    0x8000

/**
 * Add a diagnosis entry.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:   The AREP.
 * @param api              In:   The API.
 * @param slot             In:   The slot.
 * @param subslot          In:   The sub-slot.
 * @param ch               In:   The channel number
 * @param ch_properties    In:   The channel properties.
 * @param ch_error_type    In:   The channel error type.
 * @param ext_ch_error_type In:  The extended channel error type.
 * @param ext_ch_add_value In:   The extended channel error additional value.
 * @param qual_ch_qualifier In:  The qualified channel qualifier.
 * @param usi              In:   The USI.
 *                               range: 0..0x7fff for manufacturer-specific diag format.
 *                                      0x8000 for standard format.
 * @param p_manuf_data     In:   The manufacturer specific diagnosis data.
 *                               (Only needed if USI <= 0x7fff).
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_diag_add(
   pnet_t                  *net,
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                ch,
   uint16_t                ch_properties,
   uint16_t                ch_error_type,
   uint16_t                ext_ch_error_type,
   uint32_t                ext_ch_add_value,
   uint32_t                qual_ch_qualifier,
   uint16_t                usi,
   uint8_t                 *p_manuf_data);

/**
 * Update a diagnosis entry.
 *
 * If the USI of the item is in the manufacturer-specified range then
 * the USI is used to identify the item.
 * Otherwise the other parameters are used (all must match):
 * - API id.
 * - Slot number.
 * - Sub-slot number.
 * - Channel number.
 * - Channel properties (the channel direction part only).
 * - Channel error type.
 *
 * When the item is found either the manufacturer data is updated (for
 * USI in manufacturer-specific range) or the extended channel additional
 * value is updated.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:   The AREP.
 * @param api              In:   The API.
 * @param slot             In:   The slot.
 * @param subslot          In:   The sub-slot.
 * @param ch               In:   The channel number
 * @param ch_properties    In:   The channel properties.
 * @param ch_error_type    In:   The channel error type.
 * @param ext_ch_add_value In:   The extended channel error additional value.
 * @param usi              In:   The USI.
 *                               range: 0..0x7fff for manufacturer-specific diag format.
 *                                      0x8000 for standard format.
 * @param p_manuf_data     In:   The manufacturer specific diagnosis data.
 *                               (Only needed if USI <= 0x7fff).
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_diag_update(
   pnet_t                  *net,
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                ch,
   uint16_t                ch_properties,
   uint16_t                ch_error_type,
   uint32_t                ext_ch_add_value,
   uint16_t                usi,
   uint8_t                 *p_manuf_data);

/**
 * Remove a diagnosis entry.
 *
 * If the USI of the item is in the manufacturer-specified range then
 * the USI is used to identify the item.
 * Otherwise the other parameters are used (all must match):
 * - API id.
 * - Slot number.
 * - Sub-slot number.
 * - Channel number.
 * - Channel properties (the channel direction part only).
 * - Channel error type.
 *
 * @param net              InOut: The p-net stack instance
 * @param arep             In:   The AREP.
 * @param api              In:   The API.
 * @param slot             In:   The slot.
 * @param subslot          In:   The sub-slot.
 * @param ch               In:   The channel number
 * @param ch_properties    In:   The channel properties.
 * @param ch_error_type    In:   The channel error type.
 * @param usi              In:   The USI.
 *                               range: 0..0x7fff for manufacturer-specific diag format.
 *                                      0x8000 for standard format.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
PNET_EXPORT int pnet_diag_remove(
   pnet_t                  *net,
   uint32_t                arep,
   uint32_t                api,
   uint16_t                slot,
   uint16_t                subslot,
   uint16_t                ch,
   uint16_t                ch_properties,
   uint16_t                ch_error_type,
   uint16_t                usi);

/**
 * Show information from the Profinet stack.
 *
 * @param net              InOut: The p-net stack instance
 * @param level            In:   The amount of detail to show.
 *     0x0800              | Show all sessions.
 *     0x1000              | Show all ARs.
 *     0x1001              |           include IOCR.
 *     0x1002              |           include data_descriptors.
 *     0x1003              |           include IOCR and data_descriptors.
 *     0x2000              | Show CFG information.
 *     0x4000              | Show scheduler information.
 *     0x8000              | Show I&M data.
 */
PNET_EXPORT void pnet_show(
   pnet_t                  *net,
   unsigned                level);

#ifdef __cplusplus
}
#endif

#endif /* PNET_API_H */
