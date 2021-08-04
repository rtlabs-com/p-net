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

#ifndef PNAL_FILETOOLS_H
#define PNAL_FILETOOLS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* Colon separated paths to search for scripts. No colon at end. */
#define PNAL_DEFAULT_SEARCHPATH "/bin:/usr/bin"

/**
 * Check if a file or directory exists
 *
 * @param filepath         In:    Path to file or directory. Null terminated.
 *                                Trailing slash is optional for directories.
 * @return true if file exists
 */
bool pnal_does_file_exist (const char * filepath);

/**
 * Execute a script.
 *
 * The script will be searched for in:
 * - Directories given in PNAL_DEFAULT_SEARCHPATH
 * - Same directory as the main binary is located in.
 *
 * @param argv             In:    Arguments (null terminated strings).
 *                                The first argument should be the name of
 *                                the script.
 *                                The last element should be NULL.
 * @return 0 on success
 *         -1 if an error occurred
 */
int pnal_execute_script (const char * argv[]);

#ifdef __cplusplus
}
#endif

#endif /* PNAL_FILETOOLS_H */
