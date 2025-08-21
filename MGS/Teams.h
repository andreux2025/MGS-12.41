#pragma once
#include "Utils.h"
#include "Looting.h"
#include "Globals.h"


inline uint8 PickTeam(AFortGameModeAthena* Gamemode, uint8 PreferredTeam, SDK::AFortPlayerControllerAthena* PC)
{
	uint8 Ret = Globals::NextIdx;
	Globals::CurrentPlayersOnTeam++;

	if (Globals::CurrentPlayersOnTeam == Globals::MaxPlayersOnTeam)
	{
		Globals::NextIdx++;
		Globals::CurrentPlayersOnTeam = 0;
	}
	return Ret;
}