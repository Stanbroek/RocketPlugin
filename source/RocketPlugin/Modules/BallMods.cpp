// BallMods.cpp
// Ball mods for Rocket Plugin.
//
// Author:        Stanbroek
// Version:       0.6.8 18/09/21
// BMSDK version: 95
#include "BallMods.h"
#include "RocketPlugin.h"


/// <summary>Sets the number of balls in the current game.</summary>
/// <param name="newNumBalls">The new number of balls</param>
void BallMods::SetNumBalls(const int newNumBalls) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    game.SetTotalGameBalls(newNumBalls);

    const float ballScale = GetBallsScale();
    game.ResetBalls();
    SetBallsScale(ballScale);
}


/// <summary>Gets the number of balls in the current game.</summary>
/// <returns>The number of balls</returns>
int BallMods::GetNumBalls() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, 0);

    return game.GetTotalGameBalls();
}


/// <summary>Sets the scale of the balls in the current game.</summary>
/// <param name="newBallsScale">The new scale of the balls</param>
void BallMods::SetBallsScale(float newBallsScale) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    // The game crashes with negative ball scale.
    if (newBallsScale <= 0) {
        BM_WARNING_LOG("ball scale out of bounds");
        newBallsScale = 1.0f;
    }

    game.ResetBalls();
    for (BallWrapper ball : game.GetGameBalls()) {
        BMCHECK_LOOP(ball);

        ball.SetBallScale(newBallsScale);
    }
}


/// <summary>Gets the scale of the balls in the current game.</summary>
/// <returns>The scale of the balls</returns>
float BallMods::GetBallsScale() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, 1.f);

    BallWrapper ball = game.GetBall();
    BMCHECK_SILENT(ball, 1.f);

    const float ballScale = ball.GetReplicatedBallScale();

    return ballScale > 0 ? ballScale : 1.0f;
}


/// <summary>Sets the max velocity of the balls in the current game.</summary>
/// <param name="newMaxBallVelocity">The new max velocity of the balls</param>
void BallMods::SetMaxBallVelocity(const float newMaxBallVelocity) const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK(game);

    for (BallWrapper ball : game.GetGameBalls()) {
        BMCHECK_LOOP(ball);

        ball.SetMaxLinearSpeed(newMaxBallVelocity);
    }
}


/// <summary>Gets the max velocity of the balls in the current game.</summary>
/// <returns>The max velocity of the balls</returns>
float BallMods::GetMaxBallVelocity() const
{
    ServerWrapper game = Outer()->GetGame();
    BMCHECK_SILENT(game, 6000.f);

    BallWrapper ball = game.GetBall();
    BMCHECK_SILENT(ball, 6000.f);

    return ball.GetMaxLinearSpeed();
}
