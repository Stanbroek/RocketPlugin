#pragma once
#include "RocketPlugin.h"
#include "GameModes/RocketGameMode.h"


class SacredGround final : public RocketGameMode
{
public:
    explicit SacredGround(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }
    
    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

private:
    void onTick(ServerWrapper server) const;

    bool demoOnGround = false;
};
