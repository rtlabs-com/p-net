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
 * @brief Scheduler implementation
 *
 * Use the scheduler to execute callbacks after a known delay time.
 *
 * This file should not use \a pnet_t, except in \a pf_scheduler_tick() where it
 * is passed as an argument to the callback.
 */

#ifdef UNIT_TEST
#endif

#include "pf_includes.h"

#include <inttypes.h>
#include <string.h>

/**************** Utilities for linked lists ************************/

/**
 * Check if a timeout index is present in a linked list.
 *
 * @param scheduler_data   InOut: Scheduler instance
 * @param head             In:    Head of the list
 * @param ix               In:    Timeout index to look for
 * @return true if timeout index is found in the list
 */
static bool pf_scheduler_is_linked (
   volatile pf_scheduler_data_t * scheduler_data,
   uint32_t head,
   uint32_t ix)
{
   uint32_t current = head;
   uint32_t cnt = 0; /* Guard against infinite loop */

   if (ix >= PF_MAX_TIMEOUTS)
   {
      return false;
   }

   while (current < PF_MAX_TIMEOUTS)
   {
      CC_ASSERT (cnt < PF_MAX_TIMEOUTS);
      if (current == ix)
      {
         return true;
      }
      current = scheduler_data->timeouts[current].next;
      cnt++;
   }

   return false;
}

/**
 * Unlink a timeout from a linked list
 *
 * @param scheduler_data   InOut: Scheduler instance
 * @param p_head           InOut: Head of the list
 * @param ix               In:    Timeout index to unlink. Might be invalid if
 *                                the list is empty.
 */
static void pf_scheduler_unlink (
   volatile pf_scheduler_data_t * scheduler_data,
   volatile uint32_t * p_head,
   uint32_t ix)
{
   uint32_t prev_ix;
   uint32_t next_ix;

   if (ix >= PF_MAX_TIMEOUTS)
   {
      LOG_ERROR (
         PNET_LOG,
         "Sched(%d): Timeout index %u is invalid.\n",
         __LINE__,
         (unsigned)ix);
      return;
   }

   if (pf_scheduler_is_linked (scheduler_data, *p_head, ix) == false)
   {
      LOG_ERROR (
         PNET_LOG,
         "Sched(%d): Timeout %s is not in this queue\n",
         __LINE__,
         scheduler_data->timeouts[ix].name);
      return;
   }

   prev_ix = scheduler_data->timeouts[ix].prev;
   next_ix = scheduler_data->timeouts[ix].next;
   if (*p_head == ix)
   {
      *p_head = next_ix;
   }
   if (next_ix < PF_MAX_TIMEOUTS)
   {
      scheduler_data->timeouts[next_ix].prev = prev_ix;
   }
   if (prev_ix < PF_MAX_TIMEOUTS)
   {
      scheduler_data->timeouts[prev_ix].next = next_ix;
   }
}

/**
 * Link a timeout after a position in a linked list
 *
 * Will be put first in the list if \a pos is PF_MAX_TIMEOUTS or larger.
 *
 * @param scheduler_data   InOut: Scheduler instance
 * @param p_head           InOut: Head of the list
 * @param ix               In:    Timeout index to link
 * @param pos              In:    Position.
 */
static void pf_scheduler_link_after (
   volatile pf_scheduler_data_t * scheduler_data,
   volatile uint32_t * p_head,
   uint32_t ix,
   uint32_t pos)
{
   uint32_t next_ix;

   if (ix >= PF_MAX_TIMEOUTS)
   {
      LOG_ERROR (
         PNET_LOG,
         "Sched(%d): ix (%u) is invalid\n",
         __LINE__,
         (unsigned)ix);
      return;
   }

   if (pf_scheduler_is_linked (scheduler_data, *p_head, ix) == true)
   {
      LOG_ERROR (
         PNET_LOG,
         "Sched(%d): %s is already in the queue\n",
         __LINE__,
         scheduler_data->timeouts[ix].name);
      return;
   }

   if (pos >= PF_MAX_TIMEOUTS)
   {
      /* Insert it first in possibly non-empty queue */
      scheduler_data->timeouts[ix].prev = PF_MAX_TIMEOUTS;
      scheduler_data->timeouts[ix].next = *p_head;
      if (*p_head < PF_MAX_TIMEOUTS)
      {
         scheduler_data->timeouts[*p_head].prev = ix;
      }

      *p_head = ix;
      return;
   }

   if (*p_head >= PF_MAX_TIMEOUTS)
   {
      /* This queue is empty - Insert it first in the queue */
      scheduler_data->timeouts[ix].prev = PF_MAX_TIMEOUTS;
      scheduler_data->timeouts[ix].next = PF_MAX_TIMEOUTS;

      *p_head = ix;
      return;
   }

   next_ix = scheduler_data->timeouts[pos].next;

   if (next_ix < PF_MAX_TIMEOUTS)
   {
      scheduler_data->timeouts[next_ix].prev = ix;
   }
   scheduler_data->timeouts[pos].next = ix;

   scheduler_data->timeouts[ix].prev = pos;
   scheduler_data->timeouts[ix].next = next_ix;
}

/**
 * Link a timeout before a position in a linked list
 *
 * Will be put first in the list if \a pos is PF_MAX_TIMEOUTS or larger.
 *
 * @param scheduler_data   InOut: Scheduler instance
 * @param p_head           InOut: Head of the list
 * @param ix               In:    Timeout index to link
 * @param pos              In:    Position
 */
static void pf_scheduler_link_before (
   volatile pf_scheduler_data_t * scheduler_data,
   volatile uint32_t * p_head,
   uint32_t ix,
   uint32_t pos)
{
   uint32_t prev_ix;

   if (ix >= PF_MAX_TIMEOUTS)
   {
      LOG_ERROR (
         PNET_LOG,
         "Sched(%d): ix (%u) is invalid\n",
         __LINE__,
         (unsigned)ix);
      return;
   }

   if (pf_scheduler_is_linked (scheduler_data, *p_head, ix) == true)
   {
      LOG_ERROR (
         PNET_LOG,
         "Sched(%d): %s is already in this queue\n",
         __LINE__,
         scheduler_data->timeouts[ix].name);
      return;
   }

   if (pos >= PF_MAX_TIMEOUTS)
   {
      /* Insert it first in possibly non-empty queue */
      scheduler_data->timeouts[ix].prev = PF_MAX_TIMEOUTS;
      scheduler_data->timeouts[ix].next = *p_head;
      if (*p_head < PF_MAX_TIMEOUTS)
      {
         scheduler_data->timeouts[*p_head].prev = ix;
      }

      *p_head = ix;
      return;
   }

   if (*p_head >= PF_MAX_TIMEOUTS)
   {
      /* This queue is empty - Insert it first in the queue */
      scheduler_data->timeouts[ix].prev = PF_MAX_TIMEOUTS;
      scheduler_data->timeouts[ix].next = PF_MAX_TIMEOUTS;

      *p_head = ix;
      return;
   }

   prev_ix = scheduler_data->timeouts[pos].prev;

   if (prev_ix < PF_MAX_TIMEOUTS)
   {
      scheduler_data->timeouts[prev_ix].next = ix;
   }
   scheduler_data->timeouts[pos].prev = ix;

   scheduler_data->timeouts[ix].next = pos;
   scheduler_data->timeouts[ix].prev = prev_ix;

   if (*p_head == pos)
   {
      /* ix is now first in the queue */
      *p_head = ix;
   }
}

/****************************** Public functions ***************************/

void pf_scheduler_reset_handle (pf_scheduler_handle_t * handle)
{
   handle->timer_index = UINT32_MAX;
}

void pf_scheduler_init_handle (pf_scheduler_handle_t * handle, const char * name)
{
   handle->name = name;
   pf_scheduler_reset_handle (handle);
}

const char * pf_scheduler_get_name (const pf_scheduler_handle_t * handle)
{
   return handle->name;
}

uint32_t pf_scheduler_get_value (const pf_scheduler_handle_t * handle)
{
   return handle->timer_index;
}

bool pf_scheduler_is_running (const pf_scheduler_handle_t * handle)
{
   return (handle->timer_index == UINT32_MAX) ? false : true;
}

void pf_scheduler_remove_if_running (
   volatile pf_scheduler_data_t * scheduler_data,
   pf_scheduler_handle_t * handle)
{
   if (pf_scheduler_is_running (handle))
   {
      pf_scheduler_remove (scheduler_data, handle);
   }
}

int pf_scheduler_restart (
   volatile pf_scheduler_data_t * scheduler_data,
   uint32_t delay,
   pf_scheduler_timeout_ftn_t cb,
   void * arg,
   pf_scheduler_handle_t * handle,
   uint32_t current_time)
{
   pf_scheduler_remove_if_running (scheduler_data, handle);
   return pf_scheduler_add (scheduler_data, delay, cb, arg, handle, current_time);
}

void pf_scheduler_init (
   volatile pf_scheduler_data_t * scheduler_data,
   uint32_t tick_interval)
{
   uint32_t ix;
   uint32_t j;

   scheduler_data->busylist_head = PF_MAX_TIMEOUTS; /* Nothing in queue */
   scheduler_data->freelist_head = PF_MAX_TIMEOUTS; /* Nothing in queue. */

   if (scheduler_data->mutex == NULL)
   {
      scheduler_data->mutex = os_mutex_create();
   }
   memset (
      (void *)scheduler_data->timeouts,
      0,
      sizeof (scheduler_data->timeouts));

   scheduler_data->tick_interval = tick_interval;
   CC_ASSERT (scheduler_data->tick_interval > 0);

   /* Link all entries into a list and put them into the free queue. */
   for (j = PF_MAX_TIMEOUTS; j > 0; j--)
   {
      ix = j - 1;
      scheduler_data->timeouts[ix].name = "<free>";
      scheduler_data->timeouts[ix].in_use = false;
      pf_scheduler_link_before (
         scheduler_data,
         &scheduler_data->freelist_head,
         ix,
         scheduler_data->freelist_head);
   }
}

int pf_scheduler_add (
   volatile pf_scheduler_data_t * scheduler_data,
   uint32_t delay,
   pf_scheduler_timeout_ftn_t cb,
   void * arg,
   pf_scheduler_handle_t * handle,
   uint32_t current_time)
{
   uint32_t ix_this;
   uint32_t ix_prev;
   uint32_t ix_free;

   delay =
      pf_scheduler_sanitize_delay (delay, scheduler_data->tick_interval, true);

   /* Unlink from the free list */
   os_mutex_lock (scheduler_data->mutex);
   ix_free = scheduler_data->freelist_head;
   pf_scheduler_unlink (scheduler_data, &scheduler_data->freelist_head, ix_free);
   os_mutex_unlock (scheduler_data->mutex);

   if (ix_free >= PF_MAX_TIMEOUTS)
   {
      LOG_ERROR (
         PNET_LOG,
         "SCHEDULER(%d): Out of timeout resources!!\n",
         __LINE__);
      handle->timer_index = UINT32_MAX;
      return -1;
   }

   scheduler_data->timeouts[ix_free].in_use = true;
   scheduler_data->timeouts[ix_free].name = handle->name;
   scheduler_data->timeouts[ix_free].cb = cb;
   scheduler_data->timeouts[ix_free].arg = arg;
   scheduler_data->timeouts[ix_free].when = current_time + delay;

   os_mutex_lock (scheduler_data->mutex);
   if (scheduler_data->busylist_head >= PF_MAX_TIMEOUTS)
   {
      /* Put into empty queue */
      pf_scheduler_link_before (
         scheduler_data,
         &scheduler_data->busylist_head,
         ix_free,
         PF_MAX_TIMEOUTS);
   }
   else if (
      ((int32_t) (
         scheduler_data->timeouts[ix_free].when -
         scheduler_data->timeouts[scheduler_data->busylist_head].when)) <= 0)
   {
      /* Put first in non-empty queue */
      pf_scheduler_link_before (
         scheduler_data,
         &scheduler_data->busylist_head,
         ix_free,
         scheduler_data->busylist_head);
   }
   else
   {
      /* Find correct position in non-empty queue */
      ix_prev = scheduler_data->busylist_head;
      ix_this = scheduler_data->timeouts[scheduler_data->busylist_head].next;
      while ((ix_this < PF_MAX_TIMEOUTS) &&
             (((int32_t) (
                 scheduler_data->timeouts[ix_free].when -
                 scheduler_data->timeouts[ix_this].when)) > 0))
      {
         ix_prev = ix_this;
         ix_this = scheduler_data->timeouts[ix_this].next;
      }

      /* Put after ix_prev */
      pf_scheduler_link_after (
         scheduler_data,
         &scheduler_data->busylist_head,
         ix_free,
         ix_prev);
   }
   os_mutex_unlock (scheduler_data->mutex);

   handle->timer_index = ix_free + 1; /* Make sure 0 is invalid. */

   return 0;
}

void pf_scheduler_remove (
   volatile pf_scheduler_data_t * scheduler_data,
   pf_scheduler_handle_t * handle)
{
   uint16_t ix;

   if (handle->timer_index == 0 || handle->timer_index > PF_MAX_TIMEOUTS)
   {
      LOG_ERROR (
         PNET_LOG,
         "SCHEDULER(%d): Invalid value %" PRIu32 " for timeout \"%s\". No "
         "removal.\n",
         __LINE__,
         handle->timer_index,
         handle->name);
      return;
   }

   /* See pf_scheduler_add() for handle->timer_index details */
   ix = handle->timer_index - 1;
   os_mutex_lock (scheduler_data->mutex);

   if (scheduler_data->timeouts[ix].name != handle->name)
   {
      LOG_ERROR (
         PNET_LOG,
         "SCHEDULER(%d): Expected \"%s\" but got \"%s\". No removal.\n",
         __LINE__,
         scheduler_data->timeouts[ix].name,
         handle->name);
   }
   else if (scheduler_data->timeouts[ix].in_use == false)
   {
      LOG_DEBUG (
         PNET_LOG,
         "SCHEDULER(%d): Tried to remove timeout \"%s\", but it has already "
         "been triggered.\n",
         __LINE__,
         handle->name);
   }
   else
   {
      /* Unlink from busy list */
      pf_scheduler_unlink (scheduler_data, &scheduler_data->busylist_head, ix);

      /* Insert into free list. */
      scheduler_data->timeouts[ix].in_use = false;
      scheduler_data->timeouts[ix].name = "<free>";
      pf_scheduler_link_before (
         scheduler_data,
         &scheduler_data->freelist_head,
         ix,
         scheduler_data->freelist_head);

      handle->timer_index = UINT32_MAX;
   }

   os_mutex_unlock (scheduler_data->mutex);
}

void pf_scheduler_tick (
   pnet_t * net,
   volatile pf_scheduler_data_t * scheduler_data,
   uint32_t current_time)
{
   uint32_t ix;
   pf_scheduler_timeout_ftn_t ftn;
   void * arg;

   os_mutex_lock (scheduler_data->mutex);

   /* Send event to all expired delay entries. */
   while ((scheduler_data->busylist_head < PF_MAX_TIMEOUTS) &&
          ((int32_t) (
              current_time -
              scheduler_data->timeouts[scheduler_data->busylist_head].when) >= 0))
   {
      /* Unlink from busy list */
      ix = scheduler_data->busylist_head;
      pf_scheduler_unlink (scheduler_data, &scheduler_data->busylist_head, ix);

      ftn = scheduler_data->timeouts[ix].cb;
      arg = scheduler_data->timeouts[ix].arg;

      /* Insert into free list. */
      scheduler_data->timeouts[ix].in_use = false;
      pf_scheduler_link_before (
         scheduler_data,
         &scheduler_data->freelist_head,
         ix,
         scheduler_data->freelist_head);

      /* Send event without holding the mutex. */
      os_mutex_unlock (scheduler_data->mutex);
      ftn (net, arg, current_time);
      os_mutex_lock (scheduler_data->mutex);
   }

   os_mutex_unlock (scheduler_data->mutex);
}

void pf_scheduler_show (
   volatile pf_scheduler_data_t * scheduler_data,
   uint32_t current_time)
{
   uint32_t ix;

   printf ("Scheduler (time now=%" PRIu32 " microseconds):\n", current_time);

   if (scheduler_data->mutex != NULL)
   {
      os_mutex_lock (scheduler_data->mutex);
   }

   printf (
      "%-4s  %-14s  %-6s  %-6s  %-6s  %s\n",
      "idx",
      "owner",
      "in_use",
      "next",
      "prev",
      "when");
   for (ix = 0; ix < PF_MAX_TIMEOUTS; ix++)
   {
      printf (
         "[%02u]  %-14s  %-6s  %-6u  %-6u  %u\n",
         (unsigned)ix,
         scheduler_data->timeouts[ix].name,
         scheduler_data->timeouts[ix].in_use ? "true" : "false",
         (unsigned)scheduler_data->timeouts[ix].next,
         (unsigned)scheduler_data->timeouts[ix].prev,
         (unsigned)scheduler_data->timeouts[ix].when);
   }

   if (scheduler_data->mutex != NULL)
   {
      printf ("Free list:\n");
      ix = scheduler_data->freelist_head;
      while (ix < PF_MAX_TIMEOUTS)
      {
         printf ("%u  ", (unsigned)ix);
         ix = scheduler_data->timeouts[ix].next;
      }

      printf ("\nBusy list:\n");
      ix = scheduler_data->busylist_head;
      while (ix < PF_MAX_TIMEOUTS)
      {
         printf (
            "%u  (%u)  ",
            (unsigned)ix,
            (unsigned)scheduler_data->timeouts[ix].when);
         ix = scheduler_data->timeouts[ix].next;
      }

      os_mutex_unlock (scheduler_data->mutex);
   }
   printf ("\n");
}

/********************** Numerical utilities *****************************/

/**
 * @internal
 * Sanitize the delay to use with the scheduler, by taking the stack cycle time
 * into account.
 *
 * If the requested delay is in the range 1.5 to 2.5 stack cycle times, this
 * function will return a calculated delay giving a periodicity of 2 stack cycle
 * times. If the requested time is less than 1.5 stack cycle times, the
 * resulting periodicity is 1 stack cycle time.
 *
 *    Number of stack cycle times to wait
 *
 *     ^
 *     |
 *   4 +                                         +--------
 *     |                                         |
 *     |                                         |
 *   3 +                             +-----------+
 *     |                             |
 *     |                             |
 *   2 +                 +-----------+
 *     |                 |
 *     |                 |
 *   1 +-----------------+
 *     |
 *     |                                                         Wanted delay
 *   0 +-----------+-----------+-----------+-----------+--->   (in stack cycle
 *                                                              times)
 *
 *     0           1           2           3           4
 *
 * Scheduling a delay close to a multiple of the stack cycle time is risky, and
 * should be avoided. These measurements were made using a Ubuntu Laptop:
 *
 *    With a stack cycle time of 1 ms, a scheduled delay of 0-700 microseconds
 *    will cause a nice periodicity of 1 ms. A scheduled delay of 1000
 *    microseconds will sometimes fire at next cycle, sometimes not. This gives
 *    an event spacing of 1 or 2 ms.
 *
 *    Similarly a scheduled delay of 1100 to 1700 microseconds causes
 *    a nice periodicity of 2 ms, and a scheduled delay of 2100 to 2700
 *    microseconds causes a nice periodicity of 3 ms.
 *
 * Thus calculate the number of stack cycles to wait, and calculate a delay
 * corresponding to half a cycle less
 * (by setting schedule_half_tick_in_advance = true). Some operating systems
 * do not require this.
 *
 * This function calculates the delay time required to make the
 * scheduler fire at a specific stack tick. However the time jitter in the
 * firing is largely dependent on the underlying operating system's ablitity to
 * trigger the stack execution with a high time precision.
 *
 * Will assert for stack_cycle_time == 0.
 *
 * @param wanted_delay                    In:    Delay in microseconds.
 * @param stack_cycle_time                In:    Stack cycle time in
 *                                               microseconds.
 *                                               Must be larger than 0.
 * @param schedule_half_tick_in_advance   In:    Schedule event slightly
 *                                               earlier, to not be missed.
 * @return Number of microseconds of delay to use with the scheduler.
 */
uint32_t pf_scheduler_sanitize_delay (
   uint32_t wanted_delay,
   uint32_t stack_cycle_time,
   bool schedule_half_tick_in_advance)
{
   uint32_t number_of_stack_ticks = 1; /* We must wait at least one tick */
   uint32_t resulting_delay = 0;

   CC_ASSERT (stack_cycle_time > 0);

   /* Protect against "negative" or unreasonable values.
      This might happen when subtracting two timestamps, if a deadline has been
      missed. */
   if (wanted_delay > PF_SCHEDULER_MAX_DELAY_US)
   {
      wanted_delay = 0;
   }

   /* Calculate integer number of ticks to wait. Use integer division for
    * truncation */
   if (wanted_delay > (stack_cycle_time + stack_cycle_time / 2))
   {
      number_of_stack_ticks =
         (wanted_delay + (stack_cycle_time / 2)) / stack_cycle_time;
   }
   CC_ASSERT (number_of_stack_ticks >= 1);
   CC_ASSERT (number_of_stack_ticks < 0x80000000); /* No rollover to 'negative'
                                                      numbers */

   /* Calculate delay value for the sheduler */
   resulting_delay = number_of_stack_ticks * stack_cycle_time;
   if (schedule_half_tick_in_advance == true)
   {
      resulting_delay -= (stack_cycle_time / 2);
   }

   return resulting_delay;
}
