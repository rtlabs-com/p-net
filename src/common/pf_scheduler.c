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


static bool pf_scheduler_is_linked(
   pnet_t                  *net,
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
         first = net->scheduler_timeouts[first].next;
         cnt++;
      }
   }

   return ret;
}

static void pf_scheduler_unlink(
   pnet_t                  *net,
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
   else if (pf_scheduler_is_linked(net, *p_q, ix) == false)
   {
      LOG_ERROR(PNET_LOG, "Sched(%d): %s is not in Q\n", __LINE__, net->scheduler_timeouts[ix].p_name);
   }
   else
   {
      prev_ix = net->scheduler_timeouts[ix].prev;
      next_ix = net->scheduler_timeouts[ix].next;
      if (*p_q == ix)
      {
         *p_q = next_ix;
      }
      if (next_ix < PF_MAX_TIMEOUTS)
      {
         net->scheduler_timeouts[next_ix].prev = prev_ix;
      }
      if (prev_ix < PF_MAX_TIMEOUTS)
      {
         net->scheduler_timeouts[prev_ix].next = next_ix;
      }
   }
}

static void pf_scheduler_link_after(
   pnet_t                  *net,
   volatile uint32_t       *p_q,
   uint32_t                ix,
   uint32_t                pos)
{
   uint32_t                next_ix;

   if (ix >= PF_MAX_TIMEOUTS)
   {
      LOG_ERROR(PNET_LOG, "Sched(%d): ix (%u) is invalid\n", __LINE__, (unsigned)ix);
   }
   else if (pf_scheduler_is_linked(net, *p_q, ix) == true)
   {
      LOG_ERROR(PNET_LOG, "Sched(%d): %s is already in Q\n", __LINE__, net->scheduler_timeouts[ix].p_name);
   }
   else if (pos >= PF_MAX_TIMEOUTS)
   {
      /* Put first in possible non-empty Q */
      net->scheduler_timeouts[ix].prev = PF_MAX_TIMEOUTS;
      net->scheduler_timeouts[ix].next = *p_q;
      if (*p_q < PF_MAX_TIMEOUTS)
      {
         net->scheduler_timeouts[*p_q].prev = ix;
      }

      *p_q = ix;
   }
   else if (*p_q >= PF_MAX_TIMEOUTS)
   {
      /* Q is empty - insert first in Q */
      net->scheduler_timeouts[ix].prev = PF_MAX_TIMEOUTS;
      net->scheduler_timeouts[ix].next = PF_MAX_TIMEOUTS;

      *p_q = ix;
   }
   else
   {
      next_ix = net->scheduler_timeouts[pos].next;

      if (next_ix < PF_MAX_TIMEOUTS)
      {
         net->scheduler_timeouts[next_ix].prev = ix;
      }
      net->scheduler_timeouts[pos].next = ix;

      net->scheduler_timeouts[ix].prev = pos;
      net->scheduler_timeouts[ix].next = next_ix;
   }
}

static void pf_scheduler_link_before(
   pnet_t                  *net,
   volatile uint32_t       *p_q,
   uint32_t                ix,
   uint32_t                pos)
{
   uint32_t                prev_ix;

   if (ix >= PF_MAX_TIMEOUTS)
   {
      LOG_ERROR(PNET_LOG, "Sched(%d): ix (%u) is invalid\n", __LINE__, (unsigned)ix);
   }
   else if (pf_scheduler_is_linked(net, *p_q, ix) == true)
   {
      LOG_ERROR(PNET_LOG, "Sched(%d): %s is already in Q\n", __LINE__, net->scheduler_timeouts[ix].p_name);
   }
   else if (pos >= PF_MAX_TIMEOUTS)
   {
      /* Put first in possible non-empty Q */
      net->scheduler_timeouts[ix].prev = PF_MAX_TIMEOUTS;
      net->scheduler_timeouts[ix].next = *p_q;
      if (*p_q < PF_MAX_TIMEOUTS)
      {
         net->scheduler_timeouts[*p_q].prev = ix;
      }

      *p_q = ix;
   }
   else if (*p_q >= PF_MAX_TIMEOUTS)
   {
      /* Q is empty - insert first in Q */
      net->scheduler_timeouts[ix].prev = PF_MAX_TIMEOUTS;
      net->scheduler_timeouts[ix].next = PF_MAX_TIMEOUTS;

      *p_q = ix;
   }
   else
   {
      prev_ix = net->scheduler_timeouts[pos].prev;

      if (prev_ix < PF_MAX_TIMEOUTS)
      {
         net->scheduler_timeouts[prev_ix].next = ix;
      }
      net->scheduler_timeouts[pos].prev = ix;

      net->scheduler_timeouts[ix].next = pos;
      net->scheduler_timeouts[ix].prev = prev_ix;

      if (*p_q == pos)
      {
         /* ix is now first in the Q */
         *p_q = ix;
      }
   }
}

void pf_scheduler_init(
   pnet_t                  *net,
   uint32_t                tick_interval)
{
   uint32_t ix;

   net->scheduler_timeout_first = PF_MAX_TIMEOUTS; /* Nothing in queue */
   net->scheduler_timeout_free = PF_MAX_TIMEOUTS;  /* Nothing in queue. */

   if (net->scheduler_timeout_mutex == NULL)
   {
      net->scheduler_timeout_mutex = os_mutex_create();
   }
   memset((void *)net->scheduler_timeouts, 0, sizeof(net->scheduler_timeouts));

   net->scheduler_tick_interval = tick_interval;  /* Cannot be zero */

   /* Link all entries into a list and put them into the free queue. */
   for (ix = PF_MAX_TIMEOUTS; ix > 0; ix--)
   {
      net->scheduler_timeouts[ix - 1].p_name = "<free>";
      net->scheduler_timeouts[ix - 1].in_use = false;
      pf_scheduler_link_before(net, &net->scheduler_timeout_free, ix - 1, net->scheduler_timeout_free);
   }
}

int pf_scheduler_add(
   pnet_t                  *net,
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

   os_mutex_lock(net->scheduler_timeout_mutex);
   /* Unlink from the free list */
   ix_free = net->scheduler_timeout_free;
   pf_scheduler_unlink(net, &net->scheduler_timeout_free, ix_free);
   os_mutex_unlock(net->scheduler_timeout_mutex);

   if (ix_free >= PF_MAX_TIMEOUTS)
   {
      LOG_ERROR(PNET_LOG, "SCHEDULER(%d): Out of timeout resources!!\n", __LINE__);
      return -1;
   }

   net->scheduler_timeouts[ix_free].in_use = true;
   net->scheduler_timeouts[ix_free].p_name = p_name;
   net->scheduler_timeouts[ix_free].cb = cb;
   net->scheduler_timeouts[ix_free].arg = arg;
   net->scheduler_timeouts[ix_free].when = now + delay;

   os_mutex_lock(net->scheduler_timeout_mutex);
   if (net->scheduler_timeout_first >= PF_MAX_TIMEOUTS)
   {
      /* Put into empty q */
      pf_scheduler_link_before(net, &net->scheduler_timeout_first, ix_free, PF_MAX_TIMEOUTS);
   }
   else if (((int32_t)(net->scheduler_timeouts[ix_free].when - net->scheduler_timeouts[net->scheduler_timeout_first].when)) <= 0)
   {
      /* Put first in non-empty q */
      pf_scheduler_link_before(net, &net->scheduler_timeout_first, ix_free, net->scheduler_timeout_first);
   }
   else
   {
      /* Find pos in non-empty q */
      ix_prev = net->scheduler_timeout_first;
      ix_this = net->scheduler_timeouts[net->scheduler_timeout_first].next;
      while ((ix_this < PF_MAX_TIMEOUTS) &&
             (((int32_t)(net->scheduler_timeouts[ix_free].when - net->scheduler_timeouts[ix_this].when)) > 0))
      {
         ix_prev = ix_this;
         ix_this = net->scheduler_timeouts[ix_this].next;
      }

      /* Put after ix_prev */
      pf_scheduler_link_after(net, &net->scheduler_timeout_first, ix_free, ix_prev);
   }
   os_mutex_unlock(net->scheduler_timeout_mutex);

   *p_timeout = ix_free + 1;  /* Make sure 0 is invalid. */

   return 0;
}

void pf_scheduler_remove(
   pnet_t                  *net,
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
      os_mutex_lock(net->scheduler_timeout_mutex);

      if (net->scheduler_timeouts[ix].p_name != p_name)
      {
         LOG_ERROR(PNET_LOG, "SCHEDULER(%d): Expected %s but got %s\n", __LINE__, net->scheduler_timeouts[ix].p_name, p_name);
      }
      else
      {
         pf_scheduler_unlink(net, &net->scheduler_timeout_first, ix);

         /* Insert into free list. */
         net->scheduler_timeouts[ix].in_use = false;
         pf_scheduler_link_before(net, &net->scheduler_timeout_free, ix, net->scheduler_timeout_free);
      }

      os_mutex_unlock(net->scheduler_timeout_mutex);
   }
}

void pf_scheduler_tick(
   pnet_t                  *net)
{
   uint32_t                ix;
   pf_scheduler_timeout_ftn_t ftn;
   void                    *arg;
   uint32_t                pf_current_time = os_get_current_time_us();
   uint32_t                cnt = 0x10000000;

   os_mutex_lock(net->scheduler_timeout_mutex);

   /* Send event to all expired delay entries. */
   while ((net->scheduler_timeout_first < PF_MAX_TIMEOUTS) &&
          ((int32_t)(pf_current_time - net->scheduler_timeouts[net->scheduler_timeout_first].when) >= 0))
   {
      /* Unlink from busy list */
      ix = net->scheduler_timeout_first;
      pf_scheduler_unlink(net, &net->scheduler_timeout_first, ix);

      ftn = net->scheduler_timeouts[ix].cb;
      arg = net->scheduler_timeouts[ix].arg;

      /* Insert into free list. */
      net->scheduler_timeouts[ix].in_use = false;
      pf_scheduler_link_before(net, &net->scheduler_timeout_free, ix, net->scheduler_timeout_free);

      /* Send event without holding the mutex. */
      os_mutex_unlock(net->scheduler_timeout_mutex);
      ftn(net, arg, pf_current_time);
      os_mutex_lock(net->scheduler_timeout_mutex);

      cnt++;
   }

   os_mutex_unlock(net->scheduler_timeout_mutex);
}

void pf_scheduler_show(
   pnet_t                  *net)
{
   uint32_t                ix;
   uint32_t                cnt;

   printf("Scheduler (time now=%u):\n", (unsigned)os_get_current_time_us());

   if (net->scheduler_timeout_mutex != NULL)
   {
      os_mutex_lock(net->scheduler_timeout_mutex);
   }

   printf("%-4s  %-8s  %-6s  %-6s  %-6s  %s\n", "idx", "owner", "in_use", "next", "prev", "when");
   for (ix = 0; ix < PF_MAX_TIMEOUTS; ix++)
   {
      printf("[%02u]  %-8s  %-6s  %-6u  %-6u  %u\n", (unsigned)ix,
         net->scheduler_timeouts[ix].p_name, net->scheduler_timeouts[ix].in_use?"true":"false",
         (unsigned)net->scheduler_timeouts[ix].next, (unsigned)net->scheduler_timeouts[ix].prev,
         (unsigned)net->scheduler_timeouts[ix].when);
   }

   if (net->scheduler_timeout_mutex != NULL)
   {
      printf("Free list:\n");
      ix = net->scheduler_timeout_free;
      cnt = 0;
      while ((ix < PF_MAX_TIMEOUTS) && (cnt++ < 20))
      {
         printf("%u  ", (unsigned)ix);
         ix = net->scheduler_timeouts[ix].next;
      }

      printf("\nBusy list:\n");
      ix = net->scheduler_timeout_first;
      cnt = 0;
      while ((ix < PF_MAX_TIMEOUTS) && (cnt++ < 20))
      {
         printf("%u  (%u)  ", (unsigned)ix, (unsigned)net->scheduler_timeouts[ix].when);
         ix = net->scheduler_timeouts[ix].next;
      }

      os_mutex_unlock(net->scheduler_timeout_mutex);
   }

   printf("\n");
}
