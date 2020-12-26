// GameModes/Juggernaut.cpp
// Scoring makes you the juggernaut, but there can only be one.
//
// Author:        Stanbroek
// Version:       0.2.0 24/12/20
// BMSDK version: 95
//
// BUG's:
//  - Switching teams is buggy.

#include "Juggernaut.h"

constexpr int TEAM_BLUE   = 0;
constexpr int TEAM_ORANGE = 1;
#define NOT_JUGGERNAUT_TEAM (juggernautTeam == TEAM_BLUE ? TEAM_ORANGE : TEAM_BLUE)
#define JUGGERNAUT_TEAM     juggernautTeam


/// <summary>Renders the available options for the game mode.</summary>
void Juggernaut::RenderOptions()
{
    ImGui::Banner("This game mode is temporarily disabled", ImColor(IM_COL32(211, 47, 47, 255)));
    ImGui::TextWrapped("Juggernaut Team:");
    if (ImGui::RadioButton("Blue", juggernautTeam == TEAM_BLUE)) {
        juggernautTeam = TEAM_BLUE;
    }
    if (ImGui::RadioButton("Orange", juggernautTeam == TEAM_ORANGE)) {
        juggernautTeam = TEAM_ORANGE;
    }
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool Juggernaut::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void Juggernaut::Activate(const bool active)
{
    return;
    
    if (active && !isActive) {
        initGame();
        HookEventWithCaller<ActorWrapper>("Function TAGame.PRI_TA.EventScoredGoal",
                                          [this](const ActorWrapper& caller, void*, const std::string&) {
                                              onGoalScored(caller);
                                          });
        HookEventWithCaller<ActorWrapper>("Function TAGame.PRI_TA.GiveScore",
                                          [this](const ActorWrapper& caller, void*, const std::string&) {
                                              onGiveScorePre(caller);
                                          });
        HookEventWithCallerPost<ActorWrapper>("Function TAGame.PRI_TA.GiveScore",
                                              [this](const ActorWrapper& caller, void*, const std::string&) {
                                                  onGiveScorePost(caller);
                                              });
        HookEventWithCaller<ServerWrapper>("Function TAGame.GameEvent_Soccar_TA.InitGame",
                                           [this](const ActorWrapper&, void*, const std::string&) {
                                               initGame();
                                           });
    }
    else if (!active && isActive) {
        UnhookEvent("Function TAGame.PRI_TA.EventScoredGoal");
        UnhookEvent("Function TAGame.PRI_TA.GiveScore");
        UnhookEventPost("Function TAGame.PRI_TA.GiveScore");
        UnhookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string Juggernaut::GetGameModeName()
{
    return "Juggernaut";
}


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'TAGame.PRI_TA.EventScoredGoal'.</remarks>
/// <param name="actor">instance of the PRI as <see cref="ActorWrapper"/></param>
void Juggernaut::onGoalScored(ActorWrapper actor)
{
    if (actor.IsNull()) {
        ERROR_LOG("could not get the actor");
        return;
    }

    std::vector<PriWrapper> players = rocketPlugin->getPlayers();
    for (PriWrapper player : players) {
        player.ServerChangeTeam(NOT_JUGGERNAUT_TEAM);
    }

    PriWrapper player = PriWrapper(actor.memory_address);
    if (player.GetPlayerID() == juggernaut) {
        INFO_LOG("the juggernaut scored");
        juggernaut = -1;
        player.SetMatchGoals(player.GetMatchGoals() + 1);
    }
    else {
        INFO_LOG("the juggernaut got scored on");
        juggernaut = player.GetPlayerID();
        player.ServerChangeTeam(JUGGERNAUT_TEAM);
    }
}


/// <summary>Disables getting score from scoring goals.</summary>
/// <remarks>Gets called on 'Function TAGame.PRI_TA.GiveScore'.</remarks>
/// <param name="actor">instance of the PRI as <see cref="ActorWrapper"/></param>
void Juggernaut::onGiveScorePre(ActorWrapper actor)
{
    if (actor.IsNull()) {
        ERROR_LOG("could not get the car");
        return;
    }

    PriWrapper player = PriWrapper(actor.memory_address);
    lastNormalGoals = player.GetMatchGoals();
}


/// <summary>Disables getting score from scoring goals.</summary>
/// <remarks>Gets called on 'Function TAGame.PRI_TA.GiveScore'.</remarks>
/// <param name="actor">instance of the PRI as <see cref="ActorWrapper"/></param>
void Juggernaut::onGiveScorePost(ActorWrapper actor) const
{
    if (actor.IsNull()) {
        ERROR_LOG("could not get the car");
        return;
    }

    PriWrapper player = PriWrapper(actor.memory_address);
    player.SetMatchGoals(lastNormalGoals);
}


/// <summary>Initialized the game.</summary>
void Juggernaut::initGame()
{
    juggernaut = -1;
    std::vector<PriWrapper> players = rocketPlugin->getPlayers();
    for (PriWrapper player : players) {
        player.ServerChangeTeam(NOT_JUGGERNAUT_TEAM);
    }
}
