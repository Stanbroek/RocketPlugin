#pragma once


class RumbleWrapper
{
public:
    bool Render();
    void Reset(RumbleWrapper def);
    void Update(RumblePickupComponentWrapper item) const;
    void Multiply(RumbleWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);

    bool Enabled;
    float ActivationDuration;
};


class TargetedWrapper : public RumbleWrapper
{
    using Super = RumbleWrapper;
public:
    bool Render();
    void Reset(TargetedWrapper def);
    void Update(TargetedPickup item) const;
    void Multiply(TargetedWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);

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
    bool Render();
    void Reset(SpringWrapper def);
    void Update(SpringPickup item) const;
    void Multiply(SpringWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);

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
    bool Render();
    void Reset(BallCarSpringWrapper def);
    void Update(BallCarSpringPickup item) const;
    void Multiply(BallCarSpringWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);
};


class BoostOverrideWrapper : public TargetedWrapper
{
    using Super = TargetedWrapper;
public:
    bool Render();
    void Reset(BoostOverrideWrapper def);
    void Update(BoostOverridePickup item) const;
    void Multiply(BoostOverrideWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);
};


class BallFreezeWrapper : public TargetedWrapper
{
    using Super = TargetedWrapper;
public:
    bool Render();
    void Reset(BallFreezeWrapper def);
    void Update(BallFreezePickup item) const;
    void Multiply(BallFreezeWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);

    bool MaintainMomentum;
    float TimeToStop;
    float StopMomentumPercentage;
};


class GrapplingHookWrapper : public TargetedWrapper
{
    using Super = TargetedWrapper;
public:
    bool Render();
    void Reset(GrapplingHookWrapper def);
    void Update(GrapplingHookPickup item) const;
    void Multiply(GrapplingHookWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);

    float Impulse;
    float Force;
    float MaxRopeLength;
    float PredictionSpeed;
};


class GravityWrapper : public RumbleWrapper
{
    using Super = RumbleWrapper;
public:
    bool Render();
    void Reset(GravityWrapper def);
    void Update(GravityPickup item) const;
    void Multiply(GravityWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);

    float BallGravity;
    float Range;
    bool DeactivateOnTouch;
};


class BallLassoWrapper : public SpringWrapper
{
    using Super = SpringWrapper;
public:
    bool Render();
    void Reset(BallLassoWrapper def);
    void Update(BallLassoPickup item) const;
    void Multiply(BallLassoWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);
};


class BattarangWrapper : public BallLassoWrapper
{
    using Super = BallLassoWrapper;
public:
    bool Render();
    void Reset(BattarangWrapper def);
    void Update(BattarangPickup item) const;
    void Multiply(BattarangWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);

    float SpinSpeed;
};


class HitForceWrapper : public RumbleWrapper
{
    using Super = RumbleWrapper;
public:
    bool Render();
    void Reset(HitForceWrapper def);
    void Update(HitForcePickup item) const;
    void Multiply(HitForceWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);

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
    bool Render();
    void Reset(VelcroWrapper def);
    void Update(VelcroPickup item) const;
    void Multiply(VelcroWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);

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
    bool Render();
    void Reset(SwapperWrapper def);
    void Update(SwapperPickup item) const;
    void Multiply(SwapperWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);
};


class TornadoWrapper : public RumbleWrapper
{
    using Super = RumbleWrapper;
public:
    bool Render();
    void Reset(TornadoWrapper def);
    void Update(TornadoPickup item) const;
    void Multiply(TornadoWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);

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
    bool Render();
    void Reset(HauntedWrapper def);
    //void Update(HauntedPickup item) const;
    void Multiply(HauntedWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);
};


class RugbyWrapper : public RumbleWrapper
{
    using Super = RumbleWrapper;
public:
    bool Render();
    void Reset(RugbyWrapper def);
    //void Update(RugbyPickup item) const;
    void Multiply(RugbyWrapper def, float forceMultiplier, float rangeMultiplier, float durationMultiplier);
};
