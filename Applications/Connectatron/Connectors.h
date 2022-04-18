// Definitions for all connectors, which each are (usually) incompatible with each other.
#pragma once
#include "Connectatron_Utils.h"

#include <set>
#include <map>
#include <string>

using std::set;
using std::map;

// Variable name to string parsing:
// _    : .
// __   : (space) 
// ___  : -
// ____ : /
enum class PinType
{
    // DC Power
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
    Molex,
    SATA__Power,
    SATA__Power__Slimline,
    
    // AC Power
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
    //TODO non-NEMA connectors

    // USB          // https://en.wikipedia.org/wiki/USB
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

    // Display
    // https://en.wikipedia.org/wiki/DisplayPort
    DisplayPort,
    Mini__DisplayPort,
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
    VGA,
    Mini___VGA,

    // Audio
    Audio3_5mm, //Should probably specify stereo/not?
    XLR, //?
    // https://en.wikipedia.org/wiki/TOSLINK
    TOSLINK,
    Mini___TOSLINK,

    // DIN          // https://en.wikipedia.org/wiki/DIN_connector

    // Mini-DIN     // https://en.wikipedia.org/wiki/Mini-DIN_connector
    PS____2,        // https://en.wikipedia.org/wiki/PS/2_port
    //TODO others from wiki article

    // Other
    SATA,
    Micro__SATA,
    eSATA,
    // https://en.wikipedia.org/wiki/SD_card
    SD,
    miniSD,
    microSD,
    SFF___8639,
    // https://en.wikipedia.org/wiki/Registered_jack
    RJ11,
    RJ14,
    RJ25,
    RJ45, //AKA 8P8C
};

#define LONGEST_CONNECTOR_STR "USB Micro-B SuperSpeed"

static string NameFromPinType(PinType ptype)
{
    string ret = std::string(magic_enum::enum_name(ptype));
    EnumName_Underscore2Symbol(ret);
    return ret;
}


const map<PinType, string> connectorIconFiles
{
    // DC Power
    {PinType::DC__Power__Barrel,             ""},
    {PinType::Molex,                         ""},
    {PinType::SATA__Power,                   ""},
    {PinType::SATA__Power__Slimline,         ""},

    // USB
    {PinType::USB___A,                       "data/ic_usba.jpg"},
    {PinType::USB___A__SuperSpeed,           "data/ic_usba.jpg"},
    {PinType::USB___B,                       ""},
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
    {PinType::DVI___D,                       ""},
    {PinType::DVI___A,                       ""},
    {PinType::DVI___I,                       ""},
    {PinType::Mini___DVI,                    "data/ic_mini_dvi.jpg"},
    {PinType::Micro___DVI,                   ""},
    {PinType::VGA,                           "data/ic_vga.jpg"},
    {PinType::Mini___VGA,                    ""},

    // Audio
    {PinType::Audio3_5mm,                    ""},
    {PinType::XLR,                           ""},
    {PinType::TOSLINK,                       "data/ic_toslink.jpg"},
    {PinType::Mini___TOSLINK,                ""},

    // Mini-DIN
    {PinType::PS____2,                       "data/ic_ps2.jpg"},

    // Other 
    {PinType::SATA,                          ""},
    {PinType::Micro__SATA,                   ""},
    {PinType::eSATA,                         ""},
    {PinType::SD,                            ""},
    {PinType::miniSD,                        ""},
    {PinType::microSD,                       ""},
    {PinType::SFF___8639,                    ""},
    {PinType::RJ11,                          "data/ic_rj11.jpg"},
    {PinType::RJ14,                          ""},
    {PinType::RJ25,                          ""},
    {PinType::RJ45,                          "data/ic_rj45.jpg"},
};

//TODO reinstate and update this if necessary.
//static set<PinType> GetCompatibleMalePinTypes(PinType femaletype)
//{
//    set<PinType> ret;
//    ret.insert(femaletype);
//
//    switch (femaletype)
//    {
//        //Power
//
//        //USB                               
//    case PinType::USB___A:
//        ret.insert(PinType::USB___A__SuperSpeed);
//        break;
//    case PinType::USB___A__SuperSpeed:
//        ret.insert(PinType::USB___A);
//        break;
//
//        //Display             
//        //NOTE: DVI-I male connectors cannot be inserted into DVI-D female connectors,
//        //   but the opposite works fine.              
//    case PinType::DVI___I:
//        ret.insert(PinType::DVI___D);
//        break;
//        //Audio                             
//
//        //Other                             
//
//    default:
//        break;
//    }
//
//    return ret;
//}

static set<PinType> GetCompatibleFemalePinTypes(PinType maletype)
{
    set<PinType> ret;
    ret.insert(maletype);

    switch (maletype)
    {
        // DC Power

        // USB                               
    case PinType::USB___A:
        ret.insert(PinType::USB___A__SuperSpeed);
        break;
    case PinType::USB___A__SuperSpeed:
        ret.insert(PinType::USB___A);
        break;

        // Display                           
        //NOTE: DVI-I male connectors cannot be inserted into DVI-D female connectors,
        //   but DVI-D male connectors can be inserted into DVI-I female connectors.              
    case PinType::DVI___D:
        ret.insert(PinType::DVI___I);
        break;

        // Audio                             

        // Other                             

    default:
        break;
    }

    return ret;
}

// Order of male vs female inputs matters!
static bool IsCompatiblePinType(PinType male, PinType female)
{
    auto candidates = GetCompatibleFemalePinTypes(male);
    if (candidates.find(female) != candidates.end())
        return true;
    else
        return false;
}
