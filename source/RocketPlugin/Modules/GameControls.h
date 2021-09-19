#pragma once
#include "RocketPluginModule.h"


class RocketPlugin;

class GameControls final : RocketPluginModule
{
    friend RocketPlugin;
public:
    void ForceOvertime() const;
    void PauseServer() const;
    void ResetMatch() const;
    void EndMatch() const;
    void ResetPlayers() const;
    void ResetBalls() const;

protected:

};
