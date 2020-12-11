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

#ifndef PF_FILE_H
#define PF_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * Load a binary file, and verify the file version.
 *
 * @param directory        In:    Directory for files. Terminated string. NULL
 *                                or empty string is interpreted as current
 *                                directory.
 * @param filename         In:    File name. Terminated string.
 * @param p_object         Out:   Struct to load
 * @param size             In:    Size of struct to load
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred (for example wrong version).
 */
int pf_file_load (
   const char * directory,
   const char * filename,
   void * p_object,
   size_t size);

/**
 * Save a binary file, and include version information.
 *
 * @param directory        In:    Directory for files. Terminated string. NULL
 *                                or empty string is interpreted as current
 *                                directory.
 * @param filename         In:    File name. Terminated string.
 * @param p_object         In:    Struct to save
 * @param size             In:    Size of struct to save
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_file_save (
   const char * directory,
   const char * filename,
   const void * p_object,
   size_t size);

/**
 * Save a binary file if modified, and include version information.
 *
 * No saving is done if the content would be the same. This reduces the flash
 * memory wear.
 *
 * @param directory        In:    Directory for files. Terminated string. NULL
 *                                or empty string is interpreted as current
 *                                directory.
 * @param filename         In:    File name. Terminated string.
 * @param p_object         In:    Struct to save
 * @param p_tempobject     Temp:  Temporary buffer (of same size as object) for
 *                                loading existing file.
 * @param size             In:    Size of struct to save
 * @return  2 First saving of file (no previous file with correct version found)
 *          1 Updated file
 *          0  No storing required (no changes)
 *          -1 if an error occurred.
 */
int pf_file_save_if_modified (
   const char * directory,
   const char * filename,
   const void * p_object,
   void * p_tempobject,
   size_t size);

/**
 * Clear a binary file.
 *
 * @param directory        In:    Directory for files. Terminated string. NULL
 *                                or empty string is interpreted as current
 *                                directory.
 * @param filename         In:    File name. Terminated string.
 */
void pf_file_clear (const char * directory, const char * filename);

/************ Internal functions, made available for unit testing ************/

int pf_file_join_directory_filename (
   const char * directory,
   const char * filename,
   char * fullpath,
   size_t size);

#ifdef __cplusplus
}
#endif

#endif /* PF_FILE_H */