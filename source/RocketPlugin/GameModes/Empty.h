#pragma once
#include "GameModes/RocketGameMode.h"


class Empty final : public RocketGameMode
{
public:
    Empty() { typeIdx = std::make_unique<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

private:
    void onTick(ServerWrapper server, void* params);
};
