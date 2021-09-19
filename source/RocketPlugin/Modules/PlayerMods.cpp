// PlayerMods.cpp
// Player mods for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.6.8 18/09/21
// BMSDK version: 95
#include "PlayerMods.h"
#include "RocketPlugin.h"


/// <summary>Gets the players in the current game.</summary>
/// <param name="includeBots">Bool with if the output should include bots</param>
/// <param name="mustBeAlive">Bool with if the output should only include alive players</param>
/// <returns>List of players</returns>
std::vector<PriWrapper> PlayerMods::GetPlayers(const bool includeBots, const bool mustBeAlive) const
{
    std::vector<PriWrapper> players;
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, players);

    if (mustBeAlive) {
        for (CarWrapper car : game.GetCars()) {
            if (car.IsNull() || car.GetPRI().IsNull() || (!includeBots && car.GetPRI().GetbBot())) {
                continue;
            }

            players.push_back(car.GetPRI());
        }
    }
    else {
        for (PriWrapper pri : game.GetPRIs()) {
            if (pri.IsNull() || (!includeBots && pri.GetbBot())) {
                continue;
            }

            players.push_back(pri);
        }
    }

    return players;
}


/// <summary>Gets the player names of the players in the current game.</summary>
/// <param name="includeBots">Bool with if the output should include bots</param>
/// <param name="mustBeAlive">Bool with if the output should only include alive players</param>
/// <returns>List of players names</returns>
std::vector<std::string> PlayerMods::GetPlayersNames(const bool includeBots, const bool mustBeAlive) const
{
    return GetPlayersNames(GetPlayers(includeBots, mustBeAlive));
}


/// <summary>Gets the player names of the given players.</summary>
/// <param name="players">List of players to get the names from</param>
/// <returns>List of players names</returns>
std::vector<std::string> PlayerMods::GetPlayersNames(const std::vector<PriWrapper>& players) const
{
    std::vector<std::string> playersNames;
    for (PriWrapper player : players) {
        playersNames.push_back(player.GetPlayerName().ToString());
    }

    return playersNames;
}


/// <summary>Sets if the players is admin in the current game.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to update</param>
/// <param name="isAdmin">Bool with if the player should be admin</param>
void PlayerMods::SetIsAdmin(PriWrapper player, const bool isAdmin) const
{
    BMCHECK(player);

    player.SetbMatchAdmin(isAdmin);
}


/// <summary>Gets if the players is admin in the current game.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to check</param>
/// <returns>Bool with if the players is admin</returns>
bool PlayerMods::GetIsAdmin(PriWrapper player) const
{
    BMCHECK_SILENT(player, false);

    return player.GetbMatchAdmin();
}


/// <summary>Sets if the players is hidden in the current game.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to update</param>
/// <param name="isHidden">Bool with if the player should be hidden</param>
void PlayerMods::SetIsHidden(PriWrapper player, const bool isHidden) const
{
    BMCHECK(player);
    BMCHECK(player.GetCar());

    player.GetCar().SetHidden2(isHidden);
    player.GetCar().SetbHiddenSelf(isHidden);
}


/// <summary>Gets if the players is hidden in the current game.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to check</param>
/// <returns>Bool with if the players are hidden</returns>
bool PlayerMods::GetIsHidden(PriWrapper player) const
{
    BMCHECK_SILENT(player, false);
    BMCHECK_SILENT(player.GetCar(), false);

    return player.GetCar().GetbHidden();
}


/// <summary>Demolishes the given player.</summary>
/// <param name="player">The <see cref="PriWrapper"/> to demolish</param>
void PlayerMods::Demolish(PriWrapper player) const
{
    BMCHECK(player);
    BMCHECK(player.GetCar());

    player.GetCar().Demolish();
}
