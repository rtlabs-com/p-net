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
#ifndef MAIN_LOG_H
#define MAIN_LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>

#define MAIN_LOG_LEVEL          MAIN_LOG_LEVEL_ERROR

/* Log levels */
#define MAIN_LOG_LEVEL_DEBUG    0x00
#define MAIN_LOG_LEVEL_INFO     0x01
#define MAIN_LOG_LEVEL_WARNING  0x02
#define MAIN_LOG_LEVEL_ERROR    0x03
#define MAIN_LOG_LEVEL_MASK     0x03
#define MAIN_LOG_LEVEL_GET(t)   (t & MAIN_LOG_LEVEL_MASK)

/* Log states */
#define MAIN_LOG_STATE_ON       0x80
#define MAIN_LOG_STATE_OFF      0x00

#define PNET_MAIN_LOG      MAIN_LOG_STATE_ON

/** Log a message if it is enabled */
#define MAIN_LOG(type, ...)                          \
do                                              \
{                                               \
   if ((MAIN_LOG_LEVEL_GET (type) >= MAIN_LOG_LEVEL) &&   \
       (type & MAIN_LOG_STATE_ON))                   \
      {                                         \
         os_log (type, __VA_ARGS__);            \
      }                                         \
} while(0)

/** Log debug messages */
#define MAIN_LOG_DEBUG(type, ...)                    \
   MAIN_LOG ((MAIN_LOG_LEVEL_DEBUG | type), __VA_ARGS__)

/** Log informational messages */
#define MAIN_LOG_INFO(type, ...) \
   MAIN_LOG ((MAIN_LOG_LEVEL_INFO | type), __VA_ARGS__)

/** Log warning messages */
#define MAIN_LOG_WARNING(type, ...) \
   MAIN_LOG ((MAIN_LOG_LEVEL_WARNING | type), __VA_ARGS__)

/** Log error messages */
#define MAIN_LOG_ERROR(type, ...) \
   MAIN_LOG ((MAIN_LOG_LEVEL_ERROR | type), __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* MAIN_LOG_H */
