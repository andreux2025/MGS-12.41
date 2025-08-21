#pragma once
#include "Utils.h"
#include "Globals.h"
//#include "PC.h"
#include "Bosses.h"
#include "BotBlockedIds.h"
#include "BotNames.h"
#include "Quests.h"
#include "Siphon.h"

static vector<UAthenaCharacterItemDefinition*> CIDs{};
static vector<UAthenaPickaxeItemDefinition*> Pickaxes{};
static vector<UAthenaBackpackItemDefinition*> Backpacks{};
static vector<UAthenaGliderItemDefinition*> Gliders{};
static vector<UAthenaSkyDiveContrailItemDefinition*> Contrails{};
inline vector<UAthenaDanceItemDefinition*> Dances{};

enum EBotState : uint8 {
    Warmup,
    PreBus, // Dont handle this, just there to stop bots from killing eachother before bus enters dropzone
    Bus,
    Skydiving,
    Gliding,
    Landed,
    Fleeing,
    Looting,
    LookingForPlayers
};

enum EBotWarmupChoice : uint8 {
    FindAndPickupGun,
    ShootOtherPlayers,
    PickaxeOtherPlayers,
    MoveToPlayerAndEmote,
    Emote,
    MAX
};

enum class ELootableType {
    None = -1,
    Chest = 0,
    Pickup = 1 
};

enum class EBotCombatSituation
{
    CloseRange,
    MidRange,
    LongRange,
    OutOfAmmo,
    NoWeapon,
    Default
};

struct PlayerBot
{
public:
    uint64_t tick_counter = 0;
    AFortPlayerPawnAthena* Pawn = nullptr;
    ABP_PhoebePlayerController_C* PC = nullptr;
    AFortPlayerStateAthena* PlayerState = nullptr;
    FString DisplayName = L"Bot";
    EBotState BotState = EBotState::Warmup;
    EBotWarmupChoice WarmupChoice = EBotWarmupChoice::MAX;
    EBotCombatSituation CombatSituation = EBotCombatSituation::Default;
    bool bIsDead = false;
    bool bHasThankedBusDriver = false;
    bool bIsMoving = false;
    bool bIsCurrentlyStrafing = false;
    bool bIsCrouching = false;
    bool bIsEmoting = false;
    bool bIsRunning = false;
    bool bPickaxeEquiped = true;
    bool bStartedLootAttempt = false;
    bool bIsInWeaponOverride = false;
    AActor* TargetActor = nullptr;
    AActor* TargetPOI = nullptr;
    AActor* TargetLootableActor = nullptr;
    float TimeToNextAction = 0.f;
    float LikelyHoodToDoAction = 0.f;
    float LootAttemptStartTime = 0.f;

public:
    void OnDied(AFortPlayerStateAthena* KillerState, AActor* DamageCauser, FName BoneName)
    {
        static auto Acc_DistanceShot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_DistanceShot.AccoladeId_DistanceShot");
        static auto Acc_LongShot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_051_LongShot.AccoladeId_051_LongShot");
        static auto Acc_LudicrousShot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_052_LudicrousShot.AccoladeId_052_LudicrousShot");
        static auto Acc_ImpossibleShot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_053_ImpossibleShot.AccoladeId_053_ImpossibleShot");
        static auto Acc_Headshot = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_048_HeadshotElim.AccoladeId_048_HeadshotElim");

        static auto Acc_Survival_Bronze = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_026_Survival_Default_Bronze.AccoladeId_026_Survival_Default_Bronze");
        static auto Acc_Survival_Silver = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_027_Survival_Default_Silver.AccoladeId_027_Survival_Default_Silver");
        static auto Acc_Survival_Gold = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_028_Survival_Default_Gold.AccoladeId_028_Survival_Default_Gold");

        Log("Bot Died");

        bIsDead = true;

        if (!KillerState || !DamageCauser || !PlayerState || !Pawn)
            return;

        FDeathInfo& DeathInfo = PlayerState->DeathInfo;

        DeathInfo.bDBNO = Pawn->bWasDBNOOnDeath;
        //DeathInfo.DeathLocation = Pawn->K2_GetActorLocation(); This prolly wont work since the bot is dead
        DeathInfo.DeathTags = Pawn->DeathTags;
        DeathInfo.Downer = KillerState ? KillerState : nullptr;
        AFortPawn* KillerPawn = KillerState ? KillerState->GetCurrentPawn() : nullptr;
        DeathInfo.Distance = (KillerPawn && Pawn) ? KillerPawn->GetDistanceTo(Pawn) : 0.f;
        DeathInfo.FinisherOrDowner = KillerState ? KillerState : nullptr;
        DeathInfo.DeathCause = PlayerState->ToDeathCause(DeathInfo.DeathTags, DeathInfo.bDBNO);
        DeathInfo.bInitialized = true;
        PlayerState->OnRep_DeathInfo();

        bool bIsKillerABot = KillerState->bIsABot;

        if (!PC->Inventory)
            return;

        auto KillerPC = (AFortPlayerControllerAthena*)KillerState->GetOwner();

        if (Globals::SiphonEnabled)
        {
            static auto Class = StaticLoadObject<UClass>("/Game/Creative/Abilities/Siphon/GA_Creative_OnKillSiphon.GA_Creative_OnKillSiphon_C");

            AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;

            for (size_t i = 0; i < PlayerState->AbilitySystemComponent->ActivatableAbilities.Items.Num(); i++)
            {
                if (PlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].Ability->Class == Class)
                {
                    FGameplayEffectContextHandle Handle = PlayerState->AbilitySystemComponent->MakeEffectContext();
                    FGameplayTag tag{};
                    static auto Animation = UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(TEXT("GameplayCue.Shield.PotionConsumed"));
                    tag.TagName = Animation;
                    PlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(tag, PlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].ActivationInfo.PredictionKeyWhenActivated, Handle);
                    PlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(tag, PlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].ActivationInfo.PredictionKeyWhenActivated, Handle);

                    int health = KillerPC->MyFortPawn->GetHealth() + 50;
                    bool GiveShield = health > 100;
                    int ShieldAmount = KillerPC->MyFortPawn->GetShield() + (health - 100);
                    if (health > 100)
                        health = 100;

                    if (ShieldAmount > 100)
                        ShieldAmount = 100;

                    KillerPC->MyFortPawn->SetHealth(health);
                    if (GiveShield)
                        KillerPC->MyFortPawn->SetShield(ShieldAmount);

                    static UFortItemDefinition* Wood = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
                    static UFortItemDefinition* Stone = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
                    static UFortItemDefinition* Metal = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/MetalItemData.MetalItemData");

                    GiveItem(KillerPC, Wood, 50, 0);
                    GiveItem(KillerPC, Stone, 50, 0);
                    GiveItem(KillerPC, Metal, 50, 0);
                    break;
                }
            }
        }

        if (KillerPC && KillerPC->IsA(AFortPlayerControllerAthena::StaticClass()) && !bIsKillerABot)
        {
            KillerState->KillScore++;
            GiveAccolade(KillerPC, GetDefFromEvent(EAccoladeEvent::Kill, KillerState->KillScore));

            for (size_t i = 0; i < KillerState->PlayerTeam->TeamMembers.Num(); i++)
            {
                ((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->TeamKillScore++;
                ((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->OnRep_TeamKillScore();
            }

            KillerState->ClientReportKill(PlayerState);
            KillerState->OnRep_Kills();

            /*auto KillerPawn = KillerPC->MyFortPawn;

            if (GameState->PlayersLeft && GameState->PlayerBotsLeft <= 1 && !GameState->IsRespawningAllowed(PlayerState)) // This crashes it lol
            {
                if (KillerPawn != Pawn)
                {
                    UFortWeaponItemDefinition* KillerWeaponDef = nullptr;

                    if (auto ProjectileBase = Cast<AFortProjectileBase>(DamageCauser))
                        KillerWeaponDef = ((AFortWeapon*)ProjectileBase->GetOwner())->WeaponData;
                    if (auto Weapon = Cast<AFortWeapon>(DamageCauser))
                        KillerWeaponDef = Weapon->WeaponData;

                    KillerPC->PlayWinEffects(KillerPawn, KillerWeaponDef, DeathInfo.DeathCause, false);
                    KillerPC->ClientNotifyWon(KillerPawn, KillerWeaponDef, DeathInfo.DeathCause);
                }

                KillerState->Place = 1;
                KillerState->OnRep_Place();
                GameState->WinningPlayerState = KillerState;
                GameState->WinningTeam = KillerState->TeamIndex;
                GameState->OnRep_WinningPlayerState();
                GameState->OnRep_WinningTeam();
                GameMode->EndMatch();
            }*/
        }

        if (bIsKillerABot) {
            Log("got to the end!");
            return;
        }

        float Distance = DeathInfo.Distance / 100.0f;

        if (Distance >= 100.0f && Distance < 150.0f)
        {
            GiveAccolade(KillerPC, Acc_DistanceShot); // 100-149m accolade
        }
        else if (Distance >= 150.0f && Distance < 200.0f)
        {
            GiveAccolade(KillerPC, Acc_LongShot); // 150-199m accolade
        }
        else if (Distance >= 200.0f && Distance < 250.0f)
        {
            GiveAccolade(KillerPC, Acc_LudicrousShot); // 200-249m accolade
        }
        else if (Distance >= 250.0f)
        {
            GiveAccolade(KillerPC, Acc_ImpossibleShot); // 250+m accolade
        }

        if (BoneName.ToString() == "head")
        {
            GiveAccolade(KillerPC, Acc_Headshot); // headshot accolade
        }

        AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
        if (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num() == 50)
        {
            for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
            {
                GiveAccolade(GameMode->AlivePlayers[i], Acc_Survival_Bronze);
            }
        }
        if (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num() == 25)
        {
            for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
            {
                GiveAccolade(GameMode->AlivePlayers[i], Acc_Survival_Silver);
            }
        }
        if (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num() == 10)
        {
            for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
            {
                GiveAccolade(GameMode->AlivePlayers[i], Acc_Survival_Gold);
            }
        }

        Log("Got to the end");
    }

    void LookAt(AActor* Actor)
    {
        if (!Pawn || PC->GetFocusActor() == Actor)
            return;

        if (!Actor)
        {
            PC->K2_ClearFocus();
            return;
        }

        PC->K2_SetFocus(Actor);
    }

    // took these funcs from ero since they would basically be the same anyway
    void GiveItemBot(UFortItemDefinition* Def, int Count = 1, int LoadedAmmo = 0)
    {
        UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);
        Item->OwnerInventory = PC->Inventory;
        Item->ItemEntry.LoadedAmmo = LoadedAmmo;
        PC->Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
        PC->Inventory->Inventory.ItemInstances.Add(Item);
        PC->Inventory->Inventory.MarkItemDirty(Item->ItemEntry);
        PC->Inventory->HandleInventoryLocalUpdate();
    }

    FFortItemEntry* GetEntry(UFortItemDefinition* Def)
    {
        for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
        {
            if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
                return &PC->Inventory->Inventory.ReplicatedEntries[i];
        }

        return nullptr;
    }

    ABuildingSMActor* FindNearestBuildingSMActor()
    {
        static TArray<AActor*> Array;
        static bool PopulatedArray = false;
        if (!PopulatedArray)
        {
            PopulatedArray = true;
            UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ABuildingSMActor::StaticClass(), &Array);
        }

        AActor* NearestPoi = nullptr;

        for (size_t i = 0; i < Array.Num(); i++)
        {
            if (!NearestPoi || (((ABuildingSMActor*)NearestPoi)->GetHealth() < 1500 && ((ABuildingSMActor*)NearestPoi)->GetHealth() > 1 && Array[i]->GetDistanceTo(Pawn) < NearestPoi->GetDistanceTo(Pawn)))
            {
                NearestPoi = Array[i];
            }
        }

        return (ABuildingSMActor*)NearestPoi;
    }

    ABuildingActor* FindNearestChest()
    {
        // ill make support for finding the closest chest and rare chest if its even in this gs
        static auto ChestClass = StaticLoadObject<UClass>("/Game/Building/ActorBlueprints/Containers/Tiered_Chest_Athena.Tiered_Chest_Athena_C");
        static auto FactionChestClass = StaticLoadObject<UClass>("/Game/Building/ActorBlueprints/Containers/Tiered_Chest_Athena_FactionChest.Tiered_Chest_Athena_FactionChest_C");
        TArray<AActor*> Array;
        TArray<AActor*> FactionChests;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ChestClass, &Array);
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), FactionChestClass, &FactionChests);

        AActor* NearestChest = nullptr;

        for (size_t i = 0; i < Array.Num(); i++)
        {
            AActor* Chest = Array[i];
            if (Chest->bHidden)
                continue;

            if (!NearestChest || Chest->GetDistanceTo(Pawn) < NearestChest->GetDistanceTo(Pawn))
            {
                NearestChest = Array[i];
            }
        }

        for (size_t i = 0; i < FactionChests.Num(); i++)
        {
            AActor* Chest = FactionChests[i];
            if (Chest->bHidden)
                continue;

            if (!NearestChest || Chest->GetDistanceTo(Pawn) < NearestChest->GetDistanceTo(Pawn))
            {
                NearestChest = FactionChests[i];
            }
        }
        Array.Free();
        FactionChests.Free();
        return (ABuildingActor*)NearestChest;
    }

    AFortPickupAthena* FindNearestPickup()
    {
        static auto PickupClass = AFortPickupAthena::StaticClass();
        TArray<AActor*> Array;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), PickupClass, &Array);
        AActor* NearestPickup = nullptr;

        for (size_t i = 0; i < Array.Num(); i++)
        {
            if (Array[i]->bHidden)
                continue;

            if (!NearestPickup || Array[i]->GetDistanceTo(Pawn) < NearestPickup->GetDistanceTo(Pawn))
            {
                NearestPickup = Array[i];
            }
        }

        Array.Free();
        return (AFortPickupAthena*)NearestPickup;
    }

    bool GetNearestLootable() {
        static auto ChestClass = StaticLoadObject<UClass>("/Game/Building/ActorBlueprints/Containers/Tiered_Chest_Athena.Tiered_Chest_Athena_C");
        static auto FactionChestClass = StaticLoadObject<UClass>("/Game/Building/ActorBlueprints/Containers/Tiered_Chest_Athena_FactionChest.Tiered_Chest_Athena_FactionChest_C");
        TArray<AActor*> ChestArray;
        TArray<AActor*> FactionChests;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ChestClass, &ChestArray);
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), FactionChestClass, &FactionChests);

        static auto PickupClass = AFortPickupAthena::StaticClass();
        TArray<AActor*> PickupArray;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), PickupClass, &PickupArray);
        
        AActor* NearestPickup = nullptr;
        AActor* NearestChest = nullptr;

        for (size_t i = 0; i < ChestArray.Num(); i++)
        {
            AActor* Chest = ChestArray[i];
            if (Chest->bHidden)
                continue;

            if (!NearestChest || Chest->GetDistanceTo(Pawn) < NearestChest->GetDistanceTo(Pawn))
            {
                NearestChest = ChestArray[i];
            }
        }

        for (size_t i = 0; i < FactionChests.Num(); i++)
        {
            AActor* Chest = FactionChests[i];
            if (Chest->bHidden)
                continue;

            if (!NearestChest || Chest->GetDistanceTo(Pawn) < NearestChest->GetDistanceTo(Pawn))
            {
                NearestChest = FactionChests[i];
            }
        }

        for (size_t i = 0; i < PickupArray.Num(); i++)
        {
            if (PickupArray[i]->bHidden)
                continue;

            if (!NearestPickup || PickupArray[i]->GetDistanceTo(Pawn) < NearestPickup->GetDistanceTo(Pawn))
            {
                NearestPickup = PickupArray[i];
            }
        }

        ChestArray.Free();
        FactionChests.Free();
        PickupArray.Free();
        return NearestPickup->GetDistanceTo(Pawn) > NearestChest->GetDistanceTo(Pawn);
    }

    FVector FindNearestPlayerOrBot() {
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

        AActor* NearestPlayer = nullptr;

        for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
        {
            if (!NearestPlayer || (GameMode->AlivePlayers[i]->Pawn && GameMode->AlivePlayers[i]->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn)))
            {
                NearestPlayer = GameMode->AlivePlayers[i]->Pawn;
            }
        }

        for (size_t i = 0; i < GameMode->AliveBots.Num(); i++)
        {
            if (GameMode->AliveBots[i]->Pawn != Pawn)
            {
                if (!NearestPlayer || (GameMode->AliveBots[i]->Pawn && GameMode->AliveBots[i]->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn)))
                {
                    NearestPlayer = GameMode->AliveBots[i]->Pawn;
                }
            }
        }

        for (size_t i = 0; i < Bots.size(); i++) {
            if (!NearestPlayer || (Bots[i]->Pawn && Bots[i]->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn)))
            {
                NearestPlayer = Bots[i]->Pawn;
            }
        }

        return NearestPlayer->K2_GetActorLocation();
    }

    AActor* GetNearestPlayerActor() {
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

        AActor* NearestPlayer = nullptr;

        for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
        {
            if (!NearestPlayer || (GameMode->AlivePlayers[i]->Pawn && GameMode->AlivePlayers[i]->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn)))
            {
                NearestPlayer = GameMode->AlivePlayers[i]->Pawn;
            }
        }

        for (size_t i = 0; i < GameMode->AliveBots.Num(); i++)
        {
            if (GameMode->AliveBots[i]->Pawn != Pawn)
            {
                if (!NearestPlayer || (GameMode->AliveBots[i]->Pawn && GameMode->AliveBots[i]->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn)))
                {
                    NearestPlayer = GameMode->AliveBots[i]->Pawn;
                }
            }
        }

        for (size_t i = 0; i < Bots.size(); i++) {
            if (!NearestPlayer || (Bots[i]->Pawn && Bots[i]->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn)))
            {
                NearestPlayer = Bots[i]->Pawn;
            }
        }

        return NearestPlayer;
    }

    bool IsInventoryFull()
    {
        int ItemNb = 0;
        auto InstancesPtr = &PC->Inventory->Inventory.ItemInstances;
        for (int i = 0; i < InstancesPtr->Num(); i++)
        {
            if (InstancesPtr->operator[](i))
            {
                if (GetQuickBars(InstancesPtr->operator[](i)->ItemEntry.ItemDefinition) == EFortQuickBars::Primary)
                {
                    ItemNb++;

                    if (ItemNb >= 5)
                    {
                        break;
                    }
                }
            }
        }

        return ItemNb >= 5;
    }

    void Pickup(AFortPickup* Pickup) {
        GiveItemBot(Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
        if (((UFortWeaponItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition)->GetAmmoWorldItemDefinition_BP() && ((UFortWeaponItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition)->GetAmmoWorldItemDefinition_BP() != Pickup->PrimaryPickupItemEntry.ItemDefinition)
        {
            GiveItemBot(((UFortWeaponItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition)->GetAmmoWorldItemDefinition_BP(), 60);
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

    void PickupAllItemsInRange(float Range = 320.f) {
        static auto PickupClass = AFortPickupAthena::StaticClass();
        TArray<AActor*> Array;
        UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), PickupClass, &Array);

        for (size_t i = 0; i < Array.Num(); i++)
        {
            if (Array[i]->GetDistanceTo(Pawn) < Range)
            {
                Pickup((AFortPickupAthena*)Array[i]);
            }
        }

        Array.Free();
    }

    void EquipPickaxe()
    {
        if (!Pawn || !Pawn->CurrentWeapon)
            return;

        if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
        {
            for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
            {
                if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
                {
                    Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition, PC->Inventory->Inventory.ReplicatedEntries[i].ItemGuid);
                    break;
                }
            }
        }
    }

    bool IsPickaxeEquiped() {
        if (!Pawn || !Pawn->CurrentWeapon)
            return false;

        if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
        {
            return true;
        }
        return false;
    }

    bool HasGun()
    {
        for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
        {
            auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
            if (Entry.ItemDefinition) {
                std::string ItemName = Entry.ItemDefinition->Name.ToString();
                if (ItemName.contains("Shotgun") || ItemName.contains("SMG") || ItemName.contains("Assault")
                    || ItemName.contains("Grenade") || ItemName.contains("Sniper") || ItemName.contains("Rocket") || ItemName.contains("Pistol")) {
                    return true;
                    break;
                }
            }
        }
        return false;
    }

    void SimpleSwitchToWeapon() {
        if (!HasGun()) {
            return;
        }

        if (!Pawn || !Pawn->CurrentWeapon || !Pawn->CurrentWeapon->WeaponData || !PC || !PC->Inventory || bIsDead)
            return;

        if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass())) {
            return;
        }

        if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
        {
            for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
            {
                auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
                if (Entry.ItemDefinition) {
                    std::string ItemName = Entry.ItemDefinition->Name.ToString();
                    if (ItemName.contains("Shotgun") || ItemName.contains("SMG") || ItemName.contains("Assault")
                        || ItemName.contains("Grenade") || ItemName.contains("Sniper") || ItemName.contains("Rocket") || ItemName.contains("Pistol")) {
                        Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry.ItemDefinition, Entry.ItemGuid);
                        break;
                    }
                }
            }
        }
    }

    bool HasWeaponTypeBasedOnSituation() {
        if (!PC || !PC->Inventory || bIsDead)
            return false;

        for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
        {
           auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];

           if (Entry.ItemDefinition) {
               std::string ItemName = Entry.ItemDefinition->Name.ToString();
               switch (CombatSituation) {
               case EBotCombatSituation::CloseRange:
                   if (ItemName.contains("Shotgun") || ItemName.contains("SMG") || ItemName.contains("Assault") || ItemName.contains("Pistol")) {
                       return true;
                   }
                   break;
               case EBotCombatSituation::MidRange:
                   if (ItemName.contains("SMG") || ItemName.contains("Assault") || ItemName.contains("Grenade") || ItemName.contains("Pistol")) {
                       return true;
                   }
                   break;
               case EBotCombatSituation::LongRange:
                   if (ItemName.contains("Assault") || ItemName.contains("Sniper") || ItemName.contains("Rocket") || ItemName.contains("Grenade")) {
                       return true;
                   }
                   break;
               default:
                   if (Entry.ItemDefinition->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
                   {
                       return true;
                   }
                   break;
               }
           }
		}

        return false;
    }

    bool HasSituatedWeaponEquiped() {
        if (!Pawn || !Pawn->CurrentWeapon || !Pawn->CurrentWeapon->WeaponData || !PC || !PC->Inventory || bIsDead)
            return false;

        if (!HasWeaponTypeBasedOnSituation())
            return false;

        auto& Entry = Pawn->CurrentWeapon;
        std::string ItemName = Entry->Name.ToString();

        switch (CombatSituation) {
        case EBotCombatSituation::CloseRange:
            if (ItemName.contains("Shotgun") || ItemName.contains("SMG") || ItemName.contains("Assault") || ItemName.contains("Pistol")) {
                return true;
            }
            break;
        case EBotCombatSituation::MidRange:
            if (ItemName.contains("SMG") || ItemName.contains("Assault") || ItemName.contains("Pistol") || ItemName.contains("Grenade") || ItemName.contains("Pistol")) {
                return true;
            }
            break;
        case EBotCombatSituation::LongRange:
            if (ItemName.contains("Assault") || ItemName.contains("Sniper") || ItemName.contains("Rocket") || ItemName.contains("Grenade")) {
                return true;
            }
            break;
        }

        return false;
    }

    void AdvancedSwitchToWeapon() {
        if (!Pawn || !Pawn->CurrentWeapon || !Pawn->CurrentWeapon->WeaponData || !PC || !PC->Inventory || bIsDead)
            return;

        if (!HasWeaponTypeBasedOnSituation())
            return;

        if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
        {
            for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
            {
                auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
                bool bFoundWeapon = false;

                if (Entry.ItemDefinition) {
                    std::string ItemName = Entry.ItemDefinition->Name.ToString();
                    switch (CombatSituation) {
                    case EBotCombatSituation::CloseRange:
                        if (ItemName.contains("Shotgun") || ItemName.contains("SMG") || ItemName.contains("Assault") || ItemName.contains("Pistol")) {
                            bFoundWeapon = true;
                        }
                        break;
                    case EBotCombatSituation::MidRange:
                        if (ItemName.contains("SMG") || ItemName.contains("Assault") || ItemName.contains("Grenade") || ItemName.contains("Pistol")) {
                            bFoundWeapon = true;
                        }
                        break;
                    case EBotCombatSituation::LongRange:
                        if (ItemName.contains("Assault") || ItemName.contains("Sniper") || ItemName.contains("Rocket") || ItemName.contains("Grenade")) {
                            bFoundWeapon = true;
                        }
                        break;
                    default:
                        if (ItemName.contains("Shotgun") || ItemName.contains("SMG") || ItemName.contains("Assault")
                            || ItemName.contains("Grenade") || ItemName.contains("Sniper") || ItemName.contains("Rocket") || ItemName.contains("Pistol"))
                        {
                            bFoundWeapon = true;
                        }
                        break;
                    }
                }

                if (bFoundWeapon) {
                    Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry.ItemDefinition, Entry.ItemGuid);
                    return;
                }
            }
        }

        return; 
    } // Switch to different weapons based on the current situation
};
vector<class PlayerBot*> PlayerBotArray{};

void FreeDeadBots() {
    for (size_t i = 0; i < PlayerBotArray.size();)
    {
        if (PlayerBotArray[i]->bIsDead) {
            delete PlayerBotArray[i];
            PlayerBotArray.erase(PlayerBotArray.begin() + i);
            Log("Freed a dead bot from the array!");
        }
        else {
            ++i;
        }
    }
}

void SpawnPlayerBots(AActor* SpawnLocator)
{
    if (!Globals::bPlayerBotsEnabled)
        return;

    static auto BotBP = StaticLoadObject<UClass>("/Game/Athena/AI/Phoebe/BP_PlayerPawn_Athena_Phoebe.BP_PlayerPawn_Athena_Phoebe_C");
    static UBehaviorTree* BehaviorTree = StaticLoadObject<UBehaviorTree>("/Game/Athena/AI/Phoebe/BehaviorTrees/BT_Phoebe.BT_Phoebe");

    if (!BotBP || !BehaviorTree)
        return;

    PlayerBot* bot = new PlayerBot{};

    AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

    static auto BotMutator = (AFortAthenaMutator_Bots*)GameMode->ServerBotManager->CachedBotMutator;

    bot->Pawn = BotMutator->SpawnBot(BotBP, SpawnLocator, SpawnLocator->K2_GetActorLocation(), SpawnLocator->K2_GetActorRotation(), false);

    if (!bot->Pawn || !bot->Pawn->Controller)
        return;

    bot->PC = Cast<ABP_PhoebePlayerController_C>(bot->Pawn->Controller);
    bot->PlayerState = Cast<AFortPlayerStateAthena>(bot->PC->PlayerState);

    if (!bot->PC || !bot->PlayerState)
        return;

    if (!CIDs.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(1)) { // use the random bool so that some bots will be defaults (more realistic)
        // as you could probably tell, i did not write this!
        auto CID = CIDs[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, CIDs.size() - 1)];
        bool CIDInvalid = false;
        for (const std::string& blockedCID : BlockedIDS) {
            if (blockedCID.contains(CID->HeroDefinition->GetName())) {
                CIDInvalid = true;
                Log("Blocked CID found: " + CID->HeroDefinition->GetName());
                break;
            }
        }
        if (CID->HeroDefinition && !CIDInvalid)
        {
            if (CID->HeroDefinition->Specializations.IsValid())
            {
                for (size_t i = 0; i < CID->HeroDefinition->Specializations.Num(); i++)
                {
                    UFortHeroSpecialization* Spec = StaticLoadObject<UFortHeroSpecialization>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(CID->HeroDefinition->Specializations[i].ObjectID.AssetPathName).ToString());
                    if (Spec)
                    {
                        for (size_t j = 0; j < Spec->CharacterParts.Num(); j++)
                        {
                            UCustomCharacterPart* Part = StaticLoadObject<UCustomCharacterPart>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(Spec->CharacterParts[j].ObjectID.AssetPathName).ToString());
                            if (Part)
                            {
                                bot->PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
                            }
                        }
                    }
                }
            }
        }
        bot->PC->CosmeticLoadoutBC.Character = CID;
    }
    if (!Backpacks.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.5)) { // less likely to equip than skin cause lots of ppl prefer not to use backpack
        auto Backpack = Backpacks[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Backpacks.size() - 1)];
        for (size_t j = 0; j < Backpack->CharacterParts.Num(); j++)
        {
            UCustomCharacterPart* Part = Backpack->CharacterParts[j];
            if (Part)
            {
                bot->PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
            }
        }
    }
    if (!Gliders.empty()) {
        auto Glider = Gliders[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Gliders.size() - 1)];
        bot->PC->CosmeticLoadoutBC.Glider = Glider;
    }
    if (!Contrails.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.95)) {
        auto Contrail = Contrails[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Contrails.size() - 1)];
        bot->PC->CosmeticLoadoutBC.SkyDiveContrail = Contrail;
    }

    bot->Pawn->CosmeticLoadout = bot->PC->CosmeticLoadoutBC;
    bot->Pawn->OnRep_CosmeticLoadout();

    if (Pickaxes.empty()) {
        Log("Pickaxes array is empty!");
        UFortWeaponMeleeItemDefinition* PickDef = StaticLoadObject<UFortWeaponMeleeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
        if (PickDef) {
            bot->GiveItemBot(PickDef);
            auto Entry = bot->GetEntry(PickDef);
            bot->Pawn->EquipWeaponDefinition((UFortWeaponMeleeItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid);
        }
        else {
            Log("Default Pickaxe dont exist!");
        }
    }
    else {
        auto PickDef = Pickaxes[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Pickaxes.size() - 1)];
        if (!PickDef)
        {
            Log("Cooked!");
            UFortWeaponMeleeItemDefinition* PickDef = StaticLoadObject<UFortWeaponMeleeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
            if (PickDef) {
                bot->GiveItemBot(PickDef);
                auto Entry = bot->GetEntry(PickDef);
                bot->Pawn->EquipWeaponDefinition((UFortWeaponMeleeItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid);
            }
            else {
                Log("Default Pickaxe dont exist!");
            }
        }
        else {
            if (PickDef && PickDef->WeaponDefinition)
            {
                bot->GiveItemBot(PickDef->WeaponDefinition);
            }

            auto Entry = bot->GetEntry(PickDef->WeaponDefinition);
            bot->Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid);
        }
    }

    if (BotDisplayNames.size() != 0) {
        std::srand(static_cast<unsigned int>(std::time(0)));
        int randomIndex = std::rand() % BotDisplayNames.size();
        std::string rdName = BotDisplayNames[randomIndex];
        BotDisplayNames.erase(BotDisplayNames.begin() + randomIndex);

        int size_needed = MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), NULL, 0);
        std::wstring wideString(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), &wideString[0], size_needed);


        FString CVName = FString(wideString.c_str());
        GameMode->ChangeName(bot->PC, CVName, true);
        bot->DisplayName = CVName;


        bot->PlayerState->OnRep_PlayerName();
    }

    static UBlackboardData* BlackboardData = StaticLoadObject<UBlackboardData>("/Game/Athena/AI/Phoebe/BehaviorTrees/BB_Phoebe.BB_Phoebe");

    for (auto SkillSet : bot->PC->DigestedBotSkillSets) // are they even needed
    {
        if (!SkillSet)
            continue;

        if (auto AimingSkill = Cast<UFortAthenaAIBotAimingDigestedSkillSet>(SkillSet))
            bot->PC->CacheAimingDigestedSkillSet = AimingSkill;

        if (auto HarvestSkill = Cast<UFortAthenaAIBotHarvestDigestedSkillSet>(SkillSet))
            bot->PC->CacheHarvestDigestedSkillSet = HarvestSkill;

        if (auto InventorySkill = Cast<UFortAthenaAIBotInventoryDigestedSkillSet>(SkillSet))
            bot->PC->CacheInventoryDigestedSkillSet = InventorySkill;

        if (auto LootingSkill = Cast<UFortAthenaAIBotLootingDigestedSkillSet>(SkillSet))
            bot->PC->CacheLootingSkillSet = LootingSkill;

        if (auto MovementSkill = Cast<UFortAthenaAIBotMovementDigestedSkillSet>(SkillSet))
            bot->PC->CacheMovementSkillSet = MovementSkill;

        if (auto PerceptionSkill = Cast<UFortAthenaAIBotPerceptionDigestedSkillSet>(SkillSet))
            bot->PC->CachePerceptionDigestedSkillSet = PerceptionSkill;

        if (auto PlayStyleSkill = Cast<UFortAthenaAIBotPlayStyleDigestedSkillSet>(SkillSet))
            bot->PC->CachePlayStyleSkillSet = PlayStyleSkill;
    }

    bot->PC->BehaviorTree = BehaviorTree;
    bot->PC->RunBehaviorTree(BehaviorTree);
    bot->PC->UseBlackboard(BehaviorTree->BlackboardAsset, &bot->PC->Blackboard);
    bot->PC->UseBlackboard(BehaviorTree->BlackboardAsset, &bot->PC->Blackboard1);

    bot->Pawn->SetMaxHealth(100);
    bot->Pawn->SetHealth(100);
    bot->Pawn->SetMaxShield(100);
    bot->Pawn->SetShield(0);

    PlayerBotArray.push_back(bot); // gotta do this so we can tick them all
    //Log("Bot Spawned With DisplayName: " + bot->DisplayName.ToString());
}

void PlayerBotTick() {
    if (!PlayerBotArray.empty()) {
        if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.001f))
        {
            FreeDeadBots();
        }
    }
    else {
        return;
    }

    auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
    auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
    auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

    for (PlayerBot* bot : PlayerBotArray)
    {
        if (!bot || !bot->Pawn || !bot->PC || !bot->PlayerState || bot->bIsDead)
            continue;

        if (bot->tick_counter <= 150) {
            bot->tick_counter++;
            continue;
        }

        if (Globals::bPlayerBotsPerformanceModeEnabled) {
            if (bot->tick_counter % 2 != 0) {
                bot->tick_counter++;
                continue;
            }
        }

        FVector Vel = bot->Pawn->GetVelocity();
        float Speed = sqrtf(Vel.X * Vel.X + Vel.Y * Vel.Y);

        if (Speed < 0.5f && bot->bIsMoving) {
            if (!bot->IsPickaxeEquiped()) {
                bot->EquipPickaxe();
                bot->bPickaxeEquiped = true;
            }
            bot->Pawn->PawnStartFire(0);
        }
        else {
            bot->Pawn->PawnStopFire(0);
        }

        /*if (bot->bIsMoving && !bot->bIsRunning) {
            bot->Run();
        }*/

        if (bot->bIsMoving && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.001)) {
            bot->Pawn->Jump();
        }

        if (bot->BotState == EBotState::Warmup) {
            if (bot->WarmupChoice == EBotWarmupChoice::MAX) {
                if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.04)) {
                    bot->WarmupChoice = EBotWarmupChoice::FindAndPickupGun;
                }
                else if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.02)) {
                    bot->WarmupChoice = EBotWarmupChoice::PickaxeOtherPlayers;
                }
                else if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.02)) {
                    bot->WarmupChoice = EBotWarmupChoice::MoveToPlayerAndEmote;
                }
                else {
                    bot->WarmupChoice = EBotWarmupChoice::Emote;
                }
            }
            else if (bot->WarmupChoice == EBotWarmupChoice::FindAndPickupGun) {
                if (!bot->bIsMoving) {
                    bot->bIsMoving = true;
                }

                AFortPickup* Pickup = bot->FindNearestPickup();
                if (!Pickup) {
                    continue;
                }
                FVector BotLoc = bot->Pawn->K2_GetActorLocation();
                float Dist = Math->Vector_Distance(BotLoc, Pickup->K2_GetActorLocation());
                if (Dist < 250.f) {
                    bot->Pawn->PawnStopFire(0);
                    bot->PC->StopMovement();
                    if (Pickup)
                    {
                        bot->PickupAllItemsInRange(500.f);
                        bot->SimpleSwitchToWeapon();
                    }

                    bot->TimeToNextAction = 0;
                    bot->WarmupChoice = EBotWarmupChoice::ShootOtherPlayers;
                }
                else {
                    float VerticalOffset = Pickup->K2_GetActorLocation().Z - BotLoc.Z;
                    FVector Vel = bot->Pawn->GetVelocity();
                    float Speed = sqrtf(Vel.X * Vel.X + Vel.Y * Vel.Y);

                    if (Speed < 0.3f) {
                        bot->Pawn->PawnStartFire(0);
                    }
                    else {
                        bot->Pawn->PawnStopFire(0);
                    }

                    auto BotPos = bot->Pawn->K2_GetActorLocation();
                    /*auto TestRot = Math->FindLookAtRotation(BotPos, Pickup->K2_GetActorLocation());

                    bot->PC->SetControlRotation(TestRot);
                    bot->PC->K2_SetActorRotation(TestRot, true);*/
                    bot->LookAt(Pickup);
                    bot->PC->MoveToActor(Pickup, 0, true, false, true, nullptr, true);
                    //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
                }
            }
            else if (bot->WarmupChoice == EBotWarmupChoice::ShootOtherPlayers) {
                if (!bot->HasGun()) {
                    bot->WarmupChoice = EBotWarmupChoice::FindAndPickupGun;
                }
                if (!bot->bIsMoving) {
                    bot->bIsMoving = true;
                }
                FVector BotLoc = bot->Pawn->K2_GetActorLocation();
                AActor* NearestPlayerActor = bot->GetNearestPlayerActor();
                if (!NearestPlayerActor) {
                    continue;
                }
                FVector Nearest = NearestPlayerActor->K2_GetActorLocation();

                FRotator TestRot;
                FVector TargetPosmod = Nearest;

                if (!Nearest.IsZero()) {
                    float Dist = Math->Vector_Distance(BotLoc, Nearest);
                    if (Dist < 500 + rand() % 1000 && bot->PC->LineOfSightTo(NearestPlayerActor, {}, true)) {
                        if (bot->tick_counter % 30 == 0) {
                            //float RandomXmod = ((rand() % 120000 - rand() % 30000) + 90000);
                            float RandomYmod = ((rand() % 50) - rand() % 100);
                            float RandomZmod = ((rand() % 50) - rand() % 100);

                            FVector TargetPosMod{ Nearest.X, Nearest.Y + RandomYmod, Nearest.Z + RandomZmod };

                            TestRot = Math->FindLookAtRotation(BotLoc, TargetPosMod);
                        }

                        if (bot->tick_counter % 30 == 0) {
                            bot->PC->SetControlRotation(TestRot);
                            bot->PC->K2_SetActorRotation(TestRot, true);
                        }

                        if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.05)) {
                            TestRot = Math->FindLookAtRotation(BotLoc, Nearest);

                            bot->PC->SetControlRotation(TestRot);
                            bot->PC->K2_SetActorRotation(TestRot, true);
                        }

                        bot->PC->MoveToActor(NearestPlayerActor, rand() % 500 + 500, true, false, true, nullptr, true);

                        if (!bot->bIsCrouching && UKismetMathLibrary::RandomBoolWithWeight(0.03)) {
                            bot->bIsCrouching = true;
                            bot->Pawn->Crouch(false);
                        }

                        if (bot->bIsCrouching && UKismetMathLibrary::RandomBoolWithWeight(0.03)) {
                            bot->bIsCrouching = false;
                            bot->Pawn->UnCrouch(false);
                        }

                        if (!bot->bIsCurrentlyStrafing) {
                            if (UKismetMathLibrary::RandomBoolWithWeight(0.05)) {
                                std::thread([bot]() {
                                    bot->bIsCurrentlyStrafing = true;
                                    auto start = std::chrono::high_resolution_clock::now();
                                    auto duration = std::chrono::milliseconds(3000);
                                    while (std::chrono::high_resolution_clock::now() - start < duration) {
                                        bot->Pawn->AddMovementInput(bot->Pawn->GetActorRightVector(), 1.1f, true);
                                    }
                                    bot->bIsCurrentlyStrafing = false;
                                    }).detach();
                            }

                            if (UKismetMathLibrary::RandomBoolWithWeight(0.05)) {
                                std::thread([bot]() {
                                    bot->bIsCurrentlyStrafing = true;
                                    auto start = std::chrono::high_resolution_clock::now();
                                    auto duration = std::chrono::milliseconds(3000);
                                    while (std::chrono::high_resolution_clock::now() - start < duration) {
                                        FVector LeftVector = bot->Pawn->GetActorRightVector() * -1.0f;
                                        bot->Pawn->AddMovementInput(LeftVector, 1.1f, true);
                                    }
                                    bot->bIsCurrentlyStrafing = false;
                                    }).detach();
                            }
                        }
                        else {
                            FVector BackVector = bot->Pawn->GetActorForwardVector() * -1.0f;
                            bot->Pawn->AddMovementInput(BackVector, 1.1f, true);
                        }

                        //bot->PC->StopMovement();
                        bot->Pawn->PawnStartFire(0);
                    }
                    else if (Dist < 2000.f + rand() % 1000 && bot->PC->LineOfSightTo(NearestPlayerActor, {}, true)) {
                        if (bot->tick_counter % 30 == 0) {
                            //float RandomXmod = ((rand() % 120000 - rand() % 30000) + 90000);
                            float RandomYmod = ((rand() % 100) - rand() % 200);
                            float RandomZmod = ((rand() % 100) - rand() % 200);

                            FVector TargetPosMod{ Nearest.X, Nearest.Y + RandomYmod, Nearest.Z + RandomZmod };

                            TestRot = Math->FindLookAtRotation(BotLoc, TargetPosMod);
                        }

                        if (bot->tick_counter % 30 == 0) {
                            bot->PC->SetControlRotation(TestRot);
                            bot->PC->K2_SetActorRotation(TestRot, true);
                        }

                        if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.05)) {
                            TestRot = Math->FindLookAtRotation(BotLoc, Nearest);

                            bot->PC->SetControlRotation(TestRot);
                            bot->PC->K2_SetActorRotation(TestRot, true);
                        }

                        if (!bot->bIsCrouching && UKismetMathLibrary::RandomBoolWithWeight(0.03)) {
                            bot->bIsCrouching = true;
                            bot->Pawn->Crouch(false);
                        }

                        if (bot->bIsCrouching && UKismetMathLibrary::RandomBoolWithWeight(0.03)) {
                            bot->bIsCrouching = false;
                            bot->Pawn->UnCrouch(false);
                        }

                        if (!bot->bIsCurrentlyStrafing) {
                            if (UKismetMathLibrary::RandomBoolWithWeight(0.05)) {
                                std::thread([bot]() {
                                    bot->bIsCurrentlyStrafing = true;
                                    auto start = std::chrono::high_resolution_clock::now();
                                    auto duration = std::chrono::milliseconds(1500);
                                    while (std::chrono::high_resolution_clock::now() - start < duration) {
                                        bot->Pawn->AddMovementInput(bot->Pawn->GetActorRightVector(), 1.1f, true);
                                    }
                                    bot->bIsCurrentlyStrafing = false;
                                    }).detach();
                            }

                            if (UKismetMathLibrary::RandomBoolWithWeight(0.05)) {
                                std::thread([bot]() {
                                    bot->bIsCurrentlyStrafing = true;
                                    auto start = std::chrono::high_resolution_clock::now();
                                    auto duration = std::chrono::milliseconds(1500);
                                    while (std::chrono::high_resolution_clock::now() - start < duration) {
                                        FVector LeftVector = bot->Pawn->GetActorRightVector() * -1.0f;
                                        bot->Pawn->AddMovementInput(LeftVector, 1.1f, true);
                                    }
                                    bot->bIsCurrentlyStrafing = false;
                                    }).detach();
                            }
                        }

                        bot->PC->MoveToActor(NearestPlayerActor, rand() % 500, true, false, true, nullptr, true);
                        //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
                        bot->Pawn->PawnStartFire(0);
                    }
                    else {
                        /*TestRot = Math->FindLookAtRotation(BotLoc, Nearest);

                        bot->PC->SetControlRotation(TestRot);
                        bot->PC->K2_SetActorRotation(TestRot, true);*/
                        bot->LookAt(NearestPlayerActor);
                        bot->PC->MoveToActor(NearestPlayerActor, 0, true, false, true, nullptr, true);
                        //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
                    }
                }
                else {
                    bot->bIsMoving = false;
                }
            }
            else if (bot->WarmupChoice == EBotWarmupChoice::PickaxeOtherPlayers) {
                if (!bot->bIsMoving) {
                    bot->bIsMoving = true;
                }
                auto BotPos = bot->Pawn->K2_GetActorLocation();
                AActor* NearestPlayerActor = bot->GetNearestPlayerActor();
                if (!NearestPlayerActor) {
                    continue;
                }
                FVector Nearest = NearestPlayerActor->K2_GetActorLocation();
                if (!Nearest.IsZero()) {
                    float Dist = Math->Vector_Distance(BotPos, Nearest);
                    if (Dist < 150.f + rand() % 100) {
                        /*auto TestRot = Math->FindLookAtRotation(BotPos, Nearest);

                        bot->PC->SetControlRotation(TestRot);
                        bot->PC->K2_SetActorRotation(TestRot, true);*/
                        bot->LookAt(NearestPlayerActor);
                        bot->Pawn->PawnStartFire(0);
                    }
                    else {
                        /*auto TestRot = Math->FindLookAtRotation(BotPos, Nearest);

                        bot->PC->SetControlRotation(TestRot);
                        bot->PC->K2_SetActorRotation(TestRot, true);*/

                        bot->LookAt(NearestPlayerActor);
                        bot->PC->MoveToActor(NearestPlayerActor, 0, true, false, true, nullptr, true);
                        //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1, true);
                    }
                }
            }
            else if (bot->WarmupChoice == EBotWarmupChoice::MoveToPlayerAndEmote) {
                if (!bot->bIsMoving) {
                    bot->bIsMoving = true;
                }
                auto BotPos = bot->Pawn->K2_GetActorLocation();
                AActor* NearestPlayerActor = bot->GetNearestPlayerActor();
                if (!NearestPlayerActor) {
                    continue;
                }
                FVector Nearest = NearestPlayerActor->K2_GetActorLocation();
                if (!Nearest.IsZero()) {
                    float Dist = Math->Vector_Distance(BotPos, Nearest);
                    if (Dist < 200.f + rand() % 200) {
                        bot->bIsMoving = false;
                        /*auto TestRot = Math->FindLookAtRotation(BotPos, Nearest);

                        bot->PC->SetControlRotation(TestRot);
                        bot->PC->K2_SetActorRotation(TestRot, true);*/
                        bot->LookAt(NearestPlayerActor);
                        //bot->Emote();
                    }
                    else {
                        /*auto TestRot = Math->FindLookAtRotation(BotPos, Nearest);

                        bot->PC->SetControlRotation(TestRot);
                        bot->PC->K2_SetActorRotation(TestRot, true);*/

                        bot->LookAt(NearestPlayerActor);
                        bot->PC->MoveToActor(NearestPlayerActor, 0, true, false, true, nullptr, true);
                        //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1, true);
                    }
                }
            }
            else if (bot->WarmupChoice == EBotWarmupChoice::Emote) {
                bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1, true);
                if (!bot->bIsEmoting) {
                    //bot->Emote(); // Emoting doesent work lol
                }
            }
            else {
                bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1, true);
                if (!bot->bIsEmoting) {
                    //bot->Emote(); // Emoting doesent work lol
                }
            }

            /*auto BotPos = bot->Pawn->K2_GetActorLocation();
            FVector Nearest = bot->FindNearestPlayerOrBot();
            if (!Nearest.IsZero()) {
                float Dist = Math->Vector_Distance(BotPos, Nearest);
                if (Dist < 2000.f && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.05f)) {
                    auto TestRot = Math->FindLookAtRotation(Nearest, BotPos);

                    bot->PC->SetControlRotation(TestRot);
                    bot->PC->K2_SetActorRotation(TestRot, true);
                    bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
                }
                else {
                    bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.f, true);
                }
            }
            else {
                bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.f, true);
            }*/
            bot->Pawn->SetHealth(100);
            bot->Pawn->SetShield(100);
        }
        else if (bot->BotState == EBotState::PreBus) {
            bot->Pawn->SetHealth(100);
            bot->Pawn->SetShield(100);
            if (bot->bIsMoving) {
                bot->Pawn->PawnStopFire(0);
                bot->bIsMoving = false;
            }
            if (!bot->bHasThankedBusDriver && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.0005f))
            {
                bot->bHasThankedBusDriver = true;
                bot->PC->ThankBusDriver();
            }
        }
        else if (bot->BotState == EBotState::Bus) {
            bot->Pawn->SetShield(0);
            if (bot->bIsMoving) {
                bot->Pawn->PawnStopFire(0);
                bot->bIsMoving = false;
            }
            if (!bot->bHasThankedBusDriver && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.0005f))
            {
                bot->bHasThankedBusDriver = true;
                bot->PC->ThankBusDriver();
            }

            if (!bot->TimeToNextAction) {
                bot->TimeToNextAction = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
                bot->LikelyHoodToDoAction = 0.0008f;
            }

            if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) - bot->TimeToNextAction >= 5.f) {
                bot->LikelyHoodToDoAction = 0.002f;
            }
            else if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) - bot->TimeToNextAction >= 9.f) {
                bot->LikelyHoodToDoAction = 0.0075f;
            }
            else if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) - bot->TimeToNextAction >= 13.5f) {
                bot->LikelyHoodToDoAction = 0.075f;
            }
            else if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) - bot->TimeToNextAction >= 15.f) {
                bot->LikelyHoodToDoAction = 0.0075f;
            }
            else if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) - bot->TimeToNextAction >= 20.f) {
                bot->LikelyHoodToDoAction = 0.002f;
            }

            if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(bot->LikelyHoodToDoAction)) {
                if (!bot->bHasThankedBusDriver && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.5f)) {
                    bot->bHasThankedBusDriver = true;
                    bot->PC->ThankBusDriver();
                }
                bot->Pawn->K2_TeleportTo(GameState->GetAircraft(0)->K2_GetActorLocation(), {});
                bot->Pawn->BeginSkydiving(true);
                bot->TimeToNextAction = 0;
                bot->LikelyHoodToDoAction = 0;
                bot->BotState = EBotState::Skydiving;
            }
        }
        else if (bot->BotState == EBotState::Skydiving) {
            if (!bot->Pawn->bIsSkydiving) {
                bot->Pawn->bInGliderRedeploy = true;
                bot->BotState = EBotState::Gliding;
            }
            else {
                auto BotPos = bot->Pawn->K2_GetActorLocation();
                FVector Nearest = bot->FindNearestPlayerOrBot();
                AActor* NearestChest = bot->FindNearestChest();
                AActor* NearestPickup = (AActor*)bot->FindNearestPickup();
                if (!NearestChest || !NearestPickup) {
                    continue;
                }
                AActor* NearestLootable = NearestChest;
                if (NearestChest->GetDistanceTo(bot->Pawn) > NearestPickup->GetDistanceTo(bot->Pawn)) {
                    NearestLootable = NearestPickup;
                }
                if (!Nearest.IsZero()) {
                    float Dist = Math->Vector_Distance(BotPos, Nearest);
                    if (Dist < 3000.f) {
                        auto TestRot = Math->FindLookAtRotation(Nearest, BotPos);

                        bot->PC->SetControlRotation(TestRot);
                        bot->PC->K2_SetActorRotation(TestRot, true);
                        bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
                    }
                    else {
                        auto TestRot = Math->FindLookAtRotation(BotPos, NearestLootable->K2_GetActorLocation());

                        bot->PC->SetControlRotation(TestRot);
                        bot->PC->K2_SetActorRotation(TestRot, true);
                        bot->LookAt(NearestLootable);

                        bot->PC->MoveToActor(NearestLootable, 0.f, true, false, true, nullptr, true);
                        bot->Pawn->AddMovementInput(UKismetMathLibrary::GetDefaultObj()->NegateVector(bot->Pawn->GetActorUpVector()), 1, true);
                        //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
                    }
                }
                else {
                    auto TestRot = Math->FindLookAtRotation(BotPos, NearestLootable->K2_GetActorLocation());

                    bot->PC->SetControlRotation(TestRot);
                    bot->PC->K2_SetActorRotation(TestRot, true);
                    bot->LookAt(NearestLootable);

                    bot->PC->MoveToActor(NearestLootable, 0.f, true, false, true, nullptr, true);
                    bot->Pawn->AddMovementInput(UKismetMathLibrary::GetDefaultObj()->NegateVector(bot->Pawn->GetActorUpVector()), 1, true);
                    //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
                }
            }
        }
        else if (bot->BotState == EBotState::Gliding) {
            FVector Vel = bot->Pawn->GetVelocity();
            float Speed = Vel.Z;
            if (Speed == 0.f) {
                bot->BotState = EBotState::Landed;
            }

            auto BotPos = bot->Pawn->K2_GetActorLocation();
            FVector Nearest = bot->FindNearestPlayerOrBot();
            AActor* NearestChest = bot->FindNearestChest();
            AActor* NearestPickup = (AActor*)bot->FindNearestPickup();
            if (!NearestChest || !NearestPickup) {
                continue;
            }
            AActor* NearestLootable = NearestChest;
            if (NearestChest->GetDistanceTo(bot->Pawn) > NearestPickup->GetDistanceTo(bot->Pawn)) {
                NearestLootable = NearestPickup;
            }
            float Dist = Math->Vector_Distance(BotPos, Nearest);
            if (!Nearest.IsZero()) {
                if (Dist < 3500.f) {
                    auto TestRot = Math->FindLookAtRotation(Nearest, BotPos);

                    bot->PC->SetControlRotation(TestRot);
                    bot->PC->K2_SetActorRotation(TestRot, true);
                    bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
                }
                else {
                    bot->TargetLootableActor = bot->FindNearestChest();
                    auto TestRot = Math->FindLookAtRotation(BotPos, NearestLootable->K2_GetActorLocation());

                    bot->PC->SetControlRotation(TestRot);
                    bot->PC->K2_SetActorRotation(TestRot, true);
                    bot->LookAt(NearestLootable);

                    bot->PC->MoveToActor(NearestLootable, 0.f, true, false, true, nullptr, true);
                    //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
                }
            }
            else {
                bot->TargetLootableActor = bot->FindNearestChest();
                auto TestRot = Math->FindLookAtRotation(BotPos, NearestLootable->K2_GetActorLocation());

                bot->PC->SetControlRotation(TestRot);
                bot->PC->K2_SetActorRotation(TestRot, true);
                bot->LookAt(NearestLootable);

                bot->PC->MoveToActor(NearestLootable, 0.f, true, false, true, nullptr, true);
                //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
            }
        }
        else if (bot->BotState == EBotState::Landed) {
            if (!bot->bIsMoving) {
                bot->bIsMoving = true;
            }
            FVector BotLoc = bot->Pawn->K2_GetActorLocation();
            FVector Nearest = bot->FindNearestPlayerOrBot();
            if (!Nearest.IsZero()) {
                float Dist = Math->Vector_Distance(BotLoc, Nearest);
                if (Dist < 3500.f) {
                    bot->BotState = EBotState::Fleeing;
                }
                else {
                    bot->BotState = EBotState::Looting;
                }
            }
            else {
                bot->BotState = EBotState::Looting;
            }
        }
        else if (bot->BotState == EBotState::Fleeing) {
            FVector BotLoc = bot->Pawn->K2_GetActorLocation();
            FVector Nearest = bot->FindNearestPlayerOrBot();
            if (!Nearest.IsZero()) {
                float Dist = Math->Vector_Distance(BotLoc, Nearest);
                if (Dist < 1000) {
                    bot->CombatSituation = EBotCombatSituation::CloseRange;
                }
                else if (Dist < 3000 && Dist > 1000) {
                    bot->CombatSituation = EBotCombatSituation::MidRange;
                }
                else if (Dist < 6000 && Dist > 3000) {
                    bot->CombatSituation = EBotCombatSituation::LongRange;
                }

                if (bot->HasWeaponTypeBasedOnSituation()) {
                    bot->BotState = EBotState::LookingForPlayers;
                    continue;
                }

                if (Dist < 3500.f) {
                    auto TestRot = Math->FindLookAtRotation(Nearest, BotLoc);

                    bot->PC->SetControlRotation(TestRot);
                    bot->PC->K2_SetActorRotation(TestRot, true);
                    bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
                }
                else {
                    bot->BotState = EBotState::Looting;
                }
            }
            else {
                bot->BotState = EBotState::Looting;
            }
        }
        else if (bot->BotState == EBotState::Looting) {
            if (bot->bIsDead) {
                continue;
            }
            else {
                AActor* NearestChest = bot->FindNearestChest();
                AActor* NearestPickup = (AActor*)bot->FindNearestPickup();
                if (!NearestChest || !NearestPickup) {
                    continue;
                }
                AActor* Nearest = NearestChest;
                ELootableType NearestLootable = ELootableType::Chest;
                if (NearestChest->GetDistanceTo(bot->Pawn) > NearestPickup->GetDistanceTo(bot->Pawn)) {
                    NearestLootable = ELootableType::Pickup;
                    Nearest = NearestPickup;
                }
                FVector BotLoc = bot->Pawn->K2_GetActorLocation();
                if (!BotLoc.IsZero()) {
                    FVector Nearest = bot->FindNearestPlayerOrBot();
                    if (!Nearest.IsZero()) {
                        float Dist = Math->Vector_Distance(BotLoc, Nearest);
                        if (Dist < 1000) {
                            bot->CombatSituation = EBotCombatSituation::CloseRange;
                        }
                        else if (Dist < 3000 && Dist > 1000) {
                            bot->CombatSituation = EBotCombatSituation::MidRange;
                        }
                        else if (Dist < 6000 && Dist > 3000) {
                            bot->CombatSituation = EBotCombatSituation::LongRange;
                        }

                        if (bot->HasWeaponTypeBasedOnSituation()) {
                            bot->BotState = EBotState::LookingForPlayers;
                            continue;
                        }
                    }
                }
                float Dist = Math->Vector_Distance(BotLoc, Nearest->K2_GetActorLocation());

                if (Dist < 300.f) {
                    bot->Pawn->PawnStopFire(0);
                    bot->PC->StopMovement();
                    if (!bot->TimeToNextAction || !bot->Pawn->bStartedInteractSearch && NearestLootable == ELootableType::Chest) {
                        bot->TimeToNextAction = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
                        bot->Pawn->bStartedInteractSearch = true;
                        bot->Pawn->OnRep_StartedInteractSearch();
                    }
                    else if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) - bot->TimeToNextAction >= 1.5f && NearestLootable == ELootableType::Chest) {
                        SpawnLoot((ABuildingContainer*)Nearest);
                        Nearest->bHidden = true;
                        AFortPickup* Pickup = bot->FindNearestPickup();
                        if (Pickup)
                        {
                            bot->PickupAllItemsInRange();
                            bot->SimpleSwitchToWeapon();
                        }

                        bot->Pawn->bStartedInteractSearch = false;
                        bot->Pawn->OnRep_StartedInteractSearch();
                        bot->TimeToNextAction = 0;
                        bot->BotState = EBotState::LookingForPlayers;
                    }
                    else if (NearestLootable == ELootableType::Pickup) {
                        bot->PickupAllItemsInRange(400.f);
                        bot->TimeToNextAction = 0;
                        bot->BotState = EBotState::LookingForPlayers;
                    }
                }
                else if (Dist < 2000.f) {
                    bot->Pawn->PawnStartFire(0);
                    if (!bot->TimeToNextAction) {
                        bot->TimeToNextAction = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
                    }

                    if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) - bot->TimeToNextAction >= 15.f) {
                        Nearest->bHidden = true;
                        bot->TimeToNextAction = 0.f;
                    }

                    auto BotPos = bot->Pawn->K2_GetActorLocation();
                    auto TestRot = Math->FindLookAtRotation(BotPos, Nearest->K2_GetActorLocation());

                    bot->PC->SetControlRotation(TestRot);
                    bot->PC->K2_SetActorRotation(TestRot, true);
                    bot->LookAt(Nearest);
                    bot->PC->MoveToActor(Nearest, 0, true, false, true, nullptr, true);
                    //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
                }
                else {
                    FVector Vel = bot->Pawn->GetVelocity();
                    float Speed = sqrtf(Vel.X * Vel.X + Vel.Y * Vel.Y);
                    
                    if (Speed < 0.5f) {
                        bot->Pawn->PawnStartFire(0);
                        if (!bot->TimeToNextAction) {
                            bot->TimeToNextAction = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
                        }
                    }
                    else {
                        bot->Pawn->PawnStopFire(0);
                    }

                    if (UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld()) - bot->TimeToNextAction >= 15.f) {
                        Nearest->bHidden = true;
                        bot->TimeToNextAction = 0.f;
                    }

                    auto BotPos = bot->Pawn->K2_GetActorLocation();
                    auto TestRot = Math->FindLookAtRotation(BotPos, Nearest->K2_GetActorLocation());

                    bot->PC->SetControlRotation(TestRot);
                    bot->PC->K2_SetActorRotation(TestRot, true);
                    bot->LookAt(Nearest);
                    bot->PC->MoveToActor(Nearest, 0, true, false, true, nullptr, true);
                    //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
                }
            }
        }
        else if (bot->BotState == EBotState::LookingForPlayers) {
            if (bot->bIsDead) {
                continue;
            }
            if (!bot->HasGun()) {
                bot->BotState = EBotState::Looting;
            }
            FVector BotLoc = bot->Pawn->K2_GetActorLocation();
            AActor* NearestPlayerActor = bot->GetNearestPlayerActor();
            FVector Nearest = NearestPlayerActor->K2_GetActorLocation();

            FRotator TestRot;
            FVector TargetPosmod = Nearest;

            if (!Nearest.IsZero()) {
                float Dist = Math->Vector_Distance(BotLoc, Nearest);
                if (Dist < 1000) {
                    bot->CombatSituation = EBotCombatSituation::CloseRange;
                }
                else if (Dist < 3000 && Dist > 1000) {
                    bot->CombatSituation = EBotCombatSituation::MidRange;
                }
                else if (Dist < 6000 && Dist > 3000) {
                    bot->CombatSituation = EBotCombatSituation::LongRange;
                }

                if (!bot->HasWeaponTypeBasedOnSituation() && !bot->bIsInWeaponOverride) {
                    if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.03f)) {
                        bot->bIsInWeaponOverride = true;
                    }
                    else {
                        bot->BotState = EBotState::Looting;
                    }
                    continue;
                }

                if (bot->bIsInWeaponOverride) {
                    if (bot->HasGun()) {
                        bot->SimpleSwitchToWeapon();
                    }
                    else {
                        bot->BotState = EBotState::Looting;
                    }
                }

                if (!bot->HasSituatedWeaponEquiped()) {
                    bot->AdvancedSwitchToWeapon();
                }

                if (Dist < 6000 && bot->PC->LineOfSightTo(NearestPlayerActor, {}, true)) {
                    if (bot->tick_counter % 30 == 0) {
                        //float RandomXmod = ((rand() % 120000 - rand() % 30000) + 90000);
                        float RandomYmod = ((rand() % 50) - rand() % 100);
                        float RandomZmod = ((rand() % 50) - rand() % 100);

                        if (Dist < 3000 && Dist > 1000) {
                            RandomYmod = ((rand() % 100) - rand() % 200);
                            RandomZmod = ((rand() % 100) - rand() % 200);
                        }
                        else if (Dist < 6000 && Dist > 3000) {
                            RandomYmod = ((rand() % 70) - rand() % 170);
                            RandomZmod = ((rand() % 70) - rand() % 170);
                        }

                        FVector TargetPosMod{ Nearest.X, Nearest.Y + RandomYmod, Nearest.Z + RandomZmod };

                        TestRot = Math->FindLookAtRotation(BotLoc, TargetPosMod);
                    }

                    if (bot->tick_counter % 30 == 0) {
                        bot->PC->SetControlRotation(TestRot);
                        bot->PC->K2_SetActorRotation(TestRot, true);
                    }

                    if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.05)) {
                        TestRot = Math->FindLookAtRotation(BotLoc, Nearest);

                        bot->PC->SetControlRotation(TestRot);
                        bot->PC->K2_SetActorRotation(TestRot, true);
                    }

                    if (!bot->bIsCrouching && UKismetMathLibrary::RandomBoolWithWeight(0.03)) {
                        bot->bIsCrouching = true;
                        bot->Pawn->Crouch(false);
                    }

                    if (bot->bIsCrouching && UKismetMathLibrary::RandomBoolWithWeight(0.03)) {
                        bot->bIsCrouching = false;
                        bot->Pawn->UnCrouch(false);
                    }

                    if (!bot->bIsCurrentlyStrafing) {
                        if (UKismetMathLibrary::RandomBoolWithWeight(0.05)) {
                            std::thread([bot]() {
                                bot->bIsCurrentlyStrafing = true;
                                auto start = std::chrono::high_resolution_clock::now();
                                auto duration = std::chrono::milliseconds(3000);
                                while (std::chrono::high_resolution_clock::now() - start < duration && bot && !bot->bIsDead) {
                                    bot->Pawn->AddMovementInput(bot->Pawn->GetActorRightVector(), 1.1f, true);
                                }
                                if (bot) {
                                    bot->bIsCurrentlyStrafing = false;
                                }
                                }).detach();
                        }

                        if (UKismetMathLibrary::RandomBoolWithWeight(0.05)) {
                            std::thread([bot]() {
                                bot->bIsCurrentlyStrafing = true;
                                auto start = std::chrono::high_resolution_clock::now();
                                auto duration = std::chrono::milliseconds(3000);
                                while (std::chrono::high_resolution_clock::now() - start < duration && bot && !bot->bIsDead) {
                                    FVector LeftVector = bot->Pawn->GetActorRightVector() * -1.0f;
                                    bot->Pawn->AddMovementInput(LeftVector, 1.1f, true);
                                }
                                if (bot) {
                                    bot->bIsCurrentlyStrafing = false;
                                }
                                }).detach();
                        }
                    }
                    else {
                        FVector BackVector = bot->Pawn->GetActorForwardVector() * -1.0f;
                        bot->Pawn->AddMovementInput(BackVector, 1.1f, true);
                    }

                    if (Dist < 1000) {
                        FVector BackVector = bot->Pawn->GetActorForwardVector() * -1.0f;
                        bot->Pawn->AddMovementInput(BackVector, 1.1f, true);
                    }

                    if (Dist < 1000) {
                        bot->PC->MoveToActor(NearestPlayerActor, rand() % 750 + 750, true, false, true, nullptr, true);
                    }
                    else if (Dist < 3000 && Dist > 1000) {
                        bot->PC->MoveToActor(NearestPlayerActor, 750 + rand() % 2000, true, false, true, nullptr, true);
                    }
                    else if (Dist < 6000 && Dist > 3000) {
                        bot->PC->MoveToActor(NearestPlayerActor, 3000 + rand() + 2000, true, false, true, nullptr, true);
                    }

                    //bot->PC->StopMovement();
                    bot->Pawn->PawnStartFire(0);
                }
                else {
                    bot->BotState = EBotState::Looting;
                }
            }
            else {}
        }

        bot->tick_counter++;
    }
}