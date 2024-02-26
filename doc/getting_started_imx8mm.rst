.. _getting-started-imx8mm:

Getting started using i.MX 8M Mini Cortex-M / FreeRTOS / lwIP
=============================================================

In this tutorial, the P-Net Profinet device stack and its sample application runs on the `i.MX 8M Mini EVK <https://www.nxp.com/design/design-center/development-boards/i-mx-evaluation-and-development-boards/evaluation-kit-for-the-i-mx-8m-mini-applications-processor:8MMINILPD4-EVK>`_ board. The i.MX 8M Mini EVK has four Cortex-A53 and one Cortex-M4. The current tutorial describes how to run the application on Cortex-M4. If you want to run the P-Net Profinet device stack on the Cortex-A53 cores, please follow the Linux tutorial.

A Windows PC will be used as a PLC (Programmable Logic Controller = IO controller) running Codesys soft PLC.

Additional information about this setup can be found in the `NXP AN13933 <https://www.nxp.com/docs/en/application-note/AN13933.pdf>`_.

Hardware and software requirements
----------------------------------
Necessary hardware:
^^^^^^^^^^^^^^^^^^^
* 1 i.MX 8M Mini as IO-device + 1 32GB SD card
* 1 Windows PC as IO-controller
* 1 Ethernet cable
* 1 LED and 1 push button for testing a digital-out and a digital-in port on the i.MX 8M Mini running as IO-device.

Sources:
^^^^^^^^
The ``p-net/samples/pn_dev`` directory in the P-Net repository contains the source code for this example. It also contains a GSD file (written in GSDML), which tells the IO-controller how to communicate with the IO-device.

Those parts of the sample application that are dependent on whether you run
Linux or an RTOS are located in ``p-net/src/ports``.

Board specific files can be found in the ``p-net/osal/src/freertos/iMX8MM`` directory. The `osal <https://github.com/rtlabs-com/osal>`_ directory will be downloaded during build.

Prerequisites
-------------
Hardware setup
^^^^^^^^^^^^^^

#. Connect the LED and the push button to the i.MX 8M Mini, following the schematic on the left. For reference, the physical setup can be seen on the right side:
    
   .. image:: illustrations/iMX8MMHwSetup.png

#. Connect an ethernet cable between the i.MX 8M Mini EVK and the Windows PC.

#. Connect the i.MX 8M Mini board to the Windows PC via USB cable between the DEBUG USB-UART connector and the PC USB connector.

   The ports will show with consecutive numbers in the Device Manager. Typically, the first port is allocated for Cortex-M4, while the second one is allocated for the Cortex-A53. 

   Open both serial devices in your preferred serial terminal emulator (ex. PuTTY). Set the speed to 115200 bps, 8 data bits, 1 stop bit (115200, 8N1), and no parity.

Software setup
^^^^^^^^^^^^^^
Windows PC setup
''''''''''''''''
#. Download and install the CODESYS Development System. Download the Windows-based software `CODESYS Development System V3 <https://store.codesys.com/en/codesys.html>`_. For this demo, CODESYS Development System V3.5.19 is used.

#. Implement the PLC Demo IO-Controller

   .. note::
       Keep in mind that the IP address of the PC's interface must be set manually in the same subnet with that of the i.MX 8M Mini. The IP address of the board is ``192.168.11.3``. If you want to replace this IP address, you can change the configuration in the ``osal/src/freertos/iMX8MM/Src/main.c`` file.

   * To create the Codesys project on the Windows PC, please follow Section 3.1 from `AN13933 <https://www.nxp.com/docs/en/application-note/AN13933.pdf>`_.

   * To configure the Codesys project, follow the below steps:
   
     #. In the CODESYS menu :guilabel:`Tools`, select :guilabel:`Device Repository`, click :guilabel:`Install`, and then select the `GSDML <https://github.com/rtlabs-com/p-net/blob/master/samples/pn_dev/GSDML-V2.4-RT-Labs-P-Net-Sample-App-20220324.xml>`_ file.
      
     #. Add the following devices:
         * Right-click :guilabel:`Device (<variant>)` from the left panel and select :menuselection:`Add Device --> Fieldbuses --> Ethernet Adapter --> Ethernet`.
         * Right-click :guilabel:`Ethernet` and select :menuselection:`Add Device --> Profinet IO --> Profinet IO Master --> PN-Controller`.
         * Right-click :guilabel:`PN_Controller` and select :menuselection:`Add Device --> Profinet IO Slave --> I/O --> P-Net Sample --> P-Net multi-module sample app`.
         * Right-click :guilabel:`P_Net_multi_module_sample_app` and select :menuselection:`Add Device --> Profinet IO Module --> DIO 8xLogicLevel`
         
     #. Double-click the :guilabel:`Ethernet` node in the left menu and select the network interface with the ``192.168.11.2`` IP address. The IP address is automatically updated.
      
     #. Double-click the :guilabel:`PN_controller` node in the left menu and set the First IP and the Last IP to both the existing IP address of the IO-device. In this use case, it is ``192.168.11.3``. Set the default gateway to ``192.168.11.1``.

     #. Double-click the :guilabel:`P_Net_multi_module_sample_app` node in the left menu and set the IP address to the existing address of the IO-device. In this use case, it is ``192.168.11.3``. Set the default gateway to ``192.168.11.1``.
      
   * To create the Codesys application, follow :ref:`using-codesys`, Section ``Creating a controller application``.
   
   * To build the application, in the top menu, use :menuselection:`Build --> Generate Code`.

#. In order to cross-compile the P-Net stack and its application in Windows, the WSL + Ubuntu 22.04 LTS must be configured and installed. Alternatively, a separate Ubuntu PC/virtual machine can be used.

#. In Ubuntu, install the ARM embedded toolchain. ::

    mkdir ~/gcc_compiler
    cd ~/gcc_compiler
    wget -v https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
    tar -xf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2
    
   Create a new system environment variable and name it ``ARMGCC_DIR``. The value of this variable should point to the Arm GCC embedded toolchain installation path. For this example, the path is ``~/gcc_compiler/gccarm-none-eabi-10.3-2021.10``. Add the below line to ``~/.bashrc`` file::

    export ARMGCC_DIR=~/gcc_compiler/gcc-arm-none-eabi-10.3-2021.10
   
   To reload bashrc settings, run::
   
    source ~/.bashrc
    
i.MX 8M Mini setup
''''''''''''''''''
#. Download and flash the precompiled `Real Time Edge Software <https://www.nxp.com/design/design-center/software/development-software/real-time-edge-software:REALTIME-EDGE-SOFTWARE>`_ image on the SD card.  

   .. note:: 
      You can use any SD card writer tool, like `Win32 Disk Imager <https://win32diskimager.org/>`_ or `Balena Etcher <https://etcher.balena.io/>`_ to flash the precompiled image ``nxp-image-real-time-edge-imx8mm-lpddr4-evk.wic``.

Build the application
^^^^^^^^^^^^^^^^^^^^^
  
#. Clone the MCUXpresso SDK firmware

   MCUXpresso SDK is a comprehensive software enablement package designed to simplify and accelerate application development with Arm® Cortex®-M-based devices from NXP.

   You need to have both Git and West installed, then execute below commands to achieve the whole SDK delivery at revision **MCUX_2.12.0** and place it in a folder named **mcuxsdk**::

		west init -m https://github.com/NXPmicro/mcux-sdk --mr MCUX_2.12.0 mcuxsdk
		cd mcuxsdk
 		west update
    
#. LwIP is not supported by default on the Cortex-M of i.MX 8M Mini EVK board. Some patches must be applied to add the lwIP support for i.MX 8M Mini.

   * Download the lwIP stack and place it into the ``mcuxsdk/middleware`` directory::

        cd ~/mcuxsdk/middleware
        git clone https://github.com/lwip-tcpip/lwip.git
        cd lwip
        git checkout 239918ccc173cb2c2a62f41a40fd893f57faf1d6
    
   * Download the `imx8m_lwip_port.patch <https://github.com/nxp-imx-support/lwip_demo/blob/master/imx8m_lwip_port.patch>`_ patch and apply it to the lwip directory. This fetches the port support for i.MX 8M (bare-metal lwIP and with FreeRTOS)::

        cd ~/mcuxsdk/middleware/lwip
        wget https://raw.githubusercontent.com/nxp-imx-support/lwip_demo/master/imx8m_lwip_port.patch
        git apply --whitespace=nowarn imx8m_lwip_port.patch

#. Download and compile the P-Net

   * Clone the source::
    
        cd
        git clone --recurse-submodules https://github.com/rtlabs-com/p-net.git

   * Configure the CPU, board and path to the cloned git repository::

		cd ~/p-net
		cmake -B build.imx8mm -DBOARD=iMX8MM \
			-DMCUXSDK_DIR=<path_to_mcuxsdk> \
			-DCMAKE_TOOLCHAIN_FILE="cmake/tools/toolchain/imx8mm.cmake" \
 			-DCMAKE_BUILD_TYPE=ddr_release -DBUILD_TESTING=OFF -G "Unix Makefiles"
 		cmake --build build.imx8mm

#. When the build completes you can find the sample-app binary in
   ``build.imx8mm/pn_dev.bin``. 
  
Deploy and run the binary on the target
---------------------------------------
 
#. Start the controller application.

   * Start the application by using the top menu :menuselection:`Online --> Login`. Press :guilabel:`Yes` in the pop-up window.
   * In the top menu, use :menuselection:`Debug --> Start`.

#. Copy the binary on the SD card

   Insert the SD card into the PC and copy the resulted binary ``pn_dev.bin`` on the first (FAT) partition of the SD card.

#. Insert the SD card into the i.MX 8M Mini's slot, boot the board and stop the execution in U-Boot. To write and boot the binary from DDR, use the following commands::

    u-boot=> fatload mmc 1:1 0x80000000 pn_dev.bin
    u-boot=> dcache flush
    u-boot=> bootaux 0x80000000
