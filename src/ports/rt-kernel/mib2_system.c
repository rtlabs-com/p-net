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

/**
 * @file
 * @brief Functions for reading and writing SNMP MIB-II system variables
 */

#include "mib2_system.h"

#include "options.h"
#include "pnet_api.h"
#include "osal.h"
#include "osal_log.h"
#include "pnal.h"
#include "pf_types.h"
#include "pf_snmp.h"
#include "pnal_snmp.h"

#include <string.h>

/**
 * Get value of sysDescr variable
 *
 * @param value            Out:   Buffer where value will be stored.
 * @param max_size         In:    Size of buffer in bytes.
 * @return Size of retrieved value (in bytes) if successful,
 *         SNMP_ERR_GENERROR if buffer was too small.
 */
static s16_t system_get_description (void * value, size_t max_size)
{
   size_t size;
   pf_snmp_system_description_t description;

   pf_snmp_get_system_description (pnal_snmp.net, &description);

   size = strlen (description.string);
   if (size > max_size)
   {
      return SNMP_ERR_GENERROR;
   }

   memcpy (value, description.string, size);
   return size;
}

/**
 * Get value of sysObjectID variable
 *
 * @param value            Out:   Buffer where value will be stored.
 * @param max_size         In:    Size of buffer in bytes.
 * @return Size of retrieved value (in bytes) if successful,
 *         SNMP_ERR_GENERROR if buffer was too small.
 */
static s16_t system_get_object_id (void * value, size_t max_size)
{
   size_t size;
   const struct snmp_obj_id * dev_enterprise_oid =
      snmp_get_device_enterprise_oid();

   size = dev_enterprise_oid->len * sizeof (u32_t);
   if (size > max_size)
   {
      return SNMP_ERR_GENERROR;
   }

   memcpy (value, dev_enterprise_oid->id, size);
   return size;
}

/**
 * Get value of sysUpTime variable
 *
 * @param value            Out:   Buffer where value will be stored.
 * @param max_size         In:    Size of buffer in bytes.
 * @return Size of retrieved value (in bytes) if successful,
 *         SNMP_ERR_GENERROR if buffer was too small.
 */
static s16_t system_get_uptime (void * value, size_t max_size)
{
   u32_t * uptime = value;

   *uptime = pnal_get_system_uptime_10ms();
   return sizeof (*uptime);
}

/**
 * Get value of sysContact variable
 *
 * @param value            Out:   Buffer where value will be stored.
 * @param max_size         In:    Size of buffer in bytes.
 * @return Size of retrieved value (in bytes) if successful,
 *         SNMP_ERR_GENERROR if buffer was too small.
 */
static s16_t system_get_contact (void * value, size_t max_size)
{
   size_t size;
   pf_snmp_system_contact_t contact;

   pf_snmp_get_system_contact (pnal_snmp.net, &contact);

   size = strlen (contact.string);
   if (size > max_size)
   {
      return SNMP_ERR_GENERROR;
   }

   memcpy (value, contact.string, size);
   return size;
}

/**
 * Test if value could be set for sysContact variable
 *
 * @param value            In:    Buffer containing new value.
 *                                Not null-terminated.
 * @param size             In:    Size of \a value in bytes.
 * @return SNMP_ERR_NOERROR if successful,
 *         SNMP_ERR_WRONGVALUE if \a size is too large.
 */
static snmp_err_t system_test_contact (const void * value, size_t size)
{
   pf_snmp_system_contact_t contact;

   if (size >= sizeof (contact.string))
   {
      return SNMP_ERR_WRONGVALUE;
   }
   else
   {
      return SNMP_ERR_NOERROR;
   }
}

/**
 * Set new value for sysContact variable
 *
 * Note that it has already been verified that \a size is not too large.
 *
 * @param value            In:    Buffer containing new value.
 *                                Not null-terminated.
 * @param size             In:    Size of \a value in bytes.
 * @return SNMP_ERR_NOERROR if successful,
 *         SNMP_ERR_GENERROR on error.
 */
static snmp_err_t system_set_contact (const void * value, size_t size)
{
   pf_snmp_system_contact_t contact;
   int error;

   memset (contact.string, '\0', sizeof (contact.string));
   memcpy (contact.string, value, size);

   error = pf_snmp_set_system_contact (pnal_snmp.net, &contact);
   if (error)
   {
      return SNMP_ERR_GENERROR;
   }
   else
   {
      return SNMP_ERR_NOERROR;
   }
}

/**
 * Get value of sysName variable
 *
 * @param value            Out:   Buffer where value will be stored.
 * @param max_size         In:    Size of buffer in bytes.
 * @return Size of retrieved value (in bytes) if successful,
 *         SNMP_ERR_GENERROR if buffer was too small.
 */
static s16_t system_get_name (void * value, size_t max_size)
{
   size_t size;
   pf_snmp_system_name_t name;

   pf_snmp_get_system_name (pnal_snmp.net, &name);

   size = strlen (name.string);
   if (size > max_size)
   {
      return SNMP_ERR_GENERROR;
   }

   memcpy (value, name.string, size);
   return size;
}

/**
 * Test if value could be set for sysName variable
 *
 * @param value            In:    Buffer containing new value.
 *                                Not null-terminated.
 * @param size             In:    Size of \a value in bytes.
 * @return SNMP_ERR_NOERROR if successful,
 *         SNMP_ERR_WRONGVALUE if \a size is too large.
 */
static snmp_err_t system_test_name (const void * value, size_t size)
{
   pf_snmp_system_name_t name;

   if (size >= sizeof (name.string))
   {
      return SNMP_ERR_WRONGVALUE;
   }
   else
   {
      return SNMP_ERR_NOERROR;
   }
}

/**
 * Set new value for sysName variable
 *
 * Note that it has already been verified that \a size is not too large.
 *
 * @param value            In:    Buffer containing new value.
 *                                Not null-terminated.
 * @param size             In:    Size of \a value in bytes.
 * @return SNMP_ERR_NOERROR if successful,
 *         SNMP_ERR_GENERROR on error.
 */
static snmp_err_t system_set_name (const void * value, size_t size)
{
   pf_snmp_system_name_t name;
   int error;

   memset (name.string, '\0', sizeof (name.string));
   memcpy (name.string, value, size);

   error = pf_snmp_set_system_name (pnal_snmp.net, &name);
   if (error)
   {
      return SNMP_ERR_GENERROR;
   }
   else
   {
      return SNMP_ERR_NOERROR;
   }
}

/**
 * Get value of sysLocation variable
 *
 * @param value            Out:   Buffer where value will be stored.
 * @param max_size         In:    Size of buffer in bytes.
 * @return Size of retrieved value (in bytes) if successful,
 *         SNMP_ERR_GENERROR if buffer was too small.
 */
static s16_t system_get_location (void * value, size_t max_size)
{
   size_t size;
   pf_snmp_system_location_t location;

   pf_snmp_get_system_location (pnal_snmp.net, &location);

   size = strlen (location.string);
   if (size > max_size)
   {
      return SNMP_ERR_GENERROR;
   }

   memcpy (value, location.string, size);
   return size;
}

/**
 * Test if value could be set for sysLocation variable
 *
 * @param value            In:    Buffer containing new value.
 *                                Not null-terminated.
 * @param size             In:    Size of \a value in bytes.
 * @return SNMP_ERR_NOERROR if successful,
 *         SNMP_ERR_WRONGVALUE if \a size is too large.
 */
static snmp_err_t system_test_location (const void * value, size_t size)
{
   pf_snmp_system_location_t location;

   if (size >= sizeof (location.string))
   {
      return SNMP_ERR_WRONGVALUE;
   }
   else
   {
      return SNMP_ERR_NOERROR;
   }
}

/**
 * Set new value for sysLocation variable
 *
 * Note that it has already been verified that \a size is not too large.
 *
 * @param value            In:    Buffer containing new value.
 *                                Not null-terminated.
 * @param size             In:    Size of \a value in bytes.
 * @return SNMP_ERR_NOERROR if successful,
 *         SNMP_ERR_GENERROR on error.
 */
static snmp_err_t system_set_location (const void * value, size_t size)
{
   pf_snmp_system_location_t location;
   int error;

   memset (location.string, '\0', sizeof (location.string));
   memcpy (location.string, value, size);

   error = pf_snmp_set_system_location (pnal_snmp.net, &location);
   if (error)
   {
      return SNMP_ERR_GENERROR;
   }
   else
   {
      return SNMP_ERR_NOERROR;
   }
}

/**
 * Get value of sysServices variable
 *
 * @param value            Out:   Buffer where value will be stored.
 * @param max_size         In:    Size of buffer in bytes.
 * @return Size of retrieved value (in bytes) if successful,
 *         SNMP_ERR_GENERROR if buffer was too small.
 */
static s16_t system_get_services (void * value, size_t max_size)
{
   s32_t * services = value;

   *services = SNMP_SYSSERVICES;
   return sizeof (*services);
}

s16_t mib2_system_get_value (u32_t column, void * value, size_t max_size)
{
   switch (column)
   {
   case 1: /* sysDescr */
      return system_get_description (value, max_size);
   case 2: /* sysObjectID */
      return system_get_object_id (value, max_size);
   case 3: /* sysUpTime */
      return system_get_uptime (value, max_size);
   case 4: /* sysContact */
      return system_get_contact (value, max_size);
   case 5: /* sysName */
      return system_get_name (value, max_size);
   case 6: /* sysLocation */
      return system_get_location (value, max_size);
   case 7: /* sysServices */
      return system_get_services (value, max_size);
   default:
      LOG_ERROR (
         PF_SNMP_LOG,
         "MIB2.system(%d): Unknown column: %" PRIu32 ".\n",
         __LINE__,
         column);
      return SNMP_ERR_GENERROR;
   }
}

snmp_err_t mib2_system_test_set_value (
   u32_t column,
   const void * value,
   size_t size)
{
   switch (column)
   {
   case 4: /* sysContact */
      return system_test_contact (value, size);
   case 5: /* sysName */
      return system_test_name (value, size);
   case 6: /* sysLocation */
      return system_test_location (value, size);
   default:
      LOG_ERROR (
         PF_SNMP_LOG,
         "MIB2.system(%d): Unknown column: %" PRIu32 ".\n",
         __LINE__,
         column);
      return SNMP_ERR_WRONGVALUE;
   }
}

snmp_err_t mib2_system_set_value (u32_t column, const void * value, size_t size)
{
   switch (column)
   {
   case 4: /* sysContact */
      return system_set_contact (value, size);
   case 5: /* sysName */
      return system_set_name (value, size);
   case 6: /* sysLocation */
      return system_set_location (value, size);
   default:
      LOG_ERROR (
         PF_SNMP_LOG,
         "MIB2.system(%d): Unknown column: %" PRIu32 ".\n",
         __LINE__,
         column);
      return SNMP_ERR_GENERROR;
   }
}
