#pragma once
#include "../RocketPlugin.h"


class CrazyRumble final : public RocketGameMode
{
public:
    explicit CrazyRumble(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }

    void updateRumbleOptions() const;
    void setItemGiveRate(int) const;
    void setItemValues(float newForceMultiplier = 1.0f, float newRangeMultiplier = 1.0f,
                       float newDurationMultiplier = 1.0f);
    void updateCars(bool update = true);

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;
private:
    void onGiveItem(const ActorWrapper&) const;
    void setRumbleOptions(CarWrapper car) const;

    float forceMultiplier = 1.0f;
    float rangeMultiplier = 1.0f;
    float durationMultiplier = 1.0f;
    int maxTimeTillItem = 10;

    // Boot - RumblePickupComponentWrapper::TargetedPickup::SpringPickup::BallCarSpringPickup
        // SpringPickup
    float bootForce = 550000.0f;
    float bootVerticalForce = 400000.0f;
    Vector bootTorque = Vector(2000, 0, 0);
    float bootRelativeForceNormalDirection = 1.0f;
    float bootMaxSpringLength = 3500.0f;
    float bootConstantForce = 0.0f;
    float bootMinSpringLength = 30.0f;
    float bootWeldedForceScalar = 3.0f;
    float bootWeldedVerticalForce = 50000.0f;
        // TargetedPickup
    bool bootCanTargetBall = false;
    bool bootCanTargetCars = true;
    bool bootCanTargetEnemyCars = true;
    bool bootCanTargetTeamCars = false;
    float bootRange = 2300.0f;
        // RumblePickupComponentWrapper
    float bootActivationDuration = 99.0f;

    // Disruptor - RumblePickupComponentWrapper::TargetedPickup::BoostOverridePickup
        // TargetedPickup
    bool disruptorCanTargetBall = false;
    bool disruptorCanTargetCars = true;
    bool disruptorCanTargetEnemyCars = true;
    bool disruptorCanTargetTeamCars = false;
    float disruptorRange = 4000.0f;
        // RumblePickupComponentWrapper
    float disruptorActivationDuration = 5.0f;

    // Freezer - RumblePickupComponentWrapper::TargetedPickup::BallFreezePickup
    bool freezerMaintainMomentum = false;
    float freezerTimeToStop = 0.5f;
    float freezerStopMomentumPercentage = 0.0f;
        // TargetedPickup
    bool freezerCanTargetBall = true;
    bool freezerCanTargetCars = false;
    bool freezerCanTargetEnemyCars = false;
    bool freezerCanTargetTeamCars = false;
    float freezerRange = 2300.0f;
        // RumblePickupComponentWrapper
    float freezerActivationDuration = 4.0f;

    // GrapplingHook - RumblePickupComponentWrapper::GrapplingHookPickup
    float grapplingHookImpulse = 80000.0f;
    float grapplingHookForce = 900000.0f;
    float grapplingHookMaxRopeLength = 3500.0f;
    float grapplingHookPredictionSpeed = 3300.0f;
        // TargetedPickup
    bool grapplingHookCanTargetBall = true;
    bool grapplingHookCanTargetCars = false;
    bool grapplingHookCanTargetEnemyCars = false;
    bool grapplingHookCanTargetTeamCars = false;
    float grapplingHookRange = 2800.0f;
        // RumblePickupComponentWrapper
    float grapplingHookActivationDuration = 6.0f;

    // Haymaker - RumblePickupComponentWrapper::TargetedPickup::SpringPickup::BallCarSpringPickup
        // SpringPickup
    float haymakerForce = 80000.0f;
    float haymakerVerticalForce = 0.0f;
    Vector haymakerTorque = Vector(0, 0, 0);
    float haymakerRelativeForceNormalDirection = 1.0f;
    float haymakerMaxSpringLength = 3500.0f;
    float haymakerConstantForce = 0.0f;
    float haymakerMinSpringLength = 30.0f;
    float haymakerWeldedForceScalar = 3.0f;
    float haymakerWeldedVerticalForce = 50000.0f;
        // TargetedPickup
    bool haymakerCanTargetBall = true;
    bool haymakerCanTargetCars = false;
    bool haymakerCanTargetEnemyCars = false;
    bool haymakerCanTargetTeamCars = false;
    float haymakerRange = 2300.0f;
        // RumblePickupComponentWrapper
    float haymakerActivationDuration = 99.0f;

    // Magnetizer - RumblePickupComponentWrapper::GravityPickup
    float magnetizerBallGravity = 55000.0f;
    float magnetizerRange = 1000.0f;
    bool magnetizerDeactivateOnTouch = false;
        // RumblePickupComponentWrapper
    float magnetizerActivationDuration = 6.0f;

    // Plunger - RumblePickupComponentWrapper::TargetedPickup::SpringPickup::BallLassoPickup::BatarangPickup
    float plungerSpinSpeed = 0.0f;
        // SpringPickup
    float plungerForce = 0.0f;
    float plungerVerticalForce = 0.0f;
    Vector plungerTorque = Vector(0, 0, 0);
    float plungerRelativeForceNormalDirection = -1.0f;
    float plungerMaxSpringLength = 3000.0f;
    float plungerConstantForce = -20.0f;
    float plungerMinSpringLength = 30.0f;
    float plungerWeldedForceScalar = 1.0f;
    float plungerWeldedVerticalForce = 0.0f;
        // TargetedPickup
    bool plungerCanTargetBall = true;
    bool plungerCanTargetCars = false;
    bool plungerCanTargetEnemyCars = false;
    bool plungerCanTargetTeamCars = false;
    float plungerRange = 1800.0f;
        // RumblePickupComponentWrapper
    float plungerActivationDuration = 99.0f;

    // PowerHitter - RumblePickupComponentWrapper::HitForcePickup
    bool powerhitterBallForce = true;
    bool powerhitterCarForce = true;
    bool powerhitterDemolishCars = true;
    float powerhitterBallHitForce = 1.5f;
    float powerhitterCarHitForce = 30.0f;
        // RumblePickupComponentWrapper
    float powerhitterActivationDuration = 12.0f;

    // Spikes - RumblePickupComponentWrapper::VelcroPickup
    float spikesAfterHitDuration = 4.5f;
    float spikesPostBreakDuration = 0.6f;
    float spikesMinBreakForce = 1200.0f;
    float spikesMinBreakTime = 0.6f;
    float spikesAttachTime = 0.0f;
    float spikesBreakTime = 0.0f;
        // RumblePickupComponentWrapper
    float spikesActivationDuration = 12.0f;

    // Swapper - RumblePickupComponentWrapper::TargetedPickup::SwapperPickup
        // TargetedPickup
    bool swapperCanTargetBall = false;
    bool swapperCanTargetCars = true;
    bool swapperCanTargetEnemyCars = true;
    bool swapperCanTargetTeamCars = false;
    float swapperRange = 10000.0f;
        // RumblePickupComponentWrapper
    float swapperActivationDuration = 0.75f;

    // Tornado - RumblePickupComponentWrapper::TornadoPickup
    float tornadoHeight = 800.0f;
    float tornadoRadius = 400.0f;
    float tornadoRotationalForce = 7.0f;
    float tornadoTorque = 5.0f;
    Vector tornadoFxScale = Vector(1, 1, 1);
    Vector tornadoMeshScale = Vector(1, 1, 1);
    float tornadoMaxVelocityOffset = 3000.0f;
    float tornadoBallMultiplier = 3.0f;
    float tornadoVelocityEase = 5.0f;
        // RumblePickupComponentWrapper
    float tornadoActivationDuration = 8.0f;

    // Haunted Ball Beam - RumblePickupComponentWrapper
        // TargetedPickup
    float hauntedBallBeamRange = 1200.0f;
        // RumblePickupComponentWrapper
    float hauntedBallBeamActivationDuration = 4.0f;

    // Rugby Spikes - RumblePickupComponentWrapper
        // RumblePickupComponentWrapper
    float rugbySpikesActivationDuration = 99.0f;
};
