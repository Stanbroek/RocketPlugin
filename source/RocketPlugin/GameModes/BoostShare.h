#pragma once
#include "RocketPlugin.h"
#include "GameModes/RocketGameMode.h"


class BoostShare final : public RocketGameMode
{
public:
    explicit BoostShare(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }

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
