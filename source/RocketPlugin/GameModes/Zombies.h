#pragma once
#include "GameModes/RocketGameMode.h"


class Zombies final : public RocketGameMode
{
public:
    Zombies() { typeIdx = std::make_unique<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

private:
    void prepareZombies(int newNumZombies) const;
    void onTick(ServerWrapper server);

    int numZombies = 5;
    bool zombiesHaveUnlimitedBoost = true;
    size_t selectedPlayer = 0;
};
