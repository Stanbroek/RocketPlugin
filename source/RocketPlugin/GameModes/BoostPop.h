#pragma once
#include "GameModes/RocketGameMode.h"
#include <algorithm>


class BoostPop final : public RocketGameMode
{
public:
    BoostPop() { typeIdx = std::make_unique<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;
    std::string GetGameModeDescription() override;

private:

    const float MAXIMUM_BOOST = 101;

    struct BoostPopModifier
    {
        // Whether the modifier should be considered.
        bool Enabled = false;
        // Maximum amount of boost player can have. (Range: 0-100)
        float MaxBoost = 100.0f;
        // Since the boost gauge is actually a percent reading this may or may not confuse ppl
        bool BoostAsPercent = false;
        // Boost amount modifiers.
        std::size_t BoostAmountModifier = 0;
        std::vector<std::string> BoostAmountModifiers = {
            "Default", "Recharge (slow)", "Recharge (fast)"
        };
    };

    void renderSingleOption(BoostPopModifier& mod, bool toggleable = true) const;
    void onTick(ServerWrapper server);

    BoostPopModifier boostModifierGeneral;
    std::array<BoostPopModifier, 2> boostModifierTeams;
};
