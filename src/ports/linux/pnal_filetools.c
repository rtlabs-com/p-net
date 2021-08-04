
/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2020 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#define _GNU_SOURCE /* Enable execvpe () */

#include "pnal_filetools.h"

#include "pnet_options.h"
#include "options.h"
#include "osal_log.h"

#include <sys/stat.h>
#include <sys/wait.h>

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool pnal_does_file_exist (const char * filepath)
{
   struct stat statbuffer;

   return (stat (filepath, &statbuffer) == 0);
}

/**
 * @internal
 * Get the path to the directory where the main binary is located.
 *
 * @param tempstring       Temp:  Temporary buffer.
 * @param dirpath          Out:   Path to directory. Terminated string.
 * @param size             In:    Size of the stringbuffers. Should be large
 *                                enough for the full path to the binary.
 *
 * @return 0 on success.
 *         -1 if an error occurred.
 */
static int pnal_get_directory_of_binary (
   char * tempstring,
   char * dirpath,
   size_t size)
{
   ssize_t num_bytes = 0;
   memset (tempstring, 0, size); /* Path to executable */
   memset (dirpath, 0, size);
   char * resulting_directory = NULL;

   num_bytes = readlink ("/proc/self/exe", tempstring, size);
   if (num_bytes == -1)
   {
      return -1;
   }

   /* Verify that we have the full path (size is big enough) */
   if (!pnal_does_file_exist (tempstring))
   {
      return -1;
   }

   resulting_directory = dirname (tempstring);
   snprintf (dirpath, size, "%s", resulting_directory);

   return 0;
}

/**
 * @internal
 * Create a PATH string used when searching for scripts
 *
 * The PATH string is colon separated, and includes the directory where
 * the main binary is located.
 *
 * @param path             Out:   Resulting path. Terminated string.
 *                                Might be modified also on failure.
 * @param size             In:    Size of the outputbuffer. Should be
 *                                sizeof(PNAL_DEFAULT_SEARCHPATH) +
 *                                PNET_MAX_DIRECTORYPATH_SIZE
 * @return 0 on success.
 *         -1 if an error occurred.
 */
int pnal_create_searchpath (char * path, size_t size)
{
   int written = 0;

   /** Should temporarily hold full path. Resulting size is
    * PNET_MAX_DIRECTORYPATH_SIZE */
   char
      directory_of_binary[PNET_MAX_DIRECTORYPATH_SIZE + PNET_MAX_FILENAME_SIZE] =
         {0};
   char tempstring[PNET_MAX_DIRECTORYPATH_SIZE + PNET_MAX_FILENAME_SIZE] = {0};

   if (
      pnal_get_directory_of_binary (
         tempstring,
         directory_of_binary,
         sizeof (directory_of_binary)) != 0)
   {
      return -1;
   }

   written = snprintf (
      path,
      size,
      "%s:%s",
      PNAL_DEFAULT_SEARCHPATH,
      directory_of_binary);
   if (written < 0 || (unsigned)written >= size)
   {
      return -1;
   }

   return 0;
}

int pnal_execute_script (const char * argv[])
{
   /** Terminated string, see pnal_create_searchpath() */
   char child_searchpath
      [sizeof (PNAL_DEFAULT_SEARCHPATH) + PNET_MAX_DIRECTORYPATH_SIZE] = {0};
   int childstatus = 0;
   pid_t childpid;
   char * scriptenviron[] = {NULL};
   int script_returnvalue = 0;
#if LOG_DEBUG_ENABLED(PF_PNAL_LOG)
   uint16_t ix;
#endif

   if (argv == NULL)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): No argument vector given for running script.\n",
         __LINE__);
      return -1;
   }

   if (argv[0] == NULL)
   {
      LOG_ERROR (PF_PNAL_LOG, "PNAL(%d): No script name given.\n", __LINE__);
      return -1;
   }

   if (pnal_create_searchpath (child_searchpath, sizeof (child_searchpath)) != 0)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): Could not build PATH to run script.\n",
         __LINE__);
      return -1;
   }

#if LOG_DEBUG_ENABLED(PF_PNAL_LOG)
   printf ("PNAL(%d): Command for script:", __LINE__);
   for (ix = 0; argv[ix] != NULL; ix++)
   {
      if (strlen (argv[ix]) > 0)
      {
         printf (" %s", argv[ix]);
      }
      else
      {
         printf (" ''");
      }
   }
   printf ("\n");
#endif

   /* Fork and exec */
   childpid = fork();
   if (childpid < 0)
   {
      LOG_ERROR (
         PF_PNAL_LOG,
         "PNAL(%d): Failed to fork the process to run script.\n",
         __LINE__);
      return -1;
   }
   else if (childpid == 0)
   {
      /* We are in the child process */

      /* setenv() will malloc memory. We will not free() it, but it does not
         matter since we do it in the short-lived child process */
      if (setenv ("PATH", child_searchpath, 1) != 0)
      {
         printf (
            "PNAL(%d): Failed to set PATH in child process to run script.\n",
            __LINE__);
         exit (EXIT_FAILURE);
      }

      execvpe (argv[0], (char * const *)argv, scriptenviron);

      printf (
         "PNAL(%d): Failed to execute in child process. Is the script file "
         "missing or lacks execution permission? %s Search path %s\n",
         __LINE__,
         argv[0],
         child_searchpath);
      exit (EXIT_FAILURE);
   }
   else
   {
      /* We are in the parent (original) process */
      wait (&childstatus);
      script_returnvalue = WEXITSTATUS (childstatus);
   }

   return (script_returnvalue == 0) ? 0 : -1;
}
