#pragma once
#include "Utils.h"
#include "Inventory.h"

void Siphon(AFortPlayerPawnAthena* Pawn)
{

	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;

	if (!PC || !PC->MyFortPawn || !Globals::SiphonEnabled)
		return;

	if (!PC->IsA(AFortPlayerControllerAthena::StaticClass()))
		return;

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

			int health = PC->MyFortPawn->GetHealth() + 50;
			bool GiveShield = health > 100;
			int ShieldAmount = PC->MyFortPawn->GetShield() + (health - 100);
			if (health > 100)
				health = 100;

			if (ShieldAmount > 100)
				ShieldAmount = 100;

			PC->MyFortPawn->SetHealth(health);
			if (GiveShield)
				PC->MyFortPawn->SetShield(ShieldAmount);

			static UFortItemDefinition* Wood = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
			static UFortItemDefinition* Stone = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
			static UFortItemDefinition* Metal = StaticLoadObject<UFortItemDefinition>("/Game/Items/ResourcePickups/MetalItemData.MetalItemData");

			GiveItem(PC, Wood, 50, 0);
			GiveItem(PC, Stone, 50, 0);
			GiveItem(PC, Metal, 50, 0);
			break;
		}
	}
}