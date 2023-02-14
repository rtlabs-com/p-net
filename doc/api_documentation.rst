.. _api-documentation:

API documentation
=================
Here is a brief overview given for some of the most important functions,
structs etc in the public API for the P-Net Profinet stack.

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
.. doxygenfunction:: pnet_sm_released_cnf


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
.. doxygentypedef:: pnet_sm_released_ind


Selected enums
--------------
.. doxygenenum:: pnet_event_values_t
.. doxygenenum:: pnet_ioxs_values_t
.. doxygenenum:: pnet_submodule_dir_t
.. doxygenenum:: pnet_control_command_t
.. doxygenenum:: pnet_data_status_bits_t
.. doxygenenum:: pnet_diag_ch_prop_type_values_t
.. doxygenenum:: pnet_diag_ch_prop_dir_values_t
.. doxygenenum:: pnet_diag_ch_prop_maint_values_t
.. doxygenenum:: pnet_diag_ch_group_values_t
.. doxygenenum:: pnet_im_supported_values_t
.. doxygenenum:: pnal_eth_mau_t


Selected structs
----------------
Network and device configuration.

Configuration of the stack is performed by transferring a structure
in the call to ``pnet_init()``.

Along with the configuration the initial (default) values of the
I&M data records are conveyed as well as the values used for
sending LLDP frames.

.. doxygenstruct:: pnet_im_0_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_im_1_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_im_2_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_im_3_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_im_4_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_cfg_device_id_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_if_cfg_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_port_cfg_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_ip_cfg_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_cfg_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_alarm_spec_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_alarm_argument_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_diag_source_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_result_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_data_cfg_t
   :members:
   :undoc-members:

.. doxygenstruct:: pnet_pnio_status_t
   :members:
   :undoc-members:
