#pragma once
#include "Utils.h"
#include "Looting.h"
#include "Globals.h"
#include "Quests.h"
#include "PlayerBots.h"

static inline UClass* Starter;
static inline UObject* JerkyLoader;

inline bool (*ReadyToStartMatchOG)(AFortGameModeAthena* GameMode);
inline bool ReadyToStartMatch(AFortGameModeAthena* GameMode)
{
	ReadyToStartMatchOG(GameMode);

	static bool StartedMatch = false;
	static bool ServerListening = false;

	AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

	if (!StartedMatch) {
		static bool LoadedPlaylist = false;
		if (!LoadedPlaylist) {
			UFortPlaylistAthena* Playlist;
			if (Globals::bEventEnabled) {
				Playlist = UObject::FindObject<UFortPlaylistAthena>("Playlist_Music_High.Playlist_Music_High");
			}
			else {
				Playlist = UObject::FindObject<UFortPlaylistAthena>("Playlist_DefaultSolo.Playlist_DefaultSolo");
			}

			if (!Playlist) {
				Log("Could not find playlist!");
				return false;
			}
			else {
				Log("Found Playlist!");
			}

			GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
			GameState->CurrentPlaylistInfo.MarkArrayDirty();
			GameState->OnRep_CurrentPlaylistInfo();

			GameMode->CurrentPlaylistName = Playlist->PlaylistName;
			//GameMode->bAlwaysDBNO = Playlist->MaxSquadSize > 1; might be our issue mr tech
			GameMode->WarmupRequiredPlayerCount = 1;

			Globals::NextIdx = Playlist->DefaultFirstTeam;
			Globals::MaxPlayersOnTeam = Playlist->MaxSquadSize;

			if (Globals::MaxPlayersOnTeam > 1)
			{
				GameMode->bDBNOEnabled = true;
				GameMode->bAlwaysDBNO = true;
				GameState->bDBNODeathEnabled = true;
				GameState->SetIsDBNODeathEnabled(true);
			}

			GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
			GameState->CurrentPlaylistId = Playlist->PlaylistId;
			GameState->CurrentPlaylistInfo.MarkArrayDirty();

			GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;
			GameMode->GameSession->MaxSpectators = 0;
			GameMode->GameSession->MaxPartySize = Playlist->MaxSquadSize;
			GameMode->GameSession->MaxSplitscreensPerConnection = 2;
			GameMode->GameSession->bRequiresPushToTalk = false;
			GameMode->GameSession->SessionName = UKismetStringLibrary::Conv_StringToName(FString(L"GameSession"));

			auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			auto DR = 60.f;

			GameState->WarmupCountdownEndTime = TS + DR;
			GameMode->WarmupCountdownDuration = DR;
			GameState->WarmupCountdownStartTime = TS;
			GameMode->WarmupEarlyCountdownDuration = DR;

			GameState->CurrentPlaylistId = Playlist->PlaylistId;
			GameState->OnRep_CurrentPlaylistId();

			GameState->bGameModeWillSkipAircraft = Playlist->bSkipAircraft;
			GameState->CachedSafeZoneStartUp = Playlist->SafeZoneStartUp;
			GameState->AirCraftBehavior = Playlist->AirCraftBehavior;
			GameState->OnRep_Aircraft();

			if (Globals::bEventEnabled) {
				Log("Event is loaded!");

				TArray<AActor*> BuildingFoundations;
				UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingFoundation::StaticClass(), &BuildingFoundations);


				for (AActor*& BuildingFoundation : BuildingFoundations) {
					ABuildingFoundation* Foundation = (ABuildingFoundation*)BuildingFoundation;
					if (!Foundation) continue;

					if (BuildingFoundation->GetName().contains("Jerky") ||
						BuildingFoundation->GetName().contains("LF_Athena_POI_19x19")) {
						ShowFoundation((ABuildingFoundation*)BuildingFoundation);
					}
				}

				BuildingFoundations.Free();
			}
			else {
				// load the event stage (travis scott foundation and shit)
				ABuildingFoundation* BuildingFound = StaticFindObject<ABuildingFoundation>(L"/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.LF_Athena_POI_19x19_2");
				ShowFoundation((ABuildingFoundation*)BuildingFound);
			}

			Log("Setup Playlist!");
			LoadedPlaylist = true;
		}

		if (!GameState->MapInfo) {
			//Log("Map isnt fully loaded yet, ReadyToStartMatch return false!"); //this spams so why log 
			return false;
		}

		static bool InitialisedBots = false;

		if (!InitialisedBots) {
			if (Globals::bEventEnabled) {
				InitialisedBots = true;
			} else if (auto BotManager = (UFortServerBotManagerAthena*)UGameplayStatics::SpawnObject(UFortServerBotManagerAthena::StaticClass(), GameMode))
			{
				GameMode->ServerBotManager = BotManager;
				BotManager->CachedGameState = GameState;
				BotManager->CachedGameMode = GameMode;

				BotMutator = SpawnActor<AFortAthenaMutator_Bots>({});
				BotManager->CachedBotMutator = BotMutator;
				BotMutator->CachedGameMode = GameMode;
				BotMutator->CachedGameState = GameState;

				AAthenaAIDirector* Director = SpawnActor<AAthenaAIDirector>({});
				GameMode->AIDirector = Director;
				//Director->Activate();

				AFortAIGoalManager* GoalManager = SpawnActor<AFortAIGoalManager>({});
				GameMode->AIGoalManager = GoalManager;

				Globals::BotsEnabled = BotMutator;
				InitialisedBots = true;
				Log("Initialised Bots!");
			}
			else
			{
				Log("BotManager is nullptr!");
			}
		}

		if (!ServerListening) {
			ServerListening = true;

			if (Globals::bPlayerBotsEnabled) {
				CIDs = GetAllObjectsOfClass<UAthenaCharacterItemDefinition>();
				Pickaxes = GetAllObjectsOfClass<UAthenaPickaxeItemDefinition>();
				Backpacks = GetAllObjectsOfClass<UAthenaBackpackItemDefinition>();
				Gliders = GetAllObjectsOfClass<UAthenaGliderItemDefinition>();
				Contrails = GetAllObjectsOfClass<UAthenaSkyDiveContrailItemDefinition>();
				Dances = GetAllObjectsOfClass<UAthenaDanceItemDefinition>();
			}
			
			GameState->OnRep_CurrentPlaylistId();
			GameState->OnRep_CurrentPlaylistInfo();

			if (Globals::bEventEnabled) {
				Starter = StaticLoadObject<UClass>("/CycloneJerky/Gameplay/BP_Jerky_Scripting.BP_Jerky_Scripting_C");
				JerkyLoader = UObject::FindObject<UObject>("BP_Jerky_Loader_C JerkyLoaderLevel.JerkyLoaderLevel.PersistentLevel.BP_Jerky_Loader_2");
			}

			auto PhantomBooth = StaticLoadObject<UClass>("/Game/Athena/Items/EnvironmentalItems/HidingProps/Props/B_HidingProp_PhantomBooth.B_HidingProp_PhantomBooth_C");
			if (PhantomBooth) {
				Log("PhantomBooth Exists");
				auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
				TArray<AActor*> PhantomBoothSpawners;
				Statics->GetAllActorsOfClass(UWorld::GetWorld(), PhantomBooth, &PhantomBoothSpawners);
				for (size_t i = 0; i < PhantomBoothSpawners.Num(); i++)
				{
					SpawnActor<AActor>(PhantomBoothSpawners[i]->K2_GetActorLocation(), PhantomBoothSpawners[i]->K2_GetActorRotation(), PhantomBoothSpawners[i], PhantomBooth);
					Log("Spawned a booth!");
				}
			}
			else {
				Log("PhantomBooth does not exist!");
			}

			FName NetDriverDef = UKismetStringLibrary::Conv_StringToName(FString(L"GameNetDriver"));

			UNetDriver* NetDriver = CreateNetDriver(UEngine::GetEngine(), UWorld::GetWorld(), NetDriverDef);
			NetDriver->NetDriverName = NetDriverDef;
			NetDriver->World = UWorld::GetWorld();

			FString Error;
			FURL url = FURL();
			url.Port = 7777;
			if (Globals::bUseRandomPorts)
				url.Port = 7000 + (std::rand() % (9999 - 7000 + 1));

			InitListen(NetDriver, UWorld::GetWorld(), url, false, Error);
			SetWorld(NetDriver, UWorld::GetWorld());

			UWorld::GetWorld()->NetDriver = NetDriver;

			for (size_t i = 0; i < UWorld::GetWorld()->LevelCollections.Num(); i++) {
				UWorld::GetWorld()->LevelCollections[i].NetDriver = NetDriver;
			}

			SetWorld(UWorld::GetWorld()->NetDriver, UWorld::GetWorld());

			for (int i = 0; i < GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels.Num(); i++)
			{
				FVector Loc{};
				FRotator Rot{};
				bool bSuccess = false;
				((ULevelStreamingDynamic*)ULevelStreamingDynamic::StaticClass()->DefaultObject)->LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels[i], Loc, Rot, &bSuccess, FString());
				FAdditionalLevelStreamed NewLevel{};
				NewLevel.LevelName = GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels[i].ObjectID.AssetPathName;
				NewLevel.bIsServerOnly = false;
				GameState->AdditionalPlaylistLevelsStreamed.Add(NewLevel);
			}

			for (int i = 0; i < GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly.Num(); i++)
			{
				FVector Loc{};
				FRotator Rot{};
				bool bSuccess = false;
				((ULevelStreamingDynamic*)ULevelStreamingDynamic::StaticClass()->DefaultObject)->LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly[i], Loc, Rot, &bSuccess, FString());
				FAdditionalLevelStreamed NewLevel{};
				NewLevel.LevelName = GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly[i].ObjectID.AssetPathName;
				NewLevel.bIsServerOnly = true;
				GameState->AdditionalPlaylistLevelsStreamed.Add(NewLevel);
			}
			GameState->OnRep_AdditionalPlaylistLevelsStreamed();
			GameState->OnFinishedStreamingAdditionalPlaylistLevel();

			GameMode->bWorldIsReady = true;

			//std::string Port = std::to_string(url.Port);
			Log("listening on Port: " + std::to_string(url.Port));
			SetConsoleTitleA(("MGS | Listening: " + std::to_string(url.Port)).c_str());
		}

		/*if (UWorld::GetWorld()->NetDriver->ClientConnections.Num() > 0) {
			std::cout << "Return true\n";
			return true;
		}*/
	}

	/*if (GameMode->bDelayedStart)
	{
		return false;
	}*/

	//std::cout << GameMode->GetMatchState().ToString() << "\n";

	//if (GameMode->GetMatchState().ToString() == "WaitingToStart")
	//{
		//if (GameMode->NumPlayers + GameMode->NumBots > 0)
		//{
		//	Log("Enough Players/Bots, Starting match!");
			//return true;
		//}
	//}

	if (GameState->PlayersLeft > 0)
	{
		return true;
	}
	else
	{
		auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
		auto DR = 60.f;

		GameState->WarmupCountdownEndTime = TS + DR;
		GameMode->WarmupCountdownDuration = DR;
		GameState->WarmupCountdownStartTime = TS;
		GameMode->WarmupEarlyCountdownDuration = DR;
	}

	return false;
	//return UWorld::GetWorld()->NetDriver->ClientConnections.Num() > 0;
}

inline APawn* SpawnDefaultPawnFor(AFortGameMode* GameMode, AFortPlayerController* Player, AActor* StartingLoc)
{
    FTransform Transform = StartingLoc->GetTransform();
    return (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(Player, Transform);
}

inline void (*DispatchRequestOG)(__int64 a1, unsigned __int64* a2, int a3);
inline void DispatchRequest(__int64 a1, unsigned __int64* a2, int a3)
{
    return DispatchRequestOG(a1, a2, 3);
}

__int64 AActorGetNetMode(AActor* a1)
{
    return 1;
}

__int64 WorldGetNetMode(UWorld* a1)
{
    return 1;
}

static __int64 (*StartAircraftPhaseOG)(AFortGameModeAthena* GameMode, char a2) = nullptr;
__int64 StartAircraftPhase(AFortGameModeAthena* GameMode, char a2)
{
	std::thread([]() {
		if (Globals::bPlayerBotsEnabled) { // if we wanna make it proper we need to figure out how to get OnAircraftEnteredDropzone called
			for (size_t i = 0; i < PlayerBotArray.size(); i++)
			{
				PlayerBotArray[i]->BotState = EBotState::PreBus;
			}
			Sleep(10000);
			for (size_t i = 0; i < PlayerBotArray.size(); i++)
			{
				PlayerBotArray[i]->BotState = EBotState::Bus;
			}
		}
	}).detach();

    return StartAircraftPhaseOG(GameMode, a2);
}

static inline void (*OriginalOnAircraftExitedDropZone)(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft);
void OnAircraftExitedDropZone(AFortGameModeAthena* GameMode, AFortAthenaAircraft* FortAthenaAircraft)
{
	if (Globals::bEventEnabled)
	{
		UFunction* StartEventFunc = JerkyLoader->Class->GetFunction("BP_Jerky_Loader_C", "startevent");

		float ToStart = 0.f;
		JerkyLoader->ProcessEvent(StartEventFunc, &ToStart);
	}

	if (Globals::bPlayerBotsEnabled) { // kick all bots out of the bus
		for (size_t i = 0; i < PlayerBotArray.size(); i++)
		{
			AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

			PlayerBotArray[i]->Pawn->K2_TeleportTo(GameState->GetAircraft(0)->K2_GetActorLocation(), {});
			PlayerBotArray[i]->Pawn->BeginSkydiving(true);
			PlayerBotArray[i]->TimeToNextAction = 0;
			PlayerBotArray[i]->LikelyHoodToDoAction = 0;
			PlayerBotArray[i]->BotState = EBotState::Skydiving;
		}
	}

	return OriginalOnAircraftExitedDropZone(GameMode, FortAthenaAircraft);
}

__int64 (*OnAircraftEnteredDropZoneOG)(AFortGameModeAthena*);
__int64 OnAircraftEnteredDropZone(AFortGameModeAthena* a1)
{
	/*Log("OnAircraftEnteredDropZone Called!");
	if (Globals::bPlayerBotsEnabled) {
		for (size_t i = 0; i < PlayerBotArray.size(); i++)
		{
			PlayerBotArray[i]->BotState = EBotState::Bus;
		}
	}*/

	return OnAircraftEnteredDropZoneOG(a1);
}

inline char (*SetUpInventoryBotOG)(AActor* Pawn, __int64 a2);
inline char __fastcall SetUpInventoryBot(AActor* Pawn, __int64 a2)
{
	*(AActor**)(__int64(Pawn) + 1352) = SpawnActor<AFortInventory>({}, {}, Pawn);

	return SetUpInventoryBotOG(Pawn, a2);
}

void (*StormOG)(AFortGameModeAthena* GameMode, int32 ZoneIndex);
void __fastcall Storm(AFortGameModeAthena* GameMode, int32 ZoneIndex)
{
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;

	for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
	{
		GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeID_SurviveStormCircle.AccoladeID_SurviveStormCircle"));
	}

	
	int StormDuration = 30;

	switch (Globals::TimesCalled)
	{
	case 0: StormDuration = 0; break;
	case 1: StormDuration = 60; break;
	case 2: StormDuration = 30; break;
	case 3: StormDuration = 15; break;
	default: break;
	}

	if (Globals::LateGame)
	{
		ZoneIndex = 4;
		GameMode->SafeZonePhase = 4;
		GameState->SafeZonePhase = 4;
		GameState->OnRep_SafeZonePhase();

		StormOG(GameMode, ZoneIndex);

		float StartDelay = 0.f;

		switch (Globals::TimesCalled)
		{
		case 1:
		case 2:
		case 3:
			StartDelay = 60.f;
			break;
		default:
			StartDelay = 0.f;
			break;
		}

		float CurrentTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
		GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = CurrentTime + StartDelay;
		GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + StormDuration;

		return;
	}


	return StormOG(GameMode, ZoneIndex);
}

void* SpawnPawnAfterReboot(ABuildingGameplayActorSpawnMachine* SpawnMachine, int SquadId)
{
	return SpawnActor<AFortPawn>(FVector{ 1, 1, 10000 }, FRotator{});
}