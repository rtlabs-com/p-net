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

#ifndef PF_CMWRR_H
#define PF_CMWRR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the CMWRR component.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmwrr_init (pnet_t * net);

/**
 * Show the CMWRR part of the specified AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 */
void pf_cmwrr_show (const pnet_t * net, const pf_ar_t * p_ar);

/**
 * Handle CMDEV events.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance. Not used,
 * @param event            In:    The new CMDEV state. Use PNET_EVENT_xxx, not
 *                                PF_CMDEV_STATE_xxx
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmwrr_cmdev_state_ind (
   pnet_t * net,
   const pf_ar_t * p_ar,
   pnet_event_values_t event);

/**
 * Handle RPC write requests.
 *
 * Triggers the \a pnet_write_ind() user callback for some values.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_write_request  In:    The write request block.
 * @param p_write_result   Out:   The write result block.
 * @param p_result         Out:   The result codes.
 * @param p_req_buf        In:    The RPC request buffer.
 * @param data_length      In:    The length of the data to write.
 * @param p_req_pos        In:    Position in p_req_buf.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmwrr_rm_write_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_request,
   pf_iod_write_result_t * p_write_result,
   pnet_result_t * p_result,
   uint8_t * p_req_buf,
   uint16_t data_length,
   uint16_t * p_req_pos);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMWRR_H */
