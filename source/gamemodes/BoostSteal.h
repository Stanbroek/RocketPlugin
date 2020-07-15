#pragma once
#include "../RocketPlugin.h"


class BoostSteal : public RocketGameMode
{
	private:
		float boostConversionPercentage = 100.0f;

		void onDemolished(CarWrapper car);
	public:
        BoostSteal(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }
		virtual void RenderOptions();
		virtual bool IsActive();
		virtual void Activate(bool active);
		virtual std::string GetGamemodeName();
};
