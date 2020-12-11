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

#include "rowindex.h"

#include "options.h"
#include "pnet_api.h"
#include "osal.h"
#include "osal_log.h"
#include "pnal.h"
#include "pf_types.h"
#include "pf_snmp.h"
#include "pnal_snmp.h"

#include <string.h>

static void rowindex_construct_for_local_port (
   struct snmp_obj_id * row_index,
   int port)
{
   row_index->id[0] = port; /* lldpLocPortNum */
   row_index->len = 1;
}

static void rowindex_construct_for_local_interface (
   struct snmp_obj_id * row_index)
{
   pf_snmp_management_address_t address;
   size_t i;

   pf_snmp_get_management_address (pnal_snmp.net, &address);
   row_index->id[0] = address.subtype; /* lldpLocManAddrSubtype */
   for (i = 0; i < address.len; i++)
   {
      row_index->id[1 + i] = address.value[i]; /* lldpLocManAddr */
   }
   row_index->len = 1 + address.len;
}

static int rowindex_construct_for_remote_device (
   struct snmp_obj_id * row_index,
   int port)
{
   uint32_t timestamp;
   int error;

   error = pf_snmp_get_peer_timestamp (pnal_snmp.net, port, &timestamp);
   if (error)
   {
      return error;
   }

   row_index->id[0] = timestamp; /* lldpRemTimeMark */
   row_index->id[1] = port;      /* lldpRemLocalPortNum */
   row_index->id[2] = port;      /* lldpRemIndex */
   row_index->len = 3;

   return error;
}

static int rowindex_construct_for_remote_interface (
   struct snmp_obj_id * row_index,
   int port)
{
   uint32_t timestamp;
   pf_snmp_management_address_t address;
   int error;
   size_t i;

   error = pf_snmp_get_peer_timestamp (pnal_snmp.net, port, &timestamp);
   if (error)
   {
      return error;
   }
   error = pf_snmp_get_peer_management_address (pnal_snmp.net, port, &address);
   if (error)
   {
      return error;
   }

   row_index->id[0] = timestamp;       /* lldpRemTimeMark */
   row_index->id[1] = port;            /* lldpRemLocalPortNum */
   row_index->id[2] = port;            /* lldpRemIndex */
   row_index->id[3] = address.subtype; /* lldpRemManAddrSubtype */
   for (i = 0; i < address.len; i++)
   {
      row_index->id[4 + i] = address.value[i]; /* lldpRemManAddr */
   }
   row_index->len = 4 + address.len;

   return error;
}

int rowindex_match_with_local_port (const u32_t * row_oid, u8_t row_oid_len)
{
   pf_port_iterator_t port_iterator;
   int port;

   pf_snmp_init_port_iterator (pnal_snmp.net, &port_iterator);
   port = pf_snmp_get_next_port (&port_iterator);
   while (port != 0)
   {
      struct snmp_obj_id port_oid;

      rowindex_construct_for_local_port (&port_oid, port);
      if (snmp_oid_equal (row_oid, row_oid_len, port_oid.id, port_oid.len))
      {
         return port;
      }

      port = pf_snmp_get_next_port (&port_iterator);
   }

   return 0;
}

int rowindex_update_with_next_local_port (struct snmp_obj_id * row_oid)
{
   struct snmp_next_oid_state state;
   u32_t next_oid[SNMP_MAX_OBJ_ID_LEN];
   pf_port_iterator_t port_iterator;
   int port;

   snmp_next_oid_init (
      &state,
      row_oid->id,
      row_oid->len,
      next_oid,
      SNMP_MAX_OBJ_ID_LEN);
   pf_snmp_init_port_iterator (pnal_snmp.net, &port_iterator);
   port = pf_snmp_get_next_port (&port_iterator);
   while (port != 0)
   {
      struct snmp_obj_id port_oid;

      rowindex_construct_for_local_port (&port_oid, port);
      snmp_next_oid_check (&state, port_oid.id, port_oid.len, (void *)port);

      port = pf_snmp_get_next_port (&port_iterator);
   }

   if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS)
   {
      snmp_oid_assign (row_oid, state.next_oid, state.next_oid_len);
      port = (int)state.reference;
   }
   else
   {
      port = 0;
   }

   return port;
}

int rowindex_match_with_local_interface (const u32_t * row_oid, u8_t row_oid_len)
{
   struct snmp_obj_id interface_oid;
   int interface;

   rowindex_construct_for_local_interface (&interface_oid);
   if (snmp_oid_equal (row_oid, row_oid_len, interface_oid.id, interface_oid.len))
   {
      interface = 1;
   }
   else
   {
      interface = 0;
   }

   return interface;
}

snmp_err_t rowindex_update_with_next_local_interface (
   struct snmp_obj_id * row_oid)
{
   struct snmp_next_oid_state state;
   u32_t next_oid[SNMP_MAX_OBJ_ID_LEN];
   struct snmp_obj_id interface_oid;
   int interface;

   snmp_next_oid_init (
      &state,
      row_oid->id,
      row_oid->len,
      next_oid,
      SNMP_MAX_OBJ_ID_LEN);
   rowindex_construct_for_local_interface (&interface_oid);
   snmp_next_oid_check (&state, interface_oid.id, interface_oid.len, NULL);

   if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS)
   {
      snmp_oid_assign (row_oid, state.next_oid, state.next_oid_len);
      interface = 1;
   }
   else
   {
      interface = 0;
   }

   return interface;
}

int rowindex_match_with_remote_device (const u32_t * row_oid, u8_t row_oid_len)
{
   pf_port_iterator_t port_iterator;
   int port;

   pf_snmp_init_port_iterator (pnal_snmp.net, &port_iterator);
   port = pf_snmp_get_next_port (&port_iterator);
   while (port != 0)
   {
      struct snmp_obj_id port_oid;
      int error;

      error = rowindex_construct_for_remote_device (&port_oid, port);
      if (
         !error &&
         snmp_oid_equal (row_oid, row_oid_len, port_oid.id, port_oid.len))
      {
         return port;
      }

      port = pf_snmp_get_next_port (&port_iterator);
   }

   return 0;
}

int rowindex_update_with_next_remote_device (struct snmp_obj_id * row_oid)
{
   struct snmp_next_oid_state state;
   u32_t next_oid[SNMP_MAX_OBJ_ID_LEN];
   pf_port_iterator_t port_iterator;
   int port;

   snmp_next_oid_init (
      &state,
      row_oid->id,
      row_oid->len,
      next_oid,
      SNMP_MAX_OBJ_ID_LEN);
   pf_snmp_init_port_iterator (pnal_snmp.net, &port_iterator);
   port = pf_snmp_get_next_port (&port_iterator);
   while (port != 0)
   {
      int error;
      struct snmp_obj_id port_oid;

      error = rowindex_construct_for_remote_device (&port_oid, port);
      if (!error)
      {
         snmp_next_oid_check (&state, port_oid.id, port_oid.len, (void *)port);
      }

      port = pf_snmp_get_next_port (&port_iterator);
   }

   if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS)
   {
      snmp_oid_assign (row_oid, state.next_oid, state.next_oid_len);
      port = (int)state.reference;
   }
   else
   {
      port = 0;
   }

   return port;
}

int rowindex_match_with_remote_interface (
   const u32_t * row_oid,
   u8_t row_oid_len)
{
   pf_port_iterator_t port_iterator;
   int port;

   pf_snmp_init_port_iterator (pnal_snmp.net, &port_iterator);
   port = pf_snmp_get_next_port (&port_iterator);
   while (port != 0)
   {
      int error;
      struct snmp_obj_id port_oid;

      error = rowindex_construct_for_remote_interface (&port_oid, port);
      if (
         !error &&
         snmp_oid_equal (row_oid, row_oid_len, port_oid.id, port_oid.len))
      {
         return port;
      }

      port = pf_snmp_get_next_port (&port_iterator);
   }

   return 0;
}

int rowindex_update_with_next_remote_interface (struct snmp_obj_id * row_oid)
{
   struct snmp_next_oid_state state;
   u32_t next_oid[SNMP_MAX_OBJ_ID_LEN];
   pf_port_iterator_t port_iterator;
   int port;

   snmp_next_oid_init (
      &state,
      row_oid->id,
      row_oid->len,
      next_oid,
      SNMP_MAX_OBJ_ID_LEN);
   pf_snmp_init_port_iterator (pnal_snmp.net, &port_iterator);
   port = pf_snmp_get_next_port (&port_iterator);
   while (port != 0)
   {
      int error;
      struct snmp_obj_id port_oid;

      error = rowindex_construct_for_remote_interface (&port_oid, port);
      if (!error)
      {
         snmp_next_oid_check (&state, port_oid.id, port_oid.len, (void *)port);
      }

      port = pf_snmp_get_next_port (&port_iterator);
   }

   if (state.status == SNMP_NEXT_OID_STATUS_SUCCESS)
   {
      snmp_oid_assign (row_oid, state.next_oid, state.next_oid_len);
      port = (int)state.reference;
   }
   else
   {
      port = 0;
   }

   return port;
}
