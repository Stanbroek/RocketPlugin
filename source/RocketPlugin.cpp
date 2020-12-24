// RocketPlugin.cpp
// A Bakkesmod plugin for joining, hosting and manipulating multiplayer games.
// 
// Author:       Stanbroek
// Version:      0.6.3 15/7/20
// BMSDKversion: 95

#include "RocketPlugin.h"
// Gamemodes
#include "gamemodes/Drainage.h"
#include "gamemodes/CrazyRumble.h"
#include "gamemodes/Zombies.h"
#include "gamemodes/BoostSteal.h"
#include "gamemodes/KeepAway.h"
#include "gamemodes/Tag.h"
#include "gamemodes/Juggernaut.h"
#include "gamemodes/BoostMod.h"

BAKKESMOD_PLUGIN(RocketPlugin, "Rocket Plugin", "0.6.3", 0x0)

std::shared_ptr<CVarManagerWrapperDebug> cvarManager;


/*
 *  General Helpers
 */

/// <summary>Returns the lowercased string from the given string.</summary>
/// <param name="str">String to change</param>
/// <param name="changeInline">Whether to change the string inline</param>
/// <returns>Returns the lowercased string from the given string</returns>
std::string RocketPlugin::toLower(std::string str, bool changeInline)
{
    std::string str_cpy = str;
    std::transform(str_cpy.begin(), str_cpy.end(), str_cpy.begin(),
        [](unsigned char c) { return (unsigned char)std::tolower(c); });

    if (changeInline) {
        str = str_cpy;
    }

    return str_cpy;
}


/*
 *  Match File Helpers
 */

 /// <summary>Checks if the given file extension is in the list of extensions.</summary>
 /// <param name="fileextension">File extension</param>
 /// <param name="extensions">List of file extensions</param>
 /// <returns>Bool with if the file extension is in the list of extensions</returns>
bool RocketPlugin::hasextension(std::string fileextension, std::vector<std::string> extensions)
{
    /* Filter out unwanted file extensions. */
    for (std::string extension : extensions) {
        if (fileextension == extension) {
            return true;
        }
    }

    return false;
}


/// <summary>Recursively gets files from a certain directory.</summary>
/// <remarks>These files can be filtered by if they end with certain file extensions.</remarks>
/// <param name="dirLoc">Path to the directory to get the files from</param>
/// <param name="extensions">List of file extensions to filter by</param>
/// <param name="depth">Current folder depth</param>
/// <param name="maxDepth">Max folder depth to iterate through</param>
/// <returns>The files from a certain directory</returns>
std::vector<std::filesystem::path> RocketPlugin::iterateDirectory(std::string dirPath, std::vector<std::string> extensions, int depth, int maxDepth)
{
    if (depth > maxDepth) {
        return std::vector<std::filesystem::path>();
    }

    std::vector<std::filesystem::path> files;
    const std::filesystem::directory_iterator end{};
    for (std::filesystem::directory_iterator iter{ dirPath }; iter != end; ++iter) {
        std::filesystem::path filePath = iter->path();
        if (iter->is_directory()) {
            std::vector<std::filesystem::path> directoryFiles = iterateDirectory(filePath.string(), extensions, depth + 1, maxDepth);
            /* Remove if directory is empty. */
            if (!directoryFiles.empty()) {
                files.insert(files.end(), directoryFiles.begin(), directoryFiles.end());
            }
        }
        else if (hasextension(filePath.extension().string(), extensions)) {
            files.push_back(filePath);
        }
    }

    return files;
}


/// <summary>Gets files from a certain directory.</summary>
/// <remarks>These files can be filtered by if they end with certain file extensions.</remarks>
/// <param name="dirPath">Path to the directory to get the files from</param>
/// <param name="numextension">Number if filters to filter the files by</param>
/// <param name="...">Multiple file extensions to filter by</param>
/// <returns>The files from a certain directory</returns>
std::vector<std::filesystem::path> RocketPlugin::getFilesFromDir(std::string dirPath, int numextension, ...)
{
    if (!std::filesystem::exists(dirPath)) {
        return std::vector<std::filesystem::path>();
    }

    va_list extensions;
    std::vector<std::string> fileExtensions;

    va_start(extensions, numextension);
    for (int i = 0; i < numextension; i++) {
        fileExtensions.push_back(va_arg(extensions, char*));
    }
    va_end(extensions);

    return iterateDirectory(dirPath, fileExtensions, 0, 1);
}


/// <summary>Gets workshop maps from the given directory.</summary>
/// <param name="dirPath">Path to the workshop directory to get the maps from</param>
/// <param name="preferedExtension">Map extention to prefer when multiple files are found</param>
/// <returns>The workshop maps from the given directory</returns>
std::vector<std::filesystem::path> RocketPlugin::getWorkshopMaps(std::string workshopPath, std::string preferedExtension)
{
    // Make sure we don't request workshop map names every tick.
    bool shouldRequestWorkshopMapNames = publishedFileID.empty();
    std::vector<std::filesystem::path> files = getFilesFromDir(workshopPath, 2, ".upk", ".udk");
    std::filesystem::path best_path;
    std::vector<std::filesystem::path> workshopMaps;
    for (std::filesystem::path file : files) {
        if (file.parent_path() != best_path.parent_path()) {
            if (!best_path.empty()) {
                uint64_t workshopMapId = std::atoll(best_path.parent_path().stem().string().c_str());
                if (shouldRequestWorkshopMapNames && subscribedWorkshopMaps.find(workshopMapId) == subscribedWorkshopMaps.end()) {
                    publishedFileID.push_back(workshopMapId);
                }
                workshopMaps.push_back(best_path);
            }
            best_path = file;
        }
        else if (best_path.extension() != preferedExtension && file.extension() == preferedExtension) {
            best_path = file;
        }
    }

    if (!best_path.empty()) {
        uint64_t workshopMapId = std::atoll(best_path.parent_path().stem().string().c_str());
        if (shouldRequestWorkshopMapNames && subscribedWorkshopMaps.find(workshopMapId) == subscribedWorkshopMaps.end()) {
            publishedFileID.push_back(workshopMapId);
        }
        workshopMaps.push_back(best_path);
    }

    if (shouldRequestWorkshopMapNames && !publishedFileID.empty()) {
        GetSubscribedWorkshopMapsAsync();
    }

    return workshopMaps;
}


/*
 *  Join Match Helpers
 */

 /// <summary>Get the type of address that is given.</summary>
 /// <param name="addr">address to get the type of</param>
 /// <returns>The type of the given address</returns>
RocketPlugin::DestAddrType RocketPlugin::getDestAddrType(const char* addr)
{
    if (Networking::isValidIPv4(addr)) {
        if (Networking::isInternalIPv4(addr)) {
            return DestAddrType::INTERAL_ADDR;
        }
        else if (Networking::isHamachiAddr(addr)) {
            return DestAddrType::HAMACHI_ADDR;
        }

        return DestAddrType::EXTERNL_ADDR;
    }
    else if (Networking::isValidDomainName(addr)) {
        return DestAddrType::EXTERNL_ADDR;
    }

    return DestAddrType::UNKNOWN_ADDR;
}


/*
 *  Commandline Parser Helpers
 */

 /// <summary>Prints how to use command options for the `rp` command.</summary>
 /// <param name="error">String to return an error to</param>
void RocketPlugin::printCommandOptions(std::string error)
{
    error += "usage: rp [option]\n"
        "options:\n"
        "\tjoin [ip](:port) (password)\n"
        "\tmutator [mutator] [value]\n"
        "\trumble [force multiplier] [range multiplier] [duration multiplier]";

    cvarManager->log(error);
}


/// <summary>Prints how to use join options for the `rp` command.</summary>
/// <param name="error">String to return an error to</param>
void RocketPlugin::printJoinOptions(std::string error)
{
    error += "usage: rp join [ip](:port) (password)\n"
        "parameters:\n"
        "\tip: valid ipv4 ip to connect to\n"
        "\tport (optional, default: 7777): port to connect to\n"
        "\tpassword (optional, default: empty): password the server is protected with";

    cvarManager->log(error);
}


/// <summary>Prints how to use the available mutators for the `rp` command.</summary>
/// <param name="error">String to return an error to</param>
void RocketPlugin::printavailableMutators(std::string error)
{
    error += "usage: rp mutator [mutator] [value]\n"
        "mutators:\n";
    for (Mutator mutator : mutators) {
        error += "\t'" + mutator.name + "' [value]\n";
    }

    cvarManager->log(error.substr(0, error.size() - 1));
}


/// <summary>Finds the index of a given string the mutators list after being sanitized.</summary>
/// <param name="e">Mutator to find the index of</param>
/// <returns>The index of a given string the mutators list or -1 or it was not found</returns>
int RocketPlugin::findSanitizedIndexInMutators(const std::string& e)
{
    for (auto i = 0u; i < mutators.size(); i++) {
        std::string m = mutators[i].name;
        m.erase(std::remove_if(m.begin(), m.end(), [](unsigned char c) { return !std::isalnum(c); }), m.end());
        std::transform(m.begin(), m.end(), m.begin(), [](unsigned char c) { return (unsigned char)std::tolower(c); });
        std::string b = e;
        b.erase(std::remove_if(b.begin(), b.end(), [](unsigned char c) { return !std::isalnum(c); }), b.end());
        std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return (unsigned char)std::tolower(c); });

        if (m == b) {
            return i;
        }
    }

    return -1;
}


/// <summary>Prints how to use the available mutator values for the `rp` command.</summary>
/// <param name="mutator"><see cref="Mutator"/> to print the values from</param>
/// <param name="error">String to return an error to</param>
void RocketPlugin::printavailableMutatorValues(Mutator mutator, std::string error)
{
    error += "usage: rp mutator '" + mutator.name + "' [value]\n"
        "values:\n";
    for (std::string value : mutator.display) {
        error += "\t'" + value + "'\n";
    }

    cvarManager->log(error.substr(0, error.size() - 1));
}


/// <summary>Finds the index of a given string the mutator values list after being sanitized.</summary>
/// <param name="mutator"><see cref="Mutator"/>'s to get the values from</param>
/// <param name="e">Mutator to find the index of</param>
/// <returns>The index of a given string the mutator values list or -1 or it was not found</returns>
int RocketPlugin::findSanitizedIndexInMutatorValues(Mutator mutator, const std::string& e)
{
    for (auto i = 0u; i < mutator.display.size(); i++) {
        std::string m = mutator.display[i];
        m.erase(std::remove_if(m.begin(), m.end(), [](unsigned char c) { return !std::isalnum(c); }), m.end());
        std::transform(m.begin(), m.end(), m.begin(), [](unsigned char c) { return (unsigned char)std::tolower(c); });
        std::string b = e;
        b.erase(std::remove_if(b.begin(), b.end(), [](unsigned char c) { return !std::isalnum(c); }), b.end());
        std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return (unsigned char)std::tolower(c); });

        if (m == b) {
            return i;
        }
    }

    return -1;
}


/// <summary>Prints how to use rumble options for the `rp` command.</summary>
/// <param name="error">String to return an error to</param>
void RocketPlugin::printRumbleOptions(std::string error)
{
    error += "usage: rp rumble [force multiplier] [range multiplier] [duration multiplier]\n"
        "parameters:\n"
        "\tforce multiplier: float to multiply with\n"
        "\trange multiplier: float to multiply with\n"
        "\tduration multiplier: float to multiply with";

    cvarManager->log(error);
}


/// <summary>Checks if the given string is a float.</summary>
/// <param name="myString">String to check if it is a float</param>
/// <returns>Bool with is the string is a float</returns>
bool isFloat(std::string myString) {
    std::istringstream iss(myString);
    float f;
    iss >> std::noskipws >> f; // noskipws considers leading whitespace invalid

    // Check the entire string was consumed and if either failbit or badbit is set
    return iss.eof() && !iss.fail();
}


/// <summary>Parses the arguments for the `rp` command.</summary>
/// <param name="arguments">Arguments given with the `rp` command</param>
void RocketPlugin::parseArguments(std::vector<std::string> arguments)
{
    if (arguments.size() < 2) {
        printCommandOptions();
        return;
    }

    if (arguments[1] == "join") {
        if (arguments.size() < 3) {
            printJoinOptions();
            return;
        }
        cvarManager->getCvar("mp_ip").setValue(arguments[2]);

        if (arguments.size() > 3) {
            cvarManager->getCvar("mp_port").setValue(arguments[3]);
        }

        std::string pswd = "";
        if (arguments.size() > 4) {
            pswd = arguments[4];
        }

        joinGame(pswd.c_str());
        return;
    }

    if (arguments[1] == "mutator") {
        if (arguments.size() < 3) {
            printavailableMutators();
            return;
        }

        int i = findSanitizedIndexInMutators(arguments[2]);
        if (i == -1) {
            printavailableMutators("Invalid mutator '" + arguments[2] + "'\n");
            return;
        }

        if (arguments.size() < 4) {
            printavailableMutatorValues(mutators[i]);
            return;
        }

        int j = findSanitizedIndexInMutatorValues(mutators[i], arguments[3]);
        if (j == -1) {
            printavailableMutatorValues(mutators[i], "Invalid value '" + arguments[3] + "' for '" + mutators[i].name + "'\n");
            return;
        }

        mutators[i].current_selected = j;
        cvarManager->log("Changed '" + mutators[i].name + "' to '" + mutators[i].display[j] + "'");
        return;
    }

    if (arguments[1] == "rumble") {
        if (arguments.size() < 5 || !isFloat(arguments[2]) || !isFloat(arguments[3]) || !isFloat(arguments[4])) {
            printRumbleOptions();
            return;
        }

        float forceMultiplier = std::stof(arguments[2]);
        float rangeMultiplier = std::stof(arguments[3]);
        float durationMultiplier = std::stof(arguments[4]);
        // This should work as long as the order in OnLoad() does not get changed.
        CrazyRumble* crazyRumble = dynamic_cast<CrazyRumble*>(gamemodes[1].get());
        crazyRumble->setItemValues(forceMultiplier, rangeMultiplier, durationMultiplier);

        return;
    }

    printCommandOptions("Invalid option '" + arguments[1] + "'\n");
}


/// <summary>Autocompletes the `rp` command.</summary>
/// <remarks>Not available in the public BakkesMod version.</remarks>
/// <param name="arguments">Arguments given with the `rp` command</param>
/// <returns>List of suggestions</returns>
std::vector<std::string> RocketPlugin::complete(std::vector<std::string> arguments)
{
    std::vector<std::string> suggestions = std::vector<std::string>();

    if (arguments.size() == 2 && arguments[1].empty()) {
        suggestions.push_back("join ;[ip](:port) (password)");
        suggestions.push_back("mutator ;[mutator] [value]");
        suggestions.push_back("rumble ;[force_multiplier] [range_multiplier] [duration_multiplier]");
    }
    else if (std::string("join").find(arguments[1]) == 0) {
        if (arguments.size() == 2) {
            suggestions.push_back("join ;[ip](:port) (password)");
        }
        else if (arguments.size() == 3) {
            if (arguments[2].empty()) {
                suggestions.push_back("join " + *ip + ":" + std::to_string(*port) + " ;(password)");
            }
            else {
                suggestions.push_back("join " + arguments[2] + ":" + std::to_string(*port) + " ;(password)");
            }
        }
        else if (arguments.size() == 4) {
            suggestions.push_back("join " + arguments[2] + " ;(password)");
        }
    }
    else if (std::string("mutator").find(arguments[1]) == 0) {
        if (arguments.size() == 2) {
            suggestions.push_back("mutator ;[mutator] [value]");
        }
        if (arguments.size() == 3) {
            for (Mutator mutator : mutators) {
                if (toLower(mutator.name).find(toLower(arguments[2])) == 0) {
                    suggestions.push_back("mutator \"" + mutator.name + "\" ;[value]");
                }
            }
        }
        else if (arguments.size() == 4) {
            int i = findSanitizedIndexInMutators(arguments[2]);
            if (i != -1) {
                for (std::string value : mutators[i].display) {
                    if (toLower(value).find(toLower(arguments[3])) == 0) {
                        suggestions.push_back("mutator \"" + arguments[2] + "\" \"" + value + "\"");
                    }
                }
            }
        }
    }
    else if (std::string("rumble").find(arguments[1]) == 0) {
        if (arguments.size() == 2) {
            suggestions.push_back("rumble ;[force_multiplier] [range_multiplier] [duration_multiplier]");
        }
        if (arguments.size() == 3) {
            suggestions.push_back("rumble ;[force_multiplier] [range_multiplier] [duration_multiplier]");
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
        if (currentMap >= (int)otherMapPaths.size()) {
            cvarManager->log("No map selected.");
            return;
        }

#ifdef DEBUG
        preLoadMap(absolute(otherMapPaths[currentMap]), false, true);
#else
        preLoadMap(absolute(otherMapPaths[currentMap]));
#endif // DEBUG
        if (failedToGetMapPackageFileCache) {
            arena = absolute(otherMapPaths[currentMap]).string();
        }
        else {
            arena = otherMapPaths[currentMap].stem().string();
        }
    }
    else {
        arena = _maps[currentMap];
    }
#ifdef _WIN64
    /* Cos we as windows want our baguettes the left way. */
    std::replace(arena.begin(), arena.end(), '/', '\\');
#endif
    std::string gameMode = "TAGame.GameInfo_" + _gameModes[currentGameMode];
    std::string gameTags = "";
    if (enableFreeplay) {
        gameTags += "Freeplay,";
    }
    for (Mutator mutator : mutators) {
        gameTags += mutator._hidden[mutator.current_selected];
    }
    if (enableBots) {
        gameTags += _botDifficulties[botDifficulty];
        gameTags += "RandomizedBotLoadouts,";
    }
    else {
        gameTags += "BotsNone,";
    }
    gameTags += "PlayerCount" + playerCount;
    int numConnections = std::max(10, playerCount);
    std::string networkOptions = "?NumPublicConnections=" + std::to_string(numConnections) + "?NumOpenPublicConnections=" + std::to_string(numConnections) + "?Lan?Listen";

    setMatchSettings(arena);

    hostingGame = true;
    if (hostWithParty) {
        shouldInviteParty = setSearchStatus("Searching", true);
    }
    if (enableWorkshopMaps || enableCustomMaps) {
        loadingScreenHooked = true;
        uint64_t workshopMapId = std::atoll(otherMapPaths[currentMap].parent_path().stem().string().c_str());
        if (subscribedWorkshopMaps.find(workshopMapId) != subscribedWorkshopMaps.end()) {
            WorkshopMap workshopMap = subscribedWorkshopMaps[workshopMapId];
            // Get the workshop map title.
            wchar_t w_mapName[1024] = L"";
            errno_t ec = mbstowcs_s(nullptr, w_mapName, workshopMap.title.data(), workshopMap.title.size());
            if (ec == 0) {
                loadingScreenMapName = w_mapName;
            }
            else {
                char errmsg[1024];
                strerror_s(errmsg, sizeof errmsg, ec);
                cvarManager->error_log(errmsg);
            }
            // Add the owners nickname.
            loadingScreenMapAuthor = GetPlayerNickname(workshopMap.owner);
            if (loadingScreenMapAuthor.empty()) {
                loadingScreenMapAuthor = L"unknown";
            }
        }
        else {
            loadingScreenMapName = otherMapPaths[currentMap].stem().wstring();
            loadingScreenMapAuthor = L"unknown";
        }
    }
    // Disable "Show boost usage" because Bakkes fixed it again.
    cvarManager->getCvar("cl_soccar_boostcounter").setValue(false);

    // Delay this so the search status can be send before loading.
    gameWrapper->SetTimeout([this, command = "open " + arena + "?Playtest?game=" + gameMode + "?GameTags=" + gameTags + networkOptions](GameWrapper*) {
        gameWrapper->ExecuteUnrealCommand(command);
    }, 0.1f);
}


/// <summary>Cancels the Rocket League call for joining a online game.</summary>
/// <remarks>Gets called on 'Function ProjectX.GFxModal_X.HandleButtonClicked'.</remarks>
/// <param name="modal">The modal</param>
/// <param name="params">The params the function got called with</param>
void RocketPlugin::onHandleButtonClicked(ActorWrapper modal, void* params)
{
    if (!isJoiningParty) {
        return;
    }

    isJoiningParty = false;
    struct UGFxModal_X_HandleButtonClicked_Params
    {
        int Index;
    };
    UGFxModal_X_HandleButtonClicked_Params* buttonClickedParams = (UGFxModal_X_HandleButtonClicked_Params*)params;
    // If the user presses OK, we cancel the Rocket League call.
    if (buttonClickedParams != nullptr && buttonClickedParams->Index == 0) {
        buttonClickedParams->Index = 1;

        cvarManager->log("Joining Rocket Plugin match");
        cvarManager->info_log(joiningPartyIp + ":" + std::to_string(joiningPartyPort) + " password=" + joiningPartyPswd);
        joinGame(joiningPartyPswd.c_str());
    }
}


/// <summary>Sets the countdown time at the start of a match.</summary>
/// <remarks>Gets called on 'TAGame.GameEvent_TA.Init'.</remarks>
/// <param name="server">The game server that started</param>
void RocketPlugin::gameEventInit(ServerWrapper server)
{
    // Clear car physics cache.
    carPhysics.clear();
    isJoiningHost = false;

    if (!hostingGame) {
        return;
    }

    hostingGame = false;
    numBots = getNumBots();

    if (enableWorkshopMaps || enableCustomMaps) {
        setMatchMapName(toLower(otherMapPaths[currentMap].stem().string()));
    }
    else {
        setMatchMapName(_maps[currentMap]);
    }
}


/// <summary>Joins a local game with the preconfigured settings.</summary>
/// <param name="pswd">Password the local game is protected with</param>
void RocketPlugin::joinGame(const char* pswd)
{
    isJoiningHost = true;
    if (joinCustomMap) {
        loadingScreenHooked = true;
        loadingScreenMapName = joinableMaps[currentJoinMap].stem().wstring();
        loadingScreenMapAuthor = L"author";
        // Add the owners nickname if we can find them.
        uint64_t workshopMapId = std::atoll(joinableMaps[currentMap].parent_path().stem().string().c_str());
        if (subscribedWorkshopMaps.find(workshopMapId) != subscribedWorkshopMaps.end()) {
            std::wstring nickname = GetPlayerNickname(subscribedWorkshopMaps[workshopMapId].owner);
            if (!nickname.empty()) {
                loadingScreenMapAuthor = nickname;
            }
        }

#ifdef DEBUG
        preLoadMap(absolute(joinableMaps[currentJoinMap]), false, true);
#else
        preLoadMap(absolute(joinableMaps[currentJoinMap]));
#endif // DEBUG
        if (failedToGetMapPackageFileCache) {
            copyMap(absolute(joinableMaps[currentJoinMap]).string());
        }
    }

    gameWrapper->ExecuteUnrealCommand("start " + *ip + ":" + std::to_string(*port) + "/?Lan?Password=" + pswd);
}


/// <summary>Joins the game after loading the map.</summary>
/// <remarks>Gets called post 'TAGame.LoadingScreen_TA.HandlePostLoadMap'.</remarks>
void RocketPlugin::onPostLoadingScreen()
{
    if (shouldInviteParty) {
        shouldInviteParty = false;
        setSearchStatus("Joining");
        gameWrapper->SetTimeout([this](GameWrapper*) {
            broadcastJoining();
            }, 1.41f);
    }
}


/// <summary>Copies the selected map to a folder in 'CookedPCConsole' where Rocket League can load it.</summary>
/// <param name="map">Map to copy, if no map is given the selected map will be copied</param>
void RocketPlugin::copyMap(std::string map)
{
    std::filesystem::path mapSrc;
    if (map.empty()) {
        mapSrc = joinableMaps[currentJoinMap];
    }
    else {
        mapSrc = map;
    }
    std::filesystem::path mapDst = "../../TAGame/CookedPCConsole/rocketplugin/" + mapSrc.stem().string() + ".upk";

    std::error_code ec;
    /* Creates parent directory. */
    if (!exists(mapDst.parent_path()) && !create_directories(mapDst.parent_path(), ec)) {
        gameWrapper->Toast("Error Fixin Map", "check the console (F6), for more details and report to the mod author if applicable.");
        cvarManager->log("Error creating directory: \"" + mapDst.parent_path().string() + "\" " + ec.message());
        return;
    }

    ec.clear();
    if (!exists(mapDst)) {
        /* Copies file to new directory. */
        std::filesystem::copy(mapSrc, mapDst, ec);
        if (ec) {
            gameWrapper->Toast("Error Fixin Map", "check the console (F6), for more details and report to the mod author if applicable.");
            cvarManager->log("Error copying file: \"" + mapSrc.string() + "\",\"" + mapDst.string() + "\" " + ec.message());
            cvarManager->log("-------------------------------------------------------");
            cvarManager->log("currentJoinMap = " + std::to_string(currentJoinMap));
            int i = 0;
            for (std::filesystem::path p : joinableMaps) {
                cvarManager->log("[" + std::to_string(i++) + "] = path:\"" + p.string() + "\", excists: " + (exists(p) ? "True" : "False"));
            }
            cvarManager->log("-------------------------------------------------------");

            return;
        }

        cvarManager->log("Created file succesfully.");
    }
    else {
        cvarManager->log("Map already exists.");
    }

    currentJoinMap = 0;
}


/// <summary>Saves the configured preset with the given filename.</summary>
/// <param name="preset">Name of the preset file</param>
void RocketPlugin::savePreset(std::string preset)
{
    std::ofstream fout;
    fout.open(*presetDirPath + preset);
    fout << "# This preset has been autogenerated by Rocket Plugin\n";
    for (Mutator& mutator : mutators) {
        if (mutator.current_selected != 0) {
            fout << "rp mutator \"" + mutator.name + "\" \"" + mutator.display[mutator.current_selected] + "\"\n";
        }
    }
    fout.close();
}


/// <summary>Resets the mutators selectors.</summary>
void RocketPlugin::resetMutators()
{
    for (Mutator& mutator : mutators) {
        mutator.current_selected = 0;
    }
}


/// <summary>Loads the preset with the given filename.</summary>
/// <param name="preset">Name of the preset file</param>
void RocketPlugin::loadPreset(std::string preset)
{
    resetMutators();
    cvarManager->log("Loading preset " + preset);
    cvarManager->loadCfg(preset);
}


/*
 *  Game Controls
 */

 /// <summary>Gets the current game as <see cref="ServerWrapper"/>.</summary>
 /// <param name="allowOnlineGame">Bool with if should try to get a online game</param>
 /// <returns>The current game as <see cref="ServerWrapper"/> or <c>NULL</c> is no game is found</returns>
ServerWrapper RocketPlugin::getGame(bool allowOnlineGame)
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
void RocketPlugin::forceOvertime()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.ForceOvertime();
}


/// <summary>Pauses the current game.</summary>
void RocketPlugin::pauseServer()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
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
void RocketPlugin::resetMatch()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.StartNewGame();
}


/// <summary>Resets the players in the current game.</summary>
void RocketPlugin::resetPlayers()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.ResetPlayers();
}


/// <summary>Resets the balls in the current game.</summary>
void RocketPlugin::resetBalls()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.ResetBalls();
}


/*
 *  Match Settings
 */

 /// <summary>Sets max players in the current game.</summary>
 /// <param name="newNumPlayers">New number of players</param>
void RocketPlugin::setMaxPlayers(int newNumPlayers)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.SetMaxPlayers(newNumPlayers);
}


/// <summary>Gets max players in the current game.</summary>
/// <returns>Max number of players</returns>
int RocketPlugin::getMaxPlayers()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return 16;
    }

    return game.GetMaxPlayers();
}


/// <summary>Sets max team size in the current game.</summary>
/// <param name="newTeamSize">New team size</param>
void RocketPlugin::setMaxTeamSize(int newTeamSize)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.SetMaxTeamSize(newTeamSize);
}


/// <summary>Gets max team size in the current game.</summary>
/// <returns>Max team size</returns>
int RocketPlugin::getMaxTeamSize()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return 3;
    }

    return game.GetMaxTeamSize();
}


/// <summary>Sets respawn time in the current game.</summary>
/// <param name="newRespawnTime">New respawn time</param>
void RocketPlugin::setRespawnTime(int newRespawnTime)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.SetRespawnTime(newRespawnTime);
}


/// <summary>Gets respawn time in the current game.</summary>
/// <returns>Respawn time</returns>
int RocketPlugin::getRespawnTime()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return 3;
    }

    return game.GetRespawnTime();
}


/// <summary>Sets blues score in the current game.</summary>
/// <param name="newScore">New score</param>
void RocketPlugin::setScoreBlue(int newScore)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
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
int RocketPlugin::getScoreBlue()
{
    ServerWrapper game = getGame(true);
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
void RocketPlugin::setScoreOrange(int newScore)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
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
int RocketPlugin::getScoreOrange()
{
    ServerWrapper game = getGame(true);
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
void RocketPlugin::setGameTimeRemaining(int newGameTimeRemaining)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.SetGameTimeRemaining((float)newGameTimeRemaining);
}


/// <summary>Gets the time remaining in the current game.</summary>
/// <returns>The time remaining</returns>
int RocketPlugin::getGameTimeRemaining()
{
    ServerWrapper game = getGame(true);
    if (game.IsNull()) {
        return 0;
    }

    return (int)ceil(game.GetGameTimeRemaining());
}


/// <summary>Sets if the goal delay is disabled in the current game.</summary>
/// <param name="isGoalDelayDisabled">Bool with if the goal delay should be disabled</param>
void RocketPlugin::setIsGoalDelayDisabled(bool isGoalDelayDisabled)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.SetbDisableGoalDelay(isGoalDelayDisabled);
}


/// <summary>Gets if the goal delay is disabled in the current game.</summary>
/// <returns>Bool with if the goal delay is disabled</returns>
bool RocketPlugin::getIsGoalDelayDisabled()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return false;
    }

    return game.GetbDisableGoalDelay();
}


/// <summary>Sets if there is unlimited time in the current game.</summary>
/// <param name="isUnlimitedTime">Bool with if there should be unlimited time</param>
void RocketPlugin::setIsUnlimitedTime(bool isUnlimitedTime)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.SetbUnlimitedTime(isUnlimitedTime);
}


/// <summary>Gets if there is unlimited time in the current game.</summary>
/// <returns>bool with if there is unlimited time</returns>
bool RocketPlugin::getIsUnlimitedTime()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return false;
    }

    return game.GetbUnlimitedTime();
}


/*
 *  Bots
 */

 /// <summary>Sets the number of bots in the current game.</summary>
 /// <param name="newNumBots">The new number of bots</param>
void RocketPlugin::setNumBots(bool newNumBots)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.SetNumBots(newNumBots);
}


/// <summary>Gets the number of bots in the current game.</summary>
/// <returns>The number of bots</returns>
int RocketPlugin::getNumBots()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return 0;
    }

    return (game.GetMaxPlayers() / 2) - (game.GetNumHumans() / 2);
}


/// <summary>Sets the bots to be able to join next reset.</summary>
/// <param name="newNumBots">The new number of bots</param>
void RocketPlugin::prepareBots(int newNumBots)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    int numPlayers = game.GetNumHumans();
    int maxPlayers = int(numPlayers / 2) * 2 + (newNumBots * 2);

    setIsAutofilledWithBots(true);
    setMaxPlayers(maxPlayers);
    setMaxTeamSize(newNumBots + numPlayers / 2);
    setNumBots(newNumBots);
}


/// <summary>Sets if bots are auto filled in the current game.</summary>
/// <param name="isAutofilledWithBots">Bool with if bots should be auto filled</param>
void RocketPlugin::setIsAutofilledWithBots(bool isAutofilledWithBots)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.SetbFillWithAI(isAutofilledWithBots);
}


/// <summary>Gets if bots are auto filled in the current game.</summary>
/// <returns>Bool with if bots are auto filled</returns>
bool RocketPlugin::getIsAutofilledWithBots()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return false;
    }

    return game.GetbFillWithAI();
}


/// <summary>Sets if teams are unfair in the current game.</summary>
/// <param name="isUnfairTeams">Bool with if teams should be unfair</param>
void RocketPlugin::setIsUnfairTeams(bool isUnfairTeams)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.SetbUnfairTeams(isUnfairTeams);
}


/// <summary>Gets if teams are unfair in the current game.</summary>
/// <returns>bool with if teams are unfair</returns>
bool RocketPlugin::getIsUnfairTeams()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return false;
    }

    return game.GetbUnfairTeams();
}


/// <summary>Freezes or unfreezes all bots.</summary>
void RocketPlugin::freezeBots()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    bool firstBot = true;
    bool shouldUnFreeze = false;
    ArrayWrapper<CarWrapper> cars = game.GetCars();
    for (int i = 0; i < cars.Count(); i++) {
        CarWrapper car = cars.Get(i);
        if (car.IsNull() || car.GetPRI().IsNull() || !car.GetPRI().GetbBot()) {
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
void RocketPlugin::setNumBalls(int newNumBalls)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }

    game.SetTotalGameBalls(newNumBalls);

    float ballScale = getBallsScale();
    resetBalls();
    setBallsScale(ballScale);
}


/// <summary>Gets the number of balls in the current game.</summary>
/// <returns>The number of balls</returns>
int RocketPlugin::getNumBalls()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return 0;
    }

    return game.GetTotalGameBalls();
}


/// <summary>Sets the scale of the balls in the current game.</summary>
/// <param name="newNumBalls">The new scale of the balls</param>
void RocketPlugin::setBallsScale(float newBallsScale)
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return;
    }
    /* The game crashes with negative ball scale */
    if (newBallsScale < 0) {
        newBallsScale = 1.0f;
    }

    ArrayWrapper<BallWrapper> balls = game.GetGameBalls();
    for (int i = 0; i < balls.Count(); i++) {
        BallWrapper ball = balls.Get(i);
        if (ball.IsNull()) {
            continue;
        }

        ball.SetDrawScale(newBallsScale);
        ball.SetBallScale(newBallsScale);
        ball.SetReplicatedBallScale(newBallsScale);
    }
}


/// <summary>Gets the scale of the balls in the current game.</summary>
/// <returns>The scale of the balls</returns>
float RocketPlugin::getBallsScale()
{
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return 1.0f;
    }

    BallWrapper ball = game.GetBall();
    if (ball.IsNull()) {
        return 1.0f;
    }

    float ballScale = ball.GetReplicatedBallScale();

    return ballScale ? ballScale : 1.0f;
}


/*
 *  Player Mods
 */

 /// <summary>Gets the players in the current game.</summary>
 /// <param name="includeBots">Bool with if the output should include bots</param>
 /// <param name="alive">Bool with if the output should only include alive players</param>
 /// <returns>List of players</returns>
std::vector<PriWrapper> RocketPlugin::getPlayers(bool includeBots, bool alive)
{
    std::vector<PriWrapper> players;
    ServerWrapper game = getGame();
    if (game.IsNull()) {
        return players;
    }

    if (alive) {
        ArrayWrapper<CarWrapper> cars = game.GetCars();
        for (int i = 0; i < cars.Count(); i++) {
            CarWrapper car = cars.Get(i);
            if (car.IsNull() || car.GetPRI().IsNull() || (!includeBots && car.GetPRI().GetbBot())) {
                continue;
            }

            players.push_back(car.GetPRI());
        }
    }
    else {
        ArrayWrapper<PriWrapper> PRIs = game.GetPRIs();
        for (int i = 0; i < PRIs.Count(); i++) {
            PriWrapper PRI = PRIs.Get(i);
            if (PRI.IsNull() || (!includeBots && PRI.GetbBot())) {
                continue;
            }

            players.push_back(PRI);
        }
    }

    return players;
}


/// <summary>Sets if the players is admin in the current game.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to update</param>
/// <param name="isAdmin">Bool with if the player should be admin</param>
void RocketPlugin::setIsAdmin(PriWrapper player, bool isAdmin)
{
    if (player.IsNull()) {
        return;
    }

    player.SetbMatchAdmin(isAdmin);
}


/// <summary>Gets if the players is admin in the current game.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to check</param>
/// <returns>Bool with if the players is admin</returns>
bool RocketPlugin::getIsAdmin(PriWrapper player)
{
    if (player.IsNull()) {
        return false;
    }

    return player.GetbMatchAdmin();
}


/// <summary>Sets if the players is hidden in the current game.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to update</param>
/// <param name="isHidden">Bool with if the player should be hidden</param>
void RocketPlugin::setIsHidden(PriWrapper player, bool isHidden)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return;
    }

    player.GetCar().SetHidden2(isHidden);
    player.GetCar().SetbHiddenSelf(isHidden);
}


/// <summary>Gets if the players is hidden in the current game.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to check</param>
/// <returns>Bool with if the players are hidden</returns>
bool RocketPlugin::getIsHidden(PriWrapper player)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return false;
    }

    return player.GetCar().GetbHidden();
}


/// <summary>Demolishes the given player.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to demolish</param>
void RocketPlugin::demolish(PriWrapper player)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return;
    }

    player.GetCar().Demolish();
}


/*
 *  Car Physics mods
 */

 /// <summary>Gets the player names for a <c>std::vector</c> of <see cref="PriWrapper"/>'s.</summary>
 /// <param name="players">List of players</param>
 /// <returns>List of players names</returns>
std::vector<std::string> RocketPlugin::getPlayersNames(std::vector<PriWrapper> players)
{
    std::vector<std::string> playersNames;

    for (PriWrapper player : players) {
        playersNames.push_back(player.GetPlayerName().ToString());
    }

    return playersNames;
}


/// <summary>Gets the currently selected player.</summary>
/// <param name="players">List of players</param>
/// <param name="currentPlayer">Index of the currently selected player</param>
/// <returns>The currently selected player or NULL the players list changed</returns>
PriWrapper RocketPlugin::getPlayerFromPlayers(std::vector<PriWrapper> players, size_t currentPlayer)
{
    size_t numPlayers = players.size();

    if (currentPlayer >= numPlayers) {
        currentPlayer = 0;
        return NULL;
    }

    return players[currentPlayer];
}


/// <summary>Sets the car physics for the player.</summary>
/// <remarks>Gets called on 'TAGame.Car_TA.EventVehicleSetup'.</remarks>
/// <param name="car">The players car to set the physics of</param>
void RocketPlugin::setPhysics(CarWrapper car)
{
    if (gameWrapper->GetGameEventAsServer().IsNull() || car.IsNull() || car.GetPRI().IsNull() || !carPhysics.count(car.GetPRI().GetUniqueId().ID)) {
        return;
    }

    PriWrapper player = car.GetPRI();
    CarPhysics playerCarPhysics = carPhysics[player.GetUniqueId().ID];

    setCarScale(player, playerCarPhysics.carScale);
    setbCarCollision(player, playerCarPhysics.carHasCollision);
    setCarIsFrozen(player, playerCarPhysics.carIsFrozen);
    setTorqueRate(player, playerCarPhysics.torqueRate);
    setMaxCarVelocity(player, playerCarPhysics.maxCarVelocity);
    setGroundStickyForce(player, playerCarPhysics.groundStickyForce);
    setWallStickyForce(player, playerCarPhysics.wallStickyForce);
}


/// <summary>Gets the car physics for the player and saves them when not found.</summary>
/// <param name="player">The player to get the car physics from</param>
/// <returns>The car physics for the player</returns>
RocketPlugin::CarPhysics RocketPlugin::getPhysics(PriWrapper player)
{
    CarPhysics playerCarPhysics = { getCarScale(player), getbCarCollision(player), getCarIsFrozen(player),
        getTorqueRate(player), getMaxCarVelocity(player), getGroundStickyForce(player), getWallStickyForce(player) };

    if (player.IsNull()) {
        return playerCarPhysics;
    }

    // Inserts if not excist, otherwise do nothing.
    unsigned long long steamID = player.GetUniqueId().ID;
    carPhysics.try_emplace(steamID, playerCarPhysics);

    return carPhysics[steamID];
}


/// <summary>Sets the if the players car has collision in the current game.</summary>
/// <param name="player">the player to update the car of</param>
/// <param name="carCollision">Bool with if the players car should have collision</param>
void RocketPlugin::setbCarCollision(PriWrapper player, bool carHasCollision)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return;
    }

    carPhysics[player.GetUniqueId().ID].carHasCollision = carHasCollision;
    player.GetCar().SetbCollideActors(carHasCollision);
}


/// <summary>Gets the if the players car has collision in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>Bool with if the players car has collision</returns>
bool RocketPlugin::getbCarCollision(PriWrapper player)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return true;
    }

    return player.GetCar().GetbCollideActors();
}


/// <summary>Sets the players car scale in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="newCarScale">The players car scale</param>
void RocketPlugin::setCarScale(PriWrapper player, float newCarScale)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return;
    }

    carPhysics[player.GetUniqueId().ID].carScale = newCarScale;
    player.GetCar().SetDrawScale(newCarScale);
    player.GetCar().SetCarScale(newCarScale);
    player.GetCar().SetReplicatedCarScale(newCarScale);
}


/// <summary>Gets the players car scale in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The players car scale</returns>
float RocketPlugin::getCarScale(PriWrapper player)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return 1.0f;
    }

    float carScale = player.GetCar().GetReplicatedCarScale();

    return carScale ? carScale : 1.0f;
}


/// <summary>Sets the if the players car should be frozen in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="carIsFrozen">Bool with if the players car should be frozen</param>
void RocketPlugin::setCarIsFrozen(PriWrapper player, bool carIsFrozen)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return;
    }

    carPhysics[player.GetUniqueId().ID].carIsFrozen = carIsFrozen;
    player.GetCar().SetFrozen(carIsFrozen);
}


/// <summary>Gets the if the players car is frozen in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <returns>Bool with if the players car is frozen</returns>
bool RocketPlugin::getCarIsFrozen(PriWrapper player)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return false;
    }

    return player.GetCar().GetbFrozen();
}


/// <summary>Sets the players car drive torque in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="torqueRate">The new drive torque of the players car</param>
void RocketPlugin::setTorqueRate(PriWrapper player, float torqueRate)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return;
    }

    carPhysics[player.GetUniqueId().ID].torqueRate = torqueRate;
    player.GetCar().GetVehicleSim().SetDriveTorque(torqueRate * 100000);
}


/// <summary>Gets the players car drive torque in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The drive torque of the players car</returns>
float RocketPlugin::getTorqueRate(PriWrapper player)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return 2.88f;
    }

    return player.GetCar().GetVehicleSim().GetDriveTorque() / 100000;
}


/// <summary>Sets the players car max velocity in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="maxCarVelocity">The new max velocity of the players car</param>
void RocketPlugin::setMaxCarVelocity(PriWrapper player, float maxCarVelocity)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return;
    }

    carPhysics[player.GetUniqueId().ID].maxCarVelocity = maxCarVelocity;
    player.GetCar().SetMaxLinearSpeed(maxCarVelocity);
}


/// <summary>Gets the players car max velocity in the current game.</summary>
/// <param name="player">the player to check the car of</param>
/// <returns>The max velocity of the players car</returns>
float RocketPlugin::getMaxCarVelocity(PriWrapper player)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return 2300.0f;
    }

    return player.GetCar().GetMaxLinearSpeed();
}


/// <summary>Gets the players car sticky force in the current game.</summary>
/// <param name="player">The player to get the sticky force of the car of</param>
/// <returns>The sticky force of the players car</returns>
StickyForceData RocketPlugin::getStickyForce(PriWrapper player)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return StickyForceData();
    }

    return player.GetCar().GetStickyForce();
}


/// <summary>Sets the players car ground sticky force in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="groundStickyForce">The new ground sticky force of the players car</param>
void RocketPlugin::setGroundStickyForce(PriWrapper player, float groundStickyForce)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return;
    }

    StickyForceData sfd = getStickyForce(player);
    sfd.Ground = groundStickyForce;

    carPhysics[player.GetUniqueId().ID].groundStickyForce = groundStickyForce;
    player.GetCar().SetStickyForce(sfd);
}


/// <summary>Gets the players car ground sticky force in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The ground sticky force of the players car</returns>
float RocketPlugin::getGroundStickyForce(PriWrapper player)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return 0.5f;
    }

    return player.GetCar().GetStickyForce().Ground;
}


/// <summary>Sets the players car wall sticky force in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="wallStickyForce">The new wall sticky force of the players car</param>
void RocketPlugin::setWallStickyForce(PriWrapper player, float wallStickyForce)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return;
    }

    StickyForceData sfd = getStickyForce(player);
    sfd.Wall = wallStickyForce;

    carPhysics[player.GetUniqueId().ID].wallStickyForce = wallStickyForce;
    player.GetCar().SetStickyForce(sfd);
}


/// <summary>Gets the players car wall sticky force in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The wall sticky force of the players car</returns>
float RocketPlugin::getWallStickyForce(PriWrapper player)
{
    if (player.IsNull() || player.GetCar().IsNull()) {
        return 1.5f;
    }

    return player.GetCar().GetStickyForce().Wall;
}


/*
 *  Keybind function
 */

 /// <summary>Registers notifiers and variables to interact with the plugin on load.</summary>
void RocketPlugin::changeKeybind()
{
    cvarManager->executeCommand("bind \"" + cvarManager->getCvar("rp_gui_keybind").getStringValue() + "\" \"togglemenu rocketplugin\"", false);
    cvarManager->log("Changed the keybind for \"togglemenu rocketplugin\" to \"" + cvarManager->getCvar("rp_gui_keybind").getStringValue() + "\"");
}


/*
 *  Bakkesmod functions
 */

/// <summary>Registers notifiers and variables to interact with the plugin on load.</summary>
void RocketPlugin::onLoad()
{
    ip = std::make_shared<std::string>("");
    port = std::make_shared<int>(0);
    password = std::make_shared<std::string>("");
    presetDirPath = std::make_shared<std::string>("");
    workshopMapDirPath = std::make_shared<std::string>("");
    customMapDirPath = std::make_shared<std::string>("");

    // Undef so we can copy the original cvarManager.
#undef cvarManager
    _cvarManager = std::reinterpret_pointer_cast<CVarManagerWrapperDebug>(BakkesMod::Plugin::BakkesModPlugin::cvarManager);
#define cvarManager _cvarManager

    cvarManager->registerCvar("mp_ip", "127.0.0.1", "Default ip for joining local matches").bindTo(ip);
    cvarManager->registerCvar("mp_port", "7777", "Default port for joining local matches").bindTo(port);
    cvarManager->registerCvar("mp_password", "", "Default password for joining local matches", true, false, 0, false, 0, false).bindTo(password);
    cvarManager->registerCvar("rp_preset_path", "bakkesmod/data/rocketplugin/presets/", "Default path for the mutator presets directory").bindTo(presetDirPath);
    cvarManager->registerCvar("rp_workshop_path", "../../../../workshop/content/252950/", "Default path for your workshop maps directory").bindTo(workshopMapDirPath);
    cvarManager->registerCvar("rp_custom_path", "../../TAGame/CookedPCConsole/mods/", "Default path for your custom maps directory").bindTo(customMapDirPath);
    cvarManager->registerCvar("rp_gui_keybind", "Home", "Keybind for the gui");

    cvarManager->registerNotifier("rp", [this](std::vector<std::string> arguments) {
        parseArguments(arguments);
    }, "Parses commands to interact with Rocket Plugin", PERMISSION_ALL);

    cvarManager->registerNotifier("rp_change_keybind", [this](std::vector<std::string> arguments) {
        changeKeybind();
    }, "Adds a keybind for \"togglemenu rocketplugin\" as $rp_gui_keybind", PERMISSION_ALL);

    cvarManager->registerNotifier("rp_broadcast_game", [this](std::vector<std::string> arguments) {
        broadcastJoining();
    }, "Broadcasts stuff", PERMISSION_SOCCAR);

    LoadDebugNotifiers();

    /* Game hooks */
    gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.EventVehicleSetup", std::bind(&RocketPlugin::setPhysics, this, std::placeholders::_1));
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.GameEvent_TA.Init", std::bind(&RocketPlugin::gameEventInit, this, std::placeholders::_1));
    gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GFxShell_TA.ShowErrorMessage", std::bind(&RocketPlugin::catchErrorMessage, this, std::placeholders::_1));
    gameWrapper->HookEventWithCallerPost<ActorWrapper>("Function TAGame.LoadingScreen_TA.HandlePreLoadMap", std::bind(&RocketPlugin::onPreLoadingScreen, this, std::placeholders::_1, std::placeholders::_2));
    gameWrapper->HookEventWithCallerPost<ActorWrapper>("Function TAGame.LoadingScreen_TA.HandlePostLoadMap", std::bind(&RocketPlugin::onPostLoadingScreen, this));
    gameWrapper->HookEventWithCallerPost<ActorWrapper>("Function TAGame.LoadingScreen_TA.GetProtipMessageWithIcons", std::bind(&RocketPlugin::onProtipMessage, this, std::placeholders::_1));
    gameWrapper->HookEventWithCaller<ActorWrapper>("Function ProjectX.OnlineGameParty_X.HandlePartyJoinGame", std::bind(&RocketPlugin::onPartyInvite, this, std::placeholders::_1, std::placeholders::_2));
    gameWrapper->HookEventWithCaller<ActorWrapper>("Function ProjectX.GFxModal_X.HandleButtonClicked", std::bind(&RocketPlugin::onHandleButtonClicked, this, std::placeholders::_1, std::placeholders::_2));

    cvarManager->loadCfg("bakkesmod/cfg/rocketplugin.cfg");

    /* Init Networking */
    upnpClient = std::shared_ptr<UPnPClient>(new UPnPClient());
    p2pHost = std::unique_ptr<P2PHost>(new P2PHost());

    /* Init gamemodes */
    gamemodes.push_back(std::shared_ptr<RocketGameMode>(new Drainage(this)));
    gamemodes.push_back(std::shared_ptr<RocketGameMode>(new CrazyRumble(this)));
    gamemodes.push_back(std::shared_ptr<RocketGameMode>(new Zombies(this)));
    gamemodes.push_back(std::shared_ptr<RocketGameMode>(new BoostSteal(this)));
    gamemodes.push_back(std::shared_ptr<RocketGameMode>(new KeepAway(this)));
    gamemodes.push_back(std::shared_ptr<RocketGameMode>(new Tag(this)));
    gamemodes.push_back(std::shared_ptr<RocketGameMode>(new Juggernaut(this)));
    gamemodes.push_back(std::make_shared<BoostMod>(this));
}


/// <summary>Unload the plugin properly.</summary>
void RocketPlugin::onUnload()
{
    /* Save all cvars to 'config.cfg'. */
    cvarManager->backupCfg("./bakkesmod/cfg/config.cfg");
}
