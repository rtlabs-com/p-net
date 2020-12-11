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

#ifndef PNAL_SYS_H
#define PNAL_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/dev.h>
#include <sys/socket.h> /* For htonl etc */
#include <lwip/pbuf.h>

#define OS_BUF_MAX_SIZE PBUF_POOL_BUFSIZE

/* Re-use lwIP pbuf for rt-kernel */
typedef struct pbuf pnal_buf_t;

/* TODO: Integrate in standard drivers?
 * Local handling to enable the NIC drivers to support a RX hook
 */
#define IOCTL_NET_SET_RX_HOOK 0x601

int eth_ioctl (drv_t * drv, void * arg, int req, void * param);

/**
 * The prototype of raw Ethernet reception call-back functions.
 * *
 * @param arg              In:   User-defined (may be NULL).
 * @param p_buf            In:   The incoming Ethernet frame
 *
 * @return  0  If the frame was NOT handled by this function.
 *          1  If the frame was handled and the buffer freed.
 */
typedef int (pnal_eth_callback_t) (void * arg, pnal_buf_t * p_buf);

typedef struct os_eth_handle
{
   pnal_eth_callback_t * callback;
   void * arg;
   int if_id;
} pnal_eth_handle_t;

#ifdef __cplusplus
}
#endif

#endif /* PNAL_SYS_H */
