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

#include "app_log.h"

#include <stdarg.h>
#include <stdio.h>

static int32_t log_level = APP_DEFAULT_LOG_LEVEL;

void app_log_set_log_level (int32_t level)
{
   log_level = level;
}

void app_log (int32_t level, const char * fmt, ...)
{
   va_list list;

   if (level >= log_level)
   {
      va_start (list, fmt);
      vprintf (fmt, list);
      va_end (list);
      fflush (stdout);
   }
}

void app_log_print_bytes (int32_t level, const uint8_t * bytes, uint32_t len)
{
   if (level >= log_level)
   {
      printf ("  Bytes: ");
      for (uint32_t i = 0; i < len; i++)
      {
         printf ("%02X ", bytes[i]);
      }
      printf ("\n");
   }
}
