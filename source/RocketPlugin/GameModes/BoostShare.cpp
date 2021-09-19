// GameModes/BoostShare.cpp
// A boost sharing game mode for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.0.4 28/08/21
// BMSDK version: 95

#include "BoostShare.h"


/// <summary>Renders the available options for the game mode.</summary>
void BoostShare::RenderOptions()
{
    constexpr unsigned short minBoostPool = 0;
    constexpr unsigned short maxBoostPool = 200;
    ImGui::SliderScalar("boost pool", ImGuiDataType_U16, &boostPool, &minBoostPool, &maxBoostPool, "%d");
    if (ImGui::IsItemJustReleased()) {
        Execute([this](GameWrapper*) {
            distributeBoostPool();
        });
    }
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool BoostShare::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void BoostShare::Activate(const bool active)
{
    if (active && !isActive) {
        HookEvent(
            "Function TAGame.GameEvent_TA.Init",
            [this](const std::string&) {
                initialize();
            });
        HookEvent(
            "Function GameEvent_Soccar_TA.Countdown.BeginState",
            [this](const std::string&) {
                distributeBoostPool();
            });
        HookEventWithCaller<ServerWrapper>(
            "Function GameEvent_Soccar_TA.Active.Tick",
            [this](const ServerWrapper& server, void* params, const std::string&) {
                onTick(server, params);
            });
        initialize();
    }
    else if (!active && isActive) {
        UnhookEvent("Function TAGame.GameEvent_TA.Init");
        UnhookEvent("Function GameEvent_Soccar_TA.Countdown.BeginState");
        UnhookEvent("Function GameEvent_Soccar_TA.Active.Tick");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string BoostShare::GetGameModeName()
{
    return "Boost Share";
}


/// <summary>Initializes the game mode.</summary>
void BoostShare::initialize() const
{
    removePickups();
    distributeBoostPool();
}


/// <summary>Distributes boost pool along players.</summary>
/// <remarks>Gets called post 'Function GameEvent_Soccar_TA.Countdown.BeginState'.</remarks>
void BoostShare::distributeBoostPool() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    ArrayWrapper<CarWrapper> cars = game.GetCars();
    BMCHECK_ARRAY(cars, 0);

    const int boostPerCar = boostPool / cars.Count();
    for (CarWrapper car : cars) {
        BMCHECK_LOOP(car);

        BoostWrapper boostComponent = car.GetBoostComponent();
        BMCHECK_LOOP(boostComponent);

        boostComponent.SetBoostAmount(static_cast<float>(boostPerCar) / 100.f);
        boostComponent.ClientGiveBoost(0);
        BM_TRACE_LOG("set {:s} there boost to {:d}", quote(car.GetOwnerName()), boostPerCar);
    }
}


/// <summary>Updates the game every game tick.</summary>
/// <remarks>Gets called on 'Function GameEvent_Soccar_TA.Active.Tick'.</remarks>
/// <param name="server"><see cref="ServerWrapper"/> instance of the current server</param>
/// <param name="params">Delay since last update</param>
void BoostShare::onTick(ServerWrapper server, void* params) const
{
    BMCHECK(server);

    // delta time since last tick in seconds.
    const float deltaTime = *static_cast<float*>(params);

    // If there is only one car on the field give them full boost.
    ArrayWrapper<CarWrapper> cars = server.GetCars();
    if (cars.Count() == 1) {
        CarWrapper car = cars.Get(0);
        BMCHECK(car);

        if (!car.GetInput().ActivateBoost) {
            return;
        }

        BoostWrapper boostComponent = car.GetBoostComponent();
        BMCHECK(boostComponent);

        boostComponent.SetBoostAmount(static_cast<float>(boostPool) / 100.f);
        boostComponent.ClientGiveBoost(0);
        return;
    }

    // Otherwise distribute it among the others.
    for (CarWrapper car : cars) {
        BMCHECK_LOOP(car);

        if (!car.GetInput().ActivateBoost) {
            continue;
        }

        BoostWrapper boostComponent = car.GetBoostComponent();
        BMCHECK_LOOP(boostComponent);

        const std::uintptr_t carAddr = car.memory_address;
        const float boostConsumed = deltaTime * boostComponent.GetBoostConsumptionRate();
        const float boostPerCar = boostConsumed / (static_cast<float>(cars.Count()) - 1);
        for (CarWrapper otherCar : cars) {
            BMCHECK_LOOP(otherCar);

            if (otherCar.memory_address == carAddr) {
                continue;
            }

            BoostWrapper otherBoostComponent = otherCar.GetBoostComponent();
            BMCHECK_LOOP(otherBoostComponent);

            otherBoostComponent.GiveBoost2(boostPerCar);
        }
    }

    // Calculate current boost pool.
    float currentBoostPool = 0;
    for (CarWrapper car : cars) {
        BMCHECK_LOOP(car);

        BoostWrapper boostComponent = car.GetBoostComponent();
        BMCHECK_LOOP(boostComponent);

        currentBoostPool += boostComponent.GetCurrentBoostAmount();
    }

    // Correct boost pool.
    const float boostPerCar = (static_cast<float>(boostPool) / 100 - currentBoostPool) /
                              static_cast<float>(cars.Count());
    if (boostPerCar == 0.f) {
        return;
    }

    for (CarWrapper car : cars) {
        BMCHECK_LOOP(car);

        BoostWrapper boostComponent = car.GetBoostComponent();
        BMCHECK_LOOP(boostComponent);

        boostComponent.GiveBoost2(boostPerCar);
    }
}
