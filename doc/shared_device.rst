Shared device
=============
A device can potentially be shared between several controllers, in the sense
that the submodules of the device need not necessarily all be owned by the same
controller. On connection to a device, a controller lists the submodules it
wants to control, and if the device supports the shared device feature, the
controller is given ownership of all submodules that are not already taken by
another controller. If two controllers want to own the same submodule, the
first one to claim it will get the ownership.

If two controllers ("C1" and "C2") are connected to the device, and C1 owns a
submodule that C2 also has listed in its connect call, then if C1 disconnects,
the submodule will be released, and the ownership of it will be transferred to
C2. If there had been a third controller ("C3") which also wanted to control
the submodule, then on disconnect of C1, the ownership of the submodule would
be given to the first one of C2 and C3 to connect. In other words, submodule
ownership is handled on a first come, first served basis.

Configuration
-------------
The p-net stack supports the shared device feature if the build configuration
variable ``PNET_MAX_AR`` is set to a value greater than 1. In this case, the
application will be required to handle the ``pnet_sm_released_ind`` callback.

The p-net stack does not yet support the closely related shared input feature.

GSDML updates
-------------
In addition to increase the number of supported ARs in the build configuration
a few updates in the the GSDML file is required to support shared device.
The required settings are available but commented out in the gsdml for the
sample application. The following attributes shall be set:

* SharedDeviceSupported="true"
  (in the DeviceAccessPointItem)
* NumberOfAR="``PNET_MAX_AR``"
  (in the InterfaceSubmoduleItem/ApplicationRelations)

Controller side
---------------
PLCs using a shared device do not need to be aware of each other. If they
attempt to use the same submodules there will be a conflict as described above.

Codesys
^^^^^^^
To use the shared device feature in a Codesys project, simply check the
"Shared Device" checkbox when the device is added to the engineering project.

If you are using the the free Raspberry PI runtime you will not be able to have
several controllers in the same project. Create several Codesys projects to
configure several PLCs to test the shared device feature.
