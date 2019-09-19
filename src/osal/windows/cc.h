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

#ifndef CC_H
#define CC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <assert.h>

#define CC_PACKED_BEGIN       __pragma(pack(push, 1))
#define CC_PACKED_END         __pragma(pack(pop))
#define CC_PACKED

#define CC_FORMAT(str,arg)

#define CC_TO_LE16(x)         (x)
#define CC_TO_LE32(x)         (x)
#define CC_TO_LE64(x)         (x)
#define CC_FROM_LE16(x)       (x)
#define CC_FROM_LE32(x)       (x)
#define CC_FROM_LE64(x)       (x)

/* TODO */
#define CC_ATOMIC_GET8(p)     (*p)
#define CC_ATOMIC_GET16(p)    (*p)
#define CC_ATOMIC_GET32(p)    (*p)
#define CC_ATOMIC_GET64(p)    (*p)

/* TODO */
#define CC_ATOMIC_SET8(p, v)  ((*p) = (v))
#define CC_ATOMIC_SET16(p, v) ((*p) = (v))
#define CC_ATOMIC_SET32(p, v) ((*p) = (v))
#define CC_ATOMIC_SET64(p, v) ((*p) = (v))

static uint8_t __inline cc_ctz (uint32_t x)
{
   DWORD n = 0;
   _BitScanForward(&n, x);
   return (uint8_t)n;
}

#define __builtin_ctz(x) cc_ctz (x)

#define CC_ASSERT(exp) assert (exp)
#define CC_STATIC_ASSERT(exp) static_assert ((exp),"")

#ifdef __cplusplus
}
#endif

#endif /* CC_H */
