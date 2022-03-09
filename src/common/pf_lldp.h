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
 * @brief Periodical transmission outgoing LLDP frames
 *        Reception of incoming LLDP frames
 */

#ifndef PF_LLDP_H
#define PF_LLDP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pnal.h"

#define PF_LLDP_SUBTYPE_MAC              4 /** Valid for ChassisID */
#define PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED 7

/**
 * Link status
 *
 * See IEEE 802.1AB-2005 (LLDPv1) Annex G.2 "MAC/PHY Configuration/Status TLV".
 * See also pnal_eth_status_t
 */
typedef struct pf_lldp_link_status
{
   bool is_autonegotiation_supported;
   bool is_autonegotiation_enabled;
   uint16_t autonegotiation_advertised_capabilities;
   uint16_t operational_mau_type;
   bool is_valid;
} pf_lldp_link_status_t;

/**
 * Port description
 *
 * "The port description field shall contain an alphanumeric string
 * that indicates the port's description. If RFC 2863
 * is implemented, the ifDescr object should be used for this field."
 * - IEEE 802.1AB (LLDP) ch. 9.5.5.2 "port description".
 *
 * Note: According to the SNMP specification, the string could be up
 * to 255 characters (excluding termination). The p-net stack limits it to
 * PNET_MAX_PORT_DESCRIPTION_SIZE (including termination).
 */
typedef struct pf_lldp_port_description
{
   char string[PNET_MAX_PORT_DESCRIPTION_SIZE]; /* Terminated string */
   bool is_valid;
   size_t len;
} pf_lldp_port_description_t;

/**
 * Port list
 *
 * "Each octet within this value specifies a set of eight ports,
 * with the first octet specifying ports 1 through 8, the second
 * octet specifying ports 9 through 16, etc. Within each octet,
 * the most significant bit represents the lowest numbered port,
 * and the least significant bit represents the highest numbered
 * port. Thus, each port of the system is represented by a
 * single bit within the value of this object. If that bit has
 * a value of '1' then that port is included in the set of ports;
 * the port is not included if its bit has a value of '0'."
 * - IEEE 802.1AB (LLDP) ch. 12.2 "LLDP MIB module" (lldpPortList).
 *
 * Se also section about lldpConfigManAddrTable in PN-Topology.
 */
typedef struct pf_lldp_port_list_t
{
   uint8_t ports[2];
} pf_lldp_port_list_t;

/**
 * Port iterator
 *
 * This iterator may be used for iterating over all physical ports
 * on the local interface. The management port is not included.
 */
typedef struct pf_port_iterator
{
   int next_port;
   int number_of_ports;
} pf_port_iterator_t;

/**
 * Chassis ID
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.2.3 "chassis ID".
 */
typedef struct pf_lldp_chassis_id
{
   /** Terminated string
       Typically this field is a string (PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED)
       If the subtype indicates a MAC address, the content is the
       raw MAC address bytes (and trailing null termination).
       Similarly for other subtypes */
   char string[PNET_LLDP_CHASSIS_ID_MAX_SIZE];

   /** PF_LLDP_SUBTYPE_xxx */
   uint8_t subtype;

   bool is_valid;

   size_t len;
} pf_lldp_chassis_id_t;

/**
 * Port ID
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.3 "Port ID TLV".
 */
typedef struct pf_lldp_port_id
{
   /** Terminated string
       Typically this field is a string (PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED)
       If the subtype indicates a MAC address, the content is the
       raw MAC address bytes (and trailing null termination).
       Similarly for other subtypes */
   char string[PNET_LLDP_PORT_ID_MAX_SIZE];

   /** PF_LLDP_SUBTYPE_xxx */
   uint8_t subtype;

   bool is_valid;

   size_t len;
} pf_lldp_port_id_t;

/**
 * Interface number
 *
 * "The interface number field shall contain the assigned number within
 * the system that identifies the specific interface associated with this
 * management address. If the value of the interface subtype is unknown,
 * this field shall be set to zero."
 * - IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.9 "Management Address TLV".
 *
 * Also see PN-Topology ch. 6.5.1 "Mapping of Ports and PROFINET Interfaces
 * in LLDP MIB and MIB-II".
 */
typedef struct pf_lldp_interface_number
{
   int32_t value;
   uint8_t subtype; /* 1 = unknown, 2 = ifIndex, 3 = systemPortNumber */
} pf_lldp_interface_number_t;

/**
 * Management address
 *
 * Usually, this is an IPv4 address. It may also be a MAC address
 * or other kinds of addresses, such as IPv6.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.9 "Management Address TLV".
 */
typedef struct pf_lldp_management_address
{
   uint8_t value[31];
   uint8_t subtype;
   size_t len;
   pf_lldp_interface_number_t interface_number;
   bool is_valid;
} pf_lldp_management_address_t;

/**
 * Station name
 *
 * This is the name of an interface. It is called NameOfStation in
 * the Profinet specification. It is usually a string, but may also
 * be a MAC address.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol) ch. 4.3.1.4.16.
 */
typedef struct pf_lldp_station_name
{
   char string[PNET_STATION_NAME_MAX_SIZE]; /** Terminated string */
   size_t len;
} pf_lldp_station_name_t;

/**
 * Port Name
 * In standard name-of-device mode this is first part of Port ID
 * In legacy mode Port ID and Port Name are the same.
 */
typedef struct pf_lldp_port_name
{
   char string[PNET_PORT_NAME_MAX_SIZE]; /** Terminated string */
   size_t len;
} pf_lldp_port_name_t;

/**
 * Measured signal delays in nanoseconds
 *
 * Valid range is 0x1 - 0xFFF.
 * If a signal delay was not measured, its value is zero.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol) ch. 4.11.2.2: "LLDP APDU
 * abstract syntax", element "LLDP_PNIO_DELAY".
 *
 * See also pf_snmp_signal_delay_t.
 */
typedef struct pf_lldp_signal_delay
{
   uint32_t rx_delay_local;
   uint32_t rx_delay_remote;
   uint32_t tx_delay_local;
   uint32_t tx_delay_remote;
   uint32_t cable_delay_local;
   bool is_valid;
} pf_lldp_signal_delay_t;

typedef struct pf_lldp_boundary
{
   bool not_send_LLDP_Frames;
   bool send_PTCP_Delay;
   bool send_PATH_Delay;
   bool reserved_bit4;
   uint8_t reserved_8;
   uint16_t rserved_16;
} pf_lldp_boundary_t;

typedef struct pf_lldp_peer_to_peer_boundary
{
   pf_lldp_boundary_t boundary;
   uint16_t properites;
} pf_lldp_peer_to_peer_boundary_t;

/**
 * LLDP Peer information used by the Profinet stack.
 */
typedef struct pf_lldp_peer_info
{
   /* LLDP TLVs */
   pf_lldp_chassis_id_t chassis_id;
   pf_lldp_port_id_t port_id;
   uint16_t ttl;
   pf_lldp_port_description_t port_description;
   pf_lldp_management_address_t management_address;
   /* PROFIBUS TLVs */
   pf_lldp_signal_delay_t port_delay;
   uint8_t port_status[4];
   pnet_ethaddr_t mac_address;
   uint32_t line_delay;
   pf_lldp_link_status_t phy_config;
} pf_lldp_peer_info_t;

/**
 * Get time when new information about remote device was received.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
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
int pf_lldp_get_peer_timestamp (
   pnet_t * net,
   int loc_port_num,
   uint32_t * p_timestamp_10ms);

/**
 * Get Chassis ID of local device.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.2 "Chassis ID TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param p_chassis_id     Out:   Chassis ID of local device.
 */
void pf_lldp_get_chassis_id (pnet_t * net, pf_lldp_chassis_id_t * p_chassis_id);

/**
 * Get Chassis ID for remote device connected to local port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.2 "Chassis ID TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 * @param p_chassis_id     Out:   Chassis ID of remote device.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_chassis_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_chassis_id_t * p_chassis_id);

/**
 * Get Port ID for local port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.3 "Port ID TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 * @param p_port_id        Out:   Port ID of local port.
 */
void pf_lldp_get_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id);

/**
 * Get Port ID for remote port.
 *
 * Note that the remote device may have multiple ports. Only the remote
 * port connected to the local port is relevant here.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.3 "Port ID TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 * @param p_port_id        Out:   Port ID of remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id);

/**
 * Get port description for local port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.5 "Port Description TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 * @param p_port_desc      Out:   Port description of local port.
 */
void pf_lldp_get_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_desc);

/**
 * Get port description for remote port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.5 "Port Description TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 * @param p_port_desc      Out:   Port description of remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_desc);

/**
 * Get Management Address for local interface.
 *
 * The management address should usually be the IP address for the local
 * interface the local ports belong to. It could also be the MAC address of
 * the local interface in case no IP address has been assigned.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.9 "Management Address TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param p_man_address    Out:   Management address of local interface.
 */
void pf_lldp_get_management_address (
   pnet_t * net,
   pf_lldp_management_address_t * p_man_address);

/**
 * Get Management Address for remote interface connected to local port.
 *
 * The management address should usually be the IP address for the remote
 * interface the remote port belongs to. It could also be the MAC address of
 * the remote interface in case no IP address has been assigned.
 *
 * Note that the remote device may have multiple interfaces. Only the remote
 * interface connected to the local port is relevant here.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.9 "Management Address TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 * @param p_man_address    Out:   Management address of remote interface.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_management_address (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_management_address_t * p_man_address);

/**
 * Get remote station name for remote interface connected to local port.
 *
 * The station name is usually a string, but may also be the MAC address
 * (as a string "AB-CD-EF-01-23-45") of the remote interface in case no string
 * has been assigned.
 *
 * Note that the remote device may have multiple interfaces. Only the remote
 * interface connected to the local port is relevant here.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 * @param p_station_name   Out:   Station name of remote interface.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_station_name (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_station_name_t * p_station_name);

/**
 * Get port name for remote port connected to local port.
 *
 * The port name is typically a string "port-001" or "port-001-00000"
 *
 * Note that the remote device may have multiple ports. Only the remote
 * port connected to the local port is relevant here.
 *
 * See Profinet 2.4 Protocol 4.3.1.4.17 "Coding of the field NameOfPort"
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 * @param p_port_name      Out:   Port name of remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_port_name (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_name_t * p_port_name);

/**
 * Get measured signal delays on local port.
 *
 * If a signal delay was not measured, its value is zero.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol): LLDP_PNIO_DELAY.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 * @param p_delays         Out:   Measured signal delays on local port.
 */
void pf_lldp_get_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_signal_delay_t * p_delays);

/**
 * Get measured signal delays on remote port.
 *
 * If a signal delay was not measured, its value is zero.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol): LLDP_PNIO_DELAY.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 * @param p_delays         Out:   Measured signal delays on remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_signal_delay_t * p_delays);

/**
 * Get link status of local port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) Annex G.2 "MAC/PHY Configuration/Status TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 * @param p_link_status    Out:   Link status of local port.
 */
void pf_lldp_get_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status);

/**
 * Get link status of remote port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) Annex G.2 "MAC/PHY Configuration/Status TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 * @param p_link_status    Out:   Link status of remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status);

/**
 * Get formatted system description
 *
 * Encode as per Profinet specification:
 * DeviceType, Blank, OrderID, Blank, IM_Serial_Number, Blank, HWRevision,
 * Blank, SWRevisionPrefix, SWRevision.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol) ch. 5.1.2 "APDU abstract syntax",
 * field "SystemIdentification".
 *
 * @param net                           In:    The p-net stack instance.
 * @param system_description            Out:   System description. Terminated
 *                                             string.
 * @param system_description_max_len    In:    Size of buffer.
 * return Length of resulting string.
 */
size_t pf_lldp_get_system_description (
   const pnet_t * net,
   char * system_description,
   size_t system_description_max_len);

/**
 * Generate a string from a byte array representing
 * a mac address. Format: "%02X-%02X-%02X-%02X-%02X-%02X".
 *
 * @param mac              In:  Mac address pointer.
 * @param mac_str          Out: Formatted MAC address string, null-terminated.
 * @param mac_str_max_len  In:  Max length of mac_str buffer.
 */
void pf_lldp_mac_address_to_string (
   const uint8_t * mac,
   char * mac_str,
   size_t mac_str_max_len);

/**
 * Initialize the LLDP component.
 *
 * This function initializes the LLDP component and
 * sends the initial LLDP message.
 * @param net              InOut: The p-net stack instance
 */
void pf_lldp_init (pnet_t * net);

/**
 * Start or restart a timer that monitors reception of LLDP frames from peer.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @param timeout_in_secs  In:    TTL of the peer, typically 20 seconds.
 */
void pf_lldp_restart_peer_timeout (
   pnet_t * net,
   int loc_port_num,
   uint16_t timeout_in_secs);

/**
 * Stop a timer that monitors reception of LLDP frames from peer, if running.
 *
 * No diagnosis check will be made.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
void pf_lldp_stop_peer_timeout (pnet_t * net, int loc_port_num);

/**
 * Mark LLDP peer data as invalid.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
void pf_lldp_invalidate_peer_info (pnet_t * net, int loc_port_num);

/**
 * Enable sending of LLDP frames on local port
 *
 * An initial LLDP frame is transmitted when operation is called.
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
void pf_lldp_send_enable (pnet_t * net, int loc_port_num);

/**
 * Disable sending of LLDP frames on local port
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
void pf_lldp_send_disable (pnet_t * net, int loc_port_num);

/**
 * Process received LLDP message.
 *
 * Parse LLDP TLV format and store selected information.
 * Trigger alarms if needed.
 * @param net              InOut: The p-net stack instance.
 * @param loc_port_num     In:    Local port number the frame was received on.
 *                                Valid range: 1 .. num_physical_ports
 *                                See pf_port_get_list_of_ports().
 *                                Frame is dropped for invalid values.
 * @param p_frame_buf      InOut: Received LLDP message.
 *                                Will be freed.
 * @param offset           In:    The offset to start of LLDP data.
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
int pf_lldp_recv (
   pnet_t * net,
   int loc_port_num,
   pnal_buf_t * p_frame_buf,
   uint16_t offset);

/**
 * Check if an alias name matches any of the LLDP peers.
 *
 * @param net              InOut: The p-net stack instance.
 * @param alias            In:    Alias to be compared with LLDP peers.
 *                                Null terminated string.
 * return                  true if the alias matches any of the peers,
 *                         false if not.
 */
bool pf_lldp_is_alias_matching (pnet_t * net, const char * alias);

/************ Internal functions, made available for unit testing ************/

int pf_lldp_generate_alias_name (
   const char * port_id,
   const char * chassis_id,
   char * alias,
   uint16_t len);

size_t pf_lldp_construct_frame (pnet_t * net, int loc_port_num, uint8_t buf[]);

int pf_lldp_parse_packet (
   const uint8_t buf[],
   uint16_t len,
   pf_lldp_peer_info_t * peer_info);

void pf_lldp_store_peer_info (
   pnet_t * net,
   int loc_port_num,
   const pf_lldp_peer_info_t * new_info);

#ifdef __cplusplus
}
#endif

#endif /* PF_LLDP_H */
