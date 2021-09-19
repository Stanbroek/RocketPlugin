#pragma once
#include "RocketPluginModule.h"


class RocketPlugin;

class BallMods final : RocketPluginModule
{
    friend RocketPlugin;
public:
    void SetNumBalls(int newNumBalls) const;
    int GetNumBalls() const;
    void SetBallsScale(float newBallsScale) const;
    float GetBallsScale() const;
    void SetMaxBallVelocity(float newMaxBallVelocity) const;
    float GetMaxBallVelocity() const;

protected:

};
