/*
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

p-net Profinet device stack
===========================

Copyright 2018 rt-labs AB

This instance of p-net is licensed for use in a specific product, as
follows:

Company: Bender GmbH & Co. KG
Product: COMTRAXX
Product description: COMTRAXX products provided by Bender GmbH & Co. KG

Use of this instance of p-net in other products is not allowed.

You are hereby given a non-exclusive, non-transferable license to use
the SOFTWARE and documentation according to the SOFTWARE LICENSE
AGREEMENT document shipped together with this instance of the
SOFTWARE. The SOFTWARE LICENSE AGREEMENT prohibits copying and
redistribution of the SOFTWARE.  It prohibits reverse-engineering
activities.  It limits warranty and liability on behalf of rt-labs.
The details of these terms are explained in subsequent chapters of the
SOFTWARE LICENSE AGREEMENT.
-----BEGIN PGP SIGNATURE-----

iQEzBAEBCgAdFiEE1e4U+gPgGJXLsxxLZlG0bopmiXIFAl/TMvQACgkQZlG0bopm
iXJE1QgAlEKblKkIrLF+hVRm6I1IfbGV6VWnBx+9IPwaHZmgQZAVTOlp4RHiNAre
Il1l1xheENWpuihbVlvz5hMCVto4fkT13IUXxOo9Va9MK8lm6JOu2lYpl1HgVIwV
R6I6dsWLeNBSBaA38DDBHIfjP4LBZC/HVYO2ISc3/OKutqasz/WIp+AjAURiKRsf
yaWGay88FITHymmqJpGaWzB+jAOvRFs8hW/GsOyX/r17AK5F5N51O+UqUTKAWNPT
f6M8QF0b07Hw+eOuB7A/0D5nvSl/rOd9LvYkiUhIUmQdHqERNkF9EJdroIjxTMXi
lmoOkPEqyGV58fh7JjMYlI60sPlGBw==
=TaN+
-----END PGP SIGNATURE-----
*/

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
 * intervals, in microseconds.
 */
void pf_scheduler_init (pnet_t * net, uint32_t tick_interval);

/**
 * Schedule a call-back at a specific time.
 *
 * @param net              InOut: The p-net stack instance
 * @param delay            In:    The delay until the function shall be called,
 *                                in microseconds. Max
 * PF_SCHEDULER_MAX_DELAY_US.
 * @param p_name           In:    Caller/owner (for debugging).
 * @param cb               In:    The call-back.
 * @param arg              In:    Argument to the call-back.
 * @param p_timeout        Out:   The timeout instance (used to remove if
 * necessary).
 * @return  0  if the call-back was scheduled.
 *          -1 if an error occurred.
 */
int pf_scheduler_add (
   pnet_t * net,
   uint32_t delay,
   const char * p_name,
   pf_scheduler_timeout_ftn_t cb,
   void * arg,
   uint32_t * p_timeout);

/**
 * Stop a timeout. If it is not scheduled then ignore.
 * @param net              InOut: The p-net stack instance
 * @param p_name           In: Must be exactly the same address as in the
 * _add().
 * @param timeout          In: Time instance to remove (see pf_scheduler_add)
 */
void pf_scheduler_remove (pnet_t * net, const char * p_name, uint32_t timeout);

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
