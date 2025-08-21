#pragma once
#include <Windows.h>
#undef max // skunky but it works
#include <algorithm>
#include "Utils.h"
#include "Abilities.h"
#include "Inventory.h"
#include "Vehicle.h"
#include "PlayerBots.h"
#include "Bosses.h"
#include "Quests.h"
#include "Siphon.h"

void (*ServerReadyToStartMatchOG)(AFortPlayerControllerAthena* PC);
void ServerReadyToStartMatch(AFortPlayerControllerAthena* PC)
{
	if (!PC) {
		return;
	}

	UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);

	AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

	AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

	if (!Globals::bEventEnabled)
		GameState->MapInfo->SupplyDropInfoList[0]->SupplyDropClass = StaticLoadObject<UClass>("/Game/Athena/SupplyDrops/AthenaSupplyDrop_Donut.AthenaSupplyDrop_Donut_C");

	GameState->DefaultBattleBus = StaticLoadObject<UAthenaBattleBusItemDefinition>("/Game/Athena/Items/Cosmetics/BattleBuses/BBID_DonutBus.BBID_DonutBus");

	for (size_t i = 0; i < GameMode->BattleBusCosmetics.Num(); i++)
	{
		GameMode->BattleBusCosmetics[i] = GameState->DefaultBattleBus;
	}

	static bool setupWorld = false;
	if (!Globals::bEventEnabled && !setupWorld) {
		SpawnFloorLoot();

		SpawnVehicle();

		SpawnLlamas();

		/*if (!meowsclesSpawned)
			spawnMeowscles();*/
	}

	return ServerReadyToStartMatchOG(PC);
}

inline void (*ServerLoadingScreenDroppedOG)(AFortPlayerControllerAthena* PC);
inline void ServerLoadingScreenDropped(AFortPlayerControllerAthena* PC)
{
	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
	AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
	AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto Pawn = (AFortPlayerPawn*)PC->Pawn;

	UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
	PlayerState->OnRep_CharacterData();

	PlayerState->SquadId = PlayerState->TeamIndex - 2;
	PlayerState->OnRep_SquadId();
	PlayerState->OnRep_TeamIndex(0);
	PlayerState->OnRep_PlayerTeam();
	PlayerState->OnRep_PlayerTeamPrivate();

	FGameMemberInfo Info{ -1, -1, -1 };
	Info.MemberUniqueId = PlayerState->UniqueId;
	Info.SquadId = PlayerState->SquadId;
	Info.TeamIndex = PlayerState->TeamIndex;

	GameState->GameMemberInfoArray.Members.Add(Info);

	GameState->GameMemberInfoArray.MarkItemDirty(Info);

	UAthenaPickaxeItemDefinition* PickaxeItemDef;
	FFortAthenaLoadout& CosmecticLoadoutPC = PC->CosmeticLoadoutPC;

	PickaxeItemDef = CosmecticLoadoutPC.Pickaxe != nullptr ? CosmecticLoadoutPC.Pickaxe : UObject::FindObject<UAthenaPickaxeItemDefinition>("DefaultPickaxe");// just incase the backend yk

	GiveItem(PC, PC->CosmeticLoadoutPC.Pickaxe->WeaponDefinition, 1, 0);

	Pawn->CosmeticLoadout = PC->CosmeticLoadoutPC;
	Pawn->OnRep_CosmeticLoadout();

	for (size_t i = 0; i < GameMode->StartingItems.Num(); i++)
	{
		if (GameMode->StartingItems[i].Count > 0)
		{
			GiveItem(PC, GameMode->StartingItems[i].Item, GameMode->StartingItems[i].Count, 0);
		}
	}

	static UFortAbilitySet* AbilitySet = StaticFindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer");
	GiveAbilitySet(PC, AbilitySet);

	auto CurrentPlaylist = GameState->CurrentPlaylistInfo.BasePlaylist;

	auto& ModifierList = *(TArray<TSoftObjectPtr<UFortGameplayModifierItemDefinition>>*)(__int64(CurrentPlaylist) + 0x170);

	for (int i = 0; i < ModifierList.Num(); i++)
	{
		auto Current = StaticLoadObject<UFortGameplayModifierItemDefinition>(UKismetStringLibrary::Conv_NameToString(ModifierList[i].ObjectID.AssetPathName).ToString());
		if (Current)
		{
			GiveModifier(Current, PC);
		}
	}

	PC->XPComponent->bRegisteredWithQuestManager = true;
	PC->XPComponent->OnRep_bRegisteredWithQuestManager();

	PlayerState->SeasonLevelUIDisplay = PC->XPComponent->CurrentLevel;
	PlayerState->OnRep_SeasonLevelUIDisplay();

	PlayerState->ForceNetUpdate();
	Pawn->ForceNetUpdate();
	PC->ForceNetUpdate();

	auto PhantomBooth = StaticLoadObject<UClass>("/Game/Athena/Items/EnvironmentalItems/HidingProps/Props/B_HidingProp_PhantomBooth.B_HidingProp_PhantomBooth_C");
	if (PhantomBooth) {
		Log("PhantomBooth Exists");
		auto SpawnLoc = Pawn->K2_GetActorLocation() + Pawn->GetActorForwardVector() * 300.f;
		auto SpawnRot = Pawn->K2_GetActorRotation();
		SpawnActor<AActor>(SpawnLoc, SpawnRot, nullptr, PhantomBooth);
	}
	else {
		Log("PhantomBooth does not exist!");
	}

	return ServerLoadingScreenDroppedOG(PC);
}


inline void ServerAttemptAircraftJump(UFortControllerComponent_Aircraft* Comp, FRotator Rotation)
{
	auto PC = (AFortPlayerControllerAthena*)Comp->GetOwner();
	UWorld::GetWorld()->AuthorityGameMode->RestartPlayer(PC);

	if (PC->MyFortPawn)
	{
		PC->ClientSetRotation(Rotation, true); 
		PC->MyFortPawn->BeginSkydiving(true);
		PC->MyFortPawn->SetHealth(100);
	}
}

void (*ServerAcknowledgePossessionOG)(AFortPlayerControllerAthena* PC, APawn* Pawn);
inline void ServerAcknowledgePossession(AFortPlayerControllerAthena* PC, APawn* Pawn)
{
	PC->AcknowledgedPawn = Pawn;
	return ServerAcknowledgePossessionOG(PC, Pawn);
}

inline void ServerExecuteInventoryItem(AFortPlayerControllerAthena* PC, FGuid Guid)
{
	if (!PC->MyFortPawn || !PC->Pawn)
		return;

	for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Guid)
		{
			UFortWeaponItemDefinition* DefToEquip = (UFortWeaponItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
			if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortGadgetItemDefinition::StaticClass()))
			{
				DefToEquip = ((UFortGadgetItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->GetWeaponItemDefinition();
			}
			else if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortDecoItemDefinition::StaticClass())) {
				auto DecoItemDefinition = (UFortDecoItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
				PC->MyFortPawn->PickUpActor(nullptr, DecoItemDefinition);
				PC->MyFortPawn->CurrentWeapon->ItemEntryGuid = Guid;
				static auto FortDecoTool_ContextTrapStaticClass = StaticLoadObject<UClass>("/Script/FortniteGame.FortDecoTool_ContextTrap");
				if (PC->MyFortPawn->CurrentWeapon->IsA(FortDecoTool_ContextTrapStaticClass))
				{
					reinterpret_cast<AFortDecoTool_ContextTrap*>(PC->MyFortPawn->CurrentWeapon)->ContextTrapItemDefinition = (UFortContextTrapItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
				}
				return;
			}
			PC->MyFortPawn->EquipWeaponDefinition(DefToEquip, Guid);
			break;
		}
	}
}

inline void (*ServerAttemptInteractOG)(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalData, EInteractionBeingAttempted InteractionBeingAttempted);
inline void ServerAttemptInteract(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalData, EInteractionBeingAttempted InteractionBeingAttempted)
{
	ServerAttemptInteractOG(Comp, ReceivingActor, InteractComponent, InteractType, OptionalData, InteractionBeingAttempted);

	std::cout << "ReceivingActor: " << ReceivingActor->GetFullName() << '\n';

	AFortPlayerControllerAthena* PC = Cast<AFortPlayerControllerAthena>(Comp->GetOwner());
	if (!PC) {
		return ServerAttemptInteractOG(Comp, ReceivingActor, InteractComponent, InteractType, OptionalData, InteractionBeingAttempted);
	}
	static UClass* AthenaQuestBGAClass = StaticLoadObject<UClass>("/Game/Athena/Items/QuestInteractablesV2/Parents/AthenaQuest_BGA.AthenaQuest_BGA_C");
	static map<AFortPlayerControllerAthena*, int> ChestsSearched{};

	if (ReceivingActor->IsA(AFortAthenaSupplyDrop::StaticClass()))
	{
		if (ReceivingActor->GetName().starts_with("AthenaSupplyDrop_Llama_C_"))
		{
			static auto Drop = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaLlama");

			auto LootDrops = PickLootDrops(Drop);

			auto CorrectLocation = ReceivingActor->K2_GetActorLocation();

			for (auto& LootDrop : LootDrops)
			{
				SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, EFortPickupSourceTypeFlag::Container, EFortPickupSpawnSource::SupplyDrop);
			}
		}
		else
		{
			static auto Drop = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaSupplyDrop");

			auto LootDrops = PickLootDrops(Drop);

			auto CorrectLocation = ReceivingActor->K2_GetActorLocation();

			for (auto& LootDrop : LootDrops)
			{
				SpawnPickup(LootDrop.ItemDefinition, LootDrop.Count, LootDrop.LoadedAmmo, CorrectLocation, EFortPickupSourceTypeFlag::Container, EFortPickupSpawnSource::SupplyDrop);
			}
		}
	}
	else if (PC->MyFortPawn && PC->MyFortPawn->IsInVehicle())
	{
		auto Vehicle = PC->MyFortPawn->GetVehicle();

		if (Vehicle)
		{
			auto SeatIdx = PC->MyFortPawn->GetVehicleSeatIndex();
			auto WeaponComp = Vehicle->GetSeatWeaponComponent(SeatIdx);
			if (WeaponComp)
			{
				GiveItem(PC, WeaponComp->WeaponSeatDefinitions[SeatIdx].VehicleWeapon, 1, 9999);
				for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition == WeaponComp->WeaponSeatDefinitions[SeatIdx].VehicleWeapon)
					{
						PC->SwappingItemDefinition = PC->MyFortPawn->CurrentWeapon->WeaponData;
						PC->ServerExecuteInventoryItem(PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid);
						break;
					}
				}
			}
		}
	}
	else if (ReceivingActor->IsA(AthenaQuestBGAClass))
	{
		ReceivingActor->ProcessEvent(ReceivingActor->Class->GetFunction("AthenaQuest_BGA_C", "BindToQuestManagerForQuestUpdate"), &PC);
		TArray<UFortQuestItemDefinition*>& QuestsRequiredOnProfile = *(TArray<UFortQuestItemDefinition*>*)(__int64(ReceivingActor) + 0x850);
		FName& Primary_BackendName = *(FName*)(__int64(ReceivingActor) + 0x860);

		ProgressQuest(PC, QuestsRequiredOnProfile[0], Primary_BackendName);
	}
	else if (ReceivingActor->Class->GetName().contains("Tiered_") || ReceivingActor->IsA(AFortAthenaSupplyDrop::StaticClass()))
	{
		ChestsSearched[PC]++;
		GiveAccolade(PC, GetDefFromEvent(EAccoladeEvent::Search, ChestsSearched[PC], ReceivingActor));
	}
	else if (ReceivingActor->GetName().contains("Wumba"))
	{
		{
			auto UpgradeBench = (AB_Athena_Wumba_C*)ReceivingActor;
			auto WoodCostCurve = UpgradeBench->WoodCostCurve;
			auto HorizontalEnabled = UpgradeBench->HorizontalEnabled;

			static auto WumbaDataTable = StaticLoadObject<UDataTable>("/Game/Items/Datatables/AthenaWumbaData.AthenaWumbaData");
			static auto LootPackagesRowMap = WumbaDataTable->RowMap;

			auto Pawn = PC->MyFortPawn;
			auto CurrentHeldWeapon = Pawn->CurrentWeapon;
			auto CurrentHeldWeaponDef = CurrentHeldWeapon->WeaponData;

			FWeaponUpgradeItemRow* FoundRow = nullptr;

			static auto WoodItemData = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
			static auto StoneItemData = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
			static auto MetalItemData = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/MetalItemData.MetalItemData");

			auto WoodInstance = FindItemInstance(PC->WorldInventory, WoodItemData);
			auto StoneInstance = FindItemInstance(PC->WorldInventory, StoneItemData);
			auto MetalInstance = FindItemInstance(PC->WorldInventory, MetalItemData);

			int WoodCount = WoodInstance ? WoodInstance->ItemEntry.Count : 0;
			int StoneCount = StoneInstance ? StoneInstance->ItemEntry.Count : 0;
			int MetalCount = MetalInstance ? MetalInstance->ItemEntry.Count : 0;

			for (int i = 0; i < LootPackagesRowMap.Elements.Num(); i++) {
				auto& Pair = LootPackagesRowMap.Elements[i];

				auto RowFName = Pair.First;
				if (!RowFName.ComparisonIndex)
					continue;

				auto Row = (FWeaponUpgradeItemRow*)Pair.Second;

				if (InteractionBeingAttempted == EInteractionBeingAttempted::SecondInteraction) {
					if (Row->CurrentWeaponDef == CurrentHeldWeaponDef &&
						Row->Direction == EFortWeaponUpgradeDirection::Horizontal) {
						FoundRow = Row;
						break;
					}
				}
				else {
					if (Row->CurrentWeaponDef == CurrentHeldWeaponDef) {
						FoundRow = Row;
						break;
					}
				}
			}

			if (!FoundRow)
				return;

			auto NewDefinition = FoundRow->UpgradedWeaponDef;

			int WoodCost = static_cast<int>(FoundRow->WoodCost) * 50;
			int StoneCost = static_cast<int>(FoundRow->BrickCost) * 50;
			int MetalCost = static_cast<int>(FoundRow->MetalCost) * 50;

			if (!PC->bInfiniteAmmo) {
				if (FoundRow->Direction == EFortWeaponUpgradeDirection::Vertical) {
					int brick = std::max(0, int(FoundRow->BrickCost) - 8);
					int metal = std::max(0, int(FoundRow->MetalCost) - 4);

					RemoveItem(PC, WoodInstance->ItemEntry.ItemDefinition, WoodCost);
					RemoveItem(PC, StoneInstance->ItemEntry.ItemDefinition, brick * 50);
					RemoveItem(PC, MetalInstance->ItemEntry.ItemDefinition, metal * 50);
				}
				else {
					RemoveItem(PC, WoodInstance->ItemEntry.ItemDefinition, 20);
					RemoveItem(PC, StoneInstance->ItemEntry.ItemDefinition, 20);
					RemoveItem(PC, MetalInstance->ItemEntry.ItemDefinition, 20);
				}
			}

			RemoveItem(PC, CurrentHeldWeapon->ItemEntryGuid, 1);
			GiveItem(PC, NewDefinition, 1, CurrentHeldWeapon->GetMagazineAmmoCount());
		}

	}
	else if (ReceivingActor->IsA(ABGA_Athena_Keycard_Lock_Parent_C::StaticClass()))
	{
		UFortItemDefinition* Def = *(UFortItemDefinition**)(__int64(ReceivingActor) + 0x8F0);// this took long to find
		RemoveItem(PC, Def, 1);
	}
}

inline void (*ServerAttemptExitVehicleOG)(AFortPlayerController* PC);
inline void ServerAttemptExitVehicle(AFortPlayerControllerZone* PC) 
{
	if (!PC)
		return;

	auto Pawn = (AFortPlayerPawn*)PC->Pawn;

	ServerAttemptExitVehicleOG(PC);

	if (!Pawn->GetVehicleSeatIndex() == 0)
		return;

	if (!Pawn->CurrentWeapon || !Pawn->CurrentWeapon->IsA(AFortWeaponRangedForVehicle::StaticClass()))
		return;

	RemoveItem((AFortPlayerController*)Pawn->Controller, Pawn->CurrentWeapon->GetInventoryGUID(), 1);

	UFortWorldItemDefinition* SwappingItemDef = ((AFortPlayerControllerAthena*)PC)->SwappingItemDefinition;
	if (!SwappingItemDef) 
		return; 

	FFortItemEntry* SwappingItemEntry = FindItemEntry(PC, SwappingItemDef);
	if (!SwappingItemEntry) 
		return; 

	PC->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)SwappingItemDef, SwappingItemEntry->ItemGuid);
}

inline void (*ServerHandlePickupOG)(AFortPlayerPawn* Pawn, AFortPickup* Pickup, float InFlyTime, FVector InStartDirection, bool bPlayPickupSound);
inline void ServerHandlePickup(AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, float InFlyTime, const FVector& InStartDirection, bool bPlayPickupSound)
{
	if (!Pickup || !Pawn || !Pawn->Controller || Pickup->bPickedUp)
		return;

	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;

	UFortItemDefinition* Def = Pickup->PrimaryPickupItemEntry.ItemDefinition;
	FFortItemEntry* FoundEntry = nullptr;
	FFortItemEntry& PickupEntry = Pickup->PrimaryPickupItemEntry;
	float MaxStackSize = GetMaxStack(Def);
	bool Stackable = Def->IsStackable();
	UFortItemDefinition* PickupItemDef = PickupEntry.ItemDefinition;
	bool Found = false;
	FFortItemEntry* GaveEntry = nullptr;

	if (IsInventoryFull(PC))
	{
		if (Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
		{
			GiveItemStack(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);

			Pickup->PickupLocationData.bPlayPickupSound = true;
			Pickup->PickupLocationData.FlyTime = 0.3f;
			Pickup->PickupLocationData.ItemOwner = Pawn;
			Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
			Pickup->PickupLocationData.PickupTarget = Pawn;
			Pickup->OnRep_PickupLocationData();

			Pickup->bPickedUp = true;
			Pickup->OnRep_bPickedUp();
			return;
		}

		if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
		{
			if (Stackable)
			{
				for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

					if (Entry.ItemDefinition == PickupItemDef)
					{
						Found = true;
						if ((MaxStackSize - Entry.Count) > 0)
						{
							Entry.Count += PickupEntry.Count;

							if (Entry.Count > MaxStackSize)
							{
								SpawnStack((APlayerPawn_Athena_C*)PC->Pawn, PickupItemDef, Entry.Count - MaxStackSize);
								Entry.Count = MaxStackSize;
							}

							PC->WorldInventory->Inventory.MarkItemDirty(Entry);
						}
						else
						{
							if (IsPrimaryQuickbar(PickupItemDef))
							{
								GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count);
							}
						}
						break;
					}
				}
				if (!Found)
				{
					for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
					{
						if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Pawn->CurrentWeapon->GetInventoryGUID())
						{
							PC->ServerAttemptInventoryDrop(Pawn->CurrentWeapon->GetInventoryGUID(), PC->WorldInventory->Inventory.ReplicatedEntries[i].Count, false);
							break;
						}
					}
					GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count, false, 0, true);
				}

				Pickup->PickupLocationData.bPlayPickupSound = true;
				Pickup->PickupLocationData.FlyTime = 0.3f;
				Pickup->PickupLocationData.ItemOwner = Pawn;
				Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
				Pickup->PickupLocationData.PickupTarget = Pawn;
				Pickup->OnRep_PickupLocationData();

				Pickup->bPickedUp = true;
				Pickup->OnRep_bPickedUp();
				return;
			}

			for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
			{
				if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid == Pawn->CurrentWeapon->GetInventoryGUID())
				{
					PC->ServerAttemptInventoryDrop(Pawn->CurrentWeapon->GetInventoryGUID(), PC->WorldInventory->Inventory.ReplicatedEntries[i].Count, false);
					break;
				}
			}
		}
	}

	if (!IsInventoryFull(PC))
	{
		if (Stackable && !Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Stackable && !Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
		{
			for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
			{
				FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

				if (Entry.ItemDefinition == PickupItemDef)
				{
					Found = true;
					if ((MaxStackSize - Entry.Count) > 0)
					{
						Entry.Count += PickupEntry.Count;

						if (Entry.Count > MaxStackSize)
						{
							SpawnStack((APlayerPawn_Athena_C*)PC->Pawn, PickupItemDef, Entry.Count - MaxStackSize);
							Entry.Count = MaxStackSize;
						}

						PC->WorldInventory->Inventory.MarkItemDirty(Entry);
					}
					else
					{
						if (IsPrimaryQuickbar(PickupItemDef))
						{
							GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count);
						}
					}
					break;
				}
			}
			if (!Found)
			{
				GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count, false, 0, true);
			}

			Pickup->PickupLocationData.bPlayPickupSound = true;
			Pickup->PickupLocationData.FlyTime = 0.3f;
			Pickup->PickupLocationData.ItemOwner = Pawn;
			Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
			Pickup->PickupLocationData.PickupTarget = Pawn;
			Pickup->OnRep_PickupLocationData();

			Pickup->bPickedUp = true;
			Pickup->OnRep_bPickedUp();
			return;
		}

		if (Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
		{
			GiveItemStack(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
		}
		else {
			GiveItem(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
		}
	}

	Pickup->PickupLocationData.bPlayPickupSound = true;
	Pickup->PickupLocationData.FlyTime = 0.3f;
	Pickup->PickupLocationData.ItemOwner = Pawn;
	Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
	Pickup->PickupLocationData.PickupTarget = Pawn;
	Pickup->OnRep_PickupLocationData();

	Pickup->bPickedUp = true;
	Pickup->OnRep_bPickedUp();
}

void (*OnCapsuleBeginOverlapOG)(AFortPlayerPawn* Pawn, UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, FHitResult SweepResult);
void OnCapsuleBeginOverlap(AFortPlayerPawn* Pawn, UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, FHitResult SweepResult)
{
	if (OtherActor->IsA(AFortPickup::StaticClass()))
	{
		AFortPickup* Pickup = (AFortPickup*)OtherActor;

		if (Pickup->PawnWhoDroppedPickup == Pawn)
			return OnCapsuleBeginOverlapOG(Pawn, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

		UFortWorldItemDefinition* Def = (UFortWorldItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition;

		if (!Def) {
			return OnCapsuleBeginOverlapOG(Pawn, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
		}

		auto PC = (AFortPlayerControllerAthena*)Pawn->GetOwner();
		FFortItemEntry* FoundEntry = nullptr;
		auto HighestCount = 0;

		if (PC->IsA(ABP_PhoebePlayerController_C::StaticClass())) return OnCapsuleBeginOverlapOG(Pawn, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
		for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

			if (Entry.ItemDefinition == Def && (Entry.Count <= GetMaxStackSize(Def)))
			{
				FoundEntry = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
				if (Entry.Count > HighestCount)
					HighestCount = Entry.Count;
			}
		}
		if (Def->IsA(UFortAmmoItemDefinition::StaticClass()) || Def->IsA(UFortResourceItemDefinition::StaticClass()) || Def->IsA(UFortTrapItemDefinition::StaticClass())) {
			if (FoundEntry && HighestCount < GetMaxStackSize(Def)) {
				Pawn->ServerHandlePickup(Pickup, 0.40f, FVector(), true);
			}
			else if (!FoundEntry) {
				Pawn->ServerHandlePickup(Pickup, 0.40f, FVector(), true);
			}
		}
		else if (FoundEntry)
		{
			if (FoundEntry->Count < GetMaxStackSize(Def)) {
				Pawn->ServerHandlePickup(Pickup, 0.40f, FVector(), true);
			}
		}
	}

	return OnCapsuleBeginOverlapOG(Pawn, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

inline void ServerAttemptInventoryDrop(AFortPlayerControllerAthena* PC, FGuid ItemGuid, int Count, bool bTrash)
{
	FFortItemEntry* Entry = FindEntry(PC, ItemGuid);
	SpawnPickup(Entry->ItemDefinition, Count, Entry->LoadedAmmo, PC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset);
	RemoveItem(PC, ItemGuid, Count);
}

inline void ServerCreateBuildingActor(AFortPlayerControllerAthena* PC, FCreateBuildingActorData CreateBuildingData)
{
	if (!PC || PC->IsInAircraft())
		return;

	UClass* BuildingClass = PC->BroadcastRemoteClientInfo->RemoteBuildableClass.Get();
	char a7;
	TArray<AActor*> BuildingsToRemove;
	if (!CantBuild(UWorld::GetWorld(), BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, CreateBuildingData.bMirrored, &BuildingsToRemove, &a7))
	{
		auto ResDef = UFortKismetLibrary::GetDefaultObj()->K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->DefaultObject)->ResourceType);
		RemoveItem(PC, ResDef, 10);

		ABuildingSMActor* NewBuilding = SpawnActor<ABuildingSMActor>(CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, PC, BuildingClass);

		NewBuilding->bPlayerPlaced = true;
		NewBuilding->InitializeKismetSpawnedBuildingActor(NewBuilding, PC, true);
		NewBuilding->TeamIndex = ((AFortPlayerStateAthena*)PC->PlayerState)->TeamIndex;
		NewBuilding->Team = EFortTeam(NewBuilding->TeamIndex);

		for (size_t i = 0; i < BuildingsToRemove.Num(); i++)
		{
			BuildingsToRemove[i]->K2_DestroyActor();
		}
		UKismetArrayLibrary::Array_Clear((TArray<int32>&)BuildingsToRemove);
	}
}


inline __int64 (*OnDamageServerOG)(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, FHitResult HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle EffectContext);
inline __int64 OnDamageServer(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, FHitResult HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle EffectContext)
{
	if (!Actor || !InstigatedBy || !InstigatedBy->IsA(AFortPlayerControllerAthena::StaticClass()) || !DamageCauser->IsA(AFortWeapon::StaticClass()) || !((AFortWeapon*)DamageCauser)->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) || Actor->bPlayerPlaced)
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	auto Def = UFortKismetLibrary::K2_GetResourceItemDefinition(Actor->ResourceType);
	static map<AFortPlayerControllerAthena*, int> NumWeakSpots{};

	if (Def)
	{
		auto& BuildingResourceAmountOverride = Actor->BuildingResourceAmountOverride;
		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		FString CurveTableAssetPath = UKismetStringLibrary::Conv_NameToString(GameState->CurrentPlaylistInfo.BasePlaylist->ResourceRates.ObjectID.AssetPathName);
		static auto CurveTable = StaticLoadObject<UCurveTable>(CurveTableAssetPath.ToString());
		if (!CurveTable)
			CurveTable = StaticLoadObject<UCurveTable>("/Game/Athena/Balance/DataTables/AthenaResourceRates.AthenaResourceRates");

		float Average = 1;
		EEvaluateCurveTableResult OutCurveTable;
		UDataTableFunctionLibrary::EvaluateCurveTableRow(CurveTable, BuildingResourceAmountOverride.RowName, 0.f, &OutCurveTable, &Average, FString());
		float FinalResourceCount = round(Average / (Actor->GetMaxHealth() / Damage));

		if (FinalResourceCount > 0)
		{
			if (Damage == 100.f && GameState->GamePhase != EAthenaGamePhase::Warmup)
			{
				UFortAccoladeItemDefinition* AccoladeDef = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_066_WeakSpotsInARow.AccoladeId_066_WeakSpotsInARow");

				NumWeakSpots[InstigatedBy]++; 
				if (NumWeakSpots[InstigatedBy] == 5) {
					GiveAccolade(InstigatedBy, AccoladeDef);
					NumWeakSpots[InstigatedBy] = 0;
				}
			}
		
			InstigatedBy->ClientReportDamagedResourceBuilding(Actor, Actor->ResourceType, FinalResourceCount, false, Damage == 100.f);
			GiveItemStack(InstigatedBy, Def, FinalResourceCount, 0);
		}
	}

	return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
}


inline void ServerBeginEditingBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit)
{
	if (!BuildingActorToEdit || !BuildingActorToEdit->bPlayerPlaced || !PC->MyFortPawn)
		return;

	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
	BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_Awake);
	BuildingActorToEdit->EditingPlayer = PlayerState;
	for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
	{
		auto Item = PC->WorldInventory->Inventory.ItemInstances[i];
		if (Item->GetItemDefinitionBP()->IsA(UFortEditToolItemDefinition::StaticClass()))
		{
			PC->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Item->GetItemDefinitionBP(), Item->GetItemGuid());
			break;
		}
	}
	if (!PC->MyFortPawn->CurrentWeapon || !PC->MyFortPawn->CurrentWeapon->WeaponData || !PC->MyFortPawn->CurrentWeapon->IsA(AFortWeap_EditingTool::StaticClass()))
		return;

	AFortWeap_EditingTool* EditTool = (AFortWeap_EditingTool*)PC->MyFortPawn->CurrentWeapon;
	EditTool->EditActor = BuildingActorToEdit;
	EditTool->OnRep_EditActor();
}

inline void ServerEndEditingBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit)
{
	if (!BuildingActorToEdit || !PC->MyFortPawn || BuildingActorToEdit->bDestroyed == 1 || BuildingActorToEdit->EditingPlayer != PC->PlayerState)
		return;
	BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_DormantAll);
	BuildingActorToEdit->EditingPlayer = nullptr;
	for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
	{
		auto Item = PC->WorldInventory->Inventory.ItemInstances[i];
		if (Item->GetItemDefinitionBP()->IsA(UFortEditToolItemDefinition::StaticClass()))
		{
			PC->MyFortPawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Item->GetItemDefinitionBP(), Item->GetItemGuid());
			break;
		}
	}
	if (!PC->MyFortPawn->CurrentWeapon || !PC->MyFortPawn->CurrentWeapon->WeaponData || !PC->MyFortPawn->CurrentWeapon->IsA(AFortWeap_EditingTool::StaticClass()))
		return;

	AFortWeap_EditingTool* EditTool = (AFortWeap_EditingTool*)PC->MyFortPawn->CurrentWeapon;
	EditTool->EditActor = nullptr;
	EditTool->OnRep_EditActor();
}

inline void ServerEditBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored)
{
	if (!BuildingActorToEdit || BuildingActorToEdit->EditingPlayer != PC->PlayerState || !NewBuildingClass.Get() || BuildingActorToEdit->bDestroyed == 1)
		return;

	BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_DormantAll);
	BuildingActorToEdit->EditingPlayer = nullptr;
	ABuildingSMActor* NewActor = ReplaceBuildingActor(BuildingActorToEdit, 1, NewBuildingClass.Get(), 0, RotationIterations, bMirrored, PC);
	if (NewActor)
		NewActor->bPlayerPlaced = true;
}

void ServerRepairBuildingActor(AFortPlayerController* PC, ABuildingSMActor* BuildingActorToRepair)
{
	auto FortKismet = (UFortKismetLibrary*)UFortKismetLibrary::StaticClass()->DefaultObject;
	if (!BuildingActorToRepair)
		return;

	if (BuildingActorToRepair->EditingPlayer)
	{
		return;
	}

	float BuildingHealthPercent = BuildingActorToRepair->GetHealthPercent();

	float BuildingCost = 10;
	float RepairCostMultiplier = 0.75;

	float BuildingHealthPercentLost = 1.0f - BuildingHealthPercent;
	float RepairCostUnrounded = (BuildingCost * BuildingHealthPercentLost) * RepairCostMultiplier;
	float RepairCost = std::floor(RepairCostUnrounded > 0 ? RepairCostUnrounded < 1 ? 1 : RepairCostUnrounded : 0);

	if (RepairCost < 0)
		return;

	auto ResDef = FortKismet->K2_GetResourceItemDefinition(BuildingActorToRepair->ResourceType);

	if (!ResDef)
		return;

	if (!PC->bBuildFree)
	{
		RemoveItem(PC, ResDef, (int)RepairCost);
	}

	BuildingActorToRepair->RepairBuilding(PC, (int)RepairCost);
}

//taken from ero
int64_t(*ApplyCostOG)(UGameplayAbility* arg1, int32_t arg2, void* arg3, void* arg4);
int64_t ApplyCost(UFortGameplayAbility* arg1, int32_t arg2, void* arg3, void* arg4)
{
	auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
	if (arg1->GetName().starts_with("GA_Athena_AppleSun_Passive_C_")) {
		auto Def = StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Consumables/AppleSun/WID_Athena_AppleSun.WID_Athena_AppleSun");
		auto ASC = arg1->GetActivatingAbilityComponent();
		AFortPlayerStateAthena* PS = (AFortPlayerStateAthena*)ASC->GetOwner();
		auto Pawn = PS->GetCurrentPawn();
		AFortPlayerController* PC = nullptr;
		PC = (AFortPlayerController*)Pawn->GetOwner();

		if (!PC->bInfiniteAmmo) {
			RemoveItem(PC, Def, 1);
		}
	}
	else if (arg1->GetName().starts_with("GA_Ranged_") || arg1->GetName().starts_with("GAB_Melee_ImpactCombo_Athena_")) {
		auto ASC = arg1->GetActivatingAbilityComponent();
		AFortPlayerStateAthena* PS = (AFortPlayerStateAthena*)ASC->GetOwner();
		auto Pawn = PS->GetCurrentPawn();
		AFortPlayerController* PC = nullptr;
		PC = (AFortPlayerController*)Pawn->GetOwner();
		if (PC->IsA(ABP_PhoebePlayerController_C::StaticClass())) {
			return ApplyCostOG(arg1, arg2, arg3, arg4);
		}
		auto t = std::floor(Statics->GetTimeSeconds(UWorld::GetWorld()));
		if (Globals::LastTime != (int)t) {
			Globals::LastTime = (int)t;
			Shots[PC] = 0;
		}
		if (Weapons[PC] != Pawn->CurrentWeapon->WeaponData) {
			Shots[PC] = 0;
			Weapons[PC] = Pawn->CurrentWeapon->WeaponData;
		}

		auto WSH = Weapons[PC]->WeaponStatHandle;
	}
	return ApplyCostOG(arg1, arg2, arg3, arg4);
}

//taken from ero
__int64 (*OnExplodedOG)(AB_Prj_Athena_Consumable_Thrown_C* Consumable, TArray<class AActor*>& HitActors, TArray<struct FHitResult>& HitResults) = decltype(OnExplodedOG)(__int64(GetModuleHandleA(0)) + 0x3EA2740);
__int64 OnExploded(AB_Prj_Athena_Consumable_Thrown_C* Consumable, TArray<class AActor*>& HitActors, TArray<struct FHitResult>& HitResults)
{
	if (!Consumable)
		return OnExplodedOG(Consumable, HitActors, HitResults);
	else if (Consumable->GetName() == "B_Prj_Athena_Bucket_Old_C" || Consumable->GetName() == "B_Prj_Athena_Bucket_Nice_C") {
		auto PC = Consumable->GetOwnerPlayerController();
		auto Pawn = PC->MyFortPawn;
		auto Def = *(UFortItemDefinition**)(__int64(Consumable) + 0x888);
		if (!Def)
			return OnExplodedOG(Consumable, HitActors, HitResults);

		AFortPickupAthena* NewPickup = Actors<AFortPickupAthena>(AFortPickupAthena::StaticClass(), Consumable->K2_GetActorLocation());
		NewPickup->bRandomRotation = true;
		NewPickup->PrimaryPickupItemEntry.ItemDefinition = Def;
		NewPickup->PrimaryPickupItemEntry.Count = 1;
		NewPickup->PrimaryPickupItemEntry.LoadedAmmo = 1;
		NewPickup->OnRep_PrimaryPickupItemEntry();
		NewPickup->PawnWhoDroppedPickup = Pawn;
		Pawn->ServerHandlePickup(NewPickup, 0.40f, FVector(), false);
	}
	if (!Consumable->GetName().starts_with("B_Prj_Athena_Consumable_Thrown_")) {
		return OnExplodedOG(Consumable, HitActors, HitResults);
	}
	auto Def = *(UFortItemDefinition**)(__int64(Consumable) + 0x888);
	if (!Def)
		return OnExplodedOG(Consumable, HitActors, HitResults);
	SpawnPickup(Def, 1, 0, Consumable->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset);
	Consumable->K2_DestroyActor();
	return OnExplodedOG(Consumable, HitActors, HitResults);
}

inline void (*ClientOnPawnDiedOG)(AFortPlayerControllerAthena*, FFortPlayerDeathReport);
inline void ClientOnPawnDied(AFortPlayerControllerAthena* DeadPC, FFortPlayerDeathReport DeathReport)
{

	Log("Pawn died");
	DeadPC->bMarkedAlive = false;

	auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	AFortPlayerStateAthena* DeadState = (AFortPlayerStateAthena*)DeadPC->PlayerState;
	AFortPlayerPawnAthena* KillerPawn = (AFortPlayerPawnAthena*)DeathReport.KillerPawn;
	AFortPlayerStateAthena* KillerState = (AFortPlayerStateAthena*)DeathReport.KillerPlayerState;

	static bool Won = false;

	Siphon(KillerPawn);

	if (!GameState->IsRespawningAllowed(DeadState))
	{
		if (DeadPC && DeadPC->WorldInventory)
		{
			for (size_t i = 0; i < DeadPC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
			{
				if (((UFortWorldItemDefinition*)DeadPC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped)
				{
					SpawnPickup(DeadPC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemDefinition, DeadPC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.Count, DeadPC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.LoadedAmmo, DeadPC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, DeadPC->MyFortPawn);
				}
			}
		}
	}

	if (!Won && DeadPC && DeadState)
	{
		if (KillerState && KillerState != DeadState)
		{
			KillerState->KillScore++;

			if (Bots.empty())
			{
				for (size_t i = 0; i < KillerState->PlayerTeam->TeamMembers.Num(); i++)
				{
					((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->TeamKillScore++;
					((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->OnRep_TeamKillScore();
				}

				KillerState->ClientReportKill(DeadState);
				KillerState->OnRep_Kills();
			}


			DeadState->PawnDeathLocation = DeadPC->Pawn->K2_GetActorLocation();
			FDeathInfo& DeathInfo = DeadState->DeathInfo;


			if (PlayerBotArray.empty() || Bots.empty())
			{

				GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, GetDefFromEvent(EAccoladeEvent::Kill, KillerState->KillScore));

				AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
				if (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num() == 50)
				{
					for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
					{
						GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_026_Survival_Default_Bronze.AccoladeId_026_Survival_Default_Bronze"));
					}
				}
				if (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num() == 25)
				{
					for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
					{
						GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_027_Survival_Default_Silver.AccoladeId_027_Survival_Default_Silver"));
					}
				}
				if (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num() == 10)
				{
					for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
					{
						GiveAccolade(GameMode->AlivePlayers[i], StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_028_Survival_Default_Gold.AccoladeId_028_Survival_Default_Gold"));
					}
				}

				float Distance = DeathInfo.Distance / 100.0f;

				if (Distance >= 100.0f && Distance < 150.0f)
				{
					GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_DistanceShot.AccoladeId_DistanceShot")); // 100-149m accolade
				}
				else if (Distance >= 150.0f && Distance < 200.0f)
				{
					GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_051_LongShot.AccoladeId_051_LongShot")); // 150-199m accolade
				}
				else if (Distance >= 200.0f && Distance < 250.0f)
				{
					GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_052_LudicrousShot.AccoladeId_052_LudicrousShot")); // 200-249m accolade
				}
				else if (Distance >= 250.0f)
				{
					GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_053_ImpossibleShot.AccoladeId_053_ImpossibleShot")); // 250+m accolade
				}

				//idrk how to get mr bone here not sure if we can pass thro like bots
				//if (BoneName.ToString() == "head")
				//{
					//GiveAccolade((AFortPlayerControllerAthena*)KillerState->Owner, StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_048_HeadshotElim.AccoladeId_048_HeadshotElim")); // headshot accolade
				//}
			}

			DeathInfo.bDBNO = DeadPC->MyFortPawn->bWasDBNOOnDeath;
			DeathInfo.bInitialized = true;
			DeathInfo.DeathLocation = DeadPC->Pawn->K2_GetActorLocation();
			DeathInfo.DeathTags = DeathReport.Tags;
			DeathInfo.Downer = KillerState;
			DeathInfo.Distance = (KillerPawn ? KillerPawn->GetDistanceTo(DeadPC->Pawn) : ((AFortPlayerPawnAthena*)DeadPC->Pawn)->LastFallDistance);
			DeathInfo.FinisherOrDowner = KillerState;
			DeathInfo.DeathCause = DeadState->ToDeathCause(DeathInfo.DeathTags, DeathInfo.bDBNO);
			DeadState->OnRep_DeathInfo();
			DeadPC->RespawnPlayerAfterDeath(true);
		}

		if (Won || !GameState->IsRespawningAllowed(DeadState))
		{
			FAthenaRewardResult Result;
			UFortPlayerControllerAthenaXPComponent* XPComponent = DeadPC->XPComponent;
			Result.TotalBookXpGained = XPComponent->TotalXpEarned;
			Result.TotalSeasonXpGained = XPComponent->TotalXpEarned;

			DeadPC->ClientSendEndBattleRoyaleMatchForPlayer(true, Result);

			FAthenaMatchStats Stats;
			FAthenaMatchTeamStats TeamStats;

			if (DeadState)
			{
				DeadState->Place = GameMode->AliveBots.Num() + GameMode->AlivePlayers.Num();
				DeadState->OnRep_Place();
			}

			for (size_t i = 0; i < 20; i++)
			{
				Stats.Stats[i] = 0;
			}

			Stats.Stats[3] = DeadState->KillScore;

			TeamStats.Place = DeadState->Place;
			TeamStats.TotalPlayers = GameState->TotalPlayers;

			DeadPC->ClientSendMatchStatsForPlayer(Stats);
			DeadPC->ClientSendTeamStatsForPlayer(TeamStats);
			FDeathInfo& DeathInfo = DeadState->DeathInfo;

			RemoveFromAlivePlayers(GameMode, DeadPC, (KillerState == DeadState ? nullptr : KillerState), KillerPawn, DeathReport.KillerWeapon ? DeathReport.KillerWeapon : nullptr, DeadState ? DeathInfo.DeathCause : EDeathCause::Rifle, 0);

			if (KillerState)
			{
				if (KillerState->Place == 1)
				{
					if (DeathReport.KillerWeapon)
					{
						((AFortPlayerControllerAthena*)KillerState->Owner)->PlayWinEffects(KillerPawn, DeathReport.KillerWeapon, EDeathCause::Rifle, false);
						((AFortPlayerControllerAthena*)KillerState->Owner)->ClientNotifyWon(KillerPawn, DeathReport.KillerWeapon, EDeathCause::Rifle);
					}

					FAthenaRewardResult Result;
					AFortPlayerControllerAthena* KillerPC = (AFortPlayerControllerAthena*)KillerState->GetOwner();
					KillerPC->ClientSendEndBattleRoyaleMatchForPlayer(true, Result);

					FAthenaMatchStats Stats;
					FAthenaMatchTeamStats TeamStats;

					for (size_t i = 0; i < 20; i++)
					{
						Stats.Stats[i] = 0;
					}

					Stats.Stats[3] = KillerState->KillScore;

					TeamStats.Place = 1;
					TeamStats.TotalPlayers = GameState->TotalPlayers;

					KillerPC->ClientSendMatchStatsForPlayer(Stats);
					KillerPC->ClientSendTeamStatsForPlayer(TeamStats);

					GameState->WinningPlayerState = KillerState;
					GameState->WinningTeam = KillerState->TeamIndex;
					GameState->OnRep_WinningPlayerState();
					GameState->OnRep_WinningTeam();
				}
			}
		}
	}

	return ClientOnPawnDiedOG(DeadPC, DeathReport);
}

static inline void(*OrginalServerSetInAircraft)(AFortPlayerStateAthena* PlayerState, bool bNewInAircraft);
void ServerSetInAircraft(AFortPlayerStateAthena* PlayerState, bool bNewInAircraft)
{

	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)PlayerState->Owner;

	if (PC && PC->WorldInventory)
	{
		for (int i = PC->WorldInventory->Inventory.ReplicatedEntries.Num() - 1; i >= 0; i--)
		{
			if (((UFortWorldItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped)
			{
				int Count = PC->WorldInventory->Inventory.ReplicatedEntries[i].Count;
				RemoveItem(PC, PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Count);
			}
		}
	}

	if (Globals::bEventEnabled)
	{
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		AActor* BattleBus = GameState->GetAircraft(0);
		auto Aircraft = GameState->GetAircraft(0);
		FVector Loc = FVector(62590, -75501, 13982);
		Aircraft->FlightInfo.FlightStartLocation = (FVector_NetQuantize100)Loc;
		Aircraft->FlightInfo.FlightSpeed = 1000;
		Aircraft->FlightInfo.TimeTillDropStart = 1;
		Aircraft->DropStartTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 1;
		GameState->bAircraftIsLocked = false;
	}

	return OrginalServerSetInAircraft(PlayerState, bNewInAircraft);
}

void ServerPlayEmoteItem(AFortPlayerControllerAthena* PC, UFortMontageItemDefinitionBase* EmoteAsset, float EmoteRandomNumber) {
	Log("ServerPlayEmoteItem Called!");

	if (!PC || !EmoteAsset)
		return;

	UClass* DanceAbility = StaticLoadObject<UClass>("/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");
	UClass* SprayAbility = StaticLoadObject<UClass>("/Game/Abilities/Sprays/GAB_Spray_Generic.GAB_Spray_Generic_C");

	if (!DanceAbility || !SprayAbility)
		return;

	auto EmoteDef = (UAthenaDanceItemDefinition*)EmoteAsset;
	if (!EmoteDef)
		return;

	PC->MyFortPawn->bMovingEmote = EmoteDef->bMovingEmote;
	PC->MyFortPawn->EmoteWalkSpeed = EmoteDef->WalkForwardSpeed;
	PC->MyFortPawn->bMovingEmoteForwardOnly = EmoteDef->bMoveForwardOnly;
	PC->MyFortPawn->EmoteWalkSpeed = EmoteDef->WalkForwardSpeed;

	FGameplayAbilitySpec Spec{};
	UGameplayAbility* Ability = nullptr;

	if (EmoteAsset->IsA(UAthenaSprayItemDefinition::StaticClass()))
	{
		Ability = (UGameplayAbility*)SprayAbility->DefaultObject;
	}
	else if (EmoteAsset->IsA(UAthenaToyItemDefinition::StaticClass())) {
		
	}

	if (Ability == nullptr) {
		Ability = (UGameplayAbility*)DanceAbility->DefaultObject;
	}

	SpecConstructor(&Spec, Ability, 1, -1, EmoteDef);
	GiveAbilityAndActivateOnce(((AFortPlayerStateAthena*)PC->PlayerState)->AbilitySystemComponent, &Spec.Handle, Spec);
}

void ServerPlaySquadQuickChatMessage(AFortPlayerControllerAthena* PC, FAthenaQuickChatActiveEntry ChatEntry, __int64) {
	Log("ServerPlaySquadQuickChatMessage Called!");

	static ETeamMemberState MemberStates[10] = {
		ETeamMemberState::ChatBubble,
		ETeamMemberState::EnemySpotted,
		ETeamMemberState::NeedMaterials,
		ETeamMemberState::NeedBandages,
		ETeamMemberState::NeedShields,
		ETeamMemberState::NeedAmmoHeavy,
		ETeamMemberState::NeedAmmoLight,
		ETeamMemberState::FIRST_CHAT_MESSAGE,
		ETeamMemberState::NeedAmmoMedium,
		ETeamMemberState::NeedAmmoShells
	};

	auto PlayerState = reinterpret_cast<AFortPlayerStateAthena*>(PC->PlayerState);

	PlayerState->ReplicatedTeamMemberState = MemberStates[ChatEntry.Index];

	/*switch (ChatEntry.Index)
	{
	case 0:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::ChatBubble;
		break;
	case 1:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::EnemySpotted;
		break;
	case 2:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedMaterials;
		break;
	case 3:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedBandages;
		break;
	case 4:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedShields;
		break;
	case 5:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedAmmoHeavy;
		break;
	case 6:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedAmmoLight;
		break;
	case 7:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::FIRST_CHAT_MESSAGE;
		break;
	case 8:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedAmmoMedium;
		break;
	case 9:
		PlayerState->ReplicatedTeamMemberState = ETeamMemberState::NeedAmmoShells;
		break;
	default:
		break;
	}*/

	PlayerState->OnRep_ReplicatedTeamMemberState();

	static auto EmojiComm = StaticFindObject<UAthenaEmojiItemDefinition>(L"/Game/Athena/Items/Cosmetics/Dances/Emoji/Emoji_Comm.Emoji_Comm");

	if (EmojiComm)
	{
		PC->ServerPlayEmoteItem(EmojiComm, 0);
	}
}

void (*OnPossessedPawnDiedOG)(ABP_PhoebePlayerController_C* PC, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum);
void OnPossessedPawnDied(ABP_PhoebePlayerController_C* PC, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum)
{
	if (!PC) {
		return;
	}

	Bot* KilledBot = nullptr;
	for (size_t i = 0; i < Bots.size(); i++)
	{
		auto bot = Bots[i];
		if (bot && bot->PC == PC)
		{
			if (bot->Pawn->GetName().starts_with("BP_Pawn_DangerGrape_")) {
				goto nodrop;
			}
			else {
				KilledBot = bot;
			}
		}
	}

	if (InstigatedBy && DamageCauser) {
		for (auto& bot : PlayerBotArray)
		{
			if (bot && bot->PC && bot->PC == PC && !bot->bIsDead)
			{
				bot->OnDied((AFortPlayerStateAthena*)InstigatedBy->PlayerState, DamageCauser, BoneName);
				break;
			}
		}
	}

	PC->PlayerBotPawn->SetMaxShield(0);
	for (int32 i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) || PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()))
			continue;
		auto Def = PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition;
		if (Def->GetName() == "AGID_Athena_Keycard_Yacht") {
			goto nodrop;
		}
		if (Def->GetName() == "WID_Boss_Tina") {
			Def = KilledBot->Weapon;
		}
		SpawnPickup(Def, PC->Inventory->Inventory.ReplicatedEntries[i].Count, PC->Inventory->Inventory.ReplicatedEntries[i].LoadedAmmo, PC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::PlayerElimination);
		int Ammo = 0;
		int AmmoC = 0;
		if (Def->GetName() == "WID_Boss_Hos_MG") {
			Ammo = 60;
			AmmoC = 60;
		}
		else if (Def->GetName().starts_with("WID_Assault_LMG_Athena_")) {
			Ammo = 45;
			AmmoC = 45;
		}
		if (Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
		{
			UFortWeaponItemDefinition* Def2 = (UFortWeaponItemDefinition*)Def;
			SpawnPickup(Def2->GetAmmoWorldItemDefinition_BP(), AmmoC != 0 ? AmmoC : GetClipSize(Def2), Ammo != 0 ? Ammo : GetClipSize(Def2), PC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::PlayerElimination);
		}
	}

nodrop:
	for (size_t i = 0; i < Bots.size(); i++)
	{
		auto bot = Bots[i];
		if (bot && bot->PC == PC)
		{
			Bots.erase(Bots.begin() + i);
			break;
		}
	}

	return OnPossessedPawnDiedOG(PC, DamagedActor, Damage, InstigatedBy, DamageCauser, HitLocation, HitComp, BoneName, Momentum);
}

void ServerReturnToMainMenu(AFortPlayerControllerAthena* PC)
{
	PC->ClientReturnToMainMenu(L"");
}

void ServerReviveFromDBNO(AFortPlayerPawnAthena* Pawn, AFortPlayerControllerAthena* Instigator)
{
	float ServerTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

	if (!Pawn || !Instigator)
		return;

	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;
	if (!PC || !PC->PlayerState)
		return;
	auto PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
	auto AbilitySystemComp = (UFortAbilitySystemComponentAthena*)PlayerState->AbilitySystemComponent;

	FGameplayEventData Data{};
	Data.EventTag = Pawn->EventReviveTag;
	Data.ContextHandle = PlayerState->AbilitySystemComponent->MakeEffectContext();
	Data.Instigator = Instigator;
	Data.Target = Pawn;
	Data.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Pawn);
	Data.TargetTags = Pawn->GameplayTags;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Pawn, Pawn->EventReviveTag, Data);

	for (auto& Ability : AbilitySystemComp->ActivatableAbilities.Items)
	{
		if (Ability.Ability->Class == UGAB_AthenaDBNO_C::StaticClass())
		{
			AbilitySystemComp->ServerCancelAbility(Ability.Handle, Ability.ActivationInfo);
			AbilitySystemComp->ServerEndAbility(Ability.Handle, Ability.ActivationInfo, Ability.ActivationInfo.PredictionKeyWhenActivated);
			AbilitySystemComp->ClientCancelAbility(Ability.Handle, Ability.ActivationInfo);
			AbilitySystemComp->ClientEndAbility(Ability.Handle, Ability.ActivationInfo);
			break;
		}
	}

	Pawn->bIsDBNO = false;
	Pawn->OnRep_IsDBNO();
	Pawn->SetHealth(30);
	PlayerState->DeathInfo = {};
	PlayerState->OnRep_DeathInfo();

	PC->ClientOnPawnRevived(Instigator);
}
