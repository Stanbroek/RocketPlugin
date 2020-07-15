// gamemodes/Drainage.cpp
// A boost draining gamemode for Rocket Plugin.
//
// Author:       Stanbroek
// Version:      0.6.3 15/7/20
// BMSDKversion: 95

#include "Drainage.h"


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the current server</param>
/// <param name="params">Delay since last update</param>
void Drainage::onTick(ServerWrapper server, void* params)
{
	if (server.IsNull()) {
		return;
	}

	// dt since last tick in seconds
	float dt = *((float*)params);

	ArrayWrapper<CarWrapper> cars = server.GetCars();
	for (int i = 0; i < cars.Count(); i++) {
		CarWrapper car = cars.Get(i);
		if (car.IsNull()) {
			continue;
		}

		BoostWrapper boost = car.GetBoostComponent();
		if (boost.IsNull()) {
			continue;
		}

		float boostAmount = boost.GetCurrentBoostAmount();
		if (boostAmount > 0) {
			if (autoDeplete) {
				boost.GiveBoost2((dt * autoDepleteRate) / 100 * -1);
			}
		}
		else {
			cvarManager->info_log("\"" + car.GetOwnerName() + "\" exploded");
			car.Demolish2(car);
		}
	}
}


/// <summary>Renders the available options for the gamemode.</summary>
void Drainage::RenderOptions()
{
	ImGui::Checkbox("Auto Deplete Boost", &autoDeplete);
	ImGui::SliderInt("Auto Deplete Boost Rate", &autoDepleteRate, 0, 100, "%d boost per second");
}


/// <summary>Gets if the gamemode is active.</summary>
/// <returns>Bool with if the gamemode is active</returns>
bool Drainage::IsActive()
{
	return isActive;
}


/// <summary>Activates the gamemode.</summary>
void Drainage::Activate(bool active)
{
	if (active && !isActive) {
		HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.Tick", std::bind(&Drainage::onTick, this, std::placeholders::_1, std::placeholders::_2));
	}
	else if (!active && isActive) {
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
	}

	isActive = active;
}


/// <summary>Gets the gamemodes name.</summary>
/// <returns>The gamemodes name</returns>
std::string Drainage::GetGamemodeName()
{
	return "Drainage";
}