#pragma once
#include "Utils.h"
#include "Gamemode.h"
#include "PC.h"
#include "Tick.h"
#include "Abilities.h"
#include "Looting.h"
#include "Bosses.h"
#include "Teams.h"
#include "minhook/MinHook.h"

#pragma comment(lib, "minhook/minhook.lib")

static auto ImageBase = InSDKUtils::GetImageBase();

int True() {
	return 1;
}

int False() {
	return 0;
}

namespace Gamemode {

	inline void InitHooks()
	{
		Log("Hooking Gamemode");

		MH_CreateHook((LPVOID)(ImageBase + 0x4640A30), ReadyToStartMatch, (LPVOID*)&ReadyToStartMatchOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x18F9BB0), StartAircraftPhase, (LPVOID*)&StartAircraftPhaseOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x18F6250), SpawnDefaultPawnFor, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0x1B6BC60), SpawnPawnAfterReboot, nullptr);
		MH_CreateHook(LPVOID(ImageBase + 0x18E6B20), PickTeam, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0x18E07D0), OnAircraftExitedDropZone, (LPVOID*)&OriginalOnAircraftExitedDropZone);
		MH_CreateHook((LPVOID)(ImageBase + 0x1AB4DE0), OnAircraftEnteredDropZone, (LPVOID*)&OnAircraftEnteredDropZoneOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x18FD350), Storm, (LPVOID*)&StormOG);
	}
}

namespace Misc {

	void InitHooks()
	{
		Log("Hooking Misc");

		MH_CreateHook((LPVOID)(ImageBase + 0x2D95E00), True, nullptr); // collect garbage
		MH_CreateHook((LPVOID)(ImageBase + 0x4155600), False, nullptr);// validation / kick
		MH_CreateHook((LPVOID)(ImageBase + 0x1E23840), False, nullptr);// change gamesession id
		MH_CreateHook((LPVOID)(ImageBase + 0x108D740), DispatchRequest, (LPVOID*)&DispatchRequestOG);
		MH_CreateHook((LPVOID)(ImageBase + 0x3EB6780), AActorGetNetMode, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0x45C9D90), WorldGetNetMode, nullptr);

	}
}

namespace PC {

	void InitHooks()
	{
		Log("Hooking PC");

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x26B, ServerLoadingScreenDropped, (LPVOID*)&ServerLoadingScreenDroppedOG);

		HookVTable(UFortControllerComponent_Aircraft::GetDefaultObj(), 0x89, ServerAttemptAircraftJump, nullptr);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x10D, ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x20D, ServerExecuteInventoryItem, nullptr);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x269, ServerReadyToStartMatch, (LPVOID*)&ServerReadyToStartMatchOG);

		HookVTable(APlayerPawn_Athena_C::GetDefaultObj(), 0x1EA, ServerHandlePickup, (LPVOID*)&ServerHandlePickupOG);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x21D, ServerAttemptInventoryDrop, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x230, ServerCreateBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x237, ServerBeginEditingBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x235, ServerEndEditingBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x232, ServerEditBuildingActor, nullptr);

		HookVTable(AAthena_PlayerController_C::GetDefaultObj(), 0x22C, ServerRepairBuildingActor, nullptr);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x41E, ServerAttemptExitVehicle, (PVOID*)&ServerAttemptExitVehicleOG);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x265, ServerReturnToMainMenu, nullptr);

		HookVTable(APlayerPawn_Athena_C::StaticClass()->DefaultObject, 0x1D2, ServerReviveFromDBNO, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x29B5C80), ClientOnPawnDied, (LPVOID*)&ClientOnPawnDiedOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x1C8EA00), ServerAttemptInteract, (LPVOID*)&ServerAttemptInteractOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x2683F80), OnDamageServer, (LPVOID*)&OnDamageServerOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x1583360), ApplyCost, (LPVOID*)&ApplyCostOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x22A08C0), OnCapsuleBeginOverlap, (LPVOID*)&OnCapsuleBeginOverlapOG);

		//MH_CreateHook((LPVOID)(ImageBase + 0x0), ServerPlayEmoteItem, nullptr);
		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x1C7, ServerPlayEmoteItem, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x197f6d0), ServerPlaySquadQuickChatMessage, nullptr);

		HookVTable(AFortPlayerStateAthena::StaticClass()->DefaultObject, 0xFF, ServerSetInAircraft, (PVOID*)&OrginalServerSetInAircraft);

		for (size_t i = 0; i < UObject::GObjects->Num(); i++)
		{
			UObject* Obj = UObject::GObjects->GetByIndex(i);
			if (Obj && Obj->IsA(AB_Prj_Athena_Consumable_Thrown_C::StaticClass()))
			{
				HookVTable(Obj->Class->DefaultObject, 0x44, OnExploded, nullptr);
			}
		}
	}
}

namespace Tick {

	void InitHooks()
	{
		Log("Hooking Tick");

		MH_CreateHook((LPVOID)(ImageBase + 0x42C3ED0), TickFlush, (LPVOID*)&TickFlushOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x4576310), GetMaxTickRate, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x1B46D00), SpawnLoot, nullptr);

	}
}

namespace Abilities {

	void InitHooks()
	{
		Log("Hooking Abilities");
		HookVTable(UAbilitySystemComponent::GetDefaultObj(), 0xFA, InternalServerTryActivateAbility, nullptr);
		HookVTable(UFortAbilitySystemComponent::GetDefaultObj(), 0xFA, InternalServerTryActivateAbility, nullptr);
		HookVTable(UFortAbilitySystemComponentAthena::GetDefaultObj(), 0xFA, InternalServerTryActivateAbility, nullptr);
	}
}

namespace Inventory {

	void InitHooks()
	{
		Log("Hooking Inventory");

		HookVTable(AFortPlayerPawnAthena::GetDefaultObj(), 0x119, NetMulticastDamageCues, (LPVOID*)&NetMulticastDamageCuesOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x260C490), OnReload, (LPVOID*)&OnReloadOG);
	}
}

namespace Phoebe {

	void InitHooks()
	{
		Log("Hooking Phoebe");
		MH_CreateHook((LPVOID)(ImageBase + 0x19E9B10), SpawnBot, (LPVOID*)&SpawnBotOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x1637F50), SetUpInventoryBot, (LPVOID*)&SetUpInventoryBotOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x163C760), OnPossessedPawnDied, (LPVOID*)&OnPossessedPawnDiedOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x163C300), OnPerceptionSensed, (LPVOID*)&OnPerceptionSensedOG);
	}
}
