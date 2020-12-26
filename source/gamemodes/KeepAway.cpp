// GameModes/KeepAway.cpp
// Touch the ball to get points.
//
// Author:        Stanbroek
// Version:       0.2.0 24/12/20
// BMSDK version: 95

#include "KeepAway.h"


/// <summary>Renders the available options for the game mode.</summary>
void KeepAway::RenderOptions()
{
    ImGui::Checkbox("Count Rumble Touches", &enableRumbleTouches);
    ImGui::SameLine();
    ImGui::Checkbox("Count Normal Points", &enableNormalScore);
    if (ImGui::SliderFloat("##SecondsPerPoint", &secPerPoint, 0.1f, 10, "%.1f seconds delay per point")) {
        if (secPerPoint < 0.1f) {
            secPerPoint = 0.1f;
        }
    }
    if (ImGui::Button("Reset points")) {
        rocketPlugin->Execute([this](GameWrapper*) {
            resetPoints();
        });
    }
    ImGui::Separator();

    ImGui::TextWrapped("Game mode suggested by: kennyak90");
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool KeepAway::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void KeepAway::Activate(const bool active)
{
    if (active && !isActive) {
        lastTouched = emptyPlayer;
        timeSinceLastPoint = 0;
        HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.Tick",
                                           [this](const ServerWrapper& caller, void* params, const std::string&) {
                                               onTick(caller, params);
                                           });
        HookEventWithCaller<ActorWrapper>("Function TAGame.PRI_TA.GiveScore",
                                          [this](const ActorWrapper& caller, void*, const std::string&) {
                                              onGiveScorePre(caller);
                                          });
        HookEventWithCallerPost<ActorWrapper>("Function TAGame.PRI_TA.GiveScore",
                                              [this](const ActorWrapper& caller, void*, const std::string&) {
                                                  onGiveScorePost(caller);
                                              });
        HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.EventCarTouch",
                                         [this](const BallWrapper&, void* params, const std::string&) {
                                             onCarTouch(params);
                                         });
    }
    else if (!active && isActive) {
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
        UnhookEvent("Function TAGame.PRI_TA.GiveScore");
        UnhookEventPost("Function TAGame.PRI_TA.GiveScore");
        UnhookEvent("Function TAGame.Ball_TA.EventCarTouch");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string KeepAway::GetGameModeName()
{
    return "Keep Away";
}


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the server</param>
/// <param name="params">Delay since last update</param>
void KeepAway::onTick(ServerWrapper server, void* params)
{
    if (server.IsNull()) {
        ERROR_LOG("could not get the server");
        return;
    }

    if (lastTouched == emptyPlayer) {
        return;
    }

    // dt since last tick in seconds
    const float dt = *static_cast<float*>(params);
    timeSinceLastPoint += dt;
    if (timeSinceLastPoint < secPerPoint) {
        return;
    }

    const int points = static_cast<int>(floorf(timeSinceLastPoint / secPerPoint));
    std::vector<PriWrapper> players = rocketPlugin->getPlayers(true);
    for (PriWrapper player : players) {
        if (player.GetUniqueIdWrapper().GetUID() == lastTouched) {
            player.SetMatchScore(player.GetMatchScore() + points);
            player.ForceNetUpdate2();
            TeamInfoWrapper team = player.GetTeam();
            if (!team.IsNull()) {
                rocketPlugin->setScore(player.GetTeamNum(), team.GetScore() + points);
            }
            else {
                WARNING_LOG("could not get players team");
            }
        }
    }
    
    timeSinceLastPoint -= static_cast<float>(points) * secPerPoint;
}


/// <summary>Disables getting score from normal events.</summary>
/// <remarks>Gets called on 'Function TAGame.PRI_TA.GiveScore'.</remarks>
/// <param name="actor">instance of the PRI as <see cref="ActorWrapper"/></param>
void KeepAway::onGiveScorePre(ActorWrapper actor)
{
    if (enableNormalScore) {
        return;
    }
    if (actor.IsNull()) {
        ERROR_LOG("could not get the actor");
        return;
    }

    PriWrapper player = PriWrapper(actor.memory_address);
    lastNormalScore = player.GetMatchScore();
}


/// <summary>Disables getting score from normal events.</summary>
/// <remarks>Gets called on 'Function TAGame.PRI_TA.GiveScore'.</remarks>
/// <param name="actor">instance of the PRI as <see cref="ActorWrapper"/></param>
void KeepAway::onGiveScorePost(ActorWrapper actor) const
{
    if (enableNormalScore) {
        return;
    }
    if (actor.IsNull()) {
        ERROR_LOG("could not get the actor");
        return;
    }

    PriWrapper player = PriWrapper(actor.memory_address);
    player.SetMatchScore(lastNormalScore);
    player.ForceNetUpdate2();
}


/// <summary>Resets the points off all players in the game.</summary>
void KeepAway::resetPoints()
{
    lastTouched = emptyPlayer;
    timeSinceLastPoint = 0;

    ServerWrapper game = rocketPlugin->getGame();
    if (game.IsNull()) {
        ERROR_LOG("could not get local game");
        return;
    }
    
    for (PriWrapper player : game.GetPRIs()) {
        if (player.IsNull()) {
            WARNING_LOG("could not get pri");
            continue;
        }

        player.SetMatchScore(0);
        player.ForceNetUpdate2();
    }
}
