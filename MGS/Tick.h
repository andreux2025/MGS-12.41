#pragma once
#include "Utils.h"
#include "Bosses.h"
#include "Gamemode.h"
#include "PlayerBots.h"

inline void (*TickFlushOG)(UNetDriver* Driver, float DeltaSeconds);
inline void TickFlush(UNetDriver* Driver, float DeltaSeconds)
{
	static void (*ServerReplicateActors)(void*) = decltype(ServerReplicateActors)(InSDKUtils::GetImageBase() + 0x1023F60);
	if (ServerReplicateActors && Driver->ReplicationDriver && Driver->ClientConnections.Num() > 0)
		ServerReplicateActors(Driver->ReplicationDriver);

	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
	auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

	if (GameState->WarmupCountdownEndTime - Statics->GetTimeSeconds(UWorld::GetWorld()) <= 0 && GameState->GamePhase == EAthenaGamePhase::Warmup)
	{
		StartAircraftPhase(GameMode, 0);
	}

	if (Globals::bPlayerBotsEnabled && !Globals::bEventEnabled) {
		static bool InitialisedPlayerStarts = false;
		if (!InitialisedPlayerStarts)
		{
			UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);
			InitialisedPlayerStarts = true;
		}
		
		if (((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->GamePhase < EAthenaGamePhase::Aircraft && GameMode->AlivePlayers.Num() > 0 && (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num()) < GameMode->GameSession->MaxPlayers && GameMode->AliveBots.Num() < Globals::MaxBotsToSpawn)
		{
			auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			auto DR = 60.f;

			GameState->WarmupCountdownEndTime = TS + DR;
			GameMode->WarmupCountdownDuration = DR;
			GameState->WarmupCountdownStartTime = TS;
			GameMode->WarmupEarlyCountdownDuration = DR;

			if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.075f))
			{
				AActor* SpawnLocator = PlayerStarts[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, PlayerStarts.Num() - 1)];

				if (SpawnLocator)
				{
					SpawnPlayerBots(SpawnLocator);
				}
			}
		}
	}
	
	if (Globals::Bosses && !Globals::bEventEnabled)
	{
		TickBots();
	}

	if (Globals::bPlayerBotsEnabled && !Globals::bEventEnabled) {
		PlayerBotTick();
	}


	return TickFlushOG(Driver, DeltaSeconds);
}


inline float GetMaxTickRate() 
{
	return 30.f;
}
