// gamemodes/BoostMod.cpp
// A general team based boost modifier gamemode for Rocket Plugin.
// Credits to Stanbroek (rocket plugin author) 
// and ubelhj (author of SpeedBoost plugin, source of some of the code here).
// 
// Author:       Al12
// Version:      0.1.1 28/07/20
// BMSDKversion: 95

#include "BoostMod.h"


/// <summary>Checks and updates boost every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the current server</param>
/// <param name="params">Delay since last update</param>
void BoostMod::onTick(ServerWrapper server, void*)
{
	if (server.IsNull()) {
		return;
	}

	float maxBoost = 1.0f;

	if (generalBoostSettings.enabled) {
		maxBoost = (float)generalBoostSettings.maxBoost / 100.0f;
	}

	auto teams = server.GetTeams();

	for (int teamIdx = 0; teamIdx < teams.Count(); teamIdx++) {
		float currentTeamMax = maxBoost;

		if (teamIdx < teamsBoostSettings.size() && teamsBoostSettings[teamIdx].enabled) {
			currentTeamMax = (float)teamsBoostSettings[teamIdx].maxBoost / 100.0f;
		}
		
		if (generalBoostSettings.enabled || (teamIdx < teamsBoostSettings.size() 
				&& teamsBoostSettings[teamIdx].enabled)) {
			auto team = teams.Get(teamIdx);
			if (team.IsNull()) { continue; }
			auto players = team.GetMembers();

			for (int playerIdx = 0; playerIdx < players.Count(); playerIdx++) {
				auto player = players.Get(playerIdx);
				if (player.IsNull()) { continue; }

				auto car = player.GetCar();
				if (car.IsNull()) { continue; }

				auto boost = car.GetBoostComponent();
				if (boost.IsNull()) { continue; }

				if (boost.GetCurrentBoostAmount() > currentTeamMax) {
					boost.SetBoostAmount(currentTeamMax);
				}
			}
		}
	}
}

/// Util function to render a single set of boost related options
void BoostMod::RenderSingleOption(BoostModValues& boostSettings)
{
	ImGui::Spacing();
	if (ImGui::SwitchCheckbox(" Enable ", 
		boostSettings.enabled)) 
	{
		boostSettings.enabled = !boostSettings.enabled;
	}
	if (boostSettings.enabled) {
		//TODO: implement boost amount type modifier.
		//ImGui::SliderArray(("Boost Amount Type" + boostSettings.displaySuffix).c_str(), &boostSettings.boostAmountType, boostAmounts);
		ImGui::SliderInt("Boost Limit ",
			&boostSettings.maxBoost, 0, 100, "%d%%");
	}
	else {
		ImGui::BeginDisabled();
		//ImGui::SliderArray(("Boost Amount Type" + boostSettings.displaySuffix).c_str(), &boostSettings.boostAmountType, boostAmounts);
		ImGui::SliderInt("Boost Limit ", &boostSettings.maxBoost, 0, 100, "%d%%");
		ImGui::EndDisabled();
	}
	ImGui::Spacing();
}

/// <summary>Renders the available options for the gamemode.</summary>
void BoostMod::RenderOptions()
{
	// general modifier (everyone), no ID
	ImGui::Text("General:");
	ImGui::Indent(20);

	RenderSingleOption(generalBoostSettings);

	ImGui::Unindent();

	
	// team specific
	if (ImGui::CollapsingHeader("Team modifiers:")) {
		ImGui::Indent(20);
		for (int teamIdx = 0; teamIdx < teamsBoostSettings.size(); teamIdx++) {
			ImGui::PushID(teamIdx);

			if (ImGui::CollapsingHeader((" Team " + std::to_string(teamIdx)).c_str())) {
				ImGui::Indent(20);

				RenderSingleOption(teamsBoostSettings[teamIdx]);

				ImGui::Unindent();
			}

			ImGui::PopID();
		}
		ImGui::Unindent();
	}

	ImGui::Separator();

	ImGui::TextWrapped("Gamemode made by: AL12");
}


/// <summary>Gets if the gamemode is active.</summary>
/// <returns>Bool with if the gamemode is active</returns>
bool BoostMod::IsActive()
{
	return isActive;
}


/// <summary>Activates the gamemode.</summary>
void BoostMod::Activate(bool active)
{
	if (active && !isActive) {
		HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.Tick", std::bind(&BoostMod::onTick, this, std::placeholders::_1, std::placeholders::_2));
	}
	else if (!active && isActive) {
		UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
		UnhookEvent("Function TAGame.VehiclePickup_Boost_TA.Pickup");
	}

	isActive = active;
}


/// <summary>Gets the gamemodes name.</summary>
/// <returns>The gamemodes name</returns>
std::string BoostMod::GetGamemodeName()
{
	return "Boost Mods";
}