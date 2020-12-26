// gamemodes/Empty.cpp
// Gamemode description.
//
// Author:       Stanbroek
// Version:      0.6.4 24/12/20
// BMSDKversion: 95

#include "Empty.h"


/// <summary>Renders the available options for the gamemode.</summary>
void Empty::RenderOptions()
{
    ImGui::TextUnformatted("Placeholder");
}


/// <summary>Gets if the gamemode is active.</summary>
/// <returns>Bool with if the gamemode is active</returns>
bool Empty::IsActive()
{
	return isActive;
}


/// <summary>Activates the gamemode.</summary>
void Empty::Activate(bool active)
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


/// <summary>Gets the gamemodes name.</summary>
/// <returns>The gamemodes name</returns>
std::string Empty::GetGamemodeName()
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
