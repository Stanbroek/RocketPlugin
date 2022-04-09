#pragma once
#include "GameModes/RocketGameMode.h"


class KeepAway final : public RocketGameMode
{
public:
    KeepAway() { typeIdx = std::make_unique<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;
    std::string GetGameModeDescription() override;

private:
    void onTick(ServerWrapper server, void* params);
    void onGiveScorePre(PriWrapper player);
    void onGiveScorePost(PriWrapper player) const;
    void onCarTouch(void*);
    void onBallTouch(CarWrapper car);
    void resetPoints();

    const unsigned long long emptyPlayer = static_cast<unsigned long long>(-1);

    int lastNormalScore = 0;
    bool enableNormalScore = false;
    bool enableRumbleTouches = false;
    float secPerPoint = 1;
    float timeSinceLastPoint = 0;
    unsigned long long lastTouched = emptyPlayer;
};
