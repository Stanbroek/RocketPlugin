#include "RocketPlugin.h"

#include "GameModes/Tag.h"
#include "GameModes/KeepAway.h"
#include "GameModes/BoostShare.h"
#include "GameModes/BoostSteal.h"
#include "GameModes/CrazyRumble.h"

#include "RLConstants.inc"


bool RocketPlugin::isMapJoinable(const std::filesystem::path&)
{
    BM_WARNING_LOG("redacted function");

    return false;
}


std::wstring RocketPlugin::getPlayerNickname(const uint64_t) const
{
    BM_WARNING_LOG("redacted function");

    return L"";
}


void RocketPlugin::getSubscribedWorkshopMapsAsync(const bool)
{
    BM_WARNING_LOG("redacted function");
}


void RocketPlugin::setMatchSettings(const std::string&) const
{
    BM_WARNING_LOG("redacted function");
}


void RocketPlugin::setMatchMapName(const std::string&) const
{
    BM_WARNING_LOG("redacted function");
}


bool RocketPlugin::setSearchStatus(const std::wstring&, const bool) const
{
    BM_WARNING_LOG("redacted function");

    return false;
}


void RocketPlugin::broadcastJoining()
{
    BM_WARNING_LOG("redacted function");
}


bool RocketPlugin::isHostingLocalGame() const
{
    BM_WARNING_LOG("redacted function");

    return false;
}


bool RocketPlugin::preLoadMap(const std::filesystem::path&, const bool, const bool)
{
    BM_WARNING_LOG("redacted function");

    return false;
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
    BM_WARNING_LOG("redacted function");

    return false;
}


void BoostShare::removePickups() const
{
    BM_WARNING_LOG("redacted function");
}


void BoostSteal::stealBoost(CarWrapper, void*) const
{
    BM_WARNING_LOG("redacted function");
}


void CrazyRumble::onGiveItem(const ActorWrapper&) const
{
    BM_WARNING_LOG("redacted function");
}


void CrazyRumble::updateDispensers(const bool, const bool) const
{
    BM_WARNING_LOG("redacted function");
}


void CrazyRumble::updateDispenserItemPool(const ActorWrapper&) const
{
    BM_WARNING_LOG("redacted function");
}


void CrazyRumble::updateDispenserMaxTimeTillItem(const ActorWrapper&) const
{
    BM_WARNING_LOG("redacted function");
}


void KeepAway::onCarTouch(void*)
{
    BM_WARNING_LOG("redacted function");
}


void Tag::onCarImpact(CarWrapper, void*)
{
    BM_WARNING_LOG("redacted function");
}


void Tag::onRumbleItemActivated(ActorWrapper, void*)
{
    BM_WARNING_LOG("redacted function");
}
