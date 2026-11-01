#include "NPCTemplate.h"

#include "Utils/IniUtils.h"

#include <array>
#include <unordered_map>

namespace NPCTemplate
{
    namespace
    {
        using Flag = RE::ACTOR_BASE_DATA::TEMPLATE_USE_FLAG;
        using IniUtils::ResolveForm;
        using IniUtils::Split;
        using IniUtils::Trim;
        using SKSE::stl::string::icontains;
        using SKSE::stl::string::iequals;

        struct Masks
        {
            std::uint16_t set = 0;
            std::uint16_t clear = 0;
        };

        std::unordered_map<const RE::TESNPC*, Masks> patched;

        constexpr std::string_view FolderPath = "Data/SKSE/Plugins/POE/NPC"sv;

        constexpr std::array<std::pair<std::string_view, Flag>, 12> FlagNames{ {
            { "traits"sv, Flag::kTraits },
            { "stats"sv, Flag::kStats },
            { "factions"sv, Flag::kFactions },
            { "spells"sv, Flag::kSpells },
            { "aidata"sv, Flag::kAIData },
            { "aipackages"sv, Flag::kAIPackages },
            { "basedata"sv, Flag::kBaseData },
            { "inventory"sv, Flag::kInventory },
            { "script"sv, Flag::kScript },
            { "aidefpacklist"sv, Flag::kAIDefPackList },
            { "attackdata"sv, Flag::kAttackData },
            { "keywords"sv, Flag::kKeywords }
        } };

        const Flag* FindFlag(std::string_view a_name)
        {
            for (const auto& [name, flag] : FlagNames) {
                if (iequals(name, a_name)) {
                    return &flag;
                }
            }
            return nullptr;
        }

        bool ParseFlags(std::string_view a_text, std::vector<Flag>& a_flags)
        {
            for (const auto token : Split(a_text, ',')) {
                const auto* flag = FindFlag(token);
                if (!flag) {
                    return false;
                }
                a_flags.push_back(*flag);
            }
            return true;
        }

        // an empty needle never matches
        bool ContainsAll(std::string_view a_editorID, const std::vector<std::string_view>& a_needles)
        {
            for (const auto needle : a_needles) {
                if (!icontains(a_editorID, needle)) {
                    return false;
                }
            }
            return true;
        }

        // flips bits in place only, never recopies from the template
        void Apply(RE::TESNPC* a_npc, bool a_set, const std::vector<Flag>& a_flags)
        {
            auto& masks = patched[a_npc];
            for (const auto flag : a_flags) {
                const auto bit = static_cast<std::uint16_t>(flag);
                const auto inverse = static_cast<std::uint16_t>(~bit);
                if (a_set) {
                    masks.set |= bit;
                    masks.clear &= inverse;
                }
                else {
                    masks.clear |= bit;
                    masks.set &= inverse;
                }
                a_npc->actorData.templateUseFlags.set(a_set, flag);
            }
        }

        // a leveled actor runs on a resolved base, so the same bits are applied to it whenever it loads
        void Propagate(RE::Actor* a_actor)
        {
            const auto* extra = a_actor->extraList.GetByType<RE::ExtraLeveledCreature>();
            auto* base = a_actor->GetActorBase();
            if (!extra || !extra->originalBase || !base) {
                return;
            }

            const auto found = patched.find(extra->originalBase->As<RE::TESNPC>());
            if (found == patched.end()) {
                return;
            }

            const auto flags = base->actorData.templateUseFlags.underlying();
            base->actorData.templateUseFlags = static_cast<Flag>((flags | found->second.set) & ~found->second.clear);
        }

        struct Load3DHook
        {
            static RE::NiAVObject* Thunk(RE::Actor* a_this, bool a_backgroundLoading)
            {
                const auto result = func(a_this, a_backgroundLoading);
                Propagate(a_this);
                return result;
            }

            static inline REL::Relocation<decltype(&Thunk)> func;
        };

        void ApplyByNpcs(std::string_view a_value, bool a_set, const std::vector<Flag>& a_flags)
        {
            for (const auto token : Split(a_value, ',')) {
                auto* form = ResolveForm(token);
                if (!form || !form->Is(RE::FormType::NPC)) {
                    logger::warn("  >Unresolved npc {}"sv, token);
                    continue;
                }

                auto* npc = form->As<RE::TESNPC>();
                if (!npc->baseTemplateForm) {
                    logger::warn("  >Npc {} has no template, skipped"sv, token);
                    continue;
                }

                const auto before = npc->actorData.templateUseFlags.underlying();
                Apply(npc, a_set, a_flags);
                logger::info("  >{:08X} template flags {:04X} -> {:04X}"sv, npc->GetFormID(), before, npc->actorData.templateUseFlags.underlying());
            }
        }

        void ApplyByEditorId(std::string_view a_value, bool a_set, const std::vector<Flag>& a_flags)
        {
            const auto needles = Split(a_value, ',');

            // GetFormEditorID() is empty for npcs, the lookup map is what po3 Tweaks fills
            std::size_t matched = 0;
            const auto& [map, lock] = RE::TESForm::GetAllFormsByEditorID();
            const RE::BSReadLockGuard locker{ lock };
            for (const auto& entry : *map) {
                if (!entry.second->Is(RE::FormType::NPC)) {
                    continue;
                }

                auto* npc = entry.second->As<RE::TESNPC>();
                if (npc->baseTemplateForm && ContainsAll(entry.first.c_str(), needles)) {
                    Apply(npc, a_set, a_flags);
                    ++matched;
                }
            }
            logger::info("  >{} npc(s) matched {}"sv, matched, a_value);
        }

        void ProcessLine(std::string_view a_text)
        {
            const auto line = Trim(a_text);
            if (line.empty() || line.front() == ';') {
                return;
            }

            const auto filterEquals = line.find('=');
            const auto separator = line.find(':');
            const auto opEquals = line.find('=', separator);
            if (filterEquals > separator || opEquals == std::string_view::npos) {
                logger::warn("  >Unsupported line: {}"sv, line);
                return;
            }

            const auto filterKey = Trim(line.substr(0, filterEquals));
            const auto filterValue = line.substr(filterEquals + 1, separator - filterEquals - 1);
            const auto opKey = Trim(line.substr(separator + 1, opEquals - separator - 1));
            const bool set = opKey == "setTemplateFlags"sv;

            std::vector<Flag> flags;
            if ((!set && opKey != "removeTemplateFlags"sv) || !ParseFlags(line.substr(opEquals + 1), flags)) {
                logger::warn("  >Unsupported key or flag: {}"sv, line);
            }
            else if (filterKey == "filterByNpcs"sv) {
                ApplyByNpcs(filterValue, set, flags);
            }
            else if (filterKey == "filterByEditorIdContains"sv) {
                ApplyByEditorId(filterValue, set, flags);
            }
            else {
                logger::warn("  >Unsupported filter {}: {}"sv, filterKey, line);
            }
        }
    }

    void Load()
    {
        logger::info("Applying npc template flags..."sv);
        IniUtils::ForEachLine(FolderPath, &ProcessLine);

        if (!patched.empty()) {
            REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_Character[0] };
            Load3DHook::func = vtbl.write_vfunc(0x6A, &Load3DHook::Thunk);  // Actor::Load3D
            logger::info("  >Installed actor load hook for {} npc(s)"sv, patched.size());
        }
    }
}
