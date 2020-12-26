// RocketPlugin.cpp
// A BakkesMod plugin for joining, hosting and manipulating multiplayer games.
// 
// Author:        Stanbroek
// Version:       0.6.4 24/12/20
// BMSDK version: 95

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

BAKKESMOD_PLUGIN(RocketPlugin, "Rocket Plugin", PLUGIN_VERSION, 0x0)

std::filesystem::path BakkesModConfigFolder;
std::filesystem::path RocketPluginDataFolder;
std::filesystem::path RocketLeagueExecutableFolder;

std::shared_ptr<int> LogLevel;
std::shared_ptr<CVarManagerWrapperDebug> GlobalCVarManager;


/*
 *  Match File Helpers
 */

 /// <summary>Checks if the given file extension is in the list of extensions.</summary>
 /// <param name="fileExtension">File extension</param>
 /// <param name="extensions">List of file extensions</param>
 /// <returns>Bool with if the file extension is in the list of extensions</returns>
bool RocketPlugin::HasExtension(const std::string& fileExtension, const std::vector<std::string>& extensions)
{
    /* Filter out unwanted file extensions. */
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
    const std::filesystem::directory_iterator end{};
    for (const std::filesystem::directory_entry& file : std::filesystem::directory_iterator(directory)) {
        const std::filesystem::path& filePath = file.path();
        if (file.is_directory()) {
            std::vector<std::filesystem::path> directoryFiles = IterateDirectory(
                filePath, extensions, depth + 1, maxDepth);
            /* Remove if directory is empty. */
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


/// <summary>Gets workshop maps from the given directory.</summary>
/// <param name="workshopPath">Path to the workshop directory to get the maps from</param>
/// <param name="extensions">List of map extensions to filter by</param>
/// <param name="preferredExtension">Map extension to prefer when multiple files are found</param>
/// <returns>The workshop maps from the given directory</returns>
std::vector<std::filesystem::path> RocketPlugin::getWorkshopMaps(const std::filesystem::path& workshopPath,
                                                                 const std::vector<std::string>& extensions,
                                                                 const std::string& preferredExtension)
{
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
    for (const std::string& value : mutator.InternalName) {
        error += "\t" + quote(value) + "\n";
    }

    LOG(error.substr(0, error.size() - 1));
}


/// <summary>Finds the index of a given string the mutator values list after being sanitized.</summary>
/// <param name="mutator"><see cref="RocketPlugin::Mutator"/>'s to get the values from</param>
/// <param name="mutatorValueToFind">Mutator value to find the index of</param>
/// <returns>The index of a given string the mutator values list or -1 or it was not found</returns>
int FindSanitizedIndexInMutatorValues(const RocketPlugin::GameSetting& mutator, std::string mutatorValueToFind)
{
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


/// <summary>Checks if the given string is a float.</summary>
/// <param name="myString">String to check if it is a float</param>
/// <returns>Bool with is the string is a float</returns>
bool IsFloat(const std::string& myString) {
    std::istringstream iss(myString);
    float f;
    iss >> std::noskipws >> f; // std::noskipws considers leading whitespace invalid.

    // Check the entire string was consumed and if either the bad bit or the fail bit is set.
    return iss.eof() && !iss.fail();
}


/// <summary>Parses the arguments for the `rp` command.</summary>
/// <param name="arguments">Arguments given with the `rp` command</param>
void RocketPlugin::parseArguments(std::vector<std::string> arguments)
{
    if (arguments.size() < 2) {
        PrintCommandOptions();
        return;
    }

    if (arguments[1] == "join") {
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

        joinGame(pswd.c_str());
        return;
    }

    if (arguments[1] == "mutator") {
        if (refreshRLConstants) {
            refreshRLConstants = false;
            loadRLConstants();
        }

        if (arguments.size() < 3) {
            PrintAvailableMutators(mutators);
            return;
        }

        // TODO, use internal mutator names.
        const int i = FindSanitizedIndexInMutators(mutators, arguments[2]);
        if (i == -1) {
            PrintAvailableMutators(mutators, "Invalid mutator '" + arguments[2] + "'\n");
            return;
        }

        if (arguments.size() < 4) {
            PrintAvailableMutatorValues(mutators[i]);
            return;
        }

        const int j = FindSanitizedIndexInMutatorValues(mutators[i], arguments[3]);
        if (j == -1) {
            PrintAvailableMutatorValues(
                mutators[i], "Invalid value '" + arguments[3] + "' for '" + mutators[i].InternalCategoryName + "'\n");
            return;
        }

        mutators[i].CurrentSelected = j;
        LOG("Changed '" + mutators[i].InternalCategoryName + "' to '" + mutators[i].InternalName[j] + "'");
        return;
    }

    if (arguments[1] == "rumble") {
        if (arguments.size() < 5 || !IsFloat(arguments[2]) || !IsFloat(arguments[3]) || !IsFloat(arguments[4])) {
            PrintRumbleOptions();
            return;
        }

        const float forceMultiplier =    std::strtof(arguments[2].c_str(), nullptr);
        const float rangeMultiplier =    std::strtof(arguments[3].c_str(), nullptr);
        const float durationMultiplier = std::strtof(arguments[4].c_str(), nullptr);
        // This should work as long as the order in OnLoad() does not get changed.
        CrazyRumble* crazyRumble = dynamic_cast<CrazyRumble*>(customGameModes[1].get());
        crazyRumble->setItemValues(forceMultiplier, rangeMultiplier, durationMultiplier);

        return;
    }

    PrintCommandOptions("Invalid option '" + arguments[1] + "'\n");
}


/// <summary>Autocomplete the `rp` command.</summary>
/// <remarks>Not available in the public BakkesMod version.</remarks>
/// <param name="arguments">Arguments given with the `rp` command</param>
/// <returns>List of suggestions</returns>
std::vector<std::string> RocketPlugin::complete(std::vector<std::string> arguments)
{
    std::vector<std::string> suggestions = std::vector<std::string>();

    if (arguments.size() == 2 && arguments[1].empty()) {
        suggestions.emplace_back("join ;[ip](:port) (password)");
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
 *  Host Match
 */

/// <summary>Hosts a local game with the preconfigured settings.</summary>
void RocketPlugin::hostGame()
{
    std::string arena;
    if (enableWorkshopMaps || enableCustomMaps) {
        if (customMapPaths.find(currentMap) == customMapPaths.end()) {
            LOG("No map selected.");
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
            LOG("No map selected.");
            return;
        }
        arena = currentMap;
    }
#ifdef _WIN32
    /* Cos we as windows want our baguettes the left way. */
    std::replace(arena.begin(), arena.end(), '/', '\\');
#endif
    const std::string gameMode = gameModes.GetSelected();
    std::string gameTags;
    for (const GameSetting& mutator : mutators) {
        if (!mutator.GetSelected().empty()) {
            gameTags += mutator.GetSelected() + ",";
        }
    }
    gameTags += botDifficulties.GetSelected() + ",";
    gameTags += "PlayerCount" + std::to_string(playerCount);
    const int numConnections = std::max(10, playerCount);
    const std::string networkOptions = "?NumPublicConnections=" + std::to_string(numConnections) +
        "?NumOpenPublicConnections=" + std::to_string(numConnections) + "?Lan?Listen";

    setMatchSettings(arena);

    hostingGame = true;
    if (hostWithParty) {
        shouldInviteParty = setSearchStatus(L"Searching", true);
    }
    if (enableWorkshopMaps || enableCustomMaps) {
        loadingScreenHooked = true;
        loadingScreenMapName = to_wstring(customMapPaths[currentMap]);
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
    // Disable "Show boost usage" because Bakkes fixed it again.
    cvarManager->getCvar("cl_soccar_boostcounter").setValue(false);

    // Delay this so the search status can be send before loading.
    const std::string command = "open " + arena + "?Playtest?game=" + gameMode + "?GameTags=" + gameTags + networkOptions;
    gameWrapper->SetTimeout([this, command = command](GameWrapper*) {
        gameWrapper->ExecuteUnrealCommand(command);
    }, 0.1f);
}


/// <summary>Sets the countdown time at the start of a match.</summary>
/// <remarks>Gets called on 'TAGame.GameEvent_TA.Init'.</remarks>
/// <param name="server">The game server that started</param>
void RocketPlugin::gameEventInit([[maybe_unused]] const ServerWrapper& server)
{
    // Clear car physics cache.
    carPhysics.clear();
    isJoiningHost = false;

    if (!hostingGame) {
        return;
    }

    hostingGame = false;

    if (enableWorkshopMaps || enableCustomMaps) {
        setMatchMapName(std::filesystem::path(currentMap).stem().string());
    }
    else {
        setMatchMapName(currentMap);
    }
}


/// <summary>Joins a local game with the preconfigured settings.</summary>
/// <param name="pswd">Password the local game is protected with</param>
void RocketPlugin::joinGame(const char* pswd)
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
            copyMap(absolute(currentJoinMap));
            return;
        }
    }

    gameWrapper->ExecuteUnrealCommand("start " + *joinIP + ":" + std::to_string(*joinPort) + "/?Lan?Password=" + pswd);
}


/// <summary>Joins the game after loading the map.</summary>
/// <remarks>Gets called post 'TAGame.LoadingScreen_TA.HandlePostLoadMap'.</remarks>
void RocketPlugin::onPostLoadingScreen()
{
    if (shouldInviteParty) {
        shouldInviteParty = false;
        if (!setSearchStatus(L"Joining")) {
            WARNING_LOG("could not set search status");
        }
        gameWrapper->SetTimeout([this](GameWrapper*) {
            broadcastJoining();
        }, 1.41f);
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
    const std::filesystem::path mapDst = L"../../TAGame/CookedPCConsole/rocketplugin/" + mapSrc.stem().wstring() + L".upk";

    std::error_code ec;
    /* Creates parent directory. */
    if (!exists(mapDst.parent_path()) && !create_directories(mapDst.parent_path(), ec)) {
        errors.push("Error Fixing Map\ncheck the console (F6), for more details and report to the mod author if applicable.");
        LOG("Error creating directory: " + quote(mapDst.parent_path().u8string()) + " " + ec.message());
        return;
    }

    ec.clear();
    if (!exists(mapDst)) {
        /* Copies file to new directory. */
        std::filesystem::copy(mapSrc, mapDst, ec);
        if (ec) {
            errors.push("Error Fixing Map\ncheck the console (F6), for more details and report to the mod author if applicable.");
            LOG("Error copying file: " + quote(mapSrc.u8string()) + "," + quote(mapDst.u8string()) + " " + ec.message());
            LOG("-------------------------------------------------------");
            LOG("currentJoinMap = " + absolute(currentJoinMap).u8string());
            int i = 0;
            for (const auto& [joinableMap, _] : joinableMaps) {
                LOG("[" + std::to_string(i++) + "] = path:" + quote(joinableMap.u8string()) + ", excists: " +
                    (exists(joinableMap) ? "True" : "False"));
            }
            LOG("-------------------------------------------------------");

            return;
        }

        LOG("Created file successfully.");
    }
    else {
        LOG("Map already exists.");
    }

    refreshJoinableMaps = true;
    refreshCustomMapPaths = true;
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


/// <summary>Resets the mutators selectors.</summary>
void RocketPlugin::resetMutators()
{
    for (GameSetting& mutator : mutators) {
        mutator.CurrentSelected = 0;
    }
}


/// <summary>Loads the preset with the given filename.</summary>
/// <param name="presetPath">Path of the preset file</param>
void RocketPlugin::loadPreset(const std::filesystem::path& presetPath)
{
    resetMutators();
    TRACE_LOG("Loading preset " + presetPath.u8string());
    cvarManager->loadCfg(presetPath.generic_string());
}


/*
 *  Game Controls
 */

 /// <summary>Gets the current game as <see cref="ServerWrapper"/>.</summary>
 /// <param name="allowOnlineGame">Bool with if should try to get a online game</param>
 /// <returns>The current game as <see cref="ServerWrapper"/> or <c>NULL</c> is no game is found</returns>
ServerWrapper RocketPlugin::getGame(const bool allowOnlineGame) const
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
        ERROR_LOG("could not get the online game");
    }
    else {
        ERROR_LOG("could not get local game");
    }

    return NULL;
}


 /// <summary>Forces overtime in the current game.</summary>
void RocketPlugin::forceOvertime() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.ForceOvertime();
}


/// <summary>Pauses the current game.</summary>
void RocketPlugin::pauseServer() const
{
    ServerWrapper game = getGame();
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
void RocketPlugin::resetMatch() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.StartNewRound();
}


/// <summary>Resets the players in the current game.</summary>
void RocketPlugin::resetPlayers() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.ResetPlayers();
}


/// <summary>Resets the balls in the current game.</summary>
void RocketPlugin::resetBalls() const
{
    ServerWrapper game = getGame();
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
void RocketPlugin::setMaxPlayers(const int newNumPlayers) const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetMaxPlayers(newNumPlayers);
}


/// <summary>Gets max players in the current game.</summary>
/// <returns>Max number of players</returns>
int RocketPlugin::getMaxPlayers() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return 16;
    }

    return game.GetMaxPlayers();
}


/// <summary>Sets max team size in the current game.</summary>
/// <param name="newTeamSize">New team size</param>
void RocketPlugin::setMaxTeamSize(const int newTeamSize) const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetMaxTeamSize(newTeamSize);
}


/// <summary>Gets max team size in the current game.</summary>
/// <returns>Max team size</returns>
int RocketPlugin::getMaxTeamSize() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return 3;
    }

    return game.GetMaxTeamSize();
}


/// <summary>Sets respawn time in the current game.</summary>
/// <param name="newRespawnTime">New respawn time</param>
void RocketPlugin::setRespawnTime(const int newRespawnTime) const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetRespawnTime(newRespawnTime);
}


/// <summary>Gets respawn time in the current game.</summary>
/// <returns>Respawn time</returns>
int RocketPlugin::getRespawnTime() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return 3;
    }

    return game.GetRespawnTime();
}


/// <summary>Sets the score of the given team in the current game.</summary>
/// <param name="team">Team to set the score of</param>
/// <param name="newScore">New score</param>
void RocketPlugin::setScore(int team, int newScore) const
{
    switch (team) {
        case 0:
            return setScoreBlue(newScore);
        case 1:
            return setScoreOrange(newScore);
        default:
            ERROR_LOG("Team #{} not found", team);
            break;
    }
}


/// <summary>Gets the score of the given team in the current game.</summary>
/// <param name="team">Team to get the score of</param>
/// <returns>Teams score</returns>
int RocketPlugin::getScore(int team) const
{
    switch (team) {
    case 0:
        return getScoreBlue();
    case 1:
        return getScoreOrange();
    default:
        ERROR_LOG("Team #{} not found", team);
        break;
    }

    return 0;
}


/// <summary>Sets blues score in the current game.</summary>
/// <param name="newScore">New score</param>
void RocketPlugin::setScoreBlue(const int newScore) const
{
    ServerWrapper game = getGame();
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
int RocketPlugin::getScoreBlue() const
{
    ServerWrapper game = getGame(true);
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
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
void RocketPlugin::setScoreOrange(const int newScore) const
{
    ServerWrapper game = getGame();
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
int RocketPlugin::getScoreOrange() const
{
    ServerWrapper game = getGame(true);
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
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
void RocketPlugin::setGameTimeRemaining(const int newGameTimeRemaining) const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetGameTimeRemaining(static_cast<float>(newGameTimeRemaining));
}


/// <summary>Gets the time remaining in the current game.</summary>
/// <returns>The time remaining</returns>
int RocketPlugin::getGameTimeRemaining() const
{
    ServerWrapper game = getGame(true);
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return 0;
    }

    return static_cast<int>(ceil(game.GetGameTimeRemaining()));
}


/// <summary>Sets if the goal delay is disabled in the current game.</summary>
/// <param name="isGoalDelayDisabled">Bool with if the goal delay should be disabled</param>
void RocketPlugin::setIsGoalDelayDisabled(const bool isGoalDelayDisabled) const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetbDisableGoalDelay(isGoalDelayDisabled);
}


/// <summary>Gets if the goal delay is disabled in the current game.</summary>
/// <returns>Bool with if the goal delay is disabled</returns>
bool RocketPlugin::getIsGoalDelayDisabled() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return false;
    }

    return game.GetbDisableGoalDelay();
}


/// <summary>Sets if there is unlimited time in the current game.</summary>
/// <param name="isUnlimitedTime">Bool with if there should be unlimited time</param>
void RocketPlugin::setIsUnlimitedTime(const bool isUnlimitedTime) const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetbUnlimitedTime(isUnlimitedTime);
}


/// <summary>Gets if there is unlimited time in the current game.</summary>
/// <returns>bool with if there is unlimited time</returns>
bool RocketPlugin::getIsUnlimitedTime() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return false;
    }

    return game.GetbUnlimitedTime();
}


/*
 *  Bots
 */

 /// <summary>Sets the number of bots per team in the current game.</summary>
 /// <param name="newMaxNumBots">The new maximum number of bots per team</param>
void RocketPlugin::setMaxNumBots(const bool newMaxNumBots) const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetNumBots(newMaxNumBots);
}


/// <summary>Gets the maximum number of bots per team in the current game.</summary>
/// <returns>The maximum number of bots per team</returns>
int RocketPlugin::getMaxNumBots() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return 0;
    }

    return game.GetNumBots();
}


/// <summary>Updates the maximum number of bots per team in the game.</summary>
/// <param name="newMaxNumBotsPerTeam">The new maximum number of bots per team</param>
void RocketPlugin::prepareBots(const int newMaxNumBotsPerTeam) const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    const int maxTeamSize = game.GetMaxTeamSize();
    const bool isFilledWithAI = game.GetbFillWithAI();
    game.SetMaxTeamSize(newMaxNumBotsPerTeam);
    game.SetbFillWithAI(true);

    const int oldMaxNumBotsPerTeam = game.GetNumBots();
    game.SetNumBots(newMaxNumBotsPerTeam);
    // UpdateBotCount() only adds/removes one bot at a time.
    for (int botsToAdd = std::max(oldMaxNumBotsPerTeam, newMaxNumBotsPerTeam) * 2; botsToAdd > 0; --botsToAdd) {
        game.UpdateBotCount();
    }

    game.SetMaxTeamSize(maxTeamSize);
    game.SetbFillWithAI(isFilledWithAI);
}


/// <summary>Sets if bots are auto filled in the current game.</summary>
/// <param name="isAutoFilledWithBots">Bool with if bots should be auto filled</param>
void RocketPlugin::setIsAutoFilledWithBots(const bool isAutoFilledWithBots) const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetbFillWithAI(isAutoFilledWithBots);
}


/// <summary>Gets if bots are auto filled in the current game.</summary>
/// <returns>Bool with if bots are auto filled</returns>
bool RocketPlugin::getIsAutoFilledWithBots() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return false;
    }

    return game.GetbFillWithAI();
}


/// <summary>Sets if teams are unfair in the current game.</summary>
/// <param name="isUnfairTeams">Bool with if teams should be unfair</param>
void RocketPlugin::setIsUnfairTeams(const bool isUnfairTeams) const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetbUnfairTeams(isUnfairTeams);
}


/// <summary>Gets if teams are unfair in the current game.</summary>
/// <returns>bool with if teams are unfair</returns>
bool RocketPlugin::getIsUnfairTeams() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return false;
    }

    return game.GetbUnfairTeams();
}


/// <summary>Freezes or unfreezes all bots.</summary>
void RocketPlugin::freezeBots() const
{
    ServerWrapper game = getGame();
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
void RocketPlugin::setNumBalls(const int newNumBalls) const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetTotalGameBalls(newNumBalls);

    const float ballScale = getBallsScale();
    resetBalls();
    setBallsScale(ballScale);
}


/// <summary>Gets the number of balls in the current game.</summary>
/// <returns>The number of balls</returns>
int RocketPlugin::getNumBalls() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return 0;
    }

    return game.GetTotalGameBalls();
}


/// <summary>Sets the scale of the balls in the current game.</summary>
/// <param name="newBallsScale">The new scale of the balls</param>
void RocketPlugin::setBallsScale(float newBallsScale) const
{
    ServerWrapper game = getGame();
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
float RocketPlugin::getBallsScale() const
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return 1.0f;
    }

    BallWrapper ball = game.GetBall();
    if (ball.IsNull()) {
        ERROR_LOG("could not get the ball");
        return 1.0f;
    }

    const float ballScale = ball.GetReplicatedBallScale();

    return ballScale > 0 ? ballScale : 1.0f;
}


/*
 *  Player Mods
 */

 /// <summary>Gets the players in the current game.</summary>
 /// <param name="includeBots">Bool with if the output should include bots</param>
 /// <param name="mustBeAlive">Bool with if the output should only include alive players</param>
 /// <returns>List of players</returns>
std::vector<PriWrapper> RocketPlugin::getPlayers(const bool includeBots, const bool mustBeAlive) const
{
    std::vector<PriWrapper> players;
    ServerWrapper game = getGame();
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
        for (PriWrapper PRI : game.GetPRIs()) {
            if (PRI.IsNull() || (!includeBots && PRI.GetbBot())) {
                continue;
            }

            players.push_back(PRI);
        }
    }

    return players;
}


/// <summary>Gets the player names of the players in the current game.</summary>
/// <param name="includeBots">Bool with if the output should include bots</param>
/// <param name="mustBeAlive">Bool with if the output should only include alive players</param>
/// <returns>List of players names</returns>
std::vector<std::string> RocketPlugin::getPlayersNames(const bool includeBots, const bool mustBeAlive) const
{
    return getPlayersNames(getPlayers(includeBots, mustBeAlive));
}


/// <summary>Gets the player names of the given players.</summary>
/// <param name="players">List of players to get the names from</param>
/// <returns>List of players names</returns>
std::vector<std::string> RocketPlugin::getPlayersNames(const std::vector<PriWrapper>& players) const
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
void RocketPlugin::setIsAdmin(PriWrapper player, const bool isAdmin) const
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
bool RocketPlugin::getIsAdmin(PriWrapper player) const
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
void RocketPlugin::setIsHidden(PriWrapper player, const bool isHidden) const
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
bool RocketPlugin::getIsHidden(PriWrapper player) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return false;
    }

    return player.GetCar().GetbHidden();
}


/// <summary>Demolishes the given player.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to demolish</param>
void RocketPlugin::demolish(PriWrapper player) const
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

/// <summary>Sets the car physics for the player.</summary>
/// <remarks>Gets called on 'TAGame.Car_TA.EventVehicleSetup'.</remarks>
/// <param name="car">The players car to set the physics of</param>
void RocketPlugin::setPhysics(CarWrapper car)
{
    if (gameWrapper->GetGameEventAsServer().IsNull() || car.IsNull() || car.GetPRI().IsNull()) {
        ERROR_LOG("could not get the car pri");
        return;
    }
    if (carPhysics.find(car.GetPRI().GetUniqueIdWrapper().GetUID()) == carPhysics.end()) {
        ERROR_LOG("could not get the car physics");
        return;
    }

    PriWrapper player = car.GetPRI();
    const CarPhysics playerCarPhysics = carPhysics[player.GetUniqueIdWrapper().GetUID()];

    setCarScale(player, playerCarPhysics.CarScale);
    setbCarCollision(player, playerCarPhysics.CarHasCollision);
    setCarIsFrozen(player, playerCarPhysics.CarIsFrozen);
    setTorqueRate(player, playerCarPhysics.TorqueRate);
    setMaxCarVelocity(player, playerCarPhysics.MaxCarVelocity);
    setGroundStickyForce(player, playerCarPhysics.GroundStickyForce);
    setWallStickyForce(player, playerCarPhysics.WallStickyForce);
}


/// <summary>Gets the car physics for the player and saves them when not found.</summary>
/// <param name="player">The player to get the car physics from</param>
/// <returns>The car physics for the player</returns>
RocketPlugin::CarPhysics RocketPlugin::getPhysics(PriWrapper player)
{
    CarPhysics playerCarPhysics = { getCarScale(player), getbCarCollision(player), getCarIsFrozen(player),
        getTorqueRate(player), getMaxCarVelocity(player), getGroundStickyForce(player), getWallStickyForce(player) };

    if (player.IsNull()) {
        ERROR_LOG("could not get the player");
        return playerCarPhysics;
    }

    // Inserts if not exist, otherwise do nothing.
    const uint64_t steamID = player.GetUniqueIdWrapper().GetUID();
    carPhysics.try_emplace(steamID, playerCarPhysics);

    return carPhysics[steamID];
}


/// <summary>Sets the if the players car has collision in the current game.</summary>
/// <param name="player">the player to update the car of</param>
/// <param name="carHasCollision">Bool with if the players car should have collision</param>
void RocketPlugin::setbCarCollision(PriWrapper player, const bool carHasCollision)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    carPhysics[player.GetUniqueIdWrapper().GetUID()].CarHasCollision = carHasCollision;
    player.GetCar().SetbCollideActors(carHasCollision);
}


/// <summary>Gets the if the players car has collision in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>Bool with if the players car has collision</returns>
bool RocketPlugin::getbCarCollision(PriWrapper player) const
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
void RocketPlugin::setCarScale(PriWrapper player, const float newCarScale)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    carPhysics[player.GetUniqueIdWrapper().GetUID()].CarScale = newCarScale;
    player.GetCar().SetCarScale(newCarScale);
    player.GetCar().RespawnInPlace();
}


/// <summary>Gets the players car scale in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The players car scale</returns>
float RocketPlugin::getCarScale(PriWrapper player) const
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
void RocketPlugin::setCarIsFrozen(PriWrapper player, const bool carIsFrozen)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    carPhysics[player.GetUniqueIdWrapper().GetUID()].CarIsFrozen = carIsFrozen;
    player.GetCar().SetFrozen(carIsFrozen);
}


/// <summary>Gets the if the players car is frozen in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <returns>Bool with if the players car is frozen</returns>
bool RocketPlugin::getCarIsFrozen(PriWrapper player) const
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
void RocketPlugin::setTorqueRate(PriWrapper player, const float torqueRate)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    carPhysics[player.GetUniqueIdWrapper().GetUID()].TorqueRate = torqueRate;
    player.GetCar().GetVehicleSim().SetDriveTorque(torqueRate * 100000);
}


/// <summary>Gets the players car drive torque in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The drive torque of the players car</returns>
float RocketPlugin::getTorqueRate(PriWrapper player) const
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
void RocketPlugin::setMaxCarVelocity(PriWrapper player, const float maxCarVelocity)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    carPhysics[player.GetUniqueIdWrapper().GetUID()].MaxCarVelocity = maxCarVelocity;
    player.GetCar().SetMaxLinearSpeed(maxCarVelocity);
}


/// <summary>Gets the players car max velocity in the current game.</summary>
/// <param name="player">the player to check the car of</param>
/// <returns>The max velocity of the players car</returns>
float RocketPlugin::getMaxCarVelocity(PriWrapper player) const
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
StickyForceData RocketPlugin::getStickyForce(PriWrapper player) const
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
void RocketPlugin::setGroundStickyForce(PriWrapper player, const float groundStickyForce)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    StickyForceData sfd = getStickyForce(player);
    sfd.Ground = groundStickyForce;

    carPhysics[player.GetUniqueIdWrapper().GetUID()].GroundStickyForce = groundStickyForce;
    player.GetCar().SetStickyForce(sfd);
}


/// <summary>Gets the players car ground sticky force in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The ground sticky force of the players car</returns>
float RocketPlugin::getGroundStickyForce(PriWrapper player) const
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
void RocketPlugin::setWallStickyForce(PriWrapper player, const float wallStickyForce)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return;
    }

    StickyForceData sfd = getStickyForce(player);
    sfd.Wall = wallStickyForce;

    carPhysics[player.GetUniqueIdWrapper().GetUID()].WallStickyForce = wallStickyForce;
    player.GetCar().SetStickyForce(sfd);
}


/// <summary>Gets the players car wall sticky force in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The wall sticky force of the players car</returns>
float RocketPlugin::getWallStickyForce(PriWrapper player) const
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        ERROR_LOG("could not get the players car");
        return 1.5f;
    }

    return player.GetCar().GetStickyForce().Wall;
}


/*
 *  Bakkesmod functions
 */

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
void RocketPlugin::onLoad()
{
    // Enables SEH exception by try catch statements.
    SE_Exception::SetTranslator();

    BakkesModConfigFolder = gameWrapper->GetBakkesModPath() / L"cfg";
    RocketPluginDataFolder = gameWrapper->GetDataFolder() / L"RocketPlugin";
    //RocketLeagueExecutableFolder = Rocket League exe folder;

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

#ifdef DEBUG
    LogLevel = std::make_shared<int>(0);
    cvarManager->registerCvar("cp_log_level", std::to_string(CVarManagerWrapperDebug::level_enum::all),
        "Log level", true, false, 0, false, 0, false).bindTo(LogLevel);

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

#ifdef DEBUG
    loadDebugNotifiers();
#endif

    /* Register Hooks */
    HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.EventVehicleSetup",
        [this](const CarWrapper& caller, void*, const std::string&) {
            setPhysics(caller);
        });

    HookEventWithCaller<ServerWrapper>("Function TAGame.GameEvent_TA.Init",
        [this](const ServerWrapper& caller, void*, const std::string&) {
            gameEventInit(caller);
        });

    HookEventWithCaller<ActorWrapper>("Function TAGame.GFxShell_TA.ShowErrorMessage",
        [this](const ActorWrapper& caller, void*, const std::string&) {
            catchErrorMessage(caller);
        });

    HookEventWithCallerPost<ActorWrapper>("Function TAGame.LoadingScreen_TA.HandlePreLoadMap",
        [this](const ActorWrapper& caller, void* params, const std::string&) {
            onPreLoadingScreen(caller, params);
        });

    HookEventWithCallerPost<ActorWrapper>("Function TAGame.LoadingScreen_TA.HandlePostLoadMap",
        [this](const ActorWrapper&, void*, const std::string&) {
            onPostLoadingScreen();
        });

    HookEventWithCallerPost<ActorWrapper>("Function TAGame.LoadingScreen_TA.GetProtipMessageWithIcons",
        [this](const ActorWrapper& caller, void*, const std::string&) {
            onProTipMessage(caller);
        });

    HookEventWithCaller<ActorWrapper>("Function ProjectX.OnlineGameParty_X.HandlePartyJoinGame",
        [this](const ActorWrapper&, void* params, const std::string&) {
            onPartyInvite(params);
        });

    HookEventWithCaller<ActorWrapper>("Function ProjectX.GFxModal_X.HandleButtonClicked",
        [this](const ActorWrapper& caller, void* params, const std::string&) {
            onHandleButtonClicked(caller, params);
        });

    /* GUI Settings */

    // Set the window bind to the default keybind if is not set.
    if (!IsGUIWindowBound(GetMenuName())) {
        cvarManager->setBind(DEFAULT_GUI_KEYBIND, "togglemenu" + GetMenuName());
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
}


/// <summary>Unload the plugin properly.</summary>
void RocketPlugin::onUnload()
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
