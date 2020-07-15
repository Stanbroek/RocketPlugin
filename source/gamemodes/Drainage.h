#pragma once
#include "../RocketPlugin.h"


class Drainage : public RocketGameMode
{
	private:
		bool autoDeplete = false;
		int autoDepleteRate = 10;

		void onTick(ServerWrapper server, void* params);
	public:
        Drainage(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }
		virtual void RenderOptions();
		virtual bool IsActive();
		virtual void Activate(bool active);
		virtual std::string GetGamemodeName();
};
