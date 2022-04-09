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
    std::string GetGameModeDescription() override;

private:
    void stealBoost(CarWrapper, void*) const;

    float boostConversionPercentage = 100.0f;
};
