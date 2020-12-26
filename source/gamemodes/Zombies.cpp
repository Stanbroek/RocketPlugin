// GameModes/Zombies.cpp
// A zombie survival game mode for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.2.1 24/12/20
// BMSDK version: 95

#include "Zombies.h"


/// <summary>Renders the available options for the game mode.</summary>
void Zombies::RenderOptions()
{
    if (ImGui::InputInt("# Zombies", &numZombies)) {
        rocketPlugin->Execute([this, newNumZombies = numZombies](GameWrapper*) {
            prepareZombies(newNumZombies);
        });
    }
    ImGui::Checkbox("Zombies Have Unlimited Boost", &zombiesHaveUnlimitedBoost);
    ImGui::Combo("Player to hunt", &selectedPlayer, rocketPlugin->getPlayersNames(), "No players found");
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool Zombies::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void Zombies::Activate(const bool active)
{
    if (active && !isActive) {
        HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.Tick",
                                           [this](const ServerWrapper& caller, void*, const std::string&) {
                                               onTick(caller);
                                           });
        HookEvent("Function TAGame.GameEvent_TA.Init", [this](const std::string&) {
            prepareZombies(numZombies);
        });
        prepareZombies(numZombies);
    }
    else if (!active && isActive) {
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
        UnhookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame");
        rocketPlugin->resetBalls();
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string Zombies::GetGameModeName()
{
    return "Zombies";
}


/// <summary>Updates the number of zombies that chase you.</summary>
/// <param name="newNumZombies">Number of zombies that chase you</param>
void Zombies::prepareZombies(const int newNumZombies) const
{
    ServerWrapper game = gameWrapper->GetGameEventAsServer();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    game.SetUnfairTeams(true);
    game.SetbFillWithAI(true);
    rocketPlugin->prepareBots(newNumZombies);
    rocketPlugin->setBallsScale(0.01f);
}


/// <summary>Updates the game every game tick.</summary>
void Zombies::onTick(ServerWrapper server)
{
    if (server.IsNull()) {
        ERROR_LOG("could not get the server");
        return;
    }

    std::vector<PriWrapper> players;
    ArrayWrapper<PriWrapper> pris = server.GetPRIs();
    for (int i = 0; i < pris.Count(); i++) {
        PriWrapper pri = pris.Get(i);
        if (pri.IsNull() || pri.GetbBot()) {
            if (zombiesHaveUnlimitedBoost) {
                CarWrapper car = pri.GetCar();
                if (car.IsNull()) {
                    continue;
                }

                BoostWrapper boostComponent = car.GetBoostComponent();
                if (boostComponent.IsNull()) {
                    continue;
                }

                boostComponent.SetBoostAmount(100.0f);
            }
            continue;
        }

        players.push_back(pri);
    }

    if (selectedPlayer >= players.size()) {
        selectedPlayer = 0;
        ERROR_LOG("selected player is out of range");
        return;
    }

    CarWrapper target = players[selectedPlayer].GetCar();
    if (target.IsNull()) {
        ERROR_LOG("could not get the target");
        return;
    }

    BallWrapper ball = server.GetBall();
    if (ball.IsNull()) {
        ERROR_LOG("could not get the ball");
        return;
    }

    ball.SetVelocity(Vector(0, 0, 1));
    ball.SetLocation(target.GetLocation());
}
