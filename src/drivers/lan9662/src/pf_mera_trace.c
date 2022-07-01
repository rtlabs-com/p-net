// Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
// SPDX-License-Identifier: MIT

/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 *
 ********************************************************************/

/**
 * Mera lib log callbacks adapted for P-Net.
 * Based on code in mera demo.
 */

#include "pf_includes.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include <string.h>
#include <inttypes.h>
#include "microchip/ethernet/rte/api.h"

#define TRACE_GROUP_DEFAULT 0
#define TRACE_GROUP_CNT     1

typedef struct
{
   const char * name;
   mera_trace_level_t level;
} trace_group_t;

static trace_group_t trace_groups[] = {
   {.name = "default", .level = MERA_TRACE_LEVEL_NOISE},
   {.name = "ib", .level = MERA_TRACE_LEVEL_NOISE},
   {.name = "ob", .level = MERA_TRACE_LEVEL_NOISE},
};

static void printf_trace_head (
   const char * mname,
   const char * gname,
   const mera_trace_level_t level,
   const char * file,
   const int line,
   const char * function,
   const char * lcont)
{
   struct timeval tv;
   int h, m, s;
   const char *p, *base_name = file;

   for (p = file; *p != 0; p++)
   {
      if (*p == '/' || *p == '\\')
      {
         base_name = (p + 1);
      }
   }

   (void)gettimeofday (&tv, NULL);
   h = (tv.tv_sec / 3600 % 24);
   m = (tv.tv_sec / 60 % 60);
   s = (tv.tv_sec % 60);
   printf (
      "%u:%02u:%02u:%05lu %s/%s/%s %s(%u) %s%s",
      h,
      m,
      s,
      tv.tv_usec,
      mname,
      gname,
      level == MERA_TRACE_LEVEL_ERROR
         ? "error"
         : level == MERA_TRACE_LEVEL_INFO
              ? "info"
              : level == MERA_TRACE_LEVEL_DEBUG
                   ? "debug"
                   : level == MERA_TRACE_LEVEL_NOISE ? "noise" : "?",
      base_name,
      line,
      function,
      lcont);
}

static void mera_printf_trace_head (
   const mera_trace_group_t group,
   const mera_trace_level_t level,
   const char * file,
   const int line,
   const char * function,
   const char * lcont)
{
   printf_trace_head (
      "p-net",
      (int)group < (int)NELEMENTS (trace_groups) ? trace_groups[group].name
                                                 : "?",
      level,
      file,
      line,
      function,
      lcont);
}

void pf_mera_callout_trace_printf (
   const mera_trace_group_t group,
   const mera_trace_level_t level,
   const char * file,
   const int line,
   const char * function,
   const char * format,
   ...)
{
   va_list args;

   mera_printf_trace_head (group, level, file, line, function, ": ");
   va_start (args, format);
   vprintf (format, args);
   va_end (args);
   printf ("\n");
   fflush (stdout);
}
