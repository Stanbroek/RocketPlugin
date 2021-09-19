// GameModes/SacredGround.cpp
// A Rocket Plugin game mode where you have to stay close to your own goal.
//
// Author:        Stanbroek
// Version:       0.0.2 15/08/21
// BMSDK version: 95

#include "SacredGround.h"


/// <summary>Renders the available options for the game mode.</summary>
void SacredGround::RenderOptions()
{
    ImGui::Checkbox("Only demo car when any wheel is touching the ground", &demoOnGround);
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool SacredGround::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void SacredGround::Activate(const bool active)
{
    if (active && !isActive) {
        HookEventWithCaller<ServerWrapper>(
            "Function GameEvent_Soccar_TA.Active.Tick",
            [this](const ServerWrapper& caller, void*, const std::string&) {
                onTick(caller);
            });
    }
    else if (!active && isActive) {
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string SacredGround::GetGameModeName()
{
    return "Sacred Ground";
}


/// <summary>Get the distance between two points.</summary>
/// <param name="p1">First point</param>
/// <param name="p2">Second point</param>
/// <returns>The distance between the two points</returns>
float GetDistance(const Vector& p1, const Vector& p2)
{
    // Using Pythagorean theorem.
    //return std::sqrtf(std::powf(p1.X - p2.X, 2) + std::powf(p1.Y - p2.Y, 2) + std::powf(p1.Z - p2.Z, 2));
    // Because we do not need the exact distance, only if it is closer or further away, we can simplify it to.
    return std::abs(p1.X - p2.X) + std::abs(p1.Y - p2.Y) + std::abs(p1.Z - p2.Z);
}


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the current server</param>
void SacredGround::onTick(ServerWrapper server) const
{
    BMCHECK(server);

    for (CarWrapper car : server.GetCars()) {
        BMCHECK_LOOP(car);

        const Vector carLocation = car.GetLocation();
        unsigned char closedGoal = car.GetTeamNum2();
        float closedGoalDistance = std::numeric_limits<float>::max();

        for (GoalWrapper goal : server.GetGoals()) {
            BMCHECK_LOOP(goal);

            const float distance = GetDistance(carLocation, goal.GetLocation());
            if (distance < closedGoalDistance) {
                closedGoal = goal.GetTeamNum();
                closedGoalDistance = distance;
            }
        }

        if (closedGoal != car.GetTeamNum2()) {
            if (demoOnGround && !car.AnyWheelTouchingGround()) {
                continue;
            }
            BM_TRACE_LOG("demolished {:s}, closed goal was {:d}", quote(car.GetOwnerName()), closedGoal);
            car.Demolish();
        }
    }
}
