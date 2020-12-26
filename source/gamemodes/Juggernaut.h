#pragma once
#include "../RocketPlugin.h"


class Juggernaut final : public RocketGameMode
{
public:
    explicit Juggernaut(RocketPlugin* rp) : RocketGameMode(rp) { _typeid = std::make_shared<std::type_index>(typeid(*this)); }

    void RenderOptions() override;
	bool IsActive() override;
	void Activate(bool active) override;
	std::string GetGameModeName() override;

private:
	void onGoalScored(ActorWrapper actor);
	void onGiveScorePre(ActorWrapper actor);
	void onGiveScorePost(ActorWrapper actor) const;
	void initGame();

	int juggernautTeam = 1;
	int juggernaut = -1;
	int lastNormalGoals = 0;
};
