Implementation details
======================

State machines
--------------

* APMR        Acyclic Protocol Machine Receiver
* APMS        Acyclic Protocol Machine Sender
* ALPMI       Alarm Protocol Machine Initiator
* ALPMR       Alarm Protocol Machine Responder
* CMDEV       Context Management Protocol Machine Device, handles connection establishment for IO-Devices.
* CMRDR       Context Management Read Record Responder protocol machine device, responds to parameter read from the IO-controller
* CMRPC       Context Management RPC Device Protocol Machine, handles RPC communication
* CMSM        Context Management Surveillance Protocol Machine Device, monitors the establishment of a connection.
* CPM         Consumer Protocol Machine, for receiving cyclic data
* PPM         Cyclic Provider Protocol Machine


CMRPC
-----
Handles the UDP communication in the start up phase, especially these
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

Incoming control (dcontrol) requests are handled by
``pf_cmrpc_rm_dcontrol_ind()`` which typically triggers these user callbacks:

* ``pnet_dcontrol_ind()`` with PNET_CONTROL_COMMAND_PRM_END
* ``pnet_state_ind()`` with PNET_EVENT_PRMEND
* (``pnet_state_ind()`` with PNET_EVENT_APPLRDY)

Show current details on the CMRPC state machine::

   pf_cmrpc_show(0xFFFF);


CMRDR (used by CMRPC)
---------------------
Contains a single function ``pf_cmrdr_rm_read_ind()``, that handles a
RPC parameter read request.


CMDEV
-----
This handles connection establishment for IO-Devices.

For example pulling and plugging modules and submodules in slots and
subslots are done in this file. Also implements handling connect, release,
CControl and DControl.

States:

* PF_CMDEV_STATE_POWER_ON, Data initialization. (Must be first)
* PF_CMDEV_STATE_W_CIND, Wait for connect indication (in the connect UDP message)
* PF_CMDEV_STATE_W_CRES, Wait for connect response from app and CMSU startup.
* PF_CMDEV_STATE_W_SUCNF, Wait for CMSU confirmation.
* PF_CMDEV_STATE_W_PEIND, Wait for PrmEnd indication (in the DControl UDP message)
* PF_CMDEV_STATE_W_PERES, Wait for PrmEnd response from app.
* PF_CMDEV_STATE_W_ARDY, Wait for app ready from app.
* PF_CMDEV_STATE_W_ARDYCNF, Wait for app ready confirmation from controller.
* PF_CMDEV_STATE_WDATA, Wait for established cyclic data exchange.
* PF_CMDEV_STATE_DATA, Data exchange and connection monitoring.
* PF_CMDEV_STATE_ABORT, Abort application relation.

Show the plugged modules and sub-modules, and number of bytes sent and received
for subslots::

   pf_cmdev_show_device();

Show current state for CMDEV state machine::

   pf_cmdev_show(p_ar);


CMSM
----
The CMSM component monitors the establishment of a connection. Once the
device enters the DATA state this component is done.

This is basically a timer, which has two states; IDLE and RUN. If not stopped
before it times out, the stack will enter PNET_EVENT_ABORT state.
The timer returns to state IDLE at timeout.

The timer is started on PNET_EVENT_STARTUP (at the connect request message),
and stopped at PNET_EVENT_DATA.

It also monitors these response and indication messages:

* Read
* Write
* DControl

It starts the timer at sending the "response" message, and stops the timer
when the "indication" message is received.


Useful functions
----------------
Show lots of details of the stack state::

   pnet_show(0xFFFF);
