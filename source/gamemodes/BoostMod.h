#pragma once
#include "../RocketPlugin.h"


class BoostMod : public RocketGameMode
{
private:
	float boostConversionPercentage = 100.0f;

	void onTick(ServerWrapper server, void* params);
	void OnBoostPickUp(ActorWrapper caller, void* params, std::string funcName);
	void setMaxBoost(int newBoost);
	std::vector<TeamWrapper> getTeams();

	struct BoostModValues {
		// A suffix to append to ui elements (ImGui uses the label to cache, so
		// you can't have elements with the same labels.
		std::string displaySuffix = "";

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

	BoostModValues generalBoostSettings{"general", false, 100, 0};

	std::vector<BoostModValues> teamsBoostSettings{ 
		BoostModValues { "Team Blue", false, 100, 0 }, 
		BoostModValues { "Team Orange", false, 100, 0 } };

public:
	BoostMod(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }
	virtual void RenderOptions();
	virtual bool IsActive();
	virtual void Activate(bool active);
	virtual std::string GetGamemodeName();
};
