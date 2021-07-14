// RumbleItems.cpp
// Rumble items wrappers for the RocketPlugin plugin.
//
// Author:       Stanbroek
// Version:      0.6.5 05/02/21

#include "RumbleItems.h"


bool RumbleWrapper::Render()
{
    bool update = false;
    update |= ImGui::SliderFloat("Activation Duration", &ActivationDuration, 0.f, 100.f);

    return update;
}


void RumbleWrapper::Reset(const RumbleWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void RumbleWrapper::Update(RumblePickupComponentWrapper item) const
{
    item.SetActivationDuration(ActivationDuration);
}


void RumbleWrapper::Multiply(const RumbleWrapper def, const float, const float, const float durationMultiplier)
{
    ActivationDuration = def.ActivationDuration * durationMultiplier;
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


void TargetedWrapper::Reset(const TargetedWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void TargetedWrapper::Update(TargetedPickup item) const
{
    Super::Update(static_cast<RumblePickupComponentWrapper>(item));
    item.SetbCanTargetBall(CanTargetBall);
    item.SetbCanTargetCars(CanTargetCars);
    item.SetbCanTargetEnemyCars(CanTargetEnemyCars);
    item.SetbCanTargetTeamCars(CanTargetTeamCars);
    item.SetRange(Range);
}


void TargetedWrapper::Multiply(const TargetedWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<RumbleWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
    Range = def.Range * rangeMultiplier;
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


void SpringWrapper::Reset(const SpringWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void SpringWrapper::Update(SpringPickup item) const
{
    Super::Update(static_cast<TargetedPickup>(item));
    item.SetForce(Force);
    item.SetVerticalForce(VerticalForce);
    item.SetTorque(Torque);
    item.SetRelativeForceNormalDirection(RelativeForceNormalDirection);
    item.SetMaxSpringLength(MaxSpringLength);
    item.SetConstantForce(ConstantForce);
    item.SetMinSpringLength(MinSpringLength);
    item.SetWeldedForceScalar(WeldedForceScalar);
    item.SetWeldedVerticalForce(WeldedVerticalForce);
}


void SpringWrapper::Multiply(const SpringWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<TargetedWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
    Force = def.Force * forceMultiplier;
    VerticalForce = def.VerticalForce * forceMultiplier;
    Torque = def.Torque * forceMultiplier;
    RelativeForceNormalDirection = def.RelativeForceNormalDirection * forceMultiplier;
    MaxSpringLength = def.MaxSpringLength * rangeMultiplier;
    ConstantForce = def.ConstantForce * forceMultiplier;
    MinSpringLength = def.MinSpringLength * rangeMultiplier;
    WeldedForceScalar = def.WeldedForceScalar * forceMultiplier;
    WeldedVerticalForce = def.WeldedVerticalForce * forceMultiplier;
}


bool BallCarSpringWrapper::Render()
{
    return Super::Render();
}


void BallCarSpringWrapper::Reset(const BallCarSpringWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void BallCarSpringWrapper::Update(BallCarSpringPickup item) const
{
    Super::Update(static_cast<SpringPickup>(item));
}


void BallCarSpringWrapper::Multiply(const BallCarSpringWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<SpringWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
}


bool BoostOverrideWrapper::Render()
{
    return Super::Render();
}


void BoostOverrideWrapper::Reset(const BoostOverrideWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void BoostOverrideWrapper::Update(BoostOverridePickup item) const
{
    Super::Update(static_cast<TargetedPickup>(item));
}


void BoostOverrideWrapper::Multiply(const BoostOverrideWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<TargetedWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
}


bool BallFreezeWrapper::Render()
{
    bool update = Super::Render();
    update |= ImGui::Checkbox("Maintain Momentum", &MaintainMomentum);
    update |= ImGui::SliderFloat("Time To Stop", &TimeToStop, 0.f, 1.f);
    update |= ImGui::SliderFloat("Stop Momentum Percentage", &StopMomentumPercentage, 0.f, 100.f);

    return update;
}


void BallFreezeWrapper::Reset(const BallFreezeWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void BallFreezeWrapper::Update(BallFreezePickup item) const
{
    Super::Update(static_cast<TargetedPickup>(item));
    item.SetbMaintainMomentum(MaintainMomentum);
    item.SetTimeToStop(TimeToStop);
    item.SetStopMomentumPercentage(StopMomentumPercentage);
}


void BallFreezeWrapper::Multiply(const BallFreezeWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<TargetedWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
    TimeToStop = def.TimeToStop * durationMultiplier;
    StopMomentumPercentage = def.StopMomentumPercentage * durationMultiplier;
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


void GrapplingHookWrapper::Reset(const GrapplingHookWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void GrapplingHookWrapper::Update(GrapplingHookPickup item) const
{
    Super::Update(static_cast<TargetedPickup>(item));
    item.SetImpulse(Impulse);
    item.SetForce(Force);
    item.SetMaxRopeLength(MaxRopeLength);
    item.SetPredictionSpeed(PredictionSpeed);
}


void GrapplingHookWrapper::Multiply(const GrapplingHookWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<TargetedWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
    Impulse = def.Impulse * forceMultiplier;
    Force = def.Force * forceMultiplier;
    MaxRopeLength = def.MaxRopeLength * rangeMultiplier;
    PredictionSpeed = def.PredictionSpeed * durationMultiplier;
}


bool GravityWrapper::Render()
{
    bool update = Super::Render();
    update |= ImGui::SliderFloat("Ball Gravity", &BallGravity, -100000.f, 100000.f);
    update |= ImGui::SliderFloat("Range", &Range, 0.f, 50000.f);
    update |= ImGui::Checkbox("Deactivate On Touch", &DeactivateOnTouch);

    return update;
}


void GravityWrapper::Reset(const GravityWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void GravityWrapper::Update(GravityPickup item) const
{
    Super::Update(static_cast<RumblePickupComponentWrapper>(item));
    item.SetBallGravity(BallGravity);
    item.SetRange(Range);
    item.SetbDeactivateOnTouch(DeactivateOnTouch);
}


void GravityWrapper::Multiply(const GravityWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<RumbleWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
    //BallGravity = def.BallGravity * forceMultiplier;
    Range = def.Range * rangeMultiplier;
}


bool BallLassoWrapper::Render()
{
    return Super::Render();
}


void BallLassoWrapper::Reset(const BallLassoWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void BallLassoWrapper::Update(BallLassoPickup item) const
{
    Super::Update(static_cast<SpringPickup>(item));
}


void BallLassoWrapper::Multiply(const BallLassoWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<SpringWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
}


bool BattarangWrapper::Render()
{
    bool update = Super::Render();
    update |= ImGui::SliderFloat("Spin Speed", &SpinSpeed, 0.f, 10.f);

    return update;
}


void BattarangWrapper::Reset(const BattarangWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void BattarangWrapper::Update(BattarangPickup item) const
{
    Super::Update(static_cast<BallLassoPickup>(item));
    item.SetSpinSpeed(SpinSpeed);
}


void BattarangWrapper::Multiply(const BattarangWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<BallLassoWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
    SpinSpeed = def.SpinSpeed * forceMultiplier;
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


void HitForceWrapper::Reset(const HitForceWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void HitForceWrapper::Update(HitForcePickup item) const
{
    Super::Update(static_cast<RumblePickupComponentWrapper>(item));
    item.SetbBallForce(BallForce);
    item.SetbCarForce(CarForce);
    item.SetbDemolishCars(DemolishCars);
    item.SetBallHitForce(BallHitForce);
    item.SetCarHitForce(CarHitForce);
}


void HitForceWrapper::Multiply(const HitForceWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<RumbleWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
    BallHitForce = def.BallHitForce * forceMultiplier;
    CarHitForce = def.CarHitForce * forceMultiplier;
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


void VelcroWrapper::Reset(const VelcroWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void VelcroWrapper::Update(VelcroPickup item) const
{
    Super::Update(static_cast<RumblePickupComponentWrapper>(item));
    item.SetAfterHitDuration(AfterHitDuration);
    item.SetPostBreakDuration(PostBreakDuration);
    item.SetMinBreakForce(MinBreakForce);
    item.SetMinBreakTime(MinBreakTime);
    item.SetAttachTime(AttachTime);
    item.SetBreakTime(BreakTime);
}


void VelcroWrapper::Multiply(const VelcroWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<RumbleWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
    AfterHitDuration = def.AfterHitDuration * durationMultiplier;
    PostBreakDuration = def.PostBreakDuration * durationMultiplier;
    MinBreakForce = def.MinBreakForce * forceMultiplier;
    MinBreakTime = def.MinBreakTime * durationMultiplier;
    AttachTime = def.AttachTime * durationMultiplier;
    BreakTime = def.BreakTime * durationMultiplier;
}


bool SwapperWrapper::Render()
{
    return Super::Render();
}


void SwapperWrapper::Reset(const SwapperWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void SwapperWrapper::Update(SwapperPickup item) const
{
    Super::Update(static_cast<TargetedPickup>(item));
}


void SwapperWrapper::Multiply(const SwapperWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<TargetedWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
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


void TornadoWrapper::Reset(const TornadoWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void TornadoWrapper::Update(TornadoPickup item) const
{
    Super::Update(static_cast<RumblePickupComponentWrapper>(item));
    item.SetHeight(Height);
    item.SetRadius(Radius);
    item.SetRotationalForce(RotationalForce);
    item.SetTorque(Torque);
    item.SetFXScale(FxScale);
    item.SetMeshScale(MeshScale);
    item.SetMaxVelocityOffset(MaxVelocityOffset);
    item.SetBallMultiplier(BallMultiplier);
    item.SetVelocityEase(VelocityEase);
}


void TornadoWrapper::Multiply(const TornadoWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<RumbleWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
    Height = def.Height * rangeMultiplier;
    Radius = def.Radius * rangeMultiplier;
    RotationalForce = def.RotationalForce * forceMultiplier;
    Torque = def.Torque * forceMultiplier;
    FxScale = def.FxScale * rangeMultiplier;
    MeshScale = def.MeshScale * rangeMultiplier;
    MaxVelocityOffset = def.MaxVelocityOffset * forceMultiplier;
    BallMultiplier = def.BallMultiplier * forceMultiplier;
    VelocityEase = def.VelocityEase * forceMultiplier;
}


bool HauntedWrapper::Render()
{
    return Super::Render();
}


void HauntedWrapper::Reset(const HauntedWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void HauntedWrapper::Multiply(const HauntedWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<GravityWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
}


bool RugbyWrapper::Render()
{
    return Super::Render();
}


void RugbyWrapper::Reset(const RugbyWrapper def)
{
    const bool wasEnabled = Enabled;
    *this = def;
    Enabled = wasEnabled;
}


void RugbyWrapper::Multiply(const RugbyWrapper def, const float forceMultiplier, const float rangeMultiplier, const float durationMultiplier)
{
    Super::Multiply(static_cast<RumbleWrapper>(def), forceMultiplier, rangeMultiplier, durationMultiplier);
}
