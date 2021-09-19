#pragma once
#include "RocketPluginModule.h"


class RocketPlugin;

class LocalMatchSettings final : RocketPluginModule
{
    friend RocketPlugin;
public:
    void SetMaxPlayers(int newNumPlayers) const;
    int GetMaxPlayers() const;
    void SetMaxTeamSize(int newTeamSize) const;
    int GetMaxTeamSize() const;
    void SetRespawnTime(int newRespawnTime) const;
    int GetRespawnTime() const;
    void SetScore(int team, int newScore) const;
    int GetScore(int team) const;
    void SetScoreBlue(int newScore) const;
    int GetScoreBlue() const;
    void SetScoreOrange(int newScore) const;
    int GetScoreOrange() const;
    void SetGameTimeRemaining(int newGameTimeRemaining) const;
    int GetGameTimeRemaining() const;
    void SetIsGoalDelayDisabled(bool isGoalDelayDisabled) const;
    bool GetIsGoalDelayDisabled() const;
    void SetIsUnlimitedTime(bool isUnlimitedTime) const;
    bool GetIsUnlimitedTime() const;

protected:

};
