// GameModes/BoostMod.cpp
// A boost-type game mode for Rocket Plugin.
//
// Author:        Naoki95957
// Version:       0.1.5 07/04/22
// BMSDK version: 95

//extra koodos to AL12 from BoostMod since I mostly moddified off that + some bois from the flock for the idea. 

#include "BoostPop.h"


/// <summary>Renders the available options for the game mode.</summary>
void BoostPop::RenderOptions()
{
    // Team specific modifiers.
    ImGui::TextUnformatted("Team modifiers:");
    for (int teamIdx = 0; static_cast<size_t>(teamIdx) < boostModifierTeams.size(); teamIdx++) {
        ImGui::PushID(teamIdx);
        ImGui::Text(teamIdx ? "Orange Team" : "Blue Team", teamIdx);
        renderSingleOption(boostModifierTeams[teamIdx]);
        ImGui::PopID();
    }
    ImGui::Separator();

    ImGui::TextWrapped("Game mode made by: Naoki95957");
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool BoostPop::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void BoostPop::Activate(const bool active)
{
    if (active && !isActive) {
        HookEventWithCaller<ServerWrapper>(
            "Function GameEvent_Soccar_TA.Active.Tick",
            [this](const ServerWrapper& caller, void*, const std::string&) {
                onTick(caller);
            });
        HookEventWithCaller<ServerWrapper>(
            "Function GameEvent_Soccar_TA.Countdown.BeginState",
            [this](const ServerWrapper& caller, void*, const std::string&) {
                onTick(caller);
            });
    }
    else if (!active && isActive) {
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
        UnhookEvent("Function GameEvent_Soccar_TA.Countdown.BeginState");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string BoostPop::GetGameModeName()
{
    return "Boost Pop";
}


/// <summary>Gets the game modes description.</summary>
/// <returns>The game modes description</returns>
std::string BoostPop::GetGameModeDescription()
{
    return "A gamemode where getting a set amount of boost causes your car to get demolished!";
}


/// <summary>Util function to render a single set of boost related options.</summary>
/// <param name="boostModifier">Boost modifier to render</param>
/// <param name="toggleable">Bool with if you can disable the modifier</param>
void BoostPop::renderSingleOption(BoostPopModifier& mod, const bool toggleable) const
{
    ImGui::SwitchCheckbox(" Enable##BoostModifierEnable", &mod.Enabled);
    if (!mod.Enabled) {
        ImGui::BeginDisabled();
    }
    ImGui::Indent(10);
    ImGui::TextUnformatted(" Boost modifier:");
    ImGui::SliderArray("##BoostModifierType", &mod.BoostAmountModifier, mod.BoostAmountModifiers);
    ImGui::TextUnformatted(" Boost limit:");
    ImGui::SliderFloat("##BoostLimit", &mod.MaxBoost, 0, 100, "%.0f%%"); 
    if (!mod.Enabled) {
        ImGui::EndDisabled();
    }
    ImGui::Unindent(10);
}


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the server</param>
void BoostPop::onTick(ServerWrapper server)
{

    BMCHECK(server);
    for (TeamWrapper team : server.GetTeams()) {
        BMCHECK_LOOP(team);
                    
        BoostPopModifier boostModifier;
        const int teamIndex = team.GetTeamIndex();
        boostModifier = boostModifierTeams[teamIndex];
        if (!boostModifier.Enabled) continue;

        //clamp boost amounts to not encounter something unexpected (like a crash idk)
        boostModifier.MaxBoost = std::min(boostModifier.MaxBoost, MAXIMUM_BOOST);
        boostModifier.MaxBoost = std::max(boostModifier.MaxBoost, (float)0);
        for (PriWrapper player : team.GetMembers()) {
            BMCHECK_LOOP(player);

            CarWrapper car = player.GetCar();
            BMCHECK_LOOP(car);

            BoostWrapper boostComponent = car.GetBoostComponent();
            BMCHECK_LOOP(boostComponent);

            // Boost modifier.  
            boostComponent.SetRechargeRate(0);
            boostComponent.SetRechargeDelay(0);
            switch (boostModifier.BoostAmountModifier) {
                // Recharge (slow)
            case 1:
                boostComponent.SetRechargeRate(0.06660f);
                boostComponent.SetRechargeDelay(2);
                break;
                // Recharge (fast)
            case 2:
                boostComponent.SetRechargeRate(0.16660f);
                boostComponent.SetRechargeDelay(2);
                break;
            default:
                break;
            }

            if (boostComponent.GetCurrentBoostAmount() >= (boostModifier.MaxBoost / 100.0f)) {
                boostComponent.GetCar().Demolish();
            }
        }
    }
}
