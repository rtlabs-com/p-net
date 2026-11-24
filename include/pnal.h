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

/**
 * @file
 * @brief Definitions for profinet abstraction layer.
 *
 * The Ethernet-related functions should not use \a loc_port_num, but
 * the interface name instead. The lookup from \a loc_port_num to
 * interface name must happen somewhere else in the stack.
 */

#ifndef PNAL_H
#define PNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "osal.h"

/**
 * Operating system dependent configuration, opaque to the stack.
 *
 * The port defines the concrete struct pnal_cfg in its own
 * pnal_config.h. It is typically used to hold the thread configuration
 * (see pnal_thread_cfg_t) of each thread started by the port. A minimal
 * port can use an empty or stub struct.
 *
 * It is part of the stack configuration passed to pnet_init(), and the stack
 * hands it on to the port, for example in pnal_eth_init().
 */
typedef struct pnal_cfg pnal_cfg_t;

/* Opaque forward declaration of pnet config. */
typedef struct pnet_cfg pnet_cfg_t;

/* Opaque forward declaration of pnet. */
typedef struct pnet pnet_t;

/**
 * Frame buffer, opaque to the stack.
 *
 * The port defines the concrete struct in its own header, typically
 * wrapping the buffer type already used by the OSAL or network stack of the
 * platform. The buffer may be non-contiguous. The stack only accesses it via
 * the frame buffer functions.
 */
typedef struct pnal_buf pnal_buf_t;

/**
 * Thread priority and stack size.
 *
 * Can be used in the port-defined pnal_cfg_t for each thread started by
 * the port.
 */
typedef struct pnal_thread_cfg
{
   uint32_t prio;
   size_t stack_size;
} pnal_thread_cfg_t;


/**
 * Socket abstraction for pnet
 *
 * We use a large enough integer to store
 * a pointer on those systems that want some
 * other logic than file descriptors.
 *
 * PNAL_INVALID_SOCKET indicates an invalid
 * socket and all other values indicate
 * a valid socket.
 */
typedef intptr_t pnal_socket_t;

/*
 * Invalid socket value
 *
 * This value is compatible with standard
 * file descriptors, as well as when a
 * pointer to an allocated structure is
 * used.
 */
#define PNAL_INVALID_SOCKET (pnal_socket_t)(-1)


#define PNAL_MAKEU32(a, b, c, d)                                               \
   (((uint32_t)((a)&0xff) << 24) | ((uint32_t)((b)&0xff) << 16) |              \
    ((uint32_t)((c)&0xff) << 8) | (uint32_t)((d)&0xff))

#define PNAL_MAKEU16(a, b) (((uint16_t)((a)&0xff) << 8) | (uint16_t)((b)&0xff))

#define PNAL_INET_ADDRSTR_SIZE 16 /** Incl termination */
#define PNAL_ETH_ADDRSTR_SIZE  18 /** Incl termination */
#ifndef PNAL_HOSTNAME_MAX_SIZE
#define PNAL_HOSTNAME_MAX_SIZE 64 /** Incl termination. Value from Linux. */
#endif

/**
 * Ethernet autonegotiation capabilities (not exhaustive)
 *
 * See IEEE 802.3 (Ethernet) ch. 30.6 "Management for link Auto-Negotiation".
 *
 * See IETF RFC 4836 "Definitions of Managed Objects for
 * IEEE 802.3 Medium Attachment Units (MAUs)", object
 * "IANAifMauAutoNegCapBits"
 */
#define PNAL_ETH_AUTONEG_CAP_1000BaseT_FULL_DUPLEX BIT (0)
#define PNAL_ETH_AUTONEG_CAP_1000BaseT_HALF_DUPLEX BIT (1)
#define PNAL_ETH_AUTONEG_CAP_1000BaseX_FULL_DUPLEX BIT (2)
#define PNAL_ETH_AUTONEG_CAP_1000BaseX_HALF_DUPLEX BIT (3)
#define PNAL_ETH_AUTONEG_CAP_100BaseTX_FULL_DUPLEX BIT (10)
#define PNAL_ETH_AUTONEG_CAP_100BaseTX_HALF_DUPLEX BIT (11)
#define PNAL_ETH_AUTONEG_CAP_10BaseT_FULL_DUPLEX   BIT (13)
#define PNAL_ETH_AUTONEG_CAP_10BaseT_HALF_DUPLEX   BIT (14)
#define PNAL_ETH_AUTONEG_CAP_UNKNOWN               BIT (15)

/**
 * Ethernet MAU type (not exhaustive).
 *
 * See IEEE 802.3 (Ethernet) ch. 30.5 "Layer management for medium
 * attachment units (MAUs)".
 *
 * See IETF RFC 4836 "Definitions of Managed Objects for
 * IEEE 802.3 Medium Attachment Units (MAUs)", objects
 * "IANAifMauTypeListBits" and "dot3MauType".
 *
 * See Profinet "5.2.12.3.12 Coding of the field MAUType"
 */
typedef enum pnal_eth_mau
{
   PNAL_ETH_MAU_UNKNOWN = 0x00, /* When link is down */
   PNAL_ETH_MAU_RADIO = 0x00,   /* When link is up */
   PNAL_ETH_MAU_COPPER_10BaseT = 0x05,
   PNAL_ETH_MAU_COPPER_100BaseTX_HALF_DUPLEX = 0x0F,
   PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX = 0x10,
   PNAL_ETH_MAU_COPPER_1000BaseT_HALF_DUPLEX = 0x1D,
   PNAL_ETH_MAU_COPPER_1000BaseT_FULL_DUPLEX = 0x1E,
   PNAL_ETH_MAU_FIBER_100BaseFX_HALF_DUPLEX = 0x11,
   PNAL_ETH_MAU_FIBER_100BaseFX_FULL_DUPLEX = 0x12,
   PNAL_ETH_MAU_FIBER_1000BaseX_HALF_DUPLEX = 0x15,
   PNAL_ETH_MAU_FIBER_1000BaseX_FULL_DUPLEX = 0x16,
   PNAL_ETH_MAU_COPPER_10BaseT1L = 0x8d,
} pnal_eth_mau_t;


/**
 * Ethernet MAU type extension (exhaustive as of v2.45).
 *
 * See Profinet "5.2.12.3.13 Coding of the field MAUTypeExtension"
 */
typedef enum pnal_eth_mau_ext
{
   PNAL_ETH_MAU_EXTENSION_NONE = 0x0000,
   PNAL_ETH_MAU_EXTENSION_POF = 0x0100,
   PNAL_ETH_MAU_EXTENSION_APL = 0x0200,
} pnal_eth_mau_ext_t;

/* See Profinet 2.4 section 5.2.28 and annex W */
typedef struct pnal_port_stats
{
   uint32_t if_in_octets;
   uint32_t if_out_octets;
   uint32_t if_in_discards;
   uint32_t if_out_discards;
   uint32_t if_in_errors;
   uint32_t if_out_errors;
} pnal_port_stats_t;

/** Set an IP address given by the four byte-parts */
#define PNAL_IP4_ADDR_TO_U32(ipaddr, a, b, c, d)                               \
   ipaddr = PNAL_MAKEU32 (a, b, c, d)

typedef enum pnal_ethertype
{
   PNAL_ETHTYPE_IP = 0x0800U,
   PNAL_ETHTYPE_ARP = 0x0806U,
   PNAL_ETHTYPE_VLAN = 0x8100U,
   PNAL_ETHTYPE_PROFINET = 0x8892U,
   PNAL_ETHTYPE_ETHERCAT = 0x88A4U,
   PNAL_ETHTYPE_LLDP = 0x88CCU,
   PNAL_ETHTYPE_MRP = 0x88E3U,
   PNAL_ETHTYPE_ALL = 0xFFFFU,
} pnal_ethertype_t;

/* 255.255.255.255 */
#ifndef PNAL_IPADDR_NONE
#define PNAL_IPADDR_NONE ((uint32_t)0xffffffffUL)
#endif
/* 127.0.0.1 */
#ifndef PNAL_IPADDR_LOOPBACK
#define PNAL_IPADDR_LOOPBACK ((uint32_t)0x7f000001UL)
#endif
/* 0.0.0.0 */
#ifndef PNAL_IPADDR_ANY
#define PNAL_IPADDR_ANY ((uint32_t)0x00000000UL)
#endif
#define PNAL_IPADDR_INVALID PNAL_IPADDR_ANY
/* 255.255.255.255 */
#ifndef PNAL_IPADDR_BROADCAST
#define PNAL_IPADDR_BROADCAST ((uint32_t)0xffffffffUL)
#endif

typedef uint32_t pnal_ipaddr_t;
typedef uint16_t pnal_ipport_t;

/**
 * The Ethernet MAC address.
 *
 * TODO: Remove this and use pnet_ethaddr_t instead
 */
typedef struct pnal_ethaddr
{
   uint8_t addr[6];
} pnal_ethaddr_t;

/**
 * Ethernet link status.
 *
 * See also type pf_lldp_link_status_t.
 */
typedef struct pnal_eth_status_t
{
   /* Is autonegotiation supported on this port? */
   bool is_autonegotiation_supported;

   /* Is autonegotiation supported on this port? */
   bool is_autonegotiation_enabled;

   /* Capabilities advertised to link partner during autonegotiation.
    *
    * See macros PNAL_ETH_AUTONEG_CAP_xxx.
    */
   uint16_t autonegotiation_advertised_capabilities;

   /* Operational MAU type.
    *
    * This specifies both the physical medium as well as
    * speed and duplex setting used for the link.
    */
   pnal_eth_mau_t operational_mau_type;

   /* MAU type extension.
    *
    * This is used to further specify the physical medium. It should
    * only be non-zero for POF (plastic fiber) and APL (10BASET1L).
    *
    * See PN-AL-Protocol, table "MAUType with MAUTypeExtension".
    */
   pnal_eth_mau_ext_t mau_type_extension;

   /* Corresponds to Linux "RUNNING" flag.
    * Up and cable connected
    */
   bool running;
} pnal_eth_status_t;

/** SNMP control modes */
typedef enum
{
   PNAL_SNMP_CONTROL_MODE_DISABLED = 0,
   PNAL_SNMP_CONTROL_MODE_READONLY = 1,
   PNAL_SNMP_CONTROL_MODE_READWRITE = 2
} pnal_snmp_control_mode_t;

/**
 * Maximum size that can be requested when
 * requesting a pnal buffer
 */
extern const size_t pnal_buf_max_size;

/**
 * Get system uptime from the SNMP implementation.
 *
 * This is the sysUpTime, as used by SNMP:
 * "The time (in hundredths of a second) since the network
 *  management portion of the system was last re-initialized."
 * - IETF RFC 3418 (SNMP MIB-2).
 *
 * Typically used to store the SNMP-timestamp for incoming LLDP-packets.
 * This timestamp is later reported back to the SNMP implementation, and
 * remote SNMP managers might use it for filtering.
 *
 * The returned value should be the one used by the SNMP implementation.
 * Ask the SNMP implementation for the value, or make sure that this
 * function and the SNMP implementation use the same mechanism to get the
 * value.
 *
 * Starts at 0, with wrap-around after ~497 days.
 *
 * @return System uptime, in units of 10 milliseconds.
 */
uint32_t pnal_get_system_uptime_10ms (void);

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
int pnal_load_file (
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
int pnal_save_file (
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
void pnal_clear_file (const char * fullpath);

/**
 * Read from frame buffer
 *
 * This will copy @a size number of bytes from the (possibly)
 * non-contiguous buffer @a p at @a offset to the buffer @a data.
 *
 * @pre No attempt is made to read past end of frame buffer.
 *
 * @param p                In:    Frame buffer to read from.
 * @param offset           In:    Byte offset in frame buffer.
 * @param data             Out:   Storage for read bytes.
 * @param size             In:    Number of bytes to read.
 */
void pnal_buf_read (
   const pnal_buf_t * p,
   int offset,
   void * data,
   size_t size);

/**
 * Write to frame buffer
 *
 * This will copy @a size number of bytes from the buffer @a data
 * to the (possibly) non-contiguous buffer @a p at @a offset.
 *
 * @pre No attempt is made to write past end of frame buffer.
 *
 * @param p                InOut: Frame buffer to write to.
 * @param offset           In:    Byte offset in frame buffer.
 * @param data             In:    Data to be written.
 * @param size             In:    Number of bytes to write.
 */
void pnal_buf_write (
   pnal_buf_t * p,
   int offset,
   const void * data,
   size_t size);

/**
 * Compare data with contents in frame buffer
 *
 * This compares @a size number of bytes in the buffer @a data to @a size
 * number of bytes in the (possibly) non-contiguous buffer @a p at @a offset.
 *
 * @pre No attempt is made to read past end of frame buffer.
 *
 * @param p                In:    Frame buffer to compare with.
 * @param offset           In:    Byte offset in frame buffer.
 * @param data             In:    Data to compare with.
 * @param size             In:    Number of bytes to compare.
 * @return 0 if contents of @a data are identical with contents of frame buffer,
 *         non-zero otherwise.
 */
int pnal_buf_compare (
   const pnal_buf_t * p,
   int offset,
   const void * data,
   size_t size);

/**
 * Fill frame buffer with byte pattern
 *
 * The sets @a size number of bytes in the frame buffer @a p to @a value,
 * starting at @a offset.
 *
 * @pre No attempt is made to write past end of frame buffer.
 *
 * @param p                InOut: Frame buffer to write to.
 * @param offset           In:    Byte offset in frame buffer.
 * @param value            In:    Byte pattern to be written.
 * @param size             In:    Number of bytes to write.
 */
void pnal_buf_fill (
   pnal_buf_t * p,
   int offset,
   uint8_t value,
   size_t size);

/**
 * Set frame buffer size
 *
 * Size can only be reduced, not increased.
 * Contents of buffer beyond new size is discarded and may be deallocated.
 *
 * @pre Configured @a size is less than or equal to current size.
 *
 * @param p                InOut: Frame buffer.
 * @param size             In:    New buffer size.
 *                                Valid range: 0 - current size.
 */
void pnal_buf_set_size (pnal_buf_t * p, size_t size);

/**
 * Get frame buffer size
 *
 * @param p                In:    Frame buffer.
 *
 * @return  Size of frame buffer in bytes
 */
size_t pnal_buf_get_size (const pnal_buf_t * p);

/**
 * Allocate a frame buffer
 *
 * The resulting \a pnal_buf_t frame buffer is defined for
 * each operating system.
 *
 * @param length           In:    Length, in bytes
 * @return a frame buffer, or NULL at failure
 */
pnal_buf_t * pnal_buf_alloc (uint16_t length);

/**
 * Free a frame buffer
 *
 * @param p           In:    Frame buffer to free
 */
void pnal_buf_free (pnal_buf_t * p);

/** Not yet used */
uint8_t pnal_buf_header (pnal_buf_t * p, int16_t header_size_increment);

/**
 * Network interface handle, forward declaration.
 */
typedef struct pnal_eth_handle pnal_eth_handle_t;

/**
 * The prototype of raw Ethernet reception call-back functions.
 *
 * @param eth_handle       InOut: Network interface handle
 * @param arg              InOut: User-defined (may be NULL).
 * @param p_buf            InOut: The incoming Ethernet frame
 *
 * @return  0  If the frame was NOT handled by this function.
 *          1  If the frame was handled and the buffer freed.
 */
typedef int (pnal_eth_callback_t) (
   pnal_eth_handle_t * eth_handle,
   void * arg,
   pnal_buf_t * p_buf);

/**
 * Get status of Ethernet link on specified port
 *
 * @param interface_name   In:    Ethernet interface name, for example eth0
 * @param status           Out:   Returned link status (auto-negotiation etc)
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred.
 */
int pnal_eth_get_status (
   const char * interface_name,
   pnal_eth_status_t * status);

/**
 * Get network interface index
 *
 * The interface index (ifIndex) is the (row) index for the network interface
 * in the table ifTable, which is part of the SNMP MIB-II data structure.
 * See RFC 2863 "The Interfaces Group MIB".
 *
 * @param interface_name   In:    Ethernet interface name, for example eth0
 *
 * @return  The interface index, or 0 if not available.
 */
int pnal_get_interface_index (const char * interface_name);

/**
 * Get network interface (port) statistics
 *
 * In Profinet naming a physical Ethernet interface is a "port".
 * This corresponds to for example a Linux Ethernet interface.
 *
 * @param interface_name   In:    Ethernet interface name for example eth0
 * @param port_stats       Out:   Returned statistics
 *
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pnal_get_port_statistics (
   const char * interface_name,
   pnal_port_stats_t * port_stats);

/**
 * Send raw Ethernet data
 *
 * @param handle           In:    Ethernet handle
 * @param buf              In:    Buffer with data to be sent
 * @return  The number of bytes sent, or -1 if an error occurred.
 */
int pnal_eth_send (pnal_eth_handle_t * handle, pnal_buf_t * buf);

/**
 * Initialize receiving of raw Ethernet frames on one interface (in separate
 * thread)
 *
 * @param if_name          In:    Ethernet interface name
 * @param receive_type     In:    Ethernet frame types that shall be received
 *                                by the network interface / port.
 * @param pnal_cfg         In:    Operating system dependent configuration
 * @param callback         In:    Callback for received raw Ethernet frames.
 *                                Called from the receiving thread for each
 *                                frame.
 * @param arg              InOut: User argument passed to the callback
 *
 * @return  the Ethernet handle, or NULL if an error occurred.
 */
pnal_eth_handle_t * pnal_eth_init (
   const char * if_name,
   pnal_ethertype_t receive_type,
   const pnal_cfg_t * pnal_cfg,
   pnal_eth_callback_t * callback,
   void * arg);

/**
 * Open an UDP socket
 *
 * @param addr             In:    IP address to listen to. Typically used with
 *                                PNAL_IPADDR_ANY.
 * @param port             In:    UDP port to listen to.
 * @return Socket ID, or -1 if an error occurred.
 */
pnal_socket_t pnal_udp_open (pnal_ipaddr_t addr, pnal_ipport_t port);

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
int pnal_udp_sendto (
   pnal_socket_t id,
   pnal_ipaddr_t dst_addr,
   pnal_ipport_t dst_port,
   const uint8_t * data,
   int size);

/**
 * Receive UDP data.
 *
 * This is a non-blocking function, and it
 * returns 0 immediately if no data is available.
 *
 * @param id               In:    Socket ID
 * @param src_addr         Out:   Source IP address
 * @param src_port         Out:   Source UDP port
 * @param data             Out:   Received data
 * @param size             In:    Size of buffer for received data
 * @return  The number of bytes received, or -1 if an error occurred.
 */
int pnal_udp_recvfrom (
   pnal_socket_t id,
   pnal_ipaddr_t * src_addr,
   pnal_ipport_t * src_port,
   uint8_t * data,
   int size);

/**
 * Close an UDP socket
 *
 * @param id               In:    Socket ID
 */
void pnal_udp_close (pnal_socket_t id);

/**
 * Initialize SNMP server.
 *
 * This function configures a platform-specific SNMP server as to
 * enable a connected SNMP client to read variables from the p-net stack,
 * as well as to write some variables to it.
 *
 * With Security Level 1, the stored SNMP configuration (see
 * pf_snmp_get_config()) must also be applied, unless the SNMP server keeps
 * its configuration between restarts.
 *
 * @param net              InOut: The p-net stack instance
 * @param pnal_cfg         In:    Operating system dependent configuration
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred.
 */
int pnal_snmp_init (pnet_t * net, const pnal_cfg_t * pnal_cfg);

/**
 * Reconfigure SNMP server.
 * Setting the control mode and community names.
 *
 * @param mode             In:  SNMP control mode (disabled, read-only,
 *                              read-write)
 * @param rocommunity      In:  The read-only (public) community name.
 * @param rwcommunity      In:  The read-write (private) community name.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred.
 */
int pnal_snmp_configure (
   pnal_snmp_control_mode_t mode,
   const char * rocommunity,
   const char * rwcommunity);

/**
 * Initialize MRP support.
 *
 * @param net              InOut: The p-net stack instance.
 * @param pnal_cfg         In:    Operating system dependent configuration.
 * @return  0 if successful,
 *         -1 if not.
 */
int pnal_mrp_init (pnet_t * net, const pnal_cfg_t * pnal_cfg);

/**
 * MRP configuration.
 */
typedef struct pnal_mrp_cfg
{
   uint8_t domain_uuid[16];
   char * domain_name;
   uint16_t link_down_T;
   uint16_t link_up_T;
   uint16_t link_nr_max;
} pnal_mrp_cfg_t;

/**
 * Configure MRP service
 *
 * This function configures a platform-specific MRP service,
 * possibly asynchronously.
 *
 * @param enable           In: Enable(true)/disable(false) MRP.
 * @param cfg              In: The MRP configuration to use.
 * @return  0 if successful,
 *         -1 if not.
 */
int pnal_mrp_config (bool enable, pnal_mrp_cfg_t * cfg);

/**
 * Trigger an MRP status information message.
 *
 * @return  0 if successful,
 *         -1 if not.
 */
int pnal_mrp_status (void);

/**
 * MRP port status.
 */
typedef enum {
   rp_state_disabled = 1,
   rp_state_blocked = 2,
   rp_state_forwarding = 3,
   rp_state_not_connected = 4,
} mrp_ring_port_state_t;

/**
 * MRP domain state.
 */
typedef enum {
  domain_state_disabled = BIT(0),
  domain_state_undefined = BIT(1),
  domain_state_ring_open = BIT(2),
  domain_state_reserved = BIT(3),
} mrp_domain_state_flags_t;

/**
 * Get the MRP port status, based on interface name and port number.
 *
 * @param interface_name   In: The platform network interface name.
 * @param loc_port_num     In: The port number.
 * @param status           Out: The MRP port status.
 * @return  0 on success,
 *         -1 on failure.
 */
int pnal_mrp_get_ring_port_status(const char * interface_name,
                                  int loc_port_num,
                                  mrp_ring_port_state_t *status);

/**
 * Get the MRP port VLAN ID, based on interface name and port number,
 * if supported.
 *
 * @param interface_name   In: The platform network interface name.
 * @param loc_port_num     In: The port number.
 * @param vlan_id          Out: The MRP port VLAN ID, or 0 if not supported.
 * @return  0 on success,
 *         -1 on failure.
 */
int pnal_mrp_get_vlan_id(const char * interface_name,
                         int loc_port_num,
                         int *vlan_id);

/**
 * Get network parameters (IP address, netmask etc)
 *
 * For example:
 *
 * IP address        Represented by
 * 1.0.0.0           0x01000000 = 16777216
 * 0.0.0.1           0x00000001 = 1
 *
 * @param interface_name   In:    Ethernet interface name, for example eth0
 * @param p_ipaddr         Out:   IPv4 address
 * @param p_netmask        Out:   Netmask
 * @param p_gw             Out:   Default gateway
 * @param hostname         Out:   Host name, for example my_laptop_4. Existing
 *                                buffer should have size
 *                                PNAL_HOSTNAME_MAX_SIZE.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pnal_get_ip_suite (
   const char * interface_name,
   pnal_ipaddr_t * p_ipaddr,
   pnal_ipaddr_t * p_netmask,
   pnal_ipaddr_t * p_gw,
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
 * @param interface_name   In:    Name of network interface
 * @return IP address on success and
 *         0 if an error occurred
 */
pnal_ipaddr_t pnal_get_ip_address (const char * interface_name);

/**
 * Read the netmask as an integer. For IPv4.
 *
 * @param interface_name   In:    Name of network interface
 * @return netmask
 */
pnal_ipaddr_t pnal_get_netmask (const char * interface_name);

/**
 * Read the default gateway address as an integer. For IPv4.
 *
 * Assumes the default gateway is found on .1 on same subnet as the IP address.
 *
 * @param interface_name   In:    Name of network interface
 * @return netmask
 */
pnal_ipaddr_t pnal_get_gateway (const char * interface_name);

/**
 * Read the MAC address.
 *
 * @param interface_name   In:    Name of network interface
 * @param p_mac            Out:   MAC address
 *
 * @return 0 on success and
 *         -1 if no such interface is available
 */
int pnal_get_macaddress (const char * interface_name, pnal_ethaddr_t * p_mac);

/**
 * Read the current host name
 *
 * The resulting string is null terminated.
 *
 * @param hostname         Out:   Host name, for example my_laptop_4. Existing
 *                                buffer should have size
 *                                PNAL_HOSTNAME_MAX_SIZE.
 * @return 0 on success and
 *         -1 if an error occurred
 */
int pnal_get_hostname (char * hostname);

/**
 * Set network parameters (IP address, netmask etc)
 *
 * For example:
 *
 * IP address        Represented by
 * 1.0.0.0           0x01000000 = 16777216
 * 0.0.0.1           0x00000001 = 1
 *
 * @param interface_name   In:    Ethernet interface name, for example eth0
 * @param p_ipaddr         In:    IPv4 address
 * @param p_netmask        In:    Netmask
 * @param p_gw             In:    Default gateway
 * @param hostname         In:    Host name, for example my_laptop_4
 * @param permanent        In:    1 if changes are permanent, or 0 if temporary
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pnal_set_ip_suite (
   const char * interface_name,
   const pnal_ipaddr_t * p_ipaddr,
   const pnal_ipaddr_t * p_netmask,
   const pnal_ipaddr_t * p_gw,
   const char * hostname,
   bool permanent);


/** Size of an MD5 digest in bytes */
#define PNAL_MD5_DIGEST_SIZE 16

/**
 * Compute the MD5 digest of the input data.
 *
 * @param input            In:    Input data.
 * @param input_size       In:    Size of input data.
 * @param output           In:    Array to hold the resulting MD5 digest.
 */
void pnal_get_md5_digest (
   const uint8_t * input,
   size_t input_size,
   uint8_t output[PNAL_MD5_DIGEST_SIZE]);

#ifdef __cplusplus
}
#endif

#endif /* PNAL_H */
