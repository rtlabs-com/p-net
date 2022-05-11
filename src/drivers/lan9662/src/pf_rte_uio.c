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
 * RTE UIO driver adapted for P-Net.
 * Implements register read and write operations.
 * Based on code in mera demo.
 */

#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/i2c.h>     /* I2C support */
#include <linux/i2c-dev.h> /* I2C support */
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <endian.h>

#include "microchip/ethernet/rte/api.h"
#include "pf_includes.h"

#define T_D(...) LOG_DEBUG (PNET_LOG, __VA_ARGS__)
#define T_I(...) LOG_INFO (PNET_LOG, __VA_ARGS__)
#define T_E(...) LOG_ERROR (PNET_LOG, __VA_ARGS__)

#if (__BYTE_ORDER == __BIG_ENDIAN)
#define PCIE_HOST_CVT(x)                                                       \
   __builtin_bswap32 ((x)) /* PCIe is LE - we're BE, so swap */
#else
#define PCIE_HOST_CVT(x) (x) /* We're LE already */
#endif

volatile uint32_t * base_mem = NULL;

int pf_rte_uio_reg_read (
   struct mera_inst * inst,
   const uintptr_t addr,
   uint32_t * data)
{
   if (base_mem != NULL)
   {
      *data = PCIE_HOST_CVT (base_mem[addr]);
      return 0;
   }
   return -1;
}

int pf_rte_uio_reg_write (
   struct mera_inst * inst,
   const uintptr_t addr,
   const uint32_t data)
{
   if (base_mem != NULL)
   {
      base_mem[addr] = PCIE_HOST_CVT (data);
      return 0;
   }
   return -1;
}

int pf_rte_uio_init (void)
{
   const char * driver = "mscc_switch";
   const char * top = "/sys/class/uio";
   DIR * dir;
   struct dirent * dent;
   char fn[PATH_MAX], devname[128];
   FILE * fp;
   char iodev[512];
   size_t mapsize;
   int dev_fd, len, rc = -1;

   if (!(dir = opendir (top)))
   {
      T_E ("operdir(%s) failed\n", top);
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
         T_E ("UIO: Failed to open: %s\n", fn);
         continue;
      }

      const char * rrd = fgets (devname, sizeof (devname), fp);
      fclose (fp);

      if (!rrd)
      {
         T_E ("UIO: Failed to read: %s\n", fn);
         continue;
      }

      len = strlen (devname);
      if (len > 0 && devname[len - 1] == '\n')
      {
         devname[len - 1] = '\0';
      }
      T_D ("UIO: %s -> %s\n", fn, devname);
      if (
         !strstr (devname, "mscc_switch") &&
         !strstr (devname, "vcoreiii_switch") &&
         !strstr (devname, "lan966x_uio"))
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
         T_I ("Using UIO device: %s\n", devname);
         break;
      }
      fclose (fp);
   }
   closedir (dir);

   if (rc < 0)
   {
      T_E ("No suitable UIO device found!\n");
      return rc;
   }

   /* Open the UIO device file */
   T_D ("Using UIO, found '%s' driver at %s\n", driver, iodev);
   dev_fd = open (iodev, O_RDWR);
   if (dev_fd < 1)
   {
      T_E ("open(%s) failed\n", iodev);
      rc = -1;
   }
   else
   {
      /* mmap the UIO device */
      base_mem =
         mmap (NULL, mapsize, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, 0);
      if (base_mem != MAP_FAILED)
      {
         T_D ("Mapped register memory @ %p\n", base_mem);
      }
      else
      {
         T_E ("mmap failed\n");
         rc = -1;
      }
   }
   return rc;
}
