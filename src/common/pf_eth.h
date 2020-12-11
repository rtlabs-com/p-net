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

#ifndef PF_ETH_H
#define PF_ETH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the ETH component.
 * @param net              InOut: The p-net stack instance
 * @return  0  if the ETH component could be initialized.
 *          -1 if an error occurred.
 */
int pf_eth_init (pnet_t * net);

/**
 * Send raw Ethernet data.
 *
 * Update interface statistics.
 *
 * @param net              InOut: The p-net stack instance
 * @param handle           In:    Ethernet handle
 * @param buf              In:    Buffer with data to be sent
 * @return  The number of bytes sent, or -1 if an error occurred.
 */
int pf_eth_send (pnet_t * net, pnal_eth_handle_t * handle, pnal_buf_t * buf);

/**
 * Add a frame_id entry to the frame id filter map.
 *
 * This function adds an entry to the frame id table.
 * This table is used to map incoming frames to the right handler functions,
 * based on the frame id.
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:   The frame ID to look for.
 * @param frame_handler    In:   The handler function to call.
 * @param p_arg            In:   Argument to handler function.
 */
void pf_eth_frame_id_map_add (
   pnet_t * net,
   uint16_t frame_id,
   pf_eth_frame_handler_t frame_handler,
   void * p_arg);

/**
 * Remove an entry from the frame id filter map (if it exists).
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:   The frame ID to remove.
 */
void pf_eth_frame_id_map_remove (pnet_t * net, uint16_t frame_id);

/**
 * Inspect and possibly handle Ethernet frames:
 *
 * Find the packet type. If it is for Profinet then send it to the right
 * handler, depending on the frame_id within the packet. The frame_id is located
 * right after the packet type. Take care of handling the VLAN tag!!
 *
 * Note that this itself is a callback, and its arguments should fulfill the
 * prototype os_raw_frame_handler_t
 *
 * @param arg              InOut: User argument, will be converted to pnet_t
 * @param p_buf            In:    The Ethernet frame. Might be freed.
 * @return  0  If the frame was NOT handled by this function.
 *          1  If the frame was handled and the buffer freed.
 */
int pf_eth_recv (void * arg, pnal_buf_t * p_buf);

#ifdef __cplusplus
}
#endif

#endif /* PF_DCPMCS_H */
