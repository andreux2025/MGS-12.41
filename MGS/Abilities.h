#pragma once
#include <string>
#include "Utils.h"
#include <vector>
#include <map>

FGameplayAbilitySpec* GiveAbility(AFortPlayerControllerAthena* PC, UClass* GameplayAbilityClass, UObject* CertainDef)
{
	if (!PC || !GameplayAbilityClass || !PC->MyFortPawn) {
		return nullptr;
	}

	auto AbilitySystemComponent = PC->MyFortPawn->AbilitySystemComponent; 

	FGameplayAbilitySpec Spec{};

	CombineStruct(&Spec, (UGameplayAbility*)GameplayAbilityClass->DefaultObject, 1, -1, CertainDef);

	GiveAbilityAndActivateOnce(AbilitySystemComponent, &Spec.Handle, Spec);

	return &Spec;
}

void GiveAbilitySet(AFortPlayerControllerAthena* PC, UFortAbilitySet* Set)
{
	if (Set)
	{
		for (int32 i = 0; i < Set->GameplayAbilities.Num(); i++)
		{
			UClass* Ability = Set->GameplayAbilities[i].Get();
			if (Ability) { GiveAbility(PC, Ability, nullptr); }
		}
	}
}

FGameplayAbilitySpec* FindAbilitySpec(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle) {

	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->ActivatableAbilities.Items) {
		if (Spec.Handle.Handle == Handle.Handle) {
			if (!Spec.PendingRemove) {
				return &Spec;
			}
		}
	}

	return nullptr;
}

void InternalServerTryActivateAbility(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, const struct FPredictionKey& InPredictionKey, FGameplayEventData* TriggerEventData)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpec(AbilitySystemComponent, Handle);
	if (!Spec) {

		return AbilitySystemComponent->ClientActivateAbilityFailed(Handle, InPredictionKey.Current);

	}
	UGameplayAbility* AbilityToActivate = Spec->Ability;

	if (!AbilityToActivate)
	{

		return AbilitySystemComponent->ClientActivateAbilityFailed(Handle, InPredictionKey.Current);
	}

	UGameplayAbility* InstancedAbility = nullptr;
	Spec->InputPressed = true;

	if (InternalTryActivateAbility(AbilitySystemComponent, Handle, InPredictionKey, &InstancedAbility, nullptr, TriggerEventData)) {

	}
	else {

		AbilitySystemComponent->ClientActivateAbilityFailed(Handle, InPredictionKey.Current);
		Spec->InputPressed = false;

		AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(*Spec);
	}
}

//kinda skidded
void GiveModifier(UFortGameplayModifierItemDefinition* Modifier, AFortPlayerControllerAthena* PC)
{

	TArray<FFortAbilitySetDeliveryInfo>& Abilities = *(TArray<FFortAbilitySetDeliveryInfo>*)(__int64(Modifier) + 0x0398);
	TArray<FFortGameplayEffectDeliveryInfo>& Effects = *(TArray<FFortGameplayEffectDeliveryInfo>*)(__int64(Modifier) + 0x0388);

	for (size_t i = 0; i < Abilities.Num(); i++)
	{
		auto& Abi = Abilities[i];
		if (Abi.DeliveryRequirements.bApplyToPlayerPawns)
		{
			for (size_t j = 0; j < Abi.AbilitySets.Num(); j++)
			{
				UFortAbilitySet* Set = StaticLoadObject<UFortAbilitySet>(UKismetStringLibrary::Conv_NameToString(Abi.AbilitySets[j].ObjectID.AssetPathName).ToString());
				GiveAbilitySet(PC, Set);
			}
		}
	}

	for (size_t i = 0; i < Effects.Num(); i++)
	{
		auto& Eff = Effects[i];
		if (Eff.DeliveryRequirements.bApplyToPlayerPawns)
		{
			for (size_t j = 0; j < Eff.GameplayEffects.Num(); j++)
			{
				UClass* Effect = StaticLoadObject<UClass>(UKismetStringLibrary::Conv_NameToString(Eff.GameplayEffects[j].GameplayEffect.ObjectID.AssetPathName).ToString());

				PC->MyFortPawn->AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(Effect, Eff.GameplayEffects[j].Level, FGameplayEffectContextHandle());
			}
		}
	}
}