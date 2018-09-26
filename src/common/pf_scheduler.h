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
extern "C"
{
#endif

/**
 * The prototype of the externally supplied call-back functions.
 * @param arg              In:   User-defined (may be NULL).
 * @param current_time     In:   The current system time.
 */
typedef void (*pf_scheduler_timeout_ftn_t)(
   void                       *arg,
   uint32_t                   current_time);

/**
 * Initialize the scheduler.
 * @param tick_interval    In:   System calls the tick function at these intervals (ms).
 */
void pf_scheduler_init(
   uint32_t                   tick_interval);

/**
 * Schedule a call-back at a specific time.
 * @param delay         In:   The delay until the function shall be called.
 * @param p_name        In:   Caller/owner (for debugging).
 * @param cb            In:   The call-back.
 * @param arg           In:   Argument to the call-back.
 * @param p_timeout     Out:  The timeout instance (used to remove if necessary).
 * @return  0  if the call-back was scheduled.
 *          -1 if an error occurred.
 */
int pf_scheduler_add(
   uint32_t                   delay,
   const char                 *p_name,
   pf_scheduler_timeout_ftn_t cb,
   void                       *arg,
   uint32_t                   *p_timeout);

/**
 * Stop a timeout. If it is not scheduled then ignore.
 * @param p_name        In: Must be exactly the same address as in the _add().
 * @param timeout       In: Time instance to remove (see pf_scheduler_add)
 */
void pf_scheduler_remove(
   const char                 *p_name,
   uint32_t                   timeout);

/**
 * Check if it is time to call a scheduled call-back.
 * Run scheduled call-backs - if any.
 */
void pf_scheduler_tick(void);

/**
 * Show scheduler (busy and free) instances.
 */
void pf_scheduler_show(void);

#ifdef __cplusplus
}
#endif

#endif /* PF_SCHEDULER_H */
