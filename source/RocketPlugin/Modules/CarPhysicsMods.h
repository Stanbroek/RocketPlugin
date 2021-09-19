#pragma once
#include "RocketPluginModule.h"


class RocketPlugin;

class CarPhysicsMods final : RocketPluginModule
{
    friend RocketPlugin;
public:
    struct CarPhysics {
        CarPhysics(const RocketPlugin* rp, PriWrapper player);

        float CarScale;
        bool CarHasCollision;
        bool CarIsFrozen;
        float TorqueRate;
        float MaxCarVelocity;
        float GroundStickyForce;
        float WallStickyForce;
    };
    void SetPhysics(CarWrapper car);
    CarPhysics GetPhysics(PriWrapper player);
    CarPhysics& GetPhysicsCache(PriWrapper player);
    void SetbCarCollision(PriWrapper player, bool carHasCollision);
    bool GetbCarCollision(PriWrapper player) const;
    void SetCarScale(PriWrapper player, float newCarScale, bool shouldRespawn = false);
    float GetCarScale(PriWrapper player) const;
    void SetCarIsFrozen(PriWrapper player, bool carIsFrozen);
    bool GetCarIsFrozen(PriWrapper player) const;
    void SetTorqueRate(PriWrapper player, float torqueRate);
    float GetTorqueRate(PriWrapper player) const;
    void SetMaxCarVelocity(PriWrapper player, float maxCarVelocity);
    float GetMaxCarVelocity(PriWrapper player) const;
    StickyForceData GetStickyForce(PriWrapper player) const;
    void SetGroundStickyForce(PriWrapper player, float groundStickyForce);
    float GetGroundStickyForce(PriWrapper player) const;
    void SetWallStickyForce(PriWrapper player, float wallStickyForce);
    float GetWallStickyForce(PriWrapper player) const;

protected:
    size_t selectedPlayer = 0;
    std::unordered_map<uint64_t, CarPhysics> carPhysics;
};
