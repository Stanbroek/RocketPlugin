#pragma once
#include "RocketPlugin.h"
#include "GameModes/RocketGameMode.h"


class SmallCars final : public RocketGameMode
{
public:
    explicit SmallCars(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }
    
    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;

private:
    float oldScale = 1.f;

    // CarWrapper
    bool bMaxLinearSpeed = false;
    bool bMaxAngularSpeed = false;

    // FlipCarComponentWrapper
    bool bFlipCarImpulse = true;
    bool bFlipCarTorque = true;
    bool bFlipCarTime = false;

    // VehicleSimWrapper
    bool bDriveTorque = true;
    bool bBrakeTorque = true;
    bool bStopThreshold = false;
    bool bIdleBrakeFactor = true;
    bool bOppositeBrakeFactor = true;
    bool bOutputThrottle = false;
    bool bOutputSteer = false;
    bool bOutputBrake = false;
    bool bOutputHandbrake = false;
    bool bSteeringSensitivity = true;

    // BoostWrapper
    bool bBoostConsumptionRate = false;
    bool bMaxBoostAmount = false;
    bool bStartBoostAmount = false;
    bool bCurrentBoostAmount = false;
    bool bBoostModifier = false;
    bool bLastBoostAmountRequestTime = false;
    bool bLastBoostAmount = false;
    bool bBoostForce = true;
    bool bMinBoostTime = false;
    bool bRechargeRate = false;
    bool bRechargeDelay = false;

    // DodgeComponentWrapper
    bool bDodgeInputThreshold = false;
    bool bSideDodgeImpulse = true;
    bool bSideDodgeImpulseMaxSpeedScale = true;
    bool bForwardDodgeImpulse = true;
    bool bForwardDodgeImpulseMaxSpeedScale = true;
    bool bBackwardDodgeImpulse = true;
    bool bBackwardDodgeImpulseMaxSpeedScale = true;
    bool bSideDodgeTorque = true;
    bool bForwardDodgeTorque = true;
    bool bDodgeTorqueTime = false;
    bool bMinDodgeTorqueTime = false;
    bool bDodgeZDamping = true;
    bool bDodgeZDampingDelay = false;
    bool bDodgeZDampingUpTime = false;
    bool bDodgeImpulseScale = true;
    bool bDodgeTorqueScale = false;

    // AirControlComponentWrapper
    bool bThrottleForce = true;
    bool bDodgeDisableTimeRemaining = false;
    bool bControlScale = false;
    bool bAirControlSensitivity = false;

    // JumpComponentWrapper
    bool bMinJumpTime = false;
    bool bJumpImpulse = true;
    bool bJumpForce = true;
    bool bJumpForceTime = false;
    bool bPodiumJumpForceTime = false;
    bool bJumpImpulseSpeed = true;
    bool bJumpAccel = true;
    bool bMaxJumpHeight = true;
    bool bMaxJumpHeightTime = false;

    // DoubleJumpComponentWrapper
    bool bImpulseScale = true;
};
