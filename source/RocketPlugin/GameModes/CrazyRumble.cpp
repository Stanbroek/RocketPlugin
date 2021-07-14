// GameModes/CrazyRumble.cpp
// A crazy rumble customizer game mode for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.3.1 16/04/21
// BMSDK version: 95

#include "CrazyRumble.h"

#include "RumbleItems/RumbleConstants.inc"


/// <summary>Renders the available options for the game mode.</summary>
void CrazyRumble::RenderOptions()
{
    if (refreshRumbleConstants) {
        if (!rumbleConstantsRequest.valid()) {
            rumbleConstantsRequest = rocketPlugin->ConstantsConfig->RequestRumbleConstants();
        }
        if (rumbleConstantsRequest._Is_ready()) {
            refreshRumbleConstants = false;
            const auto& [successful, data] = rumbleConstantsRequest.get();
            if (!successful || !RPConfig::ParseRumbleItems(data, this)) {
                rocketPlugin->Execute([this](GameWrapper*) {
                    WARNING_LOG("could not load rumble items");
                    InitializeItemsValues();
                });
            }
#ifdef DEBUG
            else {
                rocketPlugin->Execute([this](GameWrapper*) {
                    TRACE_LOG("loading rumble items from game files.");
                    InitializeItemsValues();
                });
            }
#endif
        }
    }

    bool shouldUpdateCars = false;

    ImGui::Text("Presets:");
    ImGui::Spacing();

    if (ImGui::Button("Bumper Cars")) {
        ResetItemsValues();
        powerhitter.DemolishCars = false;
        powerhitter.ActivationDuration = 300.0f;
        shouldUpdateCars = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Target Everything")) {
        ResetItemsValues();
        boot.CanTargetBall = true;
        boot.CanTargetCars = true;
        boot.CanTargetEnemyCars = true;
        boot.CanTargetTeamCars = true;
        disruptor.CanTargetBall = true;
        disruptor.CanTargetCars = true;
        disruptor.CanTargetEnemyCars = true;
        disruptor.CanTargetTeamCars = true;
        freezer.CanTargetBall = true;
        freezer.CanTargetCars = true;
        freezer.CanTargetEnemyCars = true;
        freezer.CanTargetTeamCars = true;
        grapplingHook.CanTargetBall = true;
        grapplingHook.CanTargetCars = true;
        grapplingHook.CanTargetEnemyCars = true;
        grapplingHook.CanTargetTeamCars = true;
        haymaker.CanTargetBall = true;
        haymaker.CanTargetCars = true;
        haymaker.CanTargetEnemyCars = true;
        haymaker.CanTargetTeamCars = true;
        plunger.CanTargetBall = true;
        plunger.CanTargetCars = true;
        plunger.CanTargetEnemyCars = true;
        plunger.CanTargetTeamCars = true;
        swapper.CanTargetBall = true;
        swapper.CanTargetCars = true;
        swapper.CanTargetEnemyCars = true;
        swapper.CanTargetTeamCars = true;
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
        rocketPlugin->Execute([this](GameWrapper*) {
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

    bool shouldUpdateItemPool = ImGui::Checkbox("##EnableBoot", &boot.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Boot")) {
        shouldUpdateCars |= boot.Render();
    }
    shouldUpdateItemPool |= ImGui::Checkbox("##EnableDisruptor", &disruptor.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Disruptor")) {
        shouldUpdateCars |= disruptor.Render();
    }
    shouldUpdateItemPool |= ImGui::Checkbox("##EnableFreezer", &freezer.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Freezer")) {
        shouldUpdateCars |= freezer.Render();
    }
    shouldUpdateItemPool |= ImGui::Checkbox("##EnableGrapplingHook", &grapplingHook.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Grappling Hook")) {
        shouldUpdateCars |= grapplingHook.Render();
    }
    shouldUpdateItemPool |= ImGui::Checkbox("##EnableHaymaker", &haymaker.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Haymaker")) {
        shouldUpdateCars |= haymaker.Render();
    }
    shouldUpdateItemPool |= ImGui::Checkbox("##EnableMagnetizer", &magnetizer.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Magnetizer")) {
        shouldUpdateCars |= magnetizer.Render();
    }
    shouldUpdateItemPool |= ImGui::Checkbox("##EnablePlunger", &plunger.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Plunger")) {
        shouldUpdateCars |= plunger.Render();
    }
    shouldUpdateItemPool |= ImGui::Checkbox("##EnablePowerhitter", &powerhitter.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Powerhitter")) {
        shouldUpdateCars |= powerhitter.Render();
    }
    shouldUpdateItemPool |= ImGui::Checkbox("##EnableSpikes", &spikes.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Spikes")) {
        shouldUpdateCars |= spikes.Render();
    }
    shouldUpdateItemPool |= ImGui::Checkbox("##EnableSwapper", &swapper.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Swapper")) {
        shouldUpdateCars |= swapper.Render();
    }
    shouldUpdateItemPool |= ImGui::Checkbox("##EnableTornado", &tornado.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Tornado")) {
        shouldUpdateCars |= tornado.Render();
    }
    shouldUpdateItemPool |= ImGui::Checkbox("##EnableHaunted", &haunted.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Haunted")) {
        shouldUpdateCars |= haunted.Render();
    }
    shouldUpdateItemPool |= ImGui::Checkbox("##EnableRugby", &rugby.Enabled);
    ImGui::SameLine();
    if (ImGui::CollapsingHeader("Rugby")) {
        shouldUpdateCars |= rugby.Render();
    }

    rocketPlugin->Execute([this, shouldUpdateItemPool](GameWrapper*) {
        updateDispensers(false, shouldUpdateItemPool);
    });
    rocketPlugin->Execute([this, shouldUpdateCars](GameWrapper*) {
        updateRumbleOptions(shouldUpdateCars);
    });
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
        HookEventWithCallerPost<ActorWrapper>("Function TAGame.ItemPool_TA.GiveItem",
            [this](const ActorWrapper& caller, void*, const std::string&) {
                onGiveItem(caller);
            });
        HookEventWithCallerPost<ActorWrapper>("Function TAGame.PlayerItemDispenser_TA.Init",
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


/// <summary>Initializes default rumble items values.</summary>
void CrazyRumble::InitializeItemsValues()
{
    boot = RumbleConstants::boot;
    disruptor = RumbleConstants::disruptor;
    freezer = RumbleConstants::freezer;
    grapplingHook = RumbleConstants::grapplingHook;
    haymaker = RumbleConstants::haymaker;
    magnetizer = RumbleConstants::magnetizer;
    plunger = RumbleConstants::plunger;
    powerhitter = RumbleConstants::powerhitter;
    spikes = RumbleConstants::spikes;
    swapper = RumbleConstants::swapper;
    tornado = RumbleConstants::tornado;
    haunted = RumbleConstants::haunted;
    rugby = RumbleConstants::rugby;
}


void CrazyRumble::ResetItemsValues()
{
    forceMultiplier = 1.f;
    rangeMultiplier = 1.f;
    durationMultiplier = 1.f;

    boot.Reset(RumbleConstants::boot);
    disruptor.Reset(RumbleConstants::disruptor);
    freezer.Reset(RumbleConstants::freezer);
    grapplingHook.Reset(RumbleConstants::grapplingHook);
    haymaker.Reset(RumbleConstants::haymaker);
    magnetizer.Reset(RumbleConstants::magnetizer);
    plunger.Reset(RumbleConstants::plunger);
    powerhitter.Reset(RumbleConstants::powerhitter);
    spikes.Reset(RumbleConstants::spikes);
    swapper.Reset(RumbleConstants::swapper);
    tornado.Reset(RumbleConstants::tornado);
    haunted.Reset(RumbleConstants::haunted);
    rugby.Reset(RumbleConstants::rugby);
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

    boot.Multiply(RumbleConstants::boot, forceMultiplier, rangeMultiplier, durationMultiplier);
    disruptor.Multiply(RumbleConstants::disruptor, forceMultiplier, rangeMultiplier, durationMultiplier);
    freezer.Multiply(RumbleConstants::freezer, forceMultiplier, rangeMultiplier, durationMultiplier);
    grapplingHook.Multiply(RumbleConstants::grapplingHook, forceMultiplier, rangeMultiplier, durationMultiplier);
    haymaker.Multiply(RumbleConstants::haymaker, forceMultiplier, rangeMultiplier, durationMultiplier);
    magnetizer.Multiply(RumbleConstants::magnetizer, forceMultiplier, rangeMultiplier, durationMultiplier);
    plunger.Multiply(RumbleConstants::plunger, forceMultiplier, rangeMultiplier, durationMultiplier);
    powerhitter.Multiply(RumbleConstants::powerhitter, forceMultiplier, rangeMultiplier, durationMultiplier);
    spikes.Multiply(RumbleConstants::spikes, forceMultiplier, rangeMultiplier, durationMultiplier);
    swapper.Multiply(RumbleConstants::swapper, forceMultiplier, rangeMultiplier, durationMultiplier);
    tornado.Multiply(RumbleConstants::tornado, forceMultiplier, rangeMultiplier, durationMultiplier);
    haunted.Multiply(RumbleConstants::haunted, forceMultiplier, rangeMultiplier, durationMultiplier);
    rugby.Multiply(RumbleConstants::rugby, forceMultiplier, rangeMultiplier, durationMultiplier);
}


/// <summary>Updates the rumble options for every car.</summary>
void CrazyRumble::updateRumbleOptions(const bool update) const
{
    if (!update) {
        return;
    }

    ServerWrapper game = gameWrapper->GetGameEventAsServer();
    if (game.IsNull()) {
        ERROR_LOG("could not get the game");
        return;
    }

    for (CarWrapper car : game.GetCars()) {
        if (car.IsNull()) {
            ERROR_LOG("could not get the car");
            continue;
        }

        updateRumbleOptions(car);
    }
}


/// <summary>Sets the current rumble options for the given car.</summary>
/// <param name="car">car to set the rumble options for</param>
void CrazyRumble::updateRumbleOptions(CarWrapper car) const
{
    if (car.IsNull()) {
        ERROR_LOG("could not get the car");
        return;
    }

    RumblePickupComponentWrapper rumbleItem = car.GetAttachedPickup();
    if (rumbleItem.IsNull()) {
        ERROR_LOG("could not get rumble item");
        return;
    }

    const std::string rumbleItemName = rumbleItem.GetPickupName().ToString();
    if (rumbleItemName == "BallMagnet") {
        magnetizer.Update(rumbleItem.memory_address);
    }
    else if (rumbleItemName == "CarSpring") {
        boot.Update(rumbleItem.memory_address);
    }
    else if (rumbleItemName == "BallSpring") {
        haymaker.Update(rumbleItem.memory_address);
    }
    else if (rumbleItemName == "Tornado") {
        tornado.Update(rumbleItem.memory_address);
    }
    else if (rumbleItemName == "GrapplingHook") {
        grapplingHook.Update(rumbleItem.memory_address);
    }
    else if (rumbleItemName == "Powerhitter") {
        powerhitter.Update(rumbleItem.memory_address);
    }
    else if (rumbleItemName == "EnemyBooster") {
        disruptor.Update(rumbleItem.memory_address);
    }
    else if (rumbleItemName == "BallLasso") {
        plunger.Update(rumbleItem.memory_address);
    }
    else if (rumbleItemName == "BallVelcro") {
        spikes.Update(rumbleItem.memory_address);
    }
    else if (rumbleItemName == "BallFreeze") {
        freezer.Update(rumbleItem.memory_address);
    }
    else if (rumbleItemName == "EnemySwapper") {
        swapper.Update(rumbleItem.memory_address);
    }
    else if (rumbleItemName == "HauntedBallBeam") {
        haunted.Update(rumbleItem.memory_address);
    }
    else if (rumbleItemName == "RugbySpikes") {
        rugby.Update(rumbleItem.memory_address);
    }
    else {
        ERROR_LOG("unknown rumble item {}", quote(rumbleItemName));
    }

    // TODO, replicate rumble settings.
}
