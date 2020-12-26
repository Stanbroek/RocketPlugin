#pragma once
#include "RocketPlugin.h"

namespace RLConstants
{
    const RocketPlugin::GameSetting GAME_MODES {
        "GameModes", "Game Modes", 0,
        { "Soccar",                    "Hoops",                         "Snow Day",                  "Rumble",                   "",     "Dropshot",                    "Heatseeker"},
        { "TAGame.GameInfo_Soccar_TA", "TAGame.GameInfo_Basketball_TA", "TAGame.GameInfo_Hockey_TA", "TAGame.GameInfo_Items_TA", "None", "TAGame.GameInfo_Breakout_TA", "TAGame.GameInfo_GodBall_TA" }
    };

    const std::map<std::string, std::string> MAPS {
        { "ARC_P",                      "ARCtagon" },
        { "CHN_Stadium_P",              "Forbidden Temple" },
        { "EuroStadium_Night_P",        "Mannfield (Night)" },
        { "EuroStadium_P",              "Mannfield" },
        { "EuroStadium_Rainy_P",        "Mannfield (Stormy)" },
        { "Farm_Night_P",               "Farmstead (Night)" },
        { "Haunted_TrainStation_P",     "Urban Central (Haunted)" },
        { "HoopsStadium_P",             "Dunk House" },
        { "Labs_CirclePillars_P",       "Pillars" },
        { "Labs_Cosmic_P",              "Cosmic (Old)" },
        { "Labs_Cosmic_V4_P",           "Cosmic (New)" },
        { "Labs_DoubleGoal_P",          "Double Goal (Old)" },
        { "Labs_DoubleGoal_V2_P",       "Double Goal (New)" },
        { "Labs_Octagon_02_P",          "Octagon (New)" },
        { "Labs_Octagon_P",             "Octagon (Old)" },
        { "Labs_Underpass_P",           "Underpass" },
        { "Labs_Utopia_P",              "Utopia Retro" },
        { "NeoTokyo_P",                 "Tokyo Underpass" },
        { "NeoTokyo_Standard_P",        "Neo Tokyo" },
        { "Park_Night_P",               "Beckwith Park (Midnight)" },
        { "Park_P",                     "Beckwith Park" },
        { "Park_Rainy_P",               "Beckwith Park (Stormy)" },
        { "ShatterShot_P",              "Core 707" },
        { "Stadium_Foggy_P",            "DFH Stadium (Stormy)" },
        { "Stadium_P",                  "DFH Stadium" },
        { "Stadium_Winter_P",           "DFH Stadium (Snowy)" },
        { "TrainStation_Dawn_P",        "Urban Central (Dawn)" },
        { "TrainStation_Night_P",       "Urban Central (Night)" },
        { "TrainStation_P",             "Urban Central" },
        { "Underwater_P",               "AquaDome" },
        { "UtopiaStadium_Dusk_P",       "Utopia Coliseum (Dusk)" },
        { "UtopiaStadium_P",            "Utopia Coliseum" },
        { "UtopiaStadium_Snow_P",       "Utopia Coliseum (Snowy)" },
        { "Wasteland_Night_P",          "Badlands (Night)" },
        { "Wasteland_P",                "Badlands" },
        { "arc_standard_p",             "Starbase ARC" },
        { "beach_P",                    "Salty Shores" },
        { "beach_night_p",              "Salty Shores (Night)" },
        { "cs_day_p",                   "Champions Field (Day)" },
        { "cs_hw_p",                    "Rivals Arena" },
        { "cs_p",                       "Champions Field" },
        { "eurostadium_snownight_p",    "Mannfield (Snowy)" },
        { "farm_p",                     "Farmstead" },
        { "music_p",                    "Neon Fields" },
        { "stadium_day_p",              "DFH Stadium (Day)" },
        { "throwbackhockey_p",          "Throwback Stadium (Snowy)" },
        { "throwbackstadium_P",         "Throwback Stadium" },
        { "wasteland_Night_S_P",        "Wasteland (Night)" },
        { "wasteland_s_p",              "Wasteland" }
    };

    const RocketPlugin::GameSetting BOT_DIFFICULTIES {
        "BotDifficulty", "Bot Difficulty", 0,
        { "None",     "Beginner",  "Rookie",   "Pro",        "All-Star", "Unfair" },
        { "BotsNone", "BotsIntro", "BotsEasy", "BotsMedium", "BotsHard", "BotsUnfair" }
    };

    const int HUE_COUNT = 15;
    #define IM_VEC3(R, G, B) ImVec4(R, G, B, 1)
    const std::vector<ImVec4> CUSTOM_COLORS {
        IM_VEC3(0.900000f, 0.900000f, 0.900000f), IM_VEC3(1.000000f, 0.500000f, 0.500000f), IM_VEC3(1.000000f, 0.625000f, 0.500000f), IM_VEC3(1.000000f, 0.812500f, 0.500000f), IM_VEC3(0.937500f, 1.000000f, 0.500000f), IM_VEC3(0.687500f, 1.000000f, 0.500000f), IM_VEC3(0.500000f, 1.000000f, 0.500000f), IM_VEC3(0.500000f, 1.000000f, 0.700000f), IM_VEC3(0.500000f, 0.916667f, 1.000000f), IM_VEC3(0.500000f, 0.691666f, 1.000000f), IM_VEC3(0.500000f, 0.533333f, 1.000000f), IM_VEC3(0.683334f, 0.500000f, 1.000000f), IM_VEC3(0.900000f, 0.500000f, 1.000000f), IM_VEC3(1.000000f, 0.500000f, 0.816666f), IM_VEC3(1.000000f, 0.500000f, 0.583333f),
        IM_VEC3(0.750000f, 0.750000f, 0.750000f), IM_VEC3(1.000000f, 0.350000f, 0.350000f), IM_VEC3(1.000000f, 0.512500f, 0.350000f), IM_VEC3(1.000000f, 0.756250f, 0.350000f), IM_VEC3(0.918750f, 1.000000f, 0.350000f), IM_VEC3(0.593750f, 1.000000f, 0.350000f), IM_VEC3(0.350000f, 1.000000f, 0.350000f), IM_VEC3(0.350000f, 1.000000f, 0.610000f), IM_VEC3(0.350000f, 0.891666f, 1.000000f), IM_VEC3(0.350000f, 0.599166f, 1.000000f), IM_VEC3(0.350000f, 0.393333f, 1.000000f), IM_VEC3(0.588334f, 0.350000f, 1.000000f), IM_VEC3(0.870000f, 0.350000f, 1.000000f), IM_VEC3(1.000000f, 0.350000f, 0.761666f), IM_VEC3(1.000000f, 0.350000f, 0.458334f),
        IM_VEC3(0.600000f, 0.600000f, 0.600000f), IM_VEC3(1.000000f, 0.200000f, 0.200000f), IM_VEC3(1.000000f, 0.400000f, 0.200000f), IM_VEC3(1.000000f, 0.700000f, 0.200000f), IM_VEC3(0.900000f, 1.000000f, 0.200000f), IM_VEC3(0.500000f, 1.000000f, 0.200000f), IM_VEC3(0.200000f, 1.000000f, 0.200000f), IM_VEC3(0.200000f, 1.000000f, 0.520000f), IM_VEC3(0.200000f, 0.866666f, 1.000000f), IM_VEC3(0.200000f, 0.506666f, 1.000000f), IM_VEC3(0.200000f, 0.253333f, 1.000000f), IM_VEC3(0.493334f, 0.200000f, 1.000000f), IM_VEC3(0.840000f, 0.200000f, 1.000000f), IM_VEC3(1.000000f, 0.200000f, 0.706666f), IM_VEC3(1.000000f, 0.200000f, 0.333334f),
        IM_VEC3(0.400000f, 0.400000f, 0.400000f), IM_VEC3(1.000000f, 0.000000f, 0.000000f), IM_VEC3(1.000000f, 0.250000f, 0.000000f), IM_VEC3(1.000000f, 0.625000f, 0.000000f), IM_VEC3(0.875000f, 1.000000f, 0.000000f), IM_VEC3(0.375000f, 1.000000f, 0.000000f), IM_VEC3(0.000000f, 1.000000f, 0.000000f), IM_VEC3(0.000000f, 1.000000f, 0.400000f), IM_VEC3(0.000000f, 0.833333f, 1.000000f), IM_VEC3(0.000000f, 0.383333f, 1.000000f), IM_VEC3(0.000000f, 0.066667f, 1.000000f), IM_VEC3(0.366667f, 0.000000f, 1.000000f), IM_VEC3(0.800000f, 0.000000f, 1.000000f), IM_VEC3(1.000000f, 0.000000f, 0.633333f), IM_VEC3(1.000000f, 0.000000f, 0.166667f),
        IM_VEC3(0.250000f, 0.250000f, 0.250000f), IM_VEC3(0.700000f, 0.000000f, 0.000000f), IM_VEC3(0.700000f, 0.175000f, 0.000000f), IM_VEC3(0.700000f, 0.437500f, 0.000000f), IM_VEC3(0.612500f, 0.700000f, 0.000000f), IM_VEC3(0.262500f, 0.700000f, 0.000000f), IM_VEC3(0.000000f, 0.700000f, 0.000000f), IM_VEC3(0.000000f, 0.700000f, 0.280000f), IM_VEC3(0.000000f, 0.583333f, 0.700000f), IM_VEC3(0.000000f, 0.268333f, 0.700000f), IM_VEC3(0.000000f, 0.046667f, 0.700000f), IM_VEC3(0.256667f, 0.000000f, 0.700000f), IM_VEC3(0.560000f, 0.000000f, 0.700000f), IM_VEC3(0.700000f, 0.000000f, 0.443333f), IM_VEC3(0.700000f, 0.000000f, 0.116667f),
        IM_VEC3(0.150000f, 0.150000f, 0.150000f), IM_VEC3(0.400000f, 0.000000f, 0.000000f), IM_VEC3(0.400000f, 0.100000f, 0.000000f), IM_VEC3(0.400000f, 0.250000f, 0.000000f), IM_VEC3(0.350000f, 0.400000f, 0.000000f), IM_VEC3(0.150000f, 0.400000f, 0.000000f), IM_VEC3(0.000000f, 0.400000f, 0.000000f), IM_VEC3(0.000000f, 0.400000f, 0.160000f), IM_VEC3(0.000000f, 0.333333f, 0.400000f), IM_VEC3(0.000000f, 0.153333f, 0.400000f), IM_VEC3(0.000000f, 0.026667f, 0.400000f), IM_VEC3(0.146667f, 0.000000f, 0.400000f), IM_VEC3(0.320000f, 0.000000f, 0.400000f), IM_VEC3(0.400000f, 0.000000f, 0.253333f), IM_VEC3(0.400000f, 0.000000f, 0.066667f),
        IM_VEC3(0.020000f, 0.020000f, 0.020000f), IM_VEC3(0.200000f, 0.000000f, 0.000000f), IM_VEC3(0.200000f, 0.050000f, 0.000000f), IM_VEC3(0.200000f, 0.125000f, 0.000000f), IM_VEC3(0.175000f, 0.200000f, 0.000000f), IM_VEC3(0.075000f, 0.200000f, 0.000000f), IM_VEC3(0.000000f, 0.200000f, 0.000000f), IM_VEC3(0.000000f, 0.200000f, 0.080000f), IM_VEC3(0.000000f, 0.166667f, 0.200000f), IM_VEC3(0.000000f, 0.076667f, 0.200000f), IM_VEC3(0.000000f, 0.013333f, 0.200000f), IM_VEC3(0.073333f, 0.000000f, 0.200000f), IM_VEC3(0.160000f, 0.000000f, 0.200000f), IM_VEC3(0.200000f, 0.000000f, 0.126667f), IM_VEC3(0.200000f, 0.000000f, 0.033333f)
    };
    const ImVec4 DEFAULT_BLUE_PRIMARY_COLOR = IM_VEC3(0.098f, 0.451f, 1.000f);
    const ImVec4 DEFAULT_BLUE_ACCENT_COLOR =  IM_VEC3(0.522f, 0.808f, 0.902f);
    const ImVec4 DEFAULT_ORANGE_PRIMARY_COLOR = IM_VEC3(0.765f, 0.392f, 0.098f);
    const ImVec4 DEFAULT_ORANGE_ACCENT_COLOR =  IM_VEC3(0.902f, 0.235f, 0.098f);
    #undef IM_VEC3

    const std::vector<RocketPlugin::GameSetting> MUTATORS {
        { "Free Play", "FreePlay", 0,
            { "Disable Free Play", "Enable Free Play" },
            { "",                  "FreePlay" } },
        { "Match Length", "GameTimes", 0,
            { "5 Minutes",  "10 Minutes", "20 Minutes", "Unlimited" },
            { "",           "10Minutes",  "20Minutes",  "UnlimitedTime" } },
        { "Max Score", "GameScores", 0,
            { "Default", "1 Goal", "3 Goals", "5 Goals", "7 Goals", "Unlimited" },
            { "",        "Max1",   "Max3",    "Max5",    "Max7",    "UnlimitedScore" } },
        { "Overtime", "OvertimeRules", 0,
            { "Unlimited", "+5 Max, First Score",        "+5 Max, Random Team" },
            { "",          "Overtime5MinutesFirstScore", "Overtime5MinutesRandom" } },
        { "Max Time Limit", "MaxTimeRules", 0,
            { "Default", "11 Minutes" },
            { "",        "MaxTime11Minutes" } },
        { "Series Length", "MatchGames", 0,
            { "Unlimited", "3 Games", "5 Games", "7 Games" },
            { "",          "3Games",  "5Games",  "7Games" } },
        { "Game Speed", "GameSpeed", 0,
            { "Default", "Slo-mo",         "Time Warp" },
            { "",        "SloMoGameSpeed", "SloMoDistanceBall" } },
        { "Ball Max Speed", "BallMaxSpeed", 0,
            { "Default", "Slow",     "Fast",     "Super Fast" },
            { "",        "SlowBall", "FastBall", "SuperFastBall" } },
        { "Ball Type", "BallType", 0,
            { "Default", "Cube",          "Puck",      "BasketBall",      "HauntedBall",  "BeachBall" },
            { "",        "Ball_CubeBall", "Ball_Puck", "Ball_BasketBall", "Ball_Haunted", "Ball_BeachBall" } },
        { "Ball Gravity", "BallGravity", 0,
            { "Default", "Low",            "High",            "Super High" },
            { "",        "LowGravityBall", "HighGravityBall", "SuperGravityBall" } },
        { "Ball Physics", "BallWeight", 0,
            { "Default", "Light",     "Heavy",     "Super Light",    "CurveBall",  "Beach Ball Curve" },
            { "",        "LightBall", "HeavyBall", "SuperLightBall", "MagnusBall", "MagnusBeachBall" } },
        { "Ball Size", "BallScale", 0,
            { "Default", "Small",     "Medium",     "Large",   "Gigantic" },
            { "",        "SmallBall", "MediumBall", "BigBall", "GiantBall" } },
        { "Ball Bounciness", "BallBounciness", 0,
            { "Default", "Low",           "High",           "Super High" },
            { "",        "LowBounciness", "HighBounciness", "SuperBounciness" } },
        { "Number Of Ball", "MultiBall", 0,
            { "One", "Two",      "Four",      "Six" },
            { "",    "TwoBalls", "FourBalls", "SixBalls" } },
        { "Boosts Amount", "Boosters", 0,
            { "Default", "No Boost",  "Unlimited",        "Recharge (slow)", "Recharge (fast)" },
            { "",        "NoBooster", "UnlimitedBooster", "SlowRecharge",    "RapidRecharge" } },
        { "Rumble", "Items", 0,
            { "None", "Default",   "Slow",          "Civilized",                 "Destruction Derby",        "Spring Loaded",    "Spikes Only",     "Rugby",          "Haunted Ball Beam" },
            { "",     "ItemsMode", "ItemsModeSlow", "ItemsModeBallManipulators", "ItemsModeCarManipulators", "ItemsModeSprings", "ItemsModeSpikes", "ItemsModeRugby", "ItemsModeHauntedBallBeam" } },
        { "Boost Strength", "BoosterStrengths", 0,
            { "1X", "1.5X",                "2X",                "10X" },
            { "",   "BoostMultiplier1_5x", "BoostMultiplier2x", "BoostMultiplier10x" } },
        { "Gravity", "Gravity", 0,
            { "Default", "Low",        "High",        "Super High",   "Reverse" },
            { "",        "LowGravity", "HighGravity", "SuperGravity", "ReverseGravity" } },
        { "Demolish", "Demolish", 0,
            { "Default", "Disabled",   "Friendly Fire", "On Contact",             "On Contact (FF)" },
            { "",        "NoDemolish", "DemolishAll",   "AlwaysDemolishOpposing", "AlwaysDemolish" } },
        { "Respawn Time", "RespawnTime", 0,
            { "3 Seconds", "2 Seconds",         "1 Second",          "Disable Goal Reset" },
            { "",          "TwoSecondsRespawn", "OneSecondsRespawn", "DisableGoalDelay" } },
        { "Bot Loadouts", "BotLoadouts", 0,
            { "Default", "Random" },
            { "",        "RandomizedBotLoadouts" } },
        { "Audio", "AudioRules", 0,
            { "Default", "Haunted" },
            { "",        "HauntedAudio" } },
        { "Game Event", "GameEventRules", 0,
            { "Default", "Haunted",               "Rugby" },
            { "",        "HauntedGameEventRules", "RugbyGameEventRules" } },
    };
}
