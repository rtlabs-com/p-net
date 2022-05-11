// Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
// SPDX-License-Identifier: MIT

/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 *
 ********************************************************************/

/**
 * SRAM UIO driver adapted for P-Net.
 * Implements read and write operations as well as
 * a allocation/frame concept.
 * Based on code in mera demo.
 */

#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <endian.h>

#include "microchip/ethernet/rte/api.h"
#include "pf_includes.h"

#include "pf_sram_uio.h"

#if (__BYTE_ORDER == __BIG_ENDIAN)
#define HOST_CVT(x) (x)
#else
#define HOST_CVT(x) __builtin_bswap32 ((x)) /* PCIe is LE, so swap */
#endif

/* Do not start on address 0 - 0 is used "NULL"*/
#define PF_SRAM_AREA_START_ADDRESS 100
#define PF_SRAM_FRAME_SIZE         1600
#define PF_SRAM_MAX_FRAMES         4
#define PF_SRAM_MAX_ADDRESS        0x1FFFF

typedef union pf_uio_reg
{
   uint32_t val;
   uint8_t data[4];
} pf_uio_reg_t;

typedef struct pf_sram_frame
{
   bool used;
   uint16_t start_address;
} pf_sram_frame_t;

typedef struct pf_sram_uio
{
   int dev_fd;
   volatile uint32_t * base_mem;
   pf_sram_frame_t frames[PF_SRAM_MAX_FRAMES];
} pf_sram_uio_t;

pf_sram_uio_t sram_uio = {0};

static int uio_init (pf_sram_uio_t * handle)
{
   const char * driver = "mscc_sram";
   const char * top = "/sys/class/uio";
   DIR * dir;
   struct dirent * dent;
   char fn[PATH_MAX], devname[128];
   FILE * fp;
   char iodev[512];
   size_t mapsize;
   int len, rc = -1;

   handle->dev_fd = -1;

   if (!(dir = opendir (top)))
   {
      LOG_ERROR (PF_PPM_LOG, "SRAM UIO - opendir(%s) failed", top);
      return rc;
   }

   while ((dent = readdir (dir)) != NULL)
   {
      if (dent->d_name[0] == '.')
      {
         continue;
      }

      snprintf (fn, sizeof (fn), "%s/%s/name", top, dent->d_name);
      fp = fopen (fn, "r");
      if (!fp)
      {
         LOG_ERROR (PF_PPM_LOG, "SRAM UIO -  Failed to open: %s", fn);
         continue;
      }

      const char * rrd = fgets (devname, sizeof (devname), fp);
      fclose (fp);

      if (!rrd)
      {
         LOG_ERROR (PF_PPM_LOG, "SRAM UIO - Failed to read: %s", fn);
         continue;
      }

      len = strlen (devname);
      if (len > 0 && devname[len - 1] == '\n')
      {
         devname[len - 1] = '\0';
      }

      if (!strstr (devname, driver))
      {
         continue;
      }

      snprintf (iodev, sizeof (iodev), "/dev/%s", dent->d_name);
      snprintf (fn, sizeof (fn), "%s/%s/maps/map0/size", top, dent->d_name);
      fp = fopen (fn, "r");
      if (!fp)
      {
         continue;
      }

      if (fscanf (fp, "%zi", &mapsize))
      {
         fclose (fp);
         rc = 0;
         LOG_DEBUG (PF_PPM_LOG, "SRAM UIO - Using device: %s\n", devname);
         break;
      }
      fclose (fp);
   }
   closedir (dir);

   if (rc < 0)
   {
      LOG_ERROR (PF_PPM_LOG, "SRAM UIO - No suitable UIO device found!\n");
      return rc;
   }

   /* Open the UIO device file */
   LOG_INFO (
      PF_PPM_LOG,
      "SRAM UIO - found '%s' driver at %s, size: %zu\n",
      driver,
      iodev,
      mapsize);
   handle->dev_fd = open (iodev, O_RDWR);
   if (handle->dev_fd < 1)
   {
      LOG_INFO (PF_PPM_LOG, "SRAM UIO - open(%s) failed\n", iodev);
      rc = -1;
   }
   else
   {
      /* mmap the UIO device */
      handle->base_mem = mmap (
         NULL,
         mapsize,
         PROT_READ | PROT_WRITE,
         MAP_SHARED,
         handle->dev_fd,
         0);
      if (handle->base_mem != MAP_FAILED)
      {
         LOG_DEBUG (
            PF_PPM_LOG,
            "SRAM UIO - Mapped register memory @ %p\n",
            handle->base_mem);
      }
      else
      {
         LOG_ERROR (PF_PPM_LOG, "SRAM UIO - mmap failed\n");
         rc = -1;
      }
   }
   return rc;
}

int pf_sram_init (void)
{
   uint16_t i;

   for (i = 0; i < PF_SRAM_MAX_FRAMES; i++)
   {
      sram_uio.frames[i].used = false;
      sram_uio.frames[i].start_address =
         PF_SRAM_AREA_START_ADDRESS + i * PF_SRAM_FRAME_SIZE;
   }

   if (uio_init (&sram_uio) != 0)
   {
      return -1;
   }

   return 0;
}

uint16_t pf_sram_frame_alloc (void)
{
   uint16_t i;

   for (i = 0; i < PF_SRAM_MAX_FRAMES; i++)
   {
      if (sram_uio.frames[i].used == false)
      {
         sram_uio.frames[i].used = true;
         return sram_uio.frames[i].start_address;
      }
   }

   return PF_SRAM_INVALID_ADDRESS;
}

void pf_sram_frame_free (uint16_t frame_start_address)
{
   uint16_t i;

   for (i = 0; i < PF_SRAM_MAX_FRAMES; i++)
   {
      if (sram_uio.frames[i].start_address == frame_start_address)
      {
         sram_uio.frames[i].used = false;
         return;
      }
   }

   LOG_ERROR (
      PF_PPM_LOG,
      "SRAM UIO - failed to free frame at address 0x%x\n",
      frame_start_address);
}

static int reg_check (uint32_t addr)
{
   if (addr >= PF_SRAM_MAX_ADDRESS)
   {
      LOG_ERROR (PF_PPM_LOG, "SRAM UIO - address range is 17 bits\n");
      return -1;
   }
   if (addr & 3)
   {
      LOG_ERROR (PF_PPM_LOG, "SRAM UIO - address must be 32-bit word aligned\n");
      return -1;
   }
   return 0;
}

static int reg_read (uint32_t addr, uint32_t * val)
{
   if (reg_check (addr) < 0)
   {
      return -1;
   }
   *val = HOST_CVT (sram_uio.base_mem[addr / 4]);
   return 0;
}

static int reg_write (uint32_t addr, uint32_t val)
{
   if (reg_check (addr) < 0)
   {
      return -1;
   }
   sram_uio.base_mem[addr / 4] = HOST_CVT (val);
   return 0;
}

int pf_sram_write (uint16_t address, const uint8_t * data, uint16_t length)
{
   pf_uio_reg_t tmp;
   uint32_t uio_reg_addr = address & 0xFFFFFFFC;
   uint32_t offset;
   uint32_t index = 0;
   uint32_t i = 0;

   offset = address - uio_reg_addr;
   while (index < length)
   {
      if (reg_read (uio_reg_addr, &tmp.val) != 0)
      {
         return -1;
      }

      for (i = offset; i <= 3; i++)
      {
         if (index < length)
         {
            tmp.data[3 - i] = data[index++];
         }
      }

      if (reg_write (uio_reg_addr, tmp.val) != 0)
      {
         return -1;
      }

      uio_reg_addr += 4;
      offset = 0;
   }
   return 0;
}

int pf_sram_read (uint16_t address, uint8_t * data, uint16_t length)
{
   pf_uio_reg_t tmp;
   uint32_t uio_reg_addr = address & 0xFFFFFFFC;
   uint32_t offset;
   uint32_t index = 0;
   uint32_t i = 0;

   offset = address - uio_reg_addr;
   while (index < length)
   {
      if (reg_read (uio_reg_addr, &tmp.val) != 0)
      {
         return -1;
      }

      for (i = offset; i <= 3; i++)
      {
         if (index < length)
         {
            data[index++] = tmp.data[3 - i];
         }
      }

      uio_reg_addr += 4;
      offset = 0;
   }
   return 0;
}