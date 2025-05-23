#include <application.h>
#include "utilities/builders.h"
#include "utilities/widgets.h"

#include "Connectors.h"
#include "Protocols.h"

#include <imgui_node_editor.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "misc/cpp/imgui_stdlib.h"

#include "ImGuiFileDialog.h"

//https://stackoverflow.com/questions/201593/is-there-a-simple-way-to-convert-c-enum-to-string/238157#238157
//https://stackoverflow.com/questions/28828957/enum-to-string-in-modern-c11-c14-c17-and-future-c20
#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 1000
#include <magic_enum/magic_enum.hpp>

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
const fs::path ConnectatronPath = ".";
const fs::path DevicesPath = "Devices";
const fs::path ProjectsPath = "Projects";
const fs::path ConfigurationPath = "Configuration";
#else
const fs::path ConnectatronPath = "../../../../../Applications/Connectatron";
const fs::path DevicesPath = "../../../../../Applications/Connectatron/Devices";
const fs::path ProjectsPath = "../../../../../Applications/Connectatron/Projects";
const fs::path ConfigurationPath = "../../../../../Applications/Connectatron/Configuration";
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

static string Sanitize_Filename(string str)
{
    string illegalChars = "\\/:?\"<>|";
    //https://stackoverflow.com/a/14475510/11502722
    str.erase(
        std::remove_if(str.begin(), str.end(), [&](char chr) { return illegalChars.find(chr) != std::string::npos; }),
        str.end());
    return str;
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
static bool ProjectDirty = false;

//static vector<ConnectorCategoryInfo> ConnectorCategories;
//static vector<PinType> UncategorizedConnectors;
static vector<ProtocolCategoryInfo> ProtocolCategories;
static vector<WireProtocol> UncategorizedProtocols;

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

struct PowerDelivery
{
    double PowerContribution;
    double minVoltage;
    double maxVoltage;
    double minCurrent;
    double maxCurrent;
};


enum class PinKind
{
    Output,
    Input
};

enum class NodeType
{
    Blueprint,
    Blueprint_Editing,
    Simple,
    Tree,
    Comment,
    Houdini
};

struct Node;

struct Pin
{
    ed::PinId   ID;
    weak_ptr<Node> Node;
    std::string Name;
    // Physical connector type
    Connector_ID     Type;
    // Protocols available for communication through this connector
    set<WireProtocol> Protocols;
    std::string ExtraInfo;
    bool IsFemale = true;
    PinKind     Kind;
    // Whether this pin is for an external connection on this project
    bool external_exposed = false;

    Pin(int id, const char* name, Connector_ID type):
        ID(id), Name(name), Type(type), Kind(PinKind::Input)
    {
    }
    Pin(int id, const char* name, Connector_ID type, set<WireProtocol> protocols) :
        ID(id), Name(name), Type(type), Protocols(protocols), Kind(PinKind::Input)
    {
    }
    Pin(int id, const char* name, Connector_ID type, set<WireProtocol> protocols, std::string extraInfo) :
        ID(id), Name(name), Type(type), Protocols(protocols), ExtraInfo(extraInfo), Kind(PinKind::Input)
    {
    }
    Pin(int id, const char* name, Connector_ID type, set<WireProtocol> protocols, std::string extraInfo, bool isFemale) :
        ID(id), Name(name), Type(type), Protocols(protocols), ExtraInfo(extraInfo), IsFemale(isFemale), Kind(PinKind::Input)
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

    // Whether the node has been edited and not saved to a device file.
    // https://github.com/connorjak/Connectatron/issues/19
    bool dirty;
    // Where this node was last saved
    fs::path saved_filepath;
    // Whether this node was loaded as a representation of the external exposed connections of a project
    bool is_project = false;

    std::string State;
    std::string SavedState;

    Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)) :
        ID(id), persistentID(-1), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0), SavedPosition(0, 0)
    {
        dirty = true;
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


static json GetJSONFromFile(fs::path filepath)
{
    std::ifstream i(filepath);
    if (!i)
    {
        throw std::runtime_error("Failed to open file " + filepath.u8string() + " for JSON reading.");
    }
    json js = json::parse(i,nullptr,true,true);
    //i >> js;
    return js;
}


static ordered_json GetOrderedJSONFromFile(fs::path filepath)
{
    std::ifstream i(filepath);
    if (!i)
    {
        throw std::runtime_error("Failed to open file " + filepath.u8string() + " for JSON reading.");
    }
    ordered_json js = ordered_json::parse(i, nullptr, true, true);
    //i >> js;
    return js;
}

//NOTE: saving_in_project_file has significant side effects!
// https://github.com/connorjak/Connectatron/issues/19
static json SerializeDeviceToJSON(const shared_ptr<Node> node, bool saving_in_project_file)
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
    if (saving_in_project_file)
    {
        device["ID"] = node->persistentID;

        auto pos = ed::GetNodePosition(node->ID);
        device["SavedPos"][0] = pos.x;
        device["SavedPos"][1] = pos.y;

        // If the node is not dirty, save as file reference instead of serializing details
        if (!node->dirty)
        {
            device["Dirty"] = false;
            device["File"] = node->saved_filepath.u8string(); //TODO widestring

            return device; //EARLY RETURN
        }
        else
        {
            device["Dirty"] = true;
        }
    }

    // Parse Females
    device["Females"] = json::array();
    auto& j_Females = device["Females"];
    for (const auto& female : node->Females)
    {
        auto& j_female = j_Females.emplace_back(json::object());
        j_female["Name"] = female.Name;

        auto pintype = female.Type;
        auto pintype_str = NameFromPinType(pintype);
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
        auto pintype_str = NameFromPinType(pintype);
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


    /*bool IsCompatiblePin(Pin* a, Pin* b)
    {
        if (a->IsFemale)
            return IsCompatiblePinType(b->Type, a->Type);
        else
            return IsCompatiblePinType(b->Type, a->Type);
    }*/

    bool CanCreateLink(Pin* a, Pin* b)
    {
        if (!a || !b || a == b || a->Kind == b->Kind || a->Node.lock() == b->Node.lock()
            || a->external_exposed || b->external_exposed)
            return false;

        if (a->IsFemale)
        {
            if (!IsCompatiblePinType(b->Type, a->Type))
                return false;
        }
        else
        {
            if (!IsCompatiblePinType(a->Type, b->Type))
                return false;
        }

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
        // Initialize connector types & categories
        connector_categories["UNCATEGORIZED"] = {};
        auto connectors_json = GetOrderedJSONFromFile(ConfigurationPath / "Connectors.json");
        unsigned int connector_ID_iter = 0; // 0 is always UNRECOGNIZED
        for (const auto& connector_j : connectors_json.items())
        {
            auto connector = make_shared<ConnectorInfo>();
            connector->primary_ID = connector_ID_iter++;
            connector->name = connector_j.key();
            connector->full_info = connector_j.value();
            size_t categories_count = 0;
            
            if(connector->full_info.contains("AKA"))
            {
                for (const auto& aka_j : connector->full_info["AKA"].items())
                {
                    connector->AKA_IDs.push_back(aka_j.value().get<string>());
                }
            }

            if (connector->full_info.contains("Links"))
            {
                for (const auto& link_j : connector->full_info["Links"].items())
                {
                    connector->links.push_back(link_j.value().get<string>());
                }
            }

            // Categories onto connector and connector onto categories
            for (const auto& category_j : connector->full_info["Categories"].items())
            {
                categories_count++;
                auto category = category_j.value();
                connector->categories.push_back(category);
                connector_categories[category].push_back(connector);
            }
            if (categories_count == 0)
            {
                connector->categories.push_back("UNCATEGORIZED");
                connector_categories["UNCATEGORIZED"].push_back(connector);
            }
            connectors.emplace(connector->primary_ID, connector);
            connectorsByName.emplace(connector->name, connector->primary_ID);
        }

        // Second pass to resolve MaleFitsInto
        for (const auto connector : connectors)
        {
            if (connector.second->full_info.contains("MaleFitsInto"))
            {
                for (const auto& malefits_j : connector.second->full_info["MaleFitsInto"].items())
                {
                    connector.second->maleFitsInto.push_back(connectorsByName.at(malefits_j.value().get<string>()));
                }
            }
        }

        //ConnectorCategories = GetConnectorCategories();
        //UncategorizedConnectors = GetUncategorizedConnectors();
        
        // Initialize protocol types
        ProtocolCategories = GetProtocolCategories();
        UncategorizedProtocols = GetUncategorizedProtocols();

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

     
        // Connector type icons
        for (const auto conn : connectors)
        {
            if (conn.second->full_info.contains("Icon"))
            {
                auto icon_path = conn.second->full_info["Icon"].get<string>();
                connectorIcons[conn.first] = LoadTexture(icon_path.c_str());

                if (connectorIcons.at(conn.first) == nullptr)
                    throw std::runtime_error(std::string("Icon failed loading, path: ") + icon_path);
            }
        }


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

        for (auto& icon : connectorIcons)
        {
            releaseTexture(icon.second);
        }

        if (m_Editor)
        {
            ed::DestroyEditor(m_Editor);
            m_Editor = nullptr;
        }
    }

    ImColor GetIconColor(Connector_ID type)
    {
        const auto info = connectors.at(type);
        if(info->full_info.contains("IconColor"))
        {
            auto icon_col = info->full_info["IconColor"];
            return ImColor(icon_col[0].get<int>(), icon_col[1].get<int>(), icon_col[2].get<int>());
        }
        else
            return ImColor(255, 255, 255);

        //TODO add this to the JSON
        /*switch (type)
        {
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
        case PinType::DVI___D:                      return IM_COLOR_MAGENTA;
        case PinType::DVI___A:                      return IM_COLOR_MAGENTA;
        case PinType::DVI___I:                      return IM_COLOR_MAGENTA;
        case PinType::VGA:                      return IM_COLOR_MAGENTA;

            //Audio                             
        case PinType::Audio3_5mm:               return IM_COLOR_GREEN;
        case PinType::XLR3:                      return IM_COLOR_GREEN;

            //Other                             
        case PinType::SATA:                     return IM_COLOR_RED;
        case PinType::Micro___SATA:                return IM_COLOR_RED;
        case PinType::eSATA:                    return IM_COLOR_RED;
        case PinType::RJ45:                     return IM_COLOR_RED;

        default:
            //throw std::runtime_error(std::string("Unhandled PinType ") + NameFromPinType(type));
            return ImColor(255, 255, 255);
        }*/
    };

    void DrawPinIcon(const Pin& pin, bool connected, int alpha)
    {
        DrawPinTypeIcon(pin.Type, connected, alpha);
    }

    void RenderIconInText(ImTextureID tex, ImVec2 max_size)
    {
        float height;// = ImGui::CalcTextSize("hi").y;
        float width;

        auto max_aspect_ratio = max_size.x / max_size.y;
        auto aspect_ratio = float(GetTextureWidth(tex)) / float(GetTextureHeight(tex));
        // If image hits sides of bounding box
        if (aspect_ratio > max_aspect_ratio)
        {
            width = max_size.x;
            height = width / aspect_ratio;
        }
        else // Image hits top/bottom of bounding box
        {
            height = max_size.y;
            width = height * aspect_ratio;
        }

        ImGui::Image(tex, ImVec2(width, height));
    }

    void DrawPinTypeIcon(const Connector_ID type, bool connected, int alpha)
    {
        //Set to a default in case the Connector_ID is not supported (somehow)
        IconType iconType = IconType::Circle;
        ImColor  color = GetIconColor(type);
        color.Value.w = alpha / 255.0f;


        auto drawList = ImGui::GetWindowDrawList();

        /*  auto iconPanelPos = start + ImVec2(
            paneWidth - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().IndentSpacing - saveIconWidth - restoreIconWidth - ImGui::GetStyle().ItemInnerSpacing.x * 1,
            (ImGui::GetTextLineHeight() - saveIconHeight) / 2);
        ImGui::SetCursorScreenPos(iconPanelPos);
        ImGui::SetItemAllowOverlap();*/

        
        if (connectorIcons.find(type) != connectorIcons.end())
        {
            RenderIconInText(connectorIcons.at(type), ImVec2(m_PinIconSize * 2, m_PinIconSize));
            return; //EARLY RETURN
        }

        //TODO add to JSON
        /*switch (type)
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
        case PinType::DVI___D:                      iconType = IconType::Grid; break;
        case PinType::DVI___A:                      iconType = IconType::Grid; break;
        case PinType::DVI___I:                      iconType = IconType::Grid; break;
        case PinType::VGA:                      iconType = IconType::Grid; break;

            //Audio                                 
        case PinType::Audio3_5mm:               iconType = IconType::Diamond; break;
        case PinType::XLR3:                      iconType = IconType::Diamond; break;
        case PinType::TOSLINK:                  iconType = IconType::Diamond; break;
        case PinType::Mini___TOSLINK:           iconType = IconType::Diamond; break;

            //Other                                 
        case PinType::SATA:                     iconType = IconType::Circle; break;
        case PinType::Micro___SATA:                iconType = IconType::Circle; break;
        case PinType::eSATA:                    iconType = IconType::Circle; break;
        case PinType::SD:                       iconType = IconType::Circle; break;
        case PinType::miniSD:                     iconType = IconType::Circle; break;
        case PinType::microSD:                  iconType = IconType::Circle; break;
        case PinType::U_2__SFF___8639:                iconType = IconType::Circle; break;
        case PinType::RJ11:                     iconType = IconType::Circle; break;
        case PinType::RJ14:                     iconType = IconType::Circle; break;
        case PinType::RJ25:                     iconType = IconType::Circle; break;
        case PinType::RJ45:                     iconType = IconType::Circle; break;

        default:
            //throw std::runtime_error(std::string("Unhandled PinType ") + NameFromPinType(type));
            iconType = IconType::Circle;
        }*/

        const auto info = connectors.at(type);
        if (info->full_info.contains("IconType"))
        {
            auto icon_type = info->full_info["IconColor"].get<string>();
            if(icon_type == "Flow")
            {
                iconType = IconType::Flow;
            }
            else if (icon_type == "Circle")
            {
                iconType = IconType::Circle;
            }
            else if (icon_type == "Grid")
            {
                iconType = IconType::Grid;
            }
            else if (icon_type == "Diamond")
            {
                iconType = IconType::Diamond;
            }
            else
            {
                iconType = IconType::Circle;
            }
        }

        ax::Widgets::Icon(ImVec2(m_PinIconSize, m_PinIconSize), iconType, connected, color, ImColor(32, 32, 32, alpha));
    };

    //TODO error handling (probably just wrap in try/catch when called)
//TODO load position in project?
    shared_ptr<Node> SpawnNodeFromJSON(const json& device)
    {
        ProjectDirty = true;

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

        if (device.find("ExternalPins") != device.end())
        {
            for (const auto& ext : device["ExternalPins"].items())
            {
                //format:
                /*
                ExternalPins: [
                [true, 1],
                [false, 0],
                // ^ isFemale, pin number
                ]
                */

                bool isFemale = ext.value()[0].get<bool>();
                int pinNum = ext.value()[1].get<int>();

                if (isFemale)
                    new_node->Females[pinNum].external_exposed = true;
                else
                    new_node->Males[pinNum].external_exposed = true;

            }
        }

        m_IdNodes[id] = new_node;

        return new_node;
    }

    bool DeviceHasSubDevices(const json& device) 
    {
        return device.find("Devices") != device.end();
    }

    void NewConnectorFromJSON(const json& connector, shared_ptr<Node>& new_node, bool isFemale)
    {
        auto in_name = connector["Name"].get<string>();

        auto pintype_string = connector["PinType"].get<string>();

        Connector_ID in_pintype = 0;// "UNRECOGNIZED";
        if (connectorsByName.find(pintype_string) != connectorsByName.end())
        {
            in_pintype = connectorsByName.at(pintype_string);
        }

        //Can have just pintype, or pintype+protocols, or pintype+protocols+description
        if (connector.find("Protocols") != connector.end())
        {
            set<WireProtocol> protocols;
            for (auto protocol : connector["Protocols"])
            {
                auto protocol_string = protocol.get<string>();
                EnumName_Symbol2Underscore(protocol_string);

                auto parsed_protocol = magic_enum::enum_cast<WireProtocol>(protocol_string);
                protocols.insert(parsed_protocol.value_or(WireProtocol::UNRECOGNIZED));
                //TODO handle "sets" of protocols, like the backcompat variables?
            }

            if (connector.find("Description") != connector.end())
            {
                auto description = connector["Description"].get<string>();
                if(isFemale)
                    new_node->Females.emplace_back(GetNextId(), in_name.c_str(), in_pintype, protocols, description);
                else
                    new_node->Males.emplace_back(GetNextId(), in_name.c_str(), in_pintype, protocols, description);
            }
            else
            {
                if (isFemale)
                    new_node->Females.emplace_back(GetNextId(), in_name.c_str(), in_pintype, protocols);
                else
                    new_node->Males.emplace_back(GetNextId(), in_name.c_str(), in_pintype, protocols);
            }
        }
        else
        {
            if (isFemale)
                new_node->Females.emplace_back(GetNextId(), in_name.c_str(), in_pintype);
            else
                new_node->Males.emplace_back(GetNextId(), in_name.c_str(), in_pintype);
        }
    }

    // Load node WITHOUT SPAWNING IT IN THE EDITOR!
    void InitNodeFromJSON(const json& device, shared_ptr<Node>& new_node)
    {
        if (DeviceHasSubDevices(device))
        {
            // This is a project-as-a-device
            new_node->is_project = true;
            if (device.find("SavedPos") != device.end())
            {
                new_node->SavedPosition.x = device["SavedPos"][0].get<float>();
                new_node->SavedPosition.y = device["SavedPos"][1].get<float>();
            }

            // Make nodes
            const auto& SubDevices = device["Devices"];
            for (const auto& subdevice : SubDevices)
            {
                if (subdevice.find("ExternalPins") != subdevice.end())
                {
                    if (subdevice.find("File") != subdevice.end())
                    {
                        auto subdevice_data_filepath = subdevice["File"].get<string>();
                        auto subdevice_data = GetJSONFromFile(ConnectatronPath / subdevice_data_filepath);
                        for (const auto& ext : subdevice["ExternalPins"].items())
                        {
                            auto ext_val = ext.value();
                            bool isFemale = ext_val[0].get<bool>();
                            int pinNum = ext_val[1].get<int>();

                            if (isFemale)
                            {
                                const auto& female = subdevice_data["Females"][pinNum];
                                NewConnectorFromJSON(female, new_node, true);
                            }
                            else
                            {
                                const auto& male = subdevice_data["Males"][pinNum];
                                NewConnectorFromJSON(male, new_node, false);
                            }

                        }
                    }
                    else
                    {
                        //TODO BREAKING
                    }
                }
            }
        }
        else
        {
            // This is a normal device
            if (device.find("SavedPos") != device.end())
            {
                new_node->SavedPosition.x = device["SavedPos"][0].get<float>();
                new_node->SavedPosition.y = device["SavedPos"][1].get<float>();
            }

            // If has field "Dirty"
            if (device.find("Dirty") != device.end())
            {
                // If device is not dirty
                if (!device["Dirty"].get<bool>())
                {
                    new_node->saved_filepath = device["File"].get<string>();
                    new_node->dirty = false;
                    auto actual_node_data = GetJSONFromFile(ConnectatronPath / new_node->saved_filepath);
                    InitNodeFromJSON(actual_node_data, new_node);
                    //TODO does this miss useful stuff in SpawnNodeFromJSON?
                    //   Might need to refactor...

                    return; // EARLY RETURN
                }
            }

            // Parse Females
            for (const auto& female : device["Females"])
            {
                NewConnectorFromJSON(female, new_node, true);
            }

            // Parse Males
            for (const auto& male : device["Males"])
            {
                NewConnectorFromJSON(male, new_node, false);
            }
        }
        BuildNode(new_node);
    }

    Link* SpawnLinkFromJSON(const json& connect)
    {
        try
        {
            auto start_p_id = connect[0].get<int>();
            auto end_p_id = connect[1].get<int>();

            auto start_pin_loc = connect[2].get<int>();
            auto end_pin_loc = connect[3].get<int>();

            if(m_IdNodes.find(start_p_id) == m_IdNodes.end())
                return nullptr;
            if (m_IdNodes.find(end_p_id) == m_IdNodes.end())
                return nullptr;

            auto startNode = m_IdNodes.at(start_p_id);
            auto endNode = m_IdNodes.at(end_p_id);

            if (startNode->Males.size() < start_pin_loc)
                return nullptr;
            if (endNode->Females.size() < end_pin_loc)
                return nullptr;

            auto startPinId = startNode->Males[start_pin_loc].ID;
            auto endPinId = endNode->Females[end_pin_loc].ID;

            m_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));

            m_Links.back().Color = GetIconColor(startNode->Males[start_pin_loc].Type);
        }
        catch (std::exception& e)
        {
            return nullptr;
        }
        

        return &m_Links.back();
    }

    json SerializeLinkToJSON(const Link& link)
    {
        json connect;

        auto startPin = FindPin(link.StartPinID);
        auto endPin = FindPin(link.EndPinID);
        auto startNode = startPin->Node.lock();
        auto endNode = endPin->Node.lock();
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

    void SaveDeviceToFile(shared_ptr<Node> node, fs::path filepath)
    {
        std::ofstream o(filepath);
        json device = SerializeDeviceToJSON(node, false);

        o << device.dump(1,'\t');
        o.close();
    }

    void SaveProjectToFile(fs::path filepath)
    {
        std::ofstream o(filepath);
        json project;

        project["Name"] = filepath.stem();
        project["Devices"] = json::array();
        auto& devices = project["Devices"];

        for (const auto& node : m_Nodes)
        {
            if (node->Type == NodeType::Blueprint ||
                node->Type == NodeType::Blueprint_Editing/* ||
                node->Type == NodeType::Comment TODO*/)
            {
                json device = SerializeDeviceToJSON(node, true);

                // Record which pins are external
                int iter = 0;
                bool once = false;
                for (const auto& female : node->Females)
                {
                    if(female.external_exposed)
                    {
                        if (!once)
                            device["ExternalPins"] = json::array();
                        once = true;

                        json exposed = json::array();
                        exposed.push_back(true);
                        exposed.push_back(iter);

                        device["ExternalPins"].push_back(exposed);
                    }
                    iter++;
                }
                iter = 0;
                for (const auto& male : node->Males)
                {
                    if (male.external_exposed)
                    {
                        if (!once)
                            device["ExternalPins"] = json::array();
                        once = true;

                        json exposed = json::array();
                        exposed.push_back(false);
                        exposed.push_back(iter);

                        device["ExternalPins"].push_back(exposed);
                    }
                    iter++;
                }


                devices.push_back(device);
            }
        }

        project["Links"] = json::array();
        auto& connects = project["Links"];

        for (const auto& link : m_Links)
        {
            connects.push_back(SerializeLinkToJSON(link));
        }

        o << project.dump(0);
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

            if (device.find("ExternalPins") != device.end())
            {
                for (const auto& ext : device["ExternalPins"].items())
                {
                    //format:
                    /*
                    ExternalPins: [
                    [true, 1],
                    [false, 0],
                    // ^ isFemale, pin number
                    ]
                    */

                    bool isFemale = ext.value()[0].get<bool>();
                    int pinNum = ext.value()[1].get<int>();

                    if (isFemale)
                        node->Females[pinNum].external_exposed = true;
                    else
                        node->Males[pinNum].external_exposed = true;

                }
            }

            ed::SetNodePosition(node->ID, node->SavedPosition);
        }

        const auto& Connects = project["Links"];
        for (const auto& connect : Connects)
        {
            //NOTE: returns nullptr upon failure
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
                ImGuiFileDialog::Instance()->OpenDialog("SaveProjectAs", /*ICON_IGFD_FOLDER_OPEN*/ " Save Project As", filters,
                    ProjectsPath.string(), 
                    CurrentProjectName,
                    1, nullptr, flags);
            else
                ImGuiFileDialog::Instance()->OpenModal("SaveProjectAs", /*ICON_IGFD_FOLDER_OPEN*/ " Save Project As", filters,
                    ProjectsPath.string(),
                    CurrentProjectName,
                    1, nullptr, flags);
        }

        // display
        if (ImGuiFileDialog::Instance()->Display("SaveProjectAs"))
        {
            // action if pressed OK
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                ProjectDirty = false;

                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                std::string fileName_stem = fs::path(filePathName).filename().stem().string();
                // action

                CurrentProjectName = fileName_stem;

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
            if (ProjectDirty)
            {
                //TODO warning popup
            }


            //const char* filters = ".*,.con,.dev,.json";
            const char* filters = ".con";

            ImGuiFileDialogFlags flags = 0;
            flags |= ImGuiFileDialogFlags_::ImGuiFileDialogFlags_DontShowHiddenFiles;

            if (/*standardDialogMode*/true)
                ImGuiFileDialog::Instance()->OpenDialog("OpenProject", /*ICON_IGFD_FOLDER_OPEN*/ " Open Project", filters,
                    ProjectsPath.string(), 
                    CurrentProjectName, 
                    1, nullptr, flags);
            else
                ImGuiFileDialog::Instance()->OpenModal("OpenProject", /*ICON_IGFD_FOLDER_OPEN*/ " Open Project", filters, 
                    ProjectsPath.string(), 
                    CurrentProjectName, 
                    1, nullptr, flags);
        }

        // display
        if (ImGuiFileDialog::Instance()->Display("OpenProject"))
        {
            // action if pressed OK
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                std::string fileName_stem = fs::path(filePathName).filename().stem().string();

                CurrentProjectName = fileName_stem;

                std::cout << "Opening Project:" << std::endl;
                std::cout << "filePathName: " << filePathName << std::endl;
                std::cout << "filePath: " << filePath << std::endl;
                LoadProjectFromFile(fs::path(filePathName));

                // Set not-dirty down here because LoadProjectFromFile calls functions that set this to true.
                ProjectDirty = false;
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
        ImGui::TextUnformatted("Devices");
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
        for (int i = 0; i < nodeCount; ++i) ImGui::Text("Device (%p)", selectedNodes[i].AsPointer());
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
#if _DEBUG
        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
#endif
        ed::SetCurrentEditor(m_Editor);

        string title_text = "Connectatron: " + CurrentProjectName;

        if (ProjectDirty)
            title_text += " (UNSAVED)";

        SetTitle(title_text.c_str());

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

            //////////////////////////////////////////////////////
            /// DRAW NODES
            ////////////////////////////////////////////////

            // Draw all Blueprint and Simple nodes
            for (auto& node : m_Nodes)
            {
                if (node->Type != NodeType::Blueprint && node->Type != NodeType::Simple)
                    continue;

                const auto isSimple = node->Type == NodeType::Simple;

                builder.Begin(node->ID);
                if (!isSimple)
                {
                    builder.Header(node->Color);
                        ImGui::Spring(0);
                        ImGui::TextUnformatted(node->Name.c_str());
                        ImGui::Spring(1);
                        ImGui::Dummy(ImVec2(0, 28));
                        // Dirty-marker
                        if (node->dirty)
                        {
                            ImGui::TextUnformatted("*");
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("Device as-is is only saved in project.");
                        }
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
                    if (female.external_exposed)
                    {
                        ImGui::TextUnformatted("EX");
                    }
                    DrawPinIcon(female, IsPinLinked(female.ID), (int)(alpha * 255));
                    ImGui::Spring(0);
                    if (!female.Name.empty())
                    {
                        ImGui::TextUnformatted(female.Name.c_str());
                        ImGui::Spring(0);
                    }
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
                    auto alpha = ImGui::GetStyle().Alpha;
                    if (newLinkPin && !CanCreateLink(newLinkPin, &male) && &male != newLinkPin)
                        alpha = alpha * (48.0f / 255.0f);

                    builder.Output(male.ID);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                    if (!male.Name.empty())
                    {
                        ImGui::Spring(0);
                        ImGui::TextUnformatted(male.Name.c_str());
                    }
                    ImGui::Spring(0);
                    DrawPinIcon(male, IsPinLinked(male.ID), (int)(alpha * 255));
                    if (male.external_exposed)
                    {
                        ImGui::TextUnformatted("EX");
                    }
                    ImGui::PopStyleVar();
                    builder.EndOutput();
                }

                builder.End();
            }

            // Draw all Blueprint_Editing
            for (auto& node : m_Nodes)
            {
                if (node->Type != NodeType::Blueprint_Editing)
                    continue;

                builder.Begin(node->ID);

                builder.Header(node->Color);
                    ImGui::Spring(0);
                    string for_width_calc = node->Name + "  ";

                    ImGui::PushItemWidth(ImGui::CalcTextSize(for_width_calc.c_str()).x);
                    ImGui::InputText("##NodeName", &node->Name);
                    ImGui::PopItemWidth();
                    ImGui::Spring(1);
                    ImGui::Dummy(ImVec2(0, 28));
                    // Dirty-marker
                    if (node->dirty)
                    {
                        ImGui::TextUnformatted("*");
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Device as-is is only saved in project.");
                    }
                    ImGui::Spring(0);
                builder.EndHeader();

                int itr = 0;

                // Draw female pin icons
                for (auto& female : node->Females)
                {
                    auto alpha = ImGui::GetStyle().Alpha;
                    if (newLinkPin && !CanCreateLink(newLinkPin, &female) && &female != newLinkPin)
                        alpha = alpha * (48.0f / 255.0f);

                    builder.Input(female.ID);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                    if (female.external_exposed)
                    {
                        ImGui::TextUnformatted("EX");
                    }
                    DrawPinIcon(female, IsPinLinked(female.ID), (int)(alpha * 255));
                    ImGui::Spring(0);
                    if (true)
                    {
                        static bool wasActive = false;
                        ImGui::PushItemWidth(ImGui::CalcTextSize(female.Name.c_str()).x + 10.0f);

                        auto val = female.Name;
                        auto pastval = val;
                        ImGui::InputText("", &val);
                        if (pastval != val)
                            female.Name = val;

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
                    }
                    ImGui::PopStyleVar();
                    builder.EndInput();
                }

                builder.Input_NoPin(node.get() + 1); //TODO pointer arithmetic? Might be flawed
                if (ImGui::Button("New Connector"))
                {
                    node->dirty = true;
                    ProjectDirty = true;
                    auto new_id = GetNextId();
                    string new_name = "connector " + std::to_string(new_id);
                    auto& new_connector = node->Females.emplace_back(new_id, new_name.c_str(), connectorsByName.at("Proprietary"));
                    BuildNode(node); //NOTE: overkill but effective.
                }
                builder.EndInput_NoPin();

                // Draw male pin icons
                for (auto& male : node->Males)
                {
                    ImGui::PushID(itr++);
                    auto alpha = ImGui::GetStyle().Alpha;
                    if (newLinkPin && !CanCreateLink(newLinkPin, &male) && &male != newLinkPin)
                        alpha = alpha * (48.0f / 255.0f);

                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                    builder.Output(male.ID);
                    if (true)
                    {
                        static bool wasActive = false;
                        ImGui::PushItemWidth(ImGui::CalcTextSize(male.Name.c_str()).x + 10.0f);

                        auto val = male.Name;
                        auto pastval = val;
                        ImGui::InputText("", &val);
                        if (pastval != val)
                            male.Name = val;

                        //ImGui::PopItemWidth();
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
                    }
                    ImGui::Spring(0);
                    DrawPinIcon(male, IsPinLinked(male.ID), (int)(alpha * 255));
                    if (male.external_exposed)
                    {
                        ImGui::TextUnformatted("EX");
                    }
                    ImGui::PopStyleVar();
                    builder.EndOutput();
                    ImGui::PopID();
                }

                builder.Output_NoPin(node.get() + 2); //TODO pointer arithmetic? Might be flawed
                if (ImGui::Button("New Connector"))
                {
                    node->dirty = true;
                    ProjectDirty = true;
                    auto new_id = GetNextId();
                    string new_name = "connector " + std::to_string(new_id);
                    auto& new_pin = node->Males.emplace_back(new_id, new_name.c_str(), connectorsByName.at("Proprietary"));
                    BuildNode(node); //NOTE: overkill but effective.
                }
                builder.EndOutput_NoPin();

                builder.End();
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


            //////////////////////////////////////////////////////
            /// DRAW LINKS
            ////////////////////////////////////////////////

            // Draw all links
            for (auto& link : m_Links)
                ed::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);


            // Single-entrance flag for single Create New Node popup
            // (Deactivates
            if (!createNewNode)
            {
                //////////////////////////////////////////////////////
                /// PIN-DRAGGING
                ////////////////////////////////////////////////
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

                        auto malePin = startPin;
                        auto malePinId = startPinId;
                        auto femalePin = endPin;
                        auto femalePinId = endPinId;
                        if (malePin->Kind == PinKind::Input)
                        {
                            std::swap(malePin, femalePin);
                            std::swap(malePinId, femalePinId);
                        }
                        // We ensure that the malePin is output/male

                        if (malePin && femalePin)
                        {
                            if (femalePin == malePin)
                            {
                                // Can't loop back to self
                                ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                            }
                            else if (femalePin->Kind == malePin->Kind)
                            {
                                showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
                                ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                            }
                            //else if (femalePin->Node == malePin->Node)
                            //{
                            //    showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
                            //    ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
                            //}
                            else if (!IsCompatiblePinType(malePin->Type, femalePin->Type))
                            {
                                showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
                                ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                            }
                            else
                            {
                                showLabel("+ Create Link", ImColor(32, 45, 32, 180));
                                if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
                                {
                                    if (GetConnectorMultiplePerPin(malePin->Type) && GetConnectorMultiplePerPin(femalePin->Type))
                                    {
                                        // Allow multiple connections per pin
                                        m_Links.emplace_back(Link(GetNextId(), malePinId, femalePinId));
                                        m_Links.back().Color = GetIconColor(malePin->Type);
                                    }
                                    else
                                    {
                                        // Replace any existing connections with new connection

                                        std::vector<size_t> linksToDelete;
                                        size_t idx = 0;
                                        for (auto& link : m_Links)
                                        {
                                            if(link.StartPinID == malePinId)
                                                linksToDelete.push_back(idx);
                                            if (link.EndPinID == femalePinId)
                                                linksToDelete.push_back(idx);
                                            idx++;
                                        }

                                        // Sort linksToDelete in reverse order
                                        std::sort(linksToDelete.rbegin(), linksToDelete.rend());

                                        // Erase at indices
                                        for (auto& linkIdx : linksToDelete)
                                        {
                                            //ed::DeleteLink(linkId);
                                            m_Links.erase(m_Links.begin() + linkIdx);
                                            //m_Links.erase(linkId);
                                        }

                                        m_Links.emplace_back(Link(GetNextId(), malePinId, femalePinId));
                                        m_Links.back().Color = GetIconColor(malePin->Type);
                                    }
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
                            showLabel("+ Create Device", ImColor(32, 45, 32, 180));

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

                //////////////////////////////////////////////////////
                /// HANDLE DELETIONS
                ////////////////////////////////////////////////
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
        } // end "Node Editor" UI area

        //////////////////////////////////////////////////////
        /// TRIGGER MOUSE-CLICK POPUP CONTEXT MENUS
        ////////////////////////////////////////////////
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

        //////////////////////////////////////////////////////
        /// DEVICE INFO POPUP
        ////////////////////////////////////////////////

        static shared_ptr<Node> node_to_save;

        ed::Suspend();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        if (ImGui::BeginPopup("Node Context Menu"))
        {
            auto node = FindNode(contextNodeId);

            ImGui::TextUnformatted("Device Information");
            ImGui::Separator();
            if (node)
            {
                if (node->Type == NodeType::Blueprint)
                {
                    if (node->is_project)
                    {
                        string msg = "Edit project file " + node->saved_filepath.string() + " to change this node.";
                        ImGui::Text(msg.c_str());
                    }
                    else
                    {
                        if (ImGui::Button("Edit Device"))
                        {
                            node->Type = NodeType::Blueprint_Editing;
                        }
                    }
                }
                else if (node->Type == NodeType::Blueprint_Editing)
                {
                    //if (ImGui::Button(/*ICON_IGFD_FOLDER_OPEN*/ "Save Device"))
                    //{
                    //    node->Type = NodeType::Blueprint;
                    //    
                    //}

                    if (ImGui::Button(/*ICON_IGFD_FOLDER_OPEN*/ "Save Device As..."))
                    {
                        const char* filters = ".json";

                        node_to_save = node;

                        ImGuiFileDialogFlags flags = 0;
                        flags |= ImGuiFileDialogFlags_::ImGuiFileDialogFlags_ConfirmOverwrite;
                        flags |= ImGuiFileDialogFlags_::ImGuiFileDialogFlags_DontShowHiddenFiles;

                        
                        // If previously-saved-as path isn't default
                        if (node->saved_filepath.u8string() != fs::path())
                        {
                            auto starting_filepath = ConnectatronPath / node->saved_filepath;

                            if (/*standardDialogMode*/true)
                                ImGuiFileDialog::Instance()->OpenDialog("SaveDeviceAs", /*ICON_IGFD_FOLDER_OPEN*/ " Save Device As", filters, 
                                    starting_filepath.parent_path().string(),
                                    starting_filepath.stem().string(),
                                    1, nullptr, flags);
                            else
                                ImGuiFileDialog::Instance()->OpenModal("SaveDeviceAs", /*ICON_IGFD_FOLDER_OPEN*/ " Save Device As", filters, 
                                    starting_filepath.parent_path().string(),
                                    starting_filepath.stem().string(),
                                    1, nullptr, flags);
                        }
                        else
                        {
                            fs::path os_filename;
                            // Need to come up with a new name for this file
                            auto unclean = node->Name + ".json";
                            auto clean = Sanitize_Filename(unclean);

                            os_filename = fs::path(clean);

                            if (/*standardDialogMode*/true)
                                ImGuiFileDialog::Instance()->OpenDialog("SaveDeviceAs", /*ICON_IGFD_FOLDER_OPEN*/ " Save Device As", filters, 
                                    DevicesPath.string(), 
                                    os_filename.string(), 
                                    1, nullptr, flags);
                            else
                                ImGuiFileDialog::Instance()->OpenModal("SaveDeviceAs", /*ICON_IGFD_FOLDER_OPEN*/ " Save Device As", filters, 
                                    DevicesPath.string(), 
                                    os_filename.string(), 
                                    1, nullptr, flags);
                        }
                    }

                    // See below the EndPopup() for the file dialog GUI
                }

                string last_saved_as = node->saved_filepath.u8string(); // TODO widestring
                if (last_saved_as == fs::path())
                {
                    ImGui::Text("Device not yet saved outside of this project.");
                }
                else
                {
                    string last_saved_as_msg = "Last Saved As: " + last_saved_as;
                    ImGui::Text(last_saved_as_msg.c_str());
                }

                ImGui::Text("%d Female Connectors", (int)node->Females.size());
                ImGui::Text("%d Male Connectors", (int)node->Males.size());
#ifdef _DEBUG
                ImGui::Separator();
                ImGui::Text("Type: %s", node->Type == NodeType::Blueprint ? "Blueprint" : (node->Type == NodeType::Blueprint_Editing ? "Blueprint_Editing" : "Comment"));
                ImGui::Text("ID: %p", node->ID.AsPointer());
#endif
            }
            else
                ImGui::Text("Unknown Device: %p", contextNodeId.AsPointer());
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
                ed::DeleteNode(contextNodeId);
            ImGui::EndPopup();
        }

        //////////////////////////////////////////////////////
        /// SAVE-DEVICE-AS FILE DIALOG
        ////////////////////////////////////////////////

        // display
        if (ImGuiFileDialog::Instance()->Display("SaveDeviceAs"))
        {
            // action if pressed OK
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                ProjectDirty = true; //TODO should it?

                node_to_save->Type = NodeType::Blueprint;
                std::string abs_filePath_withName = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string abs_filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                std::string fileName_stem = fs::path(abs_filePath_withName).filename().stem().string();

                fs::path abs_filepath = fs::path(abs_filePath_withName);
                auto rel_filepath = fs::relative(abs_filepath, ConnectatronPath);
                //TODO if not under DevicesPath, do something else???
                
                // Change cached save-location on the node
                
                node_to_save->saved_filepath = rel_filepath;
                node_to_save->dirty = false;

                // If still using the name from empty_device.json, change the name to match the filename
                if(node_to_save->Name == "New Custom Node")
                    node_to_save->Name = fileName_stem;

                std::cout << "Saving Device As:" << std::endl;
                std::cout << "abs_filePath_withName: " << abs_filePath_withName << std::endl;
                std::cout << "abs_filePath: " << abs_filePath << std::endl;
                std::cout << "rel_filepath: " << rel_filepath << std::endl;
                SaveDeviceToFile(node_to_save, ConnectatronPath / rel_filepath);
                node_to_save.reset();
            }

            // close
            ImGuiFileDialog::Instance()->Close();
        }

        //////////////////////////////////////////////////////
        /// CONNECTOR INFO POPUP
        ////////////////////////////////////////////////

        if (ImGui::BeginPopup("Pin Context Menu"))
        {
            auto pin = FindPin(contextPinId);

            ImGui::TextUnformatted("Connector Information");
            ImGui::Separator();
            if (pin)
            {
                auto on_node = pin->Node.lock();

                // Connector info //
                ImGui::Text("Gender: ");
                ImGui::SameLine();
                ImGui::Text(pin->IsFemale ? "Female" : "Male");

                if (ImGui::Button("Duplicate Connector"))
                {
                    on_node->dirty = true;
                    ProjectDirty = true;
                    auto new_id = GetNextId();
                    string new_name = pin->Name + " copy";
                    if(pin->IsFemale)
                    {
                        auto& new_connector = on_node->Females.emplace_back(new_id, new_name.c_str(), pin->Type);
                        BuildNode(on_node); //NOTE: overkill but effective.
                        new_connector.Protocols = pin->Protocols;
                    }
                    else
                    {
                        auto& new_connector = on_node->Males.emplace_back(new_id, new_name.c_str(), pin->Type);
                        BuildNode(on_node); //NOTE: overkill but effective.
                        new_connector.Protocols = pin->Protocols;

                    }
                }

                string ex_exp_title = "External Exposed";
                bool changed = ImGui::Checkbox(ex_exp_title.c_str(), &pin->external_exposed);
                if (changed)
                    ProjectDirty = true;
                if (on_node->Type == NodeType::Blueprint_Editing)
                {
                    auto connector_width = ImGui::CalcTextSize(LONGEST_CONNECTOR_STR).x * 1.5;
                    auto current_connect_string = NameFromPinType(pin->Type);
                    ImGui::Text("Type:");
                    //ImGui::BeginChild("##Connector Editing", ImVec2(protocol_width, ImGui::GetTextLineHeightWithSpacing() * 10), true);
                    //https://github.com/ocornut/imgui/issues/1658#issue-302026543
                    
                    string editing_title = current_connect_string + "##Connector Editing";

                    if (ImGui::BeginMenu(editing_title.c_str()))
                    {
                        for (const auto& possible_connect : connector_categories.at("UNCATEGORIZED"))
                        {
                            auto connect_string = NameFromPinType(possible_connect->primary_ID);
                            /*EnumName_Underscore2Symbol(connect_string);*/

                            DrawPinTypeIcon(possible_connect->primary_ID, false, 255);
                            ImGui::SameLine();

                            bool is_selected = possible_connect->primary_ID == pin->Type;
                            if (ImGui::MenuItem(connect_string.c_str(), "", is_selected))
                            {
                                ProjectDirty = true;
                                on_node->dirty = true;
                                pin->Type = possible_connect->primary_ID;
                            }
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }

                        for (const auto& category : connector_categories)
                        {
                            if (ImGui::BeginMenu(category.first.c_str()))
                            {
                                for (const auto possible_connect : category.second)
                                {
                                    auto connect_string = NameFromPinType(possible_connect->primary_ID);
                                    //EnumName_Underscore2Symbol(connect_string);

                                    DrawPinTypeIcon(possible_connect->primary_ID, false, 255);
                                    ImGui::SameLine();

                                    bool is_selected = possible_connect->primary_ID == pin->Type;
                                    //TODO also try Selectable?
                                    if (ImGui::MenuItem(connect_string.c_str(), "", is_selected))
                                    {
                                        ProjectDirty = true;
                                        on_node->dirty = true;
                                        pin->Type = possible_connect->primary_ID;
                                    }
                                    if (is_selected)
                                        ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndMenu();
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
                else // Not editing
                {
                    ImGui::Text("Connector Type: ");
                    ImGui::SameLine();
                    auto pin_enum_name = NameFromPinType(pin->Type);
                    ImGui::Text(pin_enum_name.c_str());
                }

                const auto& aka_ids = connectors.at(pin->Type)->AKA_IDs;
                if (aka_ids.size() > 0)
                {
                    ImGui::Text("AKA:");
                    string aka_text;
                    for (const auto& aka : connectors.at(pin->Type)->AKA_IDs)
                    {
                        aka_text += "\"" + aka + "\", ";
                    }
                
                    aka_text.pop_back();
                    aka_text.pop_back();
                    // Remove last 2 chars.
                    ImGui::SameLine();
                    ImGui::Text(aka_text.c_str());
                }

                ImGui::Separator();
                
                // Protocols info //
                ImGui::Text("Supported Protocols:");
                if (on_node->Type == NodeType::Blueprint_Editing)
                {
                    //float protocol_width = ImGui::CalcTextSize(LONGEST_PROTOCOL_STR).x * 1.5;
                    float protocol_width = 0; //Assuming LONGEST_CONNECTOR_STR stuff above will eliminate the need to specify particular width

                    string editing_title = "Select Protocols...";

                    if (ImGui::BeginMenu(editing_title.c_str()))
                    {
                        for (const auto& possible_proto : UncategorizedProtocols)
                        {
                            auto proto_string = NameFromProtocol(possible_proto);
                            EnumName_Underscore2Symbol(proto_string);

                            //TODO might use icons for protocols later?
                            /*DrawPinTypeIcon(possible_proto, false, 255);
                            ImGui::SameLine();*/

                            bool val = pin->Protocols.find(possible_proto) != pin->Protocols.end();
                            auto pastval = val;
                            ImGui::Checkbox(proto_string.c_str(), &val);
                            if (val != pastval)
                            {
                                ProjectDirty = true;
                                on_node->dirty = true;
                                if (val)
                                    pin->Protocols.insert(possible_proto);
                                else
                                    pin->Protocols.erase(possible_proto);
                            }
                        }

                        for (const auto& category : ProtocolCategories)
                        {
                            if (ImGui::BeginMenu(category.name.c_str()))
                            {
                                for (const auto& possible_proto : category.protocols)
                                {
                                    auto proto_string = NameFromProtocol(possible_proto);
                                    EnumName_Underscore2Symbol(proto_string);

                                    //TODO might use icons for protocols later?
                                    /*DrawPinTypeIcon(possible_proto, false, 255);
                                    ImGui::SameLine();*/

                                    bool val = pin->Protocols.find(possible_proto) != pin->Protocols.end();
                                    auto pastval = val;
                                    ImGui::Checkbox(proto_string.c_str(), &val);
                                    if (val != pastval)
                                    {
                                        ProjectDirty = true;
                                        on_node->dirty = true;
                                        if (val)
                                            pin->Protocols.insert(possible_proto);
                                        else
                                            pin->Protocols.erase(possible_proto);
                                    }
                                }
                                ImGui::EndMenu();
                            }
                        }
                        ImGui::EndMenu();
                    }
                } //End Blueprint_Editing-specific stuff

                auto protocol_width = ImGui::CalcTextSize(LONGEST_PROTOCOL_STR).x * 1.1;
                ImGui::BeginChild("##Protocol Viewing", ImVec2(protocol_width, ImGui::GetTextLineHeightWithSpacing() * 10), true);
                if (pin->Protocols.size() == 0)
                {
                    ImGui::Text("(None specified)");
                }
                else
                {
                    for (auto supportedProto : pin->Protocols)
                    {
                        ImGui::Text(NameFromProtocol(supportedProto).c_str());
                    }
                }
                ImGui::EndChild();

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
                if (on_node)
                    ImGui::Text("Node: %p", on_node->ID.AsPointer());
                else
                    ImGui::Text("Node: %s", "<none>");
#endif
                if (on_node->Type == NodeType::Blueprint_Editing)
                {
                    if (ImGui::Button("Delete Connector"))
                    {
                        auto& males_vec = on_node->Males;
                        auto& females_vec = on_node->Females;
                        if (pin->IsFemale)
                        {
                            auto to_del = std::find_if(females_vec.begin(), females_vec.end(), [pin](const Pin& test_pin) {
                                return test_pin.ID == pin->ID;
                                });

                            females_vec.erase(to_del);
                        }
                        else
                        {
                            auto to_del = std::find_if(males_vec.begin(), males_vec.end(), [pin](const Pin& test_pin) {
                                return test_pin.ID == pin->ID;
                                });

                            males_vec.erase(to_del);
                        }

                        ImGui::CloseCurrentPopup();
                    }
                }
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
            {
                ed::DeleteLink(contextLinkId);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        //////////////////////////////////////////////////////
        /// CREATE NEW NODE POPUP
        ////////////////////////////////////////////////

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
            Connector_ID valid_type;
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

            if (ImGui::MenuItem("Create New Custom Device"))
            {
                node = SpawnNodeFromJSON(GetJSONFromFile(DevicesPath / ".." / "data" / "empty_device.json"));
                node->Type = NodeType::Blueprint_Editing;
            }
            ImGui::Separator();

            if (limitByPinType) // Limit selection by pin compatibility
            {
                auto valid_type_str = NameFromPinType(valid_type);
                
                string m_or_f = newNodeLinkPin->IsFemale ? "female" : "male";

                string compat_msg = "Compatible with " + m_or_f + " " + valid_type_str + " connector: ";

                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 220, 170, 255));
                ImGui::Text(compat_msg.c_str());
                ImGui::PopStyleColor();
            }

            vector<fs::path> categories = { ".", "Cables (M-M)", "Adapters (M-F)", "../Projects" };
            // Iterate through files in Devices and show them as options
            //TODO use recursive directory iterator instead!
            for(auto category : categories)
            {
                string category_label = category.generic_string();
                auto search_dir = DevicesPath / category;
                if (category_label == ".")
                    category_label = "Devices";

                auto header_flags = ImGuiTreeNodeFlags_CollapsingHeader | ImGuiTreeNodeFlags_DefaultOpen;
                if(ImGui::CollapsingHeader(category_label.c_str(), header_flags))
                {
                    bool nothing_shown_yet = true;
                    for (auto const& devicePath : fs::directory_iterator{ search_dir })
                    {
                        if(devicePath.path().extension() == fs::path(".json") || devicePath.path().extension() == fs::path(".con"))
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
                                        if (IsCompatiblePinType(valid_type, pin.Type))
                                        {
                                            this_node_is_valid = true;
                                            break;
                                        }
                                    }
                                    break;
                                case PinKind::Output: // Will be connecting to a male pin on new node
                                    for (const auto& pin : temp_node->Males)
                                    {
                                        if (IsCompatiblePinType(pin.Type, valid_type))
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
                                    {
                                        node = SpawnNodeFromJSON(GetJSONFromFile(devicePath.path()));
                                        node->saved_filepath = fs::relative(devicePath, ConnectatronPath);
                                        node->dirty = false;
                                    }
                                }
                            }
                            else // Any node would be fine
                            {
                                nothing_shown_yet = false;
                                if (ImGui::MenuItem(devicePath.path().stem().string().c_str()))
                                {
                                    node = SpawnNodeFromJSON(GetJSONFromFile(devicePath.path()));
                                    node->saved_filepath = fs::relative(devicePath, ConnectatronPath);
                                    node->dirty = false;
                                }
                            }
                        }
                        //TODO maybe cache devices for performance? Would require invalidation to keep supporting editing devices at runtime
                    }
                    if (nothing_shown_yet)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(110, 110, 110, 255));
                        ImGui::Text("(Nothing with a compatible connection)");
                        ImGui::PopStyleColor();
                    }
                }
            }


            // If we actually have created a new node this tick
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
    // Connector type icons
    map<Connector_ID, ImTextureID> connectorIcons;

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