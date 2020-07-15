// gamemodes/BoostSteal.cpp
// A boost stealing gamemode for Rocket Plugin.
//
// Author:       Stanbroek
// Version:      0.6.3 15/7/20
// BMSDKversion: 95

#include "BoostSteal.h"


/// <summary>Gets called when a car gets demoed.</summary>
/// <param name='car'><see cref="CarWrapper"/> of the car that was demoed</param>
void BoostSteal::onDemolished(CarWrapper car)
{
	if (car.IsNull()) {
		return;
	}

	ServerWrapper game = gameWrapper->GetGameEventAsServer();
	if (game.IsNull()) {
		return;
	}

	std::uintptr_t demolisher = NULL;
	Vector carLoc = car.GetLocation();
	ArrayWrapper<CarWrapper> cars = game.GetCars();
	for (auto i = 0; i < cars.Count(); i++) {
		CarWrapper otherCar = cars.Get(i);
		if (car.IsNull()) {
			continue;
		}

		Vector otherCarLoc = otherCar.GetLocation();
		if (demolisher && (carLoc - otherCarLoc).magnitude() >= (carLoc - CarWrapper(demolisher).GetLocation()).magnitude()) {
			continue;
		}

		if (car.memory_address != otherCar.memory_address) {
			demolisher = otherCar.memory_address;
		}
	}

	CarWrapper demolisherCar = CarWrapper(demolisher);
	if (!demolisher || (carLoc - demolisherCar.GetLocation()).magnitude() > 500) {
		cvarManager->info_log("Could not find the demolisher.");
		return;
	}

	BoostWrapper demolishedBoost = car.GetBoostComponent();
	if (demolishedBoost.IsNull()) {
		cvarManager->info_log("Could not get the demolished boost.");
		return;
	}
	BoostWrapper demolishersBoost = demolisherCar.GetBoostComponent();
	if (demolishersBoost.IsNull()) {
		cvarManager->info_log("Could not get the demolishers boost.");
		return;
	}

	float boostStolen = (demolishedBoost.GetPercentBoostFull() * demolishedBoost.GetMaxBoostAmount()) * (boostConversionPercentage / 100);
	cvarManager->info_log("\"" + demolisherCar.GetOwnerName() + "\" stole " + std::to_string(boostStolen) + " from \""+ car.GetOwnerName() + "\"");
	demolishersBoost.GiveBoost2(boostStolen);
}


/// <summary>Renders the available options for the gamemode.</summary>
void BoostSteal::RenderOptions()
{
    ImGui::SliderFloat("boost converted when demoed", &boostConversionPercentage, 0.0f, 100.0f, "%.1f%%");
}


/// <summary>Gets if the gamemode is active.</summary>
/// <returns>Bool with if the gamemode is active</returns>
bool BoostSteal::IsActive()
{
	return isActive;
}


/// <summary>Activates the gamemode.</summary>
void BoostSteal::Activate(bool active)
{
	if (active && !isActive) {
        HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.OnDemolished", std::bind(&BoostSteal::onDemolished, this, std::placeholders::_1));
	}
	else if (!active && isActive) {
        UnhookEvent("Function TAGame.Car_TA.OnDemolished");
	}

	isActive = active;
}


/// <summary>Gets the gamemodes name.</summary>
/// <returns>The gamemodes name</returns>
std::string BoostSteal::GetGamemodeName()
{
	return "Boost Steal";
}