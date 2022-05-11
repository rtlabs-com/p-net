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

/* Linux application intended to be used
 * to read and write shared memory areas supported
 * by the lan9662 sample application.
 * See show_usage() for a functional description.
 */

#include <getopt.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_FILE_NAME    200
#define MAX_STDIO_BUFFER 10000

#define DEBUG_LOG (1)
#if DEBUG_LOG == 1
#define LOG_ERR(...)                                                           \
   fprintf (stderr, "Line:%d ", __LINE__);                                     \
   fprintf (stderr, __VA_ARGS__);
#else
#define LOG_ERR(...)
#endif

typedef struct app
{
   enum
   {
      UNDEFINED = 0,
      READ,
      WRITE,
      CREATE,
      READ_BIT
   } mode;

   char name[MAX_FILE_NAME]; /* Shared memory file name*/
   int fd;                   /* Shared memory file descriptor */
   caddr_t mem_address;      /* Address of mapped shared memory */
   sem_t * sem;              /* Named semaphore */
   size_t shm_size;          /* The size of an existing shared memory area */
   size_t create_size;       /* Size argument used when creating a shared memory
                                area*/
   uint32_t bitnumber;       /* Bit number to read */
   uint8_t buffer[MAX_STDIO_BUFFER];
} app_t;

static void show_usage (void)
{
   printf ("\nTool for accessing shared memory files\n");
   printf ("Intended to be used in combination with the \n"
           "pn_lan9662 sample application. \n");
   printf ("Arguments:\n");
   printf ("   -h             Show help and exit\n");
   printf (
      "   -r <shm_name>  Read shared memory and write content to stdout \n");
   printf ("   -w <shm_name>  Write content on stdin to shared memory \n");
   printf ("   -c <shm_name>  Create shared memory area\n");
   printf ("   -s <size>      Size of shared memory, only used with create\n");
   printf ("   -b <shm_name>  Read bit from shared memory. Write '1' or '0' "
           "to stdout\n");
   printf (
      "   -n <bitnumber> Bit number, only used with read bit. Defaults to\n");
   printf (
      "                  bit number 0 (least significant bit in first byte)\n");
   printf (
      "Partial write is allowed. Offsets are not supported. If write data\n");
   printf ("is larger than shared memory, data is truncated.\n");

   printf ("\nExample usage:\n");
   printf ("Read slot and passing data to hexdump:\n");
   printf ("pn_shm_tool -r pnet-in-1-1-digital_input_1x8 | hexdump -C\n");
   printf ("Write slot:\n");
   printf ("echo -n -e \"\\x02\" | pn_shm_tool -w "
           "pnet-in-1-1-digital_input_1x8\n");
   printf ("Create 4 bytes shared memory area (for testing this tool):\n");
   printf ("pn_shm_tool -c test_shm -s 4\n");
}

/**
 * Open an existing shared memory area
 * figure out its size and map it.
 * @param   app      InOut: Application context
 * @return 0 on success, -1 on error
 */
int open_shm (app_t * app)
{
   struct stat st;

   if (app->mode == CREATE)
   {
      return -1;
   }

   app->fd = shm_open (app->name, O_RDWR, 0666);

   if (app->fd < 0)
   {
      LOG_ERR ("\nFailed to open %s %s\n", app->name, strerror (errno));
      return -1;
   }

   if (fstat (app->fd, &st))
   {
      LOG_ERR ("\nfstat error: [%s]\n", strerror (errno));
      close (app->fd);
      return -1;
   }

   app->shm_size = st.st_size;

   if (ftruncate (app->fd, app->shm_size) != 0)
   {
      return -1;
   }

   app->mem_address =
      mmap (NULL, app->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, app->fd, 0);

   if (app->mem_address == MAP_FAILED)
   {
      LOG_ERR ("\nmmap error: [%s]\n", strerror (errno));
      return -1;
   }

   app->sem = sem_open (app->name, 0, 0644, 1);

   if (app->sem == NULL)
   {
      LOG_ERR ("\nFailed to open semaphore: [%s]\n", strerror (errno));
      return -1;
   }

   return 0;
}

/**
 * Create a new shared memory area
 * @param   app      InOut: Application context
 * @return 0 on success, -1 on error
 */
int create_shm (app_t * app)
{
   if (app->mode != CREATE)
   {
      return -1;
   }

   app->fd = shm_open (app->name, O_RDWR | O_CREAT, 0666);

   if (app->fd < 0)
   {
      LOG_ERR ("\nFailed to create %s %s\n", app->name, strerror (errno));
      return -1;
   }

   if (ftruncate (app->fd, app->create_size) != 0)
   {
      return -1;
   }

   app->mem_address = mmap (
      NULL,
      app->create_size,
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      app->fd,
      0);

   if (app->mem_address == MAP_FAILED)
   {
      LOG_ERR ("\nmmap error: [%s]\n", strerror (errno));
      return -1;
   }

   app->sem = sem_open (app->name, O_CREAT, 0644, 1);

   if (app->sem == NULL)
   {
      LOG_ERR ("\nFailed to open semaphore: [%s]\n", strerror (errno));
      return -1;
   }

   return 0;
}

/**
 * Close a shared memory area and its
 * open resources.
 * @param   app      InOut: Application context
 * @return 0 always
 */
int close_shm (app_t * app)
{
   if (app->mem_address != 0)
   {
      munmap (app->mem_address, app->shm_size);
      app->mem_address = 0;
   }

   if (app->fd >= 0)
   {
      close (app->fd);
      app->fd = -1;
   }

   if (app->sem != NULL)
   {
      sem_close (app->sem);
      app->sem = NULL;
   }

   return 0;
}

/**
 * Read a shared memory area and print its content
 * to STDOUT.
 * @param   app      InOut: Application context
 * @return 0 on success, -1 on error
 */
int read_shm (app_t * app)
{
   int write_len;

   if (sem_wait (app->sem) != 0)
   {
      LOG_ERR ("\nFailed to take semaphore: [%s]\n", strerror (errno));
      return -1;
   }

   memcpy (app->buffer, app->mem_address, app->shm_size);

   if (sem_post (app->sem) != 0)
   {
      LOG_ERR ("\nFailed to release semaphore: [%s]\n", strerror (errno));
      return -1;
   }

   write_len = write (STDOUT_FILENO, app->buffer, app->shm_size);
   if (write_len != (int)app->shm_size)
   {
      LOG_ERR ("\nFailed to write stdout\n");
      return -1;
   }

   return 0;
}

/**
 * Read a bit from shared memory area and print "0" or "1" to STDOUT.
 * @param   app      InOut: Application context
 * @return 0 on success, -1 on error
 */
int read_bit_shm (app_t * app)
{
   int write_len = 0;
   uint32_t byte_number = 0;
   uint8_t bitnumber_in_byte = 0;
   uint8_t byte_value = 0;
   uint8_t bit_value = 0;

   if (app->shm_size == 0)
   {
      LOG_ERR ("\nShared memory size is zero\n");
      return -1;
   }

   if (app->bitnumber >= MAX_STDIO_BUFFER * 8)
   {
      LOG_ERR (
         "\nToo large bitnumber. Given: %u (Buffer size %u bytes)\n",
         app->bitnumber,
         MAX_STDIO_BUFFER);
      return -1;
   }

   if (app->bitnumber >= app->shm_size * 8)
   {
      LOG_ERR (
         "\nToo large bitnumber. Given: %u (Memory size %zu bytes)\n",
         app->bitnumber,
         app->shm_size);
      return -1;
   }

   if (sem_wait (app->sem) != 0)
   {
      LOG_ERR ("\nFailed to take semaphore: [%s]\n", strerror (errno));
      return -1;
   }

   memcpy (app->buffer, app->mem_address, app->shm_size);

   if (sem_post (app->sem) != 0)
   {
      LOG_ERR ("\nFailed to release semaphore: [%s]\n", strerror (errno));
      return -1;
   }

   byte_number = app->bitnumber / 8; /* byte_number 0 is first */
   byte_value = app->buffer[byte_number];
   bitnumber_in_byte = app->bitnumber % 8;
   bit_value = (byte_value >> bitnumber_in_byte) & 0x01;

   write_len = write (STDOUT_FILENO, bit_value ? "1\n" : "0\n", 2);
   if (write_len != 2)
   {
      LOG_ERR ("\nFailed to write stdout\n");
      return -1;
   }

   return 0;
}

/**
 * Read STDIN and copy the data to shared memory area.
 * @param   app      InOut: Application context
 * @return 0 on success, -1 on error
 */
int write_shm (app_t * app)
{
   int len = read (STDIN_FILENO, app->buffer, app->shm_size);
   if (len > (int)app->shm_size)
   {
      /* Truncate len of data to match shared memory */
      len = app->shm_size;
   }

   if (len <= 0)
   {
      LOG_ERR ("\nFailed to read stdin: [%s]\n", strerror (errno));
      return -1;
   }

   if (sem_wait (app->sem) != 0)
   {
      LOG_ERR ("\nFailed to take semaphore: [%s]\n", strerror (errno));
      return -1;
   }

   memcpy (app->mem_address, app->buffer, len);

   if (sem_post (app->sem) != 0)
   {
      LOG_ERR ("\nFailed to release semaphore: [%s]\n", strerror (errno));
      return -1;
   }

   return 0;
}

/**
 * Execute a command given the configuration
 * set in the app variable.
 * Creation, read and write of shared memory regions
 * are supported.
 * @param   app      InOut: Application context
 * @return 0 on success, -1 on error
 */
static int run (app_t * app)
{
   int error = 0;

   if (app->mode == CREATE)
   {
      error = create_shm (app);
      close_shm (app);
      return error;
   }

   error = open_shm (app);
   if (!error)
   {
      if (app->mode == READ)
      {
         error = read_shm (app);
      }
      else if (app->mode == READ_BIT)
      {
         error = read_bit_shm (app);
      }
      else if (app->mode == WRITE)
      {
         error = write_shm (app);
      }
      error |= close_shm (app);
   }
   return error;
}

/**
 * Parse command line arguments and execute selected command.
 * @param argc  In:  Number of args
 * @param argv  In:  Arguments
 * @return 0 on success, 1 on error
 */
int main (int argc, char * argv[])
{
   int error = 0;
   int option = 0;
   app_t app = {0};

   if (argc < 2)
   {
      show_usage();
      exit (EXIT_FAILURE);
   }

   while ((option = getopt (argc, argv, "hw:r:c:s:b:n:")) != -1)
   {
      switch (option)
      {
      case 'r':
         if (app.mode != UNDEFINED)
         {
            show_usage();
            printf ("\nOnly one operation allowed\n");
            exit (EXIT_FAILURE);
         }
         strncpy (app.name, optarg, MAX_FILE_NAME - 1);
         app.mode = READ;
         break;
      case 'b':
         if (app.mode != UNDEFINED)
         {
            show_usage();
            printf ("\nOnly one operation allowed\n");
            exit (EXIT_FAILURE);
         }
         strncpy (app.name, optarg, MAX_FILE_NAME - 1);
         app.mode = READ_BIT;
         break;
      case 'w':
         if (app.mode != UNDEFINED)
         {
            show_usage();
            printf ("\nOnly one operation allowed\n");
            exit (EXIT_FAILURE);
         }
         strncpy (app.name, optarg, MAX_FILE_NAME - 1);
         app.mode = WRITE;
         break;
      case 'c':
         if (app.mode != UNDEFINED)
         {
            show_usage();
            printf ("\nOnly one operation allowed\n");
            exit (EXIT_FAILURE);
         }
         strncpy (app.name, optarg, MAX_FILE_NAME - 1);
         app.mode = CREATE;
         break;
      case 's':
         app.create_size = atoi (optarg);
         break;
      case 'n':
         app.bitnumber = atoi (optarg);
         break;
      case 'h':
         show_usage();
         exit (EXIT_SUCCESS);
      default:
         show_usage();
         exit (EXIT_FAILURE);
      }
   }
   if (app.mode == CREATE)
   {
      if (app.create_size <= 0 || app.create_size > MAX_STDIO_BUFFER)
      {
         show_usage();
         printf ("\nSize must be set when creating shared mem area\n");
         printf ("Min size 1, Max size %d\n", MAX_STDIO_BUFFER);
         exit (EXIT_FAILURE);
      }
   }

   error = run (&app);
   if (error != 0)
   {
      fprintf (stderr, "Operation failed.\n");
      exit (EXIT_FAILURE);
   }

   exit (EXIT_SUCCESS);
}
