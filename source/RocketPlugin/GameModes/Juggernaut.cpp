// GameModes/Juggernaut.cpp
// Scoring makes you the juggernaut, but there can only be one.
//
// Author:        Stanbroek
// Version:       0.2.2 28/08/21
// BMSDK version: 95

#include "Juggernaut.h"

constexpr int TEAM_BLUE   = 0;
constexpr int TEAM_ORANGE = 1;
#define NOT_JUGGERNAUT_TEAM (juggernautTeam == TEAM_BLUE ? TEAM_ORANGE : TEAM_BLUE)
#define JUGGERNAUT_TEAM     juggernautTeam


/// <summary>Renders the available options for the game mode.</summary>
void Juggernaut::RenderOptions()
{
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
    if (active && !isActive) {
        HookEventWithCaller<ServerWrapper>(
            "Function TAGame.GameEvent_TA.EventMatchStarted",
            [this](const ActorWrapper&, void*, const std::string&) {
                initGame();
            });
        HookEventWithCaller<ActorWrapper>(
            "Function TAGame.PRI_TA.EventScoredGoal",
            [this](const ActorWrapper& caller, void*, const std::string&) {
                onGoalScored(caller);
            });
        HookEventWithCaller<ActorWrapper>(
            "Function TAGame.PRI_TA.GiveScore",
            [this](const ActorWrapper& caller, void*, const std::string&) {
                onGiveScorePre(caller);
            });
        HookEventWithCallerPost<ActorWrapper>(
            "Function TAGame.PRI_TA.GiveScore",
            [this](const ActorWrapper& caller, void*, const std::string&) {
                onGiveScorePost(caller);
            });
        initGame();
    }
    else if (!active && isActive) {
        UnhookEvent("Function TAGame.GameEvent_TA.EventMatchStarted");
        UnhookEvent("Function TAGame.PRI_TA.EventScoredGoal");
        UnhookEvent("Function TAGame.PRI_TA.GiveScore");
        UnhookEventPost("Function TAGame.PRI_TA.GiveScore");
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
/// <param name="caller">instance of the PRI as <see cref="ActorWrapper"/></param>
void Juggernaut::onGoalScored(ActorWrapper caller)
{
    PriWrapper scorer = PriWrapper(caller.memory_address);
    BMCHECK(scorer);

    const std::vector<PriWrapper> players = Outer()->playerMods.GetPlayers();
    for (PriWrapper player : players) {
        player.ServerChangeTeam(NOT_JUGGERNAUT_TEAM);
    }

    if (scorer.GetPlayerID() == juggernaut) {
        BM_TRACE_LOG("the juggernaut scored");
        juggernaut = -1;
        scorer.SetMatchGoals(scorer.GetMatchGoals() + 1);
    }
    else {
        BM_TRACE_LOG("the juggernaut got scored on");
        juggernaut = scorer.GetPlayerID();
        scorer.ServerChangeTeam(JUGGERNAUT_TEAM);
    }
}


/// <summary>Disables getting score from scoring goals.</summary>
/// <remarks>Gets called on 'Function TAGame.PRI_TA.GiveScore'.</remarks>
/// <param name="caller">instance of the PRI as <see cref="ActorWrapper"/></param>
void Juggernaut::onGiveScorePre(ActorWrapper caller)
{
    PriWrapper player = PriWrapper(caller.memory_address);
    BMCHECK(player);

    lastNormalGoals = player.GetMatchGoals();
}


/// <summary>Disables getting score from scoring goals.</summary>
/// <remarks>Gets called on 'Function TAGame.PRI_TA.GiveScore'.</remarks>
/// <param name="caller">instance of the PRI as <see cref="ActorWrapper"/></param>
void Juggernaut::onGiveScorePost(ActorWrapper caller) const
{
    PriWrapper player = PriWrapper(caller.memory_address);
    BMCHECK(player);

    player.SetMatchGoals(lastNormalGoals);
}


/// <summary>Initialized the game.</summary>
void Juggernaut::initGame()
{
    juggernaut = -1;
    const std::vector<PriWrapper> players = Outer()->playerMods.GetPlayers();
    for (PriWrapper player : players) {
        player.ServerChangeTeam(NOT_JUGGERNAUT_TEAM);
    }
    BM_TRACE_LOG("initialized juggernaut game mode");
}
