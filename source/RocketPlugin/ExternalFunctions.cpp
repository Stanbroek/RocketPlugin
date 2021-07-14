#include "RocketPlugin.h"

#include "GameModes/Tag.h"
#include "GameModes/KeepAway.h"
#include "GameModes/BoostShare.h"
#include "GameModes/BoostSteal.h"
#include "GameModes/CrazyRumble.h"

#include "RLConstants.inc"


bool RocketPlugin::isMapJoinable(const std::filesystem::path&)
{
    WARNING_LOG("redacted function");

    return false;
}


std::wstring RocketPlugin::getPlayerNickname(const uint64_t) const
{
    WARNING_LOG("redacted function");

    return L"";
}


void RocketPlugin::getSubscribedWorkshopMapsAsync(const bool)
{
    WARNING_LOG("redacted function");
}


void RocketPlugin::setMatchSettings(const std::string&) const
{
    WARNING_LOG("redacted function");
}


void RocketPlugin::setMatchMapName(const std::string&) const
{
    WARNING_LOG("redacted function");
}


bool RocketPlugin::setSearchStatus(const std::wstring&, const bool) const
{
    WARNING_LOG("redacted function");

    return false;
}


void RocketPlugin::broadcastJoining()
{
    WARNING_LOG("redacted function");
}


bool RocketPlugin::isHostingLocalGame() const
{
    WARNING_LOG("redacted function");

    return false;
}


bool RocketPlugin::preLoadMap(const std::filesystem::path&, const bool, const bool)
{
    WARNING_LOG("redacted function");

    return false;
}


void RocketPlugin::registerExternalNotifiers()
{
#ifdef DEBUG
    RegisterNotifier("rp_test_paths", [this](const std::vector<std::string>&) {
        LOG(absolute(BINDS_FILE_PATH).string());
        LOG(absolute(CONFIG_FILE_PATH).string());
        LOG(absolute(PRESETS_PATH).string());
        LOG(absolute(PRO_TIPS_FILE_PATH).string());
        LOG(absolute(CUSTOM_MAPS_PATH).string());
        LOG(absolute(WORKSHOP_MAPS_PATH).string());
    }, "Logs paths", PERMISSION_ALL);

    RegisterNotifier("rp_test_logs", [this](const std::vector<std::string>&) {
        LOG("normal log");
        TRACE_LOG("trace log");
        INFO_LOG("info log");
        WARNING_LOG("warning log");
        ERROR_LOG("error log");
        CRITICAL_LOG("critical log");
    }, "Logs a bunch", PERMISSION_ALL);

    RegisterNotifier("rp_run_tests", [this](const std::vector<std::string>& arguments) {
        std::vector<std::string> tests = { "", "", "" };
        if (arguments.size() >= 2) {
            tests = std::vector<std::string>(arguments.begin() + 1, arguments.end());
        }

        if (std::find(tests.begin(), tests.end(), "x") != tests.end()) {
        }
    }, "Runs a bunch of tests", PERMISSION_ALL);

    RegisterNotifier("rp_crash", [this](const std::vector<std::string>&) {
        *static_cast<unsigned*>(nullptr) = 0;
    }, "Crashes by dereferencing nullptr", PERMISSION_ALL);

    RegisterNotifier("rp_throw", [this](const std::vector<std::string>&) {
        throw std::runtime_error("NotifierInterrupt");
    }, "Throws exception", PERMISSION_ALL);

    RegisterNotifier("rp_dump_cache", [this](const std::vector<std::string>&) {
        LOG("Join Maps:");
        LOG("Current Join Map: {}", quote(currentJoinMap.u8string()));
        for (const auto& [path, name] : joinableMaps) {
            LOG("\t{}: {}", quote(absolute(path).u8string()), quote(name));
        }
        if (enableWorkshopMaps || enableCustomMaps) {
            LOG("Custom Maps:");
            LOG("Current Maps: {}", quote(currentMap));
            for (const auto& [path, name] : customMapPaths) {
                LOG("\t{}: {}", quote(absolute(path).u8string()), quote(name));
            }
        }
        else {
            LOG("Maps:");
            LOG("Current Maps: {}", quote(currentMap));
            for (const auto& [internal, name] : maps) {
                LOG("\t{}: {}", quote(internal), quote(name));
            }
        }
        LOG("Game Modes:");
        LOG("Selected Game Mode: {}", quote(gameModes.GetSelected()));
        for (size_t i = 0; i < gameModes.InternalName.size(); i++) {
            LOG("\t{}: {}", quote(gameModes.InternalName[i]), quote(gameModes.DisplayName[i]));
        }
        LOG("Bot Difficulties:");
        LOG("Selected Bot Difficulty: {}", quote(botDifficulties.GetSelected()));
        for (size_t i = 0; i < botDifficulties.InternalName.size(); i++) {
            LOG("\t{}: {}", quote(botDifficulties.InternalName[i]), quote(botDifficulties.DisplayName[i]));
        }
        LOG("Custom Colors:");
        LOG("\tDefault Blue Primary Color: {:X}", ImGui::ColorConvertFloat4ToU32(defaultBluePrimaryColor));
        LOG("\tDefault Orange Primary Color: {:X}", ImGui::ColorConvertFloat4ToU32(defaultOrangePrimaryColor));
        LOG("\tClub Color Hues: {}", clubColorHues);
        std::string clubColorBuffer;
        for (size_t i = 0; i < clubColors.size(); i++) {
            if (i != 0 && i % clubColorHues == 0) {
                LOG("\t{}", clubColorBuffer);
                clubColorBuffer.clear();
            }
            clubColorBuffer += to_hex(ImGui::ColorConvertFloat4ToU32(clubColors[i])) + ", ";
        }
        if (!clubColorBuffer.empty()) {
            LOG("\t{}", clubColorBuffer);
        }
        LOG("\tDefault Blue Accent Color: {:X}", ImGui::ColorConvertFloat4ToU32(defaultBlueAccentColor));
        LOG("\tDefault Orange Accent Color: {:X}", ImGui::ColorConvertFloat4ToU32(defaultOrangeAccentColor));
        LOG("\tCustom Color Hues: {}", customColorHues);
        std::string customColorBuffer;
        for (size_t i = 0; i < customColors.size(); i++) {
            if (i != 0 && i % customColorHues == 0) {
                LOG("\t{}", customColorBuffer);
                customColorBuffer.clear();
            }
            customColorBuffer += to_hex(ImGui::ColorConvertFloat4ToU32(customColors[i])) + ", ";
        }
        if (!customColorBuffer.empty()) {
            LOG("\t{}", customColorBuffer);
        }
        LOG("Mutators:");
        for (const GameSetting& mutator : mutators) {
            LOG("\t{} ({}):", quote(mutator.InternalCategoryName), quote(mutator.DisplayCategoryName));
            for (size_t i = 0; i < mutator.InternalName.size(); i++) {
                LOG("\t\t{}: {}", quote(mutator.InternalName[i]), quote(mutator.DisplayName[i]));
            }
        }
    }, "Prints current cache", PERMISSION_ALL);

    RegisterNotifier("rp_dump_selected_match_settings", [this](const std::vector<std::string>&) {
        if (enableWorkshopMaps || enableCustomMaps) {
            if (customMapPaths.find(currentMap) == customMapPaths.end()) {
                ERROR_LOG("No map selected to host.");
            }
            LOG("MapName: {}", quote(std::filesystem::absolute(currentMap).string()));
        }
        else {
            if (maps.find(currentMap) == maps.end()) {
                ERROR_LOG("No map selected to host.");
            }
            LOG("MapName: {}", quote(currentMap));
        }
        if (joinableMaps.find(currentJoinMap) == joinableMaps.end()) {
            ERROR_LOG("No map selected to join.");
        }
        LOG("JoinMapName: {}", quote(absolute(currentJoinMap).u8string()));
        LOG("GameMode: {}", quote(gameModes.GetSelected()));
        LOG("GameModeIndex: {}", gameModes.CurrentSelected);
        LOG("MaxPlayerCount: {}", playerCount);
        LOG("Bot Difficulty: {}", quote(botDifficulties.GetSelected()));
        LOG("Password: {}", quote(hostPswd));

        LOG("TeamSettings 1");
        LOG("\tName: {}", quote(team1Name));
        LOG("\tColors");
        LOG("\t\tTeamColorID: {:d}", team1PrimCol);
        LOG("\t\tCustomColorID: {:d}", team1AccCol);
        LOG("\t\tbTeamColorSet: {}", team1PrimCol != -1);
        LOG("\t\tbCustomColorSet: {}", team1AccCol != -1);
        LOG("TeamSettings 2");
        LOG("\tName: {}", quote(team2Name));
        LOG("\tColors");
        LOG("\t\tTeamColorID: {:d}", team2PrimCol);
        LOG("\t\tCustomColorID: {:d}", team2AccCol);
        LOG("\t\tbTeamColorSet: {}", team2PrimCol != -1);
        LOG("\t\tbCustomColorSet: {}", team2AccCol != -1);
        LOG("bClubServer: {}", clubMatch);

        LOG("Mutators:");
        for (const GameSetting& mutator : mutators) {
            LOG("\t{}: {}", quote(mutator.InternalCategoryName), quote(mutator.GetSelected()));
        }
    }, "Print selected match settings", PERMISSION_ALL);
#endif
}


void RocketPlugin::registerExternalHooks()
{
    WARNING_LOG("redacted function");
}


void RocketPlugin::loadRLConstants()
{
    gameModes = RLConstants::GAME_MODES;
    maps = RLConstants::MAPS;
    botDifficulties = RLConstants::BOT_DIFFICULTIES;
    customColorHues = RLConstants::CUSTOM_HUE_COUNT;
    customColors = RLConstants::CUSTOM_COLORS;
    clubColorHues = RLConstants::CLUB_HUE_COUNT;
    clubColors = RLConstants::CUSTOM_COLORS;
    defaultBluePrimaryColor = RLConstants::DEFAULT_BLUE_PRIMARY_COLOR;
    defaultBlueAccentColor = RLConstants::DEFAULT_BLUE_ACCENT_COLOR;
    defaultOrangePrimaryColor = RLConstants::DEFAULT_ORANGE_PRIMARY_COLOR;
    defaultOrangeAccentColor = RLConstants::DEFAULT_ORANGE_ACCENT_COLOR;
    mutators = RLConstants::MUTATORS;
}


bool RocketPlugin::isCurrentMapModded() const
{
    WARNING_LOG("redacted function");

    return false;
}


void BoostShare::removePickups() const
{
    WARNING_LOG("redacted function");
}


void BoostSteal::stealBoost(CarWrapper, void*) const
{
    WARNING_LOG("redacted function");
}


void CrazyRumble::onGiveItem(const ActorWrapper&) const
{
    WARNING_LOG("redacted function");
}


void CrazyRumble::updateDispensers(const bool, const bool) const
{
    WARNING_LOG("redacted function");
}


void CrazyRumble::updateDispenserItemPool(const ActorWrapper&) const
{
    WARNING_LOG("redacted function");
}


void CrazyRumble::updateDispenserMaxTimeTillItem(const ActorWrapper&) const
{
    WARNING_LOG("redacted function");
}


void KeepAway::onCarTouch(void*)
{
    WARNING_LOG("redacted function");
}


void Tag::onCarImpact(CarWrapper, void*)
{
    WARNING_LOG("redacted function");
}


void Tag::onRumbleItemActivated(ActorWrapper, void*)
{
    WARNING_LOG("redacted function");
}
