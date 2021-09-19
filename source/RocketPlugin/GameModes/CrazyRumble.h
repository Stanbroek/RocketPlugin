#pragma once
#include "RPConfig.h"
#include "GameModes/RocketGameMode.h"

#include "RumbleItems/RumbleItems.h"
#include "RumbleItems/RumbleConstants.inc"


class CrazyRumble final : public RocketGameMode
{
public:
    CrazyRumble()
    {
        typeIdx = std::make_unique<std::type_index>(typeid(*this));

        boot = std::make_unique<BallCarSpringWrapper>(&RumbleConstants::boot);
        rumbleItems.push_back(boot);
        disruptor = std::make_unique<BoostOverrideWrapper>(&RumbleConstants::disruptor);
        rumbleItems.push_back(disruptor);
        freezer = std::make_unique<BallFreezeWrapper>(&RumbleConstants::freezer);
        rumbleItems.push_back(freezer);
        grapplingHook = std::make_unique<GrapplingHookWrapper>(&RumbleConstants::grapplingHook);
        rumbleItems.push_back(grapplingHook);
        haymaker = std::make_unique<BallCarSpringWrapper>(&RumbleConstants::haymaker);
        rumbleItems.push_back(haymaker);
        magnetizer = std::make_unique<GravityWrapper>(&RumbleConstants::magnetizer);
        rumbleItems.push_back(magnetizer);
        plunger = std::make_unique<BattarangWrapper>(&RumbleConstants::plunger);
        rumbleItems.push_back(plunger);
        powerhitter = std::make_unique<HitForceWrapper>(&RumbleConstants::powerhitter);
        rumbleItems.push_back(powerhitter);
        spikes = std::make_unique<VelcroWrapper>(&RumbleConstants::spikes);
        rumbleItems.push_back(spikes);
        swapper = std::make_unique<SwapperWrapper>(&RumbleConstants::swapper);
        rumbleItems.push_back(swapper);
        tornado = std::make_unique<TornadoWrapper>(&RumbleConstants::tornado);
        rumbleItems.push_back(tornado);
        haunted = std::make_unique<HauntedWrapper>(&RumbleConstants::haunted);
        rumbleItems.push_back(haunted);
        rugby = std::make_unique<RugbyWrapper>(&RumbleConstants::rugby);
        rumbleItems.push_back(rugby);
    }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

    void ResetItemsValues();
    void UpdateItemsValues(float newForceMultiplier, float newRangeMultiplier, float newDurationMultiplier);

private:
    void onGiveItem(const ActorWrapper& caller) const;
    void updateRumbleOptions() const;
    void updateRumbleOptions(CarWrapper car) const;
    void updateDispensers(bool updateMaxTimeTillItem = true, bool updateItemPool = true) const;
    void updateDispenserItemPool(const ActorWrapper& dispenser) const;
    void updateDispenserMaxTimeTillItem(const ActorWrapper& dispenser) const;

    float forceMultiplier = 1.0f;
    float rangeMultiplier = 1.0f;
    float durationMultiplier = 1.0f;
    int maxTimeTillItem = 10;

    std::shared_ptr<BallCarSpringWrapper> boot;
    std::shared_ptr<BoostOverrideWrapper> disruptor;
    std::shared_ptr<BallFreezeWrapper> freezer;
    std::shared_ptr<GrapplingHookWrapper> grapplingHook;
    std::shared_ptr<BallCarSpringWrapper> haymaker;
    std::shared_ptr<GravityWrapper> magnetizer;
    std::shared_ptr<BattarangWrapper> plunger;
    std::shared_ptr<HitForceWrapper> powerhitter;
    std::shared_ptr<VelcroWrapper> spikes;
    std::shared_ptr<SwapperWrapper> swapper;
    std::shared_ptr<TornadoWrapper> tornado;
    std::shared_ptr<HauntedWrapper> haunted;
    std::shared_ptr<RugbyWrapper> rugby;

    bool refreshRumbleConstants = true;
    std::vector<std::shared_ptr<RumbleWrapper>> rumbleItems;
    std::future<std::pair<bool, std::string>> rumbleConstantsRequest;
};
