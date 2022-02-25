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

#include "utils_for_testing.h"
#include "mocks.h"

#include "pf_includes.h"

#include <gtest/gtest.h>

#define TEST_FILE_DATA_SIZE         10
#define TEST_FILE_FILENAME          "my_filename.bin"
#define TEST_FILE_DIRECTORY         "/my_directory/"
#define TEST_FILE_OUTPUTSTRING_SIZE 40

class FileUnitTest : public PnetUnitTest
{
};

TEST_F (FileUnitTest, FileJoinDirectoryFilename)
{
   char outputstring[TEST_FILE_OUTPUTSTRING_SIZE] = {0}; /** Terminated */
   int res = 0;

   /* Happy case */
   res = pf_file_join_directory_filename (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, TEST_FILE_DIRECTORY TEST_FILE_FILENAME), 0);

   /* Missing filename or outputbuffer */
   res = pf_file_join_directory_filename (
      TEST_FILE_DIRECTORY,
      NULL,
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, -1);
   res = pf_file_join_directory_filename (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      NULL,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, -1);

   /* Too small outputbuffer */
   res = pf_file_join_directory_filename (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      outputstring,
      0);
   EXPECT_EQ (res, -1);
   res = pf_file_join_directory_filename (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      outputstring,
      4);
   EXPECT_EQ (res, -1);

   /* Too short filename */
   res = pf_file_join_directory_filename (
      TEST_FILE_DIRECTORY,
      "",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, -1);

   /* Joining directory and filename */
   res = pf_file_join_directory_filename (
      "abc",
      "def",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "abc/def"), 0);

   res = pf_file_join_directory_filename (
      "abc/",
      "def",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "abc/def"), 0);

   res = pf_file_join_directory_filename (
      "abc/",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "abc/def.ghi"), 0);

   res = pf_file_join_directory_filename (
      "/abc/",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "/abc/def.ghi"), 0);

   res = pf_file_join_directory_filename (
      "/",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "/def.ghi"), 0);

   res = pf_file_join_directory_filename (
      "a",
      "d",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "a/d"), 0);

   res = pf_file_join_directory_filename (
      "a",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "a/def.ghi"), 0);

   res = pf_file_join_directory_filename (
      ".",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "./def.ghi"), 0);

   res = pf_file_join_directory_filename (
      "..",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "../def.ghi"), 0);

   res = pf_file_join_directory_filename (
      "/",
      "d",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "/d"), 0);

   /* No directory given (NULL or empty string) */
   res = pf_file_join_directory_filename (
      NULL,
      "d",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "d"), 0);

   res = pf_file_join_directory_filename (
      "",
      "d",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "d"), 0);

   res = pf_file_join_directory_filename (
      NULL,
      "def",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "def"), 0);

   res = pf_file_join_directory_filename (
      "",
      "def",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "def"), 0);

   res = pf_file_join_directory_filename (
      NULL,
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "def.ghi"), 0);

   res = pf_file_join_directory_filename (
      "",
      "def.ghi",
      outputstring,
      TEST_FILE_OUTPUTSTRING_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (strcmp (outputstring, "def.ghi"), 0);
}

TEST_F (FileUnitTest, FileCheckMagicAndVersion)
{
   uint8_t testdata[TEST_FILE_DATA_SIZE] =
      {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'};
   uint8_t retrieved[TEST_FILE_DATA_SIZE] = {0};
   uint8_t temporary_byte;
   int res = 0;
   int i;

   /* Save data together with file version information */
   res = pf_file_save (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &testdata,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (mock_os_data.file_size, TEST_FILE_DATA_SIZE + 8); /* Implementation
                                                                   detail, but
                                                                   verifies mock
                                                                   functionality
                                                                 */

   /* Save: Validate mock (protection of max mocked file size) */
   res =
      pf_file_save (TEST_FILE_DIRECTORY, TEST_FILE_FILENAME, &testdata, 10000);
   EXPECT_EQ (res, -1);

   /* Load: Non-existent file */
   res = pf_file_load (
      TEST_FILE_DIRECTORY,
      "unknown_filename",
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, -1);

   /* Retrieve data */
   res = pf_file_load (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);
   for (i = 0; i < TEST_FILE_DATA_SIZE; i++)
   {
      EXPECT_EQ (testdata[i], retrieved[i]);
   }

   /* Invalid magic bytes in simulated file */
   temporary_byte = mock_os_data.file_content[0];
   mock_os_data.file_content[0] = 'A';
   res = pf_file_load (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, -1);
   mock_os_data.file_content[0] = temporary_byte; /* Reset to initial value */

   /* Invalid file version in simulated file */
   temporary_byte = mock_os_data.file_content[4];
   mock_os_data.file_content[4] = 200;
   res = pf_file_load (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, -1);
   mock_os_data.file_content[4] = temporary_byte; /* Reset to initial value */

   /* Verify that mock does not delete file for other name */
   pf_file_clear (TEST_FILE_DIRECTORY, "nonexistent file");
   EXPECT_GT (mock_os_data.file_size, 1);

   /* Clear: Invalid directory */
   pf_file_clear (NULL, TEST_FILE_FILENAME);
   pf_file_clear ("", TEST_FILE_FILENAME);

   /* Check that it is OK when we use correct sample data again */
   res = pf_file_load (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);

   /* Verify that the mock can delete file entry */
   pf_file_clear (TEST_FILE_DIRECTORY, TEST_FILE_FILENAME);
   EXPECT_EQ (mock_os_data.file_size, 0); /* Verifies mock functionality */
   res = pf_file_load (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, -1);

   /* Save to current directory */
   res =
      pf_file_save (NULL, TEST_FILE_FILENAME, &testdata, TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);
   EXPECT_EQ (mock_os_data.file_size, TEST_FILE_DATA_SIZE + 8); /* Implementation
                                                                   detail, but
                                                                   verifies mock
                                                                   functionality
                                                                 */

   /* Retrieve data from current directory */
   res =
      pf_file_load (NULL, TEST_FILE_FILENAME, &retrieved, TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);
   for (i = 0; i < TEST_FILE_DATA_SIZE; i++)
   {
      EXPECT_EQ (testdata[i], retrieved[i]);
   }

   res = pf_file_load ("", TEST_FILE_FILENAME, &retrieved, TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);
   for (i = 0; i < TEST_FILE_DATA_SIZE; i++)
   {
      EXPECT_EQ (testdata[i], retrieved[i]);
   }

   /* Delete file in current directory */
   pf_file_clear (NULL, TEST_FILE_FILENAME);
   EXPECT_EQ (mock_os_data.file_size, 0); /* Verifies mock functionality */
   res =
      pf_file_load (NULL, TEST_FILE_FILENAME, &retrieved, TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, -1);
}

TEST_F (FileUnitTest, FileCheckSaveIfModified)
{
   uint8_t testdata[TEST_FILE_DATA_SIZE] =
      {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'};
   uint8_t tempobject[TEST_FILE_DATA_SIZE]; /* Not initialized on purpose */
   uint8_t retrieved[TEST_FILE_DATA_SIZE] = {0};
   int res = 0;
   int i;

   pf_file_clear (TEST_FILE_DIRECTORY, TEST_FILE_FILENAME);

   /* First saving */
   res = pf_file_save_if_modified (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &testdata,
      &tempobject,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 2);

   /* No update */
   res = pf_file_save_if_modified (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &testdata,
      &tempobject,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);

   res = pf_file_save_if_modified (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &testdata,
      &tempobject,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);

   /* Updated data */
   testdata[0] = 's';
   res = pf_file_save_if_modified (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &testdata,
      &tempobject,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 1);

   res = pf_file_save_if_modified (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &testdata,
      &tempobject,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);

   /* Verify updated data */
   res = pf_file_load (
      TEST_FILE_DIRECTORY,
      TEST_FILE_FILENAME,
      &retrieved,
      TEST_FILE_DATA_SIZE);
   EXPECT_EQ (res, 0);

   EXPECT_EQ (retrieved[0], 's');
   for (i = 1; i < TEST_FILE_DATA_SIZE; i++)
   {
      EXPECT_EQ (retrieved[i], testdata[i]);
   }
}
