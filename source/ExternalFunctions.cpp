#include "RocketPlugin.h"

#include "GameModes/Tag.h"
#include "GameModes/KeepAway.h"
#include "GameModes/BoostSteal.h"
#include "GameModes/CrazyRumble.h"

#include "RLConstants.inc"


std::wstring RocketPlugin::getPlayerNickname(const uint64_t) const
{
    WARNING_LOG("redacted function");
    
    return L"";
}


void RocketPlugin::getSubscribedWorkshopMapsAsync(const bool) const
{
    WARNING_LOG("redacted function");
}


bool RocketPlugin::isMapJoinable(const std::filesystem::path&) const
{
    WARNING_LOG("redacted function");

    return false;
}


bool RocketPlugin::setSearchStatus(const std::wstring&, const bool) const
{
    WARNING_LOG("redacted function");

    return false;
}


bool RocketPlugin::clearSearchStatus() const
{
    WARNING_LOG("redacted function");

    return false;
}


void RocketPlugin::setMatchSettings(const std::string&) const
{
    WARNING_LOG("redacted function");
}


void RocketPlugin::setMatchMapName(const std::string&) const
{
    WARNING_LOG("redacted function");
}


void RocketPlugin::onHandleButtonClicked(const ActorWrapper&, void*) const
{
    WARNING_LOG("redacted function");
}


void RocketPlugin::onPartyInvite(void*) const
{
    WARNING_LOG("redacted function");
}


void RocketPlugin::catchErrorMessage(const ActorWrapper&) const
{
    WARNING_LOG("redacted function");
}


void RocketPlugin::onProTipMessage(const ActorWrapper&) const
{
    WARNING_LOG("redacted function");
}


void RocketPlugin::onPreLoadingScreen(const ActorWrapper&, void*) const
{
    WARNING_LOG("redacted function");
}


void RocketPlugin::broadcastJoining() const
{
    WARNING_LOG("redacted function");
}


bool RocketPlugin::preLoadMap(const std::filesystem::path&, const bool, const bool) const
{
    WARNING_LOG("redacted function");

    return false;
}


/// <summary>Loads Rocket League constants.</summary>
void RocketPlugin::loadRLConstants()
{
    gameModes = RLConstants::GAME_MODES;
    maps = RLConstants::MAPS;
    botDifficulties = RLConstants::BOT_DIFFICULTIES;
    customColorHues = RLConstants::HUE_COUNT;
    customColors = RLConstants::CUSTOM_COLORS;
    clubColorHues = RLConstants::HUE_COUNT;
    clubColors = RLConstants::CUSTOM_COLORS;
    defaultBluePrimaryColor = RLConstants::DEFAULT_BLUE_PRIMARY_COLOR;
    defaultBlueAccentColor = RLConstants::DEFAULT_BLUE_ACCENT_COLOR;
    defaultOrangePrimaryColor = RLConstants::DEFAULT_ORANGE_PRIMARY_COLOR;
    defaultOrangeAccentColor = RLConstants::DEFAULT_ORANGE_ACCENT_COLOR;
    mutators = RLConstants::MUTATORS;
}


void BoostSteal::stealBoost(CarWrapper, void*) const
{
    WARNING_LOG("redacted function");
}


void CrazyRumble::onGiveItem(const ActorWrapper&) const
{
    WARNING_LOG("redacted function");
}


void CrazyRumble::setItemGiveRate(const int) const
{
    WARNING_LOG("redacted function");
}


void KeepAway::onCarTouch(void*) const
{
    WARNING_LOG("redacted function");
}


void Tag::onCarImpact(CarWrapper, void*) const
{
    WARNING_LOG("redacted function");
}


void Tag::onRumbleItemActivated(ActorWrapper, void*) const
{
    WARNING_LOG("redacted function");
}


/// <summary>Notifiers for debugging Rocket Plugin.</summary>
void RocketPlugin::loadDebugNotifiers()
{
#ifdef DEBUG
    RegisterNotifier("rp_test_paths", [this](const std::vector<std::string>&) {
        INFO_LOG(absolute(BINDS_FILE_PATH).string());
        INFO_LOG(absolute(CONFIG_FILE_PATH).string());
        INFO_LOG(absolute(PRESETS_PATH).string());
        INFO_LOG(absolute(PRO_TIPS_FILE_PATH).string());
        INFO_LOG(absolute(CUSTOM_MAPS_PATH).string());
        INFO_LOG(absolute(WORKSHOP_MAPS_PATH).string());
    }, "Logs paths", PERMISSION_ALL);

    RegisterNotifier("rp_test_logs", [this](const std::vector<std::string>&) {
        LOG("normal log");
        TRACE_LOG("trace log");
        INFO_LOG("info log");
        WARNING_LOG("warning log");
        ERROR_LOG("error log");
        CRITICAL_LOG("critical log");
    }, "Logs a bunch", PERMISSION_ALL);

    RegisterNotifier("rp_dump_cache", [this](const std::vector<std::string>&) {
        INFO_LOG("Join Maps:");
        INFO_LOG("Current Join Map: " + quote(currentJoinMap.u8string()));
        for (const auto& [path, name] : joinableMaps) {
            INFO_LOG("\t" + quote(absolute(path).u8string()) + ": " + quote(name));
        }
        if (enableWorkshopMaps || enableCustomMaps) {
            INFO_LOG("Custom Maps:");
            INFO_LOG("Current Maps: " + quote(currentMap));
            for (const auto& [path, name] : customMapPaths) {
                INFO_LOG("\t" + quote(absolute(path).u8string()) + ": " + quote(name));
            }
        }
        else {
            INFO_LOG("Maps:");
            INFO_LOG("Current Maps: " + quote(currentMap));
            for (const auto& [internal, name] : maps) {
                INFO_LOG("\t" + quote(internal) + ": " + quote(name));
            }
        }
        INFO_LOG("Game Modes:");
        INFO_LOG("Selected Game Mode: " + quote(gameModes.GetSelected()));
        for (size_t i = 0; i < gameModes.InternalName.size(); i++) {
            INFO_LOG("\t" + quote(gameModes.InternalName[i]) + ": " + quote(gameModes.DisplayName[i]));
        }
        INFO_LOG("Bot Difficulties:");
        INFO_LOG("Selected Bot Difficulty: " + quote(botDifficulties.GetSelected()));
        for (size_t i = 0; i < botDifficulties.InternalName.size(); i++) {
            INFO_LOG("\t" + quote(botDifficulties.InternalName[i]) + ": " + quote(botDifficulties.DisplayName[i]));
        }
        INFO_LOG("Custom Colors:");
        INFO_LOG("\tDefault Blue Primary Color: " + to_hex(ImGui::ColorConvertFloat4ToU32(defaultBluePrimaryColor)));
        INFO_LOG("\tDefault Orange Primary Color: " + to_hex(ImGui::ColorConvertFloat4ToU32(defaultOrangePrimaryColor)));
        INFO_LOG("\tClub Color Hues: " + std::to_string(clubColorHues));
        std::string clubColorBuffer;
        for (size_t i = 0; i < clubColors.size(); i++) {
            if (i != 0 && i % clubColorHues == 0) {
                INFO_LOG("\t" + clubColorBuffer);
                clubColorBuffer.clear();
            }
            clubColorBuffer += to_hex(ImGui::ColorConvertFloat4ToU32(clubColors[i])) + ", ";
        }
        if (!clubColorBuffer.empty()) {
            INFO_LOG("\t" + clubColorBuffer);
        }
        INFO_LOG("\tDefault Blue Accent Color: " + to_hex(ImGui::ColorConvertFloat4ToU32(defaultBlueAccentColor)));
        INFO_LOG("\tDefault Orange Accent Color: " + to_hex(ImGui::ColorConvertFloat4ToU32(defaultOrangeAccentColor)));
        INFO_LOG("\tCustom Color Hues: " + std::to_string(customColorHues));
        std::string customColorBuffer;
        for (size_t i = 0; i < customColors.size(); i++) {
            if (i != 0 && i % customColorHues == 0) {
                INFO_LOG("\t" + customColorBuffer);
                customColorBuffer.clear();
            }
            customColorBuffer += to_hex(ImGui::ColorConvertFloat4ToU32(customColors[i])) + ", ";
        }
        if (!customColorBuffer.empty()) {
            INFO_LOG("\t" + customColorBuffer);
        }
        INFO_LOG("Mutators:");
        for (const GameSetting& mutator : mutators) {
            INFO_LOG("\t" + mutator.InternalCategoryName + " (" + quote(mutator.DisplayCategoryName) + "): ");
            for (size_t i = 0; i < mutator.InternalName.size(); i++) {
                INFO_LOG("\t\t" + mutator.InternalName[i] + ": " + quote(mutator.DisplayName[i]));
            }
        }
    }, "Prints current cache", PERMISSION_ALL);

    RegisterNotifier("rp_dump_selected_match_settings", [this](const std::vector<std::string>&) {
        if (enableWorkshopMaps || enableCustomMaps) {
            if (customMapPaths.find(currentMap) == customMapPaths.end()) {
                ERROR_LOG("No map selected to host.");
            }
            INFO_LOG("MapName: " + quote(std::filesystem::absolute(currentMap).string()));
        }
        else {
            if (maps.find(currentMap) == maps.end()) {
                ERROR_LOG("No map selected to host.");
            }
            INFO_LOG("MapName: " + quote(currentMap));
        }
        if (joinableMaps.find(currentJoinMap) == joinableMaps.end()) {
            ERROR_LOG("No map selected to join.");
        }
        INFO_LOG("JoinMapName: " + quote(absolute(currentJoinMap).u8string()));
        INFO_LOG("GameMode: " + quote(gameModes.GetSelected()));
        INFO_LOG("GameModeIndex: " + std::to_string(gameModes.CurrentSelected));
        INFO_LOG("MaxPlayerCount: " + std::to_string(playerCount));
        INFO_LOG("Bot Difficulty: " + botDifficulties.GetSelected());
        INFO_LOG("Password: " + quote(hostPswd));

        INFO_LOG("TeamSettings 1");
        INFO_LOG("\tName:" + quote(team1Name));
        INFO_LOG("\tColors");
        INFO_LOG("\t\tTeamColorID:" + std::to_string(team1PrimCol));
        INFO_LOG("\t\tCustomColorID:" + std::to_string(team1AccCol));
        INFO_LOG("\t\tbTeamColorSet:" + std::string(team1PrimCol != -1 ? "True" : "False"));
        INFO_LOG("\t\tbCustomColorSet:" + std::string(team1AccCol != -1 ? "True" : "False"));
        INFO_LOG("TeamSettings 2");
        INFO_LOG("\tName:" + quote(team2Name));
        INFO_LOG("\tColors");
        INFO_LOG("\t\tTeamColorID:" + std::to_string(team2PrimCol));
        INFO_LOG("\t\tCustomColorID:" + std::to_string(team2AccCol));
        INFO_LOG("\t\tbTeamColorSet:" + std::string(team2PrimCol != -1 ? "True" : "False"));
        INFO_LOG("\t\tbCustomColorSet:" + std::string(team2AccCol != -1 ? "True" : "False"));
        INFO_LOG("bClubServer: " + std::string(clubMatch ? "True" : "False"));

        INFO_LOG("Mutators:");
        for (const GameSetting& mutator : mutators) {
            INFO_LOG("\t" + mutator.InternalCategoryName + ": " + mutator.GetSelected());
        }
    }, "Print selected match settings", PERMISSION_ALL);
#endif
}
