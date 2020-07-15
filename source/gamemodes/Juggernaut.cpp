// gamemodes/Juggernaut.cpp
// Gamemode description.
//
// Author:       Stanbroek
// Version:      0.6.3 15/7/20
// BMSDKversion: 95

#include "Juggernaut.h"

#define TEAM_BLUE 0
#define TEAM_ORANGE  1
#define NOT_JUGGERNAUT_TEAM (juggernautTeam == TEAM_BLUE ? TEAM_ORANGE : TEAM_BLUE)
#define JUGGERNAUT_TEAM     juggernautTeam


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'TAGame.PRI_TA.EventScoredGoal'.</remarks>
/// <param name="actor">instance of the PRI as <see cref="ActorWrapper"/></param>
void Juggernaut::onGoalScored(ActorWrapper actor)
{
    if (actor.IsNull()) {
        return;
    }

    std::vector<PriWrapper> players = rocketPlugin->getPlayers();
    for (PriWrapper player : players) {
        player.ServerChangeTeam(NOT_JUGGERNAUT_TEAM);
    }

    PriWrapper player = PriWrapper(actor.memory_address);
    if (player.GetPlayerID() == juggernaut) {
        cvarManager->info_log("the juggernaut scored");
        juggernaut = -1;
        player.SetMatchGoals(player.GetMatchGoals() + 1);
    }
    else {
        cvarManager->info_log("the juggernaut got scored on");
        juggernaut = player.GetPlayerID();
        player.ServerChangeTeam(JUGGERNAUT_TEAM);
    }
}


/// <summary>Disables gettings score from scoring goals.</summary>
/// <remarks>Gets called on 'Function TAGame.PRI_TA.GiveScore'.</remarks>
/// <param name="actor">instance of the PRI as <see cref="ActorWrapper"/></param>
void Juggernaut::onGiveScorePre(ActorWrapper actor)
{
    if (actor.IsNull()) {
        return;
    }

    PriWrapper player = PriWrapper(actor.memory_address);
    lastNormalGoals = player.GetMatchGoals();
}


/// <summary>Disables gettings score from scoring goals.</summary>
/// <remarks>Gets called on 'Function TAGame.PRI_TA.GiveScore'.</remarks>
/// <param name="actor">instance of the PRI as <see cref="ActorWrapper"/></param>
void Juggernaut::onGiveScorePost(ActorWrapper actor)
{
    if (actor.IsNull()) {
        return;
    }

    PriWrapper player = PriWrapper(actor.memory_address);
    player.SetMatchGoals(lastNormalGoals);
}


/// <summary>Initialised the game.</summary>
void Juggernaut::initGame(ServerWrapper server)
{
    juggernaut = -1;
    std::vector<PriWrapper> players = rocketPlugin->getPlayers();
    for (PriWrapper player : players) {
        player.ServerChangeTeam(NOT_JUGGERNAUT_TEAM);
    }
}


/// <summary>Renders the available options for the gamemode.</summary>
void Juggernaut::RenderOptions()
{
    ImGui::TextWrapped("Juggernaut Team:");
    if (ImGui::RadioButton("Blue", juggernautTeam == 0)) {
        juggernautTeam = 0;
    }
    if (ImGui::RadioButton("Orange", juggernautTeam == 1)) {
        juggernautTeam = 1;
    }
}


/// <summary>Gets if the gamemode is active.</summary>
/// <returns>Bool with if the gamemode is active</returns>
bool Juggernaut::IsActive()
{
	return isActive;
}


/// <summary>Activates the gamemode.</summary>
void Juggernaut::Activate(bool active)
{
    if (active && !isActive) {
        initGame(gameWrapper->GetGameEventAsServer());
        HookEventWithCaller<ActorWrapper>("Function TAGame.PRI_TA.EventScoredGoal", std::bind(&Juggernaut::onGoalScored, this, std::placeholders::_1));
        HookEventWithCaller<ActorWrapper>("Function TAGame.PRI_TA.GiveScore", std::bind(&Juggernaut::onGiveScorePre, this, std::placeholders::_1));
        HookEventWithCallerPost<ActorWrapper>("Function TAGame.PRI_TA.GiveScore", std::bind(&Juggernaut::onGiveScorePost, this, std::placeholders::_1));
        HookEventWithCaller<ServerWrapper>("Function TAGame.GameEvent_Soccar_TA.InitGame", std::bind(&Juggernaut::initGame, this, std::placeholders::_1));
    }
    else if (!active && isActive) {
        UnhookEvent("Function TAGame.PRI_TA.EventScoredGoal");
        UnhookEvent("Function TAGame.PRI_TA.GiveScore");
        UnhookEventPost("Function TAGame.PRI_TA.GiveScore");
        UnhookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame");
    }

    isActive = active;
}


/// <summary>Gets the gamemodes name.</summary>
/// <returns>The gamemodes name</returns>
std::string Juggernaut::GetGamemodeName()
{
	return "Juggernaut";
}