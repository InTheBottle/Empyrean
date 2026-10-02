#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "REX/REX.h"

#include <unordered_set>
#include <fstream>
#include <spdlog/sinks/basic_file_sink.h>

#include "Plugin.h"

#ifdef NDEBUG
#include <spdlog/sinks/basic_file_sink.h>
#else
#include <spdlog/sinks/msvc_sink.h>
#endif

#define DLLEXPORT __declspec(dllexport)

#ifndef NDEBUG
#define LOG_DEBUG(msg, ...) logger::debug(msg, ##__VA_ARGS__)
#else
#define LOG_DEBUG(msg, ...)
#endif

namespace logger = SKSE::log;

using namespace std::literals;
using namespace RE::literals;

template <typename Tag, typename, const REL::RelocationID& ID, std::ptrdiff_t Off>
struct CallHookImpl;

template <typename Tag, typename R, typename... Args, const REL::RelocationID& ID, std::ptrdiff_t Off>
struct CallHookImpl<Tag, R(Args...), ID, Off>
{
    static R func(Args...);
};

namespace util
{
    [[nodiscard]] constexpr int ascii_tolower(int ch) noexcept
    {
        if (ch >= 'A' && ch <= 'Z')
            ch += 'a' - 'A';
        return ch;
    }

    template <auto F>
    struct original_func
    {
        inline static REL::Relocation<decltype(F)> value;
    };

    template <auto F>
    inline static const auto& call_original = original_func<F>::value;

    [[nodiscard]] inline bool is_call_site(std::uintptr_t a_address)
    {
        return REL::make_pattern<"E8">().match(a_address);
    }

    template <typename Tag, typename F, const REL::RelocationID& ID, std::ptrdiff_t Off>
    struct CallHook
    {
        template <std::size_t N, bool TAIL = false>
        static bool write()
        {
            auto hook = REL::Relocation<std::uintptr_t>(ID, Off);
            const bool matched = []()
                {
                    if constexpr (N == 5)
                        if constexpr (!TAIL)
                            return REL::make_pattern<"E8">();
                        else
                            return REL::make_pattern<"E9">();
                    else if constexpr (N == 6)
                        if constexpr (!TAIL)
                            return REL::make_pattern<"FF 15">();
                        else
                            return REL::make_pattern<"FF 25">();
                }()
                    .match(hook.address());

                if (!matched) {
                    logger::error("Call hook for ID {} at offset {:#x} did not find the expected opcode at {:#x}, skipping"sv, ID.id(), Off, hook.address());
                    return false;
                }

                std::uintptr_t addr = 0;
                if constexpr (!TAIL)
                    addr = SKSE::GetTrampoline().write_call<N>(hook.address(), &CallHookImpl<Tag, F, ID, Off>::func);
                else
                    addr = SKSE::GetTrampoline().write_branch<N>(hook.address(), &CallHookImpl<Tag, F, ID, Off>::func);

                if constexpr (N == 6)
                    addr = *reinterpret_cast<std::uintptr_t*>(addr);

                original_func<&CallHookImpl<Tag, F, ID, Off>::func>::value = addr;

                return true;
        }

        static bool write5() { return write<5, false>(); }
        static bool write5_tail() { return write<5, true>(); }
        static bool write6() { return write<6, false>(); }
        static bool write6_tail() { return write<6, true>(); }
    };

    struct iless
    {
        using is_transparent = int;

        template <std::ranges::contiguous_range S1, std::ranges::contiguous_range S2>
            requires(
        std::is_same_v<std::ranges::range_value_t<S1>, char>&&
            std::is_same_v<std::ranges::range_value_t<S2>, char>)
            constexpr bool operator()(S1&& a_str1, S2&& a_str2) const
        {
            std::size_t count = std::ranges::size(a_str2);
            const std::size_t len1 = std::ranges::size(a_str1);
            const bool shorter = len1 < count;
            if (shorter)
                count = len1;

            if (count) {
                const char* p1 = std::ranges::data(a_str1);
                const char* p2 = std::ranges::data(a_str2);

                do {
                    const int ch1 = ascii_tolower(*p1++);
                    const int ch2 = ascii_tolower(*p2++);
                    if (ch1 != ch2)
                        return ch1 < ch2;
                } while (--count);
            }

            return shorter;
        }
    };

    using SKSE::stl::report_and_fail;

    template <class T>
    using istring_map = std::map<std::string, T, iless>;
}

namespace stl {
    template <typename R, typename... Args>
    inline std::uintptr_t function_ptr(R(*fn)(Args...))
    {
        return reinterpret_cast<std::uintptr_t>(fn);
    }

    template <typename Class, typename R, typename... Args>
    inline std::uintptr_t function_ptr(R(Class::* fn)(Args...))
    {
        return reinterpret_cast<std::uintptr_t>((void*&)fn);
    }
}

// Used as a compile guard in certain templated function (see INISettings.h, if present)
template <class T>
inline constexpr bool always_false = false;

#define SECTION_SEPARATOR logger::info("=========================================================="sv)