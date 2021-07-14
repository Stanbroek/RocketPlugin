// GameModes/Empty.cpp
// TODO, game mode description.
//
// Author:        Stanbroek
// Version:       0.0.1 00/00/00
// BMSDK version: 95

#include "Empty.h"


/// <summary>Renders the available options for the game mode.</summary>
void Empty::RenderOptions()
{
    ImGui::TextUnformatted("Placeholder");
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool Empty::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void Empty::Activate(const bool active)
{
    if (active && !isActive) {
        HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.Tick",
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
std::string Empty::GetGameModeName()
{
    return "Empty";
}


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the current server</param>
/// <param name="params">Delay since last update</param>
void Empty::onTick(ServerWrapper server, void* params)
{

}
