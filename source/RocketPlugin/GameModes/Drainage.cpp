// GameModes/Drainage.cpp
// A boost draining game mode for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.1.1 15/08/21
// BMSDK version: 95

#include "Drainage.h"


/// <summary>Renders the available options for the game mode.</summary>
void Drainage::RenderOptions()
{
    ImGui::Checkbox("Auto Deplete Boost", &autoDeplete);
    ImGui::SliderInt("Auto Deplete Boost Rate", &autoDepleteRate, 0, 100, "%d boost per second");
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool Drainage::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void Drainage::Activate(const bool active)
{
    if (active && !isActive) {
        HookEventWithCaller<ServerWrapper>(
            "Function GameEvent_Soccar_TA.Active.Tick",
            [this](const ServerWrapper& caller, void* params, const std::string&) {
                onTick(caller, params);
            });
    }
    else if (!active && isActive) {
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string Drainage::GetGameModeName()
{
    return "Drainage";
}


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the current server</param>
/// <param name="params">Delay since last update</param>
void Drainage::onTick(ServerWrapper server, void* params) const
{
    BMCHECK(server);
    NULLCHECK(params);

    // dt since last tick in seconds
    const float dt = *static_cast<float*>(params);

    for (CarWrapper car : server.GetCars()) {
        BMCHECK_LOOP(car);

        BoostWrapper boost = car.GetBoostComponent();
        BMCHECK_LOOP(boost);

        const float boostAmount = boost.GetCurrentBoostAmount();
        if (boostAmount > 0) {
            if (autoDeplete) {
                boost.GiveBoost2(dt * static_cast<float>(autoDepleteRate) / 100 * -1);
            }
        }
        else {
            BM_TRACE_LOG("{:s} exploded", quote(car.GetOwnerName()));
            car.Demolish2(*dynamic_cast<RBActorWrapper*>(&car));
        }
    }
}
