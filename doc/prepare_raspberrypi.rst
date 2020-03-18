Install Raspbian on the Raspberry Pi (by using a Linux laptop)
==============================================================
Download the Raspbian image from https://www.raspberrypi.org/downloads/raspbian/
Use the full version with "recommended software". Follow the instructions on
the page to burn a SD-card.

Unplug, and reinsert your SD-card to mount it. To enable SSH logging in on the
Raspberry Pi, create an empty file on the named ``ssh`` in the boot partition::

    touch ssh

If you would like to change hostname from "raspberrypi" to "pndevice-pi", change
the texts in the files ``etc/hostname`` and ``etc/hosts`` in the rootfs
partition.

To make sure that you subsequently are logging in to the correct Raspberry Pi,
you can create a file in the home directory in the rootfs partition. Change
name to something informative, for example::

    touch home/pi/ProfinetDevice

Unmount the SD-card, and plug it in into your Raspberry Pi. Power up the
Raspberry Pi.

Find the IP address of it by running this on a Linux machine on the network
(replace the hostname if you have changed it)::

    ping raspberrypi.local

Verify that it is the correct machine by checking that is disappears when the
power is disconnected.

Log in to it::

    ssh pi@<IP>

The default password is "raspberry".


Install cmake on Raspberry Pi
-----------------------------
In order to compile p-net on Raspberry Pi, you need a recent version of cmake.

In Raspbian the cmake is recent enough. Install it::

    sudo apt-get update
    sudo apt-get install cmake

Verify the installed version::

    cmake --version

Compare the installed version with the minimum version required for p-net (see first page).


Connect LED and buttons to Raspberry Pi
---------------------------------------
You need these components:

+-----------------------+-----------------+
| Component             | Number required |
+=======================+=================+
| LED                   | 1               |
+-----------------------+-----------------+
| Resistor 220 Ohm      | 3               |
+-----------------------+-----------------+
| Button switch         | 2               |
+-----------------------+-----------------+

Connect them like:

+------+---------+-------------------------------------+
| Pin  | Name    | Description                         |
+======+=========+=====================================+
| 9    | GND     |                                     |
+------+---------+-------------------------------------+
| 11   | GPIO17  | Connect LED to GND via 220 Ohm      |
+------+---------+-------------------------------------+
| 13   | GPIO27  | Connect button1 to 3.3V via 220 Ohm |
+------+---------+-------------------------------------+
| 15   | GPIO22  | Connect button2 to 3.3V via 220 Ohm |
+------+---------+-------------------------------------+
| 17   | 3.3V    |                                     |
+------+---------+-------------------------------------+

The resistors for the buttons are to limit the consequences of connecting the
wires to wrong pins.

Set up the GPIO pins::

    echo 17 > /sys/class/gpio/export
    echo 27 > /sys/class/gpio/export
    echo 22 > /sys/class/gpio/export
    echo out > /sys/class/gpio/gpio17/direction

Turn on and off the LED::

    echo 1 > /sys/class/gpio/gpio17/value
    echo 0 > /sys/class/gpio/gpio17/value

Show state of buttons::

    cat /sys/class/gpio/gpio22/value
    cat /sys/class/gpio/gpio27/value

.. image:: illustrations/RaspberryPiLedButtons.jpg


Control of built-in LEDs
------------------------
The Raspberry Pi board has LEDs on the board, typically a red PWR LED and a
green ACT (activity) LED.

Manually control the green (ACT) LED on Raspberry Pi 3::

    echo none > /sys/class/leds/led0/trigger
    echo 1 > /sys/class/leds/led0/brightness

And to turn it off::

    echo 0 > /sys/class/leds/led0/brightness

Note that you need root privileges to control the LEDs.

Similarly for the red (power) LED, which is called ``led1``.


Set static IP address permanently
---------------------------------
In order to use a static IP address instead of DHCP, modify the file
``/etc/dhcpcd.conf`` in the root file system. Insert these lines::

   interface eth0
   static ip_address=192.168.137.4/24

You can still ping the <hostname>.local address to find it on the network.
To re-enable HDCP, remove the lines again from ``/etc/dhcpcd.conf``.
