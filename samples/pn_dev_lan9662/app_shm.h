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

#ifndef APP_SHM_H
#define APP_SHM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_shm_t app_shm_t;

/**
 * Create a shared memory area used as input.
 *
 * Shared memory is read by this application.
 * Intended for submodule input data.
 * The slot and and subslot info is used to
 * generate the name of the shared memory area.
 *
 * @param name       In:  Submodule id
 * @param slot       In:  Slot number
 * @param subslot    In:  Subslot number
 * @param size       In:  Size of area
 * @return Handle to created shared memory area. NULL on error.
 */
app_shm_t * app_shm_create_input (
   const char * name,
   int slot,
   int subslot,
   int size);

/**
 * Create a shared memory area used as output.
 *
 * Shared memory is written by this application.
 * Intended for submodule output data.
 * The slot and and subslot info is used to
 * generate the name of the shared memory area.
 *
 * @param name       In:  Submodule id
 * @param slot       In:  Slot number
 * @param subslot    In:  Subslot number
 * @param size       In:  Size of area
 * @return Handle to created shared memory area. NULL on error.
 */
app_shm_t * app_shm_create_output (
   const char * name,
   int slot,
   int subslot,
   int size);

/**
 * Free the resources of a shared memory area
 *
 * @param handle       In:  Shared memory object handle
 */
void app_shm_destroy (app_shm_t * handle);

/**
 * Read shared memory area into a buffer
 *
 * @param handle     In:  Shared memory object handle
 * @param data       In:  Start of buffer
 * @param size       In:  Number of bytes to read
 * @return 0 on success, -1 on error
 */
int app_shm_read (app_shm_t * handle, void * data, size_t size);

/**
 * Write to shared memory
 *
 * @param handle     In:  Shared memory object handle
 * @param data       In:  Start of buffer
 * @param size       In:  Number of bytes to write
 * @return 0 on success, -1 on error
 */
int app_shm_write (app_shm_t * handle, void * data, size_t size);

/**
 * Set shared memory to zero
 *
 * @param handle     In:  Shared memory object handle
 * @return 0 on success, -1 on error
 */
int app_shm_reset (app_shm_t * handle);

#endif /* APP_SHM_H */
