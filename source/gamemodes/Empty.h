#pragma once
#include "../RocketPlugin.h"
#include "../SDK.hpp"


class Empty : public RocketGameMode
{
	private:
		void onTick(ServerWrapper server);
	public:
        Empty(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }
		virtual void RenderOptions();
		virtual bool IsActive();
		virtual void Activate(bool active);
		virtual std::string GetGamemodeName();
};
