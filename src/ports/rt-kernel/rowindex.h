/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2020 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

/**
 * @file
 * @brief SNMP table row index utilities
 *
 * Contains functions to:
 * - Check if a row index matches existing row index in table.
 * - Update a row index with next existing row index in table.
 */

#ifndef ROWINDEX_H
#define ROWINDEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lwip/apps/snmp.h>

/**
 * Match content of table row index with a local port.
 *
 * @param row_oid          In:    Table row index (array).
 * @param row_oid_len      In:    Number of elements in array.
 * @return Local port number if found,
 *         0 otherwise.
 */
int rowindex_match_with_local_port (const u32_t * row_oid, u8_t row_oid_len);

/**
 * Update content of table row index with next matching local port.
 *
 * @param row_oid          InOut: Table row index.
 * @return Local port number if found,
 *         0 otherwise.
 */
int rowindex_update_with_next_local_port (struct snmp_obj_id * row_oid);

/**
 * Match content of table row index with a local interface.
 *
 * Note that we only support a single local interface.
 *
 * @param row_oid          In:    Table row index (array).
 * @param row_oid_len      In:    Number of elements in array.
 * @return 1 if found,
 *         0 otherwise.
 */
int rowindex_match_with_local_interface (
   const u32_t * row_oid,
   u8_t row_oid_len);

/**
 * Update content of table row index with next matching local interface.
 *
 * Note that we only support a single local interface.
 *
 * @param row_oid          InOut: Table row index.
 * @return 1 if found,
 *         0 otherwise.
 */
snmp_err_t rowindex_update_with_next_local_interface (
   struct snmp_obj_id * row_oid);

/**
 * Match content of table row index with a remote device.
 *
 * @param row_oid          In:    Table row index (array).
 * @param row_oid_len      In:    Number of elements in array.
 * @return Port number for local port connected to the remote device if found,
 *         0 otherwise.
 */
int rowindex_match_with_remote_device (const u32_t * row_oid, u8_t row_oid_len);

/**
 * Update content of table row index with next matching remote device.
 *
 * @param row_oid          InOut: Table row index.
 * @return Port number for local port connected to the remote device if found,
 *         0 otherwise.
 */
int rowindex_update_with_next_remote_device (struct snmp_obj_id * row_oid);

/**
 * Match content of table row index with a remote interface.
 *
 * @param row_oid          In:    Table row index (array).
 * @param row_oid_len      In:    Number of elements in array.
 * @return Port number for local port connected to the remote interface
 *         if found, 0 otherwise.
 */
int rowindex_match_with_remote_interface (
   const u32_t * row_oid,
   u8_t row_oid_len);

/**
 * Update content of table row index with next matching remote interface.
 *
 * @param row_oid          InOut: Table row index.
 * @return Port number for local port connected to the remote interface
 *         if found, 0 otherwise.
 */
int rowindex_update_with_next_remote_interface (struct snmp_obj_id * row_oid);

#ifdef __cplusplus
}
#endif

#endif /* ROWINDEX_H */
