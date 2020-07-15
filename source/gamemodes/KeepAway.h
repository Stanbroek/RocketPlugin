#pragma once
#include "../RocketPlugin.h"


class KeepAway : public RocketGameMode
{
    private:
        int lastNormalScore = 0;
        bool enableNormalScore = false;
        bool enableRumbleTouches = false;
        float pointPerSec = 1;
        float timeSinceLastPoint = 0;
        int lastTouched = -1;

        void onTick(ServerWrapper server, void* params);
        void onGiveScorePre(ActorWrapper actor);
        void onGiveScorePost(ActorWrapper actor);
        void onCarTouch(BallWrapper server, void* params);
    public:
        KeepAway(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }
        virtual void RenderOptions();
        virtual bool IsActive();
        virtual void Activate(bool active);
        virtual std::string GetGamemodeName();
};
