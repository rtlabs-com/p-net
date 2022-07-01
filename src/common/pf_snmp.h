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

/**
 * @file
 * @brief Helper functions for use by platform-dependent SNMP server (agent).
 *
 * The p-net stack does not contain any SNMP server stack. Instead, it
 * relies on external software to supply the needed SNMP services. In
 * the p-net abstraction layer (pnal), the file pnal_snmp.c is
 * expected to implement the glue code needed make the external SNMP
 * server communicate with the p-net stack (in particular, to get
 * variables stored in the p-net stack).
 *
 * The purposes of this header file is to make it easier to implement
 * the glue code in pnal_snmp.c by declaring all needed functions in a
 * single header file.  This API should not be used by other modules
 * than the SNMP server.
 *
 * The API assumes the following model:
 * The Profinet device has a single interface (IP address), but may have
 * multiple ports. Each port may be connected to 1 or 0 remote network
 * entities, where a network entity may be a Profinet Device/Controller/
 * Supervisor or an LLDP-aware network switch.
 *
 * The SNMP variables are declared in the following MIBs:
 * - MIB-II. See IETF RFC 3418 (SNMP MIB).
 * - LLDP-MIB. See IEEE 802.1AB-2005 (LLDPv1).
 * - LLDP-EXT-DOT3-MIB. See IEEE 802.1AB-2005 (LLDPv1).
 * - LLDP-EXT-PNO-MIB. See IEC CDV 61158-6-10 (PN-AL-Protocol).
 * The Profinet guideline PN-Topology describes usage of SNMP (see ch. 6.2).
 *
 * Getter functions for the following variables for the local device, interface
 * and ports are provided:
 * - sysName
 * - sysContact
 * - sysLocation
 * - sysDescr
 * - lldpLocChassisId
 * - lldpLocChassisIdSubtype
 * - lldpLocPortNum/lldpRemLocalPortNum
 * - lldpLocPortId
 * - lldpLocPortIdSubtype
 * - lldpLocPortDesc
 * - lldpLocManAddr
 * - lldpLocManAddrSubtype
 * - lldpLocManAddrIfId
 * - lldpLocManAddrIfSubtype
 * - lldpConfigManAddrPortsTxEnable
 * - lldpXPnoLocPortNoS
 * - lldpXPnoLocLPDValue
 * - lldpXPnoLocPortTxDValue
 * - lldpXPnoLocPortRxDValue
 * - lldpXdot3LocPortAutoNegSupported
 * - lldpXdot3LocPortAutoNegEnabled
 * - lldpXdot3LocPortAutoNegAdvertisedCap
 * - lldpXdot3LocPortOperMauType
 *
 * Setter functions for the following variables for the local device are
 * provided:
 * - sysName
 * - sysContact
 * - sysLocation
 *
 * Getter functions for the following variables for the remote devices,
 * interfaces and ports are provided:
 * - lldpRemChassisId
 * - lldpRemChassisIdSubtype
 * - lldpRemPortId
 * - lldpRemPortIdSubtype
 * - lldpRemPortDesc
 * - lldpRemManAddr
 * - lldpRemManAddrSubtype
 * - lldpRemManAddrIfId
 * - lldpRemManAddrIfIdSubtype
 * - lldpRemTimeMark
 * - lldpXPnoRemPortNoS
 * - lldpXPnoRemLPDValue
 * - lldpXPnoRemPortTxDValue
 * - lldpXPnoRemPortRxDValue
 * - lldpXdot3RemPortAutoNegSupported
 * - lldpXdot3RemPortAutoNegEnabled
 * - lldpXdot3RemPortAutoNegAdvertisedCap
 * - lldpXdot3RemPortOperMauType
 *
 * All functions are thread-safe.
 */

#ifndef PF_SNMP_H
#define PF_SNMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pf_lldp.h"

/* SNMP-related filenames used by p-net stack. A filename may not be longer
 * than PNET_MAX_FILENAME_SIZE (termination included).
 */
#define PF_FILENAME_SNMP_SYSCONTACT  "pnet_data_syscontact.bin"
#define PF_FILENAME_SNMP_SYSNAME     "pnet_data_sysname.bin"
#define PF_FILENAME_SNMP_SYSLOCATION "pnet_data_syslocation.bin"

/**
 * System name (sysName).
 *
 * "An administratively-assigned name for this managed
 * node. By convention, this is the node's fully-qualified
 * domain name. If the name is unknown, the value is
 * the zero-length string."
 * - IETF RFC 3418 (SNMP MIB-II).
 *
 * Is the same as LLDP's "System Name".
 * See PN-Topology Annex A.
 *
 * Note: According to the SNMP specification, the string could be up
 * to 255 characters (excluding termination). The p-net stack limits it to
 * PNAL_HOSTNAME_MAX_SIZE, including null-termination.
 *
 * This is a writable variable. As such it is be in persistent memory.
 * Only writable variables (using SNMP Set) need to be stored
 * in persistent memory.
 * See IEC CDV 61158-5-10 (PN-AL-Services) ch. 7.3.3.3.6.2: "Persistency".
 */
typedef struct pf_snmp_system_name
{
   char string[PNAL_HOSTNAME_MAX_SIZE]; /* Terminated string */
} pf_snmp_system_name_t;

/**
 * System contact (sysContact).
 *
 * "The textual identification of the contact person
 * for this managed node, together with information
 * on how to contact this person. If no contact information is
 * known, the value is the zero-length string."
 * - IETF RFC 3418 (SNMP MIB-II).
 *
 * The value is supplied by network manager. By default, it is
 * the zero-length string.
 *
 * An extra byte is added as to ensure null-termination.
 *
 * This is a writable variable. As such, it is stored in persistent memory.
 * Only writable variables (using SNMP Set) need to be stored
 * in persistent memory.
 * See IEC CDV 61158-5-10 (PN-AL-Services) ch. 7.3.3.3.6.2: "Persistency".
 */
typedef struct pf_snmp_system_contact
{
   char string[255 + 1]; /* Terminated */
} pf_snmp_system_contact_t;

/**
 * System description (sysDescr).
 *
 * "A textual description of the entity. This value
 * should include the full name and version
 * identification of the system's hardware type,
 * software operating-system, and networking
 * software. It is mandatory that this only contain
 * printable ASCII characters."
 * - IETF RFC 3418 (SNMP MIB-II) ch. 6 "Definitions".
 *
 * Note that MIB-II's sysDescr should have the same value as LLDP's
 * lldpLocSysDesc and (preferably) as Profinet's SystemIdentification.
 * The Chassis ID may also be encoded as SystemIdentification.
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 12.2 "LLDP MIB module" and
 * IEC CDV 61158-6-10 (PN-AL-Protocol) ch. 4.16.4 "IETF RFC 1213-MIB" and
 * ch. 4.11.3.18.1 "Coding of the field LLDP_ChassisID".
 *
 * Note:
 * An extra byte is included as to ensure null-termination.
 */
typedef struct pf_snmp_system_description
{
   char string[PNET_LLDP_CHASSIS_ID_MAX_SIZE]; /** Terminated string */
} pf_snmp_system_description_t;

/**
 * System location (sysLocation).
 *
 * "The physical location of this node (e.g.,
 * 'telephone closet, 3rd floor'). If the location is unknown,
 *  the value is the zero-length string."
 * - IETF RFC 3418 (SNMP MIB-II).
 *
 * The value is supplied by network manager. By default, it is a string with
 * 22 space (' ') characters.
 *
 * The first 22 bytes should have the same value as "IM_Tag_Location" in I&M1.
 * See PN-Topology ch. 11.5.2: "Consistency".
 *
 * This is a writable variable. As such, it is stored in persistent memory.
 * Only writable variables (using SNMP Set) need to be stored
 * in persistent memory.
 * See IEC CDV 61158-5-10 (PN-AL-Services) ch. 7.3.3.3.6.2: "Persistency".
 */
typedef struct pf_snmp_system_location
{
   char string[255 + 1]; /* Terminated string */
} pf_snmp_system_location_t;

/**
 * Encoded management address.
 *
 * Contains similar information as pf_lldp_management_address_t, but the
 * fields have been encoded so they may be immediately placed in SNMP response.
 */
typedef struct pf_snmp_management_address
{
   /** First byte is size of actual address */
   uint8_t value[1 + 31];

   /** 1 for IPv4 */
   uint8_t subtype;

   /** 5 for IPv4 */
   size_t len;
} pf_snmp_management_address_t;

/**
 * Encoded link status.
 *
 * Contains the same information as pf_lldp_link_status_t, but the fields
 * have been encoded so they may be immediately placed in SNMP response.
 */
typedef struct pf_snmp_link_status
{
   /** 1 if true, 2 if false */
   int32_t auto_neg_supported;

   /** 1 if true, 2 if false */
   int32_t auto_neg_enabled;

   /** OCTET STRING encoding of BITS */
   uint8_t auto_neg_advertised_cap[2];

   int32_t oper_mau_type;
} pf_snmp_link_status_t;

/**
 * Measured signal delays in nanoseconds
 *
 * If a signal delay was not measured, its value is zero.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol) Annex U: "LLDP EXT MIB", fields
 * lldpXPnoLocLPDValue / lldpXPnoRemLPDValue,
 * lldpXPnoLocPortTxDValue / lldpXPnoRemPortTxDValue,
 * lldpXPnoLocPortRxDValue / lldpXPnoRemPortRxDValue.
 *
 * See also pf_lldp_signal_delay_t.
 */
typedef struct pf_snmp_signal_delay
{
   uint32_t port_tx_delay_ns;
   uint32_t port_rx_delay_ns;
   uint32_t line_propagation_delay_ns;
} pf_snmp_signal_delay_t;

/**
 * Initialize SNMP related data, by reading from file.
 *
 * @param net              InOut: The p-net stack instance.
 */
void pf_snmp_data_init (pnet_t * net);

/**
 * Clear SNMP related data.
 *
 * Clears in-memory data and calls pf_snmp_remove_data_files().
 *
 * Note that it does not clear the IM_Tag_Location" from I&M1
 * (which typically is the same as SNMP SysLocation).
 *
 * @param net              InOut: The p-net stack instance.
 */
void pf_snmp_data_clear (pnet_t * net);

/**
 * Removes SNMP related data files.
 *
 * Used by pf_snmp_data_clear() and other operations.
 *
 * @param net              InOut: The p-net stack instance.
 */
void pf_snmp_remove_data_files (const char * file_directory);

/**
 * Update SNMP SysLocation with the present I&M1 location value.
 *
 * Does remove the corresponding file. Upon next startup will we use the
 * I&M location value.
 *
 * @param net              InOut: The p-net stack instance.
 */
void pf_snmp_fspm_im_location_ind (pnet_t * net);

/**
 * Get system description.
 *
 * See IETF RFC 3418 (SNMP MIB-II) ch. 2 "Definitions". Relevant fields:
 * - SysDescr.
 *
 * @param net              InOut: The p-net stack instance.
 * @param p_description    Out:   System description.
 */
void pf_snmp_get_system_description (
   pnet_t * net,
   pf_snmp_system_description_t * p_description);

/**
 * Get system name.
 *
 * The value will be loaded from file.
 * If no file is available, the hostname is returned.
 *
 * Note: User may choose to get the fully qualified domain name (FQDN) instead
 * of calling this function, assuming the operating system supports it.
 *
 * See IETF RFC 3418 (SNMP MIB-II) ch. 2 "Definitions". Relevant fields:
 * - SysName.
 *
 * @param net              InOut: The p-net stack instance.
 * @param p_name           Out:   System name.
 */
void pf_snmp_get_system_name (pnet_t * net, pf_snmp_system_name_t * p_name);

/**
 * Set system name.
 *
 * The value will be stored to file.
 * The hostname will not be updated.
 *
 * Note: User may choose to set the fully qualified domain name (FQDN) instead
 * of calling this function, assuming the operating system supports it.
 *
 * See IETF RFC 3418 (SNMP MIB-II) ch. 2 "Definitions". Relevant fields:
 * - SysName.
 *
 * @param net              InOut: The p-net stack instance.
 * @param p_name           In:    System name.
 * @return  0  if the operation succeeded.
 *         -1 if an error occurred (could not store to file).
 */
int pf_snmp_set_system_name (pnet_t * net, const pf_snmp_system_name_t * p_name);

/**
 * Get system contact.
 *
 * The value will be loaded from file.
 * If no file is available, an empty string is returned.
 *
 * See IETF RFC 3418 (SNMP MIB-II) ch. 2 "Definitions". Relevant fields:
 * - SysContact.
 *
 * @param net              InOut: The p-net stack instance.
 * @param p_contact        Out:   System contact.
 */
void pf_snmp_get_system_contact (
   pnet_t * net,
   pf_snmp_system_contact_t * p_contact);

/**
 * Set system contact.
 *
 * The value will be stored to file.
 *
 * See IETF RFC 3418 (SNMP MIB-II) ch. 2 "Definitions". Relevant fields:
 * - SysContact.
 *
 * @param net              InOut: The p-net stack instance.
 * @param p_contact        In:    System contact.
 * @return  0  if the operation succeeded.
 *         -1 if an error occurred (could not store to file).
 */
int pf_snmp_set_system_contact (
   pnet_t * net,
   const pf_snmp_system_contact_t * p_contact);

/**
 * Get system location.
 *
 * The value will be loaded from file. If file is not available,
 * the device location from I&M1 will be used.
 *
 * See IETF RFC 3418 (SNMP MIB-II) ch. 2 "Definitions". Relevant fields:
 * - SysLocation.
 *
 * @param net              InOut: The p-net stack instance.
 * @param p_location       Out:   System location.
 */
void pf_snmp_get_system_location (
   pnet_t * net,
   pf_snmp_system_location_t * p_location);

/**
 * Set system location.
 *
 * The value will be stored to file.
 * The device location in I&M1 will also be replaced with the first
 * 22 characters of system location.
 *
 * See IETF RFC 3418 (SNMP MIB-II) ch. 2 "Definitions". Relevant fields:
 * - SysLocation.
 *
 * @param net              InOut: The p-net stack instance.
 * @param p_location       In:    System location.
 * @return  0  if the operation succeeded.
 *         -1 if an error occurred (could not store to file).
 */
int pf_snmp_set_system_location (
   pnet_t * net,
   const pf_snmp_system_location_t * p_location);

/**
 * Get list of local ports.
 *
 * This is the list of physical ports on the local interface.
 * The management port is not included.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 12.2. Relevant fields:
 * - lldpConfigManAddrPortsTxEnable.
 *
 * @param net              In:    The p-net stack instance.
 * @param p_list           Out:   List of local ports.
 */
void pf_snmp_get_port_list (pnet_t * net, pf_lldp_port_list_t * p_list);

/**
 * Initialise iterator for iterating over local ports.
 *
 * This iterator may be used for iterating over all physical ports
 * on the local interface. The management port is not included.
 * See pf_snmp_get_next_port().
 *
 * @param net              In:    The p-net stack instance.
 * @param p_iterator       Out:   Port iterator.
 */
void pf_snmp_init_port_iterator (pnet_t * net, pf_port_iterator_t * p_iterator);

/**
 * Get next local port.
 *
 * If no more ports are available, 0 is returned.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 12.2. Relevant fields:
 * - lldpLocPortNum,
 * - lldpRemLocalPortNum.
 *
 * @param p_iterator       InOut: Port iterator.
 * @return Local port number for next port on local interface.
 *         If no more ports are available, 0 is returned.
 */
int pf_snmp_get_next_port (pf_port_iterator_t * p_iterator);

/**
 * Get time when new information about remote device was received.
 *
 * Information about the remote device is received in an LLDP packet on a
 * local port and relevant information is stored by LLDP stack.
 * If an LLDP packet has been received, a timestamp will also be saved.
 * This may be used by the SNMP server thread for time-filtering.
 * See PN-Topology ch. 6.2.3: "Time-filtered OIDs".
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 12.2. Relevant fields:
 * - lldpRemTimeMark.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_timestamp_10ms Out:   Time when the LLDP packet with the info
 *                                was first received, in units of
 *                                10 milliseconds with system startup
 *                                as the zero point (as in SNMP sysUpTime).
 *                                Note that if an LLDP packet with identical
 *                                info is received, the timestamp is not
 *                                updated.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_snmp_get_peer_timestamp (
   pnet_t * net,
   int loc_port_num,
   uint32_t * p_timestamp_10ms);

/**
 * Get Chassis ID of local device.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 12.2. Relevant fields:
 * - lldpLocChassisId,
 * - lldpLocChassisIdSubtype.
 *
 * @param net              In:    The p-net stack instance.
 * @param p_chassis_id     Out:   Chassis ID of local device.
 */
void pf_snmp_get_chassis_id (pnet_t * net, pf_lldp_chassis_id_t * p_chassis_id);

/**
 * Get Chassis ID for remote device connected to local port.
 *
 * The remote Chassis ID is contained in an LLDP packet sent from a port
 * on the remote device to a local port with no intermediate switches.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 12.2. Relevant fields:
 * - lldpRemChassisId,
 * - lldpRemChassisIdSubtype,
 * - lldpRemLocalPortNum.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_chassis_id     Out:   Chassis ID of remote device.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_snmp_get_peer_chassis_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_chassis_id_t * p_chassis_id);

/**
 * Get Port ID for local port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 12.2. Relevant fields:
 * - lldpLocPortId,
 * - lldpLocPortIdSubtype,
 * - lldpLocPortNum.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_port_id        Out:   Port ID of local port.
 */
void pf_snmp_get_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id);

/**
 * Get Port ID for remote port.
 *
 * The remote Port ID is contained in an LLDP packet sent from a port
 * on the remote device to the local port with no intermediate switches.
 *
 * Note that the remote device may have multiple ports. Only the remote
 * port connected to the local port is relevant here.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 12.2. Relevant fields:
 * - lldpRemPortId,
 * - lldpRemPortIdSubtype,
 * - lldpRemLocalPortNum.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_port_id        Out:   Port ID of remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_snmp_get_peer_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id);

/**
 * Get port description for local port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 12.2. Relevant fields:
 * - lldpLocPortDesc,
 * - lldpLocPortNum.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_port_desc      Out:   Port description of local port.
 */
void pf_snmp_get_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_desc);

/**
 * Get port description for remote port.
 *
 * The remote Port description is contained in an LLDP packet sent from a port
 * on the remote device to the local port with no intermediate switches.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 12.2. Relevant fields:
 * - lldpRemPortDesc,
 * - lldpRemLocalPortNum.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_port_desc      Out:   Port description of remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_snmp_get_peer_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_desc);

/**
 * Get encoded Management Address for local interface.
 *
 * The returned management address is encoded for use in SNMP response.
 * See PN-Topology ch. 6.3.1.
 *
 * See IEEE 802.1AB-2005 ch. 12.2. Relevant fields:
 * - lldpLocManAddr,
 * - lldpLocManAddrLen,
 * - lldpLocManAddrSubtype.
 *
 * @param net              In:    The p-net stack instance.
 * @param p_man_address    Out:   Management address of local interface.
 */
void pf_snmp_get_management_address (
   pnet_t * net,
   pf_snmp_management_address_t * p_man_address);

/**
 * Get encoded Management Address for remote interface connected to local port.
 *
 * The returned management address is encoded for use in SNMP response.
 * See PN-Topology ch. 6.3.1.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 12.2. Relevant fields:
 * - lldpRemManAddr,
 * - lldpRemManAddrLen,
 * - lldpRemManAddrSubtype,
 * - lldpRemLocalPortNum.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_man_address    Out:   Management address of remote interface.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_snmp_get_peer_management_address (
   pnet_t * net,
   int loc_port_num,
   pf_snmp_management_address_t * p_man_address);

/**
 * Get ManAddrIfId for local interface.
 *
 * The row in lldpLocManAddrTable will contain a reference to a row in IfTable,
 * which is part of MIB-II.
 * See PN-Topology ch. 6.5.1 "Mapping of Ports and PROFINET Interfaces
 * in LLDP MIB and MIB-II".
 *
 * Note that the local device may have multiple interfaces (such as a
 * loopback interface). Only the local interface used by the p-net stack
 * is relevant here.
 *
 * See IEEE 802.1AB-2005 ch. 12.2. Relevant fields:
 * - lldpLocManAddrIfId,
 * - lldpLocManAddrIfSubtype.
 *
 * @param net              In:    The p-net stack instance.
 * @param p_man_port_index Out:   Index in IfTable for Management port.
 */
void pf_snmp_get_management_port_index (
   pnet_t * net,
   pf_lldp_interface_number_t * p_man_port_index);

/**
 * Get ManAddrIfId for remote interface connected to local port.
 *
 * The ManAddrIfId of remote device is contained in an LLDP packet sent from a
 * port on the remote device to the local port with no intermediate switches.
 *
 * Note that the remote device may have multiple interfaces. Only the remote
 * interface connected to the local port is relevant here.
 *
 * See IEEE 802.1AB-2005 ch. 12.2. Relevant fields:
 * - lldpRemManAddrIfId,
 * - lldpRemManAddrIfIdSubtype,
 * - lldpRemLocalPortNum.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_man_port_index Out:   Index in remote IfTable for Management port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_snmp_get_peer_management_port_index (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_interface_number_t * p_man_port_index);

/**
 * Get local station name.
 *
 * The local station name (NameOfStation) is the name of the local interface.
 *
 * The station name is usually a string, but may also be the MAC address
 * (as a string "AB-CD-EF-01-23-45") of the remote interface in case no string
 * has been assigned.
 *
 * Note that the local device may have multiple interfaces (such as a
 * loopback interface). Only the local interface used by the p-net stack
 * is relevant here.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol) annex U (LLDP EXT MIB). Relevant
 * fields:
 * - lldpXPnoLocPortNoS.
 *
 * @param net              In:    The p-net stack instance.
 * @param p_station_name   Out:   Station name of local interface.
 */
void pf_snmp_get_station_name (
   pnet_t * net,
   pf_lldp_station_name_t * p_station_name);

/**
 * Get remote station name for remote interface connected to local port.
 *
 * The remote station name (NameOfStation) is the name of the remote interface
 * and is contained in an LLDP packet sent from a port
 * on the remote device to the local port with no intermediate switches.
 *
 * The station name is usually a string, but may also be the MAC address
 * (as a string "AB-CD-EF-01-23-45") of the remote interface in case no string
 * has been assigned.
 *
 * Note that the remote device may have multiple interfaces. Only the remote
 * interface connected to the local port is relevant here.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol) annex U (LLDP EXT MIB). Relevant
 * fields:
 * - lldpXPnoRemPortNoS,
 * - lldpRemLocalPortNum.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_station_name   Out:   Station name of remote interface.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_snmp_get_peer_station_name (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_station_name_t * p_station_name);

/**
 * Get measured signal delays on local port.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol) Annex U: "LLDP EXT MIB". Relevant
 * fields:
 * - lldpXPnoLocLPDValue,
 * - lldpXPnoLocPortTxDValue,
 * - lldpXPnoLocPortRxDValue,
 * - lldpLocPortNum.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_delays         Out:   Measured signal delays on local port.
 */
void pf_snmp_get_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_snmp_signal_delay_t * p_delays);

/**
 * Get measured signal delays on remote port.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol) Annex U: "LLDP EXT MIB". Relevant
 * fields:
 * - lldpXPnoRemLPDValue,
 * - lldpXPnoRemPortTxDValue,
 * - lldpXPnoRemPortRxDValue,
 * - lldpRemLocalPortNum.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_delays         Out:   Measured signal delays on remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_snmp_get_peer_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_snmp_signal_delay_t * p_delays);

/**
 * Get encoded link status of local port.
 *
 * The returned fields are encoded and ready to be put in SNMP response.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) Annex G.7.1: "IEEE 802.3 LLDP extension
 * MIB module". Relevant fields:
 * - lldpXdot3LocPortAutoNegSupported,
 * - lldpXdot3LocPortAutoNegEnabled,
 * - lldpXdot3LocPortAutoNegAdvertisedCap,
 * - lldpXdot3LocPortOperMauType,
 * - lldpLocPortNum.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_link_status    Out:   Link status of local port.
 */
void pf_snmp_get_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_snmp_link_status_t * p_link_status);

/**
 * Get encoded link status of remote port.
 *
 * The returned fields are encoded and ready to be put in SNMP response.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) annex G.7.1: "IEEE 802.3 LLDP extension
 * MIB module". Relevant fields:
 * - lldpXdot3RemPortAutoNegSupported,
 * - lldpXdot3RemPortAutoNegEnabled,
 * - lldpXdot3RemPortAutoNegAdvertisedCap,
 * - lldpXdot3RemPortOperMauType,
 * - lldpRemLocalPortNum.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_snmp_get_next_port().
 * @param p_link_status    Out:   Link status of remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_snmp_get_peer_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_snmp_link_status_t * p_link_status);

/**
 * Show SNMP details
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_snmp_show (pnet_t * net);

#ifdef __cplusplus
}
#endif

#endif /* PF_SNMP_H */
