// gamemodes/Zombies.cpp
// A zombie survival gamemode for Rocket Plugin.
//
// Author:       Stanbroek
// Version:      0.6.3 15/7/20
// BMSDKversion: 95

#include "Zombies.h"


/// <summary>Get the player names in the current server.</summary>
/// <returns>The player names in the current server</returns>
std::vector<std::string> Zombies::getPlayerNames()
{
	std::vector<std::string> getPlayerNames;

	ServerWrapper server = gameWrapper->GetGameEventAsServer();
	if (server.IsNull()) {
		return getPlayerNames;
	}

	ArrayWrapper<PriWrapper> PRIs = server.GetPRIs();
	for (int i = 0; i < PRIs.Count(); i++) {
		PriWrapper PRI = PRIs.Get(i);
		if (PRI.IsNull() || PRI.GetbBot()) {
			continue;
		}

		getPlayerNames.push_back(PRI.GetPlayerName().ToString());
	}

	return getPlayerNames;
}


/// <summary>Get the players in the current server.</summary>
/// <param name='server'><see cref="ServerWrapper"/> instance of the current server</param>
/// <returns>The players in the current server</returns>
std::vector<PriWrapper> Zombies::getPlayers(ServerWrapper server)
{
	std::vector<PriWrapper> players;
	ArrayWrapper<PriWrapper> PRIs = server.GetPRIs();
	for (int i = 0; i < PRIs.Count(); i++) {
		PriWrapper PRI = PRIs.Get(i);
		if (PRI.IsNull() || PRI.GetbBot()) {
			continue;
		}

		players.push_back(PRI);
	}

	return players;
}


/// <summary>Sets the bots to be able to join next reset.</summary>
void Zombies::prepareZombies(int newNumBots)
{
	ServerWrapper server = gameWrapper->GetGameEventAsServer();
	if (server.IsNull()) {
		return;
	}
	int numPlayers = server.GetNumHumans();

	server.SetUnfairTeams(true);
	server.SetbFillWithAI(true);
	server.SetMaxPlayers(numPlayers + newNumBots);
	server.SetMaxTeamSize(newNumBots);
	server.SetNumBots(newNumBots);
}


/// <summary>Updates the game every game tick.</summary>
void Zombies::onTick(ServerWrapper server)
{
	if (server.IsNull()) {
		return;
	}

	std::vector<PriWrapper> players;
	ArrayWrapper<PriWrapper> PRIs = server.GetPRIs();
	for (int i = 0; i < PRIs.Count(); i++) {
		PriWrapper PRI = PRIs.Get(i);
		if (PRI.IsNull() || PRI.GetbBot()) {
			if (zombiesHaveUnlimitedBoost) {
				CarWrapper car = PRI.GetCar();
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

		players.push_back(PRI);
	}

	if (selectedPlayer >= (int)players.size()) {
		selectedPlayer = 0;
		return;
	}

	CarWrapper target = players[selectedPlayer].GetCar();
	if (target.IsNull()) {
		return;
	}

	BallWrapper ball = server.GetBall();
	if (ball.IsNull()) {
		return;
	}

	ball.SetDrawScale(0.01f);
	ball.SetBallScale(0.01f);
	ball.SetReplicatedBallScale(0.01f);
	ball.SetVelocity(Vector(0, 0, 1));
	ball.SetLocation(target.GetLocation()); 
}


/// <summary>Renders the available options for the gamemode.</summary>
void Zombies::RenderOptions()
{
	if (ImGui::InputInt("# Zombies", &numZombies)) {
		gameWrapper->Execute([this, newnumZombies = numZombies](GameWrapper*) {
			prepareZombies(newnumZombies);
		});
	}
	ImGui::Checkbox("Zombies Have Unlimited Boost", &zombiesHaveUnlimitedBoost);
	ImGui::Combo("Player to hunt", &selectedPlayer, getPlayerNames(), "No players found");
}


/// <summary>Gets if the gamemode is active.</summary>
/// <returns>Bool with if the gamemode is active</returns>
bool Zombies::IsActive()
{
	return isActive;
}


/// <summary>Activates the gamemode.</summary>
void Zombies::Activate(bool active)
{
	if (active && !isActive) {
        HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.Tick", std::bind(&Zombies::onTick, this, std::placeholders::_1));
        HookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame", std::bind(&Zombies::prepareZombies, this, numZombies));
	}
	else if (!active && isActive) {
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
        UnhookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame");
		rocketPlugin->resetBalls();
	}

	isActive = active;
}


/// <summary>Gets the gamemodes name.</summary>
/// <returns>The gamemodes name</returns>
std::string Zombies::GetGamemodeName()
{
	return "Zombies";
}