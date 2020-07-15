// gamemodes/Tag.cpp
// Tag others by bumping other and try to survive untill the very end.
//
// Author:       Stanbroek
// Version:      0.6.3 15/7/20
// BMSDKversion: 95

#include "Tag.h"


/// <summary>Gets a random player from the players in the match.</summary>
void Tag::tagRandomPlayer()
{
    timeTagged = 0;
    std::vector<PriWrapper> players = rocketPlugin->getPlayers(false, true);

    if (players.size() <= 1) {
        tagged = -1;
        cvarManager->info_log("no players to tag");
        return;
    }

    // I know this is a bias randomizer but no one will notice.
    PriWrapper randomPlayer = players[rand() % players.size()];
    tagged = randomPlayer.GetPlayerID();
    cvarManager->info_log("\"" + randomPlayer.GetPlayerName().ToString() + "\" is now tagged");
    highlightTaggedPlayer(randomPlayer);
}


/// <summary>Highlights the tagged player.</summary>
void Tag::highlightTaggedPlayer(PriWrapper player)
{
    CarWrapper car = player.GetCar();

    switch (taggedOption)
    {
    // None
    case 0:
        break;
    // Unlimited Boost
    case 1:
        if (!car.IsNull() && !car.GetBoostComponent().IsNull()) {
            car.GetBoostComponent().SetUnlimitedBoost2(true);
            car.SetbOverrideBoostOn(true);
        }
        break;
    }
    // TBD
}


/// <summary>Removes the highlight.</summary>
void Tag::removeHighlight(PriWrapper player)
{
    if (player.IsNull()) {
        return;
    }

    // Unlimited Boost
    CarWrapper car = player.GetCar();
    if (!car.IsNull()) {
        car.GetBoostComponent().SetUnlimitedBoost2(false);
        car.SetbOverrideBoostOn(false);
    }
    // TBD
}


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the server</param>
/// <param name="params">Delay since last update</param>
void Tag::onTick(ServerWrapper server, void* params)
{
    if (server.IsNull()) {
        return;
    }

    if (tagged == -1) {
        tagRandomPlayer();
        return;
    }

    // dt since last tick in seconds
    float dt = *((float*)params);
    timeTagged += dt;
    if (timeTagged < timeTillDemolition) {
        return;
    }

    std::vector<PriWrapper> players = rocketPlugin->getPlayers(true);
    for (PriWrapper player : players) {
        if (player.GetPlayerID() == tagged) {
            rocketPlugin->demolish(player);
            player.ServerChangeTeam(-1);
        }
    }

    tagRandomPlayer();
}


/// <summary>Tags the person that got bumped by the tagged player.</summary>
/// <remarks>Gets called on 'Function TAGame.Car_TA.IsBumperHit'.</remarks>
/// <param name="server"><see cref="CarWrapper"/> instance of the car</param>
/// <param name="params">The params the function got called with</param>
void Tag::onCarImpact(CarWrapper car, void* param)
{
    if (car.IsNull() || car.GetPRI().IsNull() || car.GetPRI().GetPlayerID() != tagged) {
        return;
    }
    if (timeTagged < invulnerabilityPeriod) {
        cvarManager->info_log("\"" + car.GetOwnerName() + "\" is invulnerable");
        return;
    }

    CarWrapper otherCar = CarWrapper(*reinterpret_cast<std::uintptr_t*>(param));
    timeTagged = 0;
    tagged = otherCar.GetPRI().GetPlayerID();
    removeHighlight(car.GetPRI());
    cvarManager->info_log("\"" + car.GetOwnerName() + "\" tagged \"" + otherCar.GetOwnerName() + "\"");
    highlightTaggedPlayer(otherCar.GetPRI());
}


/// <summary>Tags the person that got bumped with a rumble item by the tagged player.</summary>
/// <remarks>Gets called on 'TAGame.SpecialPickup_Targeted_TA.TryActivate'.</remarks>
/// <param name="server"><see cref="ActorWrapper"/> instance of the actor</param>
/// <param name="params">The params the function got called with</param>
void Tag::onRumbleItemActivated(ActorWrapper actor, void* /*param*/)
{
    if (!enableRumbleTouches || actor.IsNull()) {
        return;
    }

    cvarManager->log("function not implemented.");
}


/// <summary>Renders the available options for the gamemode.</summary>
void Tag::RenderOptions()
{
    ImGui::BeginDisabled();
    ImGui::Checkbox("Enable Rumble Touches", &enableRumbleTouches);
    ImGui::EndDisabled();
    ImGui::SliderFloat("##TimeTillDemolition", &timeTillDemolition, 1, 60, "%.1f Seconds Till Demolition");
    ImGui::SliderFloat("##InvulnerabilityPeriod", &invulnerabilityPeriod, 0, 1, "%.1f Seconds Invulnerable");
    ImGui::Separator();

    ImGui::TextWrapped("Highlight Tagged Player:");
    if (ImGui::RadioButton("None", taggedOption == 0)) {
        taggedOption = 0;
    }
    if (ImGui::RadioButton("Unlimited Boost", taggedOption == 1)) {
        taggedOption = 1;
    }
    ImGui::Separator();

    ImGui::TextWrapped("Gamemode suggested by: SimpleAOB");
}


/// <summary>Gets if the gamemode is active.</summary>
/// <returns>Bool with if the gamemode is active</returns>
bool Tag::IsActive()
{
	return isActive;
}


/// <summary>Activates the gamemode.</summary>
void Tag::Activate(bool active)
{
    if (active && !isActive) {
        tagRandomPlayer();
        HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.Tick", std::bind(&Tag::onTick, this, std::placeholders::_1, std::placeholders::_2));
        HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.ApplyCarImpactForces", std::bind(&Tag::onCarImpact, this, std::placeholders::_1, std::placeholders::_2));
        HookEventWithCaller<CarWrapper>("Function TAGame.SpecialPickup_Targeted_TA.TryActivate", std::bind(&Tag::onRumbleItemActivated, this, std::placeholders::_1, std::placeholders::_2));
        HookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame", std::bind(&Tag::tagRandomPlayer, this));
    }
    else if (!active && isActive) {
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
        UnhookEvent("Function TAGame.Car_TA.ApplyCarImpactForces");
        UnhookEvent("Function TAGame.SpecialPickup_Targeted_TA.TryActivate");
        UnhookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame");
    }

	isActive = active;
}


/// <summary>Gets the gamemodes name.</summary>
/// <returns>The gamemodes name</returns>
std::string Tag::GetGamemodeName()
{
	return "Tag";
}