#include "IniUtils.h"

#include <charconv>
#include <chrono>
#include <filesystem>

namespace IniUtils
{
    namespace
    {
        using SKSE::stl::string::iequals;

        bool IsFileActive(const std::filesystem::path& a_path)
        {
            const auto stem = a_path.stem();
            const auto extension = stem.extension().string();
            if (!iequals(extension, ".esp"sv) && !iequals(extension, ".esl"sv) && !iequals(extension, ".esm"sv)) {
                return true;
            }
            return RE::TESDataHandler::GetSingleton()->LookupModByName(stem.string()) != nullptr;
        }
    }

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

    RE::TESForm* ResolveForm(std::string_view a_token)
    {
        const auto delimiter = a_token.find('|');
        if (delimiter == std::string_view::npos) {
            return RE::TESForm::LookupByEditorID(std::string(a_token));  // the key is read as a c string, so it needs its own terminator
        }

        const auto dataHandler = RE::TESDataHandler::GetSingleton();
        const auto modName = a_token.substr(0, delimiter);
        const auto id = a_token.substr(delimiter + 1);
        const auto file = dataHandler->LookupModByName(modName);
        RE::FormID rawID = 0;
        const auto [pointer, error] = std::from_chars(id.data(), id.data() + id.size(), rawID, 16);
        if (!file || error != std::errc{} || pointer != id.data() + id.size()) {
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

    void ForEachLine(std::string_view a_folder, void (*a_callback)(std::string_view))
    {
        const auto start = std::chrono::steady_clock::now();

        std::error_code error;
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::path{ a_folder }, error)) {
            if (entry.is_regular_file() && entry.path().extension() == ".ini" && IsFileActive(entry.path())) {
                files.push_back(entry.path());
            }
        }
        std::ranges::sort(files);

        for (const auto& path : files) {
            logger::info("  >Reading {}"sv, path.filename().string());
            std::ifstream file(path);
            std::string line;
            while (std::getline(file, line)) {
                a_callback(line);
            }
        }

        logger::info("  >Finished in {:.2f} ms"sv, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
    }
}