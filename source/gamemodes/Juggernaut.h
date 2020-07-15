#pragma once
#include "../RocketPlugin.h"


class Juggernaut : public RocketGameMode
{
	private:
		struct AStatFactory_TA_execOnGoalScored_Parameters
		{
			class AGameEvent_Soccar_TA* InGameEvent;
			class ABall_TA* Ball;
			class UGoal_TA* Goal;
			int ScoreIndex;
			int AssistIndex;
		};

        int juggernautTeam = 1;
        int juggernaut = -1;
        int lastNormalGoals = 0;

		void onGoalScored(ActorWrapper actor);
        void onGiveScorePre(ActorWrapper actor);
        void onGiveScorePost(ActorWrapper actor);
        void initGame(ServerWrapper server);
	public:
        Juggernaut(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }
        virtual void RenderOptions();
		virtual bool IsActive();
		virtual void Activate(bool active);
		virtual std::string GetGamemodeName();
};
