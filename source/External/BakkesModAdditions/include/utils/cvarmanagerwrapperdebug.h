#pragma once
#include <array>
#include <memory>
#include <string>
#include <cstdarg>
#include <filesystem>
#include <utility>

/// https://fmt.dev/latest/syntax.html
#ifndef FMT_HEADER_ONLY
    #define FMT_HEADER_ONLY
#endif
#pragma warning(push, 0)
#include "fmt/format.h"
#include "fmt/ostream.h"
#include "fmt/color.h"
#pragma warning(pop)

#pragma warning(push, 0)
#include "bakkesmod/wrappers/cvarmanagerwrapper.h"
#pragma warning(pop)


//#include "threading.h"
static bool is_game_thread();
static bool is_render_thread();

#ifndef STRINGIZE
    #define STRINGIZE2(s)   #s
    #define STRINGIZE(s)    STRINGIZE2(s)
#endif

#if defined(BMDEBUG) && defined(DEBUG)
    #error Cannot define BMDEBUG and DEBUG.
#endif

#if defined(BMDEBUG)
    #define DEBUG
#endif


constexpr fmt::rgb TraceColor    = fmt::rgb(0x21, 0x96, 0xF3);
constexpr fmt::rgb InfoColor     = fmt::rgb(0x9E, 0x9E, 0x9E);
constexpr fmt::rgb WarningColor  = fmt::rgb(0xFF, 0x98, 0x00);
constexpr fmt::rgb ErrorColor    = fmt::rgb(0xF4, 0x43, 0x36);
constexpr fmt::rgb CriticalColor = fmt::rgb(0x9C, 0x27, 0xB0);

static constexpr std::string_view filename_(const std::string_view& path)
{
    return path.substr(path.find_last_of('\\') + 1);
}

#ifndef PRETTY_FILENAME
    #define PRETTY_FILENAME     (filename_(__FILE__))
#endif

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
        normal      = info | error | critical
    };

private:
    std::string get_thread() const
    {
#ifdef DEBUG
        if (is_game_thread()) {
            return "Game Thread: ";
        }
        if (is_render_thread()) {
            return "Game Thread: ";
        }
        return fmt::format("Thread #{:s}: ", std::this_thread::get_id());
#else
        return "";
#endif
    }

    void debug_log(const std::string& text, const int level)
    {
        if (LogLevel && (*LogLevel & level)) {
            log(get_thread() + text);
        }
    }

public:
    template <typename... Args>
    void trace_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format(fg(TraceColor), "TRACE: " + text, args...), level_enum::trace);
    }

    template <typename... Args>
    void info_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format(fg(InfoColor), "INFO: " +  text, args...), level_enum::info);
    }

    template <typename... Args>
    void warning_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format(fg(WarningColor), "WARNING: " +  text, args...), level_enum::warning);
    }

    template <typename... Args>
    void error_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format(fg(ErrorColor), "ERROR: " +  text, args...), level_enum::error);
    }

    template <typename... Args>
    void critical_log(const std::string& text, Args&&... args)
    {
        debug_log(fmt::format(fg(CriticalColor), "CRITICAL: " +  text, args...), level_enum::critical);
    }

    static constexpr std::string replace_brackets(const std::string_view& original)
    {
        std::string str;
        str.reserve(original.size() * 2 + 1);

        for (const char c : original) {
            str += c;
            if (c == '{' || c == '}') {
                str += c;
            }
        }
        str.shrink_to_fit();

        return str;
    }
};

#ifdef DEBUG
    #define PREPEND_DEBUG_INFO(...) CVarManagerWrapperDebug::replace_brackets(PRETTY_FILENAME) + ": " + CVarManagerWrapperDebug::replace_brackets(__FUNCTION__) + "(), " STRINGIZE(__LINE__) ": " + __VA_ARGS__
    //#define PREPEND_DEBUG_INFO(...) CVarManagerWrapperDebug::replace_brackets(PRETTY_FILENAME) + ": " + CVarManagerWrapperDebug::replace_brackets(__PRETTY_FUNCTION__) + "(), " STRINGIZE(__LINE__) ": " + __VA_ARGS__
#else
    #define PREPEND_DEBUG_INFO(...) __VA_ARGS__
#endif

#define BM_LOG(...)            GlobalCVarManager->log(fmt::format(PREPEND_DEBUG_INFO(__VA_ARGS__)))
#define BM_TRACE_LOG(...)      GlobalCVarManager->trace_log    (PREPEND_DEBUG_INFO(__VA_ARGS__))
#define BM_INFO_LOG(...)       GlobalCVarManager->info_log     (PREPEND_DEBUG_INFO(__VA_ARGS__))
#define BM_WARNING_LOG(...)    GlobalCVarManager->warning_log  (PREPEND_DEBUG_INFO(__VA_ARGS__))
#define BM_ERROR_LOG(...)      GlobalCVarManager->error_log    (PREPEND_DEBUG_INFO(__VA_ARGS__))
#define BM_CRITICAL_LOG(...)   GlobalCVarManager->critical_log (PREPEND_DEBUG_INFO(__VA_ARGS__))

#ifdef DEBUG
    #define BM_DEBUG_TRACE_LOG(...)     BM_TRACE_LOG(std::string("*") + __VA_ARGS__)
    #define BM_DEBUG_INFO_LOG(...)      BM_INFO_LOG(std::string("*") + __VA_ARGS__)
    #define BM_DEBUG_WARNING_LOG(...)   BM_WARNING_LOG(std::string("*") + __VA_ARGS__)
    #define BM_DEBUG_ERROR_LOG(...)     BM_ERROR_LOG(std::string("*") + __VA_ARGS__)
    #define BM_DEBUG_CRITICAL_LOG(...)  BM_CRITICAL_LOG(std::string("*") + __VA_ARGS__)
#else
    #ifdef _WIN32
        #define NOOP_   __noop
    #else
        #define NOOP_   (void(0))
    #endif
    #define BM_DEBUG_TRACE_LOG(...)     NOOP_
    #define BM_DEBUG_INFO_LOG(...)      NOOP_
    #define BM_DEBUG_WARNING_LOG(...)   NOOP_
    #define BM_DEBUG_ERROR_LOG(...)     NOOP_
    #define BM_DEBUG_CRITICAL_LOG(...)  NOOP_
#endif


#define EXPAND_(x) x

#define GET_3TH_ARG_(arg1, arg2, arg3, ...) arg3
#define GET_4TH_ARG_(arg1, arg2, arg3, arg4, ...) arg4

#define CHECK_VALUE(value_check, log, ret)  \
    if (value_check) {                      \
        log;                                \
        ret;                                \
    }

#define CHECK_IF_VALUES_ARE_EQUAL(lvalue, rvalue)   \
    CHECK_VALUE((lvalue) != (rvalue), BM_ERROR_LOG(#lvalue " == " #rvalue " failed: {}, != {}", (lvalue), (rvalue)), )

#define CHECK_IF_VALUE_IS_NULLPTR_LOG(value, logger, ret)   \
    CHECK_VALUE((value) == nullptr, logger("could not get the " #value), ret)

#define CHECK_IF_VALUE_IS_NULLPTR(value, ret)   \
    CHECK_VALUE((value) == nullptr, , ret)

#define CHECK_IF_BM_VALUE_IS_NULL_LOG(value, logger, ret)   \
    CHECK_VALUE((value).IsNull(), logger("could not get the " #value), ret)

#define CHECK_IF_BM_VALUE_IS_NULL(value, ret)   \
    CHECK_VALUE((value).IsNull(), , ret)

#define CHECK_IF_TARRAY_INDEX_IS_VALID_LOG(arr, idx, logger, ret)   \
    CHECK_VALUE(!(arr).IsValid() || !(arr).IsValidIndex(idx), logger("could not get the " #arr), ret)

#define CHECK_IF_TARRAY_INDEX_IS_VALID(arr, idx, ret)   \
    CHECK_VALUE(!(arr).IsValid() || !(arr).IsValidIndex(idx), , ret)

#define CHECK_IF_BM_ARRAY_INDEX_IS_VALID_LOG(arr, idx, logger, ret) \
    CHECK_VALUE((arr).IsNull() || (arr).Count() <= (idx), logger("could not get the " #arr), ret)

#define CHECK_IF_BM_ARRAY_INDEX_IS_VALID(arr, idx, ret) \
    CHECK_VALUE((arr).IsNull() || (arr).Count() <= (idx), , ret)


/* NULLCHECK */

#define NULLCHECK_WITHOUT_RETURN(value) \
    CHECK_IF_VALUE_IS_NULLPTR_LOG(value, BM_DEBUG_ERROR_LOG, return)

#define NULLCHECK_WITH_RETURN(value, ret)   \
    CHECK_IF_VALUE_IS_NULLPTR_LOG(value, BM_DEBUG_ERROR_LOG, return (ret))

#define NULLCHECK(...)  \
    EXPAND_(GET_3TH_ARG_(__VA_ARGS__, NULLCHECK_WITH_RETURN, NULLCHECK_WITHOUT_RETURN)(__VA_ARGS__))

#define NULLCHECK_SILENT_WITHOUT_RETURN(value)  \
    CHECK_IF_VALUE_IS_NULLPTR(value, return)

#define NULLCHECK_SILENT_WITH_RETURN(value, ret)    \
    CHECK_IF_VALUE_IS_NULLPTR(value, return (ret))

#define NULLCHECK_SILENT(...)  \
    EXPAND_(GET_3TH_ARG_(__VA_ARGS__, NULLCHECK_SILENT_WITH_RETURN, NULLCHECK_SILENT_WITHOUT_RETURN)(__VA_ARGS__))

#define NULLCHECK_BREAK(value) \
    CHECK_IF_VALUE_IS_NULLPTR_LOG(value, BM_DEBUG_WARNING_LOG, break)

#define NULLCHECK_BREAK_SILENT(value) \
    CHECK_IF_VALUE_IS_NULLPTR(value, break)


/* BMCHECK */

#define BMCHECK_WITHOUT_RETURN(value)   \
    CHECK_IF_BM_VALUE_IS_NULL_LOG(value, BM_ERROR_LOG, return)

#define BMCHECK_WITH_RETURN(value, ret) \
    CHECK_IF_BM_VALUE_IS_NULL_LOG(value, BM_ERROR_LOG, return (ret))

#define BMCHECK(...)    \
    EXPAND_(GET_3TH_ARG_(__VA_ARGS__, BMCHECK_WITH_RETURN, BMCHECK_WITHOUT_RETURN)(__VA_ARGS__))

#define BMCHECK_SILENT_WITHOUT_RETURN(value)    \
    CHECK_IF_BM_VALUE_IS_NULL(value, return)

#define BMCHECK_SILENT_WITH_RETURN(value, ret)  \
    CHECK_IF_BM_VALUE_IS_NULL(value, return (ret))

#define BMCHECK_SILENT(...) \
    EXPAND_(GET_3TH_ARG_(__VA_ARGS__, BMCHECK_SILENT_WITH_RETURN, BMCHECK_SILENT_WITHOUT_RETURN)(__VA_ARGS__))

#define BMCHECK_DEBUG_WITHOUT_RETURN(value)  \
    CHECK_IF_BM_VALUE_IS_NULL_LOG(value, BM_DEBUG_ERROR_LOG, return)

#define BMCHECK_DEBUG_WITH_RETURN(value, ret)    \
    CHECK_IF_BM_VALUE_IS_NULL_LOG(value, BM_DEBUG_ERROR_LOG, return (ret))

#define BMCHECK_DEBUG(...)   \
    EXPAND_(GET_3TH_ARG_(__VA_ARGS__, BMCHECK_DEBUG_WITH_RETURN, BMCHECK_DEBUG_WITHOUT_RETURN)(__VA_ARGS__))

#define BMCHECK_BREAK(value) \
    CHECK_IF_BM_VALUE_IS_NULL_LOG(value, BM_DEBUG_WARNING_LOG, break)

#define BMCHECK_BREAK_SILENT(value) \
    CHECK_IF_BM_VALUE_IS_NULL(value, break)


/* NULLCHECK_LOOP */

#define NULLCHECK_LOOP(value)   \
    CHECK_IF_VALUE_IS_NULLPTR_LOG(value, BM_DEBUG_WARNING_LOG, continue)

#define NULLCHECK_LOOP_SILENT(value)    \
    CHECK_IF_VALUE_IS_NULLPTR(value, continue)


/* BMCHECK_LOOP */

#define BMCHECK_LOOP(value) \
    CHECK_IF_BM_VALUE_IS_NULL_LOG(value, BM_WARNING_LOG, continue)

#define BMCHECK_LOOP_SILENT(value)  \
    CHECK_IF_BM_VALUE_IS_NULL(value, continue)


/* NULLCHECK_TARRAY */

#define NULLCHECK_TARRAY_WITHOUT_INDEX(arr) \
    CHECK_IF_TARRAY_INDEX_IS_VALID_LOG(arr, 0, BM_DEBUG_WARNING_LOG, return)

#define NULLCHECK_TARRAY_WITHOUT_RETURN(arr, idx)   \
    CHECK_IF_TARRAY_INDEX_IS_VALID_LOG(arr, idx, BM_DEBUG_WARNING_LOG, return)

#define NULLCHECK_TARRAY_WITH_RETURN(arr, idx, ret) \
    CHECK_IF_TARRAY_INDEX_IS_VALID_LOG(arr, idx, BM_DEBUG_WARNING_LOG, return (ret))

#define NULLCHECK_TARRAY(...)   \
    EXPAND_(GET_4TH_ARG_(__VA_ARGS__, NULLCHECK_TARRAY_WITH_RETURN, NULLCHECK_TARRAY_WITHOUT_RETURN, NULLCHECK_TARRAY_WITHOUT_INDEX)(__VA_ARGS__))

#define NULLCHECK_TARRAY_SILENT_WITHOUT_INDEX(arr, idx) \
    CHECK_IF_TARRAY_INDEX_IS_VALID(arr, 0, return)

#define NULLCHECK_TARRAY_SILENT_WITHOUT_RETURN(arr, idx)    \
    CHECK_IF_TARRAY_INDEX_IS_VALID(arr, idx, return)

#define NULLCHECK_TARRAY_SILENT_WITH_RETURN(arr, idx, ret)  \
    CHECK_IF_TARRAY_INDEX_IS_VALID(arr, idx, return (ret))

#define NULLCHECK_TARRAY_SILENT(...)    \
    EXPAND_(GET_4TH_ARG_(__VA_ARGS__, NULLCHECK_TARRAY_SILENT_WITH_RETURN, NULLCHECK_TARRAY_SILENT_WITHOUT_RETURN, NULLCHECK_TARRAY_SILENT_WITHOUT_INDEX)(__VA_ARGS__))


/* BMCHECK_ARRAY */

#define BMCHECK_ARRAY_WITHOUT_INDEX(arr)    \
    CHECK_IF_BM_ARRAY_INDEX_IS_VALID_LOG(arr, 0, BM_WARNING_LOG, return)

#define BMCHECK_ARRAY_WITHOUT_RETURN(arr, idx)  \
    CHECK_IF_BM_ARRAY_INDEX_IS_VALID_LOG(arr, idx, BM_WARNING_LOG, return)

#define BMCHECK_ARRAY_WITH_RETURN(arr, idx, ret)    \
    CHECK_IF_BM_ARRAY_INDEX_IS_VALID_LOG(arr, idx, BM_WARNING_LOG, return (ret))

#define BMCHECK_ARRAY(...)  \
    EXPAND_(GET_4TH_ARG_(__VA_ARGS__, BMCHECK_ARRAY_WITH_RETURN, BMCHECK_ARRAY_WITHOUT_RETURN, BMCHECK_ARRAY_WITHOUT_INDEX)(__VA_ARGS__))

#define BMCHECK_ARRAY_SILENT_WITHOUT_INDEX(arr) \
    CHECK_IF_BM_ARRAY_INDEX_IS_VALID(arr, 0, return)

#define BMCHECK_ARRAY_SILENT_WITHOUT_RETURN(arr, idx)   \
    CHECK_IF_BM_ARRAY_INDEX_IS_VALID(arr, idx, return)

#define BMCHECK_ARRAY_SILENT_WITH_RETURN(arr, idx, ret) \
    CHECK_IF_BM_ARRAY_INDEX_IS_VALID(arr, idx, return (ret))

#define BMCHECK_ARRAY_SILENT(...)   \
    EXPAND_(GET_4TH_ARG_(__VA_ARGS__, BMCHECK_ARRAY_SILENT_WITH_RETURN, BMCHECK_ARRAY_SILENT_WITHOUT_RETURN, BMCHECK_ARRAY_SILENT_WITHOUT_INDEX)(__VA_ARGS__))

#ifdef BMDEBUG
    #undef DEBUG
#endif
