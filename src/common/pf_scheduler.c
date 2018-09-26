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

#ifdef UNIT_TEST

#endif

#include <string.h>
#include "pf_includes.h"

/**
 * The scheduler is used by both the CPM and PPM machines.
 * The DCP uses the scheduler for responding to multi-cast messages.
 * pf_cmsm uses it to supervise the startup sequence.
 */
#define PF_MAX_TIMEOUTS          (2 * (PNET_MAX_AR) * (PNET_MAX_CR) + 10)

typedef struct pf_scheduler_timeouts
{
   const char                    *p_name; /* For debugging only */
   bool                          in_use;  /* For debugging only */

   uint32_t                      when;    /* absolute time of timeout */
   uint32_t                      next;    /* Next in list */
   uint32_t                      prev;    /* Previous in list */

   pf_scheduler_timeout_ftn_t    cb;      /* Call-back to call on timeout */
   void                          *arg;    /* call-back argument */
} pf_scheduler_timeouts_t;

static volatile pf_scheduler_timeouts_t   pf_timeouts[PF_MAX_TIMEOUTS];
static volatile uint32_t                  pf_timeout_first = PF_MAX_TIMEOUTS; /* Nothing in q */
static volatile uint32_t                  pf_timeout_free = PF_MAX_TIMEOUTS;
static os_mutex_t                         *pf_timeout_mutex = NULL;
static uint32_t                           pf_tick_interval = 1;   /* Cannot be zero */

static bool pf_scheduler_is_linked(
   uint32_t                first,
   uint32_t                ix)
{
   bool                    ret = false;
   uint32_t                cnt = 0;

   if (ix < PF_MAX_TIMEOUTS)
   {
      while ((first < PF_MAX_TIMEOUTS) && (cnt < 20))
      {
         if (first == ix)
         {
            ret = true;
         }
         first = pf_timeouts[first].next;
         cnt++;
      }
   }

   return ret;
}

static void pf_scheduler_unlink(
   volatile uint32_t       *p_q,
   uint32_t                ix)
{
   /* Unlink from busy list */
   uint32_t                prev_ix;
   uint32_t                next_ix;

   if (ix >= PF_MAX_TIMEOUTS)
   {
      LOG_ERROR(PNET_LOG, "Sched(%d): ix (%u) is invalid\n", __LINE__, (unsigned)ix);
   }
   else if (pf_scheduler_is_linked(*p_q, ix) == false)
   {
      LOG_ERROR(PNET_LOG, "Sched(%d): %s is not in Q\n", __LINE__, pf_timeouts[ix].p_name);
   }
   else
   {
      prev_ix = pf_timeouts[ix].prev;
      next_ix = pf_timeouts[ix].next;
      if (*p_q == ix)
      {
         *p_q = next_ix;
      }
      if (next_ix < PF_MAX_TIMEOUTS)
      {
         pf_timeouts[next_ix].prev = prev_ix;
      }
      if (prev_ix < PF_MAX_TIMEOUTS)
      {
         pf_timeouts[prev_ix].next = next_ix;
      }
   }
}

static void pf_scheduler_link_after(
   volatile uint32_t       *p_q,
   uint32_t                ix,
   uint32_t                pos)
{
   uint32_t                next_ix;

   if (ix >= PF_MAX_TIMEOUTS)
   {
      LOG_ERROR(PNET_LOG, "Sched(%d): ix (%u) is invalid\n", __LINE__, (unsigned)ix);
   }
   else if (pf_scheduler_is_linked(*p_q, ix) == true)
   {
      LOG_ERROR(PNET_LOG, "Sched(%d): %s is already in Q\n", __LINE__, pf_timeouts[ix].p_name);
   }
   else if (pos >= PF_MAX_TIMEOUTS)
   {
      /* Put first in possible non-empty Q */
      pf_timeouts[ix].prev = PF_MAX_TIMEOUTS;
      pf_timeouts[ix].next = *p_q;
      if (*p_q < PF_MAX_TIMEOUTS)
      {
         pf_timeouts[*p_q].prev = ix;
      }

      *p_q = ix;
   }
   else if (*p_q >= PF_MAX_TIMEOUTS)
   {
      /* Q is empty - insert first in Q */
      pf_timeouts[ix].prev = PF_MAX_TIMEOUTS;
      pf_timeouts[ix].next = PF_MAX_TIMEOUTS;

      *p_q = ix;
   }
   else
   {
      next_ix = pf_timeouts[pos].next;

      if (next_ix < PF_MAX_TIMEOUTS)
      {
         pf_timeouts[next_ix].prev = ix;
      }
      pf_timeouts[pos].next = ix;

      pf_timeouts[ix].prev = pos;
      pf_timeouts[ix].next = next_ix;
   }
}

static void pf_scheduler_link_before(
   volatile uint32_t       *p_q,
   uint32_t                ix,
   uint32_t                pos)
{
   uint32_t                prev_ix;

   if (ix >= PF_MAX_TIMEOUTS)
   {
      LOG_ERROR(PNET_LOG, "Sched(%d): ix (%u) is invalid\n", __LINE__, (unsigned)ix);
   }
   else if (pf_scheduler_is_linked(*p_q, ix) == true)
   {
      LOG_ERROR(PNET_LOG, "Sched(%d): %s is already in Q\n", __LINE__, pf_timeouts[ix].p_name);
   }
   else if (pos >= PF_MAX_TIMEOUTS)
   {
      /* Put first in possible non-empty Q */
      pf_timeouts[ix].prev = PF_MAX_TIMEOUTS;
      pf_timeouts[ix].next = *p_q;
      if (*p_q < PF_MAX_TIMEOUTS)
      {
         pf_timeouts[*p_q].prev = ix;
      }

      *p_q = ix;
   }
   else if (*p_q >= PF_MAX_TIMEOUTS)
   {
      /* Q is empty - insert first in Q */
      pf_timeouts[ix].prev = PF_MAX_TIMEOUTS;
      pf_timeouts[ix].next = PF_MAX_TIMEOUTS;

      *p_q = ix;
   }
   else
   {
      prev_ix = pf_timeouts[pos].prev;

      if (prev_ix < PF_MAX_TIMEOUTS)
      {
         pf_timeouts[prev_ix].next = ix;
      }
      pf_timeouts[pos].prev = ix;

      pf_timeouts[ix].next = pos;
      pf_timeouts[ix].prev = prev_ix;

      if (*p_q == pos)
      {
         /* ix is now first in the Q */
         *p_q = ix;
      }
   }
}

void pf_scheduler_init(
   uint32_t                tick_interval)
{
   uint32_t ix;

   if (pf_timeout_mutex == NULL)
   {
      pf_timeout_mutex = os_mutex_create();
   }
   memset((void *)pf_timeouts, 0, sizeof(pf_timeouts));
   pf_timeout_first = PF_MAX_TIMEOUTS; /* Nothing in q. */
   pf_timeout_free  = PF_MAX_TIMEOUTS; /* Nothing in q. */

   pf_tick_interval = tick_interval;

   /* Link all entries into a list and put them into the free queue. */
   for (ix = PF_MAX_TIMEOUTS; ix > 0; ix--)
   {
      pf_timeouts[ix - 1].p_name = "<free>";
      pf_timeouts[ix - 1].in_use = false;
      pf_scheduler_link_before(&pf_timeout_free, ix - 1, pf_timeout_free);
   }
}

int pf_scheduler_add(
   uint32_t                delay,
   const char              *p_name,
   pf_scheduler_timeout_ftn_t cb,
   void                    *arg,
   uint32_t                *p_timeout)
{
   uint32_t                ix_this;
   uint32_t                ix_prev;
   uint32_t                ix_free;
   uint32_t                now = os_get_current_time_us();

   if (delay > 0x80000000)  /* Make sure it is reasonable */
   {
      printf("CPM(%d): Adjusting %s from %u\n", __LINE__, p_name, (unsigned)delay);
      delay = 1;
   }

   os_mutex_lock(pf_timeout_mutex);
   /* Unlink from the free list */
   ix_free = pf_timeout_free;
   pf_scheduler_unlink(&pf_timeout_free, ix_free);
   os_mutex_unlock(pf_timeout_mutex);

   if (ix_free >= PF_MAX_TIMEOUTS)
   {
      LOG_ERROR(PNET_LOG, "SCHEDULER(%d): Out of timeout resources!!\n", __LINE__);
      return -1;
   }

   pf_timeouts[ix_free].in_use = true;
   pf_timeouts[ix_free].p_name = p_name;
   pf_timeouts[ix_free].cb = cb;
   pf_timeouts[ix_free].arg = arg;
   pf_timeouts[ix_free].when = now + delay;

   os_mutex_lock(pf_timeout_mutex);
   if (pf_timeout_first >= PF_MAX_TIMEOUTS)
   {
      /* Put into empty q */
      pf_scheduler_link_before(&pf_timeout_first, ix_free, PF_MAX_TIMEOUTS);
   }
   else if (((int32_t)(pf_timeouts[ix_free].when - pf_timeouts[pf_timeout_first].when)) <= 0)
   {
      /* Put first in non-empty q */
      pf_scheduler_link_before(&pf_timeout_first, ix_free, pf_timeout_first);
   }
   else
   {
      /* Find pos in non-empty q */
      ix_prev = pf_timeout_first;
      ix_this = pf_timeouts[pf_timeout_first].next;
      while ((ix_this < PF_MAX_TIMEOUTS) &&
             (((int32_t)(pf_timeouts[ix_free].when - pf_timeouts[ix_this].when)) > 0))
      {
         ix_prev = ix_this;
         ix_this = pf_timeouts[ix_this].next;
      }

      /* Put after ix_prev */
      pf_scheduler_link_after(&pf_timeout_first, ix_free, ix_prev);
   }
   os_mutex_unlock(pf_timeout_mutex);

   *p_timeout = ix_free + 1;  /* Make sure 0 is invalid. */

   return 0;
}

void pf_scheduler_remove(
   const char              *p_name,
   uint32_t                timeout)
{
   uint16_t                      ix;

   if (timeout == 0)
   {
      LOG_DEBUG(PNET_LOG, "SCHEDULER(%d): timeout(%s) == 0\n", __LINE__, p_name);
   }
   else
   {
      ix = timeout - 1;  /* Refer to _add() on how p_timeout is created */
      os_mutex_lock(pf_timeout_mutex);

      if (pf_timeouts[ix].p_name != p_name)
      {
         LOG_ERROR(PNET_LOG, "SCHEDULER(%d): Expected %s but got %s\n", __LINE__, pf_timeouts[ix].p_name, p_name);
      }
      else
      {
         pf_scheduler_unlink(&pf_timeout_first, ix);

         /* Insert into free list. */
         pf_timeouts[ix].in_use = false;
         pf_scheduler_link_before(&pf_timeout_free, ix, pf_timeout_free);
      }

      os_mutex_unlock(pf_timeout_mutex);
   }
}

void pf_scheduler_tick(void)
{
   uint32_t                ix;
   pf_scheduler_timeout_ftn_t ftn;
   void                    *arg;
   uint32_t                pf_current_time = os_get_current_time_us();
   uint32_t                cnt = 0x10000000;

   os_mutex_lock(pf_timeout_mutex);

   /* Send event to all expired delay entries. */
   while ((pf_timeout_first < PF_MAX_TIMEOUTS) &&
          ((int32_t)(pf_current_time - pf_timeouts[pf_timeout_first].when) >= 0))
   {
      /* Unlink from busy list */
      ix = pf_timeout_first;
      pf_scheduler_unlink(&pf_timeout_first, ix);

      ftn = pf_timeouts[ix].cb;
      arg = pf_timeouts[ix].arg;

      /* Insert into free list. */
      pf_timeouts[ix].in_use = false;
      pf_scheduler_link_before(&pf_timeout_free, ix, pf_timeout_free);

      /* Send event without holding the mutex. */
      os_mutex_unlock(pf_timeout_mutex);
      ftn(arg, pf_current_time);
      os_mutex_lock(pf_timeout_mutex);

      cnt++;
   }

   os_mutex_unlock(pf_timeout_mutex);
}

void pf_scheduler_show(void)
{
   uint32_t                ix;
   uint32_t                cnt;

   printf("Scheduler (time now=%u):\n", (unsigned)os_get_current_time_us());

   if (pf_timeout_mutex != NULL)
   {
      os_mutex_lock(pf_timeout_mutex);
   }

   printf("%-4s  %-8s  %-6s  %-6s  %-6s  %s\n", "idx", "owner", "in_use", "next", "prev", "when");
   for (ix = 0; ix < PF_MAX_TIMEOUTS; ix++)
   {
      printf("[%02u]  %-8s  %-6s  %-6u  %-6u  %u\n", (unsigned)ix,
         pf_timeouts[ix].p_name, pf_timeouts[ix].in_use?"true":"false",
         (unsigned)pf_timeouts[ix].next, (unsigned)pf_timeouts[ix].prev,
         (unsigned)pf_timeouts[ix].when);
   }

   if (pf_timeout_mutex != NULL)
   {
      printf("Free list:\n");
      ix = pf_timeout_free;
      cnt = 0;
      while ((ix < PF_MAX_TIMEOUTS) && (cnt++ < 20))
      {
         printf("%u  ", (unsigned)ix);
         ix = pf_timeouts[ix].next;
      }

      printf("\nBusy list:\n");
      ix = pf_timeout_first;
      cnt = 0;
      while ((ix < PF_MAX_TIMEOUTS) && (cnt++ < 20))
      {
         printf("%u  (%u)  ", (unsigned)ix, (unsigned)pf_timeouts[ix].when);
         ix = pf_timeouts[ix].next;
      }

      os_mutex_unlock(pf_timeout_mutex);
   }

   printf("\n");
}
