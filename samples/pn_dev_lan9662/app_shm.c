/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "app_shm.h"
#include "app_log.h"

#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define APP_SHM_MAX_FILE_NAME_LEN 40

typedef struct app_shm_t
{
   char name[APP_SHM_MAX_FILE_NAME_LEN];
   int fd;
   caddr_t mem_address;
   sem_t * sem;
   size_t size;
} app_shm_t;

/**
 * Format name string
 *
 * Replace space " " with "-"
 * Convert uppercase characters to lowercase.
 * @param name  In:  NULL terminated string
 * @return 0 on success, -1 on error
 **/
static int format_name (char * name)
{
   uint32_t i;

   if (name == NULL)
   {
      return -1;
   }

   for (i = 0; name[i] != '\0'; i++)
   {
      name[i] = tolower (name[i]);
      if (name[i] == ' ')
      {
         name[i] = '_';
      }
   }

   return 0;
}

/**
 * Initialise an existing shared memory area handle
 *
 * @param handle     In:    Handle
 * @param name       In:    Name
 * @param slot       In:    Slot number
 * @param subslot    In:    Subslot number
 * @param size       In:    Size in bytes
 * @param is_output  In:    True if the shared memory area is an output
 * @return 0 on success, -1 on error
 **/
static int app_shm_init (
   app_shm_t * handle,
   const char * name,
   uint32_t slot,
   uint32_t subslot,
   uint32_t size,
   bool is_output)
{
   const int shm_flags = O_RDWR | O_CREAT;

   if (handle == NULL || name == NULL || size == 0)
   {
      return -1;
   }
   memset (handle, 0, sizeof (app_shm_t));

   /* Only include slot and subslot numbers if both module and
      submodules are fixed in slot and subslot */
   if (slot != UINT32_MAX && subslot != UINT32_MAX)
   {
      snprintf (
         handle->name,
         APP_SHM_MAX_FILE_NAME_LEN - 1,
         "pnet-%s-%d-%d-%s",
         is_output ? "out" : "in",
         slot,
         subslot,
         name);
   }
   else
   {
      snprintf (
         handle->name,
         APP_SHM_MAX_FILE_NAME_LEN - 1,
         "pnet-%s-%s",
         is_output ? "out" : "in",
         name);
   }

   format_name (handle->name);

   handle->size = size;

   handle->fd = shm_open (handle->name, shm_flags, 0666);

   if (handle->fd < 0)
   {
      APP_LOG_ERROR (
         "Failed to open shared memory\n"
         "  file name: %s\n"
         "  Input files is expected to be created by data generating "
         "  application\n",
         handle->name);

      return -1;
   }

   if (ftruncate (handle->fd, handle->size) != 0)
   {
      return -1;
   }

   handle->mem_address = mmap (
      NULL,
      handle->size,
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      handle->fd,
      0);

   if (handle->mem_address == MAP_FAILED)
   {
      return -1;
   }

   handle->sem = sem_open (handle->name, O_CREAT, 0666, 1);

   if (handle->sem == NULL)
   {
      return -1;
   }

   /* sem_unlink (handle->name); TODO */
   APP_LOG_DEBUG ("shared memory area created\n");
   APP_LOG_DEBUG ("  name=%s\n", handle->name);
   APP_LOG_DEBUG ("  size=%lu\n", handle->size);
   APP_LOG_DEBUG ("  backing file = /dev/shm/%s\n", handle->name);

   return 0;
}

app_shm_t * app_shm_create_input (
   const char * name,
   int slot,
   int subslot,
   int size)
{
   app_shm_t * handle = (app_shm_t *)malloc (sizeof (app_shm_t));

   if (handle != NULL)
   {
      if (app_shm_init (handle, name, slot, subslot, size, false) != 0)
      {
         app_shm_destroy (handle);
         handle = NULL;
      }
   }

   return handle;
}

app_shm_t * app_shm_create_output (
   const char * name,
   int slot,
   int subslot,
   int size)
{
   app_shm_t * handle = (app_shm_t *)malloc (sizeof (app_shm_t));

   if (handle != NULL)
   {
      if (app_shm_init (handle, name, slot, subslot, size, true) != 0)
      {
         app_shm_destroy (handle);
         handle = NULL;
      }
   }

   return handle;
}

/**
 * Free resources related to a shared memory area.
 *
 * Does not free the memory allocated for the handle itself.
 *
 * @param handle     In:    Handle
 **/
static void app_shm_free_resources (app_shm_t * handle)
{
   if (handle == NULL)
   {
      return;
   }

   if (handle->mem_address != 0)
   {
      munmap (handle->mem_address, handle->size);
   }

   if (handle->fd >= 0)
   {
      close (handle->fd);
   }

   if (handle->sem != NULL)
   {
      sem_close (handle->sem);
      /* sem_unlink TODO */
   }

   unlink (handle->name);
}

void app_shm_destroy (app_shm_t * handle)
{
   if (handle != NULL)
   {
      app_shm_free_resources (handle);
      free (handle);
   }
}

int app_shm_read (app_shm_t * handle, void * data, size_t size)
{
   if (handle == NULL || size == 0)
   {
      return -1;
   }

   if (sem_wait (handle->sem) != 0)
   {
      return -1;
   }

   memcpy (data, handle->mem_address, size);

   if (sem_post (handle->sem) != 0)
   {
      return -1;
   }

   return 0;
}

int app_shm_write (app_shm_t * handle, void * data, size_t size)
{
   if (handle == NULL || data == NULL || size > handle->size)
   {
      return -1;
   }

   if (sem_wait (handle->sem) != 0)
   {
      return -1;
   }

   memcpy (handle->mem_address, data, size);

   if (sem_post (handle->sem) != 0)
   {
      return -1;
   }

   return 0;
}

int app_shm_reset (app_shm_t * handle)
{
   if (handle == NULL)
   {
      return -1;
   }

   if (sem_wait (handle->sem) != 0)
   {
      return -1;
   }

   memset (handle->mem_address, 0, handle->size);

   if (sem_post (handle->sem) != 0)
   {
      return -1;
   }

   return 0;
}
