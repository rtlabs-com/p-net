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

#ifndef PF_PORT_H
#define PF_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init port data
 *
 * @param net              In:    The p-net stack instance.
 */
void pf_port_init(pnet_t * net);

/**
 * Get list of local ports.
 *
 * This is the list of physical ports on the local interface.
 * The management port is not included.
 *
 * @param net              In:    The p-net stack instance.
 * @param p_list           Out:   List of local ports.
 */
void pf_port_get_list_of_ports (pnet_t * net, pf_lldp_port_list_t * p_list);

/**
 * Initialise iterator for iterating over local ports.
 *
 * This iterator may be used for iterating over all physical ports
 * on the local interface. The management port is not included.
 * See pf_port_get_next().
 *
 * @param net              In:    The p-net stack instance.
 * @param p_iterator       Out:   Port iterator.
 */
void pf_port_init_iterator_over_ports (pnet_t * net, pf_port_iterator_t * p_iterator);

/**
 * Get next local port.
 *
 * If no more ports are available, 0 is returned.
 *
 * @param p_iterator       InOut: Port iterator.
 * @return Local port number for next port on local interface.
 *         If no more ports are available, 0 is returned.
 */
int pf_port_get_next (pf_port_iterator_t * p_iterator);

/**
 * Get a reference to port runtime data.
 *
 * If the local port number is out of range this operation will assert.
 * NULL will never be returned.
 *
 * @param net              In:    The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 * @return Address to port runtime data
 */
pf_port_t * pf_port_get_state (
   pnet_t * net,
   int loc_port_num);

#ifdef __cplusplus
}
#endif

#endif /* PF_PORT_H */
