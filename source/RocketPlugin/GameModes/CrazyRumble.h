#pragma once
#include "Config.h"
#include "RocketPlugin.h"
#include "GameModes/RocketGameMode.h"

#include "RumbleItems/RumbleItems.h"


class CrazyRumble final : public RocketGameMode
{
public:
    explicit CrazyRumble(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

    void InitializeItemsValues();
    void ResetItemsValues();
    void UpdateItemsValues(float newForceMultiplier, float newRangeMultiplier, float newDurationMultiplier);

private:
    void onGiveItem(const ActorWrapper& caller) const;
    void updateRumbleOptions(bool update = true) const;
    void updateRumbleOptions(CarWrapper car) const;
    void updateDispensers(bool updateMaxTimeTillItem = true, bool updateItemPool = true) const;
    void updateDispenserItemPool(const ActorWrapper& dispenser) const;
    void updateDispenserMaxTimeTillItem(const ActorWrapper& dispenser) const;

    float forceMultiplier = 1.0f;
    float rangeMultiplier = 1.0f;
    float durationMultiplier = 1.0f;
    int maxTimeTillItem = 10;

    BallCarSpringWrapper boot;
    BoostOverrideWrapper disruptor;
    BallFreezeWrapper freezer;
    GrapplingHookWrapper grapplingHook;
    BallCarSpringWrapper haymaker;
    GravityWrapper magnetizer;
    BattarangWrapper plunger;
    HitForceWrapper powerhitter;
    VelcroWrapper spikes;
    SwapperWrapper swapper;
    TornadoWrapper tornado;
    HauntedWrapper haunted;
    RugbyWrapper rugby;

    bool refreshRumbleConstants = true;
    std::future<std::pair<bool, std::string>> rumbleConstantsRequest;
};
