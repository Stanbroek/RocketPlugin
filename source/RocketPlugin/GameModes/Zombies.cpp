// GameModes/Zombies.cpp
// A zombie survival game mode for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.2.4 28/08/21
// BMSDK version: 95

#include "Zombies.h"


/// <summary>Renders the available options for the game mode.</summary>
void Zombies::RenderOptions()
{
    if (ImGui::InputInt("# Zombies", &numZombies)) {
        Execute([this, newNumZombies = numZombies](GameWrapper*) {
            prepareZombies(newNumZombies);
        });
    }
    std::vector<std::string> playersNames;
    if (Outer()->IsInGame()) {
        playersNames = Outer()->playerMods.GetPlayersNames();
    }
    ImGui::Checkbox("Zombies Have Unlimited Boost", &zombiesHaveUnlimitedBoost);
    ImGui::Combo("Player to hunt", &selectedPlayer, playersNames, "No players found");
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
        HookEvent(
            "Function TAGame.GameEvent_TA.EventMatchStarted", [this](const std::string&) {
                prepareZombies(numZombies);
            });
        HookEventWithCaller<ServerWrapper>(
            "Function GameEvent_Soccar_TA.Active.Tick",
            [this](const ServerWrapper& caller, void*, const std::string&) {
                onTick(caller);
            });
        HookEventWithCaller<ServerWrapper>(
            "Function GameEvent_Soccar_TA.Countdown.BeginState",
            [this](const ServerWrapper& caller, void*, const std::string&) {
                onTick(caller);
            });
        prepareZombies(numZombies);
    }
    else if (!active && isActive) {
        UnhookEvent("Function TAGame.GameEvent_TA.EventMatchStarted");
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
        UnhookEvent("Function GameEvent_Soccar_TA.Countdown.BeginState");
        Outer()->gameControls.ResetBalls();
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
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.SetUnfairTeams(true);
    game.SetbFillWithAI(true);
    Outer()->botSettings.SetNumBotsPerTeam(newNumZombies);
    Outer()->ballMods.SetBallsScale(0.01f);
    BM_TRACE_LOG("set number of bots to {:d}", newNumZombies);
}


/// <summary>Updates the game every game tick.</summary>
void Zombies::onTick(ServerWrapper server)
{
    BMCHECK(server);

    std::vector<PriWrapper> players;
    for (PriWrapper pri : server.GetPRIs()) {
        if (pri.IsNull() || pri.GetbBot()) {
            if (zombiesHaveUnlimitedBoost) {
                CarWrapper car = pri.GetCar();
                BMCHECK_LOOP(car);

                BoostWrapper boostComponent = car.GetBoostComponent();
                BMCHECK_LOOP(boostComponent);

                boostComponent.SetBoostAmount(100.0f);
            }
            continue;
        }

        players.push_back(pri);
    }

    if (selectedPlayer >= players.size()) {
        selectedPlayer = 0;
        BM_ERROR_LOG("selected player is out of range");
        return;
    }

    CarWrapper target = players[selectedPlayer].GetCar();
    BMCHECK(target);

    BallWrapper ball = server.GetBall();
    BMCHECK(ball);

    ball.SetVelocity(Vector(0, 0, 1));
    ball.SetLocation(target.GetLocation());
}
