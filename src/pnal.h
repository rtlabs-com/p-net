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

#ifndef PNAL_H
#define PNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "pnal_sys.h"

#define OS_MAKEU32(a, b, c, d)                                                 \
   (((uint32_t) ((a)&0xff) << 24) | ((uint32_t) ((b)&0xff) << 16) |            \
    ((uint32_t) ((c)&0xff) << 8) | (uint32_t) ((d)&0xff))

#define OS_MAKEU16(a, b) (((uint16_t) ((a)&0xff) << 8) | (uint16_t) ((b)&0xff))

#define OS_INET_ADDRSTRLEN 16
#define OS_ETH_ADDRSTRLEN  18
#define OS_HOST_NAME_MAX   64 /* Value from Linux */

/** Set an IP address given by the four byte-parts */
#define OS_IP4_ADDR_TO_U32(ipaddr, a, b, c, d) ipaddr = OS_MAKEU32 (a, b, c, d)

enum os_eth_type
{
   OS_ETHTYPE_IP = 0x0800U,
   OS_ETHTYPE_ARP = 0x0806U,
   OS_ETHTYPE_VLAN = 0x8100U,
   OS_ETHTYPE_PROFINET = 0x8892U,
   OS_ETHTYPE_ETHERCAT = 0x88A4U,
   OS_ETHTYPE_LLDP = 0x88CCU,
};

/* 255.255.255.255 */
#ifndef OS_IPADDR_NONE
#define OS_IPADDR_NONE ((uint32_t)0xffffffffUL)
#endif
/* 127.0.0.1 */
#ifndef OS_IPADDR_LOOPBACK
#define OS_IPADDR_LOOPBACK ((uint32_t)0x7f000001UL)
#endif
/* 0.0.0.0 */
#ifndef OS_IPADDR_ANY
#define OS_IPADDR_ANY ((uint32_t)0x00000000UL)
#endif
/* 255.255.255.255 */
#ifndef OS_IPADDR_BROADCAST
#define OS_IPADDR_BROADCAST ((uint32_t)0xffffffffUL)
#endif

typedef uint32_t os_ipaddr_t;
typedef uint16_t os_ipport_t;

/**
 * The Ethernet MAC address.
 */
typedef struct os_ethaddr
{
   uint8_t addr[6];
} os_ethaddr_t;

/**
 * The p-net stack instance.
 *
 * This is needed for SNMP in order to access various stack variables,
 * such as the location  of the device and LLDP variables.
 */
typedef struct pnet pnet_t;

/**
 * Get system uptime.
 *
 * This is the sysUpTime, as used by SNMP:
 * "The time (in hundredths of a second) since the network
 *  management portion of the system was last re-initialized."
 * - IETF RFC 3418 (SNMP MIB-2).
 *
 * Starts at 0, with wrap-around after ~497 days.
 *
 * @return System uptime, in units of 10 milliseconds.
 */
uint32_t os_get_system_uptime_10ms (void);

/**
 * Load a binary file.
 *
 * Can load the data into two buffers.
 *
 * @param fullpath         In:    Full path to the file
 * @param object_1         Out:   Data to load, or NULL. Mandatory if size_1 > 0
 * @param size_1           In:    Size of object_1.
 * @param object_2         Out:   Data to load, or NULL. Mandatory if size_2 > 0
 * @param size_2           In:    Size of object_2.
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred.
 */
int os_load_file (
   const char * fullpath,
   void * object_1,
   size_t size_1,
   void * object_2,
   size_t size_2);

/**
 * Save a binary file.
 *
 * Can handle two output buffers.
 *
 * @param fullpath         In:    Full path to the file
 * @param object_1         In:    Data to save, or NULL. Mandatory if size_1 > 0
 * @param size_1           In:    Size of object_1.
 * @param object_2         In:    Data to save, or NULL. Mandatory if size_2 > 0
 * @param size_2           In:    Size of object_2.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int os_save_file (
   const char * fullpath,
   const void * object_1,
   size_t size_1,
   const void * object_2,
   size_t size_2);

/**
 * Clear a binary file.
 *
 * @param fullpath         In:    Full path to the file
 */
void os_clear_file (const char * fullpath);


/* os_buf_t should at least contain:
 *  void * payload;
 *  uint16_t len;
 */

os_buf_t * os_buf_alloc (uint16_t length);
void os_buf_free (os_buf_t * p);
uint8_t os_buf_header (os_buf_t * p, int16_t header_size_increment);

/**
 * Send raw Ethernet data
 *
 * @param handle        In: Ethernet handle
 * @param buf           In: Buffer with data to be sent
 * @return  The number of bytes sent, or -1 if an error occurred.
 */
int os_eth_send (os_eth_handle_t * handle, os_buf_t * buf);

/**
 * Initialize receiving of raw Ethernet frames (in separate thread)
 *
 * @param if_name       In: Ethernet interface name
 * @param callback      In: Callback for received raw Ethernet frames
 * @param arg           InOut: User argument passed to the callback
 *
 * @return  the Ethernet handle, or NULL if an error occurred.
 */
os_eth_handle_t * os_eth_init (
   const char * if_name,
   os_eth_callback_t * callback,
   void * arg);

/**
 * Open an UDP socket
 *
 * @param addr             In:    IP address to listen to. Typically used with
 * OS_IPADDR_ANY.
 * @param port             In:    UDP port to listen to.
 * @return Socket ID, or -1 if an error occurred.
 */
int os_udp_open (os_ipaddr_t addr, os_ipport_t port);

/**
 * Send UDP data
 *
 * @param id               In:    Socket ID
 * @param dst_addr         In:    Destination IP address
 * @param dst_port         In:    Destination UDP port
 * @param data             In:    Data to be sent
 * @param size             In:    Size of data
 * @return  The number of bytes sent, or -1 if an error occurred.
 */
int os_udp_sendto (
   uint32_t id,
   os_ipaddr_t dst_addr,
   os_ipport_t dst_port,
   const uint8_t * data,
   int size);

/**
 * Receive UDP data.
 *
 * This is a nonblocking function, and it
 * returns 0 immediately if no data is available.
 *
 * @param id               In:    Socket ID
 * @param dst_addr         Out:   Source IP address
 * @param dst_port         Out:   Source UDP port
 * @param data             Out:   Received data
 * @param size             In:    Size of buffer for received data
 * @return  The number of bytes received, or -1 if an error occurred.
 */
int os_udp_recvfrom (
   uint32_t id,
   os_ipaddr_t * src_addr,
   os_ipport_t * src_port,
   uint8_t * data,
   int size);

/**
 * Close an UDP socket
 *
 * @param id               In:    Socket ID
 */
void os_udp_close (uint32_t id);

/**
 * Configure SNMP server.
 *
 * This function configures a platform-specific SNMP server as to
 * enable a connected SNMP client to read variables from the p-net stack,
 * as well as to write some variables to it.
 *
 * @param net              InOut: The p-net stack instance
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred.
 */
int os_snmp_init (pnet_t * net);

/**
 * Get network parameters (IP address, netmask etc)
 *
 * For example:
 *
 * IP address        Represented by
 * 1.0.0.0           0x01000000 = 16777216
 * 0.0.0.1           0x00000001 = 1
 *
 * @param interface_name      In: Ethernet interface name, for example eth0
 * @param p_ipaddr            Out: IPv4 address
 * @param p_netmask           Out: Netmask
 * @param p_gw                Out: Default gateway
 * @param hostname            Out: Host name, for example my_laptop_4. Existing
 * buffer should have length OS_HOST_NAME_MAX.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int os_get_ip_suite (
   const char * interface_name,
   os_ipaddr_t * p_ipaddr,
   os_ipaddr_t * p_netmask,
   os_ipaddr_t * p_gw,
   char * hostname);

/**
 * Read the IP address as an integer. For IPv4.
 *
 * For example:
 *
 * IP address        Represented by
 * 1.0.0.0           0x01000000 = 16777216
 * 0.0.0.1           0x00000001 = 1
 *
 * @param interface_name      In: Name of network interface
 * @return IP address on success and
 *         0 if an error occurred
 */
os_ipaddr_t os_get_ip_address (const char * interface_name);

/**
 * Read the netmask as an integer. For IPv4.
 *
 * @param interface_name      In: Name of network interface
 * @return netmask
 */
os_ipaddr_t os_get_netmask (const char * interface_name);

/**
 * Read the default gateway address as an integer. For IPv4.
 *
 * Assumes the default gateway is found on .1 on same subnet as the IP address.
 *
 * @param interface_name      In: Name of network interface
 * @return netmask
 */
os_ipaddr_t os_get_gateway (const char * interface_name);

/**
 * Read the MAC address.
 *
 * @param interface_name      In: Name of network interface
 * @param mac_addr            Out: MAC address
 *
 * @return 0 on success and
 *         -1 if an error occurred
 */
int os_get_macaddress (const char * interface_name, os_ethaddr_t * p_mac);

/**
 * Read the current host name
 *
 * @param hostname            Out: Host name, for example my_laptop_4. Existing
 * buffer should have length OS_HOST_NAME_MAX.
 * @return 0 on success and
 *         -1 if an error occurred
 */
int os_get_hostname (char * hostname);

/**
 * Set network parameters (IP address, netmask etc)
 *
 * For example:
 *
 * IP address        Represented by
 * 1.0.0.0           0x01000000 = 16777216
 * 0.0.0.1           0x00000001 = 1
 *
 * @param interface_name      In: Ethernet interface name, for example eth0
 * @param p_ipaddr            In: IPv4 address
 * @param p_netmask           In: Netmask
 * @param p_gw                In: Default gateway
 * @param hostname            In: Host name, for example my_laptop_4
 * @param permanent           In: 1 if changes are permanent, or 0 if temportary
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int os_set_ip_suite (
   const char * interface_name,
   const os_ipaddr_t * p_ipaddr,
   const os_ipaddr_t * p_netmask,
   const os_ipaddr_t * p_gw,
   const char * hostname,
   bool permanent);

#ifdef __cplusplus
}
#endif

#endif /* PNAL_H */
