// GameModes/BoostMod.cpp
// A general team based boost modifier game mode for Rocket Plugin.
//
// Author:        Al12 and Stanbroek
// Version:       0.1.3 24/12/20
// BMSDK version: 95

#include "BoostMod.h"


/// <summary>Util function to render a single set of boost related options.</summary>
/// <param name="boostModifier">Boost modifier to render</param>
/// <param name="toggleable">Bool with if you can disable the modifier</param>
void RenderSingleOption(BoostMod::BoostModifier& boostModifier, const bool toggleable = true)
{
    ImGui::Indent(10);
    if (toggleable) {
        ImGui::SwitchCheckbox(" Enable##BoostModifierEnable", &boostModifier.Enabled);
    }
    if (toggleable && !boostModifier.Enabled) {
        ImGui::BeginDisabled();
    }
    ImGui::TextUnformatted(" Boost modifier:");
    ImGui::SliderArray("##BoostModifierType", &boostModifier.BoostAmountModifier, boostModifier.BoostAmountModifiers);
    ImGui::TextUnformatted(" Boost limit:");
    ImGui::SliderFloat("##BoostLimit", &boostModifier.MaxBoost, 0, 100, "%.0f%%");
    if (toggleable && !boostModifier.Enabled) {
        ImGui::EndDisabled();
    }
    ImGui::Unindent(10);
}


/// <summary>Renders the available options for the game mode.</summary>
void BoostMod::RenderOptions()
{
    // General modifier (everyone).
    ImGui::TextUnformatted("General modifier:");
    RenderSingleOption(boostModifierGeneral, false);
    ImGui::Spacing();
    
    // Team specific modifiers.
    ImGui::TextUnformatted("Team modifiers:");
    for (int teamIdx = 0; static_cast<size_t>(teamIdx) < boostModifierTeams.size(); teamIdx++) {
        ImGui::PushID(teamIdx);
        ImGui::Text("Team %d:", teamIdx);
        RenderSingleOption(boostModifierTeams[teamIdx]);
        ImGui::PopID();
    }
    ImGui::Separator();

    ImGui::TextWrapped("Game mode made by: AL12");
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool BoostMod::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void BoostMod::Activate(const bool active)
{
    if (active && !isActive) {
        HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.Tick",
                                           [this](const ServerWrapper& caller, void*, const std::string&) {
                                               onTick(caller);
                                           });
    }
    else if (!active && isActive) {
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string BoostMod::GetGameModeName()
{
    return "BoostMod";
}


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the server</param>
void BoostMod::onTick(ServerWrapper server)
{
    if (server.IsNull()) {
        ERROR_LOG("could not get the server");
        return;
    }

    for (TeamWrapper team : server.GetTeams()) {
        if (team.IsNull()) {
            WARNING_LOG("could not get team");
            continue;
        }

        BoostModifier boostModifier;
        const int teamIndex = team.GetTeamIndex();
        if (static_cast<size_t>(teamIndex) >= boostModifierTeams.size() || !boostModifierTeams[teamIndex].Enabled) {
            boostModifier = boostModifierGeneral;
        }
        else {
            boostModifier = boostModifierTeams[teamIndex];
        }

        for (PriWrapper player : team.GetMembers()) {
            if (player.IsNull() || player.GetCar().IsNull() || player.GetCar().GetBoostComponent().IsNull()) {
                WARNING_LOG("could not get the players car boost component");
                continue;
            }

            BoostWrapper boostComponent = player.GetCar().GetBoostComponent();
            // Max boost.
            if (boostComponent.GetCurrentBoostAmount() * 100 > boostModifier.MaxBoost) {
                boostComponent.SetBoostAmount(boostModifier.MaxBoost / 100.f);
                boostComponent.ClientGiveBoost(0);
            }
            // Boost modifier.
            boostComponent.SetRechargeRate(0);
            boostComponent.SetRechargeDelay(0);
            switch (boostModifier.BoostAmountModifier) {
                // No boost
            case 1:
                boostComponent.SetBoostAmount(0);
                boostComponent.ClientGiveBoost(0);
                break;
                // Unlimited
            case 2:
                boostComponent.SetBoostAmount(1);
                boostComponent.ClientGiveBoost(0);
                break;
                // Recharge (slow)
            case 3:
                boostComponent.SetRechargeRate(0.06660f);
                boostComponent.SetRechargeDelay(2);
                break;
                // Recharge (fast)
            case 4:
                boostComponent.SetRechargeRate(0.16660f);
                boostComponent.SetRechargeDelay(2);
                break;
            default:
                break;
            }
        }
    }
}
