Implementation details
======================

This software stack is written in C, version C99.

For running the unit tests C++ (version C++11 or later) is required.

The primary p-net usecase is to run in embedded devices, why there in general
are no functions to shut down the stack.


State machines
--------------

* ALPMI       Alarm Protocol Machine Initiator
* ALPMR       Alarm Protocol Machine Responder
* APMR        Acyclic Protocol Machine Receiver
* APMS        Acyclic Protocol Machine Sender
* CMDEV       Context Management protocol machine Device, handles connection establishment for IO-Devices.
* CMINA       Context Management Ip and Name Assignment protocol machine
* CMIO        Context Management Input Output protocol machine
* CMPBE       Context Management Parameter Begin End protocol machine
* CMRDR       Context Management Read Record Responder protocol machine, responds to parameter read from the IO-controller
* CMRPC       Context Management RPC Protocol Machine, handles RPC communication
* CMSM        Context Management Surveillance protocol Machine, monitors the establishment of a connection.
* CMWRR       Context Management Write Record Responder protocol machine
* CPM         Consumer Protocol Machine, for receiving cyclic data
* FSPM        Fieldbus Application Layer Service Protocol Machine, calls user callbacks.
* PPM         Provider Protocol Machine, for sending cyclic data


FSPM - Fieldbus application layer Service Protocol Machine
----------------------------------------------------------
Stores the user-defined configuration, and calls the user-defined callbacks.
Create logbook entries. Reads and writes identification & maintenance records.


CMRPC - Context Management RPC device protocol machine
------------------------------------------------------
Handles the DCE/RPC UDP communication in the start up phase, especially these
messages:

* connect
* release
* DControl ("Parameter end" is sent to IO-Device)
* CControl ("Application ready" is sent to IO-Controller)
* parameter read (Uses CMRDR)
* parameter write

Incoming UDP packets are parsed by ``pf_cmrpc_dce_packet()``, which also
prepares the return UDP packet. This is done by putting together incoming
fragments and then calling ``pf_cmrpc_rpc_request()``.

On DCE RPC connect requests the function ``pf_cmrpc_rm_connect_ind()`` is
called, and it will create a DCE RPC connect response. It will also trigger
these user callbacks:

 * ``pnet_exp_module_ind()``
 * ``pnet_exp_submodule_ind()``
 * ``pnet_connect_ind()``
 * ``pnet_state_ind()`` with PNET_EVENT_STARTUP

The function ``pf_cmrpc_rm_write_ind()`` is called for incoming (parameter)
write request messages, and it will trigger the ``pnet_write_ind()`` user
callback for certain parameters.
Other parameters are handled by the stack itself.

Incoming control (DControl) requests are handled by
``pf_cmrpc_rm_dcontrol_ind()`` which typically triggers these user callbacks:

* ``pnet_dcontrol_ind()`` with PNET_CONTROL_COMMAND_PRM_END
* ``pnet_state_ind()`` with PNET_EVENT_PRMEND

When the IO-device is sending a request to an IO-Controller (and expects a
response) a new separate session is started.

Incoming CControl responses are handled by ``pf_cmrpc_rm_ccontrol_cnf()``,
which will trigger these user callbacks:

* ``pnet_state_ind()`` with PNET_EVENT_DATA.
* ``pnet_ccontrol_cnf()``

Show current details on the CMRPC state machine::

   pf_cmrpc_show(0xFFFF);


DCP
---
Handles these DCP messages:

* Set
* Get
* Ident
* Hello

Flashes a LED on reception of the "Set request" DCP message with suboption
"Signal".


CMINA - Context Management Ip and Name Assignment protocol machine
------------------------------------------------------------------
This state machine is responsible for assigning station name and IP address.
Does factory reset when requested by IO-controller.

States:

* SETUP
* SET_NAME
* SET_IP
* W_CONNECT

Helps handling DCP Set and DCP Get requests.


CMRDR - Context Management Read record Responder protocol machine
-----------------------------------------------------------------
Contains a single function ``pf_cmrdr_rm_read_ind()``, that handles
RPC parameter read requests.

Triggers the ``pnet_read_ind()`` user callback for some values.
Other values, for example the Identification & Maintenance (I&M)
values, are handled internally by the stack.

This state machine is used by CMRPC.


CMWRR - Context Management Write Record Responder protocol machine
------------------------------------------------------------------
Handles RPC parameter write requests.
Triggers the ``pnet_write_ind()`` user callback for some values.


CMDEV - Context Management protocol machine Device
--------------------------------------------------
This handles connection establishment for IO-Devices.

For example pulling and plugging modules and submodules in slots and
subslots are done in this file. Also implements handling connect, release,
CControl and DControl.

States:

* POWER_ON, Data initialization. (Must be first)
* W_CIND, Wait for connect indication (in the connect UDP message)
* W_CRES, Wait for connect response from app and CMSU startup.
* W_SUCNF, Wait for CMSU confirmation.
* W_PEIND, Wait for PrmEnd indication (in the DControl UDP message)
* W_PERES, Wait for PrmEnd response from app.
* W_ARDY, Wait for app ready from app.
* W_ARDYCNF, Wait for app ready confirmation from controller.
* WDATA, Wait for established cyclic data exchange.
* DATA, Data exchange and connection monitoring.
* ABORT, Abort application relation.

Implements these user functions (via ``pnet_api.c``):

* ``pnet_plug_module()``
* ``pnet_plug_submodule()``
* ``pnet_pull_module()``
* ``pnet_pull_submodule()``
* ``pnet_application_ready()`` Triggers the ``pnet_state_ind()`` user callback with PNET_EVENT_APPLRDY.
* ``pnet_ar_abort()``

Show the plugged modules and sub-modules, and number of bytes sent and received
for subslots::

   pf_cmdev_show_device();

Show current state for CMDEV state machine::

   pf_cmdev_show(p_ar);


CMSM - Context Management Surveillance protocol Machine
-------------------------------------------------------
The CMSM component monitors the establishment of a connection. Once the
device enters the DATA state this component is done.

This is basically a timer, which has two states; IDLE and RUN. If not stopped
before it times out, the stack will enter PNET_EVENT_ABORT state.
The timer returns to state IDLE at timeout. Typically the timeout setting is
around 20 seconds (can be adjusted by the IO-Controller).

The timer is started on PNET_EVENT_STARTUP (at the connect request message),
and stopped at PNET_EVENT_DATA.

It also monitors these response and indication messages:

* Read
* Write
* DControl

It starts the timer at sending the "response" message, and stops the timer
when the "indication" message is received.


CPM - Consumer Protocol Machine
-------------------------------
Receives cyclic data. Monitors that the incoming data fulfills the protocol,
and that the timing of incoming frames is correct. Stores incoming data into a
buffer.

Several instances of CPM can be used in parallel.

States:

* W_START Wait for initialization
* FRUN
* RUN Running

If there is a timeout in the RUN state, it will transition back to state
W_START.

Implements these user functions (via ``pnet_api.c``):

* ``pnet_output_get_data_and_iops()``
* ``pnet_input_get_iocs()``

Triggers the ``pnet_new_data_status_ind()`` user callback on data status
changes (not on changes in the data itself).


PPM - Provider Protocol Machine
-------------------------------
Sends cyclic data.

States:

* W_START
* RUN

Implements these user functions (via ``pnet_api.c``):

* ``pnet_input_set_data_and_iops()``
* ``pnet_output_set_iocs()``
* ``pnet_set_state()``
* ``pnet_set_redundancy_state()``
* ``pnet_set_provider_state()``


Block reader and writer
-----------------------
The files ``pf_block_reader.c`` and ``pf_block_writer.c`` implement functions
for parsing and writing data in buffers.


ETH
---
Registers and invokes frame handlers for incoming raw Ethernet frames.


LLDP - Link Layer Discovery Protocol
------------------------------------
A protocol for neighborhood detection.
An LLDP frame is sent at startup, to indicate the IO-Device IP address etc.

The LLDP frame is a layer 2 Ethernet frame with the payload consting of a number
of Type-Length-Value (TLV) blocks. The first 16 bits of each block contains info
on the block type and block payload length. It is followed by the block payload
data. Different TLV block types may have subtypes defined (within the payload).

The frame is broadcasted to MAC address 01:80:c2:00:00:0e, and has an Ethertype
of 0x88cc.

TLV types:

* 0: (End of LLDP frame indicator)
* 1: Chassis ID. Subtypes 4: MAC address. 7: Name
* 2: Port ID. Subtype 7: Local
* 3: Time to live in seconds
* 8: Management address (optional for LLDP, mandatory in Profinet). Includes IP
  address and interface number.
* 127: Organisation specific (optional for LLDP. See below.). Has an
  organisation unique code, and a subtype.

Organisation unique code 00:0e:cf belongs to Profibus Nutzerorganisation, and
supports these subtypes:

* 2: Port status. Contains RTClass2 and RTClass3 port status.
* 5: Chassis MAC address

Organisation unique code 00:12:0f belongs to the IEEE 802.3 organisation, and
supports these subtypes:

* 1: MAC/PHY configuration status. Shows autonegotiation support, and which
  speeds are supported. Also MAU type.

Autonegotiation:

* Bit 0: Supported
* Bit 1: Enabled

Speed:

* Bit 0: 1000BASE-T Full duplex
* Bit 1: 1000BASE-T Half duplex
* Bit 10: 100BASE‑TX Full duplex
* Bit 11: 100BASE‑TX Half duplex
* Bit 13: 10BASE‑T Full duplex
* Bit 14: 10BASE‑T Half duplex
* Bit 15: Unknown speed


Start up procedure
------------------

+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
| | Incoming       | | Outgoing         | | CMDEV               |  Application                               | Other                               |
| | message        | | message          | | state               |                                            |                                     |
+==================+====================+=======================+============================================+=====================================+
| Connect req      |                    |                       |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | pnet_exp_module_ind()                      |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | pnet_exp_submodule_ind()                   |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | pnet_connect_ind()                         |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    | W_CRES                |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       |                                            | PPM starts sending cyclic data      |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       |                                            | PF_CPM_STATE_FRUN                   |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    | W_SUCNF               |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | pnet_connect_ind() with PNET_EVENT_STARTUP |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       |                                            | CMSM timer started                  |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | pnet_new_data_status_ind()                 |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       |                                            | PF_CPM_STATE_RUN                    |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    | W_PEIND               |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  | Connect resp       |                       |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
| Write req        |                    |                       |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | pnet_write_ind()                           |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  | Write resp         |                       |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
| DControl req     |                    |                       |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    | W_PERES               |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | pnet_dcontrol_ind()                        |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    | W_ARDY                |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    | (PRMEND)              |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | pnet_connect_ind() with PNET_EVENT_PRMEND  |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | Run pnet_input_set_data_and_iops()         |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  | DControl resp      |                       |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | Run pnet_application_ready()               |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    | (APPLRDY)             |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | pnet_connect_ind() with PNET_EVENT_APPLRDY |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  | CControl req       |                       |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    | W_ARDYCNF             |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
| CControl resp    |                    |                       |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    | WDATA                 |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | pnet_ccontrol_cnf()                        |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    |                       | pnet_connect_ind() with PNET_EVENT_DATA    |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+
|                  |                    | DATA                  |                                            |                                     |
+------------------+--------------------+-----------------------+--------------------------------------------+-------------------------------------+


Useful functions
----------------
Show lots of details of the stack state::

   pnet_show(net, 0xFFFF);


Coding rules
------------
In order to be platform independent, use ``CC_ASSERT()`` instead of ``assert()``.

Include headers in sorted groups in this order:

* Interface header (Corresponding to the .c file)
* Headers from same project
* Headers from the operating system
* Standard C headers
