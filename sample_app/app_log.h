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

#ifndef APP_LOG_H
#define APP_LOG_H

/**
 * @file
 * @brief Application debug log utility
 *
 * Runtime configurable debug log using printf()
 * Levels matches levels used in P-Net.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define APP_LOG_LEVEL_DEBUG   0x00
#define APP_LOG_LEVEL_INFO    0x01
#define APP_LOG_LEVEL_WARNING 0x02
#define APP_LOG_LEVEL_ERROR   0x03
#define APP_LOG_LEVEL_FATAL   0x04

#define APP_DEFAULT_LOG_LEVEL APP_LOG_LEVEL_FATAL

#define APP_LOG(level, ...) app_log (level, __VA_ARGS__)

#define APP_LOG_DEBUG(...)   APP_LOG (APP_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define APP_LOG_INFO(...)    APP_LOG (APP_LOG_LEVEL_INFO, __VA_ARGS__)
#define APP_LOG_WARNING(...) APP_LOG (APP_LOG_LEVEL_WARNING, __VA_ARGS__)
#define APP_LOG_ERROR(...)   APP_LOG (APP_LOG_LEVEL_ERROR, __VA_ARGS__)
#define APP_LOG_FATAL(...)   APP_LOG (APP_LOG_LEVEL_FATAL, __VA_ARGS__)

/**
 * Print log message depending on level
 * Use the APP_LOG_xxxxx macros instead of this function.
 * @param level         In: Message log level
 * @param fmt           In: Log message format string
 */
void app_log (int32_t level, const char * fmt, ...);

/**
 * Log an array of bytes
 * @param level         In: Log level
 * @param bytes         In: Array of bytes
 * @param length        In: Length of array
 */
void app_log_print_bytes (int32_t level, const uint8_t * bytes, uint32_t length);

/**
 * Set log level
 * @param level         In: Log level
 */
void app_log_set_log_level (int32_t level);

#ifdef __cplusplus
}
#endif

#endif /* APP_LOG_H */
