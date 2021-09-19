// CarPhysicsMods.cpp
// Car physics mods for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.6.8 18/09/21
// BMSDK version: 95
#include "CarPhysicsMods.h"
#include "RocketPlugin.h"


CarPhysicsMods::CarPhysics::CarPhysics(const RocketPlugin* rp, const PriWrapper player)
{
    CarScale = rp->carPhysicsMods.GetCarScale(player);
    CarHasCollision = rp->carPhysicsMods.GetbCarCollision(player);
    CarIsFrozen = rp->carPhysicsMods.GetCarIsFrozen(player);
    TorqueRate = rp->carPhysicsMods.GetTorqueRate(player);
    MaxCarVelocity = rp->carPhysicsMods.GetMaxCarVelocity(player);
    GroundStickyForce = rp->carPhysicsMods.GetGroundStickyForce(player);
    WallStickyForce = rp->carPhysicsMods.GetWallStickyForce(player);
}


/// <summary>Sets the car physics for the player.</summary>
/// <remarks>Gets called on 'TAGame.Car_TA.EventVehicleSetup'.</remarks>
/// <param name="car">The players car to set the physics of</param>
void CarPhysicsMods::SetPhysics(CarWrapper car)
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    BMCHECK(car);

    PriWrapper player = car.GetPRI();
    BMCHECK(player);

    const auto& it = carPhysics.find(player.GetUniqueIdWrapper().GetUID());
    if (it == carPhysics.end()) {
        return;
    }

    const CarPhysics playerCarPhysics = it->second;
    SetCarScale(player, playerCarPhysics.CarScale);
    SetbCarCollision(player, playerCarPhysics.CarHasCollision);
    SetCarIsFrozen(player, playerCarPhysics.CarIsFrozen);
    SetTorqueRate(player, playerCarPhysics.TorqueRate);
    SetMaxCarVelocity(player, playerCarPhysics.MaxCarVelocity);
    SetGroundStickyForce(player, playerCarPhysics.GroundStickyForce);
    SetWallStickyForce(player, playerCarPhysics.WallStickyForce);
}


/// <summary>Gets the car physics for the player.</summary>
/// <param name="player">The player to get the car physics from</param>
/// <returns>The car physics for the player</returns>
CarPhysicsMods::CarPhysics CarPhysicsMods::GetPhysics(PriWrapper player)
{
    BMCHECK_SILENT(player, CarPhysics(Outer(), player));

    const auto& it = carPhysics.find(player.GetUniqueIdWrapper().GetUID());
    if (it == carPhysics.end()) {
        return CarPhysics(Outer(), player);
    }

    return it->second;
}


/// <summary>Gets the car physics for the player and saves them when not found.</summary>
/// <param name="player">The player to get the car physics from</param>
/// <returns>The car physics for the player</returns>
CarPhysicsMods::CarPhysics& CarPhysicsMods::GetPhysicsCache(PriWrapper player)
{
    const uint64_t steamID = player.GetUniqueIdWrapper().GetUID();
    return carPhysics.try_emplace(steamID, CarPhysics(Outer(), player)).first->second;
}


/// <summary>Sets the if the players car has collision in the current game.</summary>
/// <param name="player">the player to update the car of</param>
/// <param name="carHasCollision">Bool with if the players car should have collision</param>
void CarPhysicsMods::SetbCarCollision(PriWrapper player, const bool carHasCollision)
{
    BMCHECK(player);

    CarWrapper car = player.GetCar();
    BMCHECK(car);

    GetPhysicsCache(player).CarHasCollision = carHasCollision;
    car.SetbCollideActors(carHasCollision);
}


/// <summary>Gets the if the players car has collision in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>Bool with if the players car has collision</returns>
bool CarPhysicsMods::GetbCarCollision(PriWrapper player) const
{
    BMCHECK_SILENT(player, true);

    CarWrapper car = player.GetCar();
    BMCHECK_SILENT(car, true);

    return car.GetbCollideActors();
}


/// <summary>Sets the players car scale in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="newCarScale">The players car scale</param>
/// <param name="shouldRespawn">Bool with if the car should respawn in place</param>
void CarPhysicsMods::SetCarScale(PriWrapper player, const float newCarScale, const bool shouldRespawn)
{
    BMCHECK(player);

    CarWrapper car = player.GetCar();
    BMCHECK(car);

    GetPhysicsCache(player).CarScale = newCarScale;
    car.SetCarScale(newCarScale);
    if (shouldRespawn) {
        player.GetCar().RespawnInPlace();
    }
}


/// <summary>Gets the players car scale in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The players car scale</returns>
float CarPhysicsMods::GetCarScale(PriWrapper player) const
{
    BMCHECK_SILENT(player, 1.f);

    CarWrapper car = player.GetCar();
    BMCHECK_SILENT(car, 1.f);

    const float carScale = car.GetReplicatedCarScale();

    return carScale > 0 ? carScale : 1.0f;
}


/// <summary>Sets the if the players car should be frozen in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="carIsFrozen">Bool with if the players car should be frozen</param>
void CarPhysicsMods::SetCarIsFrozen(PriWrapper player, const bool carIsFrozen)
{
    BMCHECK(player);

    CarWrapper car = player.GetCar();
    BMCHECK(car);

    GetPhysicsCache(player).CarIsFrozen = carIsFrozen;
    car.SetFrozen(carIsFrozen);
}


/// <summary>Gets the if the players car is frozen in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <returns>Bool with if the players car is frozen</returns>
bool CarPhysicsMods::GetCarIsFrozen(PriWrapper player) const
{
    BMCHECK_SILENT(player, false);

    CarWrapper car = player.GetCar();
    BMCHECK_SILENT(car, false);

    return car.GetbFrozen();
}


/// <summary>Sets the players car drive torque in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="torqueRate">The new drive torque of the players car</param>
void CarPhysicsMods::SetTorqueRate(PriWrapper player, const float torqueRate)
{
    BMCHECK(player);

    CarWrapper car = player.GetCar();
    BMCHECK(car);

    GetPhysicsCache(player).TorqueRate = torqueRate;
    car.GetVehicleSim().SetDriveTorque(torqueRate * 100000);
}


/// <summary>Gets the players car drive torque in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The drive torque of the players car</returns>
float CarPhysicsMods::GetTorqueRate(PriWrapper player) const
{
    BMCHECK_SILENT(player, 2.88f);

    CarWrapper car = player.GetCar();
    BMCHECK_SILENT(car, 2.88f);

    return car.GetVehicleSim().GetDriveTorque() / 100000;
}


/// <summary>Sets the players car max velocity in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="maxCarVelocity">The new max velocity of the players car</param>
void CarPhysicsMods::SetMaxCarVelocity(PriWrapper player, const float maxCarVelocity)
{
    BMCHECK(player);

    CarWrapper car = player.GetCar();
    BMCHECK(car);

    GetPhysicsCache(player).MaxCarVelocity = maxCarVelocity;
    car.SetMaxLinearSpeed(maxCarVelocity);
}


/// <summary>Gets the players car max velocity in the current game.</summary>
/// <param name="player">the player to check the car of</param>
/// <returns>The max velocity of the players car</returns>
float CarPhysicsMods::GetMaxCarVelocity(PriWrapper player) const
{
    BMCHECK_SILENT(player, 2300.f);

    CarWrapper car = player.GetCar();
    BMCHECK_SILENT(car, 2300.f);

    return car.GetMaxLinearSpeed();
}


/// <summary>Gets the players car sticky force in the current game.</summary>
/// <param name="player">The player to get the sticky force of the car of</param>
/// <returns>The sticky force of the players car</returns>
StickyForceData CarPhysicsMods::GetStickyForce(PriWrapper player) const
{
    BMCHECK_SILENT(player, StickyForceData());

    CarWrapper car = player.GetCar();
    BMCHECK_SILENT(car, StickyForceData());

    return car.GetStickyForce();
}


/// <summary>Sets the players car ground sticky force in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="groundStickyForce">The new ground sticky force of the players car</param>
void CarPhysicsMods::SetGroundStickyForce(PriWrapper player, const float groundStickyForce)
{
    BMCHECK(player);

    CarWrapper car = player.GetCar();
    BMCHECK(car);

    StickyForceData sfd = GetStickyForce(player);
    sfd.Ground = groundStickyForce;

    GetPhysicsCache(player).GroundStickyForce = groundStickyForce;
    car.SetStickyForce(sfd);
}


/// <summary>Gets the players car ground sticky force in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The ground sticky force of the players car</returns>
float CarPhysicsMods::GetGroundStickyForce(PriWrapper player) const
{
    BMCHECK_SILENT(player, .5f);

    CarWrapper car = player.GetCar();
    BMCHECK_SILENT(car, .5f);

    return car.GetStickyForce().Ground;
}


/// <summary>Sets the players car wall sticky force in the current game.</summary>
/// <param name="player">The player to update the car of</param>
/// <param name="wallStickyForce">The new wall sticky force of the players car</param>
void CarPhysicsMods::SetWallStickyForce(PriWrapper player, const float wallStickyForce)
{
    BMCHECK(player);

    CarWrapper car = player.GetCar();
    BMCHECK(car);

    StickyForceData sfd = GetStickyForce(player);
    sfd.Wall = wallStickyForce;

    GetPhysicsCache(player).WallStickyForce = wallStickyForce;
    car.SetStickyForce(sfd);
}


/// <summary>Gets the players car wall sticky force in the current game.</summary>
/// <param name="player">The player to check the car of</param>
/// <returns>The wall sticky force of the players car</returns>
float CarPhysicsMods::GetWallStickyForce(PriWrapper player) const
{
    BMCHECK_SILENT(player, 1.5f);

    CarWrapper car = player.GetCar();
    BMCHECK_SILENT(car, 1.5f);

    return car.GetStickyForce().Wall;
}
