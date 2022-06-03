Creating GSD (GSDML) files
==========================
A GSD (General Station Description) file is an XML file describing a
Profinet IO-Device. The XML-based language is called GSDML (GSD Markup Language).

A single GSDML file describes a single VendorID and ProductID combination.
A family of devices can be described in the same file; Then they will share
the ProductID but have different DAP (head module) ID values.

For Profinet members, the "Profinet GSD Checker" tool is available for
download. It contains GSD example files showing different aspects of the file
format.

To create a custom GSD file, start from one of the examples. Edit the file in
an XML editor. Then run the resulting file in the "Profinet GSD Checker" tool
to verify its structure.

Below is a brief introduction to the most common elements and attributes in
GSDML files given. It contains simplifications and does not describe special
cases. For full details on GSDML file syntax, see the official GSDML
specification.

An example GSDML file is available in the samples/pn_dev directory of the p-net
repository: https://github.com/rtlabs-com/p-net/tree/master/samples/pn_dev

A GSDML file is used to describe an IO-device. The corresponding file
for describing a controller is a CDML (Controller Description Markup Language)
file (with file ending ``.xml``). The CDML is used during controller certification
(not discussed here).


XML editors
-----------
It can be helpful to use an XML editor when working with GSD files. A few
examples are given here.

**QXmlEdit**:
http://qxmledit.org/ (Windows and Linux)
It can be installed via Ubuntu Software Store.

**CAM editor**:
http://camprocessor.sourceforge.net/wiki/ (Windows and Linux)

**XML Notepad**:
https://github.com/microsoft/xmlnotepad (Windows)


Using "Profinet GSD Checker" tool on a Windows PC
-------------------------------------------------
The tool is available for Profinet members.

The file is part of the conformance test tool download. Unzip the file
PROFINET_GSD_Checker_2.34.zip, and double-click the "setup" file. After
installation, enter ``profinet`` in the Windows search bar to start the program.

Use the menu File > Open and select your GSD file. An overview of the file
is shown. In the left menu different parts of the file can be selected.

To verify the file, use the menu File > Check. Click the "GSDCheck" bar at
the bottom of the screen to open a document check view in the right part of
the screen. Use the Icons to show the details of any error messages.

If you later need to uninstall the tool, do that via Windows settings > Apps &
features.


File name
---------
The current GSD version (as of writing) is 2.4.

The file name format is specified in section 5.1 of the GSD specification. For example
``GSDML-V2.35-Vendor-Device-20171231.xml``. You can give it an optional timestamp
in the filename: ``-20171231-235900.xml``.

Note that version for example 2.3 is newer than version 2.25. From the GSDML
specification: "V2.2 < V2.25 < V2.3 < V2.31"


In-file comments
----------------

.. highlight:: xml

XML comments are written::

   <!-- Comment text -->

   <!-- Multi-line
   comment
   text -->

.. highlight:: none


Entering values
---------------
Values are written in double quotes. Different attributes have different types:

* ValueList: To enter several values (including a range of values), write ``"0..4 7 9"``.
* TokenList1: Semicolon separated values, for example ``"Legacy;Advanced"``.


File structure
--------------
The GSD files are structured like::

    ISO15745Profile
    |
    +--ProfileHeader
    +--ProfileBody
       |
       +--DeviceIdentity
       |  |
       |  +--InfoText
       |  +--VendorName
       |
       +--DeviceFunction
       |  |
       |  +--Family
       |
       +--ApplicationProcess
          |
          +--DeviceAccessPointList
          +--ModuleList
          +--SubmoduleList
          +--ValueList
          +--LogBookEntryList
          +--CategoryList
          +--ChannelDiagList
          +--ChannelProcessAlarmList
          +--UnitDiagTypeList
          +--GraphicsList
          +--ExternalTextList

The encoding is given in the first line of the XML file. Use ``utf-8`` if using non-ASCII characters in any of the text fields.
Do not change the ``<ProfileHeader>`` contents.

In the ``<DeviceIdentity>`` adjust the attributes ``VendorID``, ``DeviceID``.
Adjust the ``Value`` in ``<VendorName>``.
In the ``<Family>`` adjust the attributes for ``MainFamily`` (typically "I/O", should be from the list of allowed values)
and ``ProductFamily`` which is vendor specific.

Those values are for example used by the engineering tool to display the device
in the hardware catalog. The catalog is typically sorted by
"Profinet IO"/<MainFamily>/<VendorName>/<ProductFamily>/<IDT_MODULE_NAME_DAP1>

Details on the DeviceAccessPoint
--------------------------------
This part of the file deals mainly with communication settings.

DeviceAccessPointList element hierarchy::

    DeviceAccessPointList
        |
        +--DeviceAccessPointItem
           |
           +--ModuleInfo
           |  |
           |  +--Name
           |  +--InfoText
           |  +--VendorName
           |  +--OrderNumber
           |  +--HardwareRelease
           |  +--SoftwareRelease
           |
           +--CertificationInfo
           +--SubslotList
           |  |
           |  +--SubslotItem
           |
           +--IOConfigData
           +--UseableModules
           |  |
           |  +--ModuleItemRef
           |
           +--ARVendorBlock
           |  |
           |  +--Request
           |     |
           |     +--Const
           |
           +--VirtualSubmoduleList
           |  |
           |  +--VirtualSubmoduleItem
           |     |
           |     +--ModuleInfo
           |     |  |
           |     |  +--Name
           |     |  +--InfoText
           |     |
           |     +--IOData
           |     +--RecordDataList
           |        |
           |        +--ParameterRecordDataItem
           |           |
           |           +--Name
           |           +--Const
           |           +--Ref
           |
           +--SystemDefinedSubmoduleList
           |  |
           |  +--InterfaceSubmoduleItem
           |  |  |
           |  |  +--ApplicationRelations
           |  |     |
           |  |     +--TimingProperties
           |  |
           |  +--PortSubmoduleItem
           |     |
           |     +--MAUTypeList
           |        |
           |        +--MAUTypeItem
           |
           +--Graphics
              |
              +--GraphicItemRef

The ``<DeviceAccessPointItem>`` element has the attributes:

* ``ID="IDD_1"``
* ``PNIO_Version="V2.4"`` Which version of Profinet specification it is
  certified against.
* ``PhysicalSlots="0..4"`` Slot 0 is always used by the DAP (bus interface)
  module. Relates to the ``PNET_MAX_SLOTS`` value in the p-net stack.
* ``ModuleIdentNumber="0x00000001"`` Unsigned32hex.
* ``MinDeviceInterval="32"`` Minimum cyclic data update interval, in number
  of 31.25 us ticks. A value 32 corresponds to cyclic data sending and
  receiving every millisecond. Unsigned16. It should match the value
  ``min_device_interval`` in the p-net configuration. (It must be possible to
  achieve this time using the values in the ``SendClock`` and ``ReductionRatio``
  attributes in another element).
* ``DNS_CompatibleName="pno-example-dap"`` (Default station name)
* ``FixedInSlots="0"`` The DAP module is always in slot 0
* ``ObjectUUID_LocalIndex="0"``
* ``DeviceAccessSupported="false"`` If a limited version of AR connection is allowed.
* ``NumberOfDeviceAccessAR="1"`` Number of Device Access connections. Should only
  be given if ``DeviceAccessSupported`` is ``true``. Dependent on the ``PNET_MAX_AR``
  value in the p-net stack.
* ``MultipleWriteSupported="true"`` Multiple writes in a single request.
  Mandatory ``true`` since V2.31.
* ``SharedDeviceSupported="false"`` False if not given
* ``SharedInputSupported="false"`` False if not given
* ``RequiredSchemaVersion="V2.3"`` This file has features requiring this schema
  version. It must be at least 2.3 if legacy startup mode not is supported.
* ``CheckDeviceID_Allowed="true"`` If the VendorID and DeviceID are fine grained
  enough to verify that the same type of device is used at replacement.
* ``NameOfStationNotTransferable="false"``
* ``LLDP_NoD_Supported="true"`` Mandatory ``true`` since V2.31.
* ``ResetToFactoryModes="1..2"`` Bits describing reset possibilities. At least
  "2" should be present. Reset modes 1 and 2 are supported by p-net.
* ``ParameterizationSpeedupSupported="true"`` For fast startup.
* ``PowerOnToCommReady="700"`` For fast startup, time to first data exchange
  in milliseconds. Unsigned32.
* ``AddressAssignment="DCP"`` Can also be ``"DHCP"`` and ``"LOCAL"``. Defaults
  to DCP if not given.

General info on the Profinet IO-Device is given in ``<ModuleInfo>``
subelements. For example the vendor name and order number are given.

The ``<CertificationInfo>`` element has the attributes:

* ``ConformanceClass="B"``
* ``ApplicationClass=""`` Typically empty, but can be for example "FunctionalSafety"
* ``NetloadClass="I"``

With ``<SubslotItem>`` elements it is possible to give names to subslots. Each
element has the attributes ``SubslotNumber`` and ``TextId``.

The ``<IOConfigData>`` element has the attributes:

* ``MaxInputLength="24"`` Unsigned16, valid 0..1440
* ``MaxOutputLength="24"`` Unsigned16, valid 0..1440
* ``MaxDataLength="40"`` Defaults to MaxInputLength + MaxOutputLength. Unsigned16.

The values are in bytes and are for all submodules. For details on how to
calculate these, see the GSDML specification.

Which modules that can be used in the slots are given by the
``<ModuleItemRef>`` elements. Each has the attribute ``ModuleItemTarget``,
which is a reference to a module (as described below). The attribute
``AllowedInSlots`` is a space separated list of slots that module type can be
used in. If the module type is permanently fixed in slots, then the attribute
``FixedInSlots`` is used instead.

The ``<ARVendorBlock>`` element is optional, and is used for global parameters.
These are sent from the IO-controller (PLC) during communication start.
The ``<Request>`` element has the attributes ``Length`` (in bytes) and
``APStructureIdentifier="0"``.
Data is stored in the ``<Const>`` element, with the attribute
``Data="0x00,0x00,0x00,0x01"``.

The DAP (bus interface) module can have (non-removable = virtual) submodules.
See ``<SubmoduleItem>`` below for a general description on submodules.
One specific detail for a DAP virtual submodule is that it has the
``Writeable_IM_Records="1 2 3"`` attribute, which informs about writable
Identification & Maintenance (I&M) records. Note that record 0 and 5 are
read only, so they should never appear in this list.
You must support writing to I&M1-3 for at least one of the DAP submodules.

Other special submodules for DAP modules are ``<InterfaceSubmoduleItem>`` and
``<PortSubmoduleItem>``, both subelements to ``<SystemDefinedSubmoduleList>``.
Each interface defines for example clock synchronization, and the ports (of that
interface) define for example if they use radio or 100 Mbit/s copper cables.

The subslot number for the first interface is 0x8000, and next interface (if
any) has subslot number 0x8100. Note that p-net only supports one interface.
The first port of the first interface has subslot 0x8001,
and next port of that interface has subslot number 0x8002.


Interfaces are described using the ``<InterfaceSubmoduleItem>`` element, which
has these attributes:

* ``ID="IDS_I"``
* ``SubmoduleIdentNumber="0x00000002"`` Unsigned32hex.
* ``SubslotNumber="32768"`` This is first interface (0x8000). Unsigned16.
* ``TextId="IDT_NAME_IS"``
* ``SupportedRT_Classes="RT_CLASS_1"``
* ``SupportedProtocols="SNMP;LLDP"`` Conformance class B must support SNMP.
* ``PTP_BoundarySupported="true"``
* ``DCP_BoundarySupported="true"``
* ``DCP_HelloSupported="true"`` Used for fast startup.

If the ``DCP_HelloSupported`` attribute is set to ``"true"``, you must also
set the ``PowerOnToCommReady`` attribute of the ``<DeviceAccessPointItem>``
element.

The communication startup is described in the element ``<ApplicationRelations>``
with the attribute ``StartupMode``, which typically should be "Advanced" (the
alternative is "Legacy"). If supporting both modes, use a semicolon separated
list. The ``NumberOfAR`` attribute defaults to 1 if not given.

There is typically one input-CR and one output-CR per AR, but in the GSDML file
it is possible to set the attributes ``NumberOfAdditionalInputCR``,
``NumberOfAdditionalOutputCR``, ``NumberOfAdditionalMulticastProviderCR`` and
``NumberOfMulticastConsumerCR``. Those attributes are not supported by p-net.

The ``<TimingProperties>`` element describes the sending of cyclic IO data.
The ``SendClock`` attribute contains a list of all supported send cycle times,
in units of 31.25 us. Defaults to "32", which corresponds to 1 ms. Note that the
list must contain the value ``32``.
The attribute ``ReductionRatio`` defines how much the sending can be slowed down,
and it seems that it must be “1 2 4 8 16 32 64 128 256 512”.
The actual speed of the device should be set by the ``MinDeviceInterval`` attribute in another element.

Ethernet port properties are descried using the ``<PortSubmoduleItem>``, which
has these attributes:

* ``ID="IDS_P2"``
* ``SubmoduleIdentNumber="0x00000003"`` Unsigned32hex.
* ``SubslotNumber="32770"`` This is second port on first interface (0x8002). Unsigned16.
* ``TextId="IDT_NAME_PS2"``
* ``MaxPortRxDelay="350"`` Time delay in ns needed for receiving frames. Unsigned16.
* ``MaxPortTxDelay="160"`` Time delay in ns needed for sending frames. Unsigned16.

Use an ``<MAUTypeItem>`` element to describe the Medium Attachment Unit type,
which can be radio (0), copper at 100 Mbit/s (16), copper at 1000 Mbit/s (30)
or fiber optics etc.

With the ``<GraphicItemRef>`` element it is possible to add a logo or other symbol
to your device. It will appear in the engineering tool. Use a file in the .BMP file format.
The attribute ``Type`` should typically be ``DeviceSymbol``, and the ``GraphicItemTarget``
is a reference to the file name given in the ``<GraphicsList>`` (see below).

Additional ports
----------------
Additional physical ports are created by adding ``<PortSubmoduleItem>`` nodes
to the ``<SystemDefinedSubmoduleList>`` node.
The ID, submodule identity number and subslot number shall be unique for
all ports.


Details on the module list
--------------------------
Profinet field devices can have different hardware modules, therefore there is
a need to be able to describe those modules. There are also field devices with
non-modifiable hardware, and they are sometimes called compact devices. Also
they are described using modules (fixed in slots, as mentioned above).

ModuleList element hierarchy::

    ModuleList
    |
    +--ModuleItem
       |
       +--ModuleInfo
       |  |
       |  +--Name
       |  +--TextId
       |  +--InfoText
       |  +--OrderNumber
       |  +--HardwareRelease
       |  +--SoftwareRelease
       |
       +--UseableSubmodules
       |  |
       |  +--SubmoduleItemRef
       |
       +--VirtualSubmoduleList
          |
          +--VirtualSubmoduleItem
             |
             +--ModuleInfo
             |  |
             |  +--Name
             |  +--InfoText
             |
             +--IOData
                |
                +--Input
                |   |
                |   +--DataItem
                |
                +--Output
                   |
                   +--DataItem
                      |
                      +--BitDataItem

Each ``<ModuleItem>`` element has the attributes ``ID`` (for example "IDM_1"),
``ModuleIdentNumber`` and ``PhysicalSubslots``.  The last attribute is a space
separated list of its subslot numbers.

The element ``<ModuleInfo>`` has information on the module name in its
subelements. The elements ``<HardwareRelease>`` and ``<SoftwareRelease>`` have
``Value`` attributes.

The value for <SoftwareRelease> should correspond to the configuration values
``im_sw_revision_prefix``, ``im_sw_revision_functional_enhancement``,
``im_sw_revision_bug_fix`` and ``im_sw_revision_internal_change``.

Each ``<SubmoduleItemRef>`` element has the attributes ``SubmoduleItemTarget``
(which is a reference to a submodule) and ``AllowedInSubslots`` (which is a
space separated list of subslot numbers).

Virtual submodules are submodules that are built-in into a module (no physical
submodule can be removed). If only virtual submodules are available, the
``PhysicalSubslots`` attribute is not given in ``<ModuleItem>``.
For details on ``<VirtualSubmoduleItem>``, see ``<SubmoduleItem>`` below.

The configuration value PNET_MAX_SUBSLOTS defines the maximum number of
submodules (for each module) that the p-net stack can handle.


Details on the submodule list
-----------------------------
Some submodules are permanent parts of modules, and are then called virtual
submodules.

SubmoduleList element hierarchy::

    SubmoduleList
    |
    +--SubmoduleItem
       |
       +--ModuleInfo
       |  |
       |  +--Name
       |  +--InfoText
       |  +--OrderNumber
       |
       +--IOData
       |  |
       |  +--Input
       |  |   |
       |  |   +--DataItem
       |  |
       |  +--Output
       |     |
       |     +--DataItem
       |        |
       |        +--BitDataItem
       |
       +--RecordDataList
          |
          +--ParameterRecordDataItem
             |
             +--Name
             +--Ref
             +--Const
             +--MenuList
                |
                +--MenuItem
                   |
                   +--Name
                   +--MenuRef
                   +--ParameterRef

Each ``<SubmoduleItem>`` has the attributes ``ID`` (for example "IDS_1"),
``SubmoduleIdentNumber`` and ``MayIssueProcessAlarm`` (which can be "true" or
"false"). The element ``<ModuleInfo>`` might have an attribute ``CategoryRef``,
and also has subelements with information on the submodule name etc.

The ``<Input>`` and ``<Output>`` elements have the optional attribute
``Consistency``, which can be "Item consistency" (default if not given) or
"All items consistency" (consistency between all input/output fields for
the submodule).

The ``<DataItem>`` elements have the attributes ``TextId`` and ``DataType``
(which can be for example "Unsigned8", "Unsigned64", "Float32", "Integer8",
"Date", "VisibleString", "Boolean" or "TimeStamp"). The optional
attribute ``UseAsBits="true"`` is used when individual bits are to be displayed
in the engineering tool (only for the unsigned ``DataType`` variants). It is
recommended to use Unsigned8 when packing booleans.

Use ``<BitDataItem>`` elements to name the individual bits, by setting the
attributes ``TextId`` and ``BitOffset`` (which is a string, for example "0").
The least significant bit has offset 0.

A module parameter is typically adjustable from the IO-controller, and could
be used to set for example an input delay time. To describe parameters use
``<ParameterRecordDataItem>`` elements.  They have the attributes
``Index="123"`` and ``Length="4"`` (in bytes).
Use the ``<Name>`` subelement to give it a name.
To initialize the content, use the ``<Const>`` element (if subelement
``<Ref>`` not is given).
The subelement ``<Ref>`` has these attributes:

* ``DataType="Unsigned32"``
* ``ByteOffset="0"``
* ``DefaultValue="0"``
* ``AllowedValues="0..99"``
* ``Changeable="true"`` Whether changes of this parameter is allowed.
* ``Visible="true"`` Whether it should be visible in the engineering tool.
* ``TextId="DEMO_1"``
* ``ValueItemTarget="IDV_InputDelay"`` Optional, to reference an enum (see ``<ValueItem>``).

Each module parameter can contain several ``<Ref>`` subelements, as long as
they fit in the total ``Length`` size.

It is possible to connect parameter values to enums for use in menus in
engineering tools. This is done via the ``<MenuItem>`` element (and
subelements).

============================= ============== ============ =============== ======================
Data type                     GSDML name     Size (bytes) Name in Codesys Name in Simatic Step 7
============================= ============== ============ =============== ======================
Bit (part of lager data)      (none)         (none)       BIT
Boolean (added in ver 2.32)   Boolean        1            BOOL
Integer 8 bits                Integer8       1            SINT            SINT
Integer 16 bits               Integer16      2            INT             INT
Integer 32 bits               Integer32      3            DINT            DINT
Integer 64 bits               Integer64      4            LINT            LINT
Unsigned int 8 bits           Unsigned8      1            USINT, BYTE     USINT, BYTE
Unsigned int 16 bits          Unsigned16     2            UINT, WORD      UINT, WORD
Unsigned int 32 bits          Unsigned32     4            UDINT, DWORD    UDINT, DWORD
Unsigned int 64 bits          Unsigned64     8            ULINT, LWORD    ULINT, LWORD
Float                         Float32        4            REAL            REAL
Double precision float        Float64        8            LREAL           LREAL
============================= ============== ============ =============== ======================


Details on the value list
-------------------------
The value list is optional. It is a storage of enum values.

ValueList element hierarchy::

    ValueList
    |
    +--ValueItem
       |
       +--Help
       |
       +--Assignments
          |
          +--Assign

Each enum is described in a ``<ValueItem>`` element with an ``ID`` attribute.
Each enum value is then given in an ``<Assign>`` element with attributes
``TextId`` and ``Content`` (with a numerical value given as a string,
for example ``"5"``).
It is also possible to give a help text by using the ``<Help>`` element with
a ``TextId`` attribute.


Details on the LogBook entry list
----------------------------------
This is optional, and is used to give human-readable descriptions to
manufacturer-specific error codes.

LogBookEntryList element hierarchy::

    LogBookEntryList
    |
    +--LogBookEntryItem
       |
       +--ErrorCode2Value
       |  |
       |  +--Name
       |
       +--ErrorCode2List
          |
          +--ErrorCode2Item
             |
             +--Name

A ``<LogBookEntryItem>`` has an attribute ``Status="2130432"`` that is the
decimal version of the (hex) status value 0x208200. Those are the bytes
ErrorCode, ErrorDecode and ErrorCode1. The subelements ``<ErrorCode2Value>``
and ``<Name>`` connects it to a text entry.

Some error conditions also require information from the ErrorCode2 byte. Then
the ``<ErrorCode2Item>`` element with attribute ``ErrorCode2="4"`` is used.


Details on the category list
----------------------------
The category list is optional. It can be useful for storing categories like
"Digital input" and "Digital output".

CategoryList element hierarchy::

    CategoryList
    |
    +--CategoryItem
       |
       +--InfoText

Each ``<CategoryItem>`` element has the attributes ``ID`` and ``TextId``.
It has subelements ``<InfoText>`` with the attribute ``TextId``.

The category information is used in other elements by setting the attribute
``CategoryRef`` with the value given in the ``ID`` here. For example
``<ModuleInfo>`` elements can use category information. If a more detailed
categorization is required, then also the attribute ``SubCategory1Ref`` can
be used.


Details on the external text list
---------------------------------
Human readable text strings are located here, and referenced to from the rest
of the XML file. This is for the strings to be easy to translate to other
languages.

Remember to update the contents of all relevant texts when updating a GSDML
file.

ExternalTextList element hierarchy::

    ExternalTextList
    |
    +--PrimaryLanguage
    |   |
    |   +--Text
    |
    +--Language
       |
       +--Text

Within each ``<Text>`` element, the attributes ``TextId`` and ``Value``
are used to store the information.

Only ``<PrimaryLanguage>`` is mandatory, and corresponds to English.
If ``<Language>`` is given, the actual
language is set by for example a ``xml:lang="fr"`` attribute.

This is how the different text fields are used in engineering tools:

======================= ===================================================================================================================
Item                    TIA portal and Codesys
======================= ===================================================================================================================
DeviceIdentity infotext Unclear?
DAP module name         Device name in hardware catalog, on icon in network view and in Network overview table. Typically some model name.
DAP module infotext     Device detailed description in hardware catalog. Use up to a few hundred characters.
DAP submodule name      Codesys show this in the tree on the Device/IOxS page
DAP submodule infotext  Unclear?
InterfaceSubmodule      Interface name
PortSubmodule           Port name
Module name             List of plugged modules in Device Overview
Module infotext         Catalog information in Properties -> General
Submodule name          Codesys show this in the tree on the Device/IOxS page
Submodule infotext      Unclear?
======================= ===================================================================================================================


Details on diagnosis
--------------------
The elements ``<ChannelDiagList>`` and ``<UnitDiagTypeList>`` (with
subelements) are used to specify diagnosis functionality.

Use the ``<ChannelDiagList>`` element to describe diagnosis sent in the
standard format::

    ChannelDiagList
    |
    +--ChannelDiagItem
    |  |
    |  +--Name
    |  +--ExtChannelDiagList
    |     |
    |     +--ExtChannelDiagItem
    |        |
    |        +--Name
    |
    +--SystemDefinedChannelDiagItem
    |  |
    |  +--ExtChannelDiagList
    |     |
    |     +--ExtChannelDiagItem
    |     |  |
    |     |  +--Name
    |     |  +--ExtChannelAddValue
    |     |     |
    |     |     +--DataItem
    |     |
    |     +--ProfileExtChannelDiagItem
    |        |
    |        +--Name
    |
    +--ProfileChannelDiagItem
       |
       +--Name
       +--ExtChannelDiagList
          |
          +--ProfileExtChannelDiagItem
             |
             +--Name

To add a diagnosis with ChannelErrorType in the manufacturer specific range,
use the ``<ChannelDiagItem>`` element. Set the ChannelErrorType with the
attribute ``ErrorType="999"``, for example. Describe it using the
``<Name>`` element. In the ``<ExtChannelDiagItem>`` element, use the
``ErrorType`` attribute for the ExtChannelErrorType. Describe the
ExtChannelErrorType using the ``<Name>`` element.

It is also possible to add your own ExtChannelErrorType to a standard
ChannelErrorType. Use the ``<SystemDefinedChannelDiagItem>`` element,
with the attribute ``ErrorType`` to specify the ChannelErrorType. Add
``<ExtChannelDiagItem>`` elements as we described in the previous paragraph.
The ExtChannelAddValue is specified with the ``<ExtChannelAddValue>`` element,
and ``<DataItem>`` subelements. Use ``Id`` and ``DataType`` attributes in the
subelements.

Similarly use the ``<ProfileChannelDiagItem>`` element to add
ExtChannelErrorType to diagnosis items defined in a profile.

For diagnosis sent in the USI format (also known as manufacturer specific
format), use the ``<UnitDiagTypeList>`` element::

    UnitDiagTypeList
    |
    +--UnitDiagTypeItem
    |  |
    |  +--Name
    |  +--Ref
    |
    +--ProfileUnitDiagTypeItem
    |
    +--Name
    +--Ref

Add the USI value to the ``<UnitDiagTypeItem>`` element by using the
``UserStructureIdentifier`` attribute.
Specify the data content by setting attributes to the ``<Ref>`` element,
for example ``ByteOffset``, ``DataType``, ``DefaultValue`` and ``TextId``.

It is also possible to add diagnosis in USI format for profiles, by using
the ``<ProfileUnitDiagTypeItem>`` element. Set the ``UserStructureIdentifier``
and ``API`` attributes.


Details on the process alarm list
---------------------------------
This is optional, and is used to give human-readable descriptions to
manufacturer-specific process alarms.

ChannelProcessAlarmList element hierarchy::

    ChannelProcessAlarmList
    |
    +--ChannelProcessAlarmItem
    |  |
    |  +--Name
    |  +--Help
    |  +--ExtChannelProcessAlarmList
    |     |
    |     +--ExtChannelProcessAlarmItem
    |
    +--SystemDefinedChannelProcessAlarmItem
    |  |
    |  +--ExtChannelProcessAlarmList
    |     |
    |     +--ExtChannelProcessAlarmItem
    |     +--ProfileExtChannelProcessAlarmItem
    |
    +--ProfileChannelProcessAlarmItem
       |
       +--Name
       +--Help
       +--ExtChannelProcessAlarmList
          |
          +--ExtChannelProcessAlarmItem
          +--ProfileExtChannelProcessAlarmItem

The element ``<ChannelProcessAlarmItem>`` is used to describe custom process
alarms.

An extension to system defined process alarms is created by the element
``<SystemDefinedChannelProcessAlarmItem>``. Profiles can define process alarms
using the ``<ProfileChannelProcessAlarmItem>`` element.


Details on the graphics list
---------------------------------
File names for graphic items are mapped to IDs here.

GraphicsList element hierarchy::

    GraphicsList
    |
    +--GraphicItem


Within each ``<GraphicItem>`` element, the attributes ``ID`` and ``GraphicFile``
are used to store the information.

The ``GraphicFile`` is the filename without the BMP file extension.
