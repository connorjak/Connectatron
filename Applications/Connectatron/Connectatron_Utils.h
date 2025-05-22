#pragma once
#include <string>

//https://stackoverflow.com/questions/201593/is-there-a-simple-way-to-convert-c-enum-to-string/238157#238157
//https://stackoverflow.com/questions/28828957/enum-to-string-in-modern-c11-c14-c17-and-future-c20
#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 1000
#include <magic_enum/magic_enum.hpp>
using std::string;

//https://stackoverflow.com/a/24315631/11502722
static void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
}

static void EnumName_Underscore2Symbol(std::string& underscored)
{
    ReplaceAll(underscored, "____", "/");
    ReplaceAll(underscored, "___", "-");
    ReplaceAll(underscored, "__", " ");
    ReplaceAll(underscored, "_", ".");
}

static void EnumName_Symbol2Underscore(std::string& symboled)
{
    ReplaceAll(symboled, "/", "____");
    ReplaceAll(symboled, "-", "___");
    ReplaceAll(symboled, " ", "__");
    ReplaceAll(symboled, ".", "_");
}

struct CategoryInfo
{
    string name;
    size_t index_of_first;
    size_t index_after_last;
};