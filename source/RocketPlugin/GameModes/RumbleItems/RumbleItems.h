#pragma once
#include <utility>


class RumbleWrapper
{
public:
    constexpr RumbleWrapper(const std::string displayName, const std::string internalName, const bool enabled, const float activationDuration) :
        DisplayName(std::move(displayName)),
        InternalName(std::move(internalName)),
        Enabled(enabled),
        ActivationDuration(activationDuration) {}
    explicit constexpr RumbleWrapper(const RumbleWrapper* archetype) :
        Archetype(archetype),
        DisplayName(archetype->DisplayName),
        InternalName(archetype->InternalName),
        Enabled(archetype->Enabled),
        ActivationDuration(archetype->ActivationDuration) {}
    virtual ~RumbleWrapper() = default;

    virtual bool Render();
    virtual void Reset();
    virtual void Update(std::uintptr_t item) const;
    virtual void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier);

    const RumbleWrapper* Archetype = nullptr;
    /*const*/ std::string DisplayName;
    /*const*/ std::string InternalName;

    bool Enabled;
    float ActivationDuration;
};


class TargetedWrapper : public RumbleWrapper
{
    using Super = RumbleWrapper;
public:
    constexpr TargetedWrapper(const Super& super, const bool canTargetBall, const bool canTargetCars,
        const bool canTargetEnemyCars, const bool canTargetTeamCars, const float range) :
        Super(super),
        CanTargetBall(canTargetBall),
        CanTargetCars(canTargetCars),
        CanTargetEnemyCars(canTargetEnemyCars),
        CanTargetTeamCars(canTargetTeamCars),
        Range(range) {}
    explicit constexpr TargetedWrapper(const TargetedWrapper* archetype) :
        Super(archetype),
        CanTargetBall(archetype->CanTargetBall),
        CanTargetCars(archetype->CanTargetCars),
        CanTargetEnemyCars(archetype->CanTargetEnemyCars),
        CanTargetTeamCars(archetype->CanTargetTeamCars),
        Range(archetype->Range) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;

    bool CanTargetBall;
    bool CanTargetCars;
    bool CanTargetEnemyCars;
    bool CanTargetTeamCars;
    float Range;
};


class SpringWrapper : public TargetedWrapper
{
    using Super = TargetedWrapper;
public:
    constexpr SpringWrapper(const Super& super, const float force, const float verticalForce, const Vector& torque,
        const float relativeForceNormalDirection, const float maxSpringLength, const float constantForce,
        const float minSpringLength, const float weldedForceScalar, const float weldedVerticalForce) :
        Super(super),
        Force(force),
        VerticalForce(verticalForce),
        Torque(torque),
        RelativeForceNormalDirection(relativeForceNormalDirection),
        MaxSpringLength(maxSpringLength),
        ConstantForce(constantForce),
        MinSpringLength(minSpringLength),
        WeldedForceScalar(weldedForceScalar),
        WeldedVerticalForce(weldedVerticalForce) {}
    explicit constexpr SpringWrapper(const SpringWrapper* archetype) :
        Super(archetype),
        Force(archetype->Force),
        VerticalForce(archetype->VerticalForce),
        Torque(archetype->Torque),
        RelativeForceNormalDirection(archetype->RelativeForceNormalDirection),
        MaxSpringLength(archetype->MaxSpringLength),
        ConstantForce(archetype->ConstantForce),
        MinSpringLength(archetype->MinSpringLength),
        WeldedForceScalar(archetype->WeldedForceScalar),
        WeldedVerticalForce(archetype->WeldedVerticalForce) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;

    float Force;
    float VerticalForce;
    Vector Torque;
    float RelativeForceNormalDirection;
    float MaxSpringLength;
    float ConstantForce;
    float MinSpringLength;
    float WeldedForceScalar;
    float WeldedVerticalForce;
};


class BallCarSpringWrapper : public SpringWrapper
{
    using Super = SpringWrapper;
public:
    constexpr BallCarSpringWrapper(const Super& super) : Super(super) {}
    explicit constexpr BallCarSpringWrapper(const BallCarSpringWrapper* archetype) : Super(archetype) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;
};


class BoostOverrideWrapper : public TargetedWrapper
{
    using Super = TargetedWrapper;
public:
    constexpr BoostOverrideWrapper(const Super& super) : Super(super) {}
    explicit constexpr BoostOverrideWrapper(const BoostOverrideWrapper* archetype) : Super(archetype) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;
};


class BallFreezeWrapper : public TargetedWrapper
{
    using Super = TargetedWrapper;
public:
    constexpr BallFreezeWrapper(const Super& super, const bool maintainMomentum, const float timeToStop,
        const float stopMomentumPercentage) :
        Super(super),
        MaintainMomentum(maintainMomentum),
        TimeToStop(timeToStop),
        StopMomentumPercentage(stopMomentumPercentage) {}
    explicit constexpr BallFreezeWrapper(const BallFreezeWrapper* archetype) :
        Super(archetype),
        MaintainMomentum(archetype->MaintainMomentum),
        TimeToStop(archetype->TimeToStop),
        StopMomentumPercentage(archetype->StopMomentumPercentage) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;

    bool MaintainMomentum;
    float TimeToStop;
    float StopMomentumPercentage;
};


class GrapplingHookWrapper : public TargetedWrapper
{
    using Super = TargetedWrapper;
public:
    constexpr GrapplingHookWrapper(const Super& super, const float impulse, const float force,
        const float maxRopeLength, const float predictionSpeed) :
        Super(super),
        Impulse(impulse),
        Force(force),
        MaxRopeLength(maxRopeLength),
        PredictionSpeed(predictionSpeed) {}
    explicit constexpr GrapplingHookWrapper(const GrapplingHookWrapper* archetype) :
        Super(archetype),
        Impulse(archetype->Impulse),
        Force(archetype->Force),
        MaxRopeLength(archetype->MaxRopeLength),
        PredictionSpeed(archetype->PredictionSpeed) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;

    float Impulse;
    float Force;
    float MaxRopeLength;
    float PredictionSpeed;
};


class GravityWrapper : public RumbleWrapper
{
    using Super = RumbleWrapper;
public:
    constexpr GravityWrapper(const Super& super, const float ballGravity, const float range, const bool deactivateOnTouch) :
        Super(super),
        BallGravity(ballGravity),
        Range(range),
        DeactivateOnTouch(deactivateOnTouch) {}
    explicit constexpr GravityWrapper(const GravityWrapper* archetype) :
        Super(archetype),
        BallGravity(archetype->BallGravity),
        Range(archetype->Range),
        DeactivateOnTouch(archetype->DeactivateOnTouch) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;

    float BallGravity;
    float Range;
    bool DeactivateOnTouch;
};


class BallLassoWrapper : public SpringWrapper
{
    using Super = SpringWrapper;
public:
    constexpr BallLassoWrapper(const Super& super) : Super(super) {}
    explicit constexpr BallLassoWrapper(const BallLassoWrapper* archetype) : Super(archetype) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;
};


class BattarangWrapper : public BallLassoWrapper
{
    using Super = BallLassoWrapper;
public:
    constexpr BattarangWrapper(const Super& super, const float spinSpeed) :
        Super(super),
        SpinSpeed(spinSpeed) {}
    explicit constexpr BattarangWrapper(const BattarangWrapper* archetype) :
        Super(archetype),
        SpinSpeed(archetype->SpinSpeed) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;

    float SpinSpeed;
};


class HitForceWrapper : public RumbleWrapper
{
    using Super = RumbleWrapper;
public:
    constexpr HitForceWrapper(const Super& super, const bool ballForce, const bool carForce, const bool demolishCars,
        const float ballHitForce, const float carHitForce) :
        Super(super),
        BallForce(ballForce),
        CarForce(carForce),
        DemolishCars(demolishCars),
        BallHitForce(ballHitForce),
        CarHitForce(carHitForce) {}
    explicit constexpr HitForceWrapper(const HitForceWrapper* archetype) :
        Super(archetype),
        BallForce(archetype->BallForce),
        CarForce(archetype->CarForce),
        DemolishCars(archetype->DemolishCars),
        BallHitForce(archetype->BallHitForce),
        CarHitForce(archetype->CarHitForce) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;

    bool BallForce;
    bool CarForce;
    bool DemolishCars;
    float BallHitForce;
    float CarHitForce;
};


class VelcroWrapper : public RumbleWrapper
{
    using Super = RumbleWrapper;
public:
    constexpr VelcroWrapper(const Super& super, const float afterHitDuration, const float postBreakDuration,
        const float minBreakForce, const float minBreakTime, const float attachTime, const float breakTime) :
        Super(super),
        AfterHitDuration(afterHitDuration),
        PostBreakDuration(postBreakDuration),
        MinBreakForce(minBreakForce),
        MinBreakTime(minBreakTime),
        AttachTime(attachTime),
        BreakTime(breakTime) {}
    explicit constexpr VelcroWrapper(const VelcroWrapper* archetype) :
        Super(archetype),
        AfterHitDuration(archetype->AfterHitDuration),
        PostBreakDuration(archetype->PostBreakDuration),
        MinBreakForce(archetype->MinBreakForce),
        MinBreakTime(archetype->MinBreakTime),
        AttachTime(archetype->AttachTime),
        BreakTime(archetype->BreakTime) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;

    float AfterHitDuration;
    float PostBreakDuration;
    float MinBreakForce;
    float MinBreakTime;
    float AttachTime;
    float BreakTime;
};


class SwapperWrapper : public TargetedWrapper
{
    using Super = TargetedWrapper;
public:
    constexpr SwapperWrapper(const Super& super) : Super(super) {}
    explicit constexpr SwapperWrapper(const SwapperWrapper* archetype) : Super(archetype) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;
};


class TornadoWrapper : public RumbleWrapper
{
    using Super = RumbleWrapper;
public:
    constexpr TornadoWrapper(const Super& super, const float height, const float radius, const float rotationalForce,
        const float torque, const Vector& fxScale, const Vector& meshScale, const float maxVelocityOffset,
        const float ballMultiplier, const float velocityEase) :
        Super(super),
        Height(height),
        Radius(radius),
        RotationalForce(rotationalForce),
        Torque(torque),
        FxScale(fxScale),
        MeshScale(meshScale),
        MaxVelocityOffset(maxVelocityOffset),
        BallMultiplier(ballMultiplier),
        VelocityEase(velocityEase) {}
    explicit constexpr TornadoWrapper(const TornadoWrapper* archetype) :
        Super(archetype),
        Height(archetype->Height),
        Radius(archetype->Radius),
        RotationalForce(archetype->RotationalForce),
        Torque(archetype->Torque),
        FxScale(archetype->FxScale),
        MeshScale(archetype->MeshScale),
        MaxVelocityOffset(archetype->MaxVelocityOffset),
        BallMultiplier(archetype->BallMultiplier),
        VelocityEase(archetype->VelocityEase) {}

    bool Render() override;
    void Reset() override;
    void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;

    float Height;
    float Radius;
    float RotationalForce;
    float Torque;
    Vector FxScale;
    Vector MeshScale;
    float MaxVelocityOffset;
    float BallMultiplier;
    float VelocityEase;
};


class HauntedWrapper : public GravityWrapper
{
    using Super = GravityWrapper;
public:
    constexpr HauntedWrapper(const Super& super) : Super(super) {}
    explicit constexpr HauntedWrapper(const HauntedWrapper* archetype) : Super(archetype) {}

    bool Render() override;
    void Reset() override;
    //void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;
};


class RugbyWrapper : public RumbleWrapper
{
    using Super = RumbleWrapper;
public:
    constexpr RugbyWrapper(const Super& super) : Super(super) {}
    explicit constexpr RugbyWrapper(const RugbyWrapper* archetype) : Super(archetype) {}

    bool Render() override;
    void Reset() override;
    //void Update(std::uintptr_t item) const override;
    void Multiply(float forceMultiplier, float rangeMultiplier, float durationMultiplier) override;
};
