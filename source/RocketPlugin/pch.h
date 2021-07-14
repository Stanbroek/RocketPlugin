/// pch.h: This is a precompiled header file.
/// Files listed below are compiled only once, improving build performance for future builds.
/// This also affects IntelliSense performance, including code completion and many code browsing features.
/// However, files listed here are ALL re-compiled if any one of them is updated between builds.
/// Do not add files here that you will be updating frequently as this negates the performance advantage.

#pragma once
#ifndef PCH_H
#define PCH_H

/// add headers that you want to pre-compile here
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#define NOMINMAX

// Windows Headers
#include <Windows.h>
#include <condition_variable>
#include <unordered_map>
#include <system_error>
#include <string_view>
#include <filesystem>
#include <functional>
#include <fstream>
#include <utility>
#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <random>
#include <vector>
#include <mutex>
#include <queue>
#include <regex>
#include <map>

// BakkesMod SDK
#pragma comment(lib, "pluginsdk.lib")
#pragma warning(push, 0)
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"

#include "bakkesmod/wrappers/Engine/WorldInfoWrapper.h"
#include "bakkesmod/wrappers/http/HttpWrapper.h"
#include "utils/parser.h"
#include "utils/io.h"
#pragma warning(pop)

// BakkesMod SDK Additions
#include "utils/cvarmanagerwrapperdebug.h"
#include "utils/exception_safety.h"
#include "utils/parser_w.h"

enum {
    PLUGINTYPE_ALL = 0x00
};

// General Utils
#include "utils/xorstr.h"
#include "utils/stringify.h"
#include "utils/threading.h"
#include "utils/exception_safety.h"

// Dear ImGui
#define IM_ASSERT(_EXPR)    __noop
#include "ImGui/imgui.h"
#include "ImGui/imgui_searchablecombo.h"
#include "ImGui/imgui_rangeslider.h"
#include "ImGui/imgui_additions.h"

// SIMDJson
#pragma warning(push, 0)
#include "simdjson.h"
#pragma warning(pop)

// FMT
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "fmt/ostream.h"
#include "fmt/chrono.h"
#include "fmt/ranges.h"
#include "fmt/os.h"

#endif //PCH_H
