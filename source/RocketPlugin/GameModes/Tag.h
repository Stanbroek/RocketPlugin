#pragma once
#include "RocketPlugin.h"
#include "GameModes/RocketGameMode.h"


class Tag final : public RocketGameMode
{
public:
    Tag() { typeIdx = std::make_unique<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

private:
    void tagRandomPlayer();
    void highlightTaggedPlayer() const;
    void addHighlight(PriWrapper player) const;
    void removeHighlightsTaggedPlayer() const;
    void removeHighlights(PriWrapper player) const;
    void onTick(ServerWrapper server, void* params);
    void onCarImpact(CarWrapper car, void* params);
    void onRumbleItemActivated(ActorWrapper caller, void* params);

    const unsigned long long emptyPlayer = static_cast<unsigned long long>(-1);

    bool enableRumbleTouches = false;
    float timeTillDemolition = 10;
    float invulnerabilityPeriod = 0.5f;
    float timeTagged = 0;
    unsigned long long tagged = emptyPlayer;

    enum class TaggedOption
    {
        NONE,
        FORCED_BOOST,
        DIFFERENT_COLOR
    };

    TaggedOption taggedOption = TaggedOption::NONE;
};
