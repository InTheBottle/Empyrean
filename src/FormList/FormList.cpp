#include "FormList.h"

#include "Utils/IniUtils.h"

namespace FormList
{
    namespace
    {
        using IniUtils::ResolveForm;
        using IniUtils::Split;
        using IniUtils::Trim;

        constexpr std::string_view FolderPath = "Data/SKSE/Plugins/POE/FormList"sv;

        // a list keeps its own array, so a push here is the same as an entry in the esp and is never saved
        void Add(RE::BGSListForm* a_list, RE::TESForm* a_form)
        {
            a_list->forms.push_back(a_form);
        }

        void Remove(RE::BGSListForm* a_list, const RE::TESForm* a_form)
        {
            auto& forms = a_list->forms;
            for (auto it = forms.begin(); it != forms.end();) {
                if (*it == a_form) {
                    it = forms.erase(it);
                }
                else {
                    ++it;
                }
            }
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
            if (filterEquals > separator || opEquals == std::string_view::npos || Trim(line.substr(0, filterEquals)) != "filterByFormLists"sv) {
                logger::warn("  >Unsupported line: {}"sv, line);
                return;
            }

            const auto opKey = Trim(line.substr(separator + 1, opEquals - separator - 1));
            const bool add = opKey == "formsToAdd"sv;
            if (!add && opKey != "formsToRemove"sv) {
                logger::warn("  >Unsupported key {}: {}"sv, opKey, line);
                return;
            }

            std::vector<RE::BGSListForm*> lists;
            for (const auto token : Split(line.substr(filterEquals + 1, separator - filterEquals - 1), ',')) {
                auto* form = ResolveForm(token);
                if (!form || !form->Is(RE::FormType::FormList)) {
                    logger::warn("  >Unresolved form list {}"sv, token);
                    continue;
                }
                lists.push_back(form->As<RE::BGSListForm>());
            }

            std::vector<std::size_t> before;
            for (const auto* list : lists) {
                before.push_back(list->forms.size());
            }

            for (const auto token : Split(line.substr(opEquals + 1), ',')) {
                auto* form = ResolveForm(token);
                if (!form) {
                    logger::warn("  >Unresolved form {}"sv, token);
                    continue;
                }

                for (auto* list : lists) {
                    if (add) {
                        Add(list, form);
                    }
                    else {
                        Remove(list, form);
                    }
                }
            }

            for (std::size_t i = 0; i < lists.size(); ++i) {
                logger::info("  >{:08X} {} -> {} form(s)"sv, lists[i]->GetFormID(), before[i], lists[i]->forms.size());
            }
        }
    }

    void Load()
    {
        logger::info("Injecting form list entries..."sv);
        IniUtils::ForEachLine(FolderPath, &ProcessLine);
    }
}
