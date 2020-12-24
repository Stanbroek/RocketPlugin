#pragma once
#include "../RocketPlugin.h"


class BoostMod : public RocketGameMode
{
private:

	void onTick(ServerWrapper server, void* params);

	struct BoostModValues {
		// Whether the modifier should be considered
		bool enabled = false;

		// Maximum amount of boost player can have. Range: 0-100
		int maxBoost = 100;

		// Type of boost:
		// 0: default
		// 1: no boost
		// 2: unlimited
		// 3: recharge (sloW)
		// 4; recharge (fast)
		std::size_t boostAmountType = 0;
	};

	void RenderSingleOption(BoostModValues& boostSettings);

	const std::vector<std::string> boostAmounts = { "Default", "No Boost", "Unlimited", "Recharge (slow)", "Recharge (fast)" };

	BoostModValues generalBoostSettings{ false, 100, 0 };

	std::vector<BoostModValues> teamsBoostSettings{ 
		BoostModValues { false, 100, 0 }, 
		BoostModValues { false, 100, 0 } };

public:
	BoostMod(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }
	virtual void RenderOptions();
	virtual bool IsActive();
	virtual void Activate(bool active);
	virtual std::string GetGamemodeName();
};
