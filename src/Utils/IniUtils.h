#pragma once

#include "SKSE/Impl/Util.h"

namespace IniUtils
{
    std::string_view Trim(std::string_view a_text);
    std::vector<std::string_view> Split(std::string_view a_text, char a_delimiter);

    // editor id or plugin|hex form id
    RE::TESForm* ResolveForm(std::string_view a_token);

    // every line of every active ini under the folder, sorted by path
    void ForEachLine(std::string_view a_folder, void (*a_callback)(std::string_view));
}
