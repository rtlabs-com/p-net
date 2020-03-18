Creating GSD (GSDML) files
==========================
A GSD (General Station Description) file is an XML file describing a Profinet
IO-Device. The XML-based language is called GSDML (GSD Markup Language).

For Profinet members, the "Profinet GSD Checker" tool is available for
download. It contains GSD example files showing different aspects of the file
format.

To create a custom GSD file, start from one of the examples. Edit the file in
an XML editor. Then run the resulting file in the "Profinet GSD Checker" tool
to verify its structure.

Below is a brief introduction to the most common elements and attributes in
GSDML files given. It contains simplifications and does not describe special
cases. For full details, see the official GSDML specification.


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
installation, enter "profinet" in the Windows search bar to start the program.

Use the menu File > Open and select your GSD file. An overview of the file
is shown. In the left menu different parts of the file can be selected.

To verify the file, use the menu File > Check. Click the "GSDCheck" bar at
the bottom of the screen to open a document check view in the right part of
the screen. Use the Icons to show the details of any error messages.

If you later need to uninstall the tool, do that via Windows settings > Apps &
features.


File name
---------
The current GSD version (as of writing) is 2.35.

The file name is specified in section 5.1 of the GSD specification. For example
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
          +--UnitDiagTypeList
          +--ExternalTextList

Do not change the ``<ProfileHeader>`` contents.

In the ``<DeviceIdentity>`` adjust the attributes ``VendorID``, ``DeviceID``.
Adjust the ``Value`` in ``<VendorName>``.
In the ``<Family>`` adjust the attributes for ``MainFamily`` (typically "I/O")
and ``ProductFamily``.


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
              |
              +--InterfaceSubmoduleItem
              |  |
              |  +--ApplicationRelations
              |     |
              |     +--TimingProperties
              |
              +--PortSubmoduleItem
                 |
                 +--MAUTypeList
                    |
                    +--MAUTypeItem

The ``<DeviceAccessPointItem>`` element has the attributes:

* ``ID="IDD_1"``
* ``PNIO_Version="V2.35"`` Which version of Profinet specification it is certified against
* ``PhysicalSlots="0..4"`` Slot 0 is always used by the DAP (bus interface) module.
* ``ModuleIdentNumber="0x00000001"`` Unsigned32hex.
* ``MinDeviceInterval="32"`` Minimum cyclic data update interval, in number of 31.25 us ticks. A value 32 corresponds to cyclic data sending and receiving every millisecond. Unsigned16.
* ``DNS_CompatibleName="pno-example-dap"`` (Default station name)
* ``FixedInSlots="0"`` The DAP module is always in slot 0
* ``ObjectUUID_LocalIndex="0"``
* ``DeviceAccessSupported="true"``
* ``NumberOfDeviceAccessAR="1"`` Number of concurrent connections
* ``MultipleWriteSupported="true"``
* ``RequiredSchemaVersion="V2.1"`` This file has features requiring this schema version
* ``CheckDeviceID_Allowed="true"``
* ``NameOfStationNotTransferable="false"``
* ``LLDP_NoD_Supported="true"`` (Should be "true" for recent Profinet versions)
* ``ResetToFactoryModes="2"`` (Bits describing reset possibilities. At least "2" should be present)
* ``ParameterizationSpeedupSupported="true"`` For fast startup.
* ``PowerOnToCommReady="700"`` For fast startup, time to first data exchange in milliseconds. Unsigned32.

General info on the Profinet IO-Device is given in ``<ModuleInfo>``
subelements. For example the vendor name and order number are given.

The ``<CertificationInfo>`` element has the attributes:

* ``ConformanceClass="A"``
* ``ApplicationClass=""`` Typically empty, but can be for example "FunctionalSafety"
* ``NetloadClass="I"``

With ``<SubslotItem>`` elements it is possible to give names to subslots. Each
element has the attributes ``SubslotNumber`` and ``TextId``.

The ``<IOConfigData>`` element has the attributes:

* ``MaxInputLength="24"`` Unsigned16.
* ``MaxOutputLength="24"`` Unsigned16.
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
Identification & Maintenance records. Note that record 0 and 5 are read only,
so they should never appear in this list.

Other special submodules for DAP modules are ``<InterfaceSubmoduleItem>`` and
``<PortSubmoduleItem>``, both subelements to ``<SystemDefinedSubmoduleList>``.
Each interface defines for example clock synchronization, and the ports (of that
interface) define for example if they use radio or 100 Mbit/s copper cables.

The subslot number for the first interface is 0x8000, and next interface (if
any) has subslot number 0x8100. The first port of the first interface has
subslot 0x8001, and next port of that interface has subslot number 0x8002.

Interfaces are described using the ``<InterfaceSubmoduleItem>`` element, which
has these attributes:

* ``ID="IDS_I"``
* ``SubmoduleIdentNumber="0x00000002"`` Unsigned32hex.
* ``SubslotNumber="32768"`` This is first interface (0x8000). Unsigned16.
* ``TextId="IDT_NAME_IS"``
* ``SupportedRT_Classes="RT_CLASS_1"``
* ``SupportedProtocols="SNMP;LLDP"``
* ``PTP_BoundarySupported="true"``
* ``DCP_BoundarySupported="true"``
* ``DCP_HelloSupported="true"`` Often used for fast startup.

The communication startup is described in the element ``<ApplicationRelations>``
with the attribute ``StartupMode``, which typically should be "Advanced" (the
alternative is "Legacy").

Use the ``<TimingProperties>`` element to define the sending of cyclic IO data.
The ``SendClock`` attributes contains a list of all supported send cycle times,
in ticks of 31.25 us. Defaults to "32", which corresponds to 1 ms. The
attribute ``ReductionRatio`` defines how much the sending can be slowed down,
and defaults to "1 2 4 8 16 32 64 128 256 512".

Ethernet port properties are descried using the ``<PortSubmoduleItem>``, which
has these attributes:

* ``ID="IDS_P2"``
* ``SubmoduleIdentNumber="0x00000003"`` Unsigned32hex.
* ``SubslotNumber="32770"`` This is second port on first interface (0x8002). Unsigned16.
* ``TextId="IDT_NAME_PS2"``
* ``MaxPortRxDelay="350"`` Time delay in ns needed for receiving frames. Unsigned16.
* ``MaxPortTxDelay="160"`` Time delay in ns needed for sending frames. Unsigned16.

Use an ``<MAUTypeItem>`` element to describe the Medium Attachment Unit type,
which can be radio (0), copper at 100 Mbit/s (16) or fiber optics.


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

Each ``<SubmoduleItemRef>`` element has the attributes ``SubmoduleItemTarget``
(which is a reference to a submodule) and ``AllowedInSubslots`` (which is a
space separated list of subslot numbers).

Virtual submodules are submodules that are built-in into a module (no physical
submodule can be removed). If only virtual submodules are available, the
``PhysicalSubslots`` attribute is not given in ``<ModuleItem>``.
For details on ``<VirtualSubmoduleItem>``, see ``<SubmoduleItem>`` below.


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
"All items consistency".

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
To initialize the content, use the ``<Const>`` element.
The subelement ``<Ref>`` has these attributes:

* ``DataType="Unsigned32"``
* ``ByteOffset="0"``
* ``DefaultValue="0"``
* ``AllowedValues="0..99"``
* ``Changeable="true"`` Whether changes of this parameter is allowed.
* ``Visible="true"`` Whether it should be visible in the engineering tool.
* ``TextId="DEMO_1"``
* ``ValueItemTarget="IDV_InputDelay"`` Optional, to reference an enum (see ``<ValueItem>``).

It is possible to connect parameter values to enums for use in menus in
engineering tools. This is done via the ``<MenuItem>`` element (and
subelements).


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
for example "5").
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

Only ``<PrimaryLanguage>`` is mandatory. If ``<Language>`` is given, the actual
language is set by for example a ``xml:lang="fr"`` attribute.


Diagnosis
---------
The elements ``<ChannelDiagList>`` and ``<UnitDiagTypeList>`` (with
subelements) are used to specify diagnosis functionality.
