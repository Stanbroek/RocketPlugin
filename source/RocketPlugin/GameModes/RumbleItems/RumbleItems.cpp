// RumbleItems.cpp
// Rumble items wrappers for the RocketPlugin plugin.
//
// Author:       Stanbroek
// Version:      0.6.6 15/08/21

#include "RumbleItems.h"


bool RumbleWrapper::Render()
{
    bool update = false;
    update |= ImGui::SliderFloat("Activation Duration", &ActivationDuration, 0.f, 100.f);

    return update;
}


void RumbleWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = RumbleWrapper(Archetype);
    Enabled = wasEnabled;
}


void RumbleWrapper::Update(const std::uintptr_t item) const
{
    RumblePickupComponentWrapper rumbleItem = item;
    rumbleItem.SetActivationDuration(ActivationDuration);
}


void RumbleWrapper::Multiply(const float, const float, const float durationMultiplier)
{
    ActivationDuration = Archetype->ActivationDuration * durationMultiplier;
}


bool TargetedWrapper::Render()
{
    bool update = Super::Render();
    update |= ImGui::Checkbox("Can Target Ball", &CanTargetBall);
    update |= ImGui::Checkbox("Can Target Cars", &CanTargetCars);
    update |= ImGui::Checkbox("Can Target Enemy Cars", &CanTargetEnemyCars);
    update |= ImGui::Checkbox("Can Target Team Cars", &CanTargetTeamCars);
    update |= ImGui::SliderFloat("Range", &Range, 0.f, 50000.0f);

    return update;
}


void TargetedWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = TargetedWrapper(dynamic_cast<const TargetedWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void TargetedWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
    TargetedPickup rumbleItem = item;
    rumbleItem.SetbCanTargetBall(CanTargetBall);
    rumbleItem.SetbCanTargetCars(CanTargetCars);
    rumbleItem.SetbCanTargetEnemyCars(CanTargetEnemyCars);
    rumbleItem.SetbCanTargetTeamCars(CanTargetTeamCars);
    rumbleItem.SetRange(Range);
}


void TargetedWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
    const TargetedWrapper* arch = dynamic_cast<const TargetedWrapper*>(Archetype);
    Range = arch->Range * rangeMultiplier;
}


bool SpringWrapper::Render()
{
    bool update = Super::Render();
    update |= ImGui::SliderFloat("Force", &Force, 0.f, 1000000.f);
    update |= ImGui::SliderFloat("Vertical Force", &VerticalForce, 0.f, 1000000.f);
    update |= ImGui::DragVector("Torque", &Torque);
    update |= ImGui::SliderFloat("Relative Force Normal Direction", &RelativeForceNormalDirection, -10.f, 10.f);
    update |= ImGui::SliderFloat("Max Spring Length", &MaxSpringLength, 0.f, 10000.f);
    update |= ImGui::SliderFloat("Constant Force", &ConstantForce, -100.f, 1000000.f);
    update |= ImGui::SliderFloat("Min Spring Length", &MinSpringLength, 0.f, 10000.f);
    update |= ImGui::SliderFloat("Welded Force Scalar", &WeldedForceScalar, 0.f, 10.f);
    update |= ImGui::SliderFloat("Welded Vertical Force", &WeldedVerticalForce, 0.f, 1000000.f);

    return update;
}


void SpringWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = SpringWrapper(dynamic_cast<const SpringWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void SpringWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
    SpringPickup rumbleItem = item;
    rumbleItem.SetForce(Force);
    rumbleItem.SetVerticalForce(VerticalForce);
    rumbleItem.SetTorque(Torque);
    rumbleItem.SetRelativeForceNormalDirection(RelativeForceNormalDirection);
    rumbleItem.SetMaxSpringLength(MaxSpringLength);
    rumbleItem.SetConstantForce(ConstantForce);
    rumbleItem.SetMinSpringLength(MinSpringLength);
    rumbleItem.SetWeldedForceScalar(WeldedForceScalar);
    rumbleItem.SetWeldedVerticalForce(WeldedVerticalForce);
}


void SpringWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
    const SpringWrapper* arch = dynamic_cast<const SpringWrapper*>(Archetype);
    Force = arch->Force * forceMultiplier;
    VerticalForce = arch->VerticalForce * forceMultiplier;
    Torque = arch->Torque * forceMultiplier;
    RelativeForceNormalDirection = arch->RelativeForceNormalDirection * forceMultiplier;
    MaxSpringLength = arch->MaxSpringLength * rangeMultiplier;
    ConstantForce = arch->ConstantForce * forceMultiplier;
    MinSpringLength = arch->MinSpringLength * rangeMultiplier;
    WeldedForceScalar = arch->WeldedForceScalar * forceMultiplier;
    WeldedVerticalForce = arch->WeldedVerticalForce * forceMultiplier;
}


bool BallCarSpringWrapper::Render()
{
    return Super::Render();
}


void BallCarSpringWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = BallCarSpringWrapper(dynamic_cast<const BallCarSpringWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void BallCarSpringWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
}


void BallCarSpringWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
}


bool BoostOverrideWrapper::Render()
{
    return Super::Render();
}


void BoostOverrideWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = BoostOverrideWrapper(dynamic_cast<const BoostOverrideWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void BoostOverrideWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
}


void BoostOverrideWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
}


bool BallFreezeWrapper::Render()
{
    bool update = Super::Render();
    update |= ImGui::Checkbox("Maintain Momentum", &MaintainMomentum);
    update |= ImGui::SliderFloat("Time To Stop", &TimeToStop, 0.f, 1.f);
    update |= ImGui::SliderFloat("Stop Momentum Percentage", &StopMomentumPercentage, 0.f, 100.f);

    return update;
}


void BallFreezeWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = BallFreezeWrapper(dynamic_cast<const BallFreezeWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void BallFreezeWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
    BallFreezePickup rumbleItem = item;
    rumbleItem.SetbMaintainMomentum(MaintainMomentum);
    rumbleItem.SetTimeToStop(TimeToStop);
    rumbleItem.SetStopMomentumPercentage(StopMomentumPercentage);
}


void BallFreezeWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
    const BallFreezeWrapper* arch = dynamic_cast<const BallFreezeWrapper*>(Archetype);
    TimeToStop = arch->TimeToStop * durationMultiplier;
    StopMomentumPercentage = arch->StopMomentumPercentage * durationMultiplier;
}


bool GrapplingHookWrapper::Render()
{
    bool update = Super::Render();
    update |= ImGui::SliderFloat("Impulse", &Impulse, 0.f, 1000000.f);
    update |= ImGui::SliderFloat("Force", &Force, 0.f, 1000000.f);
    update |= ImGui::SliderFloat("Max Rope Length", &MaxRopeLength, 0.f, 10000.f);
    update |= ImGui::SliderFloat("Prediction Speed", &PredictionSpeed, 0.f, 10000.f);

    return update;
}


void GrapplingHookWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = GrapplingHookWrapper(dynamic_cast<const GrapplingHookWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void GrapplingHookWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
    GrapplingHookPickup rumbleItem = item;
    rumbleItem.SetImpulse(Impulse);
    rumbleItem.SetForce(Force);
    rumbleItem.SetMaxRopeLength(MaxRopeLength);
    rumbleItem.SetPredictionSpeed(PredictionSpeed);
}


void GrapplingHookWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
    const GrapplingHookWrapper* arch = dynamic_cast<const GrapplingHookWrapper*>(Archetype);
    Impulse = arch->Impulse * forceMultiplier;
    Force = arch->Force * forceMultiplier;
    MaxRopeLength = arch->MaxRopeLength * rangeMultiplier;
    PredictionSpeed = arch->PredictionSpeed * durationMultiplier;
}


bool GravityWrapper::Render()
{
    bool update = Super::Render();
    update |= ImGui::SliderFloat("Ball Gravity", &BallGravity, -100000.f, 100000.f);
    update |= ImGui::SliderFloat("Range", &Range, 0.f, 50000.f);
    update |= ImGui::Checkbox("Deactivate On Touch", &DeactivateOnTouch);

    return update;
}


void GravityWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = GravityWrapper(dynamic_cast<const GravityWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void GravityWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
    GravityPickup rumbleItem = item;
    rumbleItem.SetBallGravity(BallGravity);
    rumbleItem.SetRange(Range);
    rumbleItem.SetbDeactivateOnTouch(DeactivateOnTouch);
}


void GravityWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
    const GravityWrapper* arch = dynamic_cast<const GravityWrapper*>(Archetype);
    //BallGravity = arch->BallGravity * forceMultiplier;
    Range = arch->Range * rangeMultiplier;
}


bool BallLassoWrapper::Render()
{
    return Super::Render();
}


void BallLassoWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = BallLassoWrapper(dynamic_cast<const BallLassoWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void BallLassoWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
}


void BallLassoWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
}


bool BattarangWrapper::Render()
{
    bool update = Super::Render();
    update |= ImGui::SliderFloat("Spin Speed", &SpinSpeed, 0.f, 10.f);

    return update;
}


void BattarangWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = BattarangWrapper(dynamic_cast<const BattarangWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void BattarangWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
    BattarangPickup rumbleItem = item;
    rumbleItem.SetSpinSpeed(SpinSpeed);
}


void BattarangWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
    const BattarangWrapper* arch = dynamic_cast<const BattarangWrapper*>(Archetype);
    SpinSpeed = arch->SpinSpeed * forceMultiplier;
}


bool HitForceWrapper::Render()
{
    bool update = Super::Render();
    update |= ImGui::Checkbox("Ball Force", &BallForce);
    update |= ImGui::Checkbox("Car Force", &CarForce);
    update |= ImGui::Checkbox("Demolish Cars", &DemolishCars);
    update |= ImGui::SliderFloat("Ball Hit Force", &BallHitForce, 0.f, 100.f);
    update |= ImGui::SliderFloat("Car Hit Force", &CarHitForce, 0.f, 100.f);

    return update;
}


void HitForceWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = HitForceWrapper(dynamic_cast<const HitForceWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void HitForceWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
    HitForcePickup rumbleItem = item;
    rumbleItem.SetbBallForce(BallForce);
    rumbleItem.SetbCarForce(CarForce);
    rumbleItem.SetbDemolishCars(DemolishCars);
    rumbleItem.SetBallHitForce(BallHitForce);
    rumbleItem.SetCarHitForce(CarHitForce);
}


void HitForceWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
    const HitForceWrapper* arch = dynamic_cast<const HitForceWrapper*>(Archetype);
    BallHitForce = arch->BallHitForce * forceMultiplier;
    CarHitForce = arch->CarHitForce * forceMultiplier;
}


bool VelcroWrapper::Render()
{
    bool update = Super::Render();
    update |= ImGui::SliderFloat("After Hit Duration", &AfterHitDuration, 0.f, 10.f);
    update |= ImGui::SliderFloat("Post Break Duration", &PostBreakDuration, 0.f, 10.f);
    update |= ImGui::SliderFloat("Min Break Force", &MinBreakForce, 0.f, 1000000.f);
    update |= ImGui::SliderFloat("Min Break Time", &MinBreakTime, 0.f, 10.f);
    update |= ImGui::SliderFloat("Attach Time", &AttachTime, 0.f, 10.f);
    update |= ImGui::SliderFloat("Break Time", &BreakTime, 0.f, 10.f);

    return update;
}


void VelcroWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = VelcroWrapper(dynamic_cast<const VelcroWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void VelcroWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
    VelcroPickup rumbleItem = item;
    rumbleItem.SetAfterHitDuration(AfterHitDuration);
    rumbleItem.SetPostBreakDuration(PostBreakDuration);
    rumbleItem.SetMinBreakForce(MinBreakForce);
    rumbleItem.SetMinBreakTime(MinBreakTime);
    rumbleItem.SetAttachTime(AttachTime);
    rumbleItem.SetBreakTime(BreakTime);
}


void VelcroWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
    const VelcroWrapper* arch = dynamic_cast<const VelcroWrapper*>(Archetype);
    AfterHitDuration = arch->AfterHitDuration * durationMultiplier;
    PostBreakDuration = arch->PostBreakDuration * durationMultiplier;
    MinBreakForce = arch->MinBreakForce * forceMultiplier;
    MinBreakTime = arch->MinBreakTime * durationMultiplier;
    AttachTime = arch->AttachTime * durationMultiplier;
    BreakTime = arch->BreakTime * durationMultiplier;
}


bool SwapperWrapper::Render()
{
    return Super::Render();
}


void SwapperWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = SwapperWrapper(dynamic_cast<const SwapperWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void SwapperWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
}


void SwapperWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
}


bool TornadoWrapper::Render()
{
    bool update = Super::Render();
    update |= ImGui::SliderFloat("Height", &Height, 0.f, 1000.f);
    update |= ImGui::SliderFloat("Radius", &Radius, 0.f, 1000.f);
    update |= ImGui::SliderFloat("Rotational Force", &RotationalForce, 0.f, 100.f);
    update |= ImGui::SliderFloat("Torque", &Torque, 0.f, 10.f);
    update |= ImGui::DragVector("FX Scale", &FxScale);
    update |= ImGui::DragVector("Mesh Scale", &FxScale);
    update |= ImGui::SliderFloat("Max Velocity Offset", &MaxVelocityOffset, 0.f, 10000.f);
    update |= ImGui::SliderFloat("Ball Multiplier", &BallMultiplier, 0.f, 10.f);
    update |= ImGui::SliderFloat("Velocity Ease", &VelocityEase, 0.f, 10.f);

    return update;
}


void TornadoWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = TornadoWrapper(dynamic_cast<const TornadoWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void TornadoWrapper::Update(const std::uintptr_t item) const
{
    Super::Update(item);
    TornadoPickup rumbleItem = item;
    rumbleItem.SetHeight(Height);
    rumbleItem.SetRadius(Radius);
    rumbleItem.SetRotationalForce(RotationalForce);
    rumbleItem.SetTorque(Torque);
    rumbleItem.SetFXScale(FxScale);
    rumbleItem.SetMeshScale(MeshScale);
    rumbleItem.SetMaxVelocityOffset(MaxVelocityOffset);
    rumbleItem.SetBallMultiplier(BallMultiplier);
    rumbleItem.SetVelocityEase(VelocityEase);
}


void TornadoWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
    const TornadoWrapper* arch = dynamic_cast<const TornadoWrapper*>(Archetype);
    Height = arch->Height * rangeMultiplier;
    Radius = arch->Radius * rangeMultiplier;
    RotationalForce = arch->RotationalForce * forceMultiplier;
    Torque = arch->Torque * forceMultiplier;
    FxScale = arch->FxScale * rangeMultiplier;
    MeshScale = arch->MeshScale * rangeMultiplier;
    MaxVelocityOffset = arch->MaxVelocityOffset * forceMultiplier;
    BallMultiplier = arch->BallMultiplier * forceMultiplier;
    VelocityEase = arch->VelocityEase * forceMultiplier;
}


bool HauntedWrapper::Render()
{
    return Super::Render();
}


void HauntedWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = HauntedWrapper(dynamic_cast<const HauntedWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void HauntedWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
}


bool RugbyWrapper::Render()
{
    return Super::Render();
}


void RugbyWrapper::Reset()
{
    const bool wasEnabled = Enabled;
    *this = RugbyWrapper(dynamic_cast<const RugbyWrapper*>(Archetype));
    Enabled = wasEnabled;
}


void RugbyWrapper::Multiply(const float forceMultiplier, const float rangeMultiplier,
    const float durationMultiplier)
{
    Super::Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
}
