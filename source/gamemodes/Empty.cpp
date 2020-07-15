// gamemodes/Empty.cpp
// Gamemode description.
//
// Author:       Stanbroek
// Version:      0.6.1 29/3/20
// BMSDKversion: 73

#include "Empty.h"


/// <summary>Updates the game every game tick.</summary>
void Empty::onTick(ServerWrapper server)
{

}


/// <summary>Renders the available options for the gamemode.</summary>
void Empty::RenderOptions()
{

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
		HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.Tick", std::bind(&Empty::onTick, this, std::placeholders::_1));
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