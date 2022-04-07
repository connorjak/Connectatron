#include <application.h>
#include "utilities/builders.h"
#include "utilities/widgets.h"

#include <imgui_node_editor.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "ImGuiFileDialog.h"

//https://stackoverflow.com/questions/201593/is-there-a-simple-way-to-convert-c-enum-to-string/238157#238157
//https://stackoverflow.com/questions/28828957/enum-to-string-in-modern-c11-c14-c17-and-future-c20
#include <magic_enum.hpp>

#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <utility>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <random>
#include <memory>

using namespace nlohmann;
namespace fs = std::filesystem;
using std::string;
using std::vector;
using std::set;
using std::shared_ptr;
using std::weak_ptr;
using std::make_shared;

// placeholder for UUID
using NotUUID = int;

//#define IS_SHIPPING

#ifdef IS_SHIPPING
const fs::path DevicesPath = "Devices";
const fs::path ProjectsPath = "Projects";
#else
const fs::path DevicesPath = "../../../../../Applications/Connectatron/Devices";
const fs::path ProjectsPath = "../../../../../Applications/Connectatron/Projects";
#endif

//https://www.codespeedy.com/hsv-to-rgb-in-cpp/
std::array<float,3> HSVtoRGB(float H, float S, float V) {
    if (H > 360 || H < 0 || S>100 || S < 0 || V>100 || V < 0) {
        throw std::runtime_error("The givem HSV values are not in valid range");
        return {0,0,0};
    }
    float s = S / 100;
    float v = V / 100;
    float C = s * v;
    float X = C * (1 - abs(fmod(H / 60.0, 2) - 1));
    float m = v - C;
    float r, g, b;
    if (H >= 0 && H < 60) {
        r = C, g = X, b = 0;
    }
    else if (H >= 60 && H < 120) {
        r = X, g = C, b = 0;
    }
    else if (H >= 120 && H < 180) {
        r = 0, g = C, b = X;
    }
    else if (H >= 180 && H < 240) {
        r = 0, g = X, b = C;
    }
    else if (H >= 240 && H < 300) {
        r = X, g = 0, b = C;
    }
    else {
        r = C, g = 0, b = X;
    }
    float R = (r + m);
    float G = (g + m);
    float B = (b + m);
    return { R,G,B };
}


static inline ImRect ImGui_GetItemRect()
{
    return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

static inline ImRect ImRect_Expanded(const ImRect& rect, float x, float y)
{
    auto result = rect;
    result.Min.x -= x;
    result.Min.y -= y;
    result.Max.x += x;
    result.Max.y += y;
    return result;
}

namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;

using namespace ax;

using ax::Widgets::IconType;

static ed::EditorContext* m_Editor = nullptr;

static string CurrentProjectName = "my_project.con";

constexpr double HSV_HueFromStringOffset = 90;
constexpr double HSV_ValFromStringOffset = 0;

//extern "C" __declspec(dllimport) short __stdcall GetAsyncKeyState(int vkey);
//extern "C" bool Debug_KeyPress(int vkey)
//{
//    static std::map<int, bool> state;
//    auto lastState = state[vkey];
//    state[vkey] = (GetAsyncKeyState(vkey) & 0x8000) != 0;
//    if (state[vkey] && !lastState)
//        return true;
//    else
//        return false;
//}

// *** RESEARCH ***

// https://en.wikipedia.org/wiki/List_of_computer_standards
// https://en.wikipedia.org/wiki/Computer_port_(hardware)#Types_of_ports

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

struct PowerDelivery
{
    double PowerContribution;
    double minVoltage;
    double maxVoltage;
    double minCurrent;
    double maxCurrent;
};


// Variable name to string parsing:
// _   : .
// __  : (space) 
// ___ : -
enum class WireProtocol
{
    // Power
    DC__Power,
    AC__Power,
    //https://en.wikipedia.org/wiki/NEMA_connector

    // Analog Single-Wire
    GenericAnalog, //Not sure if we actually need to specify protocols here
    AnalogAudio, //Placeholder until I get a proper name for this

    // Single-Wire
    //https://en.wikipedia.org/wiki/System_Management_Bus
    UART,
    SPI,
    //https://en.wikipedia.org/wiki/I%C2%B2C
    I2C,

    // USB
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

    // FireWire
    //https://en.wikipedia.org/wiki/IEEE_1394
    MIN_VERSION_FireWire,
    FireWire__400__1394, //IEEE 1394-1995
    FireWire__400__1394a, //IEEE 1394a-2000
    FireWire__800__1394b, //IEEE 1394b-2002
    FireWire__S800T__1394c, //IEEE 1394c-2006
    FireWire__S1600, //IEEE 1394-2008
    FireWire__S3200, //IEEE 1394-2008
    MAX_VERSION_FireWire,

    // Display
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

    // Display Metadata (may be a part of another display cable protocol)
    //https://en.wikipedia.org/wiki/Display_Data_Channel
    DDC1, 
    DDC2B, 
    E___DDC, 
    //https://en.wikipedia.org/wiki/Extended_Display_Identification_Data
    EDID,

    // High-Bandwidth
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

    // Storage Interface
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

    // Ethernet
    //https://en.wikipedia.org/wiki/Ethernet_over_twisted_pair
    //https://en.wikipedia.org/wiki/Fast_Ethernet#100BASE-T1
    MIN_VERSION_Ethernet,
    Ethernet__10BASE___T, //10 Mbps
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
    MAX_VERSION_Ethernet,

    // Other
    Thunderbolt__3,
    Thunderbolt__4,
};

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

    // Other
    SATA,
    Micro__SATA,
    eSATA,
    //https://en.wikipedia.org/wiki/SD_card
    SD,
    miniSD,
    microSD,
    SFF___8639,
    RJ45, //AKA 8P8C


    
};

enum class PinKind
{
    Output,
    Input
};

enum class NodeType
{
    Blueprint,
    Simple,
    Tree,
    Comment,
    Houdini
};

struct Node;

struct Pin
{
    ed::PinId   ID;
    ::shared_ptr<Node> Node;
    std::string Name;
    // Physical connector type
    PinType     Type;
    // Protocols available for communication through this connector
    set<WireProtocol> Protocols;
    std::string ExtraInfo;
    bool IsFemale = true;
    PinKind     Kind;

    Pin(int id, const char* name, PinType type):
        ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
    {
    }
    Pin(int id, const char* name, PinType type, set<WireProtocol> protocols) :
        ID(id), Node(nullptr), Name(name), Type(type), Protocols(protocols), Kind(PinKind::Input)
    {
    }
    Pin(int id, const char* name, PinType type, set<WireProtocol> protocols, std::string extraInfo) :
        ID(id), Node(nullptr), Name(name), Type(type), Protocols(protocols), ExtraInfo(extraInfo), Kind(PinKind::Input)
    {
    }
    Pin(int id, const char* name, PinType type, set<WireProtocol> protocols, std::string extraInfo, bool isFemale) :
        ID(id), Node(nullptr), Name(name), Type(type), Protocols(protocols), ExtraInfo(extraInfo), IsFemale(isFemale), Kind(PinKind::Input)
    {
    }
};

struct Node
{
    ed::NodeId ID;
    NotUUID persistentID;
    std::string Name;
    std::vector<Pin> Females;
    std::vector<Pin> Males;
    ImColor Color;
    NodeType Type;
    ImVec2 Size;
    // Position saved in project file
    ImVec2 SavedPosition;

    std::string State;
    std::string SavedState;

    Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)) :
        ID(id), persistentID(-1), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0), SavedPosition(0, 0)
    {
    }
};

struct Link
{
    ed::LinkId ID;

    ed::PinId StartPinID;
    ed::PinId EndPinID;

    ImColor Color;

    Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId):
        ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
    {
    }
};


static std::random_device dev;
static std::mt19937 rng(dev());

static NotUUID Generate_NotUUID()
{
    std::uniform_int_distribution<std::mt19937::result_type> distPosInt(1, 100000); //2147483647 

    return distPosInt(rng);
}

struct NodeIdLess
{
    bool operator()(const ed::NodeId& lhs, const ed::NodeId& rhs) const
    {
        return lhs.AsPointer() < rhs.AsPointer();
    }
};

#define IM_COLOR_BLUE       ImColor(51, 150, 215)
#define IM_COLOR_RED        ImColor(220, 48, 48)
#define IM_COLOR_MAGENTA    ImColor(218, 0, 183)
#define IM_COLOR_GREEN      ImColor(147, 226, 74)



//https://stackoverflow.com/a/24315631/11502722
void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
}

static void EnumName_Underscore2Symbol(std::string& symboled)
{
    ReplaceAll(symboled, "___", "-");
    ReplaceAll(symboled, "__", " ");
    ReplaceAll(symboled, "_", ".");
}

static void EnumName_Symbol2Underscore(std::string& symboled)
{
    ReplaceAll(symboled, "-", "___");
    ReplaceAll(symboled, " ", "__");
    ReplaceAll(symboled, ".", "_");
}

static json GetJSONFromFile(fs::path filepath)
{
    std::ifstream i(filepath);
    json js;
    i >> js;
    return js;
}

static json SerializeDeviceToJSON(const shared_ptr<Node> node)
{
    json device;

    device["Name"] = node->Name;

    /*NotUUID id = -1;
    if (device.find("ID") != device.end())
    {
        id = device["ID"].get<int>();
    }
    else
    {
        id = Generate_NotUUID();
    }*/
    device["ID"] = node->persistentID;

    auto pos = ed::GetNodePosition(node->ID);
    device["SavedPos"][0] = pos.x;
    device["SavedPos"][1] = pos.y;
    
    // Parse Females
    device["Females"] = json::array();
    auto& j_Females = device["Females"];
    for (const auto& female : node->Females)
    {
        auto& j_female = j_Females.emplace_back(json::object());
        j_female["Name"] = female.Name;

        auto pintype = female.Type;
        auto pintype_str = string(magic_enum::enum_name(pintype));
        EnumName_Underscore2Symbol(pintype_str);
        j_female["PinType"] = pintype_str;

        //Can have just pintype, or pintype+protocols, or pintype+protocols+description
        if (female.Protocols.size() != 0)
        {
            j_female["Protocols"] = json::array();
            auto& j_Protocols = j_female["Protocols"];
            for (auto protocol : female.Protocols)
            {
                auto protocol_string = string(magic_enum::enum_name(protocol));
                EnumName_Underscore2Symbol(protocol_string);

                j_Protocols.push_back(protocol_string);
                //TODO handle "sets" of protocols, like the backcompat variables?
            }
        }

        if (female.ExtraInfo.size() != 0)
        {
            j_female["Description"] = female.ExtraInfo;
        }
    }


    // Parse Males
    device["Males"] = json::array();
    auto& j_Males = device["Males"];
    for (const auto& male : node->Males)
    {
        auto& j_male = j_Males.emplace_back(json::object());
        j_male["Name"] = male.Name;

        auto pintype = male.Type;
        auto pintype_str = string(magic_enum::enum_name(pintype));
        EnumName_Underscore2Symbol(pintype_str);
        j_male["PinType"] = pintype_str;

        //Can have just pintype, or pintype+protocols, or pintype+protocols+description
        if (male.Protocols.size() != 0)
        {
            j_male["Protocols"] = json::array();
            auto& j_Protocols = j_male["Protocols"];
            for (auto protocol : male.Protocols)
            {
                auto protocol_string = string(magic_enum::enum_name(protocol));
                EnumName_Underscore2Symbol(protocol_string);

                j_Protocols.push_back(protocol_string);
                //TODO handle "sets" of protocols, like the backcompat variables?
            }
        }

        if (male.ExtraInfo.size() != 0)
        {
            j_male["Description"] = male.ExtraInfo;
        }
    }

    return device;
}

static bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
{
    using namespace ImGui;
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID("##Splitter");
    ImRect bb;
    bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
    bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
    return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}

struct Connectatron:
    public Application
{
    using Application::Application;

    int GetNextId()
    {
        return m_NextId++;
    }

    //ed::NodeId GetNextNodeId()
    //{
    //    return ed::NodeId(GetNextId());
    //}

    ed::LinkId GetNextLinkId()
    {
        return ed::LinkId(GetNextId());
    }

    void TouchNode(ed::NodeId id)
    {
        m_NodeTouchTime[id] = m_TouchTime;
    }

    float GetTouchProgress(ed::NodeId id)
    {
        auto it = m_NodeTouchTime.find(id);
        if (it != m_NodeTouchTime.end() && it->second > 0.0f)
            return (m_TouchTime - it->second) / m_TouchTime;
        else
            return 0.0f;
    }

    void UpdateTouch()
    {
        const auto deltaTime = ImGui::GetIO().DeltaTime;
        for (auto& entry : m_NodeTouchTime)
        {
            if (entry.second > 0.0f)
                entry.second -= deltaTime;
        }
    }

    shared_ptr<Node> FindNode(ed::NodeId id)
    {
        for (auto& node : m_Nodes)
            if (node->ID == id)
                return node;

        return nullptr;
    }

    Link* FindLink(ed::LinkId id)
    {
        for (auto& link : m_Links)
            if (link.ID == id)
                return &link;

        return nullptr;
    }

    Pin* FindPin(ed::PinId id)
    {
        if (!id)
            return nullptr;

        for (auto& node : m_Nodes)
        {
            for (auto& pin : node->Females)
                if (pin.ID == id)
                    return &pin;

            for (auto& pin : node->Males)
                if (pin.ID == id)
                    return &pin;
        }

        return nullptr;
    }

    bool IsPinLinked(ed::PinId id)
    {
        if (!id)
            return false;

        for (auto& link : m_Links)
            if (link.StartPinID == id || link.EndPinID == id)
                return true;

        return false;
    }

    bool CanCreateLink(Pin* a, Pin* b)
    {
        if (!a || !b || a == b || a->Kind == b->Kind || a->Type != b->Type || a->Node == b->Node)
            return false;

        return true;
    }

    //void DrawItemRect(ImColor color, float expand = 0.0f)
    //{
    //    ImGui::GetWindowDrawList()->AddRect(
    //        ImGui::GetItemRectMin() - ImVec2(expand, expand),
    //        ImGui::GetItemRectMax() + ImVec2(expand, expand),
    //        color);
    //};

    //void FillItemRect(ImColor color, float expand = 0.0f, float rounding = 0.0f)
    //{
    //    ImGui::GetWindowDrawList()->AddRectFilled(
    //        ImGui::GetItemRectMin() - ImVec2(expand, expand),
    //        ImGui::GetItemRectMax() + ImVec2(expand, expand),
    //        color, rounding);
    //};

    void BuildNode(shared_ptr<Node> node)
    {
        for (auto& female : node->Females)
        {
            female.Node = node;
            female.Kind = PinKind::Input;
            female.IsFemale = true;
        }

        for (auto& male : node->Males)
        {
            male.Node = node;
            male.Kind = PinKind::Output;
            male.IsFemale = false;
        }
    }

    //TODO support saving comments
    /*Node* SpawnComment()
    {
        m_Nodes.emplace_back(GetNextId(), "Test Comment");
        m_Nodes.back().Type = NodeType::Comment;
        m_Nodes.back().Size = ImVec2(300, 200);

        return &m_Nodes.back();
    }*/

    void BuildNodes()
    {
        for (auto node : m_Nodes)
            BuildNode(node);
    }

    void OnStart() override
    {
        ed::Config config;

        config.SettingsFile = "Blueprints.json";

        config.UserPointer = this;

        config.LoadNodeSettings = [](ed::NodeId nodeId, char* data, void* userPointer) -> size_t
        {
            auto self = static_cast<Connectatron*>(userPointer);

            auto node = self->FindNode(nodeId);
            if (!node)
                return 0;

            if (data != nullptr)
                memcpy(data, node->State.data(), node->State.size());
            return node->State.size();
        };

        config.SaveNodeSettings = [](ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
        {
            auto self = static_cast<Connectatron*>(userPointer);

            auto node = self->FindNode(nodeId);
            if (!node)
                return false;

            node->State.assign(data, size);

            self->TouchNode(nodeId);

            return true;
        };

        m_Editor = ed::CreateEditor(&config);
        ed::SetCurrentEditor(m_Editor);

        ed::NavigateToContent();

        BuildNodes();

        m_HeaderBackground = LoadTexture("data/BlueprintBackground.png");
        m_SaveIcon         = LoadTexture("data/ic_save_white_24dp.png");
        m_RestoreIcon      = LoadTexture("data/ic_restore_white_24dp.png");


        //auto& io = ImGui::GetIO();
    }

    void OnStop() override
    {
        auto releaseTexture = [this](ImTextureID& id)
        {
            if (id)
            {
                DestroyTexture(id);
                id = nullptr;
            }
        };

        releaseTexture(m_RestoreIcon);
        releaseTexture(m_SaveIcon);
        releaseTexture(m_HeaderBackground);

        if (m_Editor)
        {
            ed::DestroyEditor(m_Editor);
            m_Editor = nullptr;
        }
    }

    ImColor GetIconColor(PinType type)
    {
        switch (type)
        {
        default:

            //Power
        case PinType::DC__Power__Barrel:          return ImColor(255, 255, 255);
        case PinType::Molex:                    return ImColor(255, 255, 255);
        case PinType::SATA__Power:               return ImColor(255, 255, 255);
        case PinType::SATA__Power__Slimline:      return ImColor(255, 255, 255);

            //USB                               
        case PinType::USB___A:                       return IM_COLOR_BLUE;
        case PinType::USB___B:                       return IM_COLOR_BLUE;
        case PinType::USB___B__SuperSpeed:          return IM_COLOR_BLUE;
        case PinType::USB___C:                       return IM_COLOR_BLUE;
        case PinType::USB__Mini___A:                return IM_COLOR_BLUE;
        case PinType::USB__Mini___B:                return IM_COLOR_BLUE;
        case PinType::USB__Mini___AB:               return IM_COLOR_BLUE;
        case PinType::USB__Micro___A:               return IM_COLOR_BLUE;
        case PinType::USB__Micro___B:               return IM_COLOR_BLUE;
        case PinType::USB__Micro___AB:              return IM_COLOR_BLUE;
        case PinType::USB__Micro___B__SuperSpeed:  return IM_COLOR_BLUE;

            //Display                           
        case PinType::DisplayPort:              return IM_COLOR_MAGENTA;
        case PinType::Mini__DisplayPort:          return IM_COLOR_MAGENTA;
        case PinType::HDMI:                     return IM_COLOR_MAGENTA;
        case PinType::Mini__HDMI:                 return IM_COLOR_MAGENTA;
        case PinType::Micro__HDMI:                return IM_COLOR_MAGENTA;
        case PinType::DVI:                      return IM_COLOR_MAGENTA;
        case PinType::VGA:                      return IM_COLOR_MAGENTA;

            //Audio                             
        case PinType::Audio3_5mm:               return IM_COLOR_GREEN;
        case PinType::XLR:                      return IM_COLOR_GREEN;

            //Other                             
        case PinType::SATA:                     return IM_COLOR_RED;
        case PinType::Micro__SATA:                return IM_COLOR_RED;
        case PinType::eSATA:                    return IM_COLOR_RED;
        case PinType::RJ45:                     return IM_COLOR_RED;
        }
    };

    void DrawPinIcon(const Pin& pin, bool connected, int alpha)
    {
        //Set to a default in case the PinType is not supported (somehow)
        IconType iconType = IconType::Square;
        ImColor  color = GetIconColor(pin.Type);
        color.Value.w = alpha / 255.0f;
        switch (pin.Type)
        {

            //Power
        case PinType::DC__Power__Barrel:          iconType = IconType::Flow; break;
        case PinType::Molex:                    iconType = IconType::Flow; break;
        case PinType::SATA__Power:               iconType = IconType::Flow; break;
        case PinType::SATA__Power__Slimline:      iconType = IconType::Flow; break;

            //USB                                   
        case PinType::USB___A:                           iconType = IconType::Circle; break;
        case PinType::USB___B:                           iconType = IconType::Circle; break;
        case PinType::USB___B__SuperSpeed:              iconType = IconType::Circle; break;
        case PinType::USB___C:                           iconType = IconType::Circle; break;
        case PinType::USB__Mini___A:                    iconType = IconType::Circle; break;
        case PinType::USB__Mini___B:                    iconType = IconType::Circle; break;
        case PinType::USB__Mini___AB:                   iconType = IconType::Circle; break;
        case PinType::USB__Micro___A:                   iconType = IconType::Circle; break;
        case PinType::USB__Micro___B:                   iconType = IconType::Circle; break;
        case PinType::USB__Micro___AB:                  iconType = IconType::Circle; break;
        case PinType::USB__Micro___B__SuperSpeed:      iconType = IconType::Circle; break;

            //Display                               
        case PinType::DisplayPort:              iconType = IconType::Grid; break;
        case PinType::Mini__DisplayPort:          iconType = IconType::Grid; break;
        case PinType::HDMI:                     iconType = IconType::Grid; break;
        case PinType::Mini__HDMI:                 iconType = IconType::Grid; break;
        case PinType::Micro__HDMI:                iconType = IconType::Grid; break;
        case PinType::DVI:                      iconType = IconType::Grid; break;
        case PinType::VGA:                      iconType = IconType::Grid; break;

            //Audio                                 
        case PinType::Audio3_5mm:               iconType = IconType::Diamond; break;
        case PinType::XLR:                      iconType = IconType::Diamond; break;

            //Other                                 
        case PinType::SATA:                     iconType = IconType::Circle; break;
        case PinType::Micro__SATA:                iconType = IconType::Circle; break;
        case PinType::eSATA:                    iconType = IconType::Circle; break;
        case PinType::RJ45:                     iconType = IconType::Circle; break;

        default:
            throw std::runtime_error(std::string("Unhandled PinType ") + std::string(magic_enum::enum_name(pin.Type)));
            return;
        }

        ax::Widgets::Icon(ImVec2(m_PinIconSize, m_PinIconSize), iconType, connected, color, ImColor(32, 32, 32, alpha));
    };

    //TODO error handling (probably just wrap in try/catch when called)
//TODO load position in project?
    shared_ptr<Node> SpawnNodeFromJSON(const json& device)
    {
        string name = device["Name"].get<string>();
        auto name_hash = std::hash<string>{}(name);
        auto name_hash_dbl = name_hash / std::pow(10, 10);
        auto name_hash_dbl2 = name_hash / std::pow(10, 6);
        auto hue = std::fmod(name_hash_dbl + HSV_HueFromStringOffset, 360);
        auto val = std::fmod(name_hash_dbl2 + HSV_ValFromStringOffset, 50) + 25;
        // *FromStringOffset is for arbitrarily shifting the color selection.

        auto rgb = HSVtoRGB(hue, 100, val);
        ImColor colFromName(rgb[0], rgb[1], rgb[2]);
        //ImColor(255, 128, 128)
        auto new_node = m_Nodes.emplace_back(new Node(GetNextId(), name.c_str(), colFromName));

        NotUUID id = -1;
        if (device.find("ID") != device.end())
        {
            id = device["ID"].get<int>();
        }
        else
        {
            id = Generate_NotUUID();
        }
        new_node->persistentID = id;

        InitNodeFromJSON(device, new_node);

        m_IdNodes[id] = new_node;

        return new_node;
    }

    // Load node WITHOUT SPAWNING IT IN THE EDITOR!
    void InitNodeFromJSON(const json& device, shared_ptr<Node>& new_node)
    {
        if (device.find("SavedPos") != device.end())
        {
            new_node->SavedPosition.x = device["SavedPos"][0].get<float>();
            new_node->SavedPosition.y = device["SavedPos"][1].get<float>();
        }

        // Parse Females
        for (const auto& female : device["Females"])
        {
            auto in_name = female["Name"].get<string>();

            auto pintype_string = female["PinType"].get<string>();
            EnumName_Symbol2Underscore(pintype_string);

            auto parsed_pintype = magic_enum::enum_cast<PinType>(pintype_string);
            auto in_pintype = parsed_pintype.value();

            //Can have just pintype, or pintype+protocols, or pintype+protocols+description
            if (female.find("Protocols") != female.end())
            {
                set<WireProtocol> protocols;
                for (auto protocol : female["Protocols"])
                {
                    auto protocol_string = protocol.get<string>();
                    EnumName_Symbol2Underscore(protocol_string);

                    auto parsed_protocol = magic_enum::enum_cast<WireProtocol>(protocol_string);
                    protocols.insert(parsed_protocol.value());
                    //TODO handle "sets" of protocols, like the backcompat variables?
                }

                if (female.find("Description") != female.end())
                {
                    auto description = female["Description"].get<string>();
                    new_node->Females.emplace_back(GetNextId(), in_name.c_str(), in_pintype, protocols, description);
                }
                else
                {
                    new_node->Females.emplace_back(GetNextId(), in_name.c_str(), in_pintype, protocols);
                }
            }
            else
            {
                new_node->Females.emplace_back(GetNextId(), in_name.c_str(), in_pintype);
            }
        }


        // Parse Males
        for (const auto& male : device["Males"])
        {
            auto in_name = male["Name"].get<string>();

            auto pintype_string = male["PinType"].get<string>();
            EnumName_Symbol2Underscore(pintype_string);

            auto parsed_pintype = magic_enum::enum_cast<PinType>(pintype_string);
            auto in_pintype = parsed_pintype.value();

            //Can have just pintype, or pintype+protocols, or pintype+protocols+description
            if (male.find("Protocols") != male.end())
            {
                set<WireProtocol> protocols;
                for (auto protocol : male["Protocols"])
                {
                    auto protocol_string = protocol.get<string>();
                    EnumName_Symbol2Underscore(protocol_string);

                    auto parsed_protocol = magic_enum::enum_cast<WireProtocol>(protocol_string);
                    protocols.insert(parsed_protocol.value());
                    //TODO handle "sets" of protocols, like the backcompat variables?
                }

                if (male.find("Description") != male.end())
                {
                    auto description = male["Description"].get<string>();
                    new_node->Males.emplace_back(GetNextId(), in_name.c_str(), in_pintype, protocols, description);
                }
                else
                {
                    new_node->Males.emplace_back(GetNextId(), in_name.c_str(), in_pintype, protocols);
                }
            }
            else
            {
                new_node->Males.emplace_back(GetNextId(), in_name.c_str(), in_pintype);
            }
        }

        BuildNode(new_node);
    }

    Link* SpawnLinkFromJSON(const json& connect)
    {
        auto start_p_id = connect[0].get<int>();
        auto end_p_id = connect[1].get<int>();

        auto start_pin_loc = connect[2].get<int>();
        auto end_pin_loc = connect[3].get<int>();

        auto startNode = m_IdNodes.at(start_p_id);
        auto endNode = m_IdNodes.at(end_p_id);
        auto startPinId = startNode->Males[start_pin_loc].ID;
        auto endPinId = endNode->Females[end_pin_loc].ID;

        m_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));

        m_Links.back().Color = GetIconColor(startNode->Males[start_pin_loc].Type);

        return &m_Links.back();
    }

    json SerializeLinkToJSON(const Link& link)
    {
        json connect;

        auto startPin = FindPin(link.StartPinID);
        auto endPin = FindPin(link.EndPinID);
        auto startNode = startPin->Node;
        auto endNode = endPin->Node;
        auto start_p_id = startNode->persistentID;
        auto end_p_id = endNode->persistentID;
        connect[0] = start_p_id;
        connect[1] = end_p_id;


        vector<Pin>* males = &startNode->Males;
        // Find index of pin that matches id with the one we have a pointer to
        auto start_pin_loc = std::find_if(males->begin(), males->end(),
            [startPin](const Pin& pin) { return startPin->ID == pin.ID; })
            - males->begin();

        vector<Pin>* females = &endNode->Females;
        // Find index of pin that matches id with the one we have a pointer to
        auto end_pin_loc = std::find_if(females->begin(), females->end(),
            [endPin](const Pin& pin) { return endPin->ID == pin.ID; })
            - females->begin();
        connect[2] = int(start_pin_loc);
        connect[3] = int(end_pin_loc);

        return connect;
    }

    void SaveProjectToFile(fs::path filepath)
    {
        std::ofstream o(filepath);
        json project;

        project["Devices"] = json::array();
        auto& devices = project["Devices"];

        for (const auto& node : m_Nodes)
        {
            devices.push_back(SerializeDeviceToJSON(node));
        }

        project["Links"] = json::array();
        auto& connects = project["Links"];

        for (const auto& link : m_Links)
        {
            connects.push_back(SerializeLinkToJSON(link));
        }

        o << project;
        o.close();
    }

    void LoadProjectFromFile(fs::path filepath)
    {
        std::ifstream i(filepath);
        json project;
        i >> project;

        m_Nodes.clear();
        m_IdNodes.clear();
        m_Links.clear();

        const auto& Devices = project["Devices"];

        for (const auto& device : Devices)
        {
            auto node = SpawnNodeFromJSON(device);

            ed::SetNodePosition(node->ID, node->SavedPosition);
        }

        const auto& Connects = project["Links"];
        for (const auto& connect : Connects)
        {
            auto link = SpawnLinkFromJSON(connect);
        }
    }

    void ShowStyleEditor(bool* show = nullptr)
    {
        if (!ImGui::Begin("Style", show))
        {
            ImGui::End();
            return;
        }

        auto paneWidth = ImGui::GetContentRegionAvail().x;

        auto& editorStyle = ed::GetStyle();
        ImGui::BeginHorizontal("Style buttons", ImVec2(paneWidth, 0), 1.0f);
        ImGui::TextUnformatted("Values");
        ImGui::Spring();
        if (ImGui::Button("Reset to defaults"))
            editorStyle = ed::Style();
        ImGui::EndHorizontal();
        ImGui::Spacing();
        ImGui::DragFloat4("Node Padding", &editorStyle.NodePadding.x, 0.1f, 0.0f, 40.0f);
        ImGui::DragFloat("Node Rounding", &editorStyle.NodeRounding, 0.1f, 0.0f, 40.0f);
        ImGui::DragFloat("Node Border Width", &editorStyle.NodeBorderWidth, 0.1f, 0.0f, 15.0f);
        ImGui::DragFloat("Hovered Node Border Width", &editorStyle.HoveredNodeBorderWidth, 0.1f, 0.0f, 15.0f);
        ImGui::DragFloat("Selected Node Border Width", &editorStyle.SelectedNodeBorderWidth, 0.1f, 0.0f, 15.0f);
        ImGui::DragFloat("Pin Rounding", &editorStyle.PinRounding, 0.1f, 0.0f, 40.0f);
        ImGui::DragFloat("Pin Border Width", &editorStyle.PinBorderWidth, 0.1f, 0.0f, 15.0f);
        ImGui::DragFloat("Link Strength", &editorStyle.LinkStrength, 1.0f, 0.0f, 500.0f);
        //ImVec2  SourceDirection;
        //ImVec2  TargetDirection;
        ImGui::DragFloat("Scroll Duration", &editorStyle.ScrollDuration, 0.001f, 0.0f, 2.0f);
        ImGui::DragFloat("Flow Marker Distance", &editorStyle.FlowMarkerDistance, 1.0f, 1.0f, 200.0f);
        ImGui::DragFloat("Flow Speed", &editorStyle.FlowSpeed, 1.0f, 1.0f, 2000.0f);
        ImGui::DragFloat("Flow Duration", &editorStyle.FlowDuration, 0.001f, 0.0f, 5.0f);
        //ImVec2  PivotAlignment;
        //ImVec2  PivotSize;
        //ImVec2  PivotScale;
        //float   PinCorners;
        //float   PinRadius;
        //float   PinArrowSize;
        //float   PinArrowWidth;
        ImGui::DragFloat("Group Rounding", &editorStyle.GroupRounding, 0.1f, 0.0f, 40.0f);
        ImGui::DragFloat("Group Border Width", &editorStyle.GroupBorderWidth, 0.1f, 0.0f, 15.0f);

        ImGui::Separator();

        static ImGuiColorEditFlags edit_mode = ImGuiColorEditFlags_DisplayRGB;
        ImGui::BeginHorizontal("Color Mode", ImVec2(paneWidth, 0), 1.0f);
        ImGui::TextUnformatted("Filter Colors");
        ImGui::Spring();
        ImGui::RadioButton("RGB", &edit_mode, ImGuiColorEditFlags_DisplayRGB);
        ImGui::Spring(0);
        ImGui::RadioButton("HSV", &edit_mode, ImGuiColorEditFlags_DisplayHSV);
        ImGui::Spring(0);
        ImGui::RadioButton("HEX", &edit_mode, ImGuiColorEditFlags_DisplayHex);
        ImGui::EndHorizontal();

        static ImGuiTextFilter filter;
        filter.Draw("", paneWidth);

        ImGui::Spacing();

        ImGui::PushItemWidth(-160);
        for (int i = 0; i < static_cast<int>(ed::StyleColor_Count); ++i)
        {
            auto name = ed::GetStyleColorName((ed::StyleColor)i);
            if (!filter.PassFilter(name))
                continue;

            ImGui::ColorEdit4(name, &editorStyle.Colors[i].x, edit_mode);
        }
        ImGui::PopItemWidth();

        ImGui::End();
    }

    void ShowLeftPane(float paneWidth)
    {
        auto& io = ImGui::GetIO();

        if (ImGui::Button(/*ICON_IGFD_FOLDER_OPEN*/ " Save Project As..."))
        {
            const char* filters = ".con";

            ImGuiFileDialogFlags flags = 0;
            flags |= ImGuiFileDialogFlags_::ImGuiFileDialogFlags_ConfirmOverwrite;
            flags |= ImGuiFileDialogFlags_::ImGuiFileDialogFlags_DontShowHiddenFiles;

            if (/*standardDialogMode*/true)
                ImGuiFileDialog::Instance()->OpenDialog("SaveProjectAs", /*ICON_IGFD_FOLDER_OPEN*/ " Save Project As", filters, ProjectsPath.string(), CurrentProjectName, 1, nullptr, flags);
            else
                ImGuiFileDialog::Instance()->OpenModal("SaveProjectAs", /*ICON_IGFD_FOLDER_OPEN*/ " Save Project As", filters, ProjectsPath.string(), CurrentProjectName, 1, nullptr, flags);
        }

        // display
        if (ImGuiFileDialog::Instance()->Display("SaveProjectAs"))
        {
            // action if pressed OK
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                std::string fileName = fs::path(filePathName).filename().string();
                // action

                CurrentProjectName = fileName;

                std::cout << "Saving Project As:" << std::endl;
                std::cout << "filePathName: " << filePathName << std::endl;
                std::cout << "filePath: " << filePath << std::endl;
                SaveProjectToFile(fs::path(filePathName));
            }

            // close
            ImGuiFileDialog::Instance()->Close();
        }

        ImGui::SameLine();

        if (ImGui::Button(/*ICON_IGFD_FOLDER_OPEN*/ " Open Project..."))
        {
            //const char* filters = ".*,.con,.dev,.json";
            const char* filters = ".con";

            ImGuiFileDialogFlags flags = 0;
            flags |= ImGuiFileDialogFlags_::ImGuiFileDialogFlags_DontShowHiddenFiles;

            if (/*standardDialogMode*/true)
                ImGuiFileDialog::Instance()->OpenDialog("OpenProject", /*ICON_IGFD_FOLDER_OPEN*/ " Open Project", filters, ProjectsPath.string(), CurrentProjectName, 1, nullptr, flags);
            else
                ImGuiFileDialog::Instance()->OpenModal("OpenProject", /*ICON_IGFD_FOLDER_OPEN*/ " Open Project", filters, ProjectsPath.string(), CurrentProjectName, 1, nullptr, flags);
        }

        // display
        if (ImGuiFileDialog::Instance()->Display("OpenProject"))
        {
            // action if pressed OK
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                std::string fileName = fs::path(filePathName).filename().string();
                // action

                CurrentProjectName = fileName;

                std::cout << "Opening Project:" << std::endl;
                std::cout << "filePathName: " << filePathName << std::endl;
                std::cout << "filePath: " << filePath << std::endl;
                LoadProjectFromFile(fs::path(filePathName));
            }

            // close
            ImGuiFileDialog::Instance()->Close();
        }

        //ImGui::Separator();

        ImGui::BeginChild("Selection", ImVec2(paneWidth, 0));

        paneWidth = ImGui::GetContentRegionAvail().x;

        static bool showStyleEditor = false;
        ImGui::BeginHorizontal("Style Editor", ImVec2(paneWidth, 0));
        ImGui::Spring(0.0f, 0.0f);
        if (ImGui::Button("Zoom to Content"))
            ed::NavigateToContent();
        ImGui::Spring(0.0f);
        if (ImGui::Button("Show Flow"))
        {
            for (auto& link : m_Links)
                ed::Flow(link.ID);
        }
        ImGui::Spring();
        if (ImGui::Button("Edit Style"))
            showStyleEditor = true;
        ImGui::EndHorizontal();
        ImGui::Checkbox("Show Ordinals", &m_ShowOrdinals);

        if (showStyleEditor)
            ShowStyleEditor(&showStyleEditor);

        std::vector<ed::NodeId> selectedNodes;
        std::vector<ed::LinkId> selectedLinks;
        selectedNodes.resize(ed::GetSelectedObjectCount());
        selectedLinks.resize(ed::GetSelectedObjectCount());

        int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
        int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

        selectedNodes.resize(nodeCount);
        selectedLinks.resize(linkCount);

        int saveIconWidth     = GetTextureWidth(m_SaveIcon);
        int saveIconHeight    = GetTextureWidth(m_SaveIcon);
        int restoreIconWidth  = GetTextureWidth(m_RestoreIcon);
        int restoreIconHeight = GetTextureWidth(m_RestoreIcon);

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetCursorScreenPos(),
            ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
            ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
        ImGui::Spacing(); ImGui::SameLine();
        ImGui::TextUnformatted("Nodes");
        ImGui::Indent();
        for (auto& node : m_Nodes)
        {
            ImGui::PushID(node->ID.AsPointer());
            auto start = ImGui::GetCursorScreenPos();

            if (const auto progress = GetTouchProgress(node->ID))
            {
                ImGui::GetWindowDrawList()->AddLine(
                    start + ImVec2(-8, 0),
                    start + ImVec2(-8, ImGui::GetTextLineHeight()),
                    IM_COL32(255, 0, 0, 255 - (int)(255 * progress)), 4.0f);
            }

            bool isSelected = std::find(selectedNodes.begin(), selectedNodes.end(), node->ID) != selectedNodes.end();
            if (ImGui::Selectable((node->Name + "##" + std::to_string(reinterpret_cast<uintptr_t>(node->ID.AsPointer()))).c_str(), &isSelected))
            {
                if (io.KeyCtrl)
                {
                    if (isSelected)
                        ed::SelectNode(node->ID, true);
                    else
                        ed::DeselectNode(node->ID);
                }
                else
                    ed::SelectNode(node->ID, false);

                ed::NavigateToSelection();
            }
            if (ImGui::IsItemHovered() && !node->State.empty())
                ImGui::SetTooltip("State: %s", node->State.c_str());

            auto id = std::string("(") + std::to_string(reinterpret_cast<uintptr_t>(node->ID.AsPointer())) + ")";
            auto textSize = ImGui::CalcTextSize(id.c_str(), nullptr);
            auto iconPanelPos = start + ImVec2(
                paneWidth - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().IndentSpacing - saveIconWidth - restoreIconWidth - ImGui::GetStyle().ItemInnerSpacing.x * 1,
                (ImGui::GetTextLineHeight() - saveIconHeight) / 2);
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(iconPanelPos.x - textSize.x - ImGui::GetStyle().ItemInnerSpacing.x, start.y),
                IM_COL32(255, 255, 255, 255), id.c_str(), nullptr);

            auto drawList = ImGui::GetWindowDrawList();
            ImGui::SetCursorScreenPos(iconPanelPos);
            ImGui::SetItemAllowOverlap();
            if (node->SavedState.empty())
            {
                if (ImGui::InvisibleButton("save", ImVec2((float)saveIconWidth, (float)saveIconHeight)))
                    node->SavedState = node->State;

                if (ImGui::IsItemActive())
                    drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
                else if (ImGui::IsItemHovered())
                    drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
                else
                    drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
            }
            else
            {
                ImGui::Dummy(ImVec2((float)saveIconWidth, (float)saveIconHeight));
                drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
            }

            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::SetItemAllowOverlap();
            if (!node->SavedState.empty())
            {
                if (ImGui::InvisibleButton("restore", ImVec2((float)restoreIconWidth, (float)restoreIconHeight)))
                {
                    node->State = node->SavedState;
                    ed::RestoreNodeState(node->ID);
                    node->SavedState.clear();
                }

                if (ImGui::IsItemActive())
                    drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
                else if (ImGui::IsItemHovered())
                    drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
                else
                    drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
            }
            else
            {
                ImGui::Dummy(ImVec2((float)restoreIconWidth, (float)restoreIconHeight));
                drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
            }

            ImGui::SameLine(0, 0);
            ImGui::SetItemAllowOverlap();
            ImGui::Dummy(ImVec2(0, (float)restoreIconHeight));

            ImGui::PopID();
        }
        ImGui::Unindent();

        static int changeCount = 0;

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetCursorScreenPos(),
            ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
            ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
        ImGui::Spacing(); ImGui::SameLine();
        ImGui::TextUnformatted("Selection");

        ImGui::BeginHorizontal("Selection Stats", ImVec2(paneWidth, 0));
        ImGui::Text("Changed %d time%s", changeCount, changeCount > 1 ? "s" : "");
        ImGui::Spring();
        if (ImGui::Button("Deselect All"))
            ed::ClearSelection();
        ImGui::EndHorizontal();
        ImGui::Indent();
        for (int i = 0; i < nodeCount; ++i) ImGui::Text("Node (%p)", selectedNodes[i].AsPointer());
        for (int i = 0; i < linkCount; ++i) ImGui::Text("Link (%p)", selectedLinks[i].AsPointer());
        ImGui::Unindent();

        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
            for (auto& link : m_Links)
                ed::Flow(link.ID);

        if (ed::HasSelectionChanged())
            ++changeCount;

        ImGui::EndChild();
    }

    void OnFrame(float deltaTime) override
    {
        UpdateTouch();

        auto& io = ImGui::GetIO();

        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

        ed::SetCurrentEditor(m_Editor);

        SetTitle(string("Connectatron: " + CurrentProjectName).c_str());

        //auto& style = ImGui::GetStyle();

    # if 0
        {
            for (auto x = -io.DisplaySize.y; x < io.DisplaySize.x; x += 10.0f)
            {
                ImGui::GetWindowDrawList()->AddLine(ImVec2(x, 0), ImVec2(x + io.DisplaySize.y, io.DisplaySize.y),
                    IM_COL32(255, 255, 0, 255));
            }
        }
    # endif

        static ed::NodeId contextNodeId      = 0;
        static ed::LinkId contextLinkId      = 0;
        static ed::PinId  contextPinId       = 0;
        static bool createNewNode  = false;
        static Pin* newNodeLinkPin = nullptr;
        static Pin* newLinkPin     = nullptr;

        static float leftPaneWidth  = 400.0f;
        static float rightPaneWidth = 800.0f;
        Splitter(true, 4.0f, &leftPaneWidth, &rightPaneWidth, 50.0f, 50.0f);

        ShowLeftPane(leftPaneWidth - 4.0f);

        ImGui::SameLine(0.0f, 12.0f);

        ed::Begin("Node editor");
        {
            auto cursorTopLeft = ImGui::GetCursorScreenPos();

            util::BlueprintNodeBuilder builder(m_HeaderBackground, GetTextureWidth(m_HeaderBackground), GetTextureHeight(m_HeaderBackground));

            // Draw all Blueprint and Simple nodes
            for (auto& node : m_Nodes)
            {
                if (node->Type != NodeType::Blueprint && node->Type != NodeType::Simple)
                    continue;

                const auto isSimple = node->Type == NodeType::Simple;

            	/*bool hasOutputDelegates = false;
            	for (auto& male : node->Males)
            	    if (male.Type == PinType::Delegate)
             	       hasOutputDelegates = true;*/

                builder.Begin(node->ID);
                if (!isSimple)
                {
                    builder.Header(node->Color);
                        ImGui::Spring(0);
                        ImGui::TextUnformatted(node->Name.c_str());
                        ImGui::Spring(1);
                        ImGui::Dummy(ImVec2(0, 28));
                        //if (hasOutputDelegates)
                        //{
                        //    ImGui::BeginVertical("delegates", ImVec2(0, 28));
                        //    ImGui::Spring(1, 0);
                        //    for (auto& output : node->Outputs)
                        //    {
                        //        if (output.Type != PinType::Delegate)
                        //            continue;

                        //        auto alpha = ImGui::GetStyle().Alpha;
                        //        if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
                        //            alpha = alpha * (48.0f / 255.0f);

                        //        ed::BeginPin(output.ID, ed::PinKind::Output);
                        //        ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                        //        ed::PinPivotSize(ImVec2(0, 0));
                        //        ImGui::BeginHorizontal(output.ID.AsPointer());
                        //        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                        //        if (!output.Name.empty())
                        //        {
                        //            ImGui::TextUnformatted(output.Name.c_str());
                        //            ImGui::Spring(0);
                        //        }
                        //        DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
                        //        ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                        //        ImGui::EndHorizontal();
                        //        ImGui::PopStyleVar();
                        //        ed::EndPin();

                        //        //DrawItemRect(ImColor(255, 0, 0));
                        //    }
                        //    ImGui::Spring(1, 0);
                        //    ImGui::EndVertical();
                        //    ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                        //}
                        //else
                            ImGui::Spring(0);
                    builder.EndHeader();
                }

                // Draw female pin icons
                for (auto& female : node->Females)
                {
                    auto alpha = ImGui::GetStyle().Alpha;
                    if (newLinkPin && !CanCreateLink(newLinkPin, &female) && &female != newLinkPin)
                        alpha = alpha * (48.0f / 255.0f);

                    builder.Input(female.ID);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                    DrawPinIcon(female, IsPinLinked(female.ID), (int)(alpha * 255));
                    ImGui::Spring(0);
                    if (!female.Name.empty())
                    {
                        ImGui::TextUnformatted(female.Name.c_str());
                        ImGui::Spring(0);
                    }
                    /*if (female.Type == PinType::Bool)
                    {
                        ImGui::Button("Hello");
                        ImGui::Spring(0);
                    }*/
                    ImGui::PopStyleVar();
                    builder.EndInput();
                }

                if (isSimple)
                {
                    builder.Middle();

                    ImGui::Spring(1, 0);
                    ImGui::TextUnformatted(node->Name.c_str());
                    ImGui::Spring(1, 0);
                }

                // Draw male pin icons
                for (auto& male : node->Males)
                {
                    /*if (!isSimple && male.Type == PinType::Delegate)
                        continue;*/

                    auto alpha = ImGui::GetStyle().Alpha;
                    if (newLinkPin && !CanCreateLink(newLinkPin, &male) && &male != newLinkPin)
                        alpha = alpha * (48.0f / 255.0f);

                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                    builder.Output(male.ID);
                    /*if (male.Type == PinType::String)
                    {
                        static char buffer[128] = "Edit Me\nMultiline!";
                        static bool wasActive = false;

                        ImGui::PushItemWidth(100.0f);
                        ImGui::InputText("##edit", buffer, 127);
                        ImGui::PopItemWidth();
                        if (ImGui::IsItemActive() && !wasActive)
                        {
                            ed::EnableShortcuts(false);
                            wasActive = true;
                        }
                        else if (!ImGui::IsItemActive() && wasActive)
                        {
                            ed::EnableShortcuts(true);
                            wasActive = false;
                        }
                        ImGui::Spring(0);
                    }*/
                    if (!male.Name.empty())
                    {
                        ImGui::Spring(0);
                        ImGui::TextUnformatted(male.Name.c_str());
                    }
                    ImGui::Spring(0);
                    DrawPinIcon(male, IsPinLinked(male.ID), (int)(alpha * 255));
                    ImGui::PopStyleVar();
                    builder.EndOutput();
                }

                builder.End();
            }

            // Draw all tree nodes
            for (auto& node : m_Nodes)
            {
                if (node->Type != NodeType::Tree)
                    continue;

                const float rounding = 5.0f;
                const float padding  = 12.0f;

                const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

                ed::PushStyleColor(ed::StyleColor_NodeBg,        ImColor(128, 128, 128, 200));
                ed::PushStyleColor(ed::StyleColor_NodeBorder,    ImColor( 32,  32,  32, 200));
                ed::PushStyleColor(ed::StyleColor_PinRect,       ImColor( 60, 180, 255, 150));
                ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor( 60, 180, 255, 150));

                ed::PushStyleVar(ed::StyleVar_NodePadding,  ImVec4(0, 0, 0, 0));
                ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
                ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f,  1.0f));
                ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
                ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
                ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
                ed::PushStyleVar(ed::StyleVar_PinRadius, 5.0f);
                ed::BeginNode(node->ID);

                ImGui::BeginVertical(node->ID.AsPointer());
                ImGui::BeginHorizontal("inputs");
                ImGui::Spring(0, padding * 2);

                ImRect inputsRect;
                int inputAlpha = 200;
                if (!node->Females.empty())
                {
                    auto& pin = node->Females[0];
                    ImGui::Dummy(ImVec2(0, padding));
                    ImGui::Spring(1, 0);
                    inputsRect = ImGui_GetItemRect();

                    ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
                    ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
#if IMGUI_VERSION_NUM > 18101
                    ed::PushStyleVar(ed::StyleVar_PinCorners, ImDrawFlags_RoundCornersBottom);
#else
                    ed::PushStyleVar(ed::StyleVar_PinCorners, 12);
#endif
                    ed::BeginPin(pin.ID, ed::PinKind::Input);
                    ed::PinPivotRect(inputsRect.GetTL(), inputsRect.GetBR());
                    ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
                    ed::EndPin();
                    ed::PopStyleVar(3);

                    if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                        inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                }
                else
                    ImGui::Dummy(ImVec2(0, padding));

                ImGui::Spring(0, padding * 2);
                ImGui::EndHorizontal();

                ImGui::BeginHorizontal("content_frame");
                ImGui::Spring(1, padding);

                ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
                ImGui::Dummy(ImVec2(160, 0));
                ImGui::Spring(1);
                ImGui::TextUnformatted(node->Name.c_str());
                ImGui::Spring(1);
                ImGui::EndVertical();
                auto contentRect = ImGui_GetItemRect();

                ImGui::Spring(1, padding);
                ImGui::EndHorizontal();

                ImGui::BeginHorizontal("outputs");
                ImGui::Spring(0, padding * 2);

                ImRect outputsRect;
                int outputAlpha = 200;
                if (!node->Males.empty())
                {
                    auto& pin = node->Males[0];
                    ImGui::Dummy(ImVec2(0, padding));
                    ImGui::Spring(1, 0);
                    outputsRect = ImGui_GetItemRect();

#if IMGUI_VERSION_NUM > 18101
                    ed::PushStyleVar(ed::StyleVar_PinCorners, ImDrawFlags_RoundCornersTop);
#else
                    ed::PushStyleVar(ed::StyleVar_PinCorners, 3);
#endif
                    ed::BeginPin(pin.ID, ed::PinKind::Output);
                    ed::PinPivotRect(outputsRect.GetTL(), outputsRect.GetBR());
                    ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
                    ed::EndPin();
                    ed::PopStyleVar();

                    if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                        outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                }
                else
                    ImGui::Dummy(ImVec2(0, padding));

                ImGui::Spring(0, padding * 2);
                ImGui::EndHorizontal();

                ImGui::EndVertical();

                ed::EndNode();
                ed::PopStyleVar(7);
                ed::PopStyleColor(4);

                auto drawList = ed::GetNodeBackgroundDrawList(node->ID);

                //const auto fringeScale = ImGui::GetStyle().AntiAliasFringeScale;
                //const auto unitSize    = 1.0f / fringeScale;

                //const auto ImDrawList_AddRect = [](ImDrawList* drawList, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners, float thickness)
                //{
                //    if ((col >> 24) == 0)
                //        return;
                //    drawList->PathRect(a, b, rounding, rounding_corners);
                //    drawList->PathStroke(col, true, thickness);
                //};

#if IMGUI_VERSION_NUM > 18101
                const auto    topRoundCornersFlags = ImDrawFlags_RoundCornersTop;
                const auto bottomRoundCornersFlags = ImDrawFlags_RoundCornersBottom;
#else
                const auto    topRoundCornersFlags = 1 | 2;
                const auto bottomRoundCornersFlags = 4 | 8;
#endif

                drawList->AddRectFilled(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
                    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, bottomRoundCornersFlags);
                //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
                drawList->AddRect(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
                    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, bottomRoundCornersFlags);
                //ImGui::PopStyleVar();
                drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
                    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, topRoundCornersFlags);
                //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
                drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
                    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, topRoundCornersFlags);
                //ImGui::PopStyleVar();
                drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
                //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
                drawList->AddRect(
                    contentRect.GetTL(),
                    contentRect.GetBR(),
                    IM_COL32(48, 128, 255, 100), 0.0f);
                //ImGui::PopStyleVar();
            }

            // Draw all houdini nodes
            for (auto& node : m_Nodes)
            {
                if (node->Type != NodeType::Houdini)
                    continue;

                const float rounding = 10.0f;
                const float padding  = 12.0f;


                ed::PushStyleColor(ed::StyleColor_NodeBg,        ImColor(229, 229, 229, 200));
                ed::PushStyleColor(ed::StyleColor_NodeBorder,    ImColor(125, 125, 125, 200));
                ed::PushStyleColor(ed::StyleColor_PinRect,       ImColor(229, 229, 229, 60));
                ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor(125, 125, 125, 60));

                const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

                ed::PushStyleVar(ed::StyleVar_NodePadding,  ImVec4(0, 0, 0, 0));
                ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
                ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f,  1.0f));
                ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
                ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
                ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
                ed::PushStyleVar(ed::StyleVar_PinRadius, 6.0f);
                ed::BeginNode(node->ID);

                ImGui::BeginVertical(node->ID.AsPointer());
                if (!node->Females.empty())
                {
                    ImGui::BeginHorizontal("inputs");
                    ImGui::Spring(1, 0);

                    ImRect inputsRect;
                    int inputAlpha = 200;
                    for (auto& pin : node->Females)
                    {
                        ImGui::Dummy(ImVec2(padding, padding));
                        inputsRect = ImGui_GetItemRect();
                        ImGui::Spring(1, 0);
                        inputsRect.Min.y -= padding;
                        inputsRect.Max.y -= padding;

#if IMGUI_VERSION_NUM > 18101
                        const auto allRoundCornersFlags = ImDrawFlags_RoundCornersAll;
#else
                        const auto allRoundCornersFlags = 15;
#endif
                        //ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
                        //ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
                        ed::PushStyleVar(ed::StyleVar_PinCorners, allRoundCornersFlags);

                        ed::BeginPin(pin.ID, ed::PinKind::Input);
                        ed::PinPivotRect(inputsRect.GetCenter(), inputsRect.GetCenter());
                        ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
                        ed::EndPin();
                        //ed::PopStyleVar(3);
                        ed::PopStyleVar(1);

                        auto drawList = ImGui::GetWindowDrawList();
                        drawList->AddRectFilled(inputsRect.GetTL(), inputsRect.GetBR(),
                            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, allRoundCornersFlags);
                        drawList->AddRect(inputsRect.GetTL(), inputsRect.GetBR(),
                            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, allRoundCornersFlags);

                        if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                            inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                    }

                    //ImGui::Spring(1, 0);
                    ImGui::EndHorizontal();
                }

                ImGui::BeginHorizontal("content_frame");
                ImGui::Spring(1, padding);

                ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
                ImGui::Dummy(ImVec2(160, 0));
                ImGui::Spring(1);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::TextUnformatted(node->Name.c_str());
                ImGui::PopStyleColor();
                ImGui::Spring(1);
                ImGui::EndVertical();
                auto contentRect = ImGui_GetItemRect();

                ImGui::Spring(1, padding);
                ImGui::EndHorizontal();

                if (!node->Males.empty())
                {
                    ImGui::BeginHorizontal("outputs");
                    ImGui::Spring(1, 0);

                    ImRect outputsRect;
                    int outputAlpha = 200;
                    for (auto& pin : node->Males)
                    {
                        ImGui::Dummy(ImVec2(padding, padding));
                        outputsRect = ImGui_GetItemRect();
                        ImGui::Spring(1, 0);
                        outputsRect.Min.y += padding;
                        outputsRect.Max.y += padding;

#if IMGUI_VERSION_NUM > 18101
                        const auto allRoundCornersFlags = ImDrawFlags_RoundCornersAll;
                        const auto topRoundCornersFlags = ImDrawFlags_RoundCornersTop;
#else
                        const auto allRoundCornersFlags = 15;
                        const auto topRoundCornersFlags = 3;
#endif

                        ed::PushStyleVar(ed::StyleVar_PinCorners, topRoundCornersFlags);
                        ed::BeginPin(pin.ID, ed::PinKind::Output);
                        ed::PinPivotRect(outputsRect.GetCenter(), outputsRect.GetCenter());
                        ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
                        ed::EndPin();
                        ed::PopStyleVar();


                        auto drawList = ImGui::GetWindowDrawList();
                        drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR(),
                            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, allRoundCornersFlags);
                        drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR(),
                            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, allRoundCornersFlags);


                        if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
                            outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
                    }

                    ImGui::EndHorizontal();
                }

                ImGui::EndVertical();

                ed::EndNode();
                ed::PopStyleVar(7);
                ed::PopStyleColor(4);

                // auto drawList = ed::GetNodeBackgroundDrawList(node->ID);

                //const auto fringeScale = ImGui::GetStyle().AntiAliasFringeScale;
                //const auto unitSize    = 1.0f / fringeScale;

                //const auto ImDrawList_AddRect = [](ImDrawList* drawList, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners, float thickness)
                //{
                //    if ((col >> 24) == 0)
                //        return;
                //    drawList->PathRect(a, b, rounding, rounding_corners);
                //    drawList->PathStroke(col, true, thickness);
                //};

                //drawList->AddRectFilled(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
                //    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
                //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
                //drawList->AddRect(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
                //    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
                //ImGui::PopStyleVar();
                //drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
                //    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
                ////ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
                //drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
                //    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
                ////ImGui::PopStyleVar();
                //drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
                //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
                //drawList->AddRect(
                //    contentRect.GetTL(),
                //    contentRect.GetBR(),
                //    IM_COL32(48, 128, 255, 100), 0.0f);
                //ImGui::PopStyleVar();
            }

            // Draw all comment nodes
            for (auto& node : m_Nodes)
            {
                if (node->Type != NodeType::Comment)
                    continue;

                const float commentAlpha = 0.75f;

                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha);
                ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(255, 255, 255, 64));
                ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 255, 255, 64));
                ed::BeginNode(node->ID);
                ImGui::PushID(node->ID.AsPointer());
                ImGui::BeginVertical("content");
                ImGui::BeginHorizontal("horizontal");
                ImGui::Spring(1);
                ImGui::TextUnformatted(node->Name.c_str());
                ImGui::Spring(1);
                ImGui::EndHorizontal();
                ed::Group(node->Size);
                ImGui::EndVertical();
                ImGui::PopID();
                ed::EndNode();
                ed::PopStyleColor(2);
                ImGui::PopStyleVar();

                if (ed::BeginGroupHint(node->ID))
                {
                    //auto alpha   = static_cast<int>(commentAlpha * ImGui::GetStyle().Alpha * 255);
                    auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);

                    //ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha * ImGui::GetStyle().Alpha);

                    auto min = ed::GetGroupMin();
                    //auto max = ed::GetGroupMax();

                    ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
                    ImGui::BeginGroup();
                    ImGui::TextUnformatted(node->Name.c_str());
                    ImGui::EndGroup();

                    auto drawList = ed::GetHintBackgroundDrawList();

                    auto hintBounds      = ImGui_GetItemRect();
                    auto hintFrameBounds = ImRect_Expanded(hintBounds, 8, 4);

                    drawList->AddRectFilled(
                        hintFrameBounds.GetTL(),
                        hintFrameBounds.GetBR(),
                        IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);

                    drawList->AddRect(
                        hintFrameBounds.GetTL(),
                        hintFrameBounds.GetBR(),
                        IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 4.0f);

                    //ImGui::PopStyleVar();
                }
                ed::EndGroupHint();
            }

            // Draw all links
            for (auto& link : m_Links)
                ed::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

            // Single-entrance for single Create New Node popup
            if (!createNewNode)
            {
                if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
                {
                    auto showLabel = [](const char* label, ImColor color)
                    {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
                        auto size = ImGui::CalcTextSize(label);

                        auto padding = ImGui::GetStyle().FramePadding;
                        auto spacing = ImGui::GetStyle().ItemSpacing;

                        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

                        auto rectMin = ImGui::GetCursorScreenPos() - padding;
                        auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

                        auto drawList = ImGui::GetWindowDrawList();
                        drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
                        ImGui::TextUnformatted(label);
                    };

                    // Handle new-link, displaying feedback on if the link would work
                    ed::PinId startPinId = 0, endPinId = 0;
                    if (ed::QueryNewLink(&startPinId, &endPinId))
                    {
                        auto startPin = FindPin(startPinId);
                        auto endPin   = FindPin(endPinId);

                        newLinkPin = startPin ? startPin : endPin;

                        if (startPin->Kind == PinKind::Input)
                        {
                            std::swap(startPin, endPin);
                            std::swap(startPinId, endPinId);
                        }

                        if (startPin && endPin)
                        {
                            if (endPin == startPin)
                            {
                                ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                            }
                            else if (endPin->Kind == startPin->Kind)
                            {
                                showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
                                ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                            }
                            //else if (endPin->Node == startPin->Node)
                            //{
                            //    showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
                            //    ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
                            //}
                            else if (endPin->Type != startPin->Type)
                            {
                                showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
                                ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                            }
                            else
                            {
                                showLabel("+ Create Link", ImColor(32, 45, 32, 180));
                                if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
                                {
                                    m_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));
                                    m_Links.back().Color = GetIconColor(startPin->Type);
                                }
                            }
                        }
                    }

                    // Handle new-node
                    ed::PinId pinId = 0;
                    if (ed::QueryNewNode(&pinId))
                    {
                        newLinkPin = FindPin(pinId);
                        if (newLinkPin)
                            showLabel("+ Create Node", ImColor(32, 45, 32, 180));

                        if (ed::AcceptNewItem())
                        {
                            createNewNode  = true;
                            newNodeLinkPin = FindPin(pinId);
                            newLinkPin = nullptr;
                            ed::Suspend();
                            ImGui::OpenPopup("Create New Node");
                            ed::Resume();
                        }
                    }
                }
                else
                    newLinkPin = nullptr;

                ed::EndCreate();

                // Handle deletions (including bulk-deletions) of nodes and links
                if (ed::BeginDelete())
                {
                    ed::LinkId linkId = 0;
                    while (ed::QueryDeletedLink(&linkId))
                    {
                        if (ed::AcceptDeletedItem())
                        {
                            auto id = std::find_if(m_Links.begin(), m_Links.end(), [linkId](auto& link) { return link.ID == linkId; });
                            if (id != m_Links.end())
                                m_Links.erase(id);
                        }
                    }

                    ed::NodeId nodeId = 0;
                    while (ed::QueryDeletedNode(&nodeId))
                    {
                        if (ed::AcceptDeletedItem())
                        {
                            auto id = std::find_if(m_Nodes.begin(), m_Nodes.end(), [nodeId](auto& node) { return node->ID == nodeId; });
                            if (id != m_Nodes.end())
                            {
                                auto found_node = *id;

                                // Wipe Links that have pins on the node being deleted
                                std::set<ed::PinId> PinsOnDeletingNode;
                                for (auto& female : found_node->Females)
                                {
                                    PinsOnDeletingNode.insert(female.ID);
                                }
                                for (auto& male : found_node->Males)
                                {
                                    PinsOnDeletingNode.insert(male.ID);
                                }

                                //std::vector<ed::LinkId> linksToDelete;
                                std::vector<size_t> linksToDelete;
                                size_t idx = 0;
                                for (auto& link : m_Links)
                                {
                                    if (PinsOnDeletingNode.find(link.StartPinID) != PinsOnDeletingNode.end())
                                        //linksToDelete.push_back(link.ID);
                                        linksToDelete.push_back(idx);
                                    if (PinsOnDeletingNode.find(link.EndPinID) != PinsOnDeletingNode.end())
                                        //linksToDelete.push_back(link.ID);
                                        linksToDelete.push_back(idx);
                                    idx++;
                                }

                                // Sort linksToDelete in reverse order
                                std::sort(linksToDelete.rbegin(),linksToDelete.rend());

                                // Erase at indices
                                for (auto& linkIdx : linksToDelete)
                                {
                                    //ed::DeleteLink(linkId);
                                    m_Links.erase(m_Links.begin() + linkIdx);
                                    //m_Links.erase(linkId);
                                }
                                
                                m_Nodes.erase(id);
                            }
                        }
                    }
                }
                ed::EndDelete();
            }

            ImGui::SetCursorScreenPos(cursorTopLeft);
        }

    # if 1
        auto openPopupPosition = ImGui::GetMousePos();
        ed::Suspend();
        if (ed::ShowNodeContextMenu(&contextNodeId))
            ImGui::OpenPopup("Node Context Menu");
        else if (ed::ShowPinContextMenu(&contextPinId))
            ImGui::OpenPopup("Pin Context Menu");
        else if (ed::ShowLinkContextMenu(&contextLinkId))
            ImGui::OpenPopup("Link Context Menu");
        else if (ed::ShowBackgroundContextMenu())
        {
            ImGui::OpenPopup("Create New Node");
            newNodeLinkPin = nullptr;
        }
        ed::Resume();

        ed::Suspend();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        if (ImGui::BeginPopup("Node Context Menu"))
        {
            auto node = FindNode(contextNodeId);

            ImGui::TextUnformatted("Node Context Menu");
            ImGui::Separator();
            if (node)
            {
                ImGui::Text("Type: %s", node->Type == NodeType::Blueprint ? "Blueprint" : (node->Type == NodeType::Tree ? "Tree" : "Comment"));
                ImGui::Text("Females: %d", (int)node->Females.size());
                ImGui::Text("Males: %d", (int)node->Males.size());
#ifdef _DEBUG
                ImGui::Separator();
                ImGui::Text("ID: %p", node->ID.AsPointer());
#endif
            }
            else
                ImGui::Text("Unknown node: %p", contextNodeId.AsPointer());
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
                ed::DeleteNode(contextNodeId);
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Pin Context Menu"))
        {
            auto pin = FindPin(contextPinId);

            ImGui::TextUnformatted("Connector Information");
            ImGui::Separator();
            if (pin)
            {
                auto pin_enum_name = std::string(magic_enum::enum_name(pin->Type));
                EnumName_Underscore2Symbol(pin_enum_name);
                auto pin_text = "Pin Type: " + pin_enum_name;
                ImGui::Text(pin_text.c_str());
                ImGui::Text(pin->IsFemale ? "Female" : "Male");

                ImGui::Separator();
                ImGui::Text("Supported Protocols:");
                ImGui::Indent();
                if (pin->Protocols.size() == 0)
                {
                    ImGui::Text("(None specified)");
                }
                else
                {
                    for (auto supportedProto : pin->Protocols)
                    {
                        std::string proto_name(magic_enum::enum_name(supportedProto));

                        EnumName_Underscore2Symbol(proto_name);

                        ImGui::Text(proto_name.c_str());
                    }
                }
                ImGui::Unindent();
                if (pin->ExtraInfo.length() != 0)
                {
                    ImGui::Separator();
                    ImGui::Text("Extra Info:");
                    ImGui::Separator();
                    ImGui::Text(pin->ExtraInfo.c_str());
                }
#ifdef _DEBUG
                ImGui::Separator();
                ImGui::Text("ID: %p", pin->ID.AsPointer());
                if (pin->Node)
                    ImGui::Text("Node: %p", pin->Node->ID.AsPointer());
                else
                    ImGui::Text("Node: %s", "<none>");
#endif
            }
            else
                ImGui::Text("Unknown pin: %p", contextPinId.AsPointer());

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Link Context Menu"))
        {
            auto link = FindLink(contextLinkId);

            ImGui::TextUnformatted("Link Context Menu");
            ImGui::Separator();
            if (link)
            {
                ImGui::Text("ID: %p", link->ID.AsPointer());
                ImGui::Text("From: %p", link->StartPinID.AsPointer());
                ImGui::Text("To: %p", link->EndPinID.AsPointer());
            }
            else
                ImGui::Text("Unknown link: %p", contextLinkId.AsPointer());
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
                ed::DeleteLink(contextLinkId);
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Create New Node"))
        {
            auto newNodePostion = openPopupPosition;
            //ImGui::SetCursorScreenPos(ImGui::GetMousePosOnOpeningCurrentPopup());

            //auto drawList = ImGui::GetWindowDrawList();
            //drawList->AddCircleFilled(ImGui::GetMousePosOnOpeningCurrentPopup(), 10.0f, 0xFFFF00FF);

            shared_ptr<Node> node;

            // If node was created by dragging off of another node's pin,
            // determine what PinType we need to limit the device selection to.
            bool limitByPinType = false;
            PinType valid_type;
            PinKind valid_kind;
            if (auto startPin = newNodeLinkPin)
            {
                limitByPinType = true;
                valid_type = startPin->Type;
                if (startPin->IsFemale)
                    valid_kind = PinKind::Output;
                else
                    valid_kind = PinKind::Input;
            }

            vector<fs::path> categories = { ".", "Cables (M-M)", "Adapters (M-F)" };
            // Iterate through files in Devices and show them as options
            //TODO use recursive directory iterator instead!
            for(auto category : categories)
            {
                string category_label = category.generic_string();
                auto search_dir = DevicesPath / category;
                if (category_label == ".")
                    category_label = "Devices";

                ImGui::Text(category_label.c_str());
                ImGui::Separator();
                ImGui::Indent();
                bool nothing_shown_yet = true;
                for (auto const& devicePath : fs::directory_iterator{ search_dir })
                {
                    if(devicePath.path().extension() == fs::path(".json"))
                    {
                        if (limitByPinType) // Limit selection by pin compatibility
                        {
                            bool this_node_is_valid = false;
                            shared_ptr<Node> temp_node = make_shared<Node>(-1, "TEMP NODE", ImColor(255, 128, 128));
                            InitNodeFromJSON(GetJSONFromFile(devicePath.path()), temp_node);
                            switch (valid_kind)
                            {
                            case PinKind::Input: // Will be connecting to a female pin on new node
                                for (const auto& pin : temp_node->Females)
                                {
                                    if (pin.Type == valid_type)
                                    {
                                        this_node_is_valid = true;
                                        break;
                                    }
                                }
                                break;
                            case PinKind::Output: // Will be connecting to a male pin on new node
                                for (const auto& pin : temp_node->Males)
                                {
                                    if (pin.Type == valid_type)
                                    {
                                        this_node_is_valid = true;
                                        break;
                                    }
                                }
                                break;
                            }

                            if (this_node_is_valid)
                            {
                                nothing_shown_yet = false;
                                if (ImGui::MenuItem(devicePath.path().stem().string().c_str()))
                                    node = SpawnNodeFromJSON(GetJSONFromFile(devicePath.path()));
                            }
                        }
                        else // Any node would be fine
                        {
                            nothing_shown_yet = false;
                            if (ImGui::MenuItem(devicePath.path().stem().string().c_str()))
                                node = SpawnNodeFromJSON(GetJSONFromFile(devicePath.path()));
                        }
                    }
                    //TODO maybe cache devices for performance? Would require invalidation to keep supporting editing devices at runtime
                }
                if(nothing_shown_yet)
                    ImGui::Text("(Nothing with a compatible connection)");
                ImGui::Unindent();

                ImGui::Separator();
                ImGui::Separator();
            }

            if (node)
            {
                BuildNodes();

                createNewNode = false;

                ed::SetNodePosition(node->ID, newNodePostion);

                // If node was created by dragging off of another node's pin,
                // create the link if possible.
                if (auto startPin = newNodeLinkPin)
                {
                    auto& pins = startPin->Kind == PinKind::Input ? node->Males : node->Females;

                    for (auto& pin : pins)
                    {
                        if (CanCreateLink(startPin, &pin))
                        {
                            auto endPin = &pin;
                            if (startPin->Kind == PinKind::Input)
                                std::swap(startPin, endPin);

                            m_Links.emplace_back(Link(GetNextId(), startPin->ID, endPin->ID));
                            m_Links.back().Color = GetIconColor(startPin->Type);

                            break;
                        }
                    }
                }
            }

            ImGui::EndPopup();
        }
        else
            createNewNode = false;
        ImGui::PopStyleVar();
        ed::Resume();
    # endif


    /*
        cubic_bezier_t c;
        c.p0 = pointf(100, 600);
        c.p1 = pointf(300, 1200);
        c.p2 = pointf(500, 100);
        c.p3 = pointf(900, 600);

        auto drawList = ImGui::GetWindowDrawList();
        auto offset_radius = 15.0f;
        auto acceptPoint = [drawList, offset_radius](const bezier_subdivide_result_t& r)
        {
            drawList->AddCircle(to_imvec(r.point), 4.0f, IM_COL32(255, 0, 255, 255));

            auto nt = r.tangent.normalized();
            nt = pointf(-nt.y, nt.x);

            drawList->AddLine(to_imvec(r.point), to_imvec(r.point + nt * offset_radius), IM_COL32(255, 0, 0, 255), 1.0f);
        };

        drawList->AddBezierCurve(to_imvec(c.p0), to_imvec(c.p1), to_imvec(c.p2), to_imvec(c.p3), IM_COL32(255, 255, 255, 255), 1.0f);
        cubic_bezier_subdivide(acceptPoint, c);
    */

        ed::End();

        auto editorMin = ImGui::GetItemRectMin();
        auto editorMax = ImGui::GetItemRectMax();

        if (m_ShowOrdinals)
        {
            int nodeCount = ed::GetNodeCount();
            std::vector<ed::NodeId> orderedNodeIds;
            orderedNodeIds.resize(static_cast<size_t>(nodeCount));
            ed::GetOrderedNodeIds(orderedNodeIds.data(), nodeCount);


            auto drawList = ImGui::GetWindowDrawList();
            drawList->PushClipRect(editorMin, editorMax);

            int ordinal = 0;
            for (auto& nodeId : orderedNodeIds)
            {
                auto p0 = ed::GetNodePosition(nodeId);
                auto p1 = p0 + ed::GetNodeSize(nodeId);
                p0 = ed::CanvasToScreen(p0);
                p1 = ed::CanvasToScreen(p1);


                ImGuiTextBuffer builder;
                builder.appendf("#%d", ordinal++);

                auto textSize   = ImGui::CalcTextSize(builder.c_str());
                auto padding    = ImVec2(2.0f, 2.0f);
                auto widgetSize = textSize + padding * 2;

                auto widgetPosition = ImVec2(p1.x, p0.y) + ImVec2(0.0f, -widgetSize.y);

                drawList->AddRectFilled(widgetPosition, widgetPosition + widgetSize, IM_COL32(100, 80, 80, 190), 3.0f, ImDrawFlags_RoundCornersAll);
                drawList->AddRect(widgetPosition, widgetPosition + widgetSize, IM_COL32(200, 160, 160, 190), 3.0f, ImDrawFlags_RoundCornersAll);
                drawList->AddText(widgetPosition + padding, IM_COL32(255, 255, 255, 255), builder.c_str());
            }

            drawList->PopClipRect();
        }


        //ImGui::ShowTestWindow();
        //ImGui::ShowMetricsWindow();
    }

    int                  m_NextId = 1;
    const int            m_PinIconSize = 24;
    std::vector<shared_ptr<Node>>    m_Nodes;
    std::map<NotUUID, shared_ptr<Node>>    m_IdNodes;
    std::vector<Link>    m_Links;
    ImTextureID          m_HeaderBackground = nullptr;
    ImTextureID          m_SaveIcon = nullptr;
    ImTextureID          m_RestoreIcon = nullptr;
    const float          m_TouchTime = 1.0f;
    std::map<ed::NodeId, float, NodeIdLess> m_NodeTouchTime;
    bool                 m_ShowOrdinals = false;
};

int Main(int argc, char** argv)
{
    Connectatron conn("Connectatron", argc, argv);

    if (conn.Create())
        return conn.Run();

    return 0;
}