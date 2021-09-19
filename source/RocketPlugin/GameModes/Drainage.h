#pragma once
#include "GameModes/RocketGameMode.h"


class Drainage final : public RocketGameMode
{
public:
    Drainage() { typeIdx = std::make_unique<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

private:
    void onTick(ServerWrapper server, void* params) const;

    bool autoDeplete = false;
    int autoDepleteRate = 10;
};
