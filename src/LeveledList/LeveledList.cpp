#include "LeveledList.h"

#include <charconv>
#include <filesystem>

namespace LeveledList
{
    namespace
    {
        using AddEntry_t = RE::LEVELED_OBJECT* (RE::TESLeveledList*, std::uint16_t, std::uint16_t, RE::TESForm*, RE::ContainerItemExtra*);

        enum class Op
        {
            kAdd,
            kAddOnce,
            kRemove
        };

        constexpr std::string_view FolderPath = "Data/SKSE/Plugins/POE/LeveledList"sv;
        constexpr std::string_view FilterKey = "filterByLLs="sv;
        constexpr std::uint8_t MaxEntries = 255; //Steel you can control the max entries here (Engine max is 255)

        std::string_view Trim(std::string_view a_text)
        {
            while (!a_text.empty() && std::isspace(static_cast<unsigned char>(a_text.front()))) {
                a_text.remove_prefix(1);
            }
            while (!a_text.empty() && std::isspace(static_cast<unsigned char>(a_text.back()))) {
                a_text.remove_suffix(1);
            }
            return a_text;
        }

        std::vector<std::string_view> Split(std::string_view a_text, char a_delimiter)
        {
            std::vector<std::string_view> parts;
            while (true) {
                const auto position = a_text.find(a_delimiter);
                parts.push_back(Trim(a_text.substr(0, position)));
                if (position == std::string_view::npos) {
                    break;
                }
                a_text.remove_prefix(position + 1);
            }
            return parts;
        }

        template <class T>
        bool Parse(std::string_view a_text, T& a_value, int a_base = 10)
        {
            const auto end = a_text.data() + a_text.size();
            const auto [pointer, error] = std::from_chars(a_text.data(), end, a_value, a_base);
            return error == std::errc{} && pointer == end;
        }

        bool ParseEntry(Op a_op, const std::vector<std::string_view>& a_fields, std::uint16_t& a_level, std::uint16_t& a_count)
        {
            if (a_op == Op::kRemove) {
                return a_fields.size() == 1;
            }
            if (a_fields.size() > 3) {
                return false;
            }
            if (a_fields.size() > 1 && !Parse(a_fields[1], a_level)) {
                return false;
            }
            if (a_fields.size() > 2 && !Parse(a_fields[2], a_count)) {
                return false;
            }
            return true;
        }

        RE::TESForm* ResolveForm(std::string_view a_token)
        {
            const auto delimiter = a_token.find('|');
            if (delimiter == std::string_view::npos) {
                return RE::TESForm::LookupByEditorID(a_token);
            }

            const auto dataHandler = RE::TESDataHandler::GetSingleton();
            const auto modName = a_token.substr(0, delimiter);
            const auto file = dataHandler->LookupModByName(modName);
            RE::FormID rawID = 0;
            if (!file || !Parse(a_token.substr(delimiter + 1), rawID, 16)) {
                return nullptr;
            }

            if (file->IsLight()) {
                rawID &= 0xFFF;
            }
            else {
                rawID &= 0xFFFFFF;
            }
            return dataHandler->LookupForm(rawID, modName);
        }

        bool IsFileActive(const std::filesystem::path& a_path)
        {
            const auto stem = a_path.stem();
            auto extension = stem.extension().string();
            for (auto& character : extension) {
                character = static_cast<char>(util::ascii_tolower(character));
            }
            if (extension != ".esp" && extension != ".esl" && extension != ".esm") {
                return true;
            }
            return RE::TESDataHandler::GetSingleton()->LookupModByName(stem.string()) != nullptr;
        }

        bool Contains(const RE::TESLevItem* a_list, const RE::TESForm* a_form)
        {
            const auto* entries = a_list->entries.data();
            for (std::uint8_t i = 0; i < a_list->numEntries; ++i) {
                if (entries[i].form == a_form) {
                    return true;
                }
            }
            return false;
        }

        void Add(RE::TESLevItem* a_list, RE::TESForm* a_form, std::uint16_t a_level, std::uint16_t a_count)
        {
            if (!a_list->GetCanContainFormsOfType(a_form->GetFormType()) || a_list->numEntries >= MaxEntries) {
                logger::warn("  >List {:08X} cannot take form {:08X}, skipped."sv, a_list->GetFormID(), a_form->GetFormID());
                return;
            }

            static REL::Relocation<AddEntry_t> addEntry{ RELOCATION_ID(14577, 14749) };  // TESLeveledList::AddEntry
            addEntry(a_list, a_level, a_count, a_form, nullptr);
        }

        void Remove(RE::TESLevItem* a_list, const RE::TESForm* a_form)
        {
            if (a_list->unk13 != 0) {
                logger::warn("  >List {:08X} has script added entries, removal of {:08X} skipped."sv, a_list->GetFormID(), a_form->GetFormID());
                return;
            }

            auto* entries = a_list->entries.data();
            const auto count = a_list->numEntries;
            std::uint8_t kept = 0;
            for (std::uint8_t i = 0; i < count; ++i) {
                const bool sameForm = entries[i].form == a_form;
                if (sameForm && !entries[i].itemExtra) {
                    continue;
                }
                if (sameForm) {
                    logger::warn("  >List {:08X} entry {:08X} has extra data, kept."sv, a_list->GetFormID(), a_form->GetFormID());
                }
                if (kept != i) {
                    entries[kept] = entries[i];
                }
                ++kept;
            }
            for (std::uint8_t i = kept; i < count; ++i) {
                entries[i] = RE::LEVELED_OBJECT{};
            }
            a_list->numEntries = kept;
        }

        void ProcessLine(std::string_view a_text)
        {
            const auto line = Trim(a_text);
            if (line.empty() || line.front() == ';') {
                return;
            }

            const auto separator = line.find(':');
            const auto equals = line.find('=', separator);
            if (!line.starts_with(FilterKey) || separator == std::string_view::npos || equals == std::string_view::npos) {
                logger::warn("  >Unsupported line: {}"sv, line);
                return;
            }

            const auto key = Trim(line.substr(separator + 1, equals - separator - 1));
            Op op;
            if (key == "addToLLs"sv) {
                op = Op::kAdd;
            }
            else if (key == "addOnceToLLs"sv) {
                op = Op::kAddOnce;
            }
            else if (key == "removeFromLLs"sv) {
                op = Op::kRemove;
            }
            else {
                logger::warn("  >Unsupported key {}: {}"sv, key, line);
                return;
            }

            std::vector<RE::TESLevItem*> lists;
            for (const auto token : Split(line.substr(FilterKey.size(), separator - FilterKey.size()), ',')) {
                auto* form = ResolveForm(token);
                if (!form || !form->Is(RE::FormType::LeveledItem)) {
                    logger::warn("  >Unresolved leveled list {}"sv, token);
                    continue;
                }
                lists.push_back(form->As<RE::TESLevItem>());
            }

            for (const auto entryText : Split(line.substr(equals + 1), ',')) {
                const auto fields = Split(entryText, '~');
                auto* form = ResolveForm(fields[0]);
                if (!form) {
                    logger::warn("  >Unresolved form {}"sv, fields[0]);
                    continue;
                }

                std::uint16_t level = 1;
                std::uint16_t count = 1;
                if (!ParseEntry(op, fields, level, count)) {
                    logger::warn("  >Unsupported entry {}"sv, entryText);
                    continue;
                }

                for (auto* list : lists) {
                    if (op == Op::kRemove) {
                        Remove(list, form);
                    }
                    else if (op == Op::kAddOnce) {
                        if (!Contains(list, form)) {
                            Add(list, form, level, count);
                        }
                    }
                    else {
                        Add(list, form, level, count);
                    }
                }
            }
        }

        void ProcessFile(const std::filesystem::path& a_path)
        {
            logger::info("  >Reading {}"sv, a_path.filename().string());
            std::ifstream file(a_path);
            std::string line;
            while (std::getline(file, line)) {
                ProcessLine(line);
            }
        }
    }

    void Load()
    {
        logger::info("Injecting leveled list entries..."sv);

        std::error_code error;
        const std::filesystem::path folder{ FolderPath };
        if (!std::filesystem::is_directory(folder, error)) {
            logger::info("  >No leveled list folder found."sv);
            return;
        }

        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(folder, error)) {
            if (entry.is_regular_file() && entry.path().extension() == ".ini") {
                files.push_back(entry.path());
            }
        }
        std::ranges::sort(files);

        for (const auto& path : files) {
            if (IsFileActive(path)) {
                ProcessFile(path);
            }
            else {
                logger::info("  >Skipped {}, plugin not loaded."sv, path.filename().string());
            }
        }
    }
}