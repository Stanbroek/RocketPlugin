// GameModes/CrazyRumble.cpp
// A crazy rumble customizer game mode for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.3.3 07/09/21
// BMSDK version: 95

#include "CrazyRumble.h"


/// <summary>Renders the available options for the game mode.</summary>
void CrazyRumble::RenderOptions()
{
    if (refreshRumbleConstants) {
        if (!rumbleConstantsRequest.valid()) {
            rumbleConstantsRequest = Outer()->ConstantsConfig->RequestRumbleConstants();
        }
        if (rumbleConstantsRequest._Is_ready()) {
            refreshRumbleConstants = false;
            const auto& [successful, data] = rumbleConstantsRequest.get();
            if (successful) {
                RPConfig::ParseRumbleItems(data, this);
            }
        }
    }

    bool shouldUpdateCars = false;
    bool shouldUpdateItemPool = false;

    ImGui::Text("Presets:");
    ImGui::Spacing();

    if (ImGui::Button("Bumper Cars")) {
        ResetItemsValues();
        for (const std::shared_ptr<RumbleWrapper>& rumbleItem : rumbleItems) {
            rumbleItem->Enabled = false;
        }
        powerhitter->Enabled = true;
        powerhitter->DemolishCars = false;
        powerhitter->ActivationDuration = 300.0f;
        shouldUpdateCars = true;
        shouldUpdateItemPool = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Target Everything")) {
        ResetItemsValues();
        for (const std::shared_ptr<RumbleWrapper>& rumbleItem : rumbleItems) {
            if (const std::shared_ptr<TargetedWrapper>& targetedRumbleItem = std::dynamic_pointer_cast<TargetedWrapper>(rumbleItem)) {
                targetedRumbleItem->CanTargetBall = true;
                targetedRumbleItem->CanTargetCars = true;
                targetedRumbleItem->CanTargetEnemyCars = true;
                targetedRumbleItem->CanTargetTeamCars = true;
            }
        }
        shouldUpdateCars = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Negative Forces")) {
        ResetItemsValues();
        UpdateItemsValues(-1.0f, 1.0f, 1.0f);
        shouldUpdateCars = true;
    }
    if (ImGui::Button("Extreme power")) {
        ResetItemsValues();
        UpdateItemsValues(10.0f, 1.0f, 1.0f);
        shouldUpdateCars = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Long Ranged")) {
        ResetItemsValues();
        UpdateItemsValues(1.0f, 4.0f, 1.0f);
        shouldUpdateCars = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Insane Duration")) {
        ResetItemsValues();
        UpdateItemsValues(1.0f, 1.0f, 10.0f);
        shouldUpdateCars = true;
    }
    ImGui::Spacing();

    if (ImGui::SliderFloat("the item force", &forceMultiplier, 0.0f, 10.0f, "%.3fX")) {
        UpdateItemsValues(forceMultiplier, rangeMultiplier, durationMultiplier);
        shouldUpdateCars = true;
    }
    if (ImGui::SliderFloat("the item range", &rangeMultiplier, 0.0f, 10.0f, "%.3fX")) {
        UpdateItemsValues(forceMultiplier, rangeMultiplier, durationMultiplier);
        shouldUpdateCars = true;
    }
    if (ImGui::SliderFloat("the item duration", &durationMultiplier, 0.0f, 10.0f, "%.3fX")) {
        UpdateItemsValues(forceMultiplier, rangeMultiplier, durationMultiplier);
        shouldUpdateCars = true;
    }
    if (ImGui::SliderInt("till next item", &maxTimeTillItem, 0, 20, "%d seconds")) {
        Execute([this](GameWrapper*) {
            updateDispensers(true, false);
        });
    }
    if (ImGui::Button("Default everything 1X")) {
        ResetItemsValues();
        UpdateItemsValues(1.0f, 1.0f, 1.0f);
        shouldUpdateCars = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Wild everything 2X")) {
        ResetItemsValues();
        UpdateItemsValues(2.0f, 2.0f, 2.0f);
        shouldUpdateCars = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Crazy everything 5X")) {
        ResetItemsValues();
        UpdateItemsValues(5.0f, 5.0f, 5.0f);
        shouldUpdateCars = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Insanity everything 10X")) {
        ResetItemsValues();
        UpdateItemsValues(10.0f, 10.0f, 10.0f);
        shouldUpdateCars = true;
    }
    ImGui::Separator();

    ImGui::Text("Per item customization:");
    ImGui::Spacing();

    for (const std::shared_ptr<RumbleWrapper>& rumbleItem : rumbleItems) {
        shouldUpdateItemPool |= ImGui::Checkbox(fmt::format("##Enable{:s}", rumbleItem->DisplayName).c_str(), &rumbleItem->Enabled);
        ImGui::SameLine();
        if (ImGui::CollapsingHeader(rumbleItem->DisplayName.c_str())) {
            shouldUpdateCars |= rumbleItem->Render();
        }
    }

    if (shouldUpdateItemPool) {
        Execute([this](GameWrapper*) {
            updateDispensers(false, true);
        });
    }
    if (shouldUpdateCars) {
        Execute([this](GameWrapper*) {
            updateRumbleOptions();
        });
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
        HookEventWithCallerPost<ActorWrapper>(
            "Function TAGame.ItemPool_TA.GiveItem",
            [this](const ActorWrapper& caller, void*, const std::string&) {
                onGiveItem(caller);
            });
        HookEventWithCallerPost<ActorWrapper>(
            "Function TAGame.PlayerItemDispenser_TA.Init",
            [this](const ActorWrapper& caller, void*, const std::string&) {
                updateDispenserItemPool(caller);
                updateDispenserMaxTimeTillItem(caller);
            });
        updateDispensers();
        updateRumbleOptions();
    }
    else if (!active && isActive) {
        UnhookEventPost("Function TAGame.ItemPool_TA.GiveItem");
        UnhookEventPost("Function TAGame.PlayerItemDispenser_TA.Init");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string CrazyRumble::GetGameModeName()
{
    return "Crazy Rumble Items";
}


/// <summary>Resets the rumble items values</summary>
void CrazyRumble::ResetItemsValues()
{
    forceMultiplier = 1.f;
    rangeMultiplier = 1.f;
    durationMultiplier = 1.f;

    for (const std::shared_ptr<RumbleWrapper>& rumbleItem : rumbleItems) {
        rumbleItem->Reset();
    }
}


/// <summary>Resets the rumble items values, with an optional multiplier.</summary>
/// <param name="newForceMultiplier">Multiplier of the force</param>
/// <param name="newRangeMultiplier">Multiplier of the range</param>
/// <param name="newDurationMultiplier">Multiplier of the duration</param>
void CrazyRumble::UpdateItemsValues(const float newForceMultiplier, const float newRangeMultiplier,
    const float newDurationMultiplier)
{
    forceMultiplier = newForceMultiplier;
    rangeMultiplier = newRangeMultiplier;
    durationMultiplier = newDurationMultiplier;

    for (const std::shared_ptr<RumbleWrapper>& rumbleItem : rumbleItems) {
        rumbleItem->Multiply(forceMultiplier, rangeMultiplier, durationMultiplier);
    }
}


/// <summary>Updates the rumble options for every car.</summary>
void CrazyRumble::updateRumbleOptions() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    for (CarWrapper car : game.GetCars()) {
        BMCHECK_LOOP(car);

        updateRumbleOptions(car);
    }
}


/// <summary>Sets the current rumble options for the given car.</summary>
/// <param name="car">car to set the rumble options for</param>
void CrazyRumble::updateRumbleOptions(CarWrapper car) const
{
    BMCHECK(car);

    RumblePickupComponentWrapper attachedPickup = car.GetAttachedPickup();
    BMCHECK(attachedPickup);

    const std::string rumbleItemName = attachedPickup.GetPickupName().ToString();
    for (const std::shared_ptr<RumbleWrapper>& rumbleItem : rumbleItems) {
        if (rumbleItem->InternalName == rumbleItemName) {
            rumbleItem->Update(attachedPickup.memory_address);
            // TODO, replicate rumble settings.
            return;
        }
    }

    BM_ERROR_LOG("unknown rumble item {:s}", quote(rumbleItemName));
}
