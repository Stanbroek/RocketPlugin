#pragma once
#include "GameModes/RocketGameMode.h"


class BoostShare final : public RocketGameMode
{
public:
    BoostShare() { typeIdx = std::make_unique<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

private:
    void initialize() const;
    void removePickups() const;
    void distributeBoostPool() const;
    void onTick(ServerWrapper server, void* params) const;

    unsigned short boostPool = 100;
};
