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

#ifndef PF_UDP_H
#define PF_UDP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open an UDP socket. Listens to all IP addresses (PNAL_IPADDR_ANY).
 *
 * @param net              InOut: The p-net stack instance
 * @param port             In:    UDP port to listen to.
 * @return Socket ID, or -1 if an error occurred.
 */
int pf_udp_open (pnet_t * net, pnal_ipport_t port);

/**
 * Send UDP data
 *
 * Update interface statistics.
 *
 * @param net              InOut: The p-net stack instance
 * @param id               In:    Socket ID
 * @param dst_addr         In:    Destination IP address
 * @param dst_port         In:    Destination UDP port
 * @param data             In:    Data to be sent
 * @param size             In:    Size of data
 * @return  The number of bytes sent, or -1 if an error occurred.
 */
int pf_udp_sendto (
   pnet_t * net,
   uint32_t id,
   pnal_ipaddr_t dst_addr,
   pnal_ipport_t dst_port,
   const uint8_t * data,
   int size);

/**
 * Receive UDP data.
 *
 * Update interface statistics.
 *
 * This is a nonblocking function, and it
 * returns 0 immediately if no data is available.
 *
 * @param net              InOut: The p-net stack instance
 * @param id               In:    Socket ID
 * @param dst_addr         Out:   Source IP address
 * @param dst_port         Out:   Source UDP port
 * @param data             Out:   Received data
 * @param size             In:    Size of buffer for received data
 * @return  The number of bytes received, or -1 if an error occurred.
 */
int pf_udp_recvfrom (
   pnet_t * net,
   uint32_t id,
   pnal_ipaddr_t * src_addr,
   pnal_ipport_t * src_port,
   uint8_t * data,
   int size);

/**
 * Close an UDP socket.
 *
 * @param net              InOut: The p-net stack instance
 * @param id               In:    Socket ID
 */
void pf_udp_close (pnet_t * net, uint32_t id);

#ifdef __cplusplus
}
#endif

#endif /* PF_UDP_H */
