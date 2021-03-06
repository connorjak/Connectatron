// Definitions for all protocols.
#pragma once
#include "Connectatron_Utils.h"

#include <set>
#include <string>

using std::set;

// *** RESEARCH ***

// https://en.wikipedia.org/wiki/List_of_computer_standards
// https://en.wikipedia.org/wiki/Computer_port_(hardware)#Types_of_ports
// https://en.wikipedia.org/wiki/Category:Computer_buses

// Storage Interface
// https://en.wikipedia.org/wiki/Serial_ATA#eSATA

// Power
// https://en.wikipedia.org/wiki/Molex_connector

// USB
// https://en.wikipedia.org/wiki/USB#Connectors
// https://www.sony.com/electronics/support/articles/00024571
// https://www.computerlanguage.com/results.php?definition=USB+3.2+Gen+1x2
// https://en.wikipedia.org/wiki/USB
// https://en.wikipedia.org/wiki/USB_3.0
// https://en.wikipedia.org/wiki/USB_3.0#3.2
// https://en.wikipedia.org/wiki/USB4
// https://www.tripplite.com/products/usb-connectivity-types-standards

// Display
// https://en.wikipedia.org/wiki/DisplayPort
// https://en.wikipedia.org/wiki/VGA_connector
// https://en.wikipedia.org/wiki/HDMI
// https://en.wikipedia.org/wiki/Display_Data_Channel
// https://en.wikipedia.org/wiki/Extended_Display_Identification_Data

// Audio
// https://en.wikipedia.org/wiki/Phone_connector_(audio)

// High Bandwidth
// https://en.wikipedia.org/wiki/PCI_Express



// Variable name to string parsing:
// _    : .
// __   : (space) 
// ___  : -
// ____ : /
enum class WireProtocol : unsigned int
{
    UNRECOGNIZED,
    Proprietary,

    // Power
    DC__Power,
    AC__Power,
    //https://en.wikipedia.org/wiki/NEMA_connector

    // Analog Single-Wire
    GenericAnalog, //Not sure if we actually need to specify protocols here
    AnalogAudio, //Placeholder until I get a proper name for this

    // Single-Wire
    START_CATEGORY_Single___Wire,
    //https://en.wikipedia.org/wiki/System_Management_Bus
    UART,
    SPI,
    //https://en.wikipedia.org/wiki/I%C2%B2C
    I2C,
    END_CATEGORY_Single___Wire,

    // Audio
    START_CATEGORY_Audio,
    S____PIDF, //https://en.wikipedia.org/wiki/S/PDIF
    ADAT__Lightpipe, //https://en.wikipedia.org/wiki/ADAT_Lightpipe
    AES3, //https://en.wikipedia.org/wiki/AES3
    END_CATEGORY_Audio,

    // USB
    START_CATEGORY_USB,
    MIN_VERSION_USB,
    USB__1_0, //Low-Speed: 1.5 Mbps
    USB__1_1, //Full-Speed: 12 Mbps
    USB__2_0, //Hi-Speed: 480 Mbps
    USB__3__5g, //SuperSpeed 5 Gbps
    USB__3__10g, //SuperSpeed+ 10 Gbps
    USB__3__20g, //SuperSpeed+ 20 Gbps (Dual-lane, requiring USB-C connector)
    USB4__20g, //USB4 20 Gbps
    USB4__40g, //USB4 40 Gbps
    MAX_VERSION_USB,
    END_CATEGORY_USB,

    // FireWire
    START_CATEGORY_FireWire,
    //https://en.wikipedia.org/wiki/IEEE_1394
    MIN_VERSION_FireWire,
    FireWire__400__1394, //IEEE 1394-1995
    FireWire__400__1394a, //IEEE 1394a-2000
    FireWire__800__1394b, //IEEE 1394b-2002
    FireWire__S800T__1394c, //IEEE 1394c-2006
    FireWire__S1600, //IEEE 1394-2008
    FireWire__S3200, //IEEE 1394-2008
    MAX_VERSION_FireWire,
    END_CATEGORY_FireWire,

    // Display
    START_CATEGORY_Display,
    // https://en.wikipedia.org/wiki/DisplayPort
    MIN_VERSION_DisplayPort,
    DisplayPort__1_0,
    DisplayPort__1_1,
    DisplayPort__1_2,
    DisplayPort__1_2a, //Adaptive Sync support
    DisplayPort__1_3,
    DisplayPort__1_4,
    DisplayPort__1_4a,
    DisplayPort__2_0,
    MAX_VERSION_DisplayPort,
    // https://en.wikipedia.org/wiki/HDMI
    MIN_VERSION_HDMI,
    HDMI__1_0, //Dec 2002
    HDMI__1_1, //May 2004
    HDMI__1_2, //Aug 2005
    HDMI__1_2a,//Dec 2005
    HDMI__1_3, //Jun 2006
    HDMI__1_3a,//Nov 2006
    HDMI__1_4, //Jun 2009
    HDMI__1_4a,//Mar 2010
    HDMI__1_4b,//Oct 2011
    HDMI__2_0, //Sep 2013
    HDMI__2_0a,//Apr 2015
    HDMI__2_0b,//Mar 2016
    HDMI__2_1, //Nov 2017
    MAX_VERSION_HDMI,
    //https://en.wikipedia.org/wiki/Digital_Visual_Interface
    DVI___D, //Digital
    DVI___A, //Analog
    DVI___I, //Combined
    VGA, //TODO specific specs?
    HDBaseT,            //https://en.wikipedia.org/wiki/HDBaseT
    Composite__Video,   //https://en.wikipedia.org/wiki/Composite_video
    RGB__Video,         //https://en.wikipedia.org/wiki/RGB_color_model#Video_electronics
    S___Video,          //https://en.wikipedia.org/wiki/S-Video
    Component__Video,   //https://en.wikipedia.org/wiki/Component_video
    END_CATEGORY_Display,

    // Display Metadata (may be a part of another display cable protocol)
    START_CATEGORY_Display__Metadata,
    //https://en.wikipedia.org/wiki/Display_Data_Channel
    DDC1,
    DDC2B,
    E___DDC,
    //https://en.wikipedia.org/wiki/Extended_Display_Identification_Data
    EDID,
    //https://en.wikipedia.org/wiki/High-dynamic-range_video
    HDR10,              //https://en.wikipedia.org/wiki/HDR10
    HDR10__Plus,        //https://en.wikipedia.org/wiki/HDR10%2B
    Dolby__Vision,      //https://en.wikipedia.org/wiki/Dolby_Vision
    END_CATEGORY_Display__Metadata,

    // Expansion Slot
    START_CATEGORY_Expansion__Slot,
    //https://en.wikipedia.org/wiki/Industry_Standard_Architecture
    ISA,
    ISA__DMA, // Direct Memory Access
    //https://en.wikipedia.org/wiki/Parallel_ATA
    // AKA ATA, IDE
    //TODO
    //https://en.wikipedia.org/wiki/Peripheral_Component_Interconnect
    PCI__1_0,
    PCI__2_0,
    PCI__2_1,
    PCI__2_2,
    PCI__2_3,
    PCI__3_0,
    //https://en.wikipedia.org/wiki/Accelerated_Graphics_Port
    AGP__1_0__1x,
    AGP__1_0__2x,
    AGP__2_0__4x,
    AGP__3_0__8x,
    AGP__3_5__8x,
    // https://en.wikipedia.org/wiki/PCI_Express
    PCIe__1_0__x1,
    PCIe__1_0__x2,
    PCIe__1_0__x4,
    PCIe__1_0__x8,
    PCIe__1_0__x16,
    PCIe__2_0__x1,
    PCIe__2_0__x2,
    PCIe__2_0__x4,
    PCIe__2_0__x8,
    PCIe__2_0__x16,
    PCIe__3_0__x1,
    PCIe__3_0__x2,
    PCIe__3_0__x4,
    PCIe__3_0__x8,
    PCIe__3_0__x16,
    PCIe__4_0__x1,
    PCIe__4_0__x2,
    PCIe__4_0__x4,
    PCIe__4_0__x8,
    PCIe__4_0__x16,
    PCIe__5_0__x1,
    PCIe__5_0__x2,
    PCIe__5_0__x4,
    PCIe__5_0__x8,
    PCIe__5_0__x16,
    // https://en.wikipedia.org/wiki/PC_Card
    //TODO
    // https://en.wikipedia.org/wiki/ExpressCard
    //TODO
    END_CATEGORY_Expansion__Slot,

    // Storage Interface
    START_CATEGORY_Storage__Interface,
    //https://en.wikipedia.org/wiki/Serial_Attached_SCSI
    SAS___1,
    SAS___2,
    SAS___3,
    SAS___4,
    SAS___5,
    //https://en.wikipedia.org/wiki/Parallel_ATA
    //https://en.wikipedia.org/wiki/Advanced_Host_Controller_Interface
    //https://en.wikipedia.org/wiki/Serial_ATA
    SATA__1_0,
    SATA__2_0,
    SATA__2_5,
    SATA__2_6,
    SATA__3_0,
    SATA__3_1,
    SATA__3_2,
    SATA__3_3,
    SATA__3_4,
    SATA__3_5,
    //https://en.wikipedia.org/wiki/NVM_Express
    NVMe__1_0e,
    NVMe__1_1b,
    NVMe__1_2,
    NVMe__1_3,
    NVMe__1_4,
    NVMe__2_0,
    //https://en.wikipedia.org/wiki/SD_card
    SD__DefaultSpeed,
    SD__HighSpeed,
    SD__UHS___I,
    SD__UHS___II,
    SD__UHS___III,
    SD__Express,
    END_CATEGORY_Storage__Interface,

    // Fibre Channel
    //https://en.wikipedia.org/wiki/Fibre_Channel
    START_CATEGORY_Fibre__Channel,
    Fibre__133__Mbps,
    Fibre__266__Mbps,
    Fibre__533__Mbps,
    Fibre__1GFC,
    Fibre__2GFC,
    Fibre__4GFC,
    Fibre__8GFC,
    Fibre__10GFC,
    Fibre__16GFC,
    Fibre__32GFC__Gen__6,
    Fibre__64GFC__Gen__7,
    Fibre__128GFC__Gen__6,
    Fibre__256GFC__Gen__7,
    Fibre__128GFC__Gen__8,
    END_CATEGORY_Fibre__Channel,

    //TODO https://en.wikipedia.org/wiki/Optical_transport_network

    // Ethernet
    START_CATEGORY_Ethernet,
    //https://en.wikipedia.org/wiki/Ethernet_over_twisted_pair
    //https://en.wikipedia.org/wiki/Fast_Ethernet#100BASE-T1
    //https://en.wikipedia.org/wiki/10BASE5
    //https://en.wikipedia.org/wiki/Category:Ethernet_standards
    MIN_VERSION_Ethernet,
    Ethernet__10BASE5, //10 Mbps, coax instead of twisted pair
    Ethernet__10BASE2, //10 Mbps, thinner coax
    // BASE-T is Twisted-Pair
    Ethernet__10BASE___T, //10 Mbps, twisted pair
    //Ethernet__10BASE__T1S, //10 Mbps (short distance ~15 meters)
    //Ethernet__10BASE__T1L, //10 Mbps (long distance ~1000 meters)
    //NOTE: not getting into cable details at the moment
    Ethernet__100BASE___T, //100 Mbps "Fast Ethernet"
    Ethernet__1000BASE___T, //1 Gbps "Gigabit Ethernet"
    Ethernet__2_5GBASE___T, //2.5 Gbps
    Ethernet__5GBASE___T, //5 Gbps
    Ethernet__10GBASE___T, //10 Gbps
    Ethernet__25GBASE___T, //25 Gbps
    Ethernet__40GBASE___T, //40 Gbps
    // BASE-CX4 https://cs-electronics.com/cx4/
    Ethernet__10GBASE___CX4,    
    MAX_VERSION_Ethernet,
    END_CATEGORY_Ethernet,

    // TODO https://en.wikipedia.org/wiki/SCSI

    // Wireless
    START_CATEGORY_Wireless,
    //https://en.wikipedia.org/wiki/Wi-Fi
    MIN_VERSION_Wi___Fi,
    Wi___Fi__0__802_11,     //1997
    Wi___Fi__1__802_11b,    //1999
    Wi___Fi__2__802_11a,    //1999
    Wi___Fi__3__802_11g,    //2003
    Wi___Fi__4__802_11n,    //2008
    Wi___Fi__5__802_11ac,   //2014
    Wi___Fi__6__802_11ax,   //2019
    Wi___Fi__6E__802_11ax,  //2020
    Wi___Fi__7__802_11be,   //TBA
    MAX_VERSION_Wi___Fi,
    //https://en.wikipedia.org/wiki/Bluetooth
    MIN_VERSION_Bluetooth,
    Bluetooth__1_0,
    Bluetooth__1_0B,
    Bluetooth__1_1,
    Bluetooth__1_2,
    Bluetooth__2_0,
    Bluetooth__2_1,
    Bluetooth__EDR,
    Bluetooth__3_0,
    Bluetooth__HS,
    Bluetooth__4_0,
    Bluetooth__4__Low__Energy,
    Bluetooth__4_1,
    Bluetooth__4_2,
    Bluetooth__5_0,
    Bluetooth__5_1,
    Bluetooth__5_2,
    Bluetooth__5_3,
    MAX_VERSION_Bluetooth,
    //https://www.windowscentral.com/xbox-wireless
    Xbox__Wireless,
    END_CATEGORY_Wireless,

    // Other Protocols
    //https://en.wikipedia.org/wiki/Thunderbolt_(interface)
    Thunderbolt__1,
    Thunderbolt__2,
    Thunderbolt__3,
    Thunderbolt__4,
    RS___232,
    RS___422,
    RS___485,
    //https://en.wikipedia.org/wiki/IEEE_1284
    IEEE__1284, //AKA Centronics Port
    //https://en.wikipedia.org/wiki/MIL-STD-1553
    MIL___STD___1553,
    //https://en.wikipedia.org/wiki/ARINC_429
    ARINC__429,
    //https://en.wikipedia.org/wiki/CAN_bus
    CAN__bus,
};

#define LONGEST_PROTOCOL_STR "FireWire S800T 1394c"

const set<WireProtocol> BackCompat_USB2_0 = {
    WireProtocol::USB__1_0, //Low-Speed: 1.5 Mbps
    WireProtocol::USB__1_1, //Full-Speed: 12 Mbps
    WireProtocol::USB__2_0, //Hi-Speed: 480 Mbps
};

const set<WireProtocol> BackCompat_USB3__5g = {
    WireProtocol::USB__1_0, //Low-Speed: 1.5 Mbps
    WireProtocol::USB__1_1, //Full-Speed: 12 Mbps
    WireProtocol::USB__2_0, //Hi-Speed: 480 Mbps
    WireProtocol::USB__3__5g, //SuperSpeed: 5 Gbps
};

const set<WireProtocol> BackCompat_DisplayPort1_2a = {
    WireProtocol::DisplayPort__1_0,
    WireProtocol::DisplayPort__1_1,
    WireProtocol::DisplayPort__1_2,
    WireProtocol::DisplayPort__1_2a,
};

const set<WireProtocol> BackCompat_DisplayPort1_4a = {
    WireProtocol::DisplayPort__1_0,
    WireProtocol::DisplayPort__1_1,
    WireProtocol::DisplayPort__1_2,
    WireProtocol::DisplayPort__1_2a,
    WireProtocol::DisplayPort__1_3,
    WireProtocol::DisplayPort__1_4,
    WireProtocol::DisplayPort__1_4a,
};


static string NameFromProtocol(WireProtocol proto)
{
    string ret = std::string(magic_enum::enum_name(proto));
    EnumName_Underscore2Symbol(ret);
    return ret;
}

///////////////////////////////
/// CATEGORIES

struct ProtocolCategoryInfo : CategoryInfo
{
    vector<WireProtocol> protocols;
};

static vector<WireProtocol> GetUncategorizedProtocols()
{
    vector<WireProtocol> ret;

    bool in_category = false;
    for (const auto& possible_proto : magic_enum::enum_values<WireProtocol>())
    {
        if (possible_proto == WireProtocol::UNRECOGNIZED)
            continue;

        auto proto_string = NameFromProtocol(possible_proto);
        EnumName_Underscore2Symbol(proto_string);
        auto cat_strpos = proto_string.find(".CATEGORY.");
        if (cat_strpos != string::npos)
        {
            if (!in_category)
            {
                in_category = true;
            }
            else
            {
                in_category = false;
            }
        }
        else
        {
            if (!in_category)
                ret.push_back(possible_proto);
        }
    }

    return ret;
}

static vector<ProtocolCategoryInfo> GetProtocolCategories()
{
    vector<ProtocolCategoryInfo> ret;

    bool in_category = false;
    ProtocolCategoryInfo current_category;
    for (const auto& possible_proto : magic_enum::enum_values<WireProtocol>())
    {
        if (possible_proto == WireProtocol::UNRECOGNIZED)
            continue;

        auto proto_string = NameFromProtocol(possible_proto);
        EnumName_Underscore2Symbol(proto_string);

        if (proto_string.find(".VERSION.") != string::npos)
            continue;

        auto cat_strpos = proto_string.find(".CATEGORY.");
        if (cat_strpos != string::npos)
        {
            if (!in_category)
            {
                auto name_strpos = cat_strpos + string(".CATEGORY.").length();
                current_category.name = proto_string.substr(name_strpos);
                current_category.index_of_first = magic_enum::enum_index<WireProtocol>(possible_proto).value() + 1;
                in_category = true;
            }
            else
            {
                current_category.index_after_last = magic_enum::enum_index<WireProtocol>(possible_proto).value();
                in_category = false;
                ret.push_back(current_category);
                current_category.protocols.clear();
            }
        }
        else
        {
            if (in_category)
                current_category.protocols.push_back(possible_proto);
        }
    }

    return ret;
}
