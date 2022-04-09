#pragma once
#include "GameModes/RocketGameMode.h"

#include "Networking/RPNetCode.h"


class GhostCars final : public RocketGameMode, public NetworkedModule
{
public:
    GhostCars() { typeIdx = std::make_unique<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
    bool IsActive() override;
    void Activate(bool active) override;
    std::string GetGameModeName() override;
    std::string GetGameModeDescription() override;

    void Receive(PriWrapper sender, const std::string& message) override;

private:
    void setRBCollidesWithChannel(const ObjectWrapper&) const;
    void updateRBCollidesWithChannels() const;
    void resetRBCollidesWithChannels() const;

    void serverUpdateRBCollidesWithChannels(const std::string& prefix) const;
    void clientUpdateRBCollidesWithChannels() const;

    bool enableBallCollision = false;
    bool enableVehicleCollision = false;
};
