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

#ifndef PF_FILE_H
#define PF_FILE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

/**
 * Load a binary file, and verify the file version.
 *
 * @param directory        In:    Directory for files. Terminated string. NULL
 *                                or empty string is interpreted as current directory.
 * @param filename         In:    File name
 * @param object           Out:   Struct to load
 * @param size             In:    Size of struct to load
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred.
 */
int pf_file_load(
   const char              *directory,
   const char              *filename,
   void                    *object,
   size_t                  size
);

/**
 * Save a binary file, and include version information.
 *
 * @param directory        In:    Directory for files. Terminated string. NULL
 *                                or empty string is interpreted as current directory.
 * @param filename         In:    File name
 * @param object           In:    Struct to save
 * @param size             In:    Size of struct to save
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_file_save(
   const char              *directory,
   const char              *filename,
   void                    *object,
   size_t                  size
);

/**
 * Clear a binary file.
 *
 * @param directory        In:    Directory for files. Terminated string. NULL
 *                                or empty string is interpreted as current directory.
 * @param filename         In:    File name
 */
void pf_file_clear(
   const char              *directory,
   const char              *filename
);


/************ Internal functions, made available for unit testing ************/

int pf_file_join_directory_filename(
   const char              *directory,
   const char              *filename,
   char                    *fullpath,
   size_t                  size
);


#ifdef __cplusplus
}
#endif

#endif /* PF_FILE_H */