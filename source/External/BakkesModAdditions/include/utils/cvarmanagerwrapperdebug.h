#pragma once
#include <array>
#include <memory>
#include <string>
#include <cstdarg>
#include <filesystem>

/// https://fmt.dev/latest/syntax.html
#ifndef FMT_HEADER_ONLY
    #define FMT_HEADER_ONLY
#endif
#pragma warning(push, 0)
#include "fmt/format.h"
#include "fmt/os.h"
#include "fmt/chrono.h"
#include "fmt/ranges.h"
#include "fmt/ostream.h"
#pragma warning(pop)

#pragma warning(push, 0)
#include "bakkesmod/wrappers/cvarmanagerwrapper.h"
#pragma warning(pop)

#define __WIDETEXT(s)	L##s
#define WIDETEXT(s)     __WIDETEXT(s)

#define __STRINGIZE(s)  #s
#define STRINGIZE(s)    __STRINGIZE(s)

typedef unsigned int ImU32;
#ifndef IMGUI_API
    #define COL32(R, G, B, A) ((static_cast<ImU32>(A) << 24) | (static_cast<ImU32>(B) << 16) | (static_cast<ImU32>(G) << 8) | static_cast<ImU32>(R))
#else
    #define COL32 IM_COL32 
#endif

constexpr ImU32 TraceColor    = COL32(0x21, 0x96, 0xF3, 0xFF);
constexpr ImU32 InfoColor     = COL32(0x9E, 0x9E, 0x9E, 0xFF);
constexpr ImU32 WarningColor  = COL32(0xFF, 0x98, 0x00, 0xFF);
constexpr ImU32 ErrorColor    = COL32(0xF4, 0x43, 0x36, 0xFF);
constexpr ImU32 CriticalColor = COL32(0x9C, 0x27, 0xB0, 0xFF);

#undef COL32

static constexpr auto _filename(std::string_view path)
{
    return path.substr(path.find_last_of('\\') + 1);
}

#define __FILENAME__        (_filename(__FILE__))

static constexpr auto _filename(std::wstring_view path)
{
    return path.substr(path.find_last_of(L'\\') + 1);
}

#define __WIDEFILENAME__    (_filename(WIDETEXT(__FILE__)))

extern std::shared_ptr<int> LogLevel;
class CVarManagerWrapperDebug;
extern std::shared_ptr<CVarManagerWrapperDebug> GlobalCVarManager;


class CVarManagerWrapperDebug : public CVarManagerWrapper
{
public:
    enum level_enum
    {
        none        = 0,
        trace       = 1 << 0,
        info        = 1 << 1,
        warning     = 1 << 2,
        error       = 1 << 3,
        critical    = 1 << 4,

        all         = -1,
        normal      = error | critical
    };

private:
    void debug_log(const std::string& text, const int level, [[maybe_unused]] const ImU32 color)
    {
        if (LogLevel && (*LogLevel & level)) {
            log(text/*, color*/);
        }
    }

public:
    template <typename... Args>
    void trace_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format("TRACE: " + text, args...), level_enum::trace, TraceColor);
    }

    template <typename... Args>
    void info_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format("INFO: " + text, args...), level_enum::info, InfoColor);
    }

    template <typename... Args>
    void warning_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format("WARNING: " + text, args...), level_enum::warning, WarningColor);
    }

    template <typename... Args>
    void error_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format("ERROR: " + text, args...), level_enum::error, ErrorColor);
    }

    template <typename... Args>
    void critical_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format("CRITICAL: " + text, args...), level_enum::critical, CriticalColor);
    }
};

#ifdef DEBUG
    static std::string _replace_brackets(const std::string_view& original)
    {
        std::string retval;
        retval.resize(original.size() * 2 + 1);

        size_t i = 0;
        for (const char c : original) {
            retval[i++] = c;
            if (c == '{' || c == '}') {
                retval[i++] = c;
            }
        }
        retval.resize(i);

        return retval;
    }

    #define PREPEND_DEBUG_INFO(...) _replace_brackets(__FILENAME__) + ": " + _replace_brackets(__FUNCTION__) + "(), " STRINGIZE(__LINE__) ": " + __VA_ARGS__
#else
    #define PREPEND_DEBUG_INFO(...) __VA_ARGS__
#endif

#define LOG(...)            GlobalCVarManager->log(fmt::format(PREPEND_DEBUG_INFO(__VA_ARGS__)))
#define TRACE_LOG(...)      GlobalCVarManager->trace_log    (PREPEND_DEBUG_INFO(__VA_ARGS__))
#define INFO_LOG(...)       GlobalCVarManager->info_log     (PREPEND_DEBUG_INFO(__VA_ARGS__))
#define WARNING_LOG(...)    GlobalCVarManager->warning_log  (PREPEND_DEBUG_INFO(__VA_ARGS__))
#define ERROR_LOG(...)      GlobalCVarManager->error_log    (PREPEND_DEBUG_INFO(__VA_ARGS__))
#define CRITICAL_LOG(...)   GlobalCVarManager->critical_log (PREPEND_DEBUG_INFO(__VA_ARGS__))

#ifdef DEBUG
    #define _TRACE_LOG(...)      TRACE_LOG("*" + __VA_ARGS__)
    #define _INFO_LOG(...)       INFO_LOG("*" + __VA_ARGS__)
    #define _WARNING_LOG(...)    WARNING_LOG("*" + __VA_ARGS__)
    #define _ERROR_LOG(...)      ERROR_LOG("*" + __VA_ARGS__)
    #define _CRITICAL_LOG(...)   CRITICAL_LOG("*" + __VA_ARGS__)
#else
    #ifdef _WIN32
        #define __NOOP    __noop
    #else
        #define __NOOP    (void(0))
    #endif
    #define _TRACE_LOG(...)      __NOOP
    #define _INFO_LOG(...)       __NOOP
    #define _WARNING_LOG(...)    __NOOP
    #define _ERROR_LOG(...)      __NOOP
    #define _CRITICAL_LOG(...)   __NOOP
#endif
