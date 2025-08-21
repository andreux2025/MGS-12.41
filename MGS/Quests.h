#pragma once
#include "Utils.h"

inline void GiveAccolade(AFortPlayerControllerAthena* PC, UFortAccoladeItemDefinition* Def)
{
	if (!Def) return;

	FAthenaAccolades Accolade{};
	Accolade.AccoladeDef = Def;
	Accolade.Count = 1;
	std::string DefName = Def->GetName();
	Accolade.TemplateId = std::wstring(DefName.begin(), DefName.end()).c_str();

	auto ID = UKismetSystemLibrary::GetDefaultObj()->GetPrimaryAssetIdFromObject(Def);

	FXPEventInfo EventInfo{};
	EventInfo.Accolade = ID;
	EventInfo.EventName = Def->Name;
	EventInfo.EventXpValue = Def->GetAccoladeXpValue();
	EventInfo.Priority = Def->Priority;
	EventInfo.SimulatedText = Def->GetShortDescription();
	EventInfo.RestedValuePortion = EventInfo.EventXpValue;
	EventInfo.RestedXPRemaining = EventInfo.EventXpValue;
	EventInfo.SeasonBoostValuePortion = 20;
	EventInfo.TotalXpEarnedInMatch = EventInfo.EventXpValue + PC->XPComponent->TotalXpEarned;

	PC->XPComponent->MedalBonusXP += 1250;
	PC->XPComponent->MatchXp += EventInfo.EventXpValue;
	PC->XPComponent->TotalXpEarned += EventInfo.EventXpValue + 1250;

	PC->XPComponent->PlayerAccolades.Add(Accolade);
	PC->XPComponent->MedalsEarned.Add(Def);

	PC->XPComponent->ClientMedalsRecived(PC->XPComponent->PlayerAccolades);
	PC->XPComponent->OnXPEvent(EventInfo);
}

void ProgressQuest(AFortPlayerControllerAthena* PC, UFortQuestItemDefinition* QuestDef, FName BackendName)
{
	PC->GetQuestManager(ESubGame::Athena)->SelfCompletedUpdatedQuest(PC, QuestDef, BackendName, 1, 1, nullptr, true, false);
	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
	for (size_t i = 0; i < PlayerState->PlayerTeam->TeamMembers.Num(); i++)
	{
		auto pc = (AFortPlayerControllerAthena*)PlayerState->PlayerTeam->TeamMembers[i];
		if (pc && pc != PC)
		{
			pc->GetQuestManager(ESubGame::Athena)->SelfCompletedUpdatedQuest(PC, QuestDef, BackendName, 1, 1, PlayerState, true, false);
		}
	}
	auto QuestItem = PC->GetQuestManager(ESubGame::Athena)->GetQuestWithDefinition(QuestDef);

	int32 XPCount = 0;


	if (auto RewardsTable = QuestDef->RewardsTable)
	{
		static auto Name = FName(L"Default");

		auto DefaultRow = RewardsTable->Search([](FName& RName, uint8* Row) { return RName == Name; });

		XPCount = (*(FFortQuestRewardTableRow**)DefaultRow)->Quantity;
	}

	FXPEventEntry Entry{};
	Entry.EventXpValue = XPCount;
	Entry.QuestDef = QuestDef;
	Entry.Time = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
	PC->XPComponent->ChallengeXp += Entry.EventXpValue;
	PC->XPComponent->TotalXpEarned += Entry.EventXpValue;
	Entry.TotalXpEarnedInMatch = PC->XPComponent->TotalXpEarned;
	Entry.SimulatedXpEvent = QuestDef->GetSingleLineDescription();
	PC->XPComponent->RestXP += Entry.EventXpValue;
	PC->XPComponent->InMatchProfileVer++;
	PC->XPComponent->OnInMatchProfileUpdate(PC->XPComponent->InMatchProfileVer);
	PC->XPComponent->OnProfileUpdated();
	PC->XPComponent->OnXpUpdated(PC->XPComponent->CombatXp, PC->XPComponent->SurvivalXp, PC->XPComponent->MedalBonusXP, PC->XPComponent->ChallengeXp, PC->XPComponent->MatchXp, PC->XPComponent->TotalXpEarned);
	PC->XPComponent->WaitingQuestXp.Add(Entry);

	cout << PC->XPComponent->WaitingQuestXp.Num() << endl;
	PC->XPComponent->HighPrioXPEvent(Entry);
}

