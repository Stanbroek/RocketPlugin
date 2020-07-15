#pragma once
#include "../RocketPlugin.h"


class Tag : public RocketGameMode
{
	private:
        bool enableRumbleTouches = false;
        float timeTillDemolition = 10;
        float invulnerabilityPeriod = 0.5f;
        float timeTagged = 0;
        int tagged = -1;
        unsigned short taggedOption = 0;

        void tagRandomPlayer();
        void highlightTaggedPlayer(PriWrapper player);
        void removeHighlight(PriWrapper player);
        void onTick(ServerWrapper server, void* params);
        void onCarImpact(CarWrapper car, void* param);
        void onRumbleItemActivated(ActorWrapper actor, void* param);
	public:
        Tag(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }
		virtual void RenderOptions();
		virtual bool IsActive();
		virtual void Activate(bool active);
		virtual std::string GetGamemodeName();
};
