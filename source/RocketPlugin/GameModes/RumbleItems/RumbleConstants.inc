#pragma once
#include "RumbleItems.h"

namespace RumbleConstants
{
	const BallCarSpringWrapper boot = {
		{
			{
				{
			        "Boot",
			        "CarSpring",
			        true,
			        99.f
				},
				false,
				true,
				true,
				false,
				2300.f
			},
			550000.f,
			400000.f,
			{ 2000.f, 0.f, 0.f },
			1.f,
			3500.f,
			0.f,
			30.f,
			3.f,
			50000.f
		}
	};
	const BoostOverrideWrapper disruptor = {
		{
			{
			    "Disruptor",
			    "EnemyBooster",
			    true,
			    5.f
			},
			false,
			true,
			true,
			false,
			40000.f
		}
	};
	const BallFreezeWrapper freezer = {
		{
			{
			    "Freezer",
			    "BallFreeze",
			    true,
			    4.f
			},
			true,
			false,
			false,
			false,
			2300.f
		},
		false,
		.5f,
		0.f
	};
	const GrapplingHookWrapper grapplingHook = {
		{
			{
			    "Grappling Hook",
			    "GrapplingHook",
			    true,
			    6.f
			},
			true,
			false,
			false,
			false,
			2800.f
		},
		80000.f,
		900000.f,
		3500.f,
		3300.f
	};
	const BallCarSpringWrapper haymaker = {
		{
			{
				{
			        "Haymaker",
			        "BallSpring",
			        true,
			        99.f
				},
				true,
				false,
				false,
				false,
				2300.f
			},
			80000.f,
			0.f,
			{ 0, 0, 0 },
			1.f,
			3500.f,
			0.f,
			30.f,
			3.f,
			50000.f
		},
	};
	const GravityWrapper magnetizer = {
		{
			"Magnetizer",
			"BallMagnet",
			true,
			6.f
		},
		55000.f,
		1000.f,
		false
	};
	const BattarangWrapper plunger = {
		{
			{
				{
					{
			            "Plunger",
			            "BallLasso",
			            true,
			            99.f
					},
					true,
					false,
					false,
					false,
					1800.f
				},
				0.f,
				0.f,
				{ 0, 0, 0 },
				-1.f,
				3000.f,
				-20.f,
				30.f,
				1.f,
				0.f
			},
		},
		0.f
	};
	const HitForceWrapper powerhitter = {
		{
			"Powerhitter",
			"Powerhitter",
			true,
			12.f
		},
		true,
		true,
		true,
		1.5f,
		30.f
	};
	const VelcroWrapper spikes = {
		{
			"Spikes",
			"BallVelcro",
			true,
			12.f
		},
		4.5f,
		0.6f,
		1200.f,
		0.6f,
		0.f,
		0.f,
	};
	const SwapperWrapper swapper = {
		{
			{
			    "Swapper",
			    "EnemySwapper",
			    true,
			    0.75f
			},
			false,
			true,
			true,
			false,
			10000.f
		}
	};
	const TornadoWrapper tornado = {
		{
			"Tornado",
			"Tornado",
			true,
			8.f
		},
		800.f,
		400.f,
		7.f,
		5.f,
		{ 1.f, 1.f, 1.f },
		{ 1.f, 1.f, 1.f },
		3000.f,
		3.f,
		5.f
	};
	const HauntedWrapper haunted = {
		{
			{
			    "Haunted",
			    "HauntedBallBeam",
			    true,
			    4.f
			},
			3500.f,
			1200.f,
			false
		}
	};
	const RugbyWrapper rugby = {
		{
			"Rugby",
			"RugbySpikes",
			true,
			100000000000000000.f
		}
	};
}
