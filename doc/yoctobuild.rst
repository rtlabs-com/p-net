.. _yoctobuild:

Building P-Net for Linux using Yocto or Buildroot
=================================================

Yocto
-----
A Yocto layer for building P-Net on embedded Linux is available
on https://github.com/rtlabs-com/meta-rtlabs. Usage details are given there.

Add ``p-net-demo`` to your image to include the Profinet IO-device sample
application.


Buildroot
---------
It is possible to use Buildroot for including P-Net in your embedded Linux
product.

A Buildroot recipe for P-Net is available as one of the packages in
https://github.com/rtlabs-com/br2-rtlabs See the ``README.md`` file for usage
instructions. You might need to adjust ``P_NET_VERSION`` in the ``p-net.mk``
file to have the latest version.
