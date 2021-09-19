#pragma once
#include "GameModes/RocketGameMode.h"


class BoostSteal final : public RocketGameMode
{
public:
    BoostSteal() { typeIdx = std::make_unique<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

private:
    void stealBoost(CarWrapper car, void* params) const;

    float boostConversionPercentage = 100.0f;
};
