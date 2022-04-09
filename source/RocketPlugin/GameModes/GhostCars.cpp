// GameModes/GhostCars.cpp
// Ghost cars game mode for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.0.2 07/04/22
// BMSDK version: 95

#include "GhostCars.h"


/// <summary>Renders the available options for the game mode.</summary>
void GhostCars::RenderOptions()
{
    bool update = false;
    update |= ImGui::Checkbox("Enable Ball Collision", &enableBallCollision);
    update |= ImGui::Checkbox("Enable Vehicle Collision", &enableVehicleCollision);
    ImGui::Separator();
    ImGui::TextColoredWrapped(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled),
        "* This game mode uses custom Rocket Plugin networking to properly work,"
        "which means that each client has to enable the game mode in their menu.");

    if (update) {
        Execute([this](GameWrapper*) {
            serverUpdateRBCollidesWithChannels("Update");
            updateRBCollidesWithChannels();
        });
    }
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool GhostCars::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void GhostCars::Activate(const bool active)
{
    if (active && !isActive) {
        serverUpdateRBCollidesWithChannels("Activate true");
        HookEventWithCaller<ObjectWrapper>(
            "Function Engine.PrimitiveComponent.InitRBPhys",
            [this](const ObjectWrapper& caller, void*, const std::string&) {
                setRBCollidesWithChannel(caller);
            });
        Execute([this](GameWrapper*) {
            updateRBCollidesWithChannels();
        });
    }
    else if (!active && isActive) {
        serverUpdateRBCollidesWithChannels("Activate false");
        UnhookEvent("Function Engine.PrimitiveComponent.InitRBPhys");
        Execute([this](GameWrapper*) {
            resetRBCollidesWithChannels();
        });
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string GhostCars::GetGameModeName()
{
    return "Ghost Cars";
}


/// <summary>Gets the game modes description.</summary>
/// <returns>The game modes description</returns>
std::string GhostCars::GetGameModeDescription()
{
    return "GhostCars allows you to change the collision channels of\n"
           "the cars, so you can change how the ball and or cars\n"
           "collide with each other.";
}


/// <summary>Gets called when it receives a networked message from the host.</summary>
/// <param name="sender">Original sender of the message</param>
/// <param name="message">Message contents</param>
void GhostCars::Receive(PriWrapper sender, const std::string& message)
{
    BMCHECK(sender);
    BM_TRACE_LOG("{:s} send: {:s}", quote(sender.GetPlayerName().ToString()), quote(message));

    std::string params;
    if (message.starts_with("Activate true ")) {
        networked = true;
        params = message.substr(14);
    }
    else if (message.starts_with("Activate false ")) {
        networked = false;
        params = message.substr(15);
    }
    else if (message.starts_with("Update ")) {
        params = message.substr(7);
    }

    if (!isActive) {
        return;
    }

    if (params == "false false") {
        enableBallCollision = false;
        enableVehicleCollision = false;
        clientUpdateRBCollidesWithChannels();
    }
    else if (params == "false true") {
        enableBallCollision = false;
        enableVehicleCollision = true;
        clientUpdateRBCollidesWithChannels();
    }
    else if (params == "true false") {
        enableBallCollision = true;
        enableVehicleCollision = false;
        clientUpdateRBCollidesWithChannels();
    }
    else if (params == "true true") {
        enableBallCollision = true;
        enableVehicleCollision = true;
        clientUpdateRBCollidesWithChannels();
    }
}


/// <summary>Broadcast messages to the clients.</summary>
/// <param name="prefix">Prefix of the message</param>
void GhostCars::serverUpdateRBCollidesWithChannels(const std::string& prefix) const
{
    if (Outer()->IsHostingLocalGame()) {
        BroadcastMessage(fmt::format("{:s} {} {}", prefix, enableBallCollision, enableVehicleCollision));
    }
}
