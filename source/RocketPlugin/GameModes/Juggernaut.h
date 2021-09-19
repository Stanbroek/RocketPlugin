#pragma once
#include "GameModes/RocketGameMode.h"


class Juggernaut final : public RocketGameMode
{
public:
    Juggernaut() { typeIdx = std::make_unique<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

private:
    void onGoalScored(ActorWrapper caller);
    void onGiveScorePre(ActorWrapper caller);
    void onGiveScorePost(ActorWrapper caller) const;
    void initGame();

    int juggernautTeam = 1;
    int juggernaut = -1;
    int lastNormalGoals = 0;
};
