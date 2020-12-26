#pragma once
#include <array>
#include <memory>
#include <string>
#include <cstdarg>
#include <filesystem>

#pragma warning(push)
#pragma warning(disable: 4702) // unreachable code
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "fmt/ranges.h"
#pragma warning(pop)

#pragma warning(push, 0)
#include "bakkesmod/wrappers/cvarmanagerwrapper.h"
#pragma warning(pop)

#define __WIDETEXT(quote)   L##quote
#define WIDETEXT(quote)     __WIDETEXTT(quote)

#define STRINGIZE2(s)       #s
#define STRINGIZE(s)        STRINGIZE2(s)

static constexpr auto filename(std::string_view path)
{
    return path.substr(path.find_last_of('\\') + 1);
}

#define __FILENAME__        (filename(__FILE__))

static constexpr auto filename(std::wstring_view path)
{
    return path.substr(path.find_last_of(L'\\') + 1);
}

#define __WIDEFILENAME__    (filename(WIDETEXT(__FILE__)))

extern std::shared_ptr<int> LogLevel;
class CVarManagerWrapperDebug;
extern std::shared_ptr<CVarManagerWrapperDebug> GlobalCVarManager;


class CVarManagerWrapperDebug : public CVarManagerWrapper
{
public:
    enum level_enum
    {
        all         = -1,
        off         = 0,
        trace       = 1 << 0,
        info        = 1 << 1,
        warning     = 1 << 2,
        error       = 1 << 3,
        critical    = 1 << 4
    };

private:
    void debug_log(const std::string& text, const int level)
    {
        if (LogLevel && (*LogLevel & level)) {
            log(text);
        }
    }

public:
    template <typename... Args>
    void trace_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format("TRACE: " + text, args...), level_enum::trace);
    }

    template <typename... Args>
    void info_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format("INFO: " + text, args...), level_enum::info);
    }

    template <typename... Args>
    void warning_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format("WARNING: " + text, args...), level_enum::warning);
    }

    template <typename... Args>
    void error_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format("ERROR: " + text, args...), level_enum::error);
    }

    template <typename... Args>
    void critical_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format("CRITICAL: " + text, args...), level_enum::critical);
    }
};

#define LOG(...)                GlobalCVarManager->log(fmt::format(__VA_ARGS__))
#ifdef DEBUG
    #define TRACE_LOG(...)      GlobalCVarManager->trace_log    (std::string(__FILENAME__) + ": " __FUNCTION__ "(), " STRINGIZE(__LINE__) ": " + __VA_ARGS__)
    #define INFO_LOG(...)       GlobalCVarManager->info_log     (std::string(__FILENAME__) + ": " __FUNCTION__ "(), " STRINGIZE(__LINE__) ": " + __VA_ARGS__)
    #define WARNING_LOG(...)    GlobalCVarManager->warning_log  (std::string(__FILENAME__) + ": " __FUNCTION__ "(), " STRINGIZE(__LINE__) ": " + __VA_ARGS__)
    #define ERROR_LOG(...)      GlobalCVarManager->error_log    (std::string(__FILENAME__) + ": " __FUNCTION__ "(), " STRINGIZE(__LINE__) ": " + __VA_ARGS__)
    #define CRITICAL_LOG(...)   GlobalCVarManager->critical_log (std::string(__FILENAME__) + ": " __FUNCTION__ "(), " STRINGIZE(__LINE__) ": " + __VA_ARGS__)
#else
    #ifdef _WIN32
        #define _NOOP    __noop
    #else
        #define __NOOP    (void(0))
    #endif
    #define TRACE_LOG(...)      _NOOP
    #define INFO_LOG(...)       _NOOP
    #define WARNING_LOG(...)    _NOOP
    #define ERROR_LOG(...)      _NOOP
    #define CRITICAL_LOG(...)   _NOOP
    #undef NOOP
#endif // DEBUG
