#pragma once
#include "../RocketPlugin.h"


class BoostSteal final : public RocketGameMode
{
public:
    explicit BoostSteal(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;
private:
    void stealBoost(CarWrapper, void*) const;

    float boostConversionPercentage = 100.0f;
};
