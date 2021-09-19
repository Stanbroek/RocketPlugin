// GameControls.cpp
// Game controls for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.6.8 18/09/21
// BMSDK version: 95
#include "GameControls.h"
#include "RocketPlugin.h"


/// <summary>Forces overtime in the current game.</summary>
void GameControls::ForceOvertime() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.ForceOvertime();
}


/// <summary>Pauses the current game.</summary>
void GameControls::PauseServer() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    PlayerControllerWrapper pc = game.GetPauser();
    if (pc.IsNull()) {
        Outer()->cvarManager->executeCommand("mp_pause", false);
    }
    else {
        Outer()->cvarManager->executeCommand("mp_unpause", false);
    }
}


/// <summary>Resets the current game.</summary>
void GameControls::ResetMatch() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.StartNewRound();
}


/// <summary>Ends the current game.</summary>
void GameControls::EndMatch() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.EndGame();
}


/// <summary>Resets the players in the current game.</summary>
void GameControls::ResetPlayers() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.ResetPlayers();
}


/// <summary>Resets the balls in the current game.</summary>
void GameControls::ResetBalls() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.ResetBalls();
}
