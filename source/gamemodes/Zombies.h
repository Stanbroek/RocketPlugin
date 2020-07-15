#pragma once
#include "../RocketPlugin.h"


class Zombies : public RocketGameMode
{
	private:
		int numZombies = 5;
		bool zombiesHaveUnlimitedBoost = true;
		size_t selectedPlayer = 0;

		std::vector<std::string> getPlayerNames();
		std::vector<PriWrapper> getPlayers(ServerWrapper server);
		void prepareZombies(int newNumBots);
		void onTick(ServerWrapper server);
	public:
        Zombies(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }
		virtual void RenderOptions();
		virtual bool IsActive();
		virtual void Activate(bool active);
		virtual std::string GetGamemodeName();
};
