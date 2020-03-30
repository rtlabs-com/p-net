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
 * Initialize the scheduler.
 * @param net              InOut: The p-net stack instance
 * @param tick_interval    In:   System calls the tick function at these intervals (ms).
 */
void pf_scheduler_init(
   pnet_t                     *net,
   uint32_t                   tick_interval);

/**
 * Schedule a call-back at a specific time.
 * @param net              InOut: The p-net stack instance
 * @param delay         In:   The delay until the function shall be called.
 * @param p_name        In:   Caller/owner (for debugging).
 * @param cb            In:   The call-back.
 * @param arg           In:   Argument to the call-back.
 * @param p_timeout     Out:  The timeout instance (used to remove if necessary).
 * @return  0  if the call-back was scheduled.
 *          -1 if an error occurred.
 */
int pf_scheduler_add(
   pnet_t                     *net,
   uint32_t                   delay,
   const char                 *p_name,
   pf_scheduler_timeout_ftn_t cb,
   void                       *arg,
   uint32_t                   *p_timeout);

/**
 * Stop a timeout. If it is not scheduled then ignore.
 * @param net              InOut: The p-net stack instance
 * @param p_name        In: Must be exactly the same address as in the _add().
 * @param timeout       In: Time instance to remove (see pf_scheduler_add)
 */
void pf_scheduler_remove(
   pnet_t                     *net,
   const char                 *p_name,
   uint32_t                   timeout);

/**
 * Check if it is time to call a scheduled call-back.
 * Run scheduled call-backs - if any.
 * @param net              InOut: The p-net stack instance
 */
void pf_scheduler_tick(
   pnet_t                  *net);

/**
 * Show scheduler (busy and free) instances.
 * @param net              InOut: The p-net stack instance
 */
void pf_scheduler_show(
   pnet_t                  *net);

#ifdef __cplusplus
}
#endif

#endif /* PF_SCHEDULER_H */
