// BotSettings.cpp
// Bot settings for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.6.8 18/09/21
// BMSDK version: 95
#include "BotSettings.h"
#include "RocketPlugin.h"


/// <summary>Sets the number of bots per team in the current game.</summary>
/// <param name="newMaxNumBots">The new maximum number of bots per team</param>
void BotSettings::SetMaxNumBots(const bool newMaxNumBots) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.SetNumBots(newMaxNumBots);
}


/// <summary>Gets the maximum number of bots per team in the current game.</summary>
/// <returns>The maximum number of bots per team</returns>
int BotSettings::GetMaxNumBots() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, 0);

    return game.GetNumBots();
}


/// <summary>Sets the number of bots per team in the game.</summary>
/// <param name="newNumBotsPerTeam">The new number of bots per team</param>
void BotSettings::SetNumBotsPerTeam(const int newNumBotsPerTeam) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    const int maxTeamSize = game.GetMaxTeamSize();
    const bool isFilledWithAI = game.GetbFillWithAI();
    game.SetMaxTeamSize(newNumBotsPerTeam);
    game.SetbFillWithAI(true);

    const int oldMaxNumBotsPerTeam = game.GetNumBots();
    game.SetNumBots(newNumBotsPerTeam);
    // UpdateBotCount() only adds/removes one bot at a time.
    for (int botsToAdd = std::max(oldMaxNumBotsPerTeam, newNumBotsPerTeam) * 2; botsToAdd > 0; --botsToAdd) {
        game.UpdateBotCount();
    }

    game.SetMaxTeamSize(maxTeamSize);
    game.SetbFillWithAI(isFilledWithAI);
}


/// <summary>Gets the number of bots per team in the game.</summary>
/// <returns>The number of bots per team</returns>
int BotSettings::GetNumBotsPerTeam() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, 0);

    return game.GetNumBots();
}


/// <summary>Sets if bots are auto filled in the current game.</summary>
/// <param name="isAutoFilledWithBots">Bool with if bots should be auto filled</param>
void BotSettings::SetIsAutoFilledWithBots(const bool isAutoFilledWithBots) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.SetbFillWithAI(isAutoFilledWithBots);
}


/// <summary>Gets if bots are auto filled in the current game.</summary>
/// <returns>Bool with if bots are auto filled</returns>
bool BotSettings::GetIsAutoFilledWithBots() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, false);

    return game.GetbFillWithAI();
}


/// <summary>Sets if teams are unfair in the current game.</summary>
/// <param name="isUnfairTeams">Bool with if teams should be unfair</param>
void BotSettings::SetIsUnfairTeams(const bool isUnfairTeams) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.SetbUnfairTeams(isUnfairTeams);
}


/// <summary>Gets if teams are unfair in the current game.</summary>
/// <returns>bool with if teams are unfair</returns>
bool BotSettings::GetIsUnfairTeams() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, false);

    return game.GetbUnfairTeams();
}


/// <summary>Freezes or unfreezes all bots.</summary>
void BotSettings::FreezeBots() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    bool firstBot = true;
    bool shouldUnFreeze = false;
    for (CarWrapper car : game.GetCars()) {
        BMCHECK_LOOP(car);

        PriWrapper player = car.GetPRI();
        BMCHECK_LOOP(player);
        if (!player.GetbBot()) {
            continue;
        }

        if (firstBot) {
            shouldUnFreeze = car.GetbFrozen();
            firstBot = false;
        }

        if (shouldUnFreeze) {
            car.SetFrozen(false);
        }
        else {
            car.SetFrozen(true);
        }
    }
}
