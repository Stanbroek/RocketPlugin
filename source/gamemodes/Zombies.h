#pragma once
#include "../RocketPlugin.h"


class Zombies final : public RocketGameMode
{
public:
    explicit Zombies(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }
	void RenderOptions() override;
	bool IsActive() override;
	void Activate(bool active) override;
	std::string GetGameModeName() override;

private:
	void prepareZombies(int newNumZombies) const;
	void onTick(ServerWrapper server);

	int numZombies = 5;
	bool zombiesHaveUnlimitedBoost = true;
	size_t selectedPlayer = 0;
};
