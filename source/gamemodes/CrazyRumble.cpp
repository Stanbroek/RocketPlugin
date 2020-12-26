// GameModes/CrazyRumble.cpp
// A crazy rumble customizer game mode for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.2.4 24/12/20
// BMSDK version: 95
//
// BUG's:
//  - Rumble settings don't apply anymore.

#include "CrazyRumble.h"


/// <summary>Updates the rumble options for every car.</summary>
void CrazyRumble::updateRumbleOptions() const
{
    ServerWrapper game = gameWrapper->GetGameEventAsServer();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    ArrayWrapper<CarWrapper> cars = game.GetCars();
    for (auto i = 0; i < cars.Count(); i++) {
        CarWrapper car = cars.Get(i);
        if (car.IsNull()) {
            ERROR_LOG("could not get the car");
            continue;
        }

        setRumbleOptions(car);
    }
}


/// <summary>Resets the rumble item values, with an optional multiplier.</summary>
/// <param name="newForceMultiplier">Multiplier of the force</param>
/// <param name="newRangeMultiplier">Multiplier of the range</param>
/// <param name="newDurationMultiplier">Multiplier of the duration</param>
void CrazyRumble::setItemValues(const float newForceMultiplier, const float newRangeMultiplier,
                                const float newDurationMultiplier)
{
    forceMultiplier = newForceMultiplier;
    rangeMultiplier = newRangeMultiplier;
    durationMultiplier = newDurationMultiplier;

    // Boot - RumblePickupComponentWrapper::TargetedPickup::SpringPickup::BallCarSpringPickup
    //      SpringPickup
    bootForce = 550000.0f * forceMultiplier;
    bootVerticalForce = 400000.0f * forceMultiplier;
    bootTorque = Vector(2000, 0, 0) * forceMultiplier;
    bootRelativeForceNormalDirection = 1.0f * forceMultiplier;
    bootMaxSpringLength = 3500.0f * rangeMultiplier;
    bootConstantForce = 0.0f * forceMultiplier;
    bootMinSpringLength = 30.0f * rangeMultiplier;
    bootWeldedForceScalar = 3.0f * forceMultiplier;
    bootWeldedVerticalForce = 50000.0f * forceMultiplier;
    //      TargetedPickup
    bootCanTargetBall = false;
    bootCanTargetCars = true;
    bootCanTargetEnemyCars = true;
    bootCanTargetTeamCars = false;
    bootRange = 2300.0f * rangeMultiplier;
    //      RumblePickupComponentWrapper
    bootActivationDuration = 99.0f * durationMultiplier;

    // Disruptor - RumblePickupComponentWrapper::TargetedPickup::BoostOverridePickup
    //      TargetedPickup
    disruptorCanTargetBall = false;
    disruptorCanTargetCars = true;
    disruptorCanTargetEnemyCars = true;
    disruptorCanTargetTeamCars = false;
    disruptorRange = 4000.0f * rangeMultiplier;
    //      RumblePickupComponentWrapper
    disruptorActivationDuration = 5.0f * durationMultiplier;

    // Freezer - RumblePickupComponentWrapper::TargetedPickup::BallFreezePickup
    freezerMaintainMomentum = false;
    freezerTimeToStop = 0.5f * durationMultiplier;
    freezerStopMomentumPercentage = 0.0f * durationMultiplier;
    //      TargetedPickup
    freezerCanTargetBall = true;
    freezerCanTargetCars = false;
    freezerCanTargetEnemyCars = false;
    freezerCanTargetTeamCars = false;
    freezerRange = 2300.0f * rangeMultiplier;
    //      RumblePickupComponentWrapper
    freezerActivationDuration = 4.0f * durationMultiplier;

    // GrapplingHook - RumblePickupComponentWrapper::GrapplingHookPickup
    grapplingHookImpulse = 80000.0f * forceMultiplier;
    grapplingHookForce = 900000.0f * forceMultiplier;
    grapplingHookMaxRopeLength = 3500.0f * rangeMultiplier;
    grapplingHookPredictionSpeed = 3300.0f * durationMultiplier;
    //      TargetedPickup
    grapplingHookCanTargetBall = true;
    grapplingHookCanTargetCars = false;
    grapplingHookCanTargetEnemyCars = false;
    grapplingHookCanTargetTeamCars = false;
    grapplingHookRange = 2800.0f * rangeMultiplier;
    //      RumblePickupComponentWrapper
    grapplingHookActivationDuration = 6.0f * durationMultiplier;

    // Haymaker - RumblePickupComponentWrapper::TargetedPickup::SpringPickup::BallCarSpringPickup
    //      SpringPickup
    haymakerForce = 80000.0f * forceMultiplier;
    haymakerVerticalForce = 0.0f * forceMultiplier;
    haymakerTorque = Vector(0, 0, 0) * forceMultiplier;
    haymakerRelativeForceNormalDirection = 1.0f * forceMultiplier;
    haymakerMaxSpringLength = 3500.0f * rangeMultiplier;
    haymakerConstantForce = 0.0f * forceMultiplier;
    haymakerMinSpringLength = 30.0f * rangeMultiplier;
    haymakerWeldedForceScalar = 3.0f * forceMultiplier;
    haymakerWeldedVerticalForce = 50000.0f * forceMultiplier;
    //      TargetedPickup
    haymakerCanTargetBall = true;
    haymakerCanTargetCars = false;
    haymakerCanTargetEnemyCars = false;
    haymakerCanTargetTeamCars = false;
    haymakerRange = 2300.0f * rangeMultiplier;
    //      RumblePickupComponentWrapper
    haymakerActivationDuration = 99.0f * durationMultiplier;

    // Magnetizer - RumblePickupComponentWrapper::GravityPickup
    magnetizerBallGravity = 55000.0f;
    magnetizerRange = 1000.0f * rangeMultiplier;
    magnetizerDeactivateOnTouch = false;
    //      RumblePickupComponentWrapper
    magnetizerActivationDuration = 6.0f * durationMultiplier;

    // Plunger - RumblePickupComponentWrapper::TargetedPickup::SpringPickup::BallLassoPickup::BatarangPickup
    plungerSpinSpeed = 0.0f * forceMultiplier;
    //      SpringPickup
    plungerForce = 0.0f * forceMultiplier;
    plungerVerticalForce = 0.0f * forceMultiplier;
    plungerTorque = 0.0f * forceMultiplier;
    plungerRelativeForceNormalDirection = -1.0f * forceMultiplier;
    plungerMaxSpringLength = 3000.0f * rangeMultiplier;
    plungerConstantForce = -20.0f * forceMultiplier;
    plungerMinSpringLength = 30.0f * rangeMultiplier;
    plungerWeldedForceScalar = 1.0f * forceMultiplier;
    plungerWeldedVerticalForce = 0.0f * forceMultiplier;
    //      TargetedPickup
    plungerCanTargetBall = true;
    plungerCanTargetCars = false;
    plungerCanTargetEnemyCars = false;
    plungerCanTargetTeamCars = false;
    plungerRange = 1800.0f * rangeMultiplier;
    //      RumblePickupComponentWrapper
    plungerActivationDuration = 99.0f * durationMultiplier;

    // Powerhitter - RumblePickupComponentWrapper::HitForcePickup
    powerhitterBallForce = true;
    powerhitterCarForce = true;
    powerhitterDemolishCars = true;
    powerhitterBallHitForce = 1.5f * forceMultiplier;
    powerhitterCarHitForce = 30.0f * forceMultiplier;
    //      RumblePickupComponentWrapper
    powerhitterActivationDuration = 12.0f * durationMultiplier;

    // Spikes - RumblePickupComponentWrapper::VelcroPickup
    spikesAfterHitDuration = 4.5f * durationMultiplier;
    spikesPostBreakDuration = 0.6f * durationMultiplier;
    spikesMinBreakForce = 1200.0f * forceMultiplier;
    spikesMinBreakTime = 0.6f * durationMultiplier;
    spikesAttachTime = 0.0f * durationMultiplier;
    spikesBreakTime = 0.0f * durationMultiplier;
    //      RumblePickupComponentWrapper
    spikesActivationDuration = 12.0f * durationMultiplier;

    // Swapper - RumblePickupComponentWrapper::TargetedPickup::SwapperPickup
    //      TargetedPickup
    swapperCanTargetBall = false;
    swapperCanTargetCars = true;
    swapperCanTargetEnemyCars = true;
    swapperCanTargetTeamCars = false;
    swapperRange = 10000.0f * rangeMultiplier;
    //      RumblePickupComponentWrapper
    swapperActivationDuration = 0.75f * durationMultiplier;

    // Tornado - RumblePickupComponentWrapper::TornadoPickup
    tornadoHeight = 800.0f * rangeMultiplier;
    tornadoRadius = 400.0f * rangeMultiplier;
    tornadoRotationalForce = 7.0f * forceMultiplier;
    tornadoTorque = 5.0f * forceMultiplier;
    tornadoFxScale = Vector(1, 1, 1) * rangeMultiplier;
    tornadoMeshScale = Vector(1, 1, 1) * rangeMultiplier;
    tornadoMaxVelocityOffset = 3000.0f * forceMultiplier;
    tornadoBallMultiplier = 3.0f * forceMultiplier;
    tornadoVelocityEase = 5.0f * forceMultiplier;
    //      RumblePickupComponentWrapper
    tornadoActivationDuration = 8.0f * durationMultiplier;

    // Haunted Ball Beam - RumblePickupComponentWrapper::TargetedPickup
    //      TargetedPickup
    hauntedBallBeamRange = 1200.0f * rangeMultiplier;
    //      RumblePickupComponentWrapper
    hauntedBallBeamActivationDuration = 4.0f * durationMultiplier;

    // Rugby Spikes - RumblePickupComponentWrapper
    //      RumblePickupComponentWrapper
    rugbySpikesActivationDuration = 99.0f * durationMultiplier;
}


/// <summary>Calls <see cref="updateRumbleOptions()"/> on the main thread.</summary>
/// <param name="update">Bool to check if it should update</param>
void CrazyRumble::updateCars(const bool update)
{
    if (update) {
        rocketPlugin->Execute([this](GameWrapper*) {
            updateRumbleOptions();
        });
    }
}


/// <summary>Renders the available options for the game mode.</summary>
void CrazyRumble::RenderOptions()
{
    ImGui::Banner("Rumble settings seem to not work for now", ImColor(IM_COL32(211, 47, 47, 255)));
    ImGui::Text("Presets:");
    ImGui::Spacing();

    if (ImGui::Button("Bumper Cars")) {
        setItemValues();
        powerhitterDemolishCars = false;
        powerhitterActivationDuration = 300.0f;
        updateCars();
    }
    ImGui::SameLine();
    if (ImGui::Button("Target Everything")) {
        setItemValues();
        bootCanTargetBall = true;
        bootCanTargetCars = true;
        bootCanTargetEnemyCars = true;
        bootCanTargetTeamCars = true;
        disruptorCanTargetBall = true;
        disruptorCanTargetCars = true;
        disruptorCanTargetEnemyCars = true;
        disruptorCanTargetTeamCars = true;
        freezerCanTargetBall = true;
        freezerCanTargetCars = true;
        freezerCanTargetEnemyCars = true;
        freezerCanTargetTeamCars = true;
        grapplingHookCanTargetBall = true;
        grapplingHookCanTargetCars = true;
        grapplingHookCanTargetEnemyCars = true;
        grapplingHookCanTargetTeamCars = true;
        haymakerCanTargetBall = true;
        haymakerCanTargetCars = true;
        haymakerCanTargetEnemyCars = true;
        haymakerCanTargetTeamCars = true;
        plungerCanTargetBall = true;
        plungerCanTargetCars = true;
        plungerCanTargetEnemyCars = true;
        plungerCanTargetTeamCars = true;
        swapperCanTargetBall = true;
        swapperCanTargetCars = true;
        swapperCanTargetEnemyCars = true;
        swapperCanTargetTeamCars = true;
        updateCars();
    }
    ImGui::SameLine();
    if (ImGui::Button("Negative Forces")) {
        setItemValues(-1.0f, 1.0f, 1.0f);
        updateCars();
    }
    if (ImGui::Button("Extreme power")) {
        setItemValues(10.0f, 1.0f, 1.0f);
        updateCars();
    }
    ImGui::SameLine();
    if (ImGui::Button("Long Ranged")) {
        setItemValues(1.0f, 4.0f, 1.0f);
        updateCars();
    }
    ImGui::SameLine();
    if (ImGui::Button("Insane Duration")) {
        setItemValues(1.0f, 1.0f, 10.0f);
        updateCars();
    }
    ImGui::Spacing();

    if (ImGui::SliderFloat("the item force", &forceMultiplier, 0.0f, 10.0f, "%.3fX")) {
        setItemValues(forceMultiplier, rangeMultiplier, durationMultiplier);
        updateCars();
    }
    if (ImGui::SliderFloat("the item range", &rangeMultiplier, 0.0f, 10.0f, "%.3fX")) {
        setItemValues(forceMultiplier, rangeMultiplier, durationMultiplier);
        updateCars();
    }
    if (ImGui::SliderFloat("the item duration", &durationMultiplier, 0.0f, 10.0f, "%.3fX")) {
        setItemValues(forceMultiplier, rangeMultiplier, durationMultiplier);
        updateCars();
    }
    if (ImGui::SliderInt("till next item", &maxTimeTillItem, 0, 20, "%d seconds")) {
        rocketPlugin->Execute([this, newMaxTimeTillItem = maxTimeTillItem](GameWrapper*) {
            setItemGiveRate(newMaxTimeTillItem);
        });
    }
    if (ImGui::Button("Default everything 1X")) {
        setItemValues();
        updateCars();
    }
    ImGui::SameLine();
    if (ImGui::Button("Wild everything 2X")) {
        setItemValues(2.0f, 2.0f, 2.0f);
        updateCars();
    }
    ImGui::SameLine();
    if (ImGui::Button("Crazy everything 5X")) {
        setItemValues(5.0f, 5.0f, 5.0f);
        updateCars();
    }
    ImGui::SameLine();
    if (ImGui::Button("Insanity everything 10X")) {
        setItemValues(10.0f, 10.0f, 10.0f);
        updateCars();
    }
    ImGui::Separator();

    ImGui::Text("Per item customization:");
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Boot")) {
        updateCars(ImGui::SliderFloat("Boot Force", &bootForce, 0.0f, 1000000.0f));
        updateCars(ImGui::SliderFloat("Boot Vertical Force", &bootVerticalForce, 0.0f, 1000000.0f));
        updateCars(ImGui::DragVector("Boot Torque", &bootTorque));
        updateCars(ImGui::SliderFloat("Boot Relative Force Normal Direction", &bootRelativeForceNormalDirection, -10.0f,
                                      10.0f));
        updateCars(ImGui::SliderFloat("Boot Max Spring Length", &bootMaxSpringLength, 0.0f, 10000.0f));
        updateCars(ImGui::SliderFloat("Boot Constant Force", &bootConstantForce, -100.0f, 1000000.0f));
        updateCars(ImGui::SliderFloat("Boot Min Spring Length", &bootMinSpringLength, 0.0f, 10000.0f));
        updateCars(ImGui::SliderFloat("Boot Welded Force Scalar", &bootWeldedForceScalar, 0.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Boot Welded Vertical Force", &bootWeldedVerticalForce, 0.0f, 1000000.0f));
        updateCars(ImGui::Checkbox("Boot Can Target Ball", &bootCanTargetBall));
        updateCars(ImGui::Checkbox("Boot Can Target Cars", &bootCanTargetCars));
        updateCars(ImGui::Checkbox("Boot Can Target Enemy Cars", &bootCanTargetEnemyCars));
        updateCars(ImGui::Checkbox("Boot Can Target Team Cars", &bootCanTargetTeamCars));
        updateCars(ImGui::SliderFloat("Boot Range", &bootRange, 0.0f, 50000.0f));
        updateCars(ImGui::SliderFloat("Boot Activation Duration", &bootActivationDuration, 0.0f, 100.0f));
    }
    if (ImGui::CollapsingHeader("Disruptor")) {
        updateCars(ImGui::Checkbox("Disruptor Can Target Ball", &disruptorCanTargetBall));
        updateCars(ImGui::Checkbox("Disruptor Can Target Cars", &disruptorCanTargetCars));
        updateCars(ImGui::Checkbox("Disruptor Can Target Enemy Cars", &disruptorCanTargetEnemyCars));
        updateCars(ImGui::Checkbox("Disruptor Can Target Team Cars", &disruptorCanTargetTeamCars));
        updateCars(ImGui::SliderFloat("Disruptor Range", &disruptorRange, 0.0f, 50000.0f));
        updateCars(ImGui::SliderFloat("Disruptor Activation Duration", &disruptorActivationDuration, 0.0f, 100.0f));
    }
    if (ImGui::CollapsingHeader("Freezer")) {
        updateCars(ImGui::Checkbox("Freezer Maintain Momentum", &freezerMaintainMomentum));
        updateCars(ImGui::SliderFloat("Freezer Time To Stop", &freezerTimeToStop, 0.0f, 1.0f));
        updateCars(ImGui::SliderFloat("Freezer Stop Momentum Percentage", &freezerStopMomentumPercentage, 0.0f,
                                      100.0f));
        updateCars(ImGui::Checkbox("Freezer Can Target Ball", &freezerCanTargetBall));
        updateCars(ImGui::Checkbox("Freezer Can Target Cars", &freezerCanTargetCars));
        updateCars(ImGui::Checkbox("Freezer Can Target Enemy Cars", &freezerCanTargetEnemyCars));
        updateCars(ImGui::Checkbox("Freezer Can Target Team Cars", &freezerCanTargetTeamCars));
        updateCars(ImGui::SliderFloat("Freezer Range", &freezerRange, 0.0f, 50000.0f));
        updateCars(ImGui::SliderFloat("Freezer Activation Duration", &freezerActivationDuration, 0.0f, 100.0f));
    }
    if (ImGui::CollapsingHeader("Grappling Hook")) {
        updateCars(ImGui::SliderFloat("Grappling Hook Impulse", &grapplingHookImpulse, 0.0f, 1000000.0f));
        updateCars(ImGui::SliderFloat("Grappling Hook Force", &grapplingHookForce, 0.0f, 1000000.0f));
        updateCars(ImGui::SliderFloat("Grappling Hook Max Rope Length", &grapplingHookMaxRopeLength, 0.0f, 10000.0f));
        updateCars(ImGui::SliderFloat("Grappling Hook Prediction Speed", &grapplingHookPredictionSpeed, 0.0f,
                                      10000.0f));
        updateCars(ImGui::Checkbox("Grappling Hook Can Target Ball", &grapplingHookCanTargetBall));
        updateCars(ImGui::Checkbox("Grappling Hook Can Target Cars", &grapplingHookCanTargetCars));
        updateCars(ImGui::Checkbox("Grappling Hook Can Target Enemy Cars", &grapplingHookCanTargetEnemyCars));
        updateCars(ImGui::Checkbox("Grappling Hook Can Target Team Cars", &grapplingHookCanTargetTeamCars));
        updateCars(ImGui::SliderFloat("Grappling Hook Range", &grapplingHookRange, 0.0f, 50000.0f));
        updateCars(ImGui::SliderFloat("Grappling Hook Activation Duration", &grapplingHookActivationDuration, 0.0f,
                                      100.0f));
    }
    if (ImGui::CollapsingHeader("Haymaker")) {
        updateCars(ImGui::SliderFloat("Haymaker Force", &haymakerForce, 0.0f, 1000000.0f));
        updateCars(ImGui::SliderFloat("Haymaker Vertical Force", &haymakerVerticalForce, 0.0f, 1000000.0f));
        updateCars(ImGui::DragVector("Haymaker Torque", &haymakerTorque));
        updateCars(ImGui::SliderFloat("Haymaker Relative Force Normal Direction", &haymakerRelativeForceNormalDirection,
                                      -10.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Haymaker Max Spring Length", &haymakerMaxSpringLength, 0.0f, 10000.0f));
        updateCars(ImGui::SliderFloat("Haymaker Constant Force", &haymakerConstantForce, -100.0f, 1000000.0f));
        updateCars(ImGui::SliderFloat("Haymaker Min Spring Length", &haymakerMinSpringLength, 0.0f, 10000.0f));
        updateCars(ImGui::SliderFloat("Haymaker Welded Force Scalar", &haymakerWeldedForceScalar, 0.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Haymaker Welded Vertical Force", &haymakerWeldedVerticalForce, 0.0f,
                                      1000000.0f));
        updateCars(ImGui::Checkbox("Haymaker Can Target Ball", &haymakerCanTargetBall));
        updateCars(ImGui::Checkbox("Haymaker Can Target Cars", &haymakerCanTargetCars));
        updateCars(ImGui::Checkbox("Haymaker Can Target Enemy Cars", &haymakerCanTargetEnemyCars));
        updateCars(ImGui::Checkbox("Haymaker Can Target Team Cars", &haymakerCanTargetTeamCars));
        updateCars(ImGui::SliderFloat("Haymaker Range", &haymakerRange, 0.0f, 50000.0f));
        updateCars(ImGui::SliderFloat("Haymaker Activation Duration", &haymakerActivationDuration, 0.0f, 100.0f));
    }
    if (ImGui::CollapsingHeader("Magnetizer")) {
        updateCars(ImGui::SliderFloat("Magnetizer Ball Gravity", &magnetizerBallGravity, -100000.0, 100000.0f));
        updateCars(ImGui::SliderFloat("Magnetizer Range", &magnetizerRange, 0.0f, 50000.0f));
        updateCars(ImGui::Checkbox("Magnetizer Deactivate On Touch", &magnetizerDeactivateOnTouch));
        updateCars(ImGui::SliderFloat("Magnetizer Activation Duration", &magnetizerActivationDuration, 0.0f, 100.0f));
    }
    if (ImGui::CollapsingHeader("Plunger")) {
        updateCars(ImGui::SliderFloat("Plunger Spin Speed", &plungerSpinSpeed, 0.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Plunger Force", &plungerForce, 0.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Plunger Vertical Force", &plungerVerticalForce, 0.0f, 10.0f));
        updateCars(ImGui::DragVector("Plunger Torque", &plungerTorque));
        updateCars(ImGui::SliderFloat("Plunger Relative Force Normal Direction", &plungerRelativeForceNormalDirection,
                                      -10.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Plunger Max Spring Length", &plungerMaxSpringLength, 0.0f, 10000.0f));
        updateCars(ImGui::SliderFloat("Plunger Constant Force", &plungerConstantForce, -100.0f, 1000000.0f));
        updateCars(ImGui::SliderFloat("Plunger Min Spring Length", &plungerMinSpringLength, 0.0f, 10000.0f));
        updateCars(ImGui::SliderFloat("Plunger Welded Force Scalar", &plungerWeldedForceScalar, 0.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Plunger Welded Vertical Force", &plungerWeldedVerticalForce, 0.0f, 1000000.0f));
        updateCars(ImGui::Checkbox("Plunger Can Target Ball", &plungerCanTargetBall));
        updateCars(ImGui::Checkbox("Plunger Can Target Cars", &plungerCanTargetCars));
        updateCars(ImGui::Checkbox("Plunger Can Target EnemyCars", &plungerCanTargetEnemyCars));
        updateCars(ImGui::Checkbox("Plunger Can Target TeamCars", &plungerCanTargetTeamCars));
        updateCars(ImGui::SliderFloat("Plunger Range", &plungerRange, 0.0f, 50000.0f));
        updateCars(ImGui::SliderFloat("Plunger Activation Duration", &plungerActivationDuration, 0.0f, 100.0f));
    }
    if (ImGui::CollapsingHeader("Powerhitter")) {
        updateCars(ImGui::Checkbox("Powerhitter Ball Force", &powerhitterBallForce));
        updateCars(ImGui::Checkbox("Powerhitter Car Force", &powerhitterCarForce));
        updateCars(ImGui::Checkbox("Powerhitter Demolish Cars", &powerhitterDemolishCars));
        updateCars(ImGui::SliderFloat("Powerhitter Ball Hit Force", &powerhitterBallHitForce, 0.0f, 100.0f));
        updateCars(ImGui::SliderFloat("Powerhitter Car Hit Force", &powerhitterCarHitForce, 0.0f, 100.0f));
        updateCars(ImGui::SliderFloat("Powerhitter Activation Duration", &powerhitterActivationDuration, 0.0f, 100.0f));
    }
    if (ImGui::CollapsingHeader("Spikes")) {
        updateCars(ImGui::SliderFloat("Spikes After Hit Duration", &spikesAfterHitDuration, 0.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Spikes Post Break Duration", &spikesPostBreakDuration, 0.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Spikes Min Break Force", &spikesMinBreakForce, 0.0f, 1000000.0f));
        updateCars(ImGui::SliderFloat("Spikes Min Break Time", &spikesMinBreakTime, 0.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Spikes Attach Time", &spikesAttachTime, 0.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Spikes Break Time", &spikesBreakTime, 0.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Spikes Activation Duration", &spikesActivationDuration, 0.0f, 100.0f));
    }
    if (ImGui::CollapsingHeader("Swapper")) {
        updateCars(ImGui::Checkbox("Swapper Can Target Ball", &swapperCanTargetBall));
        updateCars(ImGui::Checkbox("Swapper Can Target Cars", &swapperCanTargetCars));
        updateCars(ImGui::Checkbox("Swapper Can Target Enemy Cars", &swapperCanTargetEnemyCars));
        updateCars(ImGui::Checkbox("Swapper Can Target Team Cars", &swapperCanTargetTeamCars));
        updateCars(ImGui::SliderFloat("Swapper Range", &swapperRange, 0.0f, 50000.0f));
        updateCars(ImGui::SliderFloat("Swapper Activation Duration", &swapperActivationDuration, 0.0f, 100.0f));
    }
    if (ImGui::CollapsingHeader("Tornado")) {
        updateCars(ImGui::SliderFloat("Tornado Height", &tornadoHeight, 0.0f, 1000.0f));
        updateCars(ImGui::SliderFloat("Tornado Radius", &tornadoRadius, 0.0f, 1000.0f));
        updateCars(ImGui::SliderFloat("Tornado Rotational Force", &tornadoRotationalForce, 0.0f, 100.0f));
        updateCars(ImGui::SliderFloat("Tornado Torque", &tornadoTorque, 0.0f, 10.0f));
        updateCars(ImGui::DragVector("Tornado FX Scale", &tornadoFxScale));
        updateCars(ImGui::DragVector("Tornado Mesh Scale", &tornadoMeshScale));
        updateCars(ImGui::SliderFloat("Tornado Max Velocity Offset", &tornadoMaxVelocityOffset, 0.0f, 10000.0f));
        updateCars(ImGui::SliderFloat("Tornado Ball Multiplier", &tornadoBallMultiplier, 0.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Tornado Velocity Ease", &tornadoVelocityEase, 0.0f, 10.0f));
        updateCars(ImGui::SliderFloat("Tornado Activation Duration", &tornadoActivationDuration, 0.0f, 100.0f));
    }
    if (ImGui::CollapsingHeader("Haunted")) {
        updateCars(ImGui::SliderFloat("Haunted Ball Beam Range", &hauntedBallBeamRange, 0.0f, 50000.0f));
        updateCars(ImGui::SliderFloat("Haunted Ball Beam Activation Duration", &hauntedBallBeamActivationDuration, 0.0f,
                                      100.0f));
    }
    if (ImGui::CollapsingHeader("Rugby")) {
        updateCars(ImGui::SliderFloat("Rugby Spikes Activation Duration", &rugbySpikesActivationDuration, 0.0f,
                                      100.0f));
    }
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool CrazyRumble::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void CrazyRumble::Activate(const bool active)
{
    if (active && !isActive) {
        HookEventWithCallerPost<ActorWrapper>("Function TAGame.PlayerItemDispenser_TA.GiveItem",
                                              std::bind(&CrazyRumble::onGiveItem, this, std::placeholders::_1));
        HookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame", std::bind(&CrazyRumble::updateCars, this, true));
    }
    else if (!active && isActive) {
        UnhookEventPost("Function TAGame.PlayerItemDispenser_TA.GiveItem");
        UnhookEvent("Function TAGame.GameEvent_Soccar_TA.InitGame");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string CrazyRumble::GetGameModeName()
{
    return "Crazy Rumble Items";
}


/// <summary>Sets the current rumble options for the given car.</summary>
/// <param name="car">car to set the rumble options for</param>
void CrazyRumble::setRumbleOptions(CarWrapper car) const
{
    if (car.IsNull()) {
        ERROR_LOG("could not get the car");
        return;
    }

    RumblePickupComponentWrapper rumbleItem = car.GetAttachedPickup();
    if (!rumbleItem.IsNull()) {
        std::string rumbleItemName = rumbleItem.GetPickupName().ToString();

        if (rumbleItemName == "BallMagnet") {
            GravityPickup magnetizer = GravityPickup(rumbleItem.memory_address);
            magnetizer.SetBallGravity(magnetizerBallGravity);
            magnetizer.SetRange(magnetizerRange);
            magnetizer.SetbDeactivateOnTouch(magnetizerDeactivateOnTouch);
            magnetizer.SetActivationDuration(magnetizerActivationDuration);
        }
        else if (rumbleItemName == "CarSpring") {
            BallCarSpringPickup boot = BallCarSpringPickup(rumbleItem.memory_address);
            boot.SetForce(bootForce);
            boot.SetVerticalForce(bootVerticalForce);
            boot.SetTorque(bootTorque);
            boot.SetRelativeForceNormalDirection(bootRelativeForceNormalDirection);
            boot.SetMaxSpringLength(bootMaxSpringLength);
            boot.SetConstantForce(bootConstantForce);
            boot.SetMinSpringLength(bootMinSpringLength);
            boot.SetWeldedForceScalar(bootWeldedForceScalar);
            boot.SetWeldedVerticalForce(bootWeldedVerticalForce);
            boot.SetbCanTargetBall(bootCanTargetBall);
            boot.SetbCanTargetCars(bootCanTargetCars);
            boot.SetbCanTargetEnemyCars(bootCanTargetEnemyCars);
            boot.SetbCanTargetTeamCars(bootCanTargetTeamCars);
            boot.SetRange(bootRange);
            boot.SetActivationDuration(bootActivationDuration);
        }
        else if (rumbleItemName == "BallSpring") {
            BallCarSpringPickup haymaker = BallCarSpringPickup(rumbleItem.memory_address);
            haymaker.SetForce(haymakerForce);
            haymaker.SetVerticalForce(haymakerVerticalForce);
            haymaker.SetTorque(haymakerTorque);
            haymaker.SetRelativeForceNormalDirection(haymakerRelativeForceNormalDirection);
            haymaker.SetMaxSpringLength(haymakerMaxSpringLength);
            haymaker.SetConstantForce(haymakerConstantForce);
            haymaker.SetMinSpringLength(haymakerMinSpringLength);
            haymaker.SetWeldedForceScalar(haymakerWeldedForceScalar);
            haymaker.SetWeldedVerticalForce(haymakerWeldedVerticalForce);
            haymaker.SetbCanTargetBall(haymakerCanTargetBall);
            haymaker.SetbCanTargetCars(haymakerCanTargetCars);
            haymaker.SetbCanTargetEnemyCars(haymakerCanTargetEnemyCars);
            haymaker.SetbCanTargetTeamCars(haymakerCanTargetTeamCars);
            haymaker.SetRange(haymakerRange);
            haymaker.SetActivationDuration(haymakerActivationDuration);
        }
        else if (rumbleItemName == "Tornado") {
            TornadoPickup tornado = TornadoPickup(rumbleItem.memory_address);
            tornado.SetHeight(tornadoHeight);
            tornado.SetRadius(tornadoRadius);
            tornado.SetRotationalForce(tornadoRotationalForce);
            tornado.SetTorque(tornadoTorque);
            tornado.SetFXScale(tornadoFxScale);
            tornado.SetMeshScale(tornadoMeshScale);
            tornado.SetMaxVelocityOffset(tornadoMaxVelocityOffset);
            tornado.SetBallMultiplier(tornadoBallMultiplier);
            tornado.SetVelocityEase(tornadoVelocityEase);
            tornado.SetActivationDuration(tornadoActivationDuration);
        }
        else if (rumbleItemName == "GrapplingHook") {
            GrapplingHookPickup grapplingHook = GrapplingHookPickup(rumbleItem.memory_address);
            grapplingHook.SetImpulse(grapplingHookImpulse);
            grapplingHook.SetForce(grapplingHookForce);
            grapplingHook.SetMaxRopeLength(grapplingHookMaxRopeLength);
            grapplingHook.SetPredictionSpeed(grapplingHookPredictionSpeed);
            grapplingHook.SetbCanTargetBall(grapplingHookCanTargetBall);
            grapplingHook.SetbCanTargetCars(grapplingHookCanTargetCars);
            grapplingHook.SetbCanTargetEnemyCars(grapplingHookCanTargetEnemyCars);
            grapplingHook.SetbCanTargetTeamCars(grapplingHookCanTargetTeamCars);
            grapplingHook.SetRange(grapplingHookRange);
            grapplingHook.SetActivationDuration(grapplingHookActivationDuration);
        }
        else if (rumbleItemName == "Powerhitter") {
            HitForcePickup powerhitter = HitForcePickup(rumbleItem.memory_address);
            powerhitter.SetbBallForce(powerhitterBallForce);
            powerhitter.SetbCarForce(powerhitterCarForce);
            powerhitter.SetbDemolishCars(powerhitterDemolishCars);
            powerhitter.SetBallHitForce(powerhitterBallHitForce);
            powerhitter.SetCarHitForce(powerhitterCarHitForce);
            powerhitter.SetActivationDuration(powerhitterActivationDuration);
        }
        else if (rumbleItemName == "EnemyBooster") {
            BoostOverridePickup disruptor = BoostOverridePickup(rumbleItem.memory_address);
            disruptor.SetbCanTargetBall(disruptorCanTargetBall);
            disruptor.SetbCanTargetCars(disruptorCanTargetCars);
            disruptor.SetbCanTargetEnemyCars(disruptorCanTargetEnemyCars);
            disruptor.SetbCanTargetTeamCars(disruptorCanTargetTeamCars);
            disruptor.SetRange(disruptorRange);
            disruptor.SetActivationDuration(disruptorActivationDuration);
        }
        else if (rumbleItemName == "BallLasso") {
            BattarangPickup plunger = BattarangPickup(rumbleItem.memory_address);
            plunger.SetSpinSpeed(plungerSpinSpeed);
            plunger.SetForce(plungerForce);
            plunger.SetVerticalForce(plungerVerticalForce);
            plunger.SetTorque(plungerTorque);
            plunger.SetRelativeForceNormalDirection(plungerRelativeForceNormalDirection);
            plunger.SetMaxSpringLength(plungerMaxSpringLength);
            plunger.SetConstantForce(plungerConstantForce);
            plunger.SetMinSpringLength(plungerMinSpringLength);
            plunger.SetWeldedForceScalar(plungerWeldedForceScalar);
            plunger.SetWeldedVerticalForce(plungerWeldedVerticalForce);
            plunger.SetbCanTargetBall(plungerCanTargetBall);
            plunger.SetbCanTargetCars(plungerCanTargetCars);
            plunger.SetbCanTargetEnemyCars(plungerCanTargetEnemyCars);
            plunger.SetbCanTargetTeamCars(plungerCanTargetTeamCars);
            plunger.SetRange(plungerRange);
            plunger.SetActivationDuration(plungerActivationDuration);
        }
        else if (rumbleItemName == "BallVelcro") {
            VelcroPickup spikes = VelcroPickup(rumbleItem.memory_address);
            spikes.SetAfterHitDuration(spikesAfterHitDuration);
            spikes.SetPostBreakDuration(spikesPostBreakDuration);
            spikes.SetMinBreakForce(spikesMinBreakForce);
            spikes.SetMinBreakTime(spikesMinBreakTime);
            spikes.SetAttachTime(spikesAttachTime);
            spikes.SetBreakTime(spikesBreakTime);
            spikes.SetActivationDuration(spikesActivationDuration);
        }
        else if (rumbleItemName == "BallFreeze") {
            BallFreezePickup freezer = BallFreezePickup(rumbleItem.memory_address);
            freezer.SetbMaintainMomentum(freezerMaintainMomentum);
            freezer.SetTimeToStop(freezerTimeToStop);
            freezer.SetStopMomentumPercentage(freezerStopMomentumPercentage);
            freezer.SetbCanTargetBall(freezerCanTargetBall);
            freezer.SetbCanTargetCars(freezerCanTargetCars);
            freezer.SetbCanTargetEnemyCars(freezerCanTargetEnemyCars);
            freezer.SetbCanTargetTeamCars(freezerCanTargetTeamCars);
            freezer.SetRange(freezerRange);
            freezer.SetActivationDuration(freezerActivationDuration);
        }
        else if (rumbleItemName == "EnemySwapper") {
            SwapperPickup swapper = SwapperPickup(rumbleItem.memory_address);
            swapper.SetbCanTargetBall(swapperCanTargetBall);
            swapper.SetbCanTargetCars(swapperCanTargetCars);
            swapper.SetbCanTargetEnemyCars(swapperCanTargetEnemyCars);
            swapper.SetbCanTargetTeamCars(swapperCanTargetTeamCars);
            swapper.SetRange(swapperRange);
            swapper.SetActivationDuration(swapperActivationDuration);
        }
        else if (rumbleItemName == "HauntedBallBeam") {
            TargetedPickup hauntedBallBeam = TargetedPickup(rumbleItem.memory_address);
            hauntedBallBeam.SetRange(hauntedBallBeamRange);
            hauntedBallBeam.SetActivationDuration(hauntedBallBeamActivationDuration);
        }
        else if (rumbleItemName == "RugbySpikes") {
            TargetedPickup rugbySpikes = TargetedPickup(rumbleItem.memory_address);
            rugbySpikes.SetActivationDuration(rugbySpikesActivationDuration);
        }
        else {
            LOG("Unknown item: " + quote(rumbleItemName) + ", please report to mod author.");
        }

        car.ForceNetUpdate2();
        car.ForceNetUpdatePacket();
        rumbleItem.ForceNetUpdate2();
        rumbleItem.ForceNetUpdatePacket();
    }
}
