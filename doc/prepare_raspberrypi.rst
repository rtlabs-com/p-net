Installation and configuration of Raspberry Pi
==============================================
The p-net stack and sample application
has been tested with:
* Raspberry Pi 3 Model B+

To avoid problems it is recommended to start with a fresh
Raspberry Pi OS image.

When running the Raspberry Pi as a profinet IO-device using p-net, the
network settings of the Raspberry Pi will be changed by Profinet.
Therefore it is highly recommended to use the serial console and not ssh
or you may end up in a situation were you have difficulties connecting to,
or recovering your device. If you currently do not have a suitable serial
dongle you can still build and run p-net with the sample application but
you will run into problem when a PLC is used for configuration.

Installation using Linux
------------------------
Download the Raspberry Pi OS (previously Raspbian) image from
https://www.raspberrypi.org/software/operating-systems/
Use the full version with "recommended software". Follow the instructions on
the page to burn a SD-card.

Unplug, and reinsert your SD-card to mount it. To enable SSH logging in on the
Raspberry Pi, create an empty file on the named ``ssh`` in the boot partition::

    touch ssh

To enable the serial port console add this line to the
file ``config.txt`` in the boot partition::

    enable_uart=1

The DHCP client daemon will adjust the network interface settings automatically.
This interferes with the p-net control of the Ethernet interface. So if you
run your Raspberry Pi as a Profinet IO-Device (NOT if you use it as a PLC)
and have a serial cable, you need to add this line to ``/etc/dhcpcd.conf``
in the root file system::

    denyinterfaces eth*

If you would like to change hostname from ``raspberrypi`` to ``pndevice-pi``, change
the texts in the files ``etc/hostname`` and ``etc/hosts`` in the rootfs
partition.

To make sure that you subsequently are logging in to the correct Raspberry Pi,
you can create a file in the home directory in the rootfs partition. Change
name to something informative, for example::

    touch home/pi/IAmAProfinetDevice

Unmount the SD-card, and plug it in into your Raspberry Pi. Power up the
Raspberry Pi. Log in to it via a serial cable (see below).
Use the username ``pi`` and the default password ``raspberry``.

If you do not have any serial cable (and not have disabled DCHP), connect
the Raspberry Pi to your local network.
Find the IP address of it by running this on a Linux machine on the network
(replace the hostname if you have changed it)::

    ping raspberrypi.local

Verify that it is the correct machine by checking that is disappears when the
power is disconnected.

Log in to it::

    ssh pi@<IP>

Enter the password mentioned just above.

Installation using Windows
--------------------------
This section describes how to install the Raspberry Pi OS
and how to enable ssh and serial console so that the Raspberry Pi can be
used in headless mode without a display and keyboard connected.

Step 1. Write Raspberry Pi OS image to SD card using Raspberry Pi Imager.

* Download and install Raspberry Pi Imager from
  https://www.raspberrypi.org/software/
* Start Raspberry Pi Imager
* In the Select OS dialog choose full version
* Select SD-card
* Press Write

Step 2. Initial configuration of Raspberry Pi OS.

* Eject SD-card
* Reinsert SD-card in windows PC. The SD-card will be shown as external drive named ``boot``.
* Enable ssh by creating an empty file named ``ssh`` in the root folder of ``boot``.
  The windows file explorer can be used for this.
  Note that the file ``ssh`` shall not have a txt file extension.
* Enable serial port console.
  Open ``config.txt`` in root folder of ``boot`` using Notepad.
  Add the line ``enable_uart=1`` to the end of the file.
  Save file and close Notepad.
* Eject SD-card

Step 3. Start Raspberry Pi

* Insert SD-card and power on Raspberry Pi.
* Login (preferably using serial console) with default user ``pi`` and password ``raspberry``.

Step 4. Network configuration.

Use the nano editor to edit the configuration files as described below.
For example to edit the ``/etc/dhcpcd.conf``::

    sudo nano /etc/dhcpcd.conf

Save the file in nano by pressing ``CTRL-X``, then ``Y`` and ``Enter``.

The DHCP client daemon will adjust the network interface settings automatically.
This interferes with the p-net control of the Ethernet interface. So if you
run your Raspberry Pi as a Profinet IO-Device (NOT if you use it as a PLC)
and have a serial cable, you should add the line below to ``/etc/dhcpcd.conf``::

    denyinterfaces eth*

Optionally, to change hostname from ``raspberrypi`` to ``pndevice-pi``, change
the configuration in the files ``/etc/hostname`` and ``/etc/hosts``.

To make sure that you subsequently are logging in to the correct Raspberry Pi,
you can create a file in the home directory in the rootfs partition. Change
name to something informative, for example::

    touch /home/pi/IAmAProfinetDevice

Reboot and the Raspberry Pi is now ready to run the p-net sample application.

    sudo reboot


Connect a serial cable to Raspberry Pi
--------------------------------------
The p-net Profinet stack will change the IP-address of the Raspberry Pi when
running it as an IO-Device (as requested by the PLC), why it can be
inconvenient to connect to it via ssh. Using a serial cable to connect it to
your laptop can then be helpful.

Use a USB-to-serial adapter cable with 3.3 V logic levels. For example
Adafruit sells a popular version of those cables.

If not already done, enable the serial port console by writing the line
``enable_uart=1`` in the file ``/boot/config.txt``.

The serial port within the Raspberry Pi will be named ``/dev/ttyS0``.

+-----+-----------+---------------------+-----------------------+
| Pin | Name      | Terminal on cable   | Adafruit cable color  |
+=====+===========+=====================+=======================+
| 6   | GND       | GND                 | Black                 |
+-----+-----------+---------------------+-----------------------+
| 8   | UART0_TXD | RX                  | White                 |
+-----+-----------+---------------------+-----------------------+
| 10  | UART0_RXD | TX                  | Green                 |
+-----+-----------+---------------------+-----------------------+

Use a communication program with a baud rate of 115200.

Before connecting the serial cable to your Raspberry Pi you can verify the
functionality of the cable by connecting the USB connector to your Laptop,
and connect the RX-terminal to the TX terminal of the cable. Use a communication
program to verify that text that you enter is echoed back. When removing
the RX-to-TX connection the echo should stop.


Optionally connect LEDs and buttons to Raspberry Pi
---------------------------------------------------
You need these components:

+-----------------------+-----------------+
| Component             | Number required |
+=======================+=================+
| LED                   | 2               |
+-----------------------+-----------------+
| Button switch         | 2               |
+-----------------------+-----------------+
| Resistor 220 Ohm      | 4               |
+-----------------------+-----------------+

Connect them like:

+------+---------+-----------------------------------------------------+
| Pin  | Name    | Description                                         |
+======+=========+=====================================================+
| 9    | GND     |                                                     |
+------+---------+-----------------------------------------------------+
| 11   | GPIO17  | Connect LED1 (application data) to GND via 220 Ohm  |
+------+---------+-----------------------------------------------------+
| 13   | GPIO27  | Connect Button1 to 3.3V via 220 Ohm                 |
+------+---------+-----------------------------------------------------+
| 15   | GPIO22  | Connect Button2 to 3.3V via 220 Ohm                 |
+------+---------+-----------------------------------------------------+
| 16   | GPIO23  | Connect LED2 (Profinet signal) to GND via 220 Ohm   |
+------+---------+-----------------------------------------------------+
| 17   | 3.3V    |                                                     |
+------+---------+-----------------------------------------------------+

The resistors for the buttons are to limit the consequences of connecting the
wires to wrong pins.

Set up the GPIO pins for the buttons::

    echo 22 > /sys/class/gpio/export
    echo 27 > /sys/class/gpio/export

and for the LEDs::

    echo 17 > /sys/class/gpio/export
    echo 23 > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio17/direction
    echo out > /sys/class/gpio/gpio23/direction

Turn on and off a LED::

    echo 1 > /sys/class/gpio/gpio17/value
    echo 0 > /sys/class/gpio/gpio17/value

Show state of buttons::

    cat /sys/class/gpio/gpio22/value
    cat /sys/class/gpio/gpio27/value

.. image:: illustrations/RaspberryPiLedButtons.jpg


Adjust IP address if using the Raspberry Pi as a PLC
----------------------------------------------------
If running your Raspberry Pi as a PLC (Profinet IO-Controller). you would like
to have a static IP address (it will not work if running as a Profinet IO-Device).
Instead modify the file ``/etc/dhcpcd.conf`` to include these lines::

   interface eth0
   static ip_address=192.168.0.100/24

You can still ping the <hostname>.local address to find it on the network.
To re-enable DHCP, remove the lines again from ``/etc/dhcpcd.conf``.


Advanced users only: Automatic start of sample application
----------------------------------------------------------
Use systemd to automatically start the p-net sample application on startup.
Place a systemd unit file here: ``/lib/systemd/system/pnet-sampleapp.service``

An example file is available in the ``sample_app/`` directory of this
repository. It assumes that the code is checked out into
``/home/pi/profinet/p-net/`` on your Raspberry Pi.
Install the file::

    sudo cp /home/pi/profinet/p-net/sample_app/pnet-sampleapp.service /lib/systemd/system/

Adapt the contents to your paths and hardware.

Enable automatic startup::

    sudo systemctl daemon-reload
    sudo systemctl enable pnet-sampleapp.service

Start service::

    sudo systemctl start pnet-sampleapp.service

To see the status of the process, and the log output::

    systemctl status pnet-sampleapp.service

    journalctl -u pnet-sampleapp -f

If using a serial cable, you might need to adjust the number of visible columns::

    stty cols 150 rows 40


Advanced users only: Control of built-in LEDs
---------------------------------------------
The Raspberry Pi board has LEDs on the board, typically a red PWR LED and a
green ACT (activity) LED.

Manually control the green LED (ACT = ``led0``) on Raspberry Pi 3::

    echo none > /sys/class/leds/led0/trigger
    echo 1 > /sys/class/leds/led0/brightness

And to turn it off::

    echo 0 > /sys/class/leds/led0/brightness

Note that you need root privileges to control the LEDs.

Similarly for the red (power) LED, which is called ``led1``.
