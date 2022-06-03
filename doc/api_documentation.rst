API documentation
=================
Here is a brief overview given for some of the most important functions,
structs etc in the public API for the p-net Profinet stack.

For detailed documentation, read the ``include/pnet_api.h`` header file. It
also contains error code definitions etc.

You can also build the Doxygen documentation for all functions by following
the instructions given on the page "Additional Linux Details".

Study samples/pn_dev to get more information on how to use the API.


General
-------
All functions return either ``0`` (zero) for a successful call or ``-1`` for an
unsuccessful call.


Initialization and handling
---------------------------
.. doxygenfunction:: pnet_init
.. doxygenfunction:: pnet_application_ready
.. doxygenfunction:: pnet_handle_periodic
.. doxygenfunction:: pnet_get_ar_error_codes
.. doxygenfunction:: pnet_ar_abort
.. doxygenfunction:: pnet_factory_reset
.. doxygenfunction:: pnet_show


Plug and pull modules/submodules
--------------------------------
.. doxygenfunction:: pnet_plug_module
.. doxygenfunction:: pnet_plug_submodule
.. doxygenfunction:: pnet_pull_module
.. doxygenfunction:: pnet_pull_submodule


Periodic data
-------------
.. doxygenfunction:: pnet_input_set_data_and_iops
.. doxygenfunction:: pnet_input_get_iocs
.. doxygenfunction:: pnet_output_get_data_and_iops
.. doxygenfunction:: pnet_output_set_iocs
.. doxygenfunction:: pnet_set_provider_state


Redundant state etc
-------------------
.. doxygenfunction:: pnet_set_primary_state
.. doxygenfunction:: pnet_set_redundancy_state


Alarms and diagnostics
----------------------
.. doxygenfunction:: pnet_create_log_book_entry
.. doxygenfunction:: pnet_alarm_send_process_alarm
.. doxygenfunction:: pnet_alarm_send_ack
.. doxygenfunction:: pnet_diag_std_add
.. doxygenfunction:: pnet_diag_std_update
.. doxygenfunction:: pnet_diag_std_remove

.. doxygenfunction:: pnet_diag_usi_add
.. doxygenfunction:: pnet_diag_usi_update
.. doxygenfunction:: pnet_diag_usi_remove


Low-level diagnostic functions
------------------------------
These are used internally in the functions above for handling diagnosis in
standard or in USI format. However they can be useful in situations where
detailed control is required.

.. doxygenfunction:: pnet_diag_add
.. doxygenfunction:: pnet_diag_update
.. doxygenfunction:: pnet_diag_remove


Callbacks
---------
The application should define call-back functions which are called by
the stack when specific events occurs within the stack.

Note that most of these functions are mandatory in the sense that they must
exist and return ``0`` for a functioning stack. Some functions are required
to perform specific functionality.

.. doxygentypedef:: pnet_connect_ind
.. doxygentypedef:: pnet_release_ind
.. doxygentypedef:: pnet_dcontrol_ind
.. doxygentypedef:: pnet_ccontrol_cnf
.. doxygentypedef:: pnet_state_ind
.. doxygentypedef:: pnet_reset_ind
.. doxygentypedef:: pnet_signal_led_ind
.. doxygentypedef:: pnet_read_ind
.. doxygentypedef:: pnet_write_ind
.. doxygentypedef:: pnet_exp_module_ind
.. doxygentypedef:: pnet_exp_submodule_ind
.. doxygentypedef:: pnet_new_data_status_ind
.. doxygentypedef:: pnet_alarm_ind
.. doxygentypedef:: pnet_alarm_cnf
.. doxygentypedef:: pnet_alarm_ack_cnf


Selected enums
--------------
.. doxygenenum:: pnet_event_values
.. doxygenenum:: pnet_ioxs_values
.. doxygenenum:: pnet_submodule_dir
.. doxygenenum:: pnet_control_command
.. doxygenenum:: pnet_data_status_bits
.. doxygenenum:: pnet_diag_ch_prop_type_values
.. doxygenenum:: pnet_diag_ch_prop_dir_values
.. doxygenenum:: pnet_diag_ch_prop_maint_values
.. doxygenenum:: pnet_diag_ch_group_values
.. doxygenenum:: pnet_im_supported_values
.. doxygenenum:: pnal_eth_mau


Selected structs
----------------
Network and device configuration.

Configuration of the stack is performed by transferring a structure
in the call to ``pnet_init()``.

Along with the configuration the initial (default) values of the
I&M data records are conveyed as well as the values used for
sending LLDP frames.

.. doxygenstruct:: pnet_im_0
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_im_1
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_im_2
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_im_3
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_im_4
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_cfg_device_id
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_if_cfg
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_port_cfg
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_ip_cfg
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_cfg
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_alarm_spec
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_alarm_argument
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_diag_source
   :members:
   :undoc-members:
