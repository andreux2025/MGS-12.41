#pragma once

namespace Globals {
	bool bUseRandomPorts = false;
	bool bEventEnabled = false;
	//int NumPlayerBotsOnPlayerJoin = 99;
	bool bPlayerBotsEnabled = false;
	bool bPlayerBotsPerformanceModeEnabled = false; // reduces the tick rate for the playerbots to only tick every other tick (ONLY USE IF UR TESTING CORE GAMEPLAY NOT BOTS)
	int MaxBotsToSpawn = 100;
	bool SiphonEnabled = false;
	int MaxPlayersOnTeam = 1;
	int CurrentPlayersOnTeam = 0;
	uint8 NextIdx = 3;
	auto Bosses = true;
	int LastTime = 0;
	bool BotsEnabled = true;
	bool LateGame = false;
	int TimesCalled = 0;
}