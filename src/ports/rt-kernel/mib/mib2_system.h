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

/**
 * @file
 * @brief Functions for reading and writing SNMP MIB-II system variables
 *
 * These functions should be used as callback functions to be called from the
 * lwip stack when SNMP Get/GetNext or Set requests are received for variables
 * in the system group (OID 1.3.6.1.2.1.1). The appropriate functions in
 * the p-net stack will be called as needed. For configuringlwip to use these,
 * functions, call snmp_mib2_system_set_callbacks().
 *
 * Note that it is assumed that a patched version of lwip is used.
 * The original version of lwip does not provide callback functions to read
 * variables in SNMP Get/GetNext requests.
 */

#ifndef MIB2_SYSTEM_H
#define MIB2_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lwip/apps/snmp.h>

/**
 * Get value of system variable
 *
 * This function should be called when an SNMP Get or GetNext request
 * is received.
 *
 * @param column           In:    Column index for system variable.
 * @param value            Out:   Buffer where value will be stored.
 * @param max_size         In:    Size of buffer in bytes.
 * @return Size of retrieved value if successful,
 *         SNMP_ERR_GENERROR if an error occurred.
 */
s16_t mib2_system_get_value (u32_t column, void * value, size_t max_size);

/**
 * Test if value could be set for system variable
 *
 * This function should be called when an SNMP Set request is received.
 *
 * @param column           In:    Column index for system variable.
 * @param value            In:    Buffer containing the new value.
 *                                Is not null-terminated.
 * @param size             In:    Size of \a value in bytes.
 * @return SNMP_ERR_NOERROR if successful,
 *         SNMP_ERR_WRONGVALUE if \a len is too large.
 */
snmp_err_t mib2_system_test_set_value (
   u32_t column,
   const void * value,
   size_t size);

/**
 * Set new value for system variable
 *
 * This function should be called when an SNMP Set request is received
 * after first calling mib2_system_test_set_value() to verify
 * that \a size is not too large.
 *
 * @param column           In:    Column index for system variable.
 * @param value            In:    Buffer containing the new value.
 *                                Is not null-terminated.
 * @param size             In:    Size of \a value in bytes.
 * @return SNMP_ERR_NOERROR if successful,
 *         SNMP_ERR_GENERROR on error.
 */
snmp_err_t mib2_system_set_value (u32_t column, const void * value, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* MIB2_SYSTEM_H */
