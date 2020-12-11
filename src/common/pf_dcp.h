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

#ifndef PF_DCP_H
#define PF_DCP_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The option and sub-option values are used by DCP and also by CMINA.
 */
typedef enum pf_dcp_opt_values
{
   PF_DCP_OPT_RESERVED_0 = 0,
   PF_DCP_OPT_IP,
   PF_DCP_OPT_DEVICE_PROPERTIES,
   PF_DCP_OPT_DHCP,
   PF_DCP_OPT_RESERVED_4,
   PF_DCP_OPT_CONTROL,
   PF_DCP_OPT_DEVICE_INITIATIVE,
   /*
    * Reserved                0x07 .. 0x7f
    * Manufacturer specific   0x80 .. 0xfe
    */
   PF_DCP_OPT_ALL = 0xff
} pf_dcp_opt_values_t;

typedef enum pf_dcp_sub_all_values
{
   PF_DCP_SUB_ALL = 0xff
} pf_dcp_sub_all_values_t;

typedef enum pf_dcp_sub_ip_values
{
   PF_DCP_SUB_IP_MAC = 0x01, /* Read */
   PF_DCP_SUB_IP_PAR,        /* Read/Write */
   PF_DCP_SUB_IP_SUITE       /* Read/(optional Write) */
} pf_dcp_sub_ip_values_t;

typedef enum pf_dcp_sub_dev_prop
{
   PF_DCP_SUB_DEV_PROP_VENDOR = 0x01, /* Read */
   PF_DCP_SUB_DEV_PROP_NAME,          /* Read/Write */
   PF_DCP_SUB_DEV_PROP_ID,            /* Read */
   PF_DCP_SUB_DEV_PROP_ROLE,          /* Read */
   PF_DCP_SUB_DEV_PROP_OPTIONS,       /* Read */
   PF_DCP_SUB_DEV_PROP_ALIAS,         /* Used as filter only */
   PF_DCP_SUB_DEV_PROP_INSTANCE,      /* Read, optional */
   PF_DCP_SUB_DEV_PROP_OEM_ID,        /* Read, optional */
   PF_DCP_SUB_DEV_PROP_GATEWAY        /* Read, optional */
} pf_dcp_sub_dev_prop_t;

typedef enum pf_dcp_sub_dhcp
{
   PF_DCP_SUB_DHCP_HOSTNAME = 12,
   PF_DCP_SUB_DHCP_VENDOR_SPEC = 43,
   PF_DCP_SUB_DHCP_SERVER_ID = 54,
   PF_DCP_SUB_DHCP_PAR_REQ_LIST = 55,
   PF_DCP_SUB_DHCP_CLASS_ID = 60,
   PF_DCP_SUB_DHCP_CLIENT_ID = 61,
   PF_DCP_SUB_DHCP_FQDN = 81,
   PF_DCP_SUB_DHCP_UUID_CLIENT_ID = 97,
   PF_DCP_SUB_DHCP_CONTROL = 255 /* Defined as END in the DHCP spec */
} pf_dcp_sub_dhcp_t;

typedef enum pf_dcp_sub_control
{
   PF_DCP_SUB_CONTROL_START = 0x01, /* Write */
   PF_DCP_SUB_CONTROL_STOP,         /* Write */
   PF_DCP_SUB_CONTROL_SIGNAL,       /* Write */
   PF_DCP_SUB_CONTROL_RESPONSE,
   PF_DCP_SUB_CONTROL_FACTORY_RESET,    /* Optional, Write */
   PF_DCP_SUB_CONTROL_RESET_TO_FACTORY, /* Write */
} pf_dcp_sub_control_t;

typedef enum pf_dcp_sub_dev_initiative
{
   PF_DCP_SUB_DEV_INITIATIVE_SUPPORT = 0x01 /* Read */
} pf_dcp_sub_dev_initiative_t;

typedef enum pf_dcp_block_error_values
{
   PF_DCP_BLOCK_ERROR_NO_ERROR,
   PF_DCP_BLOCK_ERROR_OPTION_NOT_SUPPORTED,
   PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SUPPORTED,
   PF_DCP_BLOCK_ERROR_SUBOPTION_NOT_SET,
   PF_DCP_BLOCK_ERROR_RESOURCE_ERROR,
   PF_DCP_BLOCK_ERROR_SET_NOT_POSSIBLE
} pf_dcp_block_error_values_t;

/**
 * Initialize the DCP component.
 *
 * Registers incoming frame IDs with the frame handler.
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_dcp_init (pnet_t * net);

/**
 * Stop the DCP component.
 *
 * Unregisters frame IDs from the frame handler.
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_dcp_exit (pnet_t * net);

/**
 * Send a DCP HELLO message.
 * @param net              InOut: The p-net stack instance
 * @return  0  if a HELLO message was sent.
 *          -1 if an error occurred.
 */
int pf_dcp_hello_req (pnet_t * net);

/************ Internal functions, made available for unit testing ************/

uint32_t pf_dcp_calculate_response_delay (
   const pnet_ethaddr_t * mac_address,
   uint16_t response_delay_factor);

#ifdef __cplusplus
}
#endif

#endif /* PF_DCP_H */
