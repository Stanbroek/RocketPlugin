// gamemodes/KeepAway.cpp
// Touch the ball to get points.
//
// Author:       Stanbroek
// Version:      0.6.3 15/7/20
// BMSDKversion: 95

#include "KeepAway.h"


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the server</param>
/// <param name="params">Delay since last update</param>
void KeepAway::onTick(ServerWrapper server, void* params)
{
    if (server.IsNull()) {
        return;
    }

    // dt since last tick in seconds
    float dt = *((float*)params);
    timeSinceLastPoint += dt;
    if (timeSinceLastPoint < 1 / pointPerSec || timeSinceLastPoint == 0) {
        return;
    }

    std::vector<PriWrapper> players = rocketPlugin->getPlayers(true);
    for (PriWrapper player : players) {
        if (player.GetPlayerID() == lastTouched) {
            player.SetMatchScore(player.GetMatchScore() + (int)roundf(pointPerSec * timeSinceLastPoint));
        }
    }

    timeSinceLastPoint = fmod(timeSinceLastPoint, 1 / pointPerSec);
}


/// <summary>Disables gettings score from normal events.</summary>
/// <remarks>Gets called on 'Function TAGame.PRI_TA.GiveScore'.</remarks>
/// <param name="actor">instance of the PRI as <see cref="ActorWrapper"/></param>
void KeepAway::onGiveScorePre(ActorWrapper actor)
{
    if (enableNormalScore || actor.IsNull()) {
        return;
    }

    PriWrapper player = PriWrapper(actor.memory_address);
    lastNormalScore = player.GetMatchScore();
}


/// <summary>Disables gettings score from normal events.</summary>
/// <remarks>Gets called on 'Function TAGame.PRI_TA.GiveScore'.</remarks>
/// <param name="actor">instance of the PRI as <see cref="ActorWrapper"/></param>
void KeepAway::onGiveScorePost(ActorWrapper actor)
{
    if (enableNormalScore || actor.IsNull()) {
        return;
    }

    PriWrapper player = PriWrapper(actor.memory_address);
    player.SetMatchScore(lastNormalScore);
}


/// <summary>Check who touched the ball.</summary>
/// <remarks>Gets called on 'Function TAGame.Ball_TA.EventCarTouch'.</remarks>
/// <param name="server"><see cref="BallWrapper"/> instance of the ball</param>
/// <param name="params">The params the function got called with</param>
void KeepAway::onCarTouch(BallWrapper, void*)
{
    cvarManager->error_log("function not implemented.");
}


/// <summary>Renders the available options for the gamemode.</summary>
void KeepAway::RenderOptions()
{
    ImGui::Checkbox("Count Rumble Touches", &enableRumbleTouches);
    ImGui::SameLine();
    ImGui::Checkbox("Count Normal Points", &enableNormalScore);
    if (ImGui::SliderFloat("##PointsPerSecond", &pointPerSec, 0.1f, 10, "%.1f points per second")) {
        if (pointPerSec <= 0) {
            pointPerSec = 0.1f;
        }
    }
    ImGui::Separator();

    ImGui::TextWrapped("Gamemode suggested by: kennyak90");
}


/// <summary>Gets if the gamemode is active.</summary>
/// <returns>Bool with if the gamemode is active</returns>
bool KeepAway::IsActive()
{
    return isActive;
}


/// <summary>Activates the gamemode.</summary>
void KeepAway::Activate(bool active)
{
    if (active && !isActive) {
        lastTouched = -1;
        HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.Tick", std::bind(&KeepAway::onTick, this, std::placeholders::_1, std::placeholders::_2));
        HookEventWithCaller<ActorWrapper>("Function TAGame.PRI_TA.GiveScore", std::bind(&KeepAway::onGiveScorePre, this, std::placeholders::_1));
        HookEventWithCallerPost<ActorWrapper>("Function TAGame.PRI_TA.GiveScore", std::bind(&KeepAway::onGiveScorePost, this, std::placeholders::_1));
        //HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.EventHitBall", std::bind(&KeepAway::onBallTouch, this, std::placeholders::_1, std::placeholders::_2));
        HookEventWithCaller<BallWrapper>("Function TAGame.Ball_TA.EventCarTouch", std::bind(&KeepAway::onCarTouch, this, std::placeholders::_1, std::placeholders::_2));
    }
    else if (!active && isActive) {
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
        UnhookEvent("Function TAGame.PRI_TA.GiveScore");
        UnhookEventPost("Function TAGame.PRI_TA.GiveScore");
        UnhookEvent("Function TAGame.Ball_TA.EventCarTouch");
    }

    isActive = active;
}


/// <summary>Gets the gamemodes name.</summary>
/// <returns>The gamemodes name</returns>
std::string KeepAway::GetGamemodeName()
{
    return "Keep Away";
}