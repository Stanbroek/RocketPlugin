// GameModes/SmallCars.cpp
// Should scale cars properly.
//
// Author:        Stanbroek
// Version:       0.0.1 13/07/21
// BMSDK version: 95

#include "SmallCars.h"

#define UPDATE_FLOAT_COMPONENT_(type, component, scale)                 \
    if (b##component) {                                                 \
        TRACE_LOG(#type"::"#component": {}", (scale));                  \
        obj##type.Set##component(obj##type.Get##component() * (scale)); \
    }

#define UPDATE_FLOAT_COMPONENT(type, component) \
    UPDATE_FLOAT_COMPONENT_(type, component, (carScale / oldScale))

#define UPDATE_FLOAT_COMPONENT_REVERSE(type, component) \
    UPDATE_FLOAT_COMPONENT_(type, component, (oldScale / carScale))

#define UPDATE_FLOAT_COMPONENT_SCALE2(type, component, scaleDown, scaleUp)  \
    UPDATE_FLOAT_COMPONENT_(type, component, (carScale / oldScale) < 1 ? std::powf(carScale / oldScale, (scaleDown)) : std::powf(carScale / oldScale, (scaleUp)))

#define UPDATE_FLOAT_COMPONENT_SCALE(type, component, scale)    \
    UPDATE_FLOAT_COMPONENT_SCALE2(type, component, scale, scale)

#define EDIT_FLOAT_COMPONENT(type, component)                           \
    ImGui::Checkbox("##"#component, &b##component);                     \
    ImGui::SameLine();                                                  \
    float f##component = obj##type.Get##component();                    \
    const float delta##component = std::max(f##component / 100, 10.f);  \
    if (ImGui::SliderFloat(#component, &f##component, f##component - delta##component, f##component + delta##component)) {  \
        obj##type.Set##component(f##component);                         \
    }

#define EDIT_FLOAT_COMPONENT_DISABLED(type, component)  \
    ImGui::BeginDisabled();                             \
    EDIT_FLOAT_COMPONENT(type, component)               \
    ImGui::EndDisabled()


/// <summary>Renders the available options for the game mode.</summary>
void SmallCars::RenderOptions()
{
    ServerWrapper game = rocketPlugin->GetGame();
    if (game.IsNull()) {
        ImGui::TextUnformatted("You must be in a game");
        return;
    }

    if (game.GetCars().IsNull() || game.GetCars().Count() < 1) {
        ImGui::TextUnformatted("You must have a car");
        return;
    }

    CarWrapper objCarWrapper = game.GetCars().Get(0);
    if (objCarWrapper.IsNull()) {
        ImGui::TextUnformatted("Your car must be valid");
        return;
    }

    FlipCarComponentWrapper objFlipCarComponentWrapper = objCarWrapper.GetFlipComponent();
    VehicleSimWrapper objVehicleSimWrapper = objCarWrapper.GetVehicleSim();
    BoostWrapper objBoostWrapper = objCarWrapper.GetBoostComponent();
    DodgeComponentWrapper objDodgeComponentWrapper = objCarWrapper.GetDodgeComponent();
    AirControlComponentWrapper objAirControlComponentWrapper = objCarWrapper.GetAirControlComponent();
    JumpComponentWrapper objJumpComponentWrapper = objCarWrapper.GetJumpComponent();
    DoubleJumpComponentWrapper objDoubleJumpComponentWrapper = objCarWrapper.GetDoubleJumpComponent();
    if (objFlipCarComponentWrapper.IsNull() || objVehicleSimWrapper.IsNull() ||
        objBoostWrapper.IsNull() || objDodgeComponentWrapper.IsNull() ||
        objAirControlComponentWrapper.IsNull() || objJumpComponentWrapper.IsNull() ||
        objDoubleJumpComponentWrapper.IsNull()) {
        ImGui::TextUnformatted("Car components must be valid");
        return;
    }

    const float carScale = rocketPlugin->GetCarScale(objCarWrapper.GetPRI());
    float carScaleTmp = carScale;
    if (ImGui::SliderFloat("Car Scale", &carScaleTmp, 0.1f, 2.0f, "%.1fX")) {
        rocketPlugin->Execute([this, player = objCarWrapper.GetPRI(), newCarScale = carScaleTmp](GameWrapper*) {
            rocketPlugin->SetCarScale(player, newCarScale, true);
        });
    }

    if (isActive && oldScale != carScale) {
        TRACE_LOG("{:.3f} -> {:.3f}", oldScale, carScale);
        UPDATE_FLOAT_COMPONENT(CarWrapper, MaxLinearSpeed);
        UPDATE_FLOAT_COMPONENT(CarWrapper, MaxAngularSpeed);
        UPDATE_FLOAT_COMPONENT(FlipCarComponentWrapper, FlipCarImpulse);
        UPDATE_FLOAT_COMPONENT(FlipCarComponentWrapper, FlipCarTorque);
        UPDATE_FLOAT_COMPONENT(FlipCarComponentWrapper, FlipCarTime);
        UPDATE_FLOAT_COMPONENT(VehicleSimWrapper, DriveTorque);
        UPDATE_FLOAT_COMPONENT(VehicleSimWrapper, BrakeTorque);
        UPDATE_FLOAT_COMPONENT(VehicleSimWrapper, StopThreshold);
        UPDATE_FLOAT_COMPONENT(VehicleSimWrapper, IdleBrakeFactor);
        UPDATE_FLOAT_COMPONENT(VehicleSimWrapper, OppositeBrakeFactor);
        UPDATE_FLOAT_COMPONENT(VehicleSimWrapper, OutputThrottle);
        UPDATE_FLOAT_COMPONENT(VehicleSimWrapper, OutputSteer);
        UPDATE_FLOAT_COMPONENT(VehicleSimWrapper, OutputBrake);
        UPDATE_FLOAT_COMPONENT(VehicleSimWrapper, OutputHandbrake);
        UPDATE_FLOAT_COMPONENT_REVERSE(VehicleSimWrapper, SteeringSensitivity);
        UPDATE_FLOAT_COMPONENT(BoostWrapper, BoostConsumptionRate);
        UPDATE_FLOAT_COMPONENT(BoostWrapper, MaxBoostAmount);
        UPDATE_FLOAT_COMPONENT(BoostWrapper, StartBoostAmount);
        UPDATE_FLOAT_COMPONENT(BoostWrapper, CurrentBoostAmount);
        UPDATE_FLOAT_COMPONENT(BoostWrapper, BoostModifier);
        UPDATE_FLOAT_COMPONENT(BoostWrapper, LastBoostAmountRequestTime);
        UPDATE_FLOAT_COMPONENT(BoostWrapper, LastBoostAmount);
        UPDATE_FLOAT_COMPONENT(BoostWrapper, BoostForce);
        UPDATE_FLOAT_COMPONENT(BoostWrapper, MinBoostTime);
        UPDATE_FLOAT_COMPONENT(BoostWrapper, RechargeRate);
        UPDATE_FLOAT_COMPONENT(BoostWrapper, RechargeDelay);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeInputThreshold);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, SideDodgeImpulse);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, SideDodgeImpulseMaxSpeedScale);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, ForwardDodgeImpulse);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, ForwardDodgeImpulseMaxSpeedScale);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, BackwardDodgeImpulse);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, BackwardDodgeImpulseMaxSpeedScale);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, SideDodgeTorque);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, ForwardDodgeTorque);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeTorqueTime);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, MinDodgeTorqueTime);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeZDamping);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeZDampingDelay);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeZDampingUpTime);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeImpulseScale);
        UPDATE_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeTorqueScale);
        UPDATE_FLOAT_COMPONENT(AirControlComponentWrapper, ThrottleForce);
        UPDATE_FLOAT_COMPONENT(AirControlComponentWrapper, DodgeDisableTimeRemaining);
        UPDATE_FLOAT_COMPONENT(AirControlComponentWrapper, ControlScale);
        UPDATE_FLOAT_COMPONENT(AirControlComponentWrapper, AirControlSensitivity);
        UPDATE_FLOAT_COMPONENT(JumpComponentWrapper, MinJumpTime);
        UPDATE_FLOAT_COMPONENT_SCALE2(JumpComponentWrapper, JumpImpulse, 2.5f, 1.5f);
        UPDATE_FLOAT_COMPONENT_SCALE2(JumpComponentWrapper, JumpForce, 2.5f, 1.5f);
        UPDATE_FLOAT_COMPONENT(JumpComponentWrapper, JumpForceTime);
        UPDATE_FLOAT_COMPONENT(JumpComponentWrapper, PodiumJumpForceTime);
        UPDATE_FLOAT_COMPONENT_SCALE2(JumpComponentWrapper, JumpImpulseSpeed, 2.5f, 1.5f);
        UPDATE_FLOAT_COMPONENT_SCALE2(JumpComponentWrapper, JumpAccel, 2.5f, 1.5f);
        UPDATE_FLOAT_COMPONENT_SCALE2(JumpComponentWrapper, MaxJumpHeight, 2.5f, 1.5f);
        UPDATE_FLOAT_COMPONENT(JumpComponentWrapper, MaxJumpHeightTime);
        UPDATE_FLOAT_COMPONENT_SCALE2(DoubleJumpComponentWrapper, ImpulseScale, 2.5f, 1.5f);
        oldScale = carScale;
    }

    /* Car options */
    if (ImGui::CollapsingHeader("Car Components")) {
        ArrayWrapper<CarComponentWrapper> carComponents = objCarWrapper.GetDefaultCarComponents();
        ImGui::Indent();
        ImGui::Text("Car Components: %d", carComponents.Count());
        /* RBActor options */
        EDIT_FLOAT_COMPONENT(CarWrapper, MaxLinearSpeed);
        EDIT_FLOAT_COMPONENT(CarWrapper, MaxAngularSpeed);
        ImGui::Unindent();
    }
    if (ImGui::CollapsingHeader("Flip Car Component")) {
        ImGui::Indent();
        EDIT_FLOAT_COMPONENT(FlipCarComponentWrapper, FlipCarImpulse);
        EDIT_FLOAT_COMPONENT(FlipCarComponentWrapper, FlipCarTorque);
        EDIT_FLOAT_COMPONENT(FlipCarComponentWrapper, FlipCarTime);
        ImGui::Unindent();
    }

    /* Vehicle options */
    if (ImGui::CollapsingHeader("Vehicle Sim")) {
        ImGui::Indent();
        //ArrayWrapper<WheelWrapper> GetWheels();
        EDIT_FLOAT_COMPONENT(VehicleSimWrapper, DriveTorque);
        EDIT_FLOAT_COMPONENT(VehicleSimWrapper, BrakeTorque);
        EDIT_FLOAT_COMPONENT(VehicleSimWrapper, StopThreshold);
        EDIT_FLOAT_COMPONENT(VehicleSimWrapper, IdleBrakeFactor);
        EDIT_FLOAT_COMPONENT(VehicleSimWrapper, OppositeBrakeFactor);
        EDIT_FLOAT_COMPONENT_DISABLED(VehicleSimWrapper, OutputThrottle);
        EDIT_FLOAT_COMPONENT_DISABLED(VehicleSimWrapper, OutputSteer);
        EDIT_FLOAT_COMPONENT_DISABLED(VehicleSimWrapper, OutputBrake);
        EDIT_FLOAT_COMPONENT_DISABLED(VehicleSimWrapper, OutputHandbrake);
        EDIT_FLOAT_COMPONENT(VehicleSimWrapper, SteeringSensitivity);
        ImGui::Unindent();
    }
    if (ImGui::CollapsingHeader("Boost Component")) {
        ImGui::Indent();
        EDIT_FLOAT_COMPONENT(BoostWrapper, BoostConsumptionRate);
        EDIT_FLOAT_COMPONENT(BoostWrapper, MaxBoostAmount);
        EDIT_FLOAT_COMPONENT(BoostWrapper, StartBoostAmount);
        EDIT_FLOAT_COMPONENT(BoostWrapper, CurrentBoostAmount);
        EDIT_FLOAT_COMPONENT(BoostWrapper, BoostModifier);
        EDIT_FLOAT_COMPONENT_DISABLED(BoostWrapper, LastBoostAmountRequestTime);
        EDIT_FLOAT_COMPONENT_DISABLED(BoostWrapper, LastBoostAmount);
        EDIT_FLOAT_COMPONENT(BoostWrapper, BoostForce);
        EDIT_FLOAT_COMPONENT(BoostWrapper, MinBoostTime);
        EDIT_FLOAT_COMPONENT(BoostWrapper, RechargeRate);
        EDIT_FLOAT_COMPONENT(BoostWrapper, RechargeDelay);
        ImGui::Unindent();

    }
    if (ImGui::CollapsingHeader("Dodge Component")) {
        ImGui::Indent();
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeInputThreshold);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, SideDodgeImpulse);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, SideDodgeImpulseMaxSpeedScale);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, ForwardDodgeImpulse);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, ForwardDodgeImpulseMaxSpeedScale);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, BackwardDodgeImpulse);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, BackwardDodgeImpulseMaxSpeedScale);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, SideDodgeTorque);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, ForwardDodgeTorque);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeTorqueTime);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, MinDodgeTorqueTime);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeZDamping);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeZDampingDelay);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeZDampingUpTime);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeImpulseScale);
        EDIT_FLOAT_COMPONENT(DodgeComponentWrapper, DodgeTorqueScale);
        ImGui::Unindent();
    }
    if (ImGui::CollapsingHeader("Air Control Component")) {
        ImGui::Indent();
        EDIT_FLOAT_COMPONENT(AirControlComponentWrapper, ThrottleForce);
        EDIT_FLOAT_COMPONENT(AirControlComponentWrapper, DodgeDisableTimeRemaining);
        EDIT_FLOAT_COMPONENT(AirControlComponentWrapper, ControlScale);
        EDIT_FLOAT_COMPONENT(AirControlComponentWrapper, AirControlSensitivity);
        ImGui::Unindent();
    }
    if (ImGui::CollapsingHeader("Jump Component")) {
        ImGui::Indent();
        EDIT_FLOAT_COMPONENT(JumpComponentWrapper, MinJumpTime);
        EDIT_FLOAT_COMPONENT(JumpComponentWrapper, JumpImpulse);
        EDIT_FLOAT_COMPONENT(JumpComponentWrapper, JumpForce);
        EDIT_FLOAT_COMPONENT(JumpComponentWrapper, JumpForceTime);
        EDIT_FLOAT_COMPONENT(JumpComponentWrapper, PodiumJumpForceTime);
        EDIT_FLOAT_COMPONENT(JumpComponentWrapper, JumpImpulseSpeed);
        EDIT_FLOAT_COMPONENT(JumpComponentWrapper, JumpAccel);
        EDIT_FLOAT_COMPONENT(JumpComponentWrapper, MaxJumpHeight);
        EDIT_FLOAT_COMPONENT(JumpComponentWrapper, MaxJumpHeightTime);
        ImGui::Unindent();
    }
    if (ImGui::CollapsingHeader("Double Jump Component")) {
        ImGui::Indent();
        //void SetJumpImpulse(float newJumpImpulse);
        EDIT_FLOAT_COMPONENT(DoubleJumpComponentWrapper, ImpulseScale);
        ImGui::Unindent();
    }
}


/// <summary>Gets if the game mode is active.</summary>
/// <returns>Bool with if the game mode is active</returns>
bool SmallCars::IsActive()
{
    return isActive;
}


/// <summary>Activates the game mode.</summary>
void SmallCars::Activate(const bool active)
{
    if (active && !isActive) {
        HookEventWithCaller<ServerWrapper>("Function TAGame.Car_TA.PostBeginPlay",
            [this](const ServerWrapper&, void*, const std::string&) {
                oldScale = 1;
            });
    }
    else if (!active && isActive) {
        UnhookEvent("Function TAGame.Car_TA.PostBeginPlay");
        //UnhookEvent("Function TAGame.Car_TA.EventVehicleSetup");
    }

    isActive = active;
}


/// <summary>Gets the game modes name.</summary>
/// <returns>The game modes name</returns>
std::string SmallCars::GetGameModeName()
{
    return "Small Cars (BETA)";
}
