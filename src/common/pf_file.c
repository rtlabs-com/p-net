/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2018 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

/**
 * @file
 * @brief Utility functions for reading and writing data from/to disk or flash.
 *
 * Adds the bytes "PNET" and a version indicator to the beginning of the file
 * when writing. Checks the corresponding values when reading.
 *
 */

#ifdef UNIT_TEST
#define os_clear_file mock_os_clear_file
#define os_load_file  mock_os_load_file
#define os_save_file  mock_os_save_file
#endif

#include "pf_includes.h"
#include "pf_block_reader.h"
#include "pf_block_writer.h"

#include <stdlib.h>
#include <string.h>

#define PF_FILE_MAGIC 0x504E4554U /* "PNET" */
#define PF_FILE_VERSION                                                        \
   0x00000001U /* Increase every time the saved contents have another format   \
                */

/**
 * @internal
 * Join directory and filename into a full path
 *
 * If no directory is given, use only the filename.
 *
 * @param directory        In:    Directory for files. Terminated string. NULL
 *                                or empty string is interpreted as current
 * directory.
 * @param filename         In:    File name
 * @param fullpath         Out:   Resulting string
 * @param size             In:    Size of outputbuffer.
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred.
 */
int pf_file_join_directory_filename (
   const char * directory,
   const char * filename,
   char * fullpath,
   size_t size)
{
   bool use_directory = true;

   if (filename == NULL || fullpath == NULL)
   {
      LOG_ERROR (
         PNET_LOG,
         "FILE(%d): Filename and outputbuffer must be given!\n",
         __LINE__);
      return -1;
   }
   if (strlen (filename) < 1)
   {
      LOG_ERROR (PNET_LOG, "FILE(%d): Too short filename!\n", __LINE__);
      return -1;
   }

   if (directory == NULL)
   {
      use_directory = false;
   }
   else
   {
      if (strlen (directory) == 0)
      {
         use_directory = false;
      }
   }

   if (use_directory == true)
   {
      if (strlen (directory) + strlen (filename) + 2 > size)
      {
         LOG_ERROR (
            PNET_LOG,
            "FILE(%d): Too long directory and filename! Max total length %zu\n",
            __LINE__,
            size);
         return -1;
      }

      strcpy (fullpath, directory);
      if (fullpath[strlen (directory) - 1] != '/')
      {
         strcat (fullpath, "/");
      }
      strcat (fullpath, filename);
   }
   else
   {
      if (strlen (filename) + 1 > size)
      {
         LOG_ERROR (
            PNET_LOG,
            "FILE(%d): Too long filename! Max total length %zu\n",
            __LINE__,
            size);
         return -1;
      }
      strcpy (fullpath, filename);
   }

   return 0;
}

int pf_file_load (
   const char * directory,
   const char * filename,
   void * object,
   size_t size)
{
   uint8_t versioning_buffer[8] = {0}; /* Two uint32_t */
   pf_get_info_t bufferinfo;
   uint32_t version = 0;
   uint32_t magic = 0;
   uint16_t pos = 0;
   char path[PNET_MAX_FILE_FULLPATH_LEN];

   bufferinfo.p_buf = versioning_buffer;
   bufferinfo.len = sizeof (versioning_buffer);
   bufferinfo.is_big_endian = true;
   bufferinfo.result = PF_PARSE_OK;

   if (
      pf_file_join_directory_filename (
         directory,
         filename,
         path,
         PNET_MAX_FILE_FULLPATH_LEN) != 0)
   {
      return -1;
   }

   /* Read file */
   if (
      os_load_file (
         path,
         versioning_buffer,
         sizeof (versioning_buffer),
         object,
         size) != 0)
   {
      return -1;
   }
   magic = pf_get_uint32 (&bufferinfo, &pos);
   version = pf_get_uint32 (&bufferinfo, &pos);

   /* Check file magic bytes */
   if (magic != PF_FILE_MAGIC)
   {
      LOG_ERROR (
         PNET_LOG,
         "FILE(%d): Wrong file magic bytes in file %s\n",
         __LINE__,
         path);
      return -1;
   }

   /* Check file version */
   if (version != PF_FILE_VERSION)
   {
      LOG_WARNING (
         PNET_LOG,
         "FILE(%d): Wrong file version identifier in file %s  Expected %" PRIu32
         " but got %" PRIu32 ".\n",
         __LINE__,
         path,
         (uint32_t)PF_FILE_VERSION,
         version);
      return -1;
   }

   return 0;
}

int pf_file_save (
   const char * directory,
   const char * filename,
   void * object,
   size_t size)
{
   char path[PNET_MAX_FILE_FULLPATH_LEN];
   uint8_t versioning_buffer[8] = {0}; /* Two uint32_t */
   uint16_t pos = 0;

   if (
      pf_file_join_directory_filename (
         directory,
         filename,
         path,
         PNET_MAX_FILE_FULLPATH_LEN) != 0)
   {
      return -1;
   }

   pf_put_uint32 (
      true,
      PF_FILE_MAGIC,
      sizeof (versioning_buffer),
      versioning_buffer,
      &pos); /* Big endian */
   pf_put_uint32 (
      true,
      PF_FILE_VERSION,
      sizeof (versioning_buffer),
      versioning_buffer,
      &pos); /* Big endian */

   return os_save_file (
      path,
      versioning_buffer,
      sizeof (versioning_buffer),
      object,
      size);
}

void pf_file_clear (const char * directory, const char * filename)
{
   char path[PNET_MAX_FILE_FULLPATH_LEN];

   if (
      pf_file_join_directory_filename (
         directory,
         filename,
         path,
         PNET_MAX_FILE_FULLPATH_LEN) != 0)
   {
      return;
   }

   return os_clear_file (path);
}
