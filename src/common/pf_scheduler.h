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

#ifndef PF_SCHEDULER_H
#define PF_SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

#define PF_SCHEDULER_MAX_DELAY_US 100000000U /* 100 seconds */

/**
 * Initialize the scheduler.
 * @param net              InOut: The p-net stack instance
 * @param tick_interval    In:    System calls the tick function at these
 *                                intervals, in microseconds. Must be
 *                                larger than 0.
 */
void pf_scheduler_init (pnet_t * net, uint32_t tick_interval);

/**
 * Initialize a timeout handle.
 * @param handle           Out:   Timeout handle.
 * @param name             In:    Descriptive name for debugging. Not NULL.
 */
void pf_scheduler_init_handle (
   pf_scheduler_handle_t * handle,
   const char * name);

/**
 * Reset the value inside the timeout handle to indicate that it's not running.
 *
 * May be used inside the callback (which is executed by the scheduler).
 *
 * To unschedule a callback, use \a pf_scheduler_remove() or
 * \a pf_scheduler_remove_if_running() instead.
 *
 * @param handle           Out:   Timeout handle.
 */
void pf_scheduler_reset_handle (pf_scheduler_handle_t * handle);

/**
 * Report whether the timeout is running (=scheduled)
 *
 * @param handle           InOut: Timeout handle.
 * @return  true if it is running
 *          false otherwise
 */
bool pf_scheduler_is_running (const pf_scheduler_handle_t * handle);

/**
 * Report the internal value of a timeout handle.
 *
 * Intended for debugging and testing.
 *
 * The value is an index into the timers, and is 0 < x <= PF_MAX_TIMEOUTS when
 * the timer is running. The value is UINT32_MAX when it is stopped.
 *
 * @param handle           InOut: Timeout handle.
 * @return the internal value (related to index in array of timeouts)
 */
uint32_t pf_scheduler_get_value (const pf_scheduler_handle_t * handle);

/**
 * Report the name of a timeout handle.
 *
 * Intended for debugging and testing.
 *
 * @param handle           InOut: Timeout handle.
 * @return the name given at initialization.
 */
const char * pf_scheduler_get_name (const pf_scheduler_handle_t * handle);

/**
 * Schedule a call-back at a specific time.
 *
 * The callback must update the internal state stored in the handle. That is
 * done by calling \a pf_scheduler_reset_handle() or again calling
 * \ pf_scheduler_add().
 *
 * @param net              InOut: The p-net stack instance
 * @param delay            In:    The delay until the function shall be called,
 *                                in microseconds. Max
 *                                PF_SCHEDULER_MAX_DELAY_US.
 * @param cb               In:    The call-back.
 * @param arg              In:    Argument to the call-back.
 * @param handle           InOut: Timeout handle.
 * @return  0  if the call-back was scheduled.
 *          -1 if an error occurred.
 */
int pf_scheduler_add (
   pnet_t * net,
   uint32_t delay,
   pf_scheduler_timeout_ftn_t cb,
   void * arg,
   pf_scheduler_handle_t * handle);

/**
 * Re-schedule a call-back at a specific time.
 *
 * Removes the already scheduled instance, if existing.
 *
 * Do not use in a scheduler callback.
 *
 * @param net              InOut: The p-net stack instance
 * @param delay            In:    The delay until the function shall be called,
 *                                in microseconds. Max
 *                                PF_SCHEDULER_MAX_DELAY_US.
 * @param cb               In:    The call-back.
 * @param arg              In:    Argument to the call-back.
 * @param handle           InOut: Timeout handle.
 * @return  0  if the call-back was scheduled.
 *          -1 if an error occurred.
 */
int pf_scheduler_restart (
   pnet_t * net,
   uint32_t delay,
   pf_scheduler_timeout_ftn_t cb,
   void * arg,
   pf_scheduler_handle_t * handle);

/**
 * Stop a timeout, if it is running.
 *
 * Silently ignoring the timeout if not running.
 *
 * @param net              InOut: The p-net stack instance
 * @param handle           InOut: Timeout handle.
 */
void pf_scheduler_remove_if_running (
   pnet_t * net,
   pf_scheduler_handle_t * handle);

/**
 * Stop a timeout.
 * @param net              InOut: The p-net stack instance
 * @param handle           InOut: Timeout handle.
 */
void pf_scheduler_remove (pnet_t * net, pf_scheduler_handle_t * handle);

/**
 * Check if it is time to call a scheduled call-back.
 * Run scheduled call-backs - if any.
 * @param net              InOut: The p-net stack instance
 */
void pf_scheduler_tick (pnet_t * net);

/**
 * Show scheduler (busy and free) instances.
 *
 * Locks the mutex temporarily.
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_scheduler_show (pnet_t * net);

/************ Internal functions, made available for unit testing ************/

uint32_t pf_scheduler_sanitize_delay (
   uint32_t wanted_delay,
   uint32_t stack_cycle_time,
   bool schedule_half_tick_in_advance);

#ifdef __cplusplus
}
#endif

#endif /* PF_SCHEDULER_H */
