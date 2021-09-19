// RocketPlugin.cpp
// A BakkesMod plugin for joining, hosting and manipulating multiplayer games.
//
// Author:        Stanbroek
// Version:       0.6.8 18/09/21
// BMSDK version: 95
#include "RPConfig.h"
#include "RocketPlugin.h"

// Game modes
#include "GameModes/Drainage.h"
#include "GameModes/CrazyRumble.h"
#include "GameModes/Zombies.h"
#include "GameModes/BoostSteal.h"
#include "GameModes/KeepAway.h"
#include "GameModes/Tag.h"
#include "GameModes/Juggernaut.h"
#include "GameModes/BoostMod.h"
#include "GameModes/BoostShare.h"
#include "GameModes/SacredGround.h"
#include "GameModes/SmallCars.h"

BAKKESMOD_PLUGIN(RocketPlugin, "Rocket Plugin", PLUGIN_VERSION, PLUGINTYPE_ALL)

std::filesystem::path BakkesModConfigFolder;
std::filesystem::path BakkesModCrashesFolder;
std::filesystem::path RocketPluginDataFolder;
std::filesystem::path RocketLeagueExecutableFolder;

std::thread::id GameThreadId;
std::thread::id RenderThreadId;

std::shared_ptr<int> LogLevel;
std::shared_ptr<CVarManagerWrapperDebug> GlobalCVarManager;


/*
 *  Map File Helpers
 */

/// <summary>Gets workshop maps from the given directory.</summary>
/// <param name="workshopPath">Path to the workshop directory to get the maps from</param>
/// <param name="extensions">List of map extensions to filter by</param>
/// <param name="preferredExtension">Map extension to prefer when multiple files are found</param>
/// <returns>The workshop maps from the given directory</returns>
std::vector<std::filesystem::path> RocketPlugin::getWorkshopMaps(const std::filesystem::path& workshopPath,
    const std::vector<std::string>& extensions, const std::string& preferredExtension)
{
    if (!exists(workshopPath)) {
        return std::vector<std::filesystem::path>();
    }

    // Make sure we don't request workshop map names every tick.
    const bool shouldRequestWorkshopMapNames = publishedFileID.empty();
    const std::vector<std::filesystem::path> files = iterate_directory(workshopPath, extensions, 0, 1);
    std::filesystem::path bestPath;
    std::vector<std::filesystem::path> workshopMaps;
    for (const std::filesystem::path& file : files) {
        if (file.parent_path() != bestPath.parent_path()) {
            if (!bestPath.empty()) {
                const uint64_t workshopMapId = std::strtoull(bestPath.parent_path().stem().string().c_str(), nullptr,
                                                             10);
                if (shouldRequestWorkshopMapNames && subscribedWorkshopMaps.find(workshopMapId) ==
                    subscribedWorkshopMaps.end()) {
                    publishedFileID.push_back(workshopMapId);
                }
                workshopMaps.push_back(bestPath);
            }
            bestPath = file;
        }
        else if (bestPath.extension() != preferredExtension && file.extension() == preferredExtension) {
            bestPath = file;
        }
    }

    if (!bestPath.empty()) {
        const uint64_t workshopMapId = std::strtoull(bestPath.parent_path().stem().string().c_str(), nullptr, 10);
        if (shouldRequestWorkshopMapNames && subscribedWorkshopMaps.find(workshopMapId) == subscribedWorkshopMaps.
            end()) {
            publishedFileID.push_back(workshopMapId);
        }
        workshopMaps.push_back(bestPath);
    }

    if (shouldRequestWorkshopMapNames && !publishedFileID.empty()) {
        getSubscribedWorkshopMapsAsync();
    }

    return workshopMaps;
}


/*
 *  Commandline Parser Helpers
 */

/// <summary>Prints how to use command options for the `rp` command.</summary>
/// <param name="error">Error to prepend</param>
void PrintCommandOptions(std::string error = "")
{
    error += "usage: rp [option]\n"
        "options:\n"
        "\tjoin [ip](:port) (password)\n"
        "\thost [map] (preset)\n"
        "\tgamemode [gamemode]\n"
        "\tmap [map]\n"
        "\tplayers [players]\n"
        "\tteam [teamNum] [teamName*] [primaryColorIdx*] [accentColorIdx*] (clubMatch)\n"
        "\tmutator [mutator] [value]\n"
        "\trumble [force multiplier] [range multiplier] [duration multiplier]";

    BM_LOG(error);
}


/// <summary>Prints how to use join options for the `rp` command.</summary>
/// <param name="error">Error to prepend</param>
void PrintJoinOptions(std::string error = "")
{
    error += "usage: rp join [ip](:port) (password)\n"
        "parameters:\n"
        "\tip: valid ipv4 ip to connect to\n"
        "\tport (optional, default: " + std::to_string(DEFAULT_PORT) + "): port to connect to\n"
        "\tpassword (optional, default: empty): password the server is protected with";

    BM_LOG(error);
}


/// <summary>Prints how to use the available mutators for the `rp` command.</summary>
/// <param name="mutators">List of available mutators</param>
/// <param name="error">Error to prepend</param>
void PrintAvailableMutators(const std::vector<RocketPlugin::GameSetting>& mutators, std::string error = "")
{
    error += "usage: rp mutator [mutator] [value]\n"
        "mutators:\n";
    for (const RocketPlugin::GameSetting& mutator : mutators) {
        error += "\t" + quote(mutator.InternalCategoryName) + " [value]\n";
    }

    BM_LOG(error.substr(0, error.size() - 1));
}


/// <summary>Finds the index of a given string the mutators list after being sanitized.</summary>
/// <param name="mutators">List of mutators</param>
/// <param name="mutatorNameToFind">Mutator to find the index of</param>
/// <returns>The index of a given string the mutators list or -1 or it was not found</returns>
int FindSanitizedIndexInMutators(const std::vector<RocketPlugin::GameSetting>& mutators, std::string mutatorNameToFind)
{
    for (size_t i = 0; i < mutators.size(); i++) {
        std::string internalMutatorName = mutators[i].InternalCategoryName;
        internalMutatorName.erase(std::remove_if(internalMutatorName.begin(), internalMutatorName.end(),
                                                 [](const char c) {
                                                     return !std::isalnum(c);
                                                 }), internalMutatorName.end());
        mutatorNameToFind.erase(std::remove_if(mutatorNameToFind.begin(), mutatorNameToFind.end(), [](const char c) {
            return !std::isalnum(c);
        }), mutatorNameToFind.end());

        if (to_lower(mutatorNameToFind) == to_lower(internalMutatorName)) {
            return static_cast<int>(i);
        }
    }

    return -1;
}


/// <summary>Prints how to use the available mutator values for the `rp` command.</summary>
/// <param name="mutator"><see cref="RocketPlugin::Mutator"/> to print the values from</param>
/// <param name="error">Error to prepend</param>
void PrintAvailableMutatorValues(const RocketPlugin::GameSetting& mutator, std::string error = "")
{
    error += "usage: rp mutator '" + mutator.InternalCategoryName + "' [value]\n"
        "values:\n";
    error += "\t" + quote("Default") + "\n";
    for (const std::string& value : mutator.InternalName) {
        if (!value.empty()) {
            error += "\t" + quote(value) + "\n";
        }
    }

    BM_LOG(error.substr(0, error.size() - 1));
}


/// <summary>Finds the index of a given string the mutator values list after being sanitized.</summary>
/// <param name="mutator"><see cref="RocketPlugin::Mutator"/>'s to get the values from</param>
/// <param name="mutatorValueToFind">Mutator value to find the index of</param>
/// <returns>The index of a given string the mutator values list or -1 or it was not found</returns>
int FindSanitizedIndexInMutatorValues(const RocketPlugin::GameSetting& mutator, std::string mutatorValueToFind)
{
    if (mutatorValueToFind == "Default") {
        return 0;
    }
    for (size_t i = 0; i < mutator.InternalName.size(); i++) {
        std::string mutatorValue = mutator.InternalName[i];
        mutatorValue.erase(std::ranges::remove_if(mutatorValue, [](const char c) {
            return !std::isalnum(c);
        }).begin(), mutatorValue.end());
        mutatorValueToFind.erase(std::ranges::remove_if(mutatorValueToFind, [](const char c) {
            return !std::isalnum(c);
        }).begin(), mutatorValueToFind.end());

        if (to_lower(mutatorValueToFind) == to_lower(mutatorValue)) {
            return static_cast<int>(i);
        }
    }

    return -1;
}


/// <summary>Prints how to use rumble options for the `rp` command.</summary>
/// <param name="error">Error to prepend</param>
void PrintRumbleOptions(std::string error = "")
{
    error += "usage: rp rumble [force multiplier] [range multiplier] [duration multiplier]\n"
        "parameters:\n"
        "\tforce multiplier: float to multiply with\n"
        "\trange multiplier: float to multiply with\n"
        "\tduration multiplier: float to multiply with";

    BM_LOG(error);
}


/// <summary>Checks if the given string is a int.</summary>
/// <param name="str">String to check if it is a int</param>
/// <returns>Bool with is the string is a int</returns>
bool IsInt(const std::string& str)
{
    std::istringstream iss(str);
    int i;
    iss >> std::noskipws >> i; // std::noskipws considers leading whitespace invalid.

    // Check the entire string was consumed and if either the bad bit or the fail bit is set.
    return iss.eof() && !iss.fail();
}


/// <summary>Checks if the given string is a float.</summary>
/// <param name="str">String to check if it is a float</param>
/// <returns>Bool with is the string is a float</returns>
bool IsFloat(const std::string& str)
{
    std::istringstream iss(str);
    float f;
    iss >> std::noskipws >> f; // std::noskipws considers leading whitespace invalid.

    // Check the entire string was consumed and if either the bad bit or the fail bit is set.
    return iss.eof() && !iss.fail();
}


/// <summary>Checks if the given string is a number.</summary>
/// <param name="str">String to check if it is a number</param>
/// <returns>Bool with is the string is a number</returns>
template<typename T, std::enable_if_t<std::is_fundamental_v<T>, bool> = true>
bool IsNumber(const std::string& str)
{
    std::istringstream iss(str);
    T num;
    iss >> std::noskipws >> num; // std::noskipws considers leading whitespace invalid.

    // Check the entire string was consumed and if either the bad bit or the fail bit is set.
    return iss.eof() && !iss.fail();
}


/// <summary>Parses the arguments for the `rp` command.</summary>
/// <param name="arguments">Arguments given with the `rp` command</param>
void RocketPlugin::parseArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 2) {
        PrintCommandOptions();
        return;
    }

    if (arguments[1] == "join") {
        parseJoinArguments(arguments);
    }
    else if (arguments[1] == "host") {
        parseHostArguments(arguments);
    }
    else if (arguments[1] == "gamemode") {
        parseGameModeArguments(arguments);
    }
    else if (arguments[1] == "map") {
        parseMapArguments(arguments);
    }
    else if (arguments[1] == "players") {
        parsePlayerCountArguments(arguments);
    }
    else if (arguments[1] == "team") {
        parseTeamArguments(arguments);
    }
    else if (arguments[1] == "mutator") {
        parseMutatorArguments(arguments);
    }
    else if (arguments[1] == "rumble") {
        parseRumbleArguments(arguments);
    }
    else {
        PrintCommandOptions("Invalid option '" + arguments[1] + "'\n");
    }
}


void RocketPlugin::parseJoinArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 3) {
        PrintJoinOptions();
        return;
    }
    cvarManager->getCvar("mp_ip").setValue(arguments[2]);

    if (arguments.size() > 3) {
        cvarManager->getCvar("mp_port").setValue(arguments[3]);
    }

    std::string pswd;
    if (arguments.size() > 4) {
        pswd = arguments[4];
    }

    JoinGame(pswd.c_str());
}


void RocketPlugin::parseHostArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 3) {
        BM_LOG("usage: rp host [map] (preset)");
        return;
    }

    if (arguments.size() > 3) {
        loadPreset((std::filesystem::path(*presetDirPath) / arguments[3]).replace_extension(".cfg"));
    }

    HostGame(arguments[2]);
}


/// <summary>Prints how to use the available game modes for the `rp` command.</summary>
/// <param name="gameModes">List of available game modes</param>
/// <param name="error">Error to prepend</param>
void PrintAvailableGameModes(const RocketPlugin::GameSetting& gameModes, std::string error = "")
{
    error += "usage: rp gamemode [gamemode]\n"
        "game modes:\n";
    for (const std::string& gameMode : gameModes.DisplayName) {
        if (!gameMode.empty()) {
            error += "\t" + quote(gameMode) + "\n";
        }
    }

    BM_LOG(error.substr(0, error.size() - 1));
}


void RocketPlugin::parseGameModeArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 3) {
        PrintAvailableGameModes(gameModes);
        return;
    }

    const std::string& gameMode = arguments[2];
    if (const auto& itDisplay = std::ranges::find(gameModes.DisplayName, gameMode);
        itDisplay != gameModes.DisplayName.end()) {
        gameModes.CurrentSelected = std::distance(gameModes.DisplayName.begin(), itDisplay);
    }
    else if (const auto& itInternal = std::ranges::find(gameModes.InternalName, gameMode);
        itInternal != gameModes.InternalName.end()) {
        gameModes.CurrentSelected = std::distance(gameModes.InternalName.begin(), itInternal);
    }
    else {
        PrintAvailableGameModes(gameModes, "Invalid game mode '" + gameMode + "'\n");
        return;
    }

    if (isHostingLocalGame()) {
        setMatchSettings();
        BM_LOG("Updated game mode for next match.");
    }
}


void RocketPlugin::parseMapArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 3) {
        BM_LOG("usage: rp map [map]");
        return;
    }

    const std::filesystem::path map = arguments[2];
#ifdef DEBUG
    const bool mapLoaded = preLoadMap(absolute(map), false, true);
#else
    const bool mapLoaded = preLoadMap(absolute(map));
#endif // DEBUG
    if (mapLoaded) {
        enableWorkshopMaps = false;
        enableCustomMaps = true;
        currentMap = map.string();
    }
    else if (const auto& it = maps.find(map.string()); it != maps.end()) {
        enableWorkshopMaps = false;
        enableCustomMaps = false;
        currentMap = it->first;
    }
    else {
        BM_ERROR_LOG("Invalid map {:s}", quote(arguments[2]));
        return;
    }

    if (isHostingLocalGame()) {
        setMatchMapName(map.stem().string());
        BM_LOG("Updated map for next match.");
    }
}


void RocketPlugin::parsePlayerCountArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 3) {
        BM_LOG("usage: rp players [players]");
        return;
    }

    const std::string& players = arguments[2];
    if (!IsInt(players)) {
        BM_ERROR_LOG("Invalid number of players {:s}", quote(players));
        return;
    }

    playerCount = std::max(2l, std::strtol(players.c_str(), nullptr, 10));

    if (isHostingLocalGame()) {
        BM_WARNING_LOG("Updating player count may not update while already hosting.");
    }
}


void setTeamColor(int8_t& teamColor, const int8_t& newTeamColor)
{
    teamColor = newTeamColor;
}


void RocketPlugin::parseTeamArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 6) {
        BM_LOG("usage: rp team [teamNum] [teamName*] [primaryColorIdx*] [accentColorIdx*] (clubMatch), *leave empty for default");
        return;
    }

    // Team number.
    const std::string& teamNumStr = arguments[2];
    const unsigned long& teamNum = std::strtoul(teamNumStr.c_str(), nullptr, 10);
    if (!IsInt(teamNumStr) || (teamNum != 0 && teamNum != 1)) {
        BM_ERROR_LOG("Invalid team number {:s}", quote(teamNumStr));
        return;
    }

    // Team name.
    if (teamNum == 0) {
        team1Name = to_upper(arguments[3]);
    }
    else {
        team2Name = to_upper(arguments[3]);
    }

    // Primary color.
    const std::string& primaryColorStr = arguments[4];
    if (primaryColorStr.empty()) {
        setTeamColor(teamNum == 0 ? team1PrimCol : team2PrimCol, -1);
    }
    else {
        const long primaryColorIdx = std::strtol(primaryColorStr.c_str(), nullptr, 10);
        if (!IsInt(primaryColorStr) || primaryColorIdx >= clubColors.size()) {
            BM_ERROR_LOG("Invalid primary color index {:s}", quote(primaryColorStr));
            return;
        }
        setTeamColor(teamNum == 0 ? team1PrimCol : team2PrimCol, static_cast<int8_t>(primaryColorIdx));
    }

    // Accent color.
    const std::string& accentColorStr = arguments[5];
    if (accentColorStr.empty()) {
        setTeamColor(teamNum == 0 ? team1AccCol : team2AccCol, -1);
    }
    else {
        const long accentColorIdx = std::strtol(accentColorStr.c_str(), nullptr, 10);
        if (!IsInt(accentColorStr) || accentColorIdx >= customColors.size()) {
            BM_ERROR_LOG("Invalid primary color index {:s}", quote(accentColorStr));
            return;
        }
        setTeamColor(teamNum == 0 ? team1AccCol : team2AccCol, static_cast<int8_t>(accentColorIdx));
    }

    // Club match.
    if (arguments.size() > 6) {
        const std::string& clubMatchStr = arguments[6];
        if (!IsNumber<bool>(clubMatchStr)) {
            BM_ERROR_LOG("Invalid club match {:s}", quote(clubMatchStr));
            return;
        }
        clubMatch = std::strtoul(clubMatchStr.c_str(), nullptr, 10);
    }

    if (isHostingLocalGame()) {
        setMatchSettings();
        BM_LOG("Updated game mode for next match.");
    }
}


void RocketPlugin::parseMutatorArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 3) {
        PrintAvailableMutators(mutators);
        return;
    }

    const std::string& mutator = arguments[2];
    const int i = FindSanitizedIndexInMutators(mutators, mutator);
    if (i == -1) {
        PrintAvailableMutators(mutators, "Invalid mutator '" + mutator + "'\n");
        return;
    }

    if (arguments.size() < 4) {
        PrintAvailableMutatorValues(mutators[i]);
        return;
    }

    const std::string& mutatorValue = arguments[3];
    const int j = FindSanitizedIndexInMutatorValues(mutators[i], mutatorValue);
    if (j == -1) {
        PrintAvailableMutatorValues(mutators[i], "Invalid value '" + mutatorValue + "' for '" +
                                    mutators[i].InternalCategoryName + "'\n");
        return;
    }

    mutators[i].CurrentSelected = j;
    BM_LOG("Changed {:s} to {:s}", quote(mutators[i].InternalCategoryName), quote(mutators[i].InternalName[j]));

    if (isHostingLocalGame()) {
        setMatchSettings();
        BM_LOG("Updated mutator for next match.");
    }
}


void RocketPlugin::parseRumbleArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 5 || !IsFloat(arguments[2]) || !IsFloat(arguments[3]) || !IsFloat(arguments[4])) {
        PrintRumbleOptions();
        return;
    }

    const float forceMultiplier    = std::strtof(arguments[2].c_str(), nullptr);
    const float rangeMultiplier    = std::strtof(arguments[3].c_str(), nullptr);
    const float durationMultiplier = std::strtof(arguments[4].c_str(), nullptr);
    const std::shared_ptr<CrazyRumble> crazyRumble = GetCustomGameMode<CrazyRumble>();
    if (crazyRumble != nullptr) {
        crazyRumble->UpdateItemsValues(forceMultiplier, rangeMultiplier, durationMultiplier);
    }
}


/// <summary>Autocomplete the `rp` command.</summary>
/// <remarks>Not available in the public BakkesMod version.</remarks>
/// <param name="arguments">Arguments given with the `rp` command</param>
/// <returns>List of suggestions</returns>
std::vector<std::string> RocketPlugin::complete(const std::vector<std::string>& arguments)
{
    std::vector<std::string> suggestions;

    if (arguments.size() == 2 && arguments[1].empty()) {
        suggestions.emplace_back("join ;[ip](:port) (password)");
        suggestions.emplace_back("host ;[map] (preset)");
        suggestions.emplace_back("gamemode ;[gamemode]");
        suggestions.emplace_back("map ;[map]");
        suggestions.emplace_back("players ;[players]");
        suggestions.emplace_back("mutator ;[mutator] [value]");
        suggestions.emplace_back("rumble ;[force_multiplier] [range_multiplier] [duration_multiplier]");
    }
    else if (std::string("join").find(arguments[1]) == 0) {
        if (arguments.size() == 2) {
            suggestions.emplace_back("join ;[ip](:port) (password)");
        }
        else if (arguments.size() == 3) {
            if (arguments[2].empty()) {
                suggestions.push_back(fmt::format("join {:s}:{:d} ;(password)", *joinIP, *joinPort));
            }
            else {
                suggestions.push_back(fmt::format("join {:s}:{:d} ;(password)", arguments[2], *joinPort));
            }
        }
        else if (arguments.size() == 4) {
            suggestions.push_back("join " + arguments[2] + " ;(password)");
        }
    }
    else if (std::string("mutator").find(arguments[1]) == 0) {
        if (arguments.size() == 2) {
            suggestions.emplace_back("mutator ;[mutator] [value]");
        }
        if (arguments.size() == 3) {
            for (const GameSetting& mutator : mutators) {
                if (to_lower(mutator.InternalCategoryName).find(to_lower(arguments[2])) == 0) {
                    suggestions.push_back("mutator " + quote(mutator.InternalCategoryName) + " ;[value]");
                }
            }
        }
        else if (arguments.size() == 4) {
            const int i = FindSanitizedIndexInMutators(mutators, arguments[2]);
            if (i != -1) {
                for (const std::string& value : mutators[i].InternalName) {
                    if (to_lower(value).find(to_lower(arguments[3])) == 0) {
                        suggestions.push_back("mutator " + quote(arguments[2]) + " " + quote(value));
                    }
                }
            }
        }
    }
    else if (std::string("rumble").find(arguments[1]) == 0) {
        if (arguments.size() == 2) {
            suggestions.emplace_back("rumble ;[force_multiplier] [range_multiplier] [duration_multiplier]");
        }
        if (arguments.size() == 3) {
            suggestions.emplace_back("rumble ;[force_multiplier] [range_multiplier] [duration_multiplier]");
        }
        else if (arguments.size() == 4) {
            suggestions.push_back("rumble " + arguments[2] + " ;[range_multiplier] [duration_multiplier]");
        }
        else if (arguments.size() == 5) {
            suggestions.push_back("rumble " + arguments[2] + " " + arguments[3] + " ;[duration_multiplier]");
        }
    }

    return suggestions;
}


/*
 *  Host/Join Match
 */


/// <summary>Hosts a local game with the preconfigured settings.</summary>
/// <param name="arena">Arena to host the local game in, if empty the selected map is used</param>
void RocketPlugin::HostGame(std::string arena)
{
    if (arena.empty()) {
        if (enableWorkshopMaps || enableCustomMaps) {
            if (!std::filesystem::exists(currentMap)) {
                BM_ERROR_LOG("no map selected.");
                PushError("Hosting map failed: no map selected");
                return;
            }

#ifdef DEBUG
            const bool mapLoaded = preLoadMap(std::filesystem::absolute(currentMap), false, true);
#else
            const bool mapLoaded = preLoadMap(std::filesystem::absolute(currentMap));
#endif // DEBUG
            if (!mapLoaded) {
                arena = std::filesystem::absolute(currentMap).string();
            }
            else {
                arena = std::filesystem::path(currentMap).stem().string();
            }
        }
        else {
            if (maps.find(currentMap) == maps.end()) {
                BM_ERROR_LOG("no map selected.");
                PushError("Hosting map failed: no map selected");
                return;
            }
            arena = currentMap;
        }
    }
#ifdef _WIN32
    /* Cos we as windows want our baguettes the left way. */
    std::ranges::replace(arena, '/', '\\');
#endif
    const std::string gameMode = gameModes.GetSelected();
    std::string gameTags = getGameTags();
    // Available PlayerCounts are 2, 4, 6 and 8.
    gameTags += ",PlayerCount" + std::to_string(std::clamp(playerCount, 2, 8) & ~1);
    const int numConnections = std::max(10, playerCount);
    const std::string networkOptions = fmt::format("?NumPublicConnections={0:d}?NumOpenPublicConnections={0:d}?Lan?Listen", numConnections);

    hostingGame = true;
    if (hostWithParty) {
        shouldInviteParty = setSearchStatus(L"Searching", true);
    }
    if (enableWorkshopMaps || enableCustomMaps) {
        loadingScreenHooked = true;
        if (customMapPaths.find(currentMap) == customMapPaths.end()) {
            loadingScreenMapName = std::filesystem::path(currentMap).stem();
        }
        else {
            loadingScreenMapName = to_wstring(customMapPaths[currentMap]);
        }
        loadingScreenMapAuthor = L"unknown";
        const uint64_t workshopMapId = std::strtoll(
            std::filesystem::path(currentMap).parent_path().stem().string().c_str(), nullptr, 10);
        if (subscribedWorkshopMaps.find(workshopMapId) != subscribedWorkshopMaps.end()) {
            const WorkshopMap workshopMap = subscribedWorkshopMaps[workshopMapId];
            const std::wstring nickname = getPlayerNickname(workshopMap.Owner);
            if (!nickname.empty()) {
                loadingScreenMapAuthor = nickname;
                loadingScreenMapName = to_wstring(workshopMap.Title);
            }
        }
    }
    // Disable "Show boost usage" when hosting LAN matches.
    cvarManager->getCvar("cl_soccar_boostcounter").setValue(false);

    // Delay this so the search status can be send before loading.
    const std::string command = fmt::format("open {:s}?Playtest?game={:s}?GameTags={:s}{:s}", arena, gameMode, gameTags,
        networkOptions);
    SetTimeout([this, command = command](GameWrapper*) {
        gameWrapper->ExecuteUnrealCommand(command);
    }, 0.1f);
}


/// <summary>Joins a local game with the preconfigured settings.</summary>
/// <param name="pswd">Password the local game is protected with</param>
void RocketPlugin::JoinGame(const char* pswd)
{
    isJoiningHost = true;
    if (joinCustomMap) {
        loadingScreenHooked = true;
        loadingScreenMapName = to_wstring(joinableMaps[currentJoinMap]);
        loadingScreenMapAuthor = L"author";
        // Add the owners nickname if we can find them.
        const uint64_t workshopMapId = std::strtoll(
            std::filesystem::path(currentMap).parent_path().stem().string().c_str(), nullptr, 10);
        if (subscribedWorkshopMaps.find(workshopMapId) != subscribedWorkshopMaps.end()) {
            const WorkshopMap workshopMap = subscribedWorkshopMaps[workshopMapId];
            const std::wstring nickname = getPlayerNickname(workshopMap.Owner);
            if (!nickname.empty()) {
                loadingScreenMapAuthor = nickname;
                loadingScreenMapName = to_wstring(workshopMap.Title);
            }
        }

#ifdef DEBUG
        const bool mapLoaded = preLoadMap(absolute(currentJoinMap), false, true);
#else
        const bool mapLoaded = preLoadMap(absolute(currentJoinMap));
#endif // DEBUG
        if (!mapLoaded) {
            PushError("Error Loading Map\ncheck the console for more details(F6)");
        }
    }

    gameWrapper->ExecuteUnrealCommand(fmt::format("start {:s}:{:d}/?Lan?Password={:s}", *joinIP, *joinPort, pswd));
}


/// <summary>Gets the currently selected game tags.</summary>
/// <returns>The currently selected game tags</returns>
std::string RocketPlugin::getGameTags() const
{
    std::string gameTags;
    for (const GameSetting& mutator : mutators) {
        if (!mutator.GetSelected().empty()) {
            gameTags += mutator.GetSelected() + ",";
        }
    }
    gameTags += botDifficulties.GetSelected();

    return gameTags;
}


/// <summary>Saves the configured preset with the given filename.</summary>
/// <param name="presetName">Name of the preset file</param>
void RocketPlugin::savePreset(const std::string& presetName)
{
    std::ofstream presetFile(std::filesystem::path(*presetDirPath) / presetName);
    if (presetFile.is_open()) {
        presetFile << "// This preset has been autogenerated by Rocket Plugin\n";
        for (const GameSetting& mutator : mutators) {
            if (mutator.CurrentSelected != 0) {
                presetFile << "rp mutator " + quote(mutator.InternalCategoryName) + " " + quote(mutator.GetSelected())
                    << std::endl;
            }
        }
    }
}


/// <summary>Loads the preset with the given filename.</summary>
/// <param name="presetPath">Path of the preset file</param>
void RocketPlugin::loadPreset(const std::filesystem::path& presetPath)
{
    resetMutators();
    BM_TRACE_LOG("loading preset {:s}", quote(presetPath.string()));
    cvarManager->loadCfg(presetPath.generic_string());
}


/// <summary>Resets the mutators selectors.</summary>
void RocketPlugin::resetMutators()
{
    for (GameSetting& mutator : mutators) {
        mutator.CurrentSelected = 0;
    }
}


/// <summary>Copies the selected map to a folder in 'CookedPCConsole' where Rocket League can load it.</summary>
/// <param name="map">Map to copy, if no map is given the selected map will be copied</param>
void RocketPlugin::copyMap(const std::filesystem::path& map)
{
    std::filesystem::path mapSrc = map;
    if (mapSrc.empty()) {
        mapSrc = currentJoinMap;
    }
    const std::filesystem::path mapDst = COPIED_MAPS_PATH / (mapSrc.stem().wstring() + L".upk");

    std::error_code ec;
    /* Creates parent directory. */
    if (!exists(mapDst.parent_path()) && !create_directories(mapDst.parent_path(), ec)) {
        PushError("Error Fixing Map\n"
                  "check the console (F6) for more details, for more details and report to the mod author if applicable.");
        BM_ERROR_LOG("failed to create directory: {:s}, {:s}", quote(mapDst.parent_path().string()), quote(ec.message()));
        return;
    }

    ec.clear();
    /* Copies file to new directory. */
    std::filesystem::copy(mapSrc, mapDst, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        PushError("Error Fixing Map\n"
                  "check the console (F6) for more details, for more details and report to the mod author if applicable.");
        BM_ERROR_LOG("failed to copy file: {:s} to {:s}, {:s}", quote(mapSrc.string()), quote(mapDst.string()),
                     quote(ec.message()));
        BM_INFO_LOG("-------------------------------------------------------");
        BM_INFO_LOG("currentJoinMap = {:s}", quote(absolute(currentJoinMap).string()));
        int i = 0;
        for (const auto& [joinableMap, _] : joinableMaps) {
            BM_INFO_LOG("[{:d}] = path: {:s}, exists: {:s}", i++, quote(joinableMap.string()), exists(joinableMap));
        }
        BM_INFO_LOG("-------------------------------------------------------");

        return;
    }

    BM_TRACE_LOG("created file successfully.");

    refreshJoinableMaps = true;
    refreshCustomMapPaths = true;
}


/// <summary>Sets the countdown time at the start of a match.</summary>
/// <remarks>Gets called on 'TAGame.GameEvent_TA.Init'.</remarks>
/// <param name="server">The game server that started</param>
void RocketPlugin::onGameEventInit([[maybe_unused]] const ServerWrapper& server)
{
    // Clear car physics cache.
    carPhysicsMods.carPhysics.clear();
    isJoiningHost = false;

    if (!hostingGame) {
        return;
    }
    hostingGame = false;

    std::string arena;
    if (enableWorkshopMaps || enableCustomMaps) {
        arena = std::filesystem::path(currentMap).stem().string();
    }
    else {
        arena = currentMap;
    }

    setMatchSettings(arena);
    //setMatchMapName(arena);

    if (shouldInviteParty) {
        shouldInviteParty = false;
        if (!setSearchStatus(L"Joining")) {
            BM_WARNING_LOG("could not set the search status");
        }
        SetTimeout([this](GameWrapper*) {
            broadcastJoining();
        }, 1.41f);
    }
}


/*
 *  Modules
 */

/// <summary>Gets the current game as <see cref="ServerWrapper"/>.</summary>
/// <param name="allowOnlineGame">Bool with if should try to get a online game</param>
/// <returns>The current game as <see cref="ServerWrapper"/> or <c>NULL</c> is no game is found</returns>
ServerWrapper RocketPlugin::GetGame(const bool allowOnlineGame) const
{
    ServerWrapper localGame = gameWrapper->GetGameEventAsServer();
    if (!localGame.IsNull()) {
        return localGame;
    }
    if (allowOnlineGame) {
        ServerWrapper onlineGame = gameWrapper->GetOnlineGame();
        if (!onlineGame.IsNull()) {
            return onlineGame;
        }
    }

    return NULL;
}


/// <summary>Checks if you are in a game.</summary>
/// <param name="allowOnlineGame">Bool with if should check online games</param>
/// <returns>Whether you are in a game</returns>
bool RocketPlugin::IsInGame(const bool allowOnlineGame) const
{
    ServerWrapper game = GetGame(allowOnlineGame);
    return !game.IsNull();
}


/*
 *  BakkesMod plugin overrides
 */

/// <summary>Checks if the GUI window is bound.</summary>
/// <param name="windowName">Name of the GUI window</param>
/// <returns>Bool with if the GUI window is bound</returns>
bool IsGUIWindowBound(const std::string& windowName)
{
    const std::string bind = "togglemenu " + windowName;
    std::ifstream file(BINDS_FILE_PATH);
    if (file.is_open()) {
        std::string line;
        while (getline(file, line)) {
            if (line.find(bind) != std::string::npos) {
                file.close();
                return true;
            }
        }
        file.close();
    }

    return false;
}


/// <summary>Registers notifiers and variables to interact with the plugin on load.</summary>
void RocketPlugin::OnLoad()
{
    BakkesModConfigFolder = gameWrapper->GetBakkesModPath() / L"cfg";
    BakkesModCrashesFolder = gameWrapper->GetBakkesModPath() / L"crashes";
    if (!exists(BakkesModCrashesFolder)) {
        create_directory(BakkesModCrashesFolder);
    }
    RocketPluginDataFolder = gameWrapper->GetDataFolder() / L"RocketPlugin";
    if (!exists(RocketPluginDataFolder)) {
        create_directory(RocketPluginDataFolder);
    }
    RocketLeagueExecutableFolder = std::filesystem::current_path();

    set_game_thread();

    // Copy the original cvarManager so we can use it everywhere.
    GlobalCVarManager = std::reinterpret_pointer_cast<CVarManagerWrapperDebug>(cvarManager);

    /* Register CVars */
    registerCVars();
    registerExternalCVars();

    /* Register Notifiers */
    registerNotifiers();
    registerExternalNotifiers();

    /* Register Hooks */
    registerHooks();
    registerExternalHooks();

    // Load RL Constants.
    loadRLConstants();

    // Load Constants Config.
    ConstantsConfig = std::make_unique<RPConfig>(DEFAULT_CONSTANTS_CONFIG_URL);

    /* GUI Settings */

    // Set the window bind to the default keybind if is not set.
    if (!IsGUIWindowBound(GetMenuName())) {
        cvarManager->setBind(DEFAULT_GUI_KEYBIND, "togglemenu " + GetMenuName());
        BM_LOG("Set window keybind to {:s}", DEFAULT_GUI_KEYBIND);
    }

    // Load the custom mutator presets.
    presetPaths = get_files_from_dir(*presetDirPath, 2, ".cfg");

    /* Init Networking */
    upnpClient = std::make_shared<UPnPClient>();
    p2pHost = std::make_shared<P2PHost>();

    /* Init Modules */
    RocketPluginModule::rocketPlugin = this;

    /* Init Game Modes */
    customGameModes.push_back(std::make_shared<Drainage>());
    customGameModes.push_back(std::make_shared<CrazyRumble>());
    customGameModes.push_back(std::make_shared<Zombies>());
    customGameModes.push_back(std::make_shared<BoostSteal>());
    customGameModes.push_back(std::make_shared<KeepAway>());
    customGameModes.push_back(std::make_shared<Tag>());
    customGameModes.push_back(std::make_shared<Juggernaut>());
    customGameModes.push_back(std::make_shared<BoostMod>());
    customGameModes.push_back(std::make_shared<BoostShare>());
    customGameModes.push_back(std::make_shared<SacredGround>());
}


/// <summary>Unload the plugin properly.</summary>
void RocketPlugin::OnUnload()
{
    //// Save all CVars to 'config.cfg'.
    //cvarManager->backupCfg(CONFIG_FILE_PATH.string());
}


/// <summary>Registers CVars for Rocket Plugin.</summary>
void RocketPlugin::registerCVars()
{
    joinIP = std::make_shared<std::string>();
    cvarManager->registerCvar("mp_ip", "127.0.0.1", "Default ip for joining local matches").bindTo(joinIP);

    joinPort = std::make_shared<int>(DEFAULT_PORT);
    cvarManager->registerCvar("mp_port", std::to_string(DEFAULT_PORT), "Default port for joining local matches").bindTo(
        joinPort);

    presetDirPath = std::make_shared<std::string>();
    cvarManager->registerCvar("rp_preset_path", PRESETS_PATH.string(), "Default path for the mutator presets directory")
               .bindTo(presetDirPath);

    workshopMapDirPath = std::make_shared<std::string>();
    cvarManager->registerCvar("rp_workshop_path", WORKSHOP_MAPS_PATH.string(),
                              "Default path for your workshop maps directory").bindTo(workshopMapDirPath);

    customMapDirPath = std::make_shared<std::string>();
    cvarManager->registerCvar("rp_custom_path", CUSTOM_MAPS_PATH.string(),
                              "Default path for your custom maps directory").bindTo(customMapDirPath);

    cvarManager->registerCvar("rp_gui_keybind", DEFAULT_GUI_KEYBIND, "Keybind for the gui");

    LogLevel = std::make_shared<int>(0);
    cvarManager->registerCvar("rp_log_level", std::to_string(CVarManagerWrapperDebug::level_enum::normal), "Log level",
                              true, false, 0, false, 0, false).bindTo(LogLevel);

#ifdef DEBUG
    cvarManager->getCvar("rp_log_level").setValue(CVarManagerWrapperDebug::level_enum::all);

    showDemoWindow = std::make_shared<bool>(false);
    cvarManager->registerCvar("rp_show_demo_window", "0", "Shows the Dear ImGui demo window", true, false, 0, false, 0,
                              false).bindTo(showDemoWindow);

    showMetricsWindow = std::make_shared<bool>(false);
    cvarManager->registerCvar("rp_show_metrics_window", "0", "Shows the Dear ImGui metrics window", true, false, 0,
                              false, 0, false).bindTo(showMetricsWindow);
#endif
}


/// <summary>Registers notifiers for Rocket Plugin.</summary>
void RocketPlugin::registerNotifiers()
{
    RegisterNotifier("rp", [this](const std::vector<std::string>& arguments) {
        parseArguments(arguments);
    }, "Parses commands to interact with Rocket Plugin.", PERMISSION_ALL);

    RegisterNotifier("rp_enable_debug_mode", [this](const std::vector<std::string>&) {
        cvarManager->getCvar("rp_log_level").setValue(CVarManagerWrapperDebug::level_enum::all);
    }, "Enables debug mode.", PERMISSION_ALL);

    RegisterNotifier("rp_change_keybind", [this](const std::vector<std::string>& arguments) {
        std::string key;
        if (arguments.size() >= 2) {
            key = arguments[1];
        }
        else {
            key = cvarManager->getCvar("rp_gui_keybind").getStringValue();
        }

        const std::string command = "togglemenu " + GetMenuName();
        cvarManager->setBind(key, command);
        BM_LOG("Set {:s} to {:s}", quote(key), quote(command));
    }, "Adds a keybind for " + quote("togglemenu " + GetMenuName()) + " as $rp_gui_keybind or given argument.",
    PERMISSION_ALL);

    RegisterNotifier("rp_broadcast_game", [this](const std::vector<std::string>&) {
        broadcastJoining();
    }, "Broadcasts a game invite to your party members.", PERMISSION_SOCCAR);

    RegisterNotifier("rp_clear_car_physics_cache", [this](const std::vector<std::string>&) {
        carPhysicsMods.carPhysics.clear();
    }, "Broadcasts a game invite to your party members.", PERMISSION_SOCCAR);

    RegisterNotifier("rp_add_beta_game_modes", [this](const std::vector<std::string>&) {
        if (GetCustomGameMode<SmallCars>() == nullptr) {
            customGameModes.push_back(std::make_shared<SmallCars>());
            BM_TRACE_LOG("added Small Cars game mode");
        }
        else {
            BM_WARNING_LOG("Small Cars game mode already added");
        }
    }, "Adds beta game modes.", PERMISSION_ALL);
}


/// <summary>Register hooks for Rocket Plugin.</summary>
void RocketPlugin::registerHooks()
{
    HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.EventVehicleSetup",
        [this](const CarWrapper& caller, void*, const std::string&) {
            carPhysicsMods.SetPhysics(caller);
        });

    HookEventWithCaller<ServerWrapper>("Function TAGame.GameEvent_TA.Init",
        [this](const ServerWrapper& caller, void*, const std::string&) {
            onGameEventInit(caller);
        });
}


/// <summary>Fixes the Display names of the game settings that could not be localized.</summary>
/// <param name="other">Game setting to fix the Display name of</param>
void RocketPlugin::GameSetting::FixDisplayNames(const GameSetting& other)
{
    if (DisplayCategoryName.empty() || DisplayCategoryName.front() == MARKED_INCOMPLETE) {
#ifdef DEBUG
        DisplayCategoryName = quote(other.DisplayCategoryName);
#else
        DisplayCategoryName = other.DisplayCategoryName;
#endif
    }
    for (size_t i = 0; i < InternalName.size(); i++) {
        // Check if we could find the correct localization.
        if (DisplayName[i].front() != MARKED_INCOMPLETE) {
            continue;
        }
        // Otherwise we will try to replace it with the other name.
        bool found = false;
        for (size_t j = 0; j < other.InternalName.size(); j++) {
            if (to_lower(other.InternalName[j]) == to_lower(InternalName[i])) {
#ifdef DEBUG
                DisplayName[i] = quote(other.DisplayName[j]);
#else
                DisplayName[i] = other.DisplayName[j];
#endif
                found = true;
                break;
            }
        }
        if (!found) {
#ifndef DEBUG
            // If we can't find the predefined name, we remove the incomplete mark.
            DisplayName[i] = DisplayName[i].substr(1);
#endif
        }
    }
}


/// <summary>Get selected game setting.</summary>
/// <returns>Selected game setting</returns>
std::string RocketPlugin::GameSetting::GetSelected() const
{
    return InternalName[CurrentSelected];
}
