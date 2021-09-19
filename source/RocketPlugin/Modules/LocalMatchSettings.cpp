// LocalMatchSettings.cpp
// Local match settings for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.6.8 18/09/21
// BMSDK version: 95
#include "LocalMatchSettings.h"
#include "RocketPlugin.h"


/// <summary>Sets max players in the current game.</summary>
/// <param name="newNumPlayers">New number of players</param>
void LocalMatchSettings::SetMaxPlayers(const int newNumPlayers) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.SetMaxPlayers(newNumPlayers);
}


/// <summary>Gets max players in the current game.</summary>
/// <returns>Max number of players</returns>
int LocalMatchSettings::GetMaxPlayers() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, 16);

    return game.GetMaxPlayers();
}


/// <summary>Sets max team size in the current game.</summary>
/// <param name="newTeamSize">New team size</param>
void LocalMatchSettings::SetMaxTeamSize(const int newTeamSize) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.SetMaxTeamSize(newTeamSize);
}


/// <summary>Gets max team size in the current game.</summary>
/// <returns>Max team size</returns>
int LocalMatchSettings::GetMaxTeamSize() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, 3);

    return game.GetMaxTeamSize();
}


/// <summary>Sets respawn time in the current game.</summary>
/// <param name="newRespawnTime">New respawn time</param>
void LocalMatchSettings::SetRespawnTime(const int newRespawnTime) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.SetRespawnTime(newRespawnTime);
}


/// <summary>Gets respawn time in the current game.</summary>
/// <returns>Respawn time</returns>
int LocalMatchSettings::GetRespawnTime() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, 3);

    return game.GetRespawnTime();
}


/// <summary>Sets the score of the given team in the current game.</summary>
/// <param name="team">Team to set the score of</param>
/// <param name="newScore">New score</param>
void LocalMatchSettings::SetScore(int team, int newScore) const
{
    switch (team) {
    case 0:
        return SetScoreBlue(newScore);
    case 1:
        return SetScoreOrange(newScore);
    default:
        BM_ERROR_LOG("team #{:d} not found", team);
    }
}


/// <summary>Gets the score of the given team in the current game.</summary>
/// <param name="team">Team to get the score of</param>
/// <returns>Teams score</returns>
int LocalMatchSettings::GetScore(int team) const
{
    switch (team) {
    case 0:
        return GetScoreBlue();
    case 1:
        return GetScoreOrange();
    default:
        BM_ERROR_LOG("invalid team #{:d}", team);
        return 0;
    }
}


/// <summary>Sets blues score in the current game.</summary>
/// <param name="newScore">New score</param>
void LocalMatchSettings::SetScoreBlue(const int newScore) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    ArrayWrapper<TeamWrapper> teams = game.GetTeams();
    if (teams.Count() < 1) {
        BM_ERROR_LOG("could not get the blue team");
        return;
    }

    TeamWrapper blueTeam = teams.Get(0);
    BMCHECK(blueTeam);

    blueTeam.SetScore(newScore);
}


/// <summary>Gets blues score in the current game.</summary>
/// <returns>Blues score</returns>
int LocalMatchSettings::GetScoreBlue() const
{
    ServerWrapper game = Outer()->GetGame(true);
    BMCHECK_SILENT(game, 0);

    ArrayWrapper<TeamWrapper> teams = game.GetTeams();
    if (teams.Count() < 1) {
        BM_ERROR_LOG("could not get the blue team");
        return 0;
    }

    TeamWrapper blueTeam = teams.Get(0);
    BMCHECK(blueTeam, 0);

    return blueTeam.GetScore();
}


/// <summary>Sets oranges score in the current game.</summary>
/// <param name="newScore">New score</param>
void LocalMatchSettings::SetScoreOrange(const int newScore) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    ArrayWrapper<TeamWrapper> teams = game.GetTeams();
    if (teams.Count() < 2) {
        BM_ERROR_LOG("could not get the orange team");
        return;
    }

    TeamWrapper orangeTeam = teams.Get(1);
    BMCHECK(orangeTeam);

    orangeTeam.SetScore(newScore);
}


/// <summary>Gets oranges score in the current game.</summary>
/// <returns>Oranges score</returns>
int LocalMatchSettings::GetScoreOrange() const
{
    ServerWrapper game = Outer()->GetGame(true);
    BMCHECK_SILENT(game, 0);

    ArrayWrapper<TeamWrapper> teams = game.GetTeams();
    if (teams.Count() < 2) {
        BM_ERROR_LOG("could not get the orange team");
        return 0;
    }

    TeamWrapper orangeTeam = teams.Get(1);
    BMCHECK(orangeTeam, 0);

    return orangeTeam.GetScore();
}


/// <summary>Sets the time remaining in the current game.</summary>
/// <param name="newGameTimeRemaining">The new time remaining</param>
void LocalMatchSettings::SetGameTimeRemaining(const int newGameTimeRemaining) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.SetGameTimeRemaining(static_cast<float>(newGameTimeRemaining));
}


/// <summary>Gets the time remaining in the current game.</summary>
/// <returns>The time remaining</returns>
int LocalMatchSettings::GetGameTimeRemaining() const
{
    ServerWrapper game = Outer()->GetGame(true);
    BMCHECK_SILENT(game, 0);

    return static_cast<int>(ceil(game.GetGameTimeRemaining()));
}


/// <summary>Sets if the goal delay is disabled in the current game.</summary>
/// <param name="isGoalDelayDisabled">Bool with if the goal delay should be disabled</param>
void LocalMatchSettings::SetIsGoalDelayDisabled(const bool isGoalDelayDisabled) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.SetbDisableGoalDelay(isGoalDelayDisabled);
}


/// <summary>Gets if the goal delay is disabled in the current game.</summary>
/// <returns>Bool with if the goal delay is disabled</returns>
bool LocalMatchSettings::GetIsGoalDelayDisabled() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, false);

    return game.GetbDisableGoalDelay();
}


/// <summary>Sets if there is unlimited time in the current game.</summary>
/// <param name="isUnlimitedTime">Bool with if there should be unlimited time</param>
void LocalMatchSettings::SetIsUnlimitedTime(const bool isUnlimitedTime) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.SetbUnlimitedTime(isUnlimitedTime);
}


/// <summary>Gets if there is unlimited time in the current game.</summary>
/// <returns>bool with if there is unlimited time</returns>
bool LocalMatchSettings::GetIsUnlimitedTime() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, false);

    return game.GetbUnlimitedTime();
}
