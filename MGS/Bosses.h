#pragma once
#include "Utils.h"
#include <intrin.h>
#include "Looting.h"

//saying this is not skidded is a lie i mean i worked on it alot on ero so why waste it???

vector<class Bot*> Bots{};
UFortServerBotManagerAthena* botManager;
auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

class Bot
{
public:
	ABP_PhoebePlayerController_C* PC;
	AFortPlayerPawnAthena* Pawn;
	bool bTickEnabled = false;
	AActor* CurrentTarget = nullptr;
	uint64_t tick_counter = 0;
	UFortWeaponItemDefinition* Weapon = nullptr;
	FGuid WeaponGuid;
	FGuid PickaxeGuid;
	UFortWeaponMeleeItemDefinition* Pickaxe = nullptr;
	bool jumping = false;
	int shotCounter = 0;
	AActor* FollowTarget;
	bool follow = true;
	EAlertLevel AlertLevel = EAlertLevel::Unaware;
	double MovementDistance = 0;
	FVector lastLocation{};
	std::string CID = "";
	std::string Name = "";
	FVector CurrentPatrolPointLoc;
	int CurrentPatrolPoint = 0;
	int MaxPatrolPoints = 0;
	thread* PatrolThread = nullptr;
	AFortAthenaPatrolPath* PatrolPath = nullptr;
	bool bIsPatrolling = false;
	bool bIsPatrollingBack = false;
	bool bIsWaitingForNextPatrol = false;
	bool bIsStuck = false;
	bool bIsCurrentlyStrafing = false;
	Bot* CurrentAssignedDBNOBot;
	Bot* CurrentlyDownedBot;

public:
	Bot(AFortPlayerPawnAthena* Pawn)
	{
		this->Pawn = Pawn;
		PC = (ABP_PhoebePlayerController_C*)Pawn->Controller;
		Bots.push_back(this);
	}

public:

	FGuid GetItem(UFortItemDefinition* Def)
	{
		for (int32 i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
				return PC->Inventory->Inventory.ReplicatedEntries[i].ItemGuid;
		}
		return FGuid();
	}

	void GiveItem(UFortItemDefinition* ODef, int Count = 1, bool equip = true)
	{
		UFortItemDefinition* Def = ODef;
		if (Def->GetName() == "AGID_Boss_Tina") {
			Def = StaticLoadObject<UFortItemDefinition>("/Game/Athena/Items/Weapons/Boss/WID_Boss_Tina.WID_Boss_Tina");
		}
		UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);
		Item->OwnerInventory = PC->Inventory;

		if (Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
		{
			Item->ItemEntry.LoadedAmmo = GetClipSize((UFortWeaponItemDefinition*)Def);
		}

		PC->Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->Inventory->Inventory.ItemInstances.Add(Item);
		PC->Inventory->Inventory.MarkItemDirty(Item->ItemEntry);
		PC->Inventory->HandleInventoryLocalUpdate();
		if (Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()) && equip)
		{
			this->Weapon = (UFortWeaponItemDefinition*)ODef;
			this->WeaponGuid = GetItem(Def);
			Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Def, GetItem(Def));
		}
		if (Def->GetName() == "WID_Harvest_Pickaxe_Athena_C_T01") {
			this->Pickaxe = (UFortWeaponMeleeItemDefinition*)Def;
			this->PickaxeGuid = GetItem(Def);
		}
	}

	virtual void OnPerceptionSensed(AActor* SourceActor, FAIStimulus& Stimulus)
	{
		if (Stimulus.bSuccessfullySensed == 1 && PC->LineOfSightTo(SourceActor, FVector(), true) && Pawn->GetDistanceTo(SourceActor) < 5000)
		{
			CurrentTarget = SourceActor;
		}
	}
};

inline FVector GetPatrolLocation(Bot* bot) {
	bot->MaxPatrolPoints = (bot->PatrolPath->PatrolPoints.Num() - 1);
	FVector Loc{};
	if (!bot || !bot->PatrolPath || bot->PatrolPath->PatrolPoints.Num() == 0)
		return Loc;

	if (bot->MaxPatrolPoints < 1)
	{
		Log("Not enough patrol points for " + bot->PC->GetFullName() + ", cancelling patrol.");
		return Loc;
	}

	/*if (!bot->CurrentPatrolPointLoc.IsZero()) {
		auto BotPos = bot->Pawn->K2_GetActorLocation();
		auto TestRot = Math->FindLookAtRotation(BotPos, bot->CurrentPatrolPointLoc);

		bot->PC->SetControlRotation(TestRot);
		bot->PC->K2_SetActorRotation(TestRot, true);

		if (Math->Vector_Distance(BotPos, bot->CurrentPatrolPointLoc) <= 600.0f) {}
		else {
			return bot->CurrentPatrolPointLoc;
		}
	}*/

		if (!bot->bIsPatrollingBack) {
			if ((bot->CurrentPatrolPoint + 1) >= bot->MaxPatrolPoints) {
				bot->bIsPatrollingBack = true;
			}
			bot->CurrentPatrolPoint = bot->CurrentPatrolPoint + 1;
			if (!bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]) {
				bot->bIsPatrollingBack = true;
			}
			else {
				FVector PatrolPointLoc = bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]->K2_GetActorLocation();
				if (PatrolPointLoc.IsZero())
				{
					Log("No Patrol Point Location For Index: " + std::to_string(bot->CurrentPatrolPoint));
				}
				else
				{
					//AActor* PatrolTarget = SpawnActor<ADefaultPawn>(PatrolPointLoc, FRotator(0.f, 0.f, 0.f));
					//Log("Currently Going To The " + std::to_string(bot->CurrentPatrolPoint) + " Patrol Point Max: " + std::to_string(bot->MaxPatrolPoints) + " for bot: " + bot->Name);
					//Log("PatrolPointLoc [" + std::to_string(bot->CurrentPatrolPoint) + "]: (" + std::to_string(PatrolPointLoc.X) + ", " + std::to_string(PatrolPointLoc.Y) + ", " + std::to_string(PatrolPointLoc.Z) + ")");
					auto BotPos = bot->Pawn->K2_GetActorLocation();
					auto TestRot = Math->FindLookAtRotation(BotPos, PatrolPointLoc);

					bot->PC->SetControlRotation(TestRot);
					bot->PC->K2_SetActorRotation(TestRot, true);
					Loc = PatrolPointLoc;
				}
			}
		}
		else {
			if ((bot->CurrentPatrolPoint - 1) <= 0) {
				bot->bIsPatrollingBack = false;
			}
			bot->CurrentPatrolPoint = bot->CurrentPatrolPoint - 1;
			if (!bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]) {
				bot->bIsPatrollingBack = false;
			}
			else {
				FVector PatrolPointLoc = bot->PatrolPath->PatrolPoints[bot->CurrentPatrolPoint]->K2_GetActorLocation();
				if (PatrolPointLoc.IsZero())
				{
					Log("No Patrol Point Location For Index: " + std::to_string(bot->CurrentPatrolPoint));
				}
				else
				{
					//AActor* PatrolTarget = SpawnActor<ADefaultPawn>(PatrolPointLoc, FRotator(0.f, 0.f, 0.f));
					//Log("Currently Going To The " + std::to_string(bot->CurrentPatrolPoint) + " Patrol Point Max: " + std::to_string(bot->MaxPatrolPoints) + " for bot: " + bot->Name);
					//Log("PatrolPointLoc [" + std::to_string(bot->CurrentPatrolPoint) + "]: (" + std::to_string(PatrolPointLoc.X) + ", " + std::to_string(PatrolPointLoc.Y) + ", " + std::to_string(PatrolPointLoc.Z) + ")");
					auto BotPos = bot->Pawn->K2_GetActorLocation();
					auto TestRot = Math->FindLookAtRotation(BotPos, PatrolPointLoc);

					bot->PC->SetControlRotation(TestRot);
					bot->PC->K2_SetActorRotation(TestRot, true);
					Loc = PatrolPointLoc;
				}
			}
		}
	

	bot->CurrentPatrolPointLoc = Loc;
	return Loc;
}

void StopPatrolling(Bot* bot)
{
	if (bot->bIsPatrolling)
	{
		bot->bIsPatrolling = false;
		/*if (bot->PatrolThread)
		{
			if (bot->PatrolThread->joinable())
				bot->PatrolThread->join();
			delete bot->PatrolThread;
			bot->PatrolThread = nullptr;
		}*/
		if (bot->PC)
			bot->PC->StopMovement();
	}
}

inline void TickBots()
{
	auto block = [](Bot* bot, std::function<void(Bot* bot)> const& SetUnaware, bool Alerted, bool Threatened, bool LKP) {
		float CurrentTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
		FVector Vel = bot->Pawn->GetVelocity();
		float Speed = sqrtf(Vel.X * Vel.X + Vel.Y * Vel.Y);

		if (bot->Pawn->bIsDBNO) {
			AFortTeamInfo* TeamIndex = ((AFortPlayerStateAthena*)bot->PC->PlayerState)->PlayerTeam;
			FVector BotLoc = bot->Pawn->K2_GetActorLocation();
			float ClosestDistance = FLT_MAX;
			Bot* ClosestBot = nullptr;

			if (!bot->CurrentAssignedDBNOBot) {
				for (int i = 0; i < Bots.size(); i++) {
					if (TeamIndex == ((AFortPlayerStateAthena*)Bots[i]->PC->PlayerState)->PlayerTeam && !Bots[i]->Pawn->bIsDBNO) {
						float Dist = Math->Vector_Distance(BotLoc, Bots[i]->Pawn->K2_GetActorLocation());

						if (Dist < ClosestDistance) {
							ClosestDistance = Dist;
							ClosestBot = Bots[i];
						}
					}
				}
			}

			if (ClosestBot && !bot->CurrentAssignedDBNOBot) {
				bot->CurrentAssignedDBNOBot = ClosestBot;
				bot->CurrentAssignedDBNOBot->CurrentlyDownedBot = bot;
				Log("assigned the downed bot!");
			}

			/*if (bot->CurrentAssignedDBNOBot) {
				bot->PC->MoveToActor(bot->CurrentAssignedDBNOBot->PC, 50, true, false, true, nullptr, true);
				//bot->CurrentAssignedDBNOBot->CurrentlyDownedBot = bot;
				bot->CurrentAssignedDBNOBot->PC->MoveToActor(bot->PC, 50, true, false, true, nullptr, true);
			}*/

			//return;
		}

		/*if (bot->CurrentlyDownedBot && bot->CurrentlyDownedBot->Pawn->bIsDBNO) {
			bot->PC->MoveToActor(bot->CurrentlyDownedBot->PC, 50, true, true, true, nullptr, true);
			return;
		}*/
		
		if (!Threatened && !Alerted && bot->PatrolPath && !bot->bIsPatrolling) {
			bot->MaxPatrolPoints = (bot->PatrolPath->PatrolPoints.Num() - 1);
			//Log("Started Patrolling " + std::to_string(bot->MaxPatrolPoints) + " Patrol Points for bot: " + bot->Name);
			bot->bIsPatrolling = true;
		}
		
		if (!Threatened && !Alerted && bot->PatrolPath && bot->bIsPatrolling) {
			//PatrolBot(bot);
			/*bot->PatrolThread = new std::thread([bot]() {
				PatrolBot(bot);
			});*/

			//bot->PC->MoveToLocation(GetPatrolLocation(bot), 20.f, true, true, true, true, nullptr, true);

			/*FVector ForwardVector = bot->Pawn->GetActorForwardVector();
			FVector BotPos = bot->Pawn->K2_GetActorLocation();
			FVector NewTargetLocation = BotPos + ForwardVector * 200.0f;*/

			if (bot->bIsWaitingForNextPatrol) {}
			else {
				/*if (bot->Pawn && bot->Pawn->CharacterMovement)
				{
				bot->Pawn->CharacterMovement->MaxWalkSpeed = 600.0f;
				bot->Pawn->CharacterMovement->MaxAcceleration = 2048.0f;
				}*/

				FVector BotPos = bot->Pawn->K2_GetActorLocation();

				if (bot->Pawn->bIsDBNO && bot->CurrentAssignedDBNOBot) {
					float Distance = Math->Vector_Distance(BotPos, bot->CurrentAssignedDBNOBot->Pawn->K2_GetActorLocation());
					auto TestRot = Math->FindLookAtRotation(BotPos, bot->CurrentAssignedDBNOBot->Pawn->K2_GetActorLocation());

					bot->PC->SetControlRotation(TestRot);
					bot->PC->K2_SetActorRotation(TestRot, true);

					if (Distance < 69.0f) {}
					else {
						bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
						//bot->PC->MoveToActor(bot->CurrentAssignedDBNOBot->PC, 50, true, false, true, nullptr, true);
					}

					return;
				}
				else if (bot->CurrentlyDownedBot && bot->CurrentlyDownedBot->Pawn->bIsDBNO) {
					float Distance = Math->Vector_Distance(bot->CurrentlyDownedBot->Pawn->K2_GetActorLocation(), BotPos);
					auto TestRot = Math->FindLookAtRotation(BotPos, bot->CurrentlyDownedBot->Pawn->K2_GetActorLocation());

					bot->PC->SetControlRotation(TestRot);
					bot->PC->K2_SetActorRotation(TestRot, true);

					if (Distance < 200.0f) {
						bot->CurrentlyDownedBot->Pawn->bIsDBNO = false;
						bot->CurrentlyDownedBot->Pawn->OnRep_IsDBNO();
						bot->Pawn->SetHealth(30);
						bot->CurrentlyDownedBot->Pawn->EquipWeaponDefinition(bot->Weapon, bot->WeaponGuid);

						return;
					}
					else {
						bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
						//bot->CurrentAssignedDBNOBot->PC->MoveToActor(bot->PC, 50, true, false, true, nullptr, true);
					}
				}
				else {
					float Distance = Math->Vector_Distance(BotPos, bot->CurrentPatrolPointLoc);

					if (!bot->CurrentPatrolPointLoc.IsZero()) {
						auto BotPos = bot->Pawn->K2_GetActorLocation();
						auto TestRot = Math->FindLookAtRotation(BotPos, bot->CurrentPatrolPointLoc);
						FVector Vel = bot->Pawn->GetVelocity();
						float Speed = sqrtf(Vel.X * Vel.X + Vel.Y * Vel.Y);

						bot->PC->SetControlRotation(TestRot);
						bot->PC->K2_SetActorRotation(TestRot, true);

						if (Distance < 190.0f) {
							bot->bIsWaitingForNextPatrol = true;
							bot->bIsStuck = false;
							bot->CurrentPatrolPointLoc = GetPatrolLocation(bot);
							std::thread([bot]() {
								Sleep(1500 + (rand() % 1500));
								bot->bIsWaitingForNextPatrol = false;
								//Log("Currently Going To The " + std::to_string(bot->CurrentPatrolPoint) + " Patrol Point Max: " + std::to_string(bot->MaxPatrolPoints) + " for bot: " + bot->Name);
							}).detach();
						}
					}
					else {
						bot->CurrentPatrolPointLoc = GetPatrolLocation(bot);
					}

					if (bot->lastLocation.IsZero()) {
						bot->lastLocation = bot->Pawn->K2_GetActorLocation();
						bot->bIsStuck = false;
					}

					if (!bot->CurrentPatrolPointLoc.IsZero()) {
						auto BotPos = bot->Pawn->K2_GetActorLocation();
						auto TestRot = Math->FindLookAtRotation(BotPos, bot->CurrentPatrolPointLoc);

						bot->PC->SetControlRotation(TestRot);
						bot->PC->K2_SetActorRotation(TestRot, true);

						bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.1f, true);
					}
					//bot->PC->MoveToLocation(bot->CurrentPatrolPointLoc, 300.0f, false, true, true, false, nullptr, false);
				}
			}
		}

		if (bot->tick_counter % 8 == 0 && bot->tick_counter > 30) {
			FVector NewLocation = bot->Pawn->K2_GetActorLocation();
			float MovedDistance = Math->Vector_Distance(bot->lastLocation, NewLocation);
			bot->lastLocation = bot->Pawn->K2_GetActorLocation();

			if (MovedDistance < 1.1f && !bot->bIsWaitingForNextPatrol)
			{
				/*if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.01f)) {
					Log("Bot is probably stuck, moved distance: " + std::to_string(MovedDistance));
				}*/
				bot->bIsStuck = true;
			}
			else {
				bot->Pawn->PawnStopFire(0);
				bot->bIsStuck = false;
				bot->Pawn->EquipWeaponDefinition(bot->Weapon, bot->WeaponGuid);
			}
		}

		/*if (Speed < 0.001f && !bot->bIsWaitingForNextPatrol && bot->tick_counter > 2000) {
			Log("Bot Speed: " + std::to_string(Speed));
			bot->bIsStuck = true;
		}*/

		if (!bot->CurrentTarget && !bot->follow) {
			bot->follow = true;
		}
		if (bot->bTickEnabled && bot->Pawn && bot->FollowTarget && bot->follow) {
			auto BotPos = bot->Pawn->K2_GetActorLocation();
			auto TargetPos = bot->FollowTarget->K2_GetActorLocation();
			auto TestRot = Math->FindLookAtRotation(BotPos, TargetPos);

			bot->PC->SetControlRotation(TestRot);
			bot->PC->K2_SetActorRotation(TestRot, true);
			bot->PC->MoveToActor(bot->FollowTarget, 50, true, false, true, nullptr, true);

			if (!bot->CurrentTarget
				&& bot->PC->PlayerBotPawn
				&& bot->FollowTarget
				&& bot->PC->PlayerBotPawn->PlayerState
				&& (AFortPlayerStateAthena*)((AFortPlayerPawnAthena*)bot->FollowTarget)->PlayerState
				&& ((AFortPlayerStateAthena*)bot->PC->PlayerBotPawn->PlayerState)->PlayerTeam
				!= ((AFortPlayerStateAthena*)((AFortPlayerPawnAthena*)bot->FollowTarget)->PlayerState)->PlayerTeam) {
				bot->CurrentTarget = bot->FollowTarget;
			}
		}
		if (bot->CurrentTarget
			&& bot->PC->PlayerBotPawn
			&& bot->PC->PlayerBotPawn->PlayerState
			&& ((AFortPlayerPawnAthena*)bot->CurrentTarget)->PlayerState
			&& ((AFortPlayerStateAthena*)bot->PC->PlayerBotPawn->PlayerState)->PlayerTeam
			== ((AFortPlayerStateAthena*)((AFortPlayerPawnAthena*)bot->CurrentTarget)->PlayerState)->PlayerTeam) {
			bot->CurrentTarget = nullptr;
		}
		if (bot->CurrentTarget && ((AFortPlayerPawnAthena*)bot->CurrentTarget)->IsDead()) {
			bot->CurrentTarget = nullptr;
			bot->Pawn->PawnStopFire(0);
		}
		if (bot->bTickEnabled
			&& bot->Pawn
			&& !bot->Pawn->bIsDBNO
			&& bot->CurrentTarget
			&& !((AFortPlayerPawnAthena*)bot->CurrentTarget)->IsDead())
		{
			if (bot->Pawn->CurrentWeapon && bot->Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponItemDefinition::StaticClass()))
			{
				auto BotPos = bot->Pawn->K2_GetActorLocation();
				auto TargetPos = bot->CurrentTarget->K2_GetActorLocation();
				float Distance = bot->Pawn->GetDistanceTo(bot->CurrentTarget);

				if (Alerted) {
					bot->Pawn->PawnStopFire(0);
					if (bot->bIsPatrolling) {
						StopPatrolling(bot);
					}
					else {
						bot->PC->StopMovement();
					}
					auto Rot = Math->FindLookAtRotation(BotPos, TargetPos);
					bot->PC->SetControlRotation(Rot);
					bot->PC->K2_SetActorRotation(Rot, true);
					bot->tick_counter++;
					if (bot->tick_counter % 150 == 0 && bot->PC->LineOfSightTo(bot->CurrentTarget, FVector(), true) && Distance < 2500) {
						bot->PC->CurrentAlertLevel = EAlertLevel::Threatened;
						bot->PC->OnAlertLevelChanged(bot->AlertLevel, EAlertLevel::Threatened);
					}
					if (bot->bIsStuck) {
						bot->bIsStuck = false;
					}
				}
				else if (Threatened) {
					if (Distance < 300 && bot->PC->LineOfSightTo(bot->CurrentTarget, FVector(), true)) {
						if (bot->bIsPatrolling) {
							StopPatrolling(bot);
						}
						else {
							bot->PC->StopMovement();
						}
					}
					if (!bot->Pawn->bIsCrouched && Math->RandomBoolWithWeight(0.01f)) {
						bot->Pawn->Crouch(false);
					}
					else if (Math->RandomBoolWithWeight(0.01f)) {
						bot->Pawn->Jump();
						bot->jumping = true;
					}
					else if (bot->Pawn->bIsCrouched && (bot->tick_counter % 30) == 0) {
						bot->Pawn->UnCrouch(false);
					}
					else if (bot->jumping && (bot->tick_counter % 10) == 0) {
						bot->Pawn->StopJumping();
						bot->jumping = false;
					}
				}
				else if (LKP) {
					bot->AlertLevel = EAlertLevel::LKP;
					auto LKPRun = [bot, BotPos, TargetPos]() {
						bot->Pawn->UnCrouch(false);
						bot->Pawn->StopJumping();
						bot->jumping = false;
						bot->PC->MoveToActor(bot->CurrentTarget, 150, true, false, true, nullptr, true);
						auto Rot = Math->FindLookAtRotation((FVector&)BotPos, (FVector&)TargetPos);
						bot->PC->SetControlRotation(Rot);
						bot->PC->K2_SetActorRotation(Rot, true);
						};
					bot->tick_counter++;

					if (bot->tick_counter % 30 == 0)
					{

						if (bot->Weapon)
						{
							bot->Pawn->EquipWeaponDefinition(bot->Weapon, bot->WeaponGuid);
							bot->Pawn->PawnStopFire(0);
						}

						bot->MovementDistance = 0;
					}
					else {
						FVector diff = BotPos - bot->lastLocation;
						double mchange = diff.X + diff.Y + diff.Z;
						bot->MovementDistance += mchange;
					}
					bot->lastLocation = BotPos;
					LKPRun();
					if (!bot->bIsStuck) {
						return;
					}
				}
				else {
					bot->CurrentTarget = nullptr;
					SetUnaware(bot);
					bot->tick_counter++;
					return;
				}

				std::string WeaponName = bot->Pawn->CurrentWeapon->Name.ToString();
				if (WeaponName.starts_with("B_Assault_MidasDrum_Athena_"))
				{
					if (bot->shotCounter >= 19) {
						bot->Pawn->PawnStopFire(0);
						bot->shotCounter = -27;
					}
				}
				else if (WeaponName.starts_with("B_Minigun_Athena_"))
				{
					if (bot->shotCounter >= 103) {
						bot->Pawn->PawnStopFire(0);
						bot->shotCounter = -27;
					}
				}
				else if (WeaponName.starts_with("B_Pistol_RapidFireSMG_Athena_"))
				{
					if (bot->shotCounter >= 15) {
						bot->Pawn->PawnStopFire(0);
						bot->shotCounter = -27;
					}
				}
				else if (WeaponName.starts_with("B_Assault_Auto_Zoom_SR_Child_Athena_"))
				{
					if (bot->shotCounter >= 44) {
						bot->Pawn->PawnStopFire(0);
						bot->shotCounter = -27;
					}
				}
				else if (WeaponName.starts_with("B_Assault_Heavy_Athena_"))
				{
					if (bot->shotCounter >= 147) {
						bot->Pawn->PawnStopFire(0);
						bot->shotCounter = -27;
					}
				}
				else if (WeaponName.starts_with("B_Assault_Heavy_SR_Athena_"))
				{
					if (bot->shotCounter >= 147) {
						bot->Pawn->PawnStopFire(0);
						bot->shotCounter = -27;
					}
				}
				else if (WeaponName.starts_with("B_Assault_Suppressed_Athena_"))
				{
					//skip
				}
				else if (WeaponName.starts_with("B_Pistol_AutoHeavy_Athena_Supp_Child_"))
				{
					if (bot->shotCounter >= 23)
					{
						bot->Pawn->PawnStopFire(0);
						bot->shotCounter = -27;
					}
				}
				else if (WeaponName.starts_with("B_Pistol_Vigilante_Athena_"))
				{
					//skip
				}
				else if (WeaponName.starts_with("B_TnTinaBow_Athena_"))
				{
					//skip
				}
				else if (WeaponName.starts_with("B_DualPistol_Donut_Athena_"))
				{
					if (bot->shotCounter >= 25) {
						bot->Pawn->PawnStopFire(0);
						bot->shotCounter = -35;
					}
				}
				else if (bot->shotCounter >= 19) {
					bot->Pawn->PawnStopFire(0);
					bot->shotCounter = -27;
				}

				if (Threatened) {
					FRotator TestRot;
					if (bot->tick_counter % 30 == 0) {
						float RandomXmod = ((rand() % 120000 - rand() % 30000) + 90000);
						float RandomYmod = ((rand() % 120000 - rand() % 30000) + 90000);
						//float RandomZmod = (rand() % 120000 - 60000);

						FVector TargetPosMod{ TargetPos.X + RandomXmod, TargetPos.Y + RandomYmod, TargetPos.Z };

						TestRot = Math->FindLookAtRotation(BotPos, TargetPosMod);
					}

					bot->follow = true;
					if (bot->tick_counter % 30 == 0) {
						bot->PC->SetControlRotation(TestRot);
						bot->PC->K2_SetActorRotation(TestRot, true);
					}
					bot->PC->MoveToActor(bot->CurrentTarget, rand() % 400 + 600, true, false, true, nullptr, true);
					
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
						bot->bIsStuck = false;
					}
					if (bot->tick_counter % 30 == 0) {
						if (bot->Weapon)
						{
							bot->Pawn->EquipWeaponDefinition(bot->Weapon, bot->WeaponGuid);

							if (bot->shotCounter >= 0) bot->Pawn->PawnStartFire(0);
						}

						bot->MovementDistance = 0;
					}
					else {
						FVector diff = BotPos - bot->lastLocation;
						double mchange = diff.X + diff.Y + diff.Z;
						bot->MovementDistance += mchange;
					}
					bot->lastLocation = BotPos;
					if (bot->Weapon) bot->Pawn->EquipWeaponDefinition(bot->Weapon, bot->WeaponGuid);
					bot->shotCounter++;
				}
			}
		}
		else if (bot->bTickEnabled && bot->Pawn && bot->Pawn->bIsDBNO)
		{
			bot->CurrentTarget = nullptr;
		}

		if (bot->bIsStuck && bot->tick_counter % 120 == 0)
		{
			bot->bIsStuck = false;
		}

		// Execute if the bot is stuck!
		if (bot->bIsStuck)
		{
			//Log("Bro is stuck");
			bot->Pawn->EquipWeaponDefinition(bot->Pickaxe, bot->PickaxeGuid);
			bot->Pawn->PawnStartFire(0);

			if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.0001f)) {
				bot->Pawn->Jump();
			}

			return;
		}

		bot->tick_counter++;
		};
	auto SetUnaware = [](Bot* bot) {
		bot->PC->CurrentAlertLevel = EAlertLevel::Unaware;
		bot->PC->OnAlertLevelChanged(bot->AlertLevel, EAlertLevel::Unaware);
		bot->Pawn->PawnStopFire(0);
		if (bot->PatrolPath) {
			bot->bIsPatrolling = true;
			/*bot->PatrolThread = new std::thread([bot]() {
				PatrolBot(bot);
			});*/
		}
		else {
			bot->PC->StopMovement();
		}
		};
	for (auto bot : Bots)
	{
		auto Alerted = bot->PC->CurrentAlertLevel == EAlertLevel::Alerted;
		auto Threatened = bot->PC->CurrentAlertLevel == EAlertLevel::Threatened;
		auto LKP = bot->PC->CurrentAlertLevel == EAlertLevel::LKP;
		block(bot, SetUnaware, Alerted, Threatened, LKP);
	}
}

wchar_t* (*OnPerceptionSensedOG)(ABP_PhoebePlayerController_C* PC, AActor* SourceActor, FAIStimulus& Stimulus);
wchar_t* OnPerceptionSensed(ABP_PhoebePlayerController_C* PC, AActor* SourceActor, FAIStimulus& Stimulus)
{
	if (SourceActor->IsA(AFortPlayerPawnAthena::StaticClass()) && Cast<AFortPlayerPawnAthena>(SourceActor)->Controller && !Cast<AFortPlayerPawnAthena>(SourceActor)->Controller->IsA(ABP_PhoebePlayerController_C::StaticClass()) /*!Cast<AFortPlayerPawnAthena>(SourceActor)->Controller->IsA(ABP_PhoebePlayerController_C::StaticClass())*/)
	{
		for (auto bot : Bots)
		{
			if (bot->PC == PC)
			{
				bot->OnPerceptionSensed(SourceActor, Stimulus);
			}
		}
	}
	return OnPerceptionSensedOG(PC, SourceActor, Stimulus);
}

static bool meowsclesSpawned = false;
void spawnMeowscles();

AFortPlayerPawnAthena* (*SpawnBotOG)(UFortServerBotManagerAthena* BotManager, FVector SpawnLoc, FRotator SpawnRot, UFortAthenaAIBotCustomizationData* BotData, FFortAthenaAIBotRunTimeCustomizationData RuntimeBotData);
AFortPlayerPawnAthena* SpawnBot(UFortServerBotManagerAthena* BotManager, FVector SpawnLoc, FRotator SpawnRot, UFortAthenaAIBotCustomizationData* BotData, FFortAthenaAIBotRunTimeCustomizationData RuntimeBotData)
{
	if (__int64(_ReturnAddress()) - __int64(GetModuleHandleA(0)) == 0x1A4153F) {
		return SpawnBotOG(BotManager, SpawnLoc, SpawnRot, BotData, RuntimeBotData);
	}

	std::string BotName = BotData->Name.ToString();

    if (BotData->GetFullName().contains("MANG_POI_Yacht"))
	{
		BotData = StaticLoadObject<UFortAthenaAIBotCustomizationData>("/Game/Athena/AI/MANG/BotData/BotData_MANG_POI_HDP.BotData_MANG_POI_HDP");
	}

	if (BotData->CharacterCustomization->CustomizationLoadout.Character->GetName() == "CID_556_Athena_Commando_F_RebirthDefaultA")
	{
		std::string Tag = RuntimeBotData.PredefinedCosmeticSetTag.TagName.ToString();
		if (Tag == "Athena.Faction.Alter") {
			BotData->CharacterCustomization->CustomizationLoadout.Character = StaticLoadObject<UAthenaCharacterItemDefinition>("/Game/Athena/Items/Cosmetics/Characters/CID_NPC_Athena_Commando_M_HenchmanBad.CID_NPC_Athena_Commando_M_HenchmanBad");
		}
		else if (Tag == "Athena.Faction.Ego") {
			BotData->CharacterCustomization->CustomizationLoadout.Character = StaticLoadObject<UAthenaCharacterItemDefinition>("/Game/Athena/Items/Cosmetics/Characters/CID_NPC_Athena_Commando_M_HenchmanGood.CID_NPC_Athena_Commando_M_HenchmanGood");
		}
	} 

	AActor* SpawnLocator = SpawnActor<ADefaultPawn>(SpawnLoc, SpawnRot);
	AFortPlayerPawnAthena* Ret = BotMutator->SpawnBot(BotData->PawnClass, SpawnLocator, SpawnLoc, SpawnRot, true);
	AFortAthenaAIBotController* PC = (AFortAthenaAIBotController*)Ret->Controller;

	static UBehaviorTree* BehaviorTree = StaticLoadObject<UBehaviorTree>("/Game/Athena/AI/MANG/BehaviorTree/BT_MANG2.BT_MANG2");
	PC->BehaviorTree = BehaviorTree;
	PC->RunBehaviorTree(BehaviorTree);
	PC->UseBlackboard(BehaviorTree->BlackboardAsset, &PC->Blackboard);

	PC->CosmeticLoadoutBC = BotData->CharacterCustomization->CustomizationLoadout;
	PC->CachedPatrollingComponent = (UFortAthenaNpcPatrollingComponent*)UGameplayStatics::SpawnObject(UFortAthenaNpcPatrollingComponent::StaticClass(), PC);
	for (int32 i = 0; i < BotData->CharacterCustomization->CustomizationLoadout.Character->HeroDefinition->Specializations.Num(); i++)
	{
		UFortHeroSpecialization* Spec = StaticLoadObject<UFortHeroSpecialization>(UKismetStringLibrary::Conv_NameToString(BotData->CharacterCustomization->CustomizationLoadout.Character->HeroDefinition->Specializations[i].ObjectID.AssetPathName).ToString());

		if (Spec)
		{
			for (int32 i = 0; i < Spec->CharacterParts.Num(); i++)
			{
				UCustomCharacterPart* Part = StaticLoadObject<UCustomCharacterPart>(UKismetStringLibrary::Conv_NameToString(Spec->CharacterParts[i].ObjectID.AssetPathName).ToString());
				Ret->ServerChoosePart(Part->CharacterPartType, Part);
			}
		}
	}

	Ret->CosmeticLoadout = BotData->CharacterCustomization->CustomizationLoadout;
	Ret->OnRep_CosmeticLoadout();

	SpawnLocator->K2_DestroyActor();
	DWORD CustomSquadId = RuntimeBotData.CustomSquadId;
	BYTE TrueByte = 1;
	BYTE FalseByte = 0;

	BotManagerSetup(__int64(BotManager), __int64(Ret), __int64(BotData->BehaviorTree), 0, &CustomSquadId, 0, __int64(BotData->StartupInventory), __int64(BotData->BotNameSettings), 0, &FalseByte, 0, &TrueByte, RuntimeBotData);

	Bot* bot = new Bot(Ret);

	bot->CID = BotData->CharacterCustomization->CustomizationLoadout.Character->GetName();

	for (int32 i = 0; i < BotData->StartupInventory->Items.Num(); i++)
	{
		bool equip = true;

		if (BotData->StartupInventory->Items[i]->GetName().starts_with("WID_Athena_FloppingRabbit") || BotData->StartupInventory->Items[i]->GetName().starts_with("WID_Boss_Adventure_GH")) {
			equip = false;
		}
		bot->GiveItem(BotData->StartupInventory->Items[i], 1, equip);
		if (auto Data = Cast<UFortWeaponItemDefinition>(BotData->StartupInventory->Items[i]))
		{
			if (Data->GetAmmoWorldItemDefinition_BP() && Data->GetAmmoWorldItemDefinition_BP() != Data)
			{
				bot->GiveItem(Data->GetAmmoWorldItemDefinition_BP(), 99999);
			}
		}
	}

	TArray<AFortAthenaPatrolPath*> PatrolPaths;

	Statics->GetAllActorsOfClass(UWorld::GetWorld(), AFortAthenaPatrolPath::StaticClass(), (TArray<AActor*>*) & PatrolPaths);

	bot->Name = BotName;
	for (int i = 0; i < PatrolPaths.Num(); i++) {
		if (PatrolPaths[i]->PatrolPoints[0]->K2_GetActorLocation() == SpawnLoc) {
			//Log("Found patrol path for bot: " + BotData->GetFullName());
			bot->PC->CachedPatrollingComponent->SetPatrolPath(PatrolPaths[i]);
			PC->CachedPatrollingComponent->SetPatrolPath(PatrolPaths[i]);
			bot->PatrolPath = PatrolPaths[i];
		}
	}

	/*if (BotData->GetFullName().contains("BP_MangPlayerPawn_Boss_Midas")) {
		bot->PC->PatrolPath = StaticLoadObject<AFortAthenaPatrolPath>("/Game/Athena/AI/MANG/Apollo_POI_Agency_MANG.Apollo_POI_Agency_MANG");
		if (PatrolPath)
		{
			PC->CachedPatrollingComponent->SetPatrolPath(PatrolPath);
		}
	}*/

	bot->bTickEnabled = true;
	botManager = BotManager;
	if (!meowsclesSpawned)
		spawnMeowscles();
	return Ret;
}

AFortPlayerPawnAthena* SpawnMeowsclesBot(UFortServerBotManagerAthena* BotManager, FVector SpawnLoc, FRotator SpawnRot, UFortAthenaAIBotCustomizationData* BotData, FFortAthenaAIBotRunTimeCustomizationData RuntimeBotData)
{
	std::string BotName = BotData->Name.ToString();

	AActor* SpawnLocator = SpawnActor<ADefaultPawn>(SpawnLoc, SpawnRot);
	AFortPlayerPawnAthena* Ret = BotMutator->SpawnBot(BotData->PawnClass, SpawnLocator, SpawnLoc, SpawnRot, true);
	AFortAthenaAIBotController* PC = (AFortAthenaAIBotController*)Ret->Controller;

	static UBehaviorTree* BehaviorTree = StaticLoadObject<UBehaviorTree>("/Game/Athena/AI/MANG/BehaviorTree/BT_MANG2.BT_MANG2");
	PC->BehaviorTree = BehaviorTree;
	PC->RunBehaviorTree(BehaviorTree);
	PC->UseBlackboard(BehaviorTree->BlackboardAsset, &PC->Blackboard);

	PC->CosmeticLoadoutBC = BotData->CharacterCustomization->CustomizationLoadout;
	for (int32 i = 0; i < BotData->CharacterCustomization->CustomizationLoadout.Character->HeroDefinition->Specializations.Num(); i++)
	{
		UFortHeroSpecialization* Spec = StaticLoadObject<UFortHeroSpecialization>(UKismetStringLibrary::Conv_NameToString(BotData->CharacterCustomization->CustomizationLoadout.Character->HeroDefinition->Specializations[i].ObjectID.AssetPathName).ToString());

		if (Spec)
		{
			for (int32 i = 0; i < Spec->CharacterParts.Num(); i++)
			{
				UCustomCharacterPart* Part = StaticLoadObject<UCustomCharacterPart>(UKismetStringLibrary::Conv_NameToString(Spec->CharacterParts[i].ObjectID.AssetPathName).ToString());
				Ret->ServerChoosePart(Part->CharacterPartType, Part);
			}
		}
	}

	Ret->CosmeticLoadout = BotData->CharacterCustomization->CustomizationLoadout;
	Ret->OnRep_CosmeticLoadout();

	SpawnLocator->K2_DestroyActor();
	DWORD CustomSquadId = RuntimeBotData.CustomSquadId;
	BYTE TrueByte = 1;
	BYTE FalseByte = 0;

	BotManagerSetup(__int64(BotManager), __int64(Ret), __int64(BotData->BehaviorTree), 0, &CustomSquadId, 0, __int64(BotData->StartupInventory), __int64(BotData->BotNameSettings), 0, &FalseByte, 0, &TrueByte, RuntimeBotData);

	Bot* bot = new Bot(Ret);

	bot->CID = BotData->CharacterCustomization->CustomizationLoadout.Character->GetName();

	for (int32 i = 0; i < BotData->StartupInventory->Items.Num(); i++)
	{
		bool equip = true;

		if (BotData->StartupInventory->Items[i]->GetName().starts_with("WID_Athena_FloppingRabbit") || BotData->StartupInventory->Items[i]->GetName().starts_with("WID_Boss_Adventure_GH")) {
			equip = false;
		}
		bot->GiveItem(BotData->StartupInventory->Items[i], 1, equip);
		if (auto Data = Cast<UFortWeaponItemDefinition>(BotData->StartupInventory->Items[i]))
		{
			if (Data->GetAmmoWorldItemDefinition_BP() && Data->GetAmmoWorldItemDefinition_BP() != Data)
			{
				bot->GiveItem(Data->GetAmmoWorldItemDefinition_BP(), 99999);
			}
		}
	}

	TArray<AFortAthenaPatrolPath*> PatrolPaths;

	Statics->GetAllActorsOfClass(UWorld::GetWorld(), AFortAthenaPatrolPath::StaticClass(), (TArray<AActor*>*) & PatrolPaths);

	for (int i = 0; i < PatrolPaths.Num(); i++) {
		if (PatrolPaths[i]->PatrolPoints[0]->K2_GetActorLocation() == SpawnLoc) {
			bot->PC->CachedPatrollingComponent->SetPatrolPath(PatrolPaths[i]);
		}
	}

	bot->bTickEnabled = true;
	return Ret;
}

void spawnMeowscles() {
	if (meowsclesSpawned) return;
	meowsclesSpawned = true;

	UFortAthenaAIBotCustomizationData* customization = StaticLoadObject<UFortAthenaAIBotCustomizationData>("/Game/Athena/AI/MANG/BotData/BotData_MANG_POI_HMW_Alter.BotData_MANG_POI_HMW_Alter");
	if (customization) {
		Log("Meow Meow Exists!");
	}
	else {
		Log("Meow Meow Does NOT Exist!");
	}
	FFortAthenaAIBotRunTimeCustomizationData runtimeData{};
	runtimeData.CustomSquadId = 0;

	FVector SpawnLocs[] = {
		// First spawn location
		{
			-69728.0,
			81376.0,
			5684.0
		},
		// Second spawn location
		{
			-70912.0,
			81376.0,
			5684.0
		},
		// Third spawn location
		{
			-67696.0,
			81540.0,
			5672.0
		},
	};

	FRotator Rotation = {
		0.0,
		-179.9999f,
		0.0
	};

	int32 SpawnCount = sizeof(SpawnLocs) / sizeof(FVector);
	int32 randomIndex = static_cast<int32>(Math->RandomFloatInRange(0.f, static_cast<float>(SpawnCount) - 1));

	auto Meowscles = SpawnMeowsclesBot(botManager, SpawnLocs[randomIndex], Rotation, customization, runtimeData);
	//auto Meowscles = SpawnMeowsclesBot(botManager, SpawnLoc, Rotation, customization, runtimeData);
	Sleep(250);
	if (!Meowscles) {
		Log("Meowscles spawn failed!");
		return;
	}
	Meowscles->SetMaxShield(400);
	Meowscles->SetShield(400);
}