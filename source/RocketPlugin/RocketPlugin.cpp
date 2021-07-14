// RocketPlugin.cpp
// A BakkesMod plugin for joining, hosting and manipulating multiplayer games.
//
// Author:        Stanbroek
// Version:       0.6.7 14/07/21
// BMSDK version: 95
#include "Config.h"
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

std::shared_ptr<int> LogLevel;
std::shared_ptr<CVarManagerWrapperDebug> GlobalCVarManager;


/*
 *  File Helpers
 */

/// <summary>Checks if the given file extension is in the list of extensions.</summary>
/// <param name="fileExtension">File extension</param>
/// <param name="extensions">List of file extensions</param>
/// <returns>Bool with if the file extension is in the list of extensions</returns>
bool RocketPlugin::HasExtension(const std::string& fileExtension, const std::vector<std::string>& extensions)
{
    // Filter out unwanted file extensions.
    return std::any_of(extensions.begin(), extensions.end(), [&](const std::string& extension) {
        return fileExtension == extension;
    });
}


/// <summary>Recursively gets files from a certain directory.</summary>
/// <remarks>These files can be filtered by if they end with certain file extensions.</remarks>
/// <param name="directory">Path to the directory to get the files from</param>
/// <param name="extensions">List of file extensions to filter by</param>
/// <param name="depth">Current folder depth</param>
/// <param name="maxDepth">Max folder depth to iterate through</param>
/// <returns>The files from a certain directory</returns>
std::vector<std::filesystem::path> RocketPlugin::IterateDirectory(const std::filesystem::path& directory,
    const std::vector<std::string>& extensions,
    const int depth, const int maxDepth)
{
    if (depth > maxDepth) {
        return std::vector<std::filesystem::path>();
    }

    std::vector<std::filesystem::path> files;
    for (const std::filesystem::directory_entry& file : std::filesystem::directory_iterator(directory)) {
        const std::filesystem::path& filePath = file.path();
        if (file.is_directory()) {
            std::vector<std::filesystem::path> directoryFiles = IterateDirectory(
                filePath, extensions, depth + 1, maxDepth);
            // Remove if directory is empty.
            if (!directoryFiles.empty()) {
                files.insert(files.end(), directoryFiles.begin(), directoryFiles.end());
            }
        }
        else if (HasExtension(filePath.extension().string(), extensions)) {
            files.push_back(filePath);
        }
    }

    return files;
}


/// <summary>Gets files from a certain directory.</summary>
/// <remarks>These files can be filtered by if they end with certain file extensions.</remarks>
/// <param name="directory">Path to the directory to get the files from</param>
/// <param name="numExtension">Number if filters to filter the files by</param>
/// <param name="...">Multiple file extensions to filter by</param>
/// <returns>The files from a certain directory</returns>
std::vector<std::filesystem::path> RocketPlugin::GetFilesFromDir(const std::filesystem::path& directory,
    int numExtension, ...)
{
    if (!exists(directory)) {
        return std::vector<std::filesystem::path>();
    }

    va_list extensions;
    std::vector<std::string> fileExtensions;

    va_start(extensions, numExtension);
    for (int i = 0; i < numExtension; i++) {
        fileExtensions.emplace_back(va_arg(extensions, const char*));
    }
    va_end(extensions);

    return IterateDirectory(directory, fileExtensions, 0, 1);
}


/*
 *  Map File Helpers
 */

/// <summary>Gets workshop maps from the given directory.</summary>
/// <param name="workshopPath">Path to the workshop directory to get the maps from</param>
/// <param name="extensions">List of map extensions to filter by</param>
/// <param name="preferredExtension">Map extension to prefer when multiple files are found</param>
/// <returns>The workshop maps from the given directory</returns>
std::vector<std::filesystem::path> RocketPlugin::getWorkshopMaps(const std::filesystem::path& workshopPath,
    const std::vector<std::string>& extensions,
    const std::string& preferredExtension)
{
    if (!exists(workshopPath)) {
        return std::vector<std::filesystem::path>();
    }

    // Make sure we don't request workshop map names every tick.
    const bool shouldRequestWorkshopMapNames = publishedFileID.empty();
    std::vector<std::filesystem::path> files = IterateDirectory(workshopPath, extensions, 0, 1);
    std::filesystem::path bestPath;
    std::vector<std::filesystem::path> workshopMaps;
    for (const std::filesystem::path& file : files) {
        if (file.parent_path() != bestPath.parent_path()) {
            if (!bestPath.empty()) {
                const uint64_t workshopMapId = std::strtoull(bestPath.parent_path().stem().string().c_str(), nullptr,
                    10);
                if (shouldRequestWorkshopMapNames && subscribedWorkshopMaps.find(workshopMapId) == subscribedWorkshopMaps.end()) {
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
        if (shouldRequestWorkshopMapNames && subscribedWorkshopMaps.find(workshopMapId) == subscribedWorkshopMaps.end()) {
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

    LOG(error);
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

    LOG(error);
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

    LOG(error.substr(0, error.size() - 1));
}


/// <summary>Finds the index of a given string the mutators list after being sanitized.</summary>
/// <param name="mutators">List of mutators</param>
/// <param name="mutatorNameToFind">Mutator to find the index of</param>
/// <returns>The index of a given string the mutators list or -1 or it was not found</returns>
int FindSanitizedIndexInMutators(const std::vector<RocketPlugin::GameSetting>& mutators, std::string mutatorNameToFind)
{
    for (size_t i = 0; i < mutators.size(); i++) {
        std::string internalMutatorName = mutators[i].InternalCategoryName;
        internalMutatorName.erase(std::remove_if(internalMutatorName.begin(), internalMutatorName.end(), [](const char c) {
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

    LOG(error.substr(0, error.size() - 1));
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
        mutatorValue.erase(std::remove_if(mutatorValue.begin(), mutatorValue.end(), [](const char c) {
            return !std::isalnum(c);
        }), mutatorValue.end());
        mutatorValueToFind.erase(std::remove_if(mutatorValueToFind.begin(), mutatorValueToFind.end(), [](const char c) {
            return !std::isalnum(c);
        }), mutatorValueToFind.end());

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

    LOG(error);
}


/// <summary>Checks if the given string is a int.</summary>
/// <param name="str">String to check if it is a int</param>
/// <returns>Bool with is the string is a int</returns>
bool IsInt(const std::string& str) {
    std::istringstream iss(str);
    int i;
    iss >> std::noskipws >> i; // std::noskipws considers leading whitespace invalid.

    // Check the entire string was consumed and if either the bad bit or the fail bit is set.
    return iss.eof() && !iss.fail();
}


/// <summary>Checks if the given string is a float.</summary>
/// <param name="str">String to check if it is a float</param>
/// <returns>Bool with is the string is a float</returns>
bool IsFloat(const std::string& str) {
    std::istringstream iss(str);
    float f;
    iss >> std::noskipws >> f; // std::noskipws considers leading whitespace invalid.

    // Check the entire string was consumed and if either the bad bit or the fail bit is set.
    return iss.eof() && !iss.fail();
}


/// <summary>Checks if the given string is a number.</summary>
/// <param name="str">String to check if it is a number</param>
/// <returns>Bool with is the string is a number</returns>
template<typename T, std::enable_if_t<std::is_fundamental<T>::value, bool> = true>
bool IsNumber(const std::string& str) {
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
        LOG("usage: rp host [map] (preset)");
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

    LOG(error.substr(0, error.size() - 1));
}


void RocketPlugin::parseGameModeArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 3) {
        PrintAvailableGameModes(gameModes);
        return;
    }

    const std::string& gameMode = arguments[2];
    if (const auto& itDisplay = std::find(gameModes.DisplayName.begin(), gameModes.DisplayName.end(), gameMode); itDisplay != gameModes.DisplayName.end()) {
        gameModes.CurrentSelected = std::distance(gameModes.DisplayName.begin(), itDisplay);
    }
    else if (const auto& itInternal = std::find(gameModes.InternalName.begin(), gameModes.InternalName.end(), gameMode); itInternal != gameModes.InternalName.end()) {
        gameModes.CurrentSelected = std::distance(gameModes.InternalName.begin(), itInternal);
    }
    else {
        PrintAvailableGameModes(gameModes, "Invalid game mode '" + gameMode + "'\n");
        return;
    }

    if (isHostingLocalGame()) {
        setMatchSettings();
        LOG("Updated game mode for next match.");
    }
}


void RocketPlugin::parseMapArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 3) {
        LOG("usage: rp map [map]");
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
        ERROR_LOG("Invalid map {}", quote(arguments[2]));
        return;
    }

    if (isHostingLocalGame()) {
        setMatchMapName(map.stem().string());
        LOG("Updated map for next match.");
    }
}


void RocketPlugin::parsePlayerCountArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 3) {
        LOG("usage: rp players [players]");
        return;
    }

    const std::string& players = arguments[2];
    if (!IsInt(players)) {
        ERROR_LOG("Invalid number of players {}", quote(players));
        return;
    }

    playerCount = std::max(2l, std::strtol(players.c_str(), nullptr, 10));

    if (isHostingLocalGame()) {
        WARNING_LOG("Updating player count may not update while already hosting.");
    }
}


void setTeamColor(int8_t& teamColor, const int8_t& newTeamColor)
{
    teamColor = newTeamColor;
}


void RocketPlugin::parseTeamArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 6) {
        LOG("usage: rp team [teamNum] [teamName*] [primaryColorIdx*] [accentColorIdx*] (clubMatch), *leave empty for default");
        return;
    }

    // Team number.
    const std::string& teamNumStr = arguments[2];
    const unsigned long& teamNum = std::strtoul(teamNumStr.c_str(), nullptr, 10);
    if (!IsInt(teamNumStr) || (teamNum != 0 && teamNum != 1)) {
        ERROR_LOG("Invalid team number {:s}", quote(teamNumStr));
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
        setTeamColor(teamNum == 0 ? team1PrimCol : team2PrimCol,-1);
    }
    else {
        const long primaryColorIdx = std::strtol(primaryColorStr.c_str(), nullptr, 10);
        if (!IsInt(primaryColorStr) || primaryColorIdx >= clubColors.size()) {
            ERROR_LOG("Invalid primary color index {:s}", quote(primaryColorStr));
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
            ERROR_LOG("Invalid primary color index {:s}", quote(accentColorStr));
            return;
        }
        setTeamColor(teamNum == 0 ? team1AccCol : team2AccCol, static_cast<int8_t>(accentColorIdx));
    }

    // Club match.
    if (arguments.size() > 6) {
        const std::string& clubMatchStr = arguments[6];
        if (!IsNumber<bool>(clubMatchStr)) {
            ERROR_LOG("Invalid club match {:s}", quote(clubMatchStr));
            return;
        }
        clubMatch = std::strtoul(clubMatchStr.c_str(), nullptr, 10);
    }

    if (isHostingLocalGame()) {
        setMatchSettings();
        LOG("Updated game mode for next match.");
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
        PrintAvailableMutatorValues(
            mutators[i], "Invalid value '" + mutatorValue + "' for '" + mutators[i].InternalCategoryName + "'\n");
        return;
    }

    mutators[i].CurrentSelected = j;
    LOG("Changed {} to {}", quote(mutators[i].InternalCategoryName), quote(mutators[i].InternalName[j]));

    if (isHostingLocalGame()) {
        setMatchSettings();
        LOG("Updated mutator for next match.");
    }
}


void RocketPlugin::parseRumbleArguments(const std::vector<std::string>& arguments)
{
    if (arguments.size() < 5 || !IsFloat(arguments[2]) || !IsFloat(arguments[3]) || !IsFloat(arguments[4])) {
        PrintRumbleOptions();
        return;
    }

    const float forceMultiplier = std::strtof(arguments[2].c_str(), nullptr);
    const float rangeMultiplier = std::strtof(arguments[3].c_str(), nullptr);
    const float durationMultiplier = std::strtof(arguments[4].c_str(), nullptr);
    std::shared_ptr<CrazyRumble> crazyRumble = GetCustomGameMode<CrazyRumble>();
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
    std::vector<std::string> suggestions = std::vector<std::string>();

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
                suggestions.push_back("join " + *joinIP + ":" + std::to_string(*joinPort) + " ;(password)");
            }
            else {
                suggestions.push_back("join " + arguments[2] + ":" + std::to_string(*joinPort) + " ;(password)");
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
                ERROR_LOG("no map selected.");
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
                ERROR_LOG("no map selected.");
                PushError("Hosting map failed: no map selected");
                return;
            }
            arena = currentMap;
        }
    }
#ifdef _WIN32
    /* Cos we as windows want our baguettes the left way. */
    std::replace(arena.begin(), arena.end(), '/', '\\');
#endif
    const std::string gameMode = gameModes.GetSelected();
    std::string gameTags = getGameTags();
    // Available PlayerCounts are 2, 4, 6 and 8.
    gameTags += ",PlayerCount" + std::to_string(std::clamp(playerCount, 2, 8) & ~1);
    const int numConnections = std::max(10, playerCount);
    const std::string networkOptions = "?NumPublicConnections=" + std::to_string(numConnections) +
        "?NumOpenPublicConnections=" + std::to_string(numConnections) + "?Lan?Listen";

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
        const uint64_t workshopMapId = std::strtoll(std::filesystem::path(currentMap).parent_path().stem().string().c_str(),
            nullptr, 10);
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
    const std::string command = "open " + arena + "?Playtest?game=" + gameMode + "?GameTags=" + gameTags + networkOptions;
    gameWrapper->SetTimeout([this, command = command](GameWrapper*) {
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
        const uint64_t workshopMapId = std::strtoll(std::filesystem::path(currentMap).parent_path().stem().string().c_str(),
            nullptr, 10);
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

    gameWrapper->ExecuteUnrealCommand("start " + *joinIP + ":" + std::to_string(*joinPort) + "/?Lan?Password=" + pswd);
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
                presetFile << "rp mutator " + quote(mutator.InternalCategoryName) + " " +
                    quote(mutator.GetSelected()) << std::endl;
            }
        }
    }
}


/// <summary>Loads the preset with the given filename.</summary>
/// <param name="presetPath">Path of the preset file</param>
void RocketPlugin::loadPreset(const std::filesystem::path& presetPath)
{
    resetMutators();
    TRACE_LOG("loading preset {}", quote(presetPath.u8string()));
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
        PushError("Error Fixing Map\ncheck the console (F6) for more details, for more details and report to the mod author if applicable.");
        ERROR_LOG("failed to create directory: {}, {}", quote(mapDst.parent_path().u8string()), quote(ec.message()));
        return;
    }

    ec.clear();
    /* Copies file to new directory. */
    std::filesystem::copy(mapSrc, mapDst, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        PushError("Error Fixing Map\ncheck the console (F6) for more details, for more details and report to the mod author if applicable.");
        ERROR_LOG("failed to copy file: {} to {}, {}", quote(mapSrc.u8string()), quote(mapDst.u8string()), quote(ec.message()));
        INFO_LOG("-------------------------------------------------------");
        INFO_LOG("currentJoinMap = {}", quote(absolute(currentJoinMap).u8string()));
        int i = 0;
        for (const auto& [joinableMap, _] : joinableMaps) {
            INFO_LOG("[" + std::to_string(i++) + "] = path:" + quote(joinableMap.u8string()) + ", excists: " +
                (exists(joinableMap) ? "True" : "False"));
        }
        INFO_LOG("-------------------------------------------------------");

        return;
    }

    TRACE_LOG("created file successfully.");

    refreshJoinableMaps = true;
    refreshCustomMapPaths = true;
}


/// <summary>Sets the countdown time at the start of a match.</summary>
/// <remarks>Gets called on 'TAGame.GameEvent_TA.Init'.</remarks>
/// <param name="server">The game server that started</param>
void RocketPlugin::onGameEventInit([[maybe_unused]] const ServerWrapper& server)
{
    // Clear car physics cache.
    carPhysics.clear();
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
            WARNING_LOG("could not set the search status");
        }
        gameWrapper->SetTimeout([this](GameWrapper*) {
            broadcastJoining();
        }, 1.41f);
    }
}


/*
 *  Game Controls
 */

/// <summary>Checks if you are in a game.</summary>
/// <param name="allowOnlineGame">Bool with if should check online games</param>
/// <returns>Whether you are in a game</returns>
bool RocketPlugin::IsInGame(const bool allowOnlineGame) const
{
    ServerWrapper localGame = gameWrapper->GetGameEventAsServer();
    if (!localGame.IsNull()) {
        return true;
    }
    if (allowOnlineGame) {
        ServerWrapper onlineGame = gameWrapper->GetOnlineGame();
        if (!onlineGame.IsNull()) {
            return true;
        }
    }

    return false;
}


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


/// <summary>Forces overtime in the current game.</summary>
void RocketPlugin::ForceOvertime() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.ForceOvertime();
}


/// <summary>Pauses the current game.</summary>
void RocketPlugin::PauseServer() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    PlayerControllerWrapper pc = game.GetPauser();
    if (pc.IsNull()) {
        cvarManager->executeCommand("mp_pause", false);
    }
    else {
        cvarManager->executeCommand("mp_unpause", false);
    }
}


/// <summary>Resets the current game.</summary>
void RocketPlugin::ResetMatch() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.StartNewRound();
}


/// <summary>Ends the current game.</summary>
void RocketPlugin::EndMatch() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.EndGame();
}


/// <summary>Resets the players in the current game.</summary>
void RocketPlugin::ResetPlayers() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.ResetPlayers();
}


/// <summary>Resets the balls in the current game.</summary>
void RocketPlugin::ResetBalls() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.ResetBalls();
}


/*
 *  Match Settings
 */

/// <summary>Sets max players in the current game.</summary>
/// <param name="newNumPlayers">New number of players</param>
void RocketPlugin::SetMaxPlayers(const int newNumPlayers) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetMaxPlayers(newNumPlayers);
}


/// <summary>Gets max players in the current game.</summary>
/// <returns>Max number of players</returns>
int RocketPlugin::GetMaxPlayers() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        return 16;
    }

    return game.GetMaxPlayers();
}


/// <summary>Sets max team size in the current game.</summary>
/// <param name="newTeamSize">New team size</param>
void RocketPlugin::SetMaxTeamSize(const int newTeamSize) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetMaxTeamSize(newTeamSize);
}


/// <summary>Gets max team size in the current game.</summary>
/// <returns>Max team size</returns>
int RocketPlugin::GetMaxTeamSize() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        return 3;
    }

    return game.GetMaxTeamSize();
}


/// <summary>Sets respawn time in the current game.</summary>
/// <param name="newRespawnTime">New respawn time</param>
void RocketPlugin::SetRespawnTime(const int newRespawnTime) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetRespawnTime(newRespawnTime);
}


/// <summary>Gets respawn time in the current game.</summary>
/// <returns>Respawn time</returns>
int RocketPlugin::GetRespawnTime() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        return 3;
    }

    return game.GetRespawnTime();
}


/// <summary>Sets the score of the given team in the current game.</summary>
/// <param name="team">Team to set the score of</param>
/// <param name="newScore">New score</param>
void RocketPlugin::SetScore(int team, int newScore) const
{
    switch (team) {
    case 0:
        return SetScoreBlue(newScore);
    case 1:
        return SetScoreOrange(newScore);
    default:
        ERROR_LOG("team #{} not found", team);
    }
}


/// <summary>Gets the score of the given team in the current game.</summary>
/// <param name="team">Team to get the score of</param>
/// <returns>Teams score</returns>
int RocketPlugin::GetScore(int team) const
{
    switch (team) {
    case 0:
        return GetScoreBlue();
    case 1:
        return GetScoreOrange();
    default:
        ERROR_LOG("invalid team #{}", team);
        return 0;
    }
}


/// <summary>Sets blues score in the current game.</summary>
/// <param name="newScore">New score</param>
void RocketPlugin::SetScoreBlue(const int newScore) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    ArrayWrapper<TeamWrapper> teams = game.GetTeams();
    if (teams.Count() >= 1) {
        TeamWrapper blueTeam = teams.Get(0);
        if (!blueTeam.IsNull()) {
            blueTeam.SetScore(newScore);
        }
    }
}


/// <summary>Gets blues score in the current game.</summary>
/// <returns>Blues score</returns>
int RocketPlugin::GetScoreBlue() const
{
    ServerWrapper game = GetGame(true);
    if (game.IsNull()) {
        return 0;
    }

    ArrayWrapper<TeamWrapper> teams = game.GetTeams();
    if (teams.Count() >= 1) {
        TeamWrapper blueTeam = teams.Get(0);
        if (!blueTeam.IsNull()) {
            return blueTeam.GetScore();
        }
    }

    return 0;
}


/// <summary>Sets oranges score in the current game.</summary>
/// <param name="newScore">New score</param>
void RocketPlugin::SetScoreOrange(const int newScore) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    ArrayWrapper<TeamWrapper> teams = game.GetTeams();
    if (teams.Count() >= 2) {
        TeamWrapper orangeTeam = teams.Get(1);
        if (!orangeTeam.IsNull()) {
            orangeTeam.SetScore(newScore);
        }
    }
}


/// <summary>Gets oranges score in the current game.</summary>
/// <returns>Oranges score</returns>
int RocketPlugin::GetScoreOrange() const
{
    ServerWrapper game = GetGame(true);
    if (game.IsNull()) {
        return 0;
    }

    ArrayWrapper<TeamWrapper> teams = game.GetTeams();
    if (teams.Count() >= 2) {
        TeamWrapper orangeTeam = teams.Get(1);
        if (!orangeTeam.IsNull()) {
            return orangeTeam.GetScore();
        }
    }

    return 0;
}


/// <summary>Sets the time remaining in the current game.</summary>
/// <param name="newGameTimeRemaining">The new time remaining</param>
void RocketPlugin::SetGameTimeRemaining(const int newGameTimeRemaining) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetGameTimeRemaining(static_cast<float>(newGameTimeRemaining));
}


/// <summary>Gets the time remaining in the current game.</summary>
/// <returns>The time remaining</returns>
int RocketPlugin::GetGameTimeRemaining() const
{
    ServerWrapper game = GetGame(true);
    if (game.IsNull()) {
        return 0;
    }

    return static_cast<int>(ceil(game.GetGameTimeRemaining()));
}


/// <summary>Sets if the goal delay is disabled in the current game.</summary>
/// <param name="isGoalDelayDisabled">Bool with if the goal delay should be disabled</param>
void RocketPlugin::SetIsGoalDelayDisabled(const bool isGoalDelayDisabled) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetbDisableGoalDelay(isGoalDelayDisabled);
}


/// <summary>Gets if the goal delay is disabled in the current game.</summary>
/// <returns>Bool with if the goal delay is disabled</returns>
bool RocketPlugin::GetIsGoalDelayDisabled() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        return false;
    }

    return game.GetbDisableGoalDelay();
}


/// <summary>Sets if there is unlimited time in the current game.</summary>
/// <param name="isUnlimitedTime">Bool with if there should be unlimited time</param>
void RocketPlugin::SetIsUnlimitedTime(const bool isUnlimitedTime) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetbUnlimitedTime(isUnlimitedTime);
}


/// <summary>Gets if there is unlimited time in the current game.</summary>
/// <returns>bool with if there is unlimited time</returns>
bool RocketPlugin::GetIsUnlimitedTime() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        return false;
    }

    return game.GetbUnlimitedTime();
}


/*
 *  Bots
 */

/// <summary>Sets the number of bots per team in the current game.</summary>
/// <param name="newMaxNumBots">The new maximum number of bots per team</param>
void RocketPlugin::SetMaxNumBots(const bool newMaxNumBots) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetNumBots(newMaxNumBots);
}


/// <summary>Gets the maximum number of bots per team in the current game.</summary>
/// <returns>The maximum number of bots per team</returns>
int RocketPlugin::GetMaxNumBots() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        return 0;
    }

    return game.GetNumBots();
}


/// <summary>Sets the number of bots per team in the game.</summary>
/// <param name="newNumBotsPerTeam">The new number of bots per team</param>
void RocketPlugin::SetNumBotsPerTeam(const int newNumBotsPerTeam) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    const int maxTeamSize = game.GetMaxTeamSize();
    const bool isFilledWithAI = game.GetbFillWithAI();
    game.SetMaxTeamSize(newNumBotsPerTeam);
    game.SetbFillWithAI(true);

    const int oldMaxNumBotsPerTeam = game.GetNumBots();
    game.SetNumBots(newNumBotsPerTeam);
    // UpdateBotCount() only adds/removes one bot at a time.
    for (int botsToAdd = std::max(oldMaxNumBotsPerTeam, newNumBotsPerTeam) * 2; botsToAdd > 0; --botsToAdd) {
        game.UpdateBotCount();
    }

    game.SetMaxTeamSize(maxTeamSize);
    game.SetbFillWithAI(isFilledWithAI);
}


/// <summary>Gets the number of bots per team in the game.</summary>
/// <returns>The number of bots per team</returns>
int RocketPlugin::GetNumBotsPerTeam() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        return 0;
    }

    return game.GetNumBots();
}


/// <summary>Sets if bots are auto filled in the current game.</summary>
/// <param name="isAutoFilledWithBots">Bool with if bots should be auto filled</param>
void RocketPlugin::SetIsAutoFilledWithBots(const bool isAutoFilledWithBots) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetbFillWithAI(isAutoFilledWithBots);
}


/// <summary>Gets if bots are auto filled in the current game.</summary>
/// <returns>Bool with if bots are auto filled</returns>
bool RocketPlugin::GetIsAutoFilledWithBots() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        return false;
    }

    return game.GetbFillWithAI();
}


/// <summary>Sets if teams are unfair in the current game.</summary>
/// <param name="isUnfairTeams">Bool with if teams should be unfair</param>
void RocketPlugin::SetIsUnfairTeams(const bool isUnfairTeams) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetbUnfairTeams(isUnfairTeams);
}


/// <summary>Gets if teams are unfair in the current game.</summary>
/// <returns>bool with if teams are unfair</returns>
bool RocketPlugin::GetIsUnfairTeams() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        return false;
    }

    return game.GetbUnfairTeams();
}


/// <summary>Freezes or unfreezes all bots.</summary>
void RocketPlugin::FreezeBots() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    bool firstBot = true;
    bool shouldUnFreeze = false;
    for (CarWrapper car : game.GetCars()) {
        if (car.IsNull() || car.GetPRI().IsNull() || !car.GetPRI().GetbBot()) {
            WARNING_LOG("could not get the car");
            continue;
        }

        if (firstBot) {
            shouldUnFreeze = car.GetbFrozen();
            firstBot = false;
        }

        if (shouldUnFreeze) {
            car.SetFrozen(0);
        }
        else {
            car.SetFrozen(1);
        }
    }
}


/*
 *  Ball Mods
 */

/// <summary>Sets the number of balls in the current game.</summary>
/// <param name="newNumBalls">The new number of balls</param>
void RocketPlugin::SetNumBalls(const int newNumBalls) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetTotalGameBalls(newNumBalls);

    const float ballScale = GetBallsScale();
    ResetBalls();
    SetBallsScale(ballScale);
}


/// <summary>Gets the number of balls in the current game.</summary>
/// <returns>The number of balls</returns>
int RocketPlugin::GetNumBalls() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        return 0;
    }

    return game.GetTotalGameBalls();
}


/// <summary>Sets the scale of the balls in the current game.</summary>
/// <param name="newBallsScale">The new scale of the balls</param>
void RocketPlugin::SetBallsScale(float newBallsScale) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }
    /* The game crashes with negative ball scale */
    if (newBallsScale <= 0) {
        WARNING_LOG("ball scale out of bounds");
        newBallsScale = 1.0f;
    }

    game.ResetBalls();
    for (BallWrapper ball : game.GetGameBalls()) {
        if (ball.IsNull()) {
            WARNING_LOG("could not get the ball");
            continue;
        }

        ball.SetBallScale(newBallsScale);
    }
}


/// <summary>Gets the scale of the balls in the current game.</summary>
/// <returns>The scale of the balls</returns>
float RocketPlugin::GetBallsScale() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        return 1.0f;
    }

    BallWrapper ball = game.GetBall();
    if (ball.IsNull()) {
        return 1.0f;
    }

    const float ballScale = ball.GetReplicatedBallScale();

    return ballScale > 0 ? ballScale : 1.0f;
}


void RocketPlugin::SetMaxBallVelocity(const float newMaxBallVelocity) const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    for (BallWrapper ball : game.GetGameBalls()) {
        if (ball.IsNull()) {
            WARNING_LOG("could not get the ball");
            continue;
        }

        ball.SetMaxLinearSpeed(newMaxBallVelocity);
    }
}


float RocketPlugin::GetMaxBallVelocity() const
{
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        return 6000.0f;
    }

    BallWrapper ball = game.GetBall();
    if (ball.IsNull()) {
        return 6000.0f;
    }

    return ball.GetMaxLinearSpeed();
}


/*
 *  Player Mods
 */

/// <summary>Gets the players in the current game.</summary>
/// <param name="includeBots">Bool with if the output should include bots</param>
/// <param name="mustBeAlive">Bool with if the output should only include alive players</param>
/// <returns>List of players</returns>
std::vector<PriWrapper> RocketPlugin::GetPlayers(const bool includeBots, const bool mustBeAlive) const
{
    std::vector<PriWrapper> players;
    ServerWrapper game = GetGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return players;
    }

    if (mustBeAlive) {
        for (CarWrapper car : game.GetCars()) {
            if (car.IsNull() || car.GetPRI().IsNull() || (!includeBots && car.GetPRI().GetbBot())) {
                continue;
            }

            players.push_back(car.GetPRI());
        }
    }
    else {
        for (PriWrapper pri : game.GetPRIs()) {
            if (pri.IsNull() || (!includeBots && pri.GetbBot())) {
                continue;
            }

            players.push_back(pri);
        }
    }

    return players;
}


/// <summary>Gets the player names of the players in the current game.</summary>
/// <param name="includeBots">Bool with if the output should include bots</param>
/// <param name="mustBeAlive">Bool with if the output should only include alive players</param>
/// <returns>List of players names</returns>
std::vector<std::string> RocketPlugin::GetPlayersNames(const bool includeBots, const bool mustBeAlive) const
{
    return GetPlayersNames(GetPlayers(includeBots, mustBeAlive));
}


/// <summary>Gets the player names of the given players.</summary>
/// <param name="players">List of players to get the names from</param>
/// <returns>List of players names</returns>
std::vector<std::string> RocketPlugin::GetPlayersNames(const std::vector<PriWrapper>& players) const
{
    std::vector<std::string> playersNames;
    for (PriWrapper player : players) {
        playersNames.push_back(player.GetPlayerName().ToString());
    }

    return playersNames;
}


/// <summary>Sets if the players is admin in the current game.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to update</param>
/// <param name="isAdmin">Bool with if the player should be admin</param>
void RocketPlugin::SetIsAdmin(PriWrapper player, const bool isAdmin) const
{
    if (player.IsNull()) {
        ERROR_LOG("could not get the player");
        return;
    }

    player.SetbMatchAdmin(isAdmin);
}


/// <summary>Gets if the players is admin in the current game.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to check</param>
/// <returns>Bool with if the players is admin</returns>
bool RocketPlugin::GetIsAdmin(PriWrapper player) const
{
    if (player.IsNull()) {
        ERROR_LOG("could not get the player");
        return false;
    }

    return player.GetbMatchAdmin();
}


/// <summary>Sets if the players is hidden in the current game.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to update</param>
/// <param name="isHidden">Bool with if the player should be hidden</param>
void RocketPlugin::SetIsHidden(PriWrapper player, const bool isHidden) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    player.GetCar().SetHidden2(isHidden);
    player.GetCar().SetbHiddenSelf(isHidden);
}


/// <summary>Gets if the players is hidden in the current game.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to check</param>
/// <returns>Bool with if the players are hidden</returns>
bool RocketPlugin::GetIsHidden(PriWrapper player) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return false;
    }

    return player.GetCar().GetbHidden();
}


/// <summary>Demolishes the given player.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to demolish</param>
void RocketPlugin::Demolish(PriWrapper player) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    player.GetCar().Demolish();
}


/*
 *  Car Physics mods
 */

RocketPlugin::CarPhysics::CarPhysics(RocketPlugin* rp, PriWrapper player)
{
    CarScale = rp->GetCarScale(player);
    CarHasCollision = rp->GetbCarCollision(player);
    CarIsFrozen = rp->GetCarIsFrozen(player);
    TorqueRate = rp->GetTorqueRate(player);
    MaxCarVelocity = rp->GetMaxCarVelocity(player);
    GroundStickyForce = rp->GetGroundStickyForce(player);
    WallStickyForce = rp->GetWallStickyForce(player);
}


/// <summary>Sets the car physics for the player.</summary>
/// <remarks>Gets called on 'TAGame.Car_TA.EventVehicleSetup'.</remarks>
/// <param name="car">The players car to set the physics of</param>
void RocketPlugin::SetPhysics(CarWrapper car)
{
    if (gameWrapper->GetGameEventAsServer().IsNull()) {
        return;
    }
    if (car.IsNull() || car.GetPRI().IsNull()) {
        ERROR_LOG("could not get the cars pri");
        return;
    }
    const auto& it = carPhysics.find(car.GetPRI().GetUniqueIdWrapper().GetUID());
    if (it == carPhysics.end()) {
        return;
    }

    const PriWrapper player = car.GetPRI();
    const CarPhysics playerCarPhysics = it->second;

    SetCarScale(player, playerCarPhysics.CarScale);
    SetbCarCollision(player, playerCarPhysics.CarHasCollision);
    SetCarIsFrozen(player, playerCarPhysics.CarIsFrozen);
    SetTorqueRate(player, playerCarPhysics.TorqueRate);
    SetMaxCarVelocity(player, playerCarPhysics.MaxCarVelocity);
    SetGroundStickyForce(player, playerCarPhysics.GroundStickyForce);
    SetWallStickyForce(player, playerCarPhysics.WallStickyForce);
}


/// <summary>Gets the car physics for the player.</summary>
/// <param name="player">The player to get the car physics from</param>
/// <returns>The car physics for the player</returns>
RocketPlugin::CarPhysics RocketPlugin::GetPhysics(PriWrapper player)
{
    if (player.IsNull()) {
        ERROR_LOG("could not get the player");
        return CarPhysics(this, player);
    }

    const uint64_t steamID = player.GetUniqueIdWrapper().GetUID();
    const auto& it = carPhysics.find(steamID);
    if (it == carPhysics.end()) {
        return CarPhysics(this, player);
    }

    return it->second;
}


/// <summary>Gets the car physics for the player and saves them when not found.</summary>
/// <param name="player">The player to get the car physics from</param>
/// <returns>The car physics for the player</returns>
RocketPlugin::CarPhysics& RocketPlugin::GetPhysicsCache(PriWrapper player)
{
    const uint64_t steamID = player.GetUniqueIdWrapper().GetUID();
    return carPhysics.try_emplace(steamID, CarPhysics(this, player)).first->second;
}


/// <summary>Sets the if the players car has collision in the current game.</summary>
/// <param name="player">the player to update the car of</param>
/// <param name="carHasCollision">Bool with if the players car should have collision</param>
void RocketPlugin::SetbCarCollision(PriWrapper player, const bool carHasCollision)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    GetPhysicsCache(player).CarHasCollision = carHasCollision;
    player.GetCar().SetbCollideActors(carHasCollision);
}


/// <summary>Gets the if the players car has collision in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>Bool with if the players car has collision</returns>
bool RocketPlugin::GetbCarCollision(PriWrapper player) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return true;
    }

    return player.GetCar().GetbCollideActors();
}


/// <summary>Sets the players car scale in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="newCarScale">The players car scale</param>
/// <param name="shouldRespawn">Bool with if the car should respawn in place</param>
void RocketPlugin::SetCarScale(PriWrapper player, const float newCarScale, const bool shouldRespawn)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    GetPhysicsCache(player).CarScale = newCarScale;
    player.GetCar().SetCarScale(newCarScale);
    if (shouldRespawn) {
        player.GetCar().RespawnInPlace();
    }
}


/// <summary>Gets the players car scale in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The players car scale</returns>
float RocketPlugin::GetCarScale(PriWrapper player) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return 1.0f;
    }

    const float carScale = player.GetCar().GetReplicatedCarScale();

    return carScale > 0 ? carScale : 1.0f;
}


/// <summary>Sets the if the players car should be frozen in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="carIsFrozen">Bool with if the players car should be frozen</param>
void RocketPlugin::SetCarIsFrozen(PriWrapper player, const bool carIsFrozen)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    GetPhysicsCache(player).CarIsFrozen = carIsFrozen;
    player.GetCar().SetFrozen(carIsFrozen);
}


/// <summary>Gets the if the players car is frozen in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <returns>Bool with if the players car is frozen</returns>
bool RocketPlugin::GetCarIsFrozen(PriWrapper player) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return false;
    }

    return player.GetCar().GetbFrozen();
}


/// <summary>Sets the players car drive torque in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="torqueRate">The new drive torque of the players car</param>
void RocketPlugin::SetTorqueRate(PriWrapper player, const float torqueRate)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    GetPhysicsCache(player).TorqueRate = torqueRate;
    player.GetCar().GetVehicleSim().SetDriveTorque(torqueRate * 100000);
}


/// <summary>Gets the players car drive torque in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The drive torque of the players car</returns>
float RocketPlugin::GetTorqueRate(PriWrapper player) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return 2.88f;
    }

    return player.GetCar().GetVehicleSim().GetDriveTorque() / 100000;
}


/// <summary>Sets the players car max velocity in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="maxCarVelocity">The new max velocity of the players car</param>
void RocketPlugin::SetMaxCarVelocity(PriWrapper player, const float maxCarVelocity)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    GetPhysicsCache(player).MaxCarVelocity = maxCarVelocity;
    player.GetCar().SetMaxLinearSpeed(maxCarVelocity);
}


/// <summary>Gets the players car max velocity in the current game.</summary>
/// <param name="player">the player to check the car of</param>
/// <returns>The max velocity of the players car</returns>
float RocketPlugin::GetMaxCarVelocity(PriWrapper player) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return 2300.0f;
    }

    return player.GetCar().GetMaxLinearSpeed();
}


/// <summary>Gets the players car sticky force in the current game.</summary>
/// <param name="player">The player to get the sticky force of the car of</param>
/// <returns>The sticky force of the players car</returns>
StickyForceData RocketPlugin::GetStickyForce(PriWrapper player) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return StickyForceData();
    }

    return player.GetCar().GetStickyForce();
}


/// <summary>Sets the players car ground sticky force in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="groundStickyForce">The new ground sticky force of the players car</param>
void RocketPlugin::SetGroundStickyForce(PriWrapper player, const float groundStickyForce)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    StickyForceData sfd = GetStickyForce(player);
    sfd.Ground = groundStickyForce;

    GetPhysicsCache(player).GroundStickyForce = groundStickyForce;
    player.GetCar().SetStickyForce(sfd);
}


/// <summary>Gets the players car ground sticky force in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The ground sticky force of the players car</returns>
float RocketPlugin::GetGroundStickyForce(PriWrapper player) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return 0.5f;
    }

    return player.GetCar().GetStickyForce().Ground;
}


/// <summary>Sets the players car wall sticky force in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="wallStickyForce">The new wall sticky force of the players car</param>
void RocketPlugin::SetWallStickyForce(PriWrapper player, const float wallStickyForce)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    StickyForceData sfd = GetStickyForce(player);
    sfd.Wall = wallStickyForce;

    GetPhysicsCache(player).WallStickyForce = wallStickyForce;
    player.GetCar().SetStickyForce(sfd);
}


/// <summary>Gets the players car wall sticky force in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The wall sticky force of the players car</returns>
float RocketPlugin::GetWallStickyForce(PriWrapper player) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return 1.5f;
    }

    return player.GetCar().GetStickyForce().Wall;
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
        std::filesystem::create_directory(BakkesModCrashesFolder);
    }
    RocketPluginDataFolder = gameWrapper->GetDataFolder() / L"RocketPlugin";
    if (!exists(RocketPluginDataFolder)) {
        std::filesystem::create_directory(RocketPluginDataFolder);
    }
    RocketLeagueExecutableFolder = std::filesystem::current_path();

    // Copy the original cvarManager so we can use it everywhere.
    GlobalCVarManager = std::reinterpret_pointer_cast<CVarManagerWrapperDebug>(cvarManager);

    /* Register CVars */
    joinIP = std::make_shared<std::string>();
    cvarManager->registerCvar("mp_ip", "127.0.0.1",
        "Default ip for joining local matches").bindTo(joinIP);

    joinPort = std::make_shared<int>(DEFAULT_PORT);
    cvarManager->registerCvar("mp_port", std::to_string(DEFAULT_PORT),
        "Default port for joining local matches").bindTo(joinPort);

    presetDirPath = std::make_shared<std::string>();
    cvarManager->registerCvar("rp_preset_path", PRESETS_PATH.string(),
        "Default path for the mutator presets directory").bindTo(presetDirPath);

    workshopMapDirPath = std::make_shared<std::string>();
    cvarManager->registerCvar("rp_workshop_path", WORKSHOP_MAPS_PATH.string(),
        "Default path for your workshop maps directory").bindTo(workshopMapDirPath);

    customMapDirPath = std::make_shared<std::string>();
    cvarManager->registerCvar("rp_custom_path", CUSTOM_MAPS_PATH.string(),
        "Default path for your custom maps directory").bindTo(customMapDirPath);

    cvarManager->registerCvar("rp_gui_keybind", DEFAULT_GUI_KEYBIND, "Keybind for the gui");

    LogLevel = std::make_shared<int>(0);
    cvarManager->registerCvar("rp_log_level", std::to_string(CVarManagerWrapperDebug::level_enum::normal),
        "Log level", true, false, 0, false, 0, false).bindTo(LogLevel);

#ifdef DEBUG
    cvarManager->getCvar("rp_log_level").setValue(CVarManagerWrapperDebug::level_enum::all);

    showDemoWindow = std::make_shared<bool>(false);
    cvarManager->registerCvar("rp_show_demo_window", "0",
        "Shows the Dear ImGui demo window", true, false, 0, false, 0, false).bindTo(showDemoWindow);

    showMetricsWindow = std::make_shared<bool>(false);
    cvarManager->registerCvar("rp_show_metrics_window", "0",
        "Shows the Dear ImGui metrics window", true, false, 0, false, 0, false).bindTo(showMetricsWindow);
#endif

    /* Register Notifiers */
    // TODO, add command completer.
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
        LOG("Set {} to {}", quote(key), quote(command));
    }, "Adds a keybind for " + quote("togglemenu " + GetMenuName()) + " as $rp_gui_keybind or given argument.", PERMISSION_ALL);

    RegisterNotifier("rp_broadcast_game", [this](const std::vector<std::string>&) {
        broadcastJoining();
    }, "Broadcasts a game invite to your party members.", PERMISSION_SOCCAR);

    RegisterNotifier("rp_add_beta_game_modes", [this](const std::vector<std::string>&) {
        if (GetCustomGameMode<SmallCars>() == nullptr) {
            customGameModes.push_back(std::make_shared<SmallCars>(this));
            TRACE_LOG("added Small Cars game mode");
        }
        else {
            WARNING_LOG("Small Cars game mode already added");
        }
    }, "Adds beta game modes.", PERMISSION_ALL);

    registerExternalNotifiers();

    /* Register Hooks */
    HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.EventVehicleSetup",
        [this](const CarWrapper& caller, void*, const std::string&) {
            SetPhysics(caller);
        });

    HookEventWithCaller<ServerWrapper>("Function TAGame.GameEvent_TA.Init",
        [this](const ServerWrapper& caller, void*, const std::string&) {
            onGameEventInit(caller);
        });

    registerExternalHooks();

    // Load RL Constants.
    loadRLConstants();

    // Load Constants Config.
    ConstantsConfig = std::make_unique<RPConfig>(DEFAULT_CONSTANTS_CONFIG_URL);

    /* GUI Settings */

    // Set the window bind to the default keybind if is not set.
    if (!IsGUIWindowBound(GetMenuName())) {
        cvarManager->setBind(DEFAULT_GUI_KEYBIND, "togglemenu " + GetMenuName());
        LOG("Set window keybind to {}", DEFAULT_GUI_KEYBIND);
    }

    // Load the custom mutator presets.
    presetPaths = GetFilesFromDir(*presetDirPath, 1, ".cfg");

    /* Init Networking */
    upnpClient = std::make_shared<UPnPClient>();
    p2pHost = std::make_shared<P2PHost>();

    /* Init Game Modes */
    customGameModes.push_back(std::make_shared<Drainage>(this));
    customGameModes.push_back(std::make_shared<CrazyRumble>(this));
    customGameModes.push_back(std::make_shared<Zombies>(this));
    customGameModes.push_back(std::make_shared<BoostSteal>(this));
    customGameModes.push_back(std::make_shared<KeepAway>(this));
    customGameModes.push_back(std::make_shared<Tag>(this));
    customGameModes.push_back(std::make_shared<Juggernaut>(this));
    customGameModes.push_back(std::make_shared<BoostMod>(this));
    customGameModes.push_back(std::make_shared<BoostShare>(this));
    customGameModes.push_back(std::make_shared<SacredGround>(this));
}


/// <summary>Unload the plugin properly.</summary>
void RocketPlugin::OnUnload()
{
    // Save all CVars to 'config.cfg'.
    cvarManager->backupCfg(CONFIG_FILE_PATH.string());
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
