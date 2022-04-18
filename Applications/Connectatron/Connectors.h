// Definitions for all connectors, which each are (usually) incompatible with each other.
#pragma once
#include "Connectatron_Utils.h"

#include <set>

using std::set;

// Variable name to string parsing:
// _    : .
// __   : (space) 
// ___  : -
// ____ : /
enum class PinType
{
    // Power
    DC__Power__Barrel, //TODO specific
    Molex,
    SATA__Power,
    SATA__Power__Slimline,
    // https://en.wikipedia.org/wiki/NEMA_connector
    //TODO NEMA connectors

    // USB
    USB___A,
    //USB___A__SuperSpeed, 
    // PinType does not discriminate between USB-A and USB-A SuperSpeed, even though they have 
    // additional pins, as they physically fit together and have at least some backward compatibility.
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
    DisplayPort,
    Mini__DisplayPort,
    HDMI,
    Mini__HDMI,
    Micro__HDMI,
    DVI,
    VGA,

    // Audio
    Audio3_5mm, //Should probably specify stereo/not?
    XLR, //?
    //https://en.wikipedia.org/wiki/TOSLINK
    TOSLINK,
    Mini___TOSLINK,

    // Other
    SATA,
    Micro__SATA,
    eSATA,
    //https://en.wikipedia.org/wiki/SD_card
    SD,
    miniSD,
    microSD,
    SFF___8639,
    //https://en.wikipedia.org/wiki/Registered_jack
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