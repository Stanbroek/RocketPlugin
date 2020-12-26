#pragma once
#include "../RocketPlugin.h"


class Drainage final : public RocketGameMode
{
public:
    explicit Drainage(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }

	void RenderOptions() override;
	bool IsActive() override;
	void Activate(bool active) override;
	std::string GetGameModeName() override;
private:
	void onTick(ServerWrapper server, void* params) const;

	bool autoDeplete = false;
	int autoDepleteRate = 10;
};
