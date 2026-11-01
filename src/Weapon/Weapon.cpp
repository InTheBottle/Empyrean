#include "Weapon.h"

#include "Utils/IniUtils.h"

namespace Weapon
{
    namespace
    {
        using IniUtils::ResolveForm;
        using IniUtils::Split;
        using IniUtils::Trim;

        constexpr std::string_view FolderPath = "Data/SKSE/Plugins/POE/Weapon"sv;

        void ProcessLine(std::string_view a_text)
        {
            const auto line = Trim(a_text);
            if (line.empty() || line.front() == ';') {
                return;
            }

            const auto filterEquals = line.find('=');
            const auto separator = line.find(':');
            const auto opEquals = line.find('=', separator);
            if (filterEquals > separator || opEquals == std::string_view::npos || Trim(line.substr(0, filterEquals)) != "filterByWeapons"sv) {
                logger::warn("  >Unsupported line: {}"sv, line);
                return;
            }

            const auto opKey = Trim(line.substr(separator + 1, opEquals - separator - 1));
            const auto value = Trim(line.substr(opEquals + 1));
            if (opKey != "fullName"sv || value.size() < 2 || value.front() != '~' || value.back() != '~') {
                logger::warn("  >Unsupported key or value: {}"sv, line);
                return;
            }

            const std::string name{ value.substr(1, value.size() - 2) };  // SetFullName reads a c string
            for (const auto token : Split(line.substr(filterEquals + 1, separator - filterEquals - 1), ',')) {
                auto* form = ResolveForm(token);
                if (!form || !form->Is(RE::FormType::Weapon)) {
                    logger::warn("  >Unresolved weapon {}"sv, token);
                    continue;
                }

                // the engine never saves a weapon's name, so this lasts until the game closes
                auto* weapon = form->As<RE::TESObjectWEAP>();
                const std::string before{ weapon->GetName() };
                weapon->SetFullName(name.c_str());
                logger::info("  >{:08X} {} -> {}"sv, weapon->GetFormID(), before, name);
            }
        }
    }

    void Load()
    {
        logger::info("Renaming weapons..."sv);
        IniUtils::ForEachLine(FolderPath, &ProcessLine);
    }
}
