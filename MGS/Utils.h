#pragma once // this is basicly my framework file 
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include "SDK.hpp"
#include <map>

using namespace std;
using namespace SDK;

FName PickaxeTagName;

static UNetDriver* (*CreateNetDriver)(void*, void*, FName) = decltype(CreateNetDriver)(InSDKUtils::GetImageBase() + 0x4573990);
static bool (*InitListen)(void*, void*, FURL&, bool, FString&) = decltype(InitListen)(InSDKUtils::GetImageBase() + 0xD44C40);
static void (*SetWorld)(void*, void*) = decltype(SetWorld)(InSDKUtils::GetImageBase() + 0x42C2B20);
inline bool (*InternalTryActivateAbility)(UAbilitySystemComponent* AbilitySystemComp, FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey InPredictionKey, UGameplayAbility** OutInstancedAbility, void* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData) = decltype(InternalTryActivateAbility)(InSDKUtils::GetImageBase() + 0x6B33F0);
inline void (*SpecConstructor)(FGameplayAbilitySpec*, UGameplayAbility*, int, int, UObject*) = decltype(SpecConstructor)(InSDKUtils::GetImageBase() + 0x6D6DD0);
static void* (*StaticFindObjectOG)(UClass*, UObject* Package, const wchar_t* OrigInName, bool ExactClass) = decltype(StaticFindObjectOG)(InSDKUtils::GetImageBase() + 0x2E1C4B0);
static void* (*StaticLoadObjectOG)(UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, uint32_t LoadFlags, UObject* Sandbox, bool bAllowObjectReconciliation, void*) = decltype(StaticLoadObjectOG)(InSDKUtils::GetImageBase() + 0x2E1CD60);
static FVector* (*PickSupplyDropLocationOG)(AFortAthenaMapInfo* MapInfo, FVector* outLocation, __int64 Center, float Radius) = decltype(PickSupplyDropLocationOG)(__int64(GetModuleHandleA(0)) + 0x18848f0);
inline static ABuildingSMActor* (*ReplaceBuildingActor)(ABuildingSMActor* BuildingSMActor, unsigned int a2, UObject* a3, unsigned int a4, int a5, bool bMirrored, AFortPlayerControllerAthena* PC) = decltype(ReplaceBuildingActor)(InSDKUtils::GetImageBase() + 0x1B951B0);
static __int64 (*CantBuild)(UWorld*, UObject*, FVector, FRotator, char, void*, char*) = decltype(CantBuild)(InSDKUtils::GetImageBase() + 0x1E57790);
static inline __int64 (*CombineStruct)(void* FGameplayAbilitySpec, UGameplayAbility* InAbility, char InLevel, int InInputID, void* InSourceObject) = decltype(CombineStruct)(InSDKUtils::GetImageBase() + 0x6D6DD0);
static inline void (*GiveAbilityAndActivateOnce)(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle* Spec, FGameplayAbilitySpec GameplayEventData) = decltype(GiveAbilityAndActivateOnce)(InSDKUtils::GetImageBase() + 0x6B19E0);
static void (*BotManagerSetup)(__int64 BotManaager, __int64 Pawn, __int64 BehaviorTree, __int64 a4, DWORD* SkillLevel, __int64 a7, __int64 StartupInventory, __int64 BotNameSettings, __int64 a10, BYTE* CanRespawnOnDeath, unsigned __int8 BitFieldDataThing, BYTE* CustomSquadId, FFortAthenaAIBotRunTimeCustomizationData InRuntimeBotData) = decltype(BotManagerSetup)(__int64(GetModuleHandleW(0)) + 0x19D93F0);
static void(*RemoveFromAlivePlayers)(AFortGameModeAthena*, AFortPlayerControllerAthena*, APlayerState*, AFortPlayerPawn*, UFortWeaponItemDefinition*, EDeathCause, char) = decltype(RemoveFromAlivePlayers)(__int64(GetModuleHandleW(0)) + 0x18ECBB0);
static void (*AddToAlivePlayers)(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* Player) = decltype(AddToAlivePlayers)(__int64(GetModuleHandleW(0)) + 0x18c35b0);

std::map<AFortPlayerController*, int> Shots{};
std::map<AFortPlayerController*, UFortWeaponItemDefinition*> Weapons{};
AFortAthenaMutator_Bots* BotMutator = nullptr;

static TArray<AActor*> PlayerStarts;

enum class EAccoladeEvent : uint8
{
	Kill,
	Search,
	MAX
};

void Log(const std::string& msg)
{
	static bool firstCall = true;

	if (firstCall)
	{
		std::ofstream logFile("MGS_log.txt", std::ios::trunc); 
		if (logFile.is_open())
		{
			logFile << "[MGS]: Log file initialized\n";
			logFile.close();
		}
		firstCall = false;
	}

	std::ofstream logFile("MGS_log.txt", std::ios::app);
	if (logFile.is_open())
	{
		logFile << "[MGS]: " << msg << std::endl;
		logFile.close();
	}

	std::cout << "[MGS]: " << msg << std::endl;
}

void HookVTable(void* Base, int Idx, void* Detour, void** OG)
{
	DWORD oldProtection;

	void** VTable = *(void***)Base;

	if (OG)
	{
		*OG = VTable[Idx];
	}

	VirtualProtect(&VTable[Idx], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection);

	VTable[Idx] = Detour;

	VirtualProtect(&VTable[Idx], sizeof(void*), oldProtection, NULL);
}

inline FQuat RotatorToQuat(FRotator Rotation) 
{ 
	FQuat Quat;
	const float DEG_TO_RAD = 3.14159f / 180.0f;
	const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

	float SP = sin(Rotation.Pitch * DIVIDE_BY_2);
	float CP = cos(Rotation.Pitch * DIVIDE_BY_2);
	float SY = sin(Rotation.Yaw * DIVIDE_BY_2);
	float CY = cos(Rotation.Yaw * DIVIDE_BY_2);
	float SR = sin(Rotation.Roll * DIVIDE_BY_2);
	float CR = cos(Rotation.Roll * DIVIDE_BY_2);

	Quat.X = CR * SP * SY - SR * CP * CY;
	Quat.Y = -CR * SP * CY - SR * CP * SY;
	Quat.Z = CR * CP * SY - SR * SP * CY;
	Quat.W = CR * CP * CY + SR * SP * SY;

	return Quat;
}

template <typename T>
static inline T* StaticFindObject(std::wstring ObjectName)
{
	return (T*)StaticFindObjectOG(T::StaticClass(), nullptr, ObjectName.c_str(), false);
}

template<typename T>
inline T* Cast(UObject* Object)
{
	if (!Object || !Object->IsA(T::StaticClass()))
		return nullptr;
	return (T*)Object;
}

template<typename T = UObject>
static inline T* StaticLoadObject(const std::string& Name)
{
	auto ConvName = std::wstring(Name.begin(), Name.end());

	T* Object = StaticFindObject<T>(ConvName);

	if (!Object)
	{
		Object = (T*)StaticLoadObjectOG(T::StaticClass(), nullptr, ConvName.c_str(), nullptr, 0, nullptr, false, nullptr);
	}

	return Object;
}

template<typename T>
T* GetDefaultObject()
{
	return (T*)T::StaticClass()->DefaultObject;
}

FName::FName(const FString& InName)
{
	*this = UKismetStringLibrary::Conv_StringToName(InName);
}

static inline FQuat FRotToQuat(FRotator Rotation) {
	FQuat Quat;
	const float DEG_TO_RAD = 3.14159f / 180.0f;
	const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

	float SP = sin(Rotation.Pitch * DIVIDE_BY_2);
	float CP = cos(Rotation.Pitch * DIVIDE_BY_2);
	float SY = sin(Rotation.Yaw * DIVIDE_BY_2);
	float CY = cos(Rotation.Yaw * DIVIDE_BY_2);
	float SR = sin(Rotation.Roll * DIVIDE_BY_2);
	float CR = cos(Rotation.Roll * DIVIDE_BY_2);

	Quat.X = CR * SP * SY - SR * CP * CY;
	Quat.Y = -CR * SP * CY - SR * CP * SY;
	Quat.Z = CR * CP * SY - SR * SP * CY;
	Quat.W = CR * CP * CY + SR * SP * SY;

	return Quat;
}

template<typename T>
inline T* SpawnActor(FVector Loc, FRotator Rot = FRotator(), AActor* Owner = nullptr, SDK::UClass* Class = T::StaticClass(), ESpawnActorCollisionHandlingMethod Handle = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
{
	FTransform Transform{};
	Transform.Scale3D = FVector{ 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = FRotToQuat(Rot);

	return (T*)UGameplayStatics::FinishSpawningActor(UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, Handle, Owner), Transform);
}

template<typename T>
static inline T* SpawnAActor(FVector Loc = { 0,0,0 }, FRotator Rot = { 0,0,0 }, AActor* Owner = nullptr)
{
	FTransform Transform{};
	Transform.Scale3D = { 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = FRotToQuat(Rot);

	AActor* NewActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), T::StaticClass(), Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner);
	return (T*)UGameplayStatics::FinishSpawningActor(NewActor, Transform);
}

template<typename T>
static inline T* SpawnActorClass(FVector Loc = { 0,0,0 }, FRotator Rot = { 0,0,0 }, UClass* Class = nullptr, AActor* Owner = nullptr)
{
	FTransform Transform{};
	Transform.Scale3D = { 1,1,1 };
	Transform.Translation = Loc;
	Transform.Rotation = RotatorToQuat(Rot);

	AActor* NewActor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner);
	return (T*)UGameplayStatics::FinishSpawningActor(NewActor, Transform);
}

template<typename T>
T* Actors(UClass* Class = T::StaticClass(), FVector Loc = {}, FRotator Rot = {}, AActor* Owner = nullptr)
{
	return SpawnActor<T>(Loc, Rot, Owner, Class);
}

AFortPickupAthena* SpawnPickup(UFortItemDefinition* ItemDef, int OverrideCount, int LoadedAmmo, FVector Loc, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, AFortPawn* Pawn = nullptr)
{
	auto SpawnedPickup = Actors<AFortPickupAthena>(AFortPickupAthena::StaticClass(), Loc);
	SpawnedPickup->bRandomRotation = true;

	auto& PickupEntry = SpawnedPickup->PrimaryPickupItemEntry;
	PickupEntry.ItemDefinition = ItemDef;
	PickupEntry.Count = OverrideCount;
	PickupEntry.LoadedAmmo = LoadedAmmo;
	PickupEntry.ReplicationKey++;
	SpawnedPickup->OnRep_PrimaryPickupItemEntry();
	SpawnedPickup->PawnWhoDroppedPickup = Pawn;

	SpawnedPickup->TossPickup(Loc, Pawn, -1, true, false, SourceType, Source);

	SpawnedPickup->SetReplicateMovement(true);
	SpawnedPickup->MovementComponent = (UProjectileMovementComponent*)GetDefaultObject<UGameplayStatics>()->SpawnObject(UProjectileMovementComponent::StaticClass(), SpawnedPickup);

	if (SourceType == EFortPickupSourceTypeFlag::Container)
	{
		SpawnedPickup->bTossedFromContainer = true;
		SpawnedPickup->OnRep_TossedFromContainer();
	}

	return SpawnedPickup;
}

map<AFortPickup*, float> PickupLifetimes;
AFortPickupAthena* SpawnStack(APlayerPawn_Athena_C* Pawn, UFortItemDefinition* Def, int Count, bool giveammo = false, int ammo = 0)
{
	auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

	FVector Loc = Pawn->K2_GetActorLocation();
	AFortPickupAthena* Pickup = Actors<AFortPickupAthena>(AFortPickupAthena::StaticClass(), Loc);
	Pickup->bReplicates = true;
	PickupLifetimes[Pickup] = Statics->GetTimeSeconds(UWorld::GetWorld());
	Pickup->PawnWhoDroppedPickup = Pawn;
	Pickup->PrimaryPickupItemEntry.Count = Count;
	Pickup->PrimaryPickupItemEntry.ItemDefinition = Def;
	if (giveammo)
	{
		Pickup->PrimaryPickupItemEntry.LoadedAmmo = ammo;
	}
	Pickup->PrimaryPickupItemEntry.ReplicationKey++;

	Pickup->OnRep_PrimaryPickupItemEntry();
	Pickup->TossPickup(Loc, Pawn, 6, true, true, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::Unset);

	Pickup->MovementComponent = (UProjectileMovementComponent*)Statics->SpawnObject(UProjectileMovementComponent::StaticClass(), Pickup);
	Pickup->MovementComponent->bReplicates = true;
	((UProjectileMovementComponent*)Pickup->MovementComponent)->SetComponentTickEnabled(true);

	return Pickup;
}

static AFortPickupAthena* SpawnPickup(FFortItemEntry ItemEntry, FVector Location, EFortPickupSourceTypeFlag PickupSource = EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset)
{
	auto Pickup = SpawnPickup(ItemEntry.ItemDefinition, ItemEntry.Count, ItemEntry.LoadedAmmo ,Location,  PickupSource, SpawnSource);
	return Pickup;
}

FVector PickSupplyDropLocation(AFortAthenaMapInfo* MapInfo, FVector Center, float Radius)
{
	if (!PickSupplyDropLocationOG)
		return FVector(0, 0, 0);

	FVector loc = FVector(0, 0, 0);
	auto PickSupplyDropLoc = PickSupplyDropLocationOG(MapInfo, &loc, __int64(&Center), Radius);
	return loc;
}

inline void ShowFoundation(ABuildingFoundation* BuildingFoundation) {
	if (!BuildingFoundation)
		return;

	BuildingFoundation->bServerStreamedInLevel = true;
	BuildingFoundation->DynamicFoundationType = EDynamicFoundationType::Static;
	BuildingFoundation->OnRep_ServerStreamedInLevel();

	BuildingFoundation->FoundationEnabledState = EDynamicFoundationEnabledState::Enabled;
	BuildingFoundation->DynamicFoundationRepData.EnabledState = EDynamicFoundationEnabledState::Enabled;
	BuildingFoundation->DynamicFoundationTransform = BuildingFoundation->GetTransform();
	BuildingFoundation->OnRep_DynamicFoundationRepData();
}

inline UFortAccoladeItemDefinition* GetDefFromEvent(EAccoladeEvent Event, int Count, UObject* Object = nullptr)
{
	UFortAccoladeItemDefinition* Def = nullptr;

	switch (Event)
	{
	case EAccoladeEvent::Kill:
		if (Count == 1)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_014_Elimination_Bronze.AccoladeId_014_Elimination_Bronze");
		}
		else if (Count == 4)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_015_Elimination_Silver.AccoladeId_015_Elimination_Silver");
		}
		else if (Count == 8)
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_016_Elimination_Gold.AccoladeId_016_Elimination_Gold");
		}
		else
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_012_Elimination.AccoladeId_012_Elimination");
		}
		break;
	case EAccoladeEvent::Search:
		if (!Object || !Object->Class->GetName().contains("Ammo"))
		{
			if (Count == 3)
			{
				Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_008_SearchChests_Bronze.AccoladeId_008_SearchChests_Bronze");
			}
			else if (Count == 7)
			{
				Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_009_SearchChests_Silver.AccoladeId_009_SearchChests_Silver");
			}
			else if (Count == 12)
			{
				Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_010_SearchChests_Gold.AccoladeId_010_SearchChests_Gold");
			}
			else
			{
				Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_007_SearchChests.AccoladeId_007_SearchChests");
			}
		}
		else
		{
			Def = StaticLoadObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_011_SearchAmmoBox.AccoladeId_011_SearchAmmoBox");
		}
		break;
	case EAccoladeEvent::MAX:
		break;
	default:
		break;
	}

	return Def;
}

template<typename T>
inline vector<T*> GetAllObjectsOfClass(UClass* Class = T::StaticClass())
{
	std::vector<T*> Objects{};

	for (int i = 0; i < UObject::GObjects->Num(); ++i)
	{
		UObject* Object = UObject::GObjects->GetByIndex(i);

		if (!Object)
			continue;

		if (Object->GetFullName().contains("Default"))
			continue;

		if (Object->GetFullName().contains("Test"))
			continue;

		if (Object->IsA(Class) && !Object->IsDefaultObject())
		{
			Objects.push_back((T*)Object);
		}
	}

	return Objects;
}