#pragma once
#include "RocketPluginModule.h"


class RocketPlugin;

class PlayerMods final : RocketPluginModule
{
    friend RocketPlugin;
public:
    std::vector<PriWrapper> GetPlayers(bool includeBots = false, bool mustBeAlive = false) const;
    std::vector<std::string> GetPlayersNames(bool includeBots = false, bool mustBeAlive = false) const;
    std::vector<std::string> GetPlayersNames(const std::vector<PriWrapper>& players) const;
    void SetIsAdmin(PriWrapper player, bool isAdmin) const;
    bool GetIsAdmin(PriWrapper player) const;
    void SetIsHidden(PriWrapper player, bool isHidden) const;
    bool GetIsHidden(PriWrapper player) const;
    void Demolish(PriWrapper player) const;

protected:

};
