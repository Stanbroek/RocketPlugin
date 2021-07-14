// GameModes/BoostSteal.cpp
// A boost stealing game mode for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.1.0 24/12/20
// BMSDK version: 95

#include "BoostSteal.h"


/// <summary>Renders the available options for the game mode.</summary>
void BoostSteal::RenderOptions()
{
    ImGui::SliderFloat("boost converted when demoed", &boostConversionPercentage, 0.0f, 100.0f, "%.1f%%");
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool BoostSteal::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void BoostSteal::Activate(const bool active)
{
    if (active && !isActive) {
        HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.Demolish",
            [this](const CarWrapper& car, void* params, const std::string&) {
                stealBoost(car, params);
            });
    }
    else if (!active && isActive) {
        UnhookEvent("Function TAGame.Car_TA.Demolish");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string BoostSteal::GetGameModeName()
{
    return "Boost Steal";
}
