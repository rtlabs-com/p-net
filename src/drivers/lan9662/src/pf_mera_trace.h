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

#ifndef PF_MERA_TRACE_H
#define PF_MERA_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "microchip/ethernet/rte/api.h"

void pf_mera_callout_trace_printf (
   const mera_trace_group_t group,
   const mera_trace_level_t level,
   const char * file,
   const int line,
   const char * function,
   const char * format,
   ...);

#ifdef __cplusplus
}
#endif

#endif /* PF_MERA_TRACE_H */
