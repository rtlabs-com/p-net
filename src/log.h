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
 * @brief Logging utilities
 */

#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "options.h"
#include "osal.h"

#include <inttypes.h>

/* Log levels */
#define LOG_LEVEL_DEBUG   0x00
#define LOG_LEVEL_INFO    0x01
#define LOG_LEVEL_WARNING 0x02
#define LOG_LEVEL_ERROR   0x03
#define LOG_LEVEL_MASK    0x03
#define LOG_LEVEL_GET(t)  (t & LOG_LEVEL_MASK)

/* Log states */
#define LOG_STATE_ON      0x80
#define LOG_STATE_OFF     0x00

/** Log a message if it is enabled */
#define LOG(type, ...)                          \
do                                              \
{                                               \
   if ((LOG_LEVEL_GET (type) >= LOG_LEVEL) &&   \
       (type & LOG_STATE_ON))                   \
      {                                         \
         os_log (type, __VA_ARGS__);            \
      }                                         \
} while(0)

/** Log debug messages */
#define LOG_DEBUG(type, ...)                    \
   LOG ((LOG_LEVEL_DEBUG | type), __VA_ARGS__)

/** Log informational messages */
#define LOG_INFO(type, ...) \
   LOG ((LOG_LEVEL_INFO | type), __VA_ARGS__)

/** Log warning messages */
#define LOG_WARNING(type, ...) \
   LOG ((LOG_LEVEL_WARNING | type), __VA_ARGS__)

/** Log error messages */
#define LOG_ERROR(type, ...) \
   LOG ((LOG_LEVEL_ERROR | type), __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* LOG_H */
