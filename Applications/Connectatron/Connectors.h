// Definitions for all connectors, which each are (usually) incompatible with each other.
#pragma once
#include "Connectatron_Utils.h"

#include <set>
#include <map>
#include <string>
#include <vector>
#include <memory>

#include <nlohmann/json.hpp>

using std::set;
using std::map;
using std::vector;
using std::shared_ptr;

using namespace nlohmann;

using Connector_ID = std::string;
using Connector_AKA = std::string;
using Category_Name = std::string;

struct ConnectorInfo
{
    Connector_ID primary_ID;
    vector<Connector_AKA> AKA_IDs;
    vector<Category_Name> categories;
    vector<string> links;
    // Connectors besides itself that a male of this connector fits into
    vector<Connector_ID> maleFitsInto;
    json full_info = json::object();
};

static map<Connector_ID, shared_ptr<ConnectorInfo>> connectors;
static map<Category_Name, vector<shared_ptr<ConnectorInfo>>> connector_categories;

/*
// Variable name to string parsing:
// _    : .
// __   : (space) 
// ___  : -
// ____ : /
enum class OLD_PinType : unsigned int
{
    UNRECOGNIZED,
    Proprietary, // For connectors that don't match any listed options
    Wireless,

    // Single-Wire
    Pin__Header,        //https://en.wikipedia.org/wiki/Pin_header
    Screw__Terminal,     //https://en.wikipedia.org/wiki/Screw_terminal

    // Two-wire bus
    Twisted__Pair,
    Coaxial,

    // DC Power
    START_CATEGORY_DC__Power,
    // https://en.wikipedia.org/wiki/DC_connector
    // https://en.wikipedia.org/wiki/Coaxial_power_connector
    DC__Power__Barrel, //This is the generic one; we also have more specific specs
    // https://en.wikipedia.org/wiki/EIAJ_connector
    EIAJ___01,
    EIAJ___02,
    EIAJ___03,
    EIAJ___04,  // AKA JSBP 4             
    EIAJ___05,  // AKA JSBP 5
    IEC__60130___10__Type__A__2_1mm__ID,
    IEC__60130___10__Type__A__2_5mm__ID,
    IEC__60130___10__Type__B__2_1mm__ID,
    IEC__60130___10__Type__B__2_5mm__ID,
    IEC__60130___10__Type__C,
    IEC__60130___10__Type__D,
    IEC__60130___10__Type__E,
    DIN__45323__5mm__OD,
    DIN__45323__6mm__OD,
    // PC Internal Connectors
    // https://en.wikipedia.org/wiki/Power_supply_unit_(computer)#Connectors
    ATX__20___pin,
    ATX12VO,
    // https://en.wikipedia.org/wiki/PCI_Express#Power
    PCIe__6___pin,
    PCIe__6___2___pin,
    PCIe__8___pin,
    PCIe__12___pin,
    PCIe__16___pin__ATX__3_0,
    // https://en.wikipedia.org/wiki/Power_supply_unit_(computer)#Entry-Level_Power_Supply_Specification
    EPS__4___pin,
    EPS__8___pin,
    EPS__24___pin,
    Molex,
    SATA__Power,
    SATA__Power__Slimline,
    END_CATEGORY_DC__Power,
    
    // AC Power
    START_CATEGORY_AC__Power,
    // https://en.wikipedia.org/wiki/NEMA_connector
    // 120VAC
    NEMA__5___15, //Typical outlet, 3 prong
    NEMA__5___20, 
    NEMA__5___30,  
    NEMA__5___50,
    NEMA__TT___30,
    NEMA__L5___15,
    NEMA__L5___20,
    NEMA__L5___30,
    NEMA__1___15, //Old outlet, 2 prong
    // 120/240VAC
    NEMA__14___20,
    NEMA__14___30, //Clothes Dryer
    NEMA__14___50, //Electric Oven
    NEMA__14___60,
    NEMA__L14___20,
    NEMA__L14___30,
    // 240VAC
    NEMA__6___15,
    NEMA__6___20,
    NEMA__6___30,
    NEMA__6___50,
    NEMA__10___30,
    NEMA__10___50,
    // Welder or Plasma Cutter
    NEMA__L6___15,
    NEMA__L6___20,
    NEMA__L6___30,
    NEMA__L6___50,
    // https://en.wikipedia.org/wiki/IEC_60320
    C1____C2,
    //C3____C4, // has been withdrawn from the standard
    C5____C6,
    C7____C8,
    C9____C10,
    //C11____C12, // has been withdrawn from the standard
    C13____C14,
    C15____C16,
    C15A____C16A,
    C17____C18,
    C19____C20,
    C21____C22,
    C23____C24,
    //TODO other AC connectors
    END_CATEGORY_AC__Power,

    // RF
    // https://en.wikipedia.org/wiki/RF_connector
    //TODO

    // USB          // https://en.wikipedia.org/wiki/USB
    START_CATEGORY_USB,
    USB___A,
    USB___A__SuperSpeed, // Has more pins for USB3 features. Fits with USB-A.
    USB___B,
    USB___B__SuperSpeed, // Has more pins for USB3 features. Taller than USB-B.
    USB___C,
    USB__Mini___A,
    USB__Mini___B,
    USB__Mini___AB,
    USB__Micro___A,
    USB__Micro___B,
    USB__Micro___AB,
    USB__Micro___B__SuperSpeed, // Has more pins for USB3 features. Wider than USB Micro-B.
    END_CATEGORY_USB,

    // Display
    START_CATEGORY_Display,
    // https://en.wikipedia.org/wiki/DisplayPort
    DisplayPort,
    Mini__DisplayPort, //Notably also used for Thunderbolt 1 and 2
    // https://en.wikipedia.org/wiki/HDMI
    HDMI,
    Mini__HDMI,
    Micro__HDMI,
    // https://en.wikipedia.org/wiki/Digital_Visual_Interface
    DVI___D,
    DVI___A,
    DVI___I,
    Mini___DVI,     // https://en.wikipedia.org/wiki/Mini-DVI
    Micro___DVI,    // https://en.wikipedia.org/wiki/Micro-DVI
    Apple__Display__Connector, // https://en.wikipedia.org/wiki/Apple_Display_Connector
    VGA,
    Mini___VGA,
    SCART,          // https://en.wikipedia.org/wiki/SCART
    //https://en.wikipedia.org/wiki/RCA_connector
    RCA__Component__Video,
    RCA__Stereo__Audio,
    RCA,
    END_CATEGORY_Display,

    // Expansion Slot
    START_CATEGORY_Expansion__Slot,
    ISA,            //https://en.wikipedia.org/wiki/Industry_Standard_Architecture
    //https://en.wikipedia.org/wiki/Parallel_ATA
    // AKA ATA, IDE
    PATA__40___pin,
    PATA__44___pin,
    PATA__80___pin,
    PCI,            //https://en.wikipedia.org/wiki/Peripheral_Component_Interconnect
    //https://en.wikipedia.org/wiki/Accelerated_Graphics_Port
    AGP__3_3V,
    AGP__1_5V,
    AGP__Universal,
    AGP__Pro__3_3V,
    AGP__Pro__1_5V,
    AGP__Pro__Universal,
    // https://en.wikipedia.org/wiki/PCI_Express
    PCIe__x1,
    PCIe__x2,
    PCIe__x4,
    PCIe__x8,
    PCIe__x16,
    Mini__PCIe,
    // https://en.wikipedia.org/wiki/PCI_Express#PCI_Express_OCuLink
    // https://blog.fosketts.net/2017/06/22/what-is-oculink/
    OCuLink, //AKA SFF-8612
    //https://en.wikipedia.org/wiki/M.2
    M_2__M__key,    // AKA Socket 3, usual for PCIe x4 / NVMe devices.
    M_2__B__M__key, // Usual for SATA devices (fits in Socket 2, 3).
    M_2__B__key,    // AKA Socket 2, usual for WWAN, GNSS, SSD, etc.
    M_2__A__E__key, // AKA Socket 1, usual for wireless.
    // https://en.wikipedia.org/wiki/Mobile_PCI_Express_Module
    MXM___I,
    MXM___II,
    MXM___III,
    MXM___A,
    MXM___B,
    // https://en.wikipedia.org/wiki/PC_Card
    // AKA PCMCIA
    PC__Card__Type__I, 
    PC__Card__Type__II,
    PC__Card__Type__III,
    PC__Card__Type__IV,
    // https://en.wikipedia.org/wiki/ExpressCard
    ExpressCard____34,
    ExpressCard____54,
    END_CATEGORY_Expansion__Slot,

    // Audio
    START_CATEGORY_Audio,
    Audio3_5mm, //Should probably specify stereo/not?
    //https://en.wikipedia.org/wiki/XLR_connector
    XLR3, //3-pin, most common
    XLR4, //4-pin
    XLR5, //5-pin
    XLR6, //6-pin
    XLR7, //7-pin
    PDN,
    Mini__XLR, // AKA TQG, TA3/TA4
    // https://en.wikipedia.org/wiki/TOSLINK
    TOSLINK,
    Mini___TOSLINK,
    END_CATEGORY_Audio,

    // DIN          // https://en.wikipedia.org/wiki/DIN_connector

    // Mini-DIN     // https://en.wikipedia.org/wiki/Mini-DIN_connector
    PS____2,        // https://en.wikipedia.org/wiki/PS/2_port
    //TODO others from wiki article

    // Storage Interface
    START_CATEGORY_Storage__Interface,
    //https://en.wikipedia.org/wiki/Serial_ATA
    SATA,
    Micro___SATA,
    Mini___SATA, 
    eSATA,
    eSATAp,         //https://en.wikipedia.org/wiki/ESATAp
    CompactFlash,   // https://en.wikipedia.org/wiki/CompactFlash
    // https://en.wikipedia.org/wiki/SD_card
    SD,
    miniSD,
    microSD,
    // SAS https://en.wikipedia.org/wiki/Serial_Attached_SCSI
    Mini___SAS__SFF___8086,
    Mini___SAS__SFF___8087,      //https://cs-electronics.com/sff-8087/
    Mini___SAS__SFF___8088,      //https://cs-electronics.com/sff-8088/
    SFP__Plus__SFF___8431,      //https://cs-electronics.com/sff-8431/
    Quad__SFP__Plus__SFF___8436,      //https://cs-electronics.com/sff-8436/
    ImfiniBand__SFF___8470,      //https://cs-electronics.com/sff-8470/
    SAS__SFF___8482,      //https://cs-electronics.com/sff-8482/
    SAS__SFF___8484,      //https://cs-electronics.com/sff-8484/
    SAS__SFF___8485,      //https://members.snia.org/document/dl/25923 https://web.archive.org/web/20190626094026/https://members.snia.org/document/dl/25923
    U_2__Mini___SAS__HD__SFF___8643, // Also SFF-8613    https://cs-electronics.com/sff-8643/
    Mini___SAS__HD__SFF___8644,      // Also SFF-8614    https://cs-electronics.com/sff-8644/
    SAS__Sideband,      
    SAS__SFF___8680,            //https://cs-electronics.com/sff-8680-2/
    U_2__SFF___8639,            //revision of SFF-8680 https://cs-electronics.com/sff-8639/
    SAS__SFF___8638,            
    SAS__SFF___8640,            
    SAS__SFF___8681,            
    SlimSAS__SFF___8654,            
    END_CATEGORY_Storage__Interface,

    // Fibre Channel
    //https://en.wikipedia.org/wiki/Fibre_Channel
    START_CATEGORY_Fibre__Channel,
    // https://en.wikipedia.org/wiki/Fibre_Channel_electrical_interface
    //DB9,  // Already in D-sub category  
    HSSDC,
    HSSDC2,
    SCA___2,
    //SFP__Plus__SFF___8431,        //already in Storage Interface category
    //Quad__SFP__Plus__SFF___8436,  //already in Storage Interface category
    END_CATEGORY_Fibre__Channel,

    // Optical Fiber
    // https://en.wikipedia.org/wiki/Optical_fiber_connector
    //TODO https://en.wikipedia.org/wiki/Small_form-factor_pluggable_transceiver
    START_CATEGORY_Optical__Fiber,
    Avio____Avim, //Aviation Intermediate Maintenance
    ADT___UNI,
    CS,             //Corning/Senko
    DMI,            //Diamond Micro Interface
    LSH, //E-2000
    EC____CF08,
    ELIO,
    ESCON,          //Enterprise Systems Connection
    F07,
    F___3000,
    FC,             //Ferrule Connector or Fiber Channel
    Fibergate,             
    FJ,      //Fiber-Jack or Opti-Jack
    LC,      //Common in modern Fibre Channel devices. Lucent Connector, Little Connector, or Local Connector
    Luxcis,
    LX___5,
    M12___FO,
    MIC____FDDI,    //Media Interface Connnector / https://en.wikipedia.org/wiki/Fiber_Distributed_Data_Interface
    MPO____MTP,    //Multiple-fiber Push-On/Pull-off
    MT,             //Mechanical Transfer
    MT___RJ,        //Mechanical Transfer Registered Jack or Media Termination - recommended jack
    MU,             //Miniature unit
    SC,             // Common in older 1GFC devices.
    SC__DC,             
    SC__QC,             
    SMA__905____F___SMA__I,             
    SMA__906____F___SMA__II,             
    SMC,             
    ST____BFOC,             //Straight Tip or Bayonet Fiber Optic Connector
    //TOSLINK (already in Audio section)
    VF___45____SG,                //Volition Fiber
    HDTV__1053,             //Broadcast connector interface //NOTE: this is supposed to be 1053 HDTV
    END_CATEGORY_Optical__Fiber,

    // Mobile
    START_CATEGORY_Mobile,
    Lightning,      // https://en.wikipedia.org/wiki/Lightning_(connector)
    //https://en.wikipedia.org/wiki/Dock_connector#30-pin_dock_connector
    Apple__30___pin,     
    Samsung__30___pin,   
    PDMI,           // https://en.wikipedia.org/wiki/PDMI
    END_CATEGORY_Mobile,

    // D-subminiature
    START_CATEGORY_D___sub,
    // https://en.wikipedia.org/wiki/D-subminiature
    // Normal Density
    DA___15,
    DB___25,    // AKA IEEE 1284 Type A. Mandated by RS-232D.
    DC___37,
    DD___50,
    DE___9,     // AKA DB9  https://edac.net/products/db9-connector/145
    // High Density
    DA___26,
    DB___44,
    DC___62,
    DD___78,
    DE___15,
    DF___104,
    // Double Density
    DA___31,
    DB___52,
    DC___79,
    DD___100,
    DE___19,
    // Other
    DB13W3,     //https://en.wikipedia.org/wiki/DB13W3
    // https://en.wikipedia.org/wiki/IEEE_1284
    Centronics, //AKA "Micro Ribbon", "IEEE 1284 Type B". 36-pin.
    Mini___Centronics, //AKA "MDR36", "HPCN36", "IEEE 1284 Type C". 36-pin.
    END_CATEGORY_D___sub,

    // Other Connectors
    // https://en.wikipedia.org/wiki/Registered_jack
    RJ11,
    RJ14,
    RJ25,
    RJ45, //AKA 8P8C
    

};
*/


#define LONGEST_CONNECTOR_STR "USB Micro-B SuperSpeed"

static string NameFromPinType(Connector_ID ptype)
{
    /*string ret = std::string(magic_enum::enum_name(ptype));
    EnumName_Underscore2Symbol(ret);
    return ret;*/
    return ptype;
}

/*
const map<PinType, string> connectorIconFiles
{
    // DC Power
    {PinType::DC__Power__Barrel,             ""},
    {PinType::Molex,                         ""},
    {PinType::SATA__Power,                   ""},
    {PinType::SATA__Power__Slimline,         ""},

    // AC Power
    {PinType::NEMA__5___15,                  "data/nema5-15.jpg"},
    {PinType::NEMA__5___20,                  "data/nema5-20.jpg"},
    {PinType::NEMA__5___30,                  "data/nema5-30.jpg"},
    {PinType::NEMA__5___50,                  "data/nema5-50.jpg"},
    {PinType::NEMA__TT___30,                  "data/nematt-30.jpg"},
    {PinType::NEMA__L5___15,                  "data/nemal5-15.jpg"},
    {PinType::NEMA__L5___20,                  "data/nemal5-20.jpg"},
    {PinType::NEMA__L5___30,                  "data/nemal5-30.jpg"},
    {PinType::NEMA__1___15,                  "data/nema1-15.jpg"},
    {PinType::NEMA__14___20,                  "data/nema14-20.jpg"},
    {PinType::NEMA__14___30,                   "data/nema14-30.jpg"},
    {PinType::NEMA__14___50,                  "data/nema14-50.jpg"},
    {PinType::NEMA__14___60,                  "data/nema14-60.jpg"},
    {PinType::NEMA__L14___20,                  "data/nemal14-20.jpg"},
    {PinType::NEMA__L14___30,                  "data/nemal14-30.jpg"},
    {PinType::NEMA__6___15,                  "data/nema6-15.jpg"},
    {PinType::NEMA__6___20,                  "data/nema6-20.jpg"},
    {PinType::NEMA__6___30,                  "data/nema6-30.jpg"},
    {PinType::NEMA__6___50,                  "data/nema6-50.jpg"},
    {PinType::NEMA__10___30,                  "data/nema10-30.jpg"},
    {PinType::NEMA__10___50,                  "data/nema10-50.jpg"},
    {PinType::NEMA__L6___15,                  "data/nemaL6-15.jpg"},
    {PinType::NEMA__L6___20,                  "data/nemaL6-20.jpg"},
    {PinType::NEMA__L6___30,                  "data/nemaL6-30.jpg"},
    {PinType::NEMA__L6___50,                  "data/nemaL6-50.jpg"},
    {PinType::C1____C2,                      "data/c1c2.png"},
    {PinType::C5____C6,                      "data/c5c6.png"},
    {PinType::C7____C8,                      "data/c7c8.png"},
    {PinType::C9____C10,                      "data/c9c10.png"},
    {PinType::C13____C14,                      "data/c13c14.png"},
    {PinType::C15____C16,                      "data/c15c16.png"},
    {PinType::C15A____C16A,                    "data/c15ac16a.png"},
    {PinType::C17____C18,                      "data/c17c18.png"},
    {PinType::C19____C20,                      "data/c19c20.png"},
    {PinType::C21____C22,                      "data/c21c22.png"},
    {PinType::C23____C24,                      "data/c23c24.png"},

    // USB
    {PinType::USB___A,                       "data/ic_usba.jpg"},
    {PinType::USB___A__SuperSpeed,           "data/ic_usba.jpg"},
    {PinType::USB___B,                       "data/ic_usbb.jpg"},
    {PinType::USB___B__SuperSpeed,           "data/ic_usbb_ss.jpg"},
    {PinType::USB___C,                       "data/ic_usbc.jpg"},
    {PinType::USB__Mini___A,                 ""},
    {PinType::USB__Mini___B,                 "data/ic_usb_mini_b.png"},
    {PinType::USB__Mini___AB,                ""},
    {PinType::USB__Micro___A,                ""},
    {PinType::USB__Micro___B,                "data/ic_usb_micro_b.jpg"},
    {PinType::USB__Micro___AB,               ""},
    {PinType::USB__Micro___B__SuperSpeed,    ""},

    // Display
    {PinType::DisplayPort,                   "data/ic_dp.png"},
    {PinType::Mini__DisplayPort,             "data/ic_mini_dp.png"},
    {PinType::HDMI,                          "data/ic_hdmi.png"},
    {PinType::Mini__HDMI,                    ""},
    {PinType::Micro__HDMI,                   ""},
    {PinType::DVI___D,                       "data/ic_dvi_d.jpg"},
    {PinType::DVI___A,                       "data/ic_dvi_d.jpg"}, //TODO WRONG
    {PinType::DVI___I,                       "data/ic_dvi_d.jpg"}, //TODO WRONG
    {PinType::Mini___DVI,                    "data/ic_mini_dvi.jpg"},
    {PinType::Micro___DVI,                   ""},
    {PinType::VGA,                           "data/ic_vga.jpg"},
    {PinType::Mini___VGA,                    ""},

    // Audio
    {PinType::Audio3_5mm,                    "data/ic_3x_3.5mm.jpg"}, //TODO (somewhat) WRONG
    {PinType::XLR3,                          ""},
    {PinType::TOSLINK,                       "data/ic_toslink.jpg"},
    {PinType::Mini___TOSLINK,                "data/ic_toslink.jpg"}, //TODO WRONG

    // Mini-DIN
    {PinType::PS____2,                       "data/ic_ps2.jpg"},

    // Other 
    {PinType::SATA,                          "data/ic_sata.jpg"},
    {PinType::Micro___SATA,                  ""},
    {PinType::eSATA,                         "data/ic_esata.jpg"},
    {PinType::SD,                            ""},
    {PinType::miniSD,                        ""},
    {PinType::microSD,                       ""},
    {PinType::U_2__SFF___8639,               ""},
    {PinType::RJ11,                          "data/ic_rj11.jpg"}, //TODO check this one
    {PinType::RJ14,                          "data/ic_rj11.jpg"}, //TODO WRONG
    {PinType::RJ25,                          "data/ic_rj11.jpg"}, //TODO WRONG
    {PinType::RJ45,                          "data/ic_rj45.jpg"},
};
*/

static bool GetConnectorMultiplePerPin(Connector_ID ptype)
{
    const auto info = connectors.at(ptype);
    if (info->full_info.contains("MultipleConnectPerPin"))
    {
        if (info->full_info["MultipleConnectPerPin"].get<bool>())
        {
            return true;
        }
    }
    return false;
}

// For a certain male connector, what female connectors besides its female
// same-name counterpart does the connector have physical (and maybe 
// electrical) compatibility with?

static set<Connector_ID> GetCompatibleFemalePinTypes(Connector_ID maletype)
{
    set<Connector_ID> ret;
    ret.insert(maletype);
    const auto info = connectors.at(maletype);
    for (const auto& fits : info->maleFitsInto)
    {
        ret.insert(fits);
    }

    /*
    switch (maletype)
    {
        // DC Power
    case PinType::PCIe__6___pin:
        ret.insert(PinType::PCIe__8___pin); //Unsure if this is correct?
        break;
    case PinType::PCIe__6___2___pin:
        ret.insert(PinType::PCIe__6___pin);
        ret.insert(PinType::PCIe__8___pin);
        break;

        // AC Power
        //TODO revise NEMA compatibility
    case PinType::NEMA__1___15:
        ret.insert(PinType::NEMA__5___15);
        ret.insert(PinType::NEMA__5___20);
        break;
    case PinType::NEMA__5___15:
        ret.insert(PinType::NEMA__5___20);
        break;
    case PinType::NEMA__14___20:
        ret.insert(PinType::NEMA__14___30);
        break;
    //case PinType::NEMA__6___15:
    //    ret.insert(PinType::NEMA__6___20);
    //    break;

        // USB                               
    case PinType::USB___A:
        ret.insert(PinType::USB___A__SuperSpeed);
        ret.insert(PinType::eSATAp);
        break;
    case PinType::USB___A__SuperSpeed:
        ret.insert(PinType::USB___A);
        ret.insert(PinType::eSATAp);
        break;
        // TODO check if USB-B has the same backwards compatibility

        // Display                           
        //NOTE: DVI-I male connectors cannot be inserted into DVI-D female connectors,
        //   but DVI-D male connectors can be inserted into DVI-I female connectors.              
    case PinType::DVI___D:
        ret.insert(PinType::DVI___I);
        break;

        // Expansion Slot

        //AGP__3_3V,
        //    AGP__1_5V,
        //    AGP__Universal,
        //    AGP__Pro__3_3V,
        //    AGP__Pro__1_5V,
        //    AGP__Pro__Universal,

    case PinType::AGP__3_3V:
        ret.insert(PinType::AGP__Universal);
        ret.insert(PinType::AGP__Pro__3_3V);
        ret.insert(PinType::AGP__Pro__Universal);
        break;
    case PinType::AGP__1_5V:
        ret.insert(PinType::AGP__Universal);
        ret.insert(PinType::AGP__Pro__1_5V);
        ret.insert(PinType::AGP__Pro__Universal);
        break;
    case PinType::AGP__Universal:
        ret.insert(PinType::AGP__Pro__Universal);
        break;
    case PinType::AGP__Pro__3_3V:
        ret.insert(PinType::AGP__Pro__Universal);
        break;
    case PinType::AGP__Pro__1_5V:
        ret.insert(PinType::AGP__Pro__Universal);
        break;

    case PinType::PCIe__x1:
        ret.insert(PinType::PCIe__x2);
        ret.insert(PinType::PCIe__x4);
        ret.insert(PinType::PCIe__x8);
        ret.insert(PinType::PCIe__x16);
        break;
    case PinType::PCIe__x2:
        ret.insert(PinType::PCIe__x4);
        ret.insert(PinType::PCIe__x8);
        ret.insert(PinType::PCIe__x16);
        break;
    case PinType::PCIe__x4:
        ret.insert(PinType::PCIe__x8);
        ret.insert(PinType::PCIe__x16);
        break;
    case PinType::PCIe__x8:
        ret.insert(PinType::PCIe__x16);
        break;

    case PinType::MXM___I:
        ret.insert(PinType::MXM___II);
        ret.insert(PinType::MXM___III);
        break;
    case PinType::MXM___II:
        ret.insert(PinType::MXM___III);
        break;
    case PinType::MXM___A:
        ret.insert(PinType::MXM___B);
        break;


        // Audio                             

        // DIN

        // Mini-DIN

        // Storage Interface
    case PinType::eSATA:
        ret.insert(PinType::eSATAp);
        break;
    case PinType::M_2__B__M__key:
        ret.insert(PinType::M_2__B__key);
        ret.insert(PinType::M_2__M__key);
        break;

        // Mobile

        // Other                             

    default:
        break;
    }
    */
    return ret;
}



// Order of male vs female inputs matters!
static bool IsCompatiblePinType(Connector_ID male, Connector_ID female)
{
    auto candidates = GetCompatibleFemalePinTypes(male);
    if (candidates.find(female) != candidates.end())
        return true;
    else
        return false;
}

///////////////////////////////
/// CATEGORIES

//struct ConnectorCategoryInfo : CategoryInfo
//{
//    vector<PinType> connectors;
//};
//
//static vector<PinType> GetUncategorizedConnectors()
//{
//    vector<PinType> ret;
//
//    bool in_category = false;
//    for (const auto& possible_connect : magic_enum::enum_values<PinType>())
//    {
//        if (possible_connect == PinType::UNRECOGNIZED)
//            continue;
//
//        auto connect_string = NameFromPinType(possible_connect);
//        EnumName_Underscore2Symbol(connect_string);
//        auto cat_strpos = connect_string.find(".CATEGORY.");
//        if (cat_strpos != string::npos)
//        {
//            if (!in_category)
//            {
//                in_category = true;
//            }
//            else
//            {
//                in_category = false;
//            }
//        }
//        else
//        {
//            if (!in_category)
//                ret.push_back(possible_connect);
//        }
//    }
//
//    return ret;
//}
//
//static vector<ConnectorCategoryInfo> GetConnectorCategories()
//{
//    vector<ConnectorCategoryInfo> ret;
//
//    bool in_category = false;
//    ConnectorCategoryInfo current_category;
//    for (const auto& possible_connect : magic_enum::enum_values<PinType>())
//    {
//        if (possible_connect == PinType::UNRECOGNIZED)
//            continue;
//
//        auto connect_string = NameFromPinType(possible_connect);
//        EnumName_Underscore2Symbol(connect_string);
//        auto cat_strpos = connect_string.find(".CATEGORY.");
//        if (cat_strpos != string::npos)
//        {
//            if (!in_category)
//            {
//                auto name_strpos = cat_strpos + string(".CATEGORY.").length();
//                current_category.name = connect_string.substr(name_strpos);
//                current_category.index_of_first = magic_enum::enum_index<PinType>(possible_connect).value() + 1;
//                in_category = true;
//            }
//            else
//            {
//                current_category.index_after_last = magic_enum::enum_index<PinType>(possible_connect).value();
//                in_category = false;
//                ret.push_back(current_category);
//                current_category.connectors.clear();
//            }
//        }
//        else
//        {
//            if (in_category)
//                current_category.connectors.push_back(possible_connect);
//        }
//    }
//
//    return ret;
//}
