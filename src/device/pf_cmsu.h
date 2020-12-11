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

#ifndef PF_CMSU_H
#define PF_CMSU_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the CMSU component.
 */
void pf_cmsu_init (pnet_t * net);

/**
 * Handle CMDEV events.
 *
 * Can close CPM and PPM instances. Closes alarm instance, which triggers an
 * alarm.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param event            In:    The new CMDEV state. Use PNET_EVENT_xxx, not
 *                                PF_CMDEV_STATE_xxx
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsu_cmdev_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_event_values_t event);

/**
 * Start all state machines for a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_stat           Out:   The result information.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsu_start_req (pnet_t * net, pf_ar_t * p_ar, pnet_result_t * p_stat);

/**
 * Handle CPM error indications for a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param err_cls          In:    ERR_CLS.
 * @param err_code         In:    ERR_CODE.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsu_cpm_error_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint8_t err_cls,
   uint8_t err_code);

/**
 * Handle PPM error indications for a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param err_cls          In:    ERR_CLS.
 * @param err_code         In:    ERR_CODE.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsu_ppm_error_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint8_t err_cls,
   uint8_t err_code);

/**
 * Handle alarm error indications for a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param err_cls          In:    ERR_CLS.
 * @param err_code         In:    ERR_CODE
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsu_alarm_error_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint8_t err_cls,
   uint8_t err_code);

/* Not used */
/**
 * Handle DMC error indications for a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param err_cls          In:    ERR_CLS
 * @param err_code         In:    ERR_CODE
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsu_cmdmc_error_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint8_t err_cls,
   uint8_t err_code);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMSU_H */
