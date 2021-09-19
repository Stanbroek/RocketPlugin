#pragma once
#include "RocketPluginModule.h"


class RocketPlugin;

class BotSettings final : RocketPluginModule
{
    friend RocketPlugin;
public:
    void SetMaxNumBots(bool newMaxNumBots) const;
    int GetMaxNumBots() const;
    void SetNumBotsPerTeam(int newNumBotsPerTeam) const;
    int GetNumBotsPerTeam() const;
    void SetIsAutoFilledWithBots(bool isAutoFilledWithBots) const;
    bool GetIsAutoFilledWithBots() const;
    void SetIsUnfairTeams(bool isUnfairTeams) const;
    bool GetIsUnfairTeams() const;
    void FreezeBots() const;

protected:

};
