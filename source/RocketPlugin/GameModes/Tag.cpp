// GameModes/Tag.cpp
// Tag others by bumping other and try to survive until the very end.
//
// Author:        Stanbroek
// Version:       0.1.4 12/09/21
// BMSDK version: 95

#include "Tag.h"


/// <summary>Renders the available options for the game mode.</summary>
void Tag::RenderOptions()
{
    ImGui::Checkbox("Enable Rumble Touches", &enableRumbleTouches);
    ImGui::SliderFloat(
        "(0 for infinite)##TimeTillDemolition", &timeTillDemolition, 1, 60, "%.1f Seconds Till Demolition");
    ImGui::SliderFloat("##InvulnerabilityPeriod", &invulnerabilityPeriod, 0, 1, "%.1f Seconds Invulnerable");
    ImGui::Separator();

    ImGui::TextWrapped("Highlight Tagged Player:");
    bool tagOptionChanged = false;
    if (ImGui::RadioButton("None", taggedOption == TaggedOption::NONE)) {
        tagOptionChanged = true;
        taggedOption = TaggedOption::NONE;
    }
    if (ImGui::RadioButton("Forced Boost", taggedOption == TaggedOption::FORCED_BOOST)) {
        tagOptionChanged = true;
        taggedOption = TaggedOption::FORCED_BOOST;
    }
    ImGui::BeginDisabled();
    if (ImGui::RadioButton("Different Color", taggedOption == TaggedOption::DIFFERENT_COLOR)) {
        tagOptionChanged = true;
        taggedOption = TaggedOption::DIFFERENT_COLOR;
    }
    ImGui::EndDisabled();
    if (tagOptionChanged) {
        removeHighlightsTaggedPlayer();
        highlightTaggedPlayer();
    }
    ImGui::Separator();

    ImGui::TextWrapped("Game mode suggested by: SimpleAOB");
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool Tag::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void Tag::Activate(const bool active)
{
    if (active && !isActive) {
        HookEvent(
            "Function TAGame.GameEvent_TA.EventMatchStarted", [this](const std::string&) {
                tagRandomPlayer();
            });
        HookEventWithCaller<ServerWrapper>(
            "Function GameEvent_Soccar_TA.Active.Tick",
            [this](const ServerWrapper& caller, void* params, const std::string&) {
                onTick(caller, params);
            });
        HookEventWithCaller<CarWrapper>(
            "Function TAGame.Car_TA.ApplyCarImpactForces",
            [this](const CarWrapper& caller, void* params, const std::string&) {
                onCarImpact(caller, params);
            });
        HookEventWithCaller<ActorWrapper>(
            "Function TAGame.SpecialPickup_Targeted_TA.TryActivate",
            [this](const ActorWrapper& caller, void* params, const std::string&) {
                onRumbleItemActivated(caller, params);
            });
        tagRandomPlayer();
    }
    else if (!active && isActive) {
        removeHighlightsTaggedPlayer();
        UnhookEvent("Function TAGame.GameEvent_TA.EventMatchStarted");
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
        UnhookEvent("Function TAGame.Car_TA.ApplyCarImpactForces");
        UnhookEvent("Function TAGame.SpecialPickup_Targeted_TA.TryActivate");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string Tag::GetGameModeName()
{
    return "Tag";
}


/// <summary>Gets a random player from the players in the match.</summary>
void Tag::tagRandomPlayer()
{
    timeTagged = 0;
    const std::vector<PriWrapper> players = Outer()->playerMods.GetPlayers(false, true);
    if (players.size() <= 1) {
        if (tagged != emptyPlayer) {
            tagged = emptyPlayer;
            BM_WARNING_LOG("no players to tag");
        }
        return;
    }

    static std::random_device rd;
    static std::default_random_engine generator(rd());
    const std::uniform_int_distribution<size_t> distribution(0, players.size() - 1);
    PriWrapper randomPlayer = players[distribution(generator)];
    tagged = randomPlayer.GetUniqueIdWrapper().GetUID();
    BM_TRACE_LOG("{:s} is now tagged", quote(randomPlayer.GetPlayerName().ToString()));
    addHighlight(randomPlayer);
}


/// <summary>Highlights the tagged player.</summary>
void Tag::highlightTaggedPlayer() const
{
    const std::vector<PriWrapper> players = Outer()->playerMods.GetPlayers(true);
    for (PriWrapper player : players) {
        if (player.GetUniqueIdWrapper().GetUID() == tagged) {
            addHighlight(player);
        }
    }
}


/// <summary>Highlights the given player.</summary>
/// <param name="player">player to give the highlight to</param>
void Tag::addHighlight(PriWrapper player) const
{
    CarWrapper car = player.GetCar();

    switch (taggedOption) {
        case TaggedOption::NONE:
            break;
        case TaggedOption::FORCED_BOOST:
            if (!car.IsNull() && !car.GetBoostComponent().IsNull()) {
                car.GetBoostComponent().SetUnlimitedBoost2(true);
                car.SetbOverrideBoostOn(true);
            }
            break;
        case TaggedOption::DIFFERENT_COLOR:
            break;
    }
}


/// <summary>Removes the highlights from the tagged player.</summary>
void Tag::removeHighlightsTaggedPlayer() const
{
    const std::vector<PriWrapper> players = Outer()->playerMods.GetPlayers(true);
    for (PriWrapper player : players) {
        if (player.GetUniqueIdWrapper().GetUID() == tagged) {
            removeHighlights(player);
        }
    }
}


/// <summary>Removes the highlight from the given player.</summary>
/// <param name="player">player to remove the highlight from</param>
void Tag::removeHighlights(PriWrapper player) const
{
    BMCHECK(player);

    // Unlimited Boost
    CarWrapper car = player.GetCar();
    BMCHECK(car);

    car.GetBoostComponent().SetUnlimitedBoost2(false);
    car.SetbOverrideBoostOn(false);
    // Different Color
    // TBD
}


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the server</param>
/// <param name="params">Delay since last update</param>
void Tag::onTick(ServerWrapper server, void* params)
{
    BMCHECK(server);

    if (tagged == emptyPlayer) {
        tagRandomPlayer();
        return;
    }

    // dt since last tick in seconds
    const float dt = *static_cast<float*>(params);
    timeTagged += dt;
    if (timeTillDemolition == 0.f || timeTagged < timeTillDemolition) {
        return;
    }

    const std::vector<PriWrapper> players = Outer()->playerMods.GetPlayers(true);
    for (PriWrapper player : players) {
        if (player.GetUniqueIdWrapper().GetUID() == tagged) {
            Outer()->playerMods.Demolish(player);
            player.ServerChangeTeam(-1);
        }
    }

    tagRandomPlayer();
}
