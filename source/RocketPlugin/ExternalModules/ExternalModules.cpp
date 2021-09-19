#include "ExternalModules.h"

#include "RocketPlugin.h"


/*
 *  BakkesMod plugin overrides
 */

/// <summary>Registers notifiers for Rocket Plugin.</summary>
void RocketPlugin::registerExternalCVars()
{
    // std::string cvar, std::string defaultValue, std::string desc = "", bool searchAble = true, bool hasMin = false, float min = 0, bool hasMax = false, float max = 0, bool saveToCfg = true
    for (const auto& [cvar, defaultValue] : ExternalModules::ExternalCVars) {
        cvarManager->registerCvar(cvar, defaultValue);
    }
}


/// <summary>Registers notifiers for Rocket Plugin.</summary>
void RocketPlugin::registerExternalNotifiers()
{
    for (const auto& [cvar, notifier, description, permissions] : ExternalModules::ExternalNotifiers) {
        RegisterNotifier(cvar, notifier, description, permissions);
    }

#ifdef DEBUG
    RegisterNotifier("rp_dump_cache", [this](const std::vector<std::string>&) {
        BM_INFO_LOG("Join Maps:");
        BM_INFO_LOG("Current Join Map: {:s}", quote(currentJoinMap.string()));
        for (const auto& [path, name] : joinableMaps) {
            BM_INFO_LOG("\t{:s}: {:s}", quote(absolute(path).string()), quote(name));
        }
        if (enableWorkshopMaps || enableCustomMaps) {
            BM_INFO_LOG("Custom Maps:");
            BM_INFO_LOG("Current Maps: {:s}", quote(currentMap));
            for (const auto& [path, name] : customMapPaths) {
                BM_INFO_LOG("\t{:s}: {:s}", quote(absolute(path).string()), quote(name));
            }
        }
        else {
            BM_INFO_LOG("Maps:");
            BM_INFO_LOG("Current Maps: {:s}", quote(currentMap));
            for (const auto& [internal, name] : maps) {
                BM_INFO_LOG("\t{:s}: {:s}", quote(internal), quote(name));
            }
        }
        BM_INFO_LOG("Game Modes:");
        BM_INFO_LOG("Selected Game Mode: {:s}", quote(gameModes.GetSelected()));
        for (size_t i = 0; i < gameModes.InternalName.size(); i++) {
            BM_INFO_LOG("\t{:s}: {:s}", quote(gameModes.InternalName[i]), quote(gameModes.DisplayName[i]));
        }
        BM_INFO_LOG("Bot Difficulties:");
        BM_INFO_LOG("Selected Bot Difficulty: {:s}", quote(botDifficulties.GetSelected()));
        for (size_t i = 0; i < botDifficulties.InternalName.size(); i++) {
            BM_INFO_LOG("\t{:s}: {:s}", quote(botDifficulties.InternalName[i]), quote(botDifficulties.DisplayName[i]));
        }
        BM_INFO_LOG("Custom Colors:");
        BM_INFO_LOG("\tDefault Blue Primary Color: {:#X}", ImGui::ColorConvertFloat4ToU32(defaultBluePrimaryColor));
        BM_INFO_LOG("\tDefault Orange Primary Color: {:#X}", ImGui::ColorConvertFloat4ToU32(defaultOrangePrimaryColor));
        BM_INFO_LOG("\tClub Color Hues: {:d}", clubColorHues);
        std::string clubColorBuffer;
        for (size_t i = 0; i < clubColors.size(); i++) {
            if (i != 0 && i % clubColorHues == 0) {
                BM_INFO_LOG("\t{:s}", clubColorBuffer);
                clubColorBuffer.clear();
            }
            clubColorBuffer += to_hex(ImGui::ColorConvertFloat4ToU32(clubColors[i])) + ", ";
        }
        if (!clubColorBuffer.empty()) {
            BM_INFO_LOG("\t{:s}", clubColorBuffer);
        }
        BM_INFO_LOG("\tDefault Blue Accent Color: {:#X}", ImGui::ColorConvertFloat4ToU32(defaultBlueAccentColor));
        BM_INFO_LOG("\tDefault Orange Accent Color: {:#X}", ImGui::ColorConvertFloat4ToU32(defaultOrangeAccentColor));
        BM_INFO_LOG("\tCustom Color Hues: {:d}", customColorHues);
        std::string customColorBuffer;
        for (size_t i = 0; i < customColors.size(); i++) {
            if (i != 0 && i % customColorHues == 0) {
                BM_INFO_LOG("\t{:s}", customColorBuffer);
                customColorBuffer.clear();
            }
            customColorBuffer += to_hex(ImGui::ColorConvertFloat4ToU32(customColors[i])) + ", ";
        }
        if (!customColorBuffer.empty()) {
            BM_INFO_LOG("\t{:s}", customColorBuffer);
        }
        BM_INFO_LOG("Mutators:");
        for (const GameSetting& mutator : mutators) {
            BM_INFO_LOG("\t{:s} ({:s}):", quote(mutator.InternalCategoryName), quote(mutator.DisplayCategoryName));
            for (size_t i = 0; i < mutator.InternalName.size(); i++) {
                BM_INFO_LOG("\t\t{:s}: {:s}", quote(mutator.InternalName[i]), quote(mutator.DisplayName[i]));
            }
        }
    }, "Prints current cache", PERMISSION_ALL);

    RegisterNotifier("rp_dump_selected_match_settings", [this](const std::vector<std::string>&) {
        if (enableWorkshopMaps || enableCustomMaps) {
            if (customMapPaths.find(currentMap) == customMapPaths.end()) {
                BM_ERROR_LOG("No map selected to host.");
            }
            BM_INFO_LOG("MapName: {:s}", quote(std::filesystem::absolute(currentMap).string()));
        }
        else {
            if (maps.find(currentMap) == maps.end()) {
                BM_ERROR_LOG("No map selected to host.");
            }
            BM_INFO_LOG("MapName: {:s}", quote(currentMap));
        }
        if (joinableMaps.find(currentJoinMap) == joinableMaps.end()) {
            BM_ERROR_LOG("No map selected to join.");
        }
        BM_INFO_LOG("JoinMapName: {:s}", quote(absolute(currentJoinMap).string()));
        BM_INFO_LOG("GameMode: {:s}", quote(gameModes.GetSelected()));
        BM_INFO_LOG("GameModeIndex: {:d}", gameModes.CurrentSelected);
        BM_INFO_LOG("MaxPlayerCount: {:d}", playerCount);
        BM_INFO_LOG("Bot Difficulty: {:s}", quote(botDifficulties.GetSelected()));
        BM_INFO_LOG("Password: {:s}", quote(hostPswd));

        BM_INFO_LOG("TeamSettings 1");
        BM_INFO_LOG("\tName: {:s}", quote(team1Name));
        BM_INFO_LOG("\tColors");
        BM_INFO_LOG("\t\tTeamColorID: {:d}", team1PrimCol);
        BM_INFO_LOG("\t\tCustomColorID: {:d}", team1AccCol);
        BM_INFO_LOG("\t\tbTeamColorSet: {}", team1PrimCol != -1);
        BM_INFO_LOG("\t\tbCustomColorSet: {}", team1AccCol != -1);
        BM_INFO_LOG("TeamSettings 2");
        BM_INFO_LOG("\tName: {:s}", quote(team2Name));
        BM_INFO_LOG("\tColors");
        BM_INFO_LOG("\t\tTeamColorID: {:d}", team2PrimCol);
        BM_INFO_LOG("\t\tCustomColorID: {:d}", team2AccCol);
        BM_INFO_LOG("\t\tbTeamColorSet: {}", team2PrimCol != -1);
        BM_INFO_LOG("\t\tbCustomColorSet: {}", team2AccCol != -1);
        BM_INFO_LOG("bClubServer: {}", clubMatch);

        BM_INFO_LOG("Mutators:");
        for (const GameSetting& mutator : mutators) {
            BM_INFO_LOG("\t{:s}: {:s}", quote(mutator.InternalCategoryName), quote(mutator.GetSelected()));
        }
    }, "Print selected match settings", PERMISSION_ALL);
#endif
}


/// <summary>Register hooks for Rocket Plugin.</summary>
void RocketPlugin::registerExternalHooks()
{
    for (const auto& [eventName, callback] : ExternalModules::ExternalEventHooksPre) {
        HookEventWithCaller<ActorWrapper>(eventName, callback);
    }

    for (const auto& [eventName, callback] : ExternalModules::ExternalEventHooksPost) {
        HookEventWithCallerPost<ActorWrapper>(eventName, callback);
    }
}
