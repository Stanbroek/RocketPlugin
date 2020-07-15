#include "RocketPlugin.h"

std::wstring RocketPlugin::GetPlayerNickname(uint64_t)
{
    cvarManager->error_log("function not implemented.");
    return L"";
}

void RocketPlugin::GetSubscribedWorkshopMapsAsync(bool)
{
    cvarManager->error_log("function not implemented.");
}

bool RocketPlugin::isMapJoinable(std::filesystem::path map)
{
    cvarManager->error_log("function not implemented.");
    return false;
}

bool RocketPlugin::setSearchStatus(std::string, bool)
{
    cvarManager->error_log("function not implemented.");
    return false;
}

void RocketPlugin::setMatchSettings(std::string)
{
    cvarManager->error_log("function not implemented.");
}

void RocketPlugin::setMatchMapName(std::string)
{
    cvarManager->error_log("function not implemented.");
}

void RocketPlugin::onPartyInvite(ActorWrapper, void*)
{
    cvarManager->error_log("function not implemented.");
}

void RocketPlugin::catchErrorMessage(ActorWrapper)
{
    cvarManager->error_log("function not implemented.");
}

void RocketPlugin::onProtipMessage(ActorWrapper)
{
    cvarManager->error_log("function not implemented.");
}

void RocketPlugin::onPreLoadingScreen(ActorWrapper, void*)
{
    cvarManager->error_log("function not implemented.");
}

void RocketPlugin::broadcastJoining()
{
    cvarManager->error_log("function not implemented.");
}

void RocketPlugin::addGodBall()
{
    cvarManager->error_log("function not implemented.");
}

void RocketPlugin::preLoadMap(std::wstring, bool, bool)
{
    cvarManager->error_log("function not implemented.");
}

void RocketPlugin::LoadDebugNotifiers()
{
    cvarManager->error_log("function not implemented.");
}