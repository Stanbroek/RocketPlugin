#pragma once
#include "../RocketPlugin.h"


class KeepAway final : public RocketGameMode
{
public:
    explicit KeepAway(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;
private:
    void onTick(ServerWrapper server, void* params);
    void onGiveScorePre(ActorWrapper actor);
    void onGiveScorePost(ActorWrapper actor) const;
    void onCarTouch(void*) const;
    void resetPoints();

    const unsigned long long emptyPlayer = static_cast<unsigned long long>(-1);

    int lastNormalScore = 0;
    bool enableNormalScore = false;
    bool enableRumbleTouches = false;
    float secPerPoint = 1;
    float timeSinceLastPoint = 0;
    unsigned long long lastTouched = emptyPlayer;
};
