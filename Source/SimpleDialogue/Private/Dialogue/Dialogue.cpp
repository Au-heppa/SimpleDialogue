// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Dialogue/Dialogue.h"
#include "Runtime/Engine/Public/Internationalization/StringTable.h"
#include "Core/Public/Internationalization/StringTableCore.h"
#include "GameFramework/PlayerController.h"

#if WITH_EDITOR
#include "HAL/PlatformApplicationMisc.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "EdGraphNode_Comment.h"
#endif //

#include "Kismet/GameplayStatics.h"
#include "Dialogue/DialogueManager.h"

//=================================================================
// 
//=================================================================
void UDialogue::Init(class UDialogueManager *InDialogueManager, class APlayerController *InPlayerController, class AActor *InPlayer, class AActor *InActor, FGameplayTag InSpeakContext, FLatentActionInfo InLatentInfo)
{
	PlayerController = InPlayerController;
	DialogueManager = InDialogueManager;
	PlayerActor = InPlayer;
	DialogueTarget = InActor;
	SpeakContext = InSpeakContext;
	bIsActive = true;
	FinishedLatentInfo = InLatentInfo;

	LastClickedAsset = NULL;
	LastClickedOption = NAME_None;
}

//===============================================================================================================================
// 
//===============================================================================================================================
class UWorld *UDialogue::GetWorld() const
{
	if ( UWorld* LastWorld = CachedWorld.Get() )
	{
		return LastWorld;
	}

	if ( HasAllFlags(RF_ClassDefaultObject) )
	{
		// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
		return nullptr;
	}

	// Could be a GameInstance, could be World, could also be a WidgetTree, so we're just going to follow
	// the outer chain to find the world we're in.
	UObject* Outer = GetOuter();

	while ( Outer )
	{
		UWorld* World = Outer->GetWorld();
		if ( World )
		{
			CachedWorld = World;
			return World;
		}

		Outer = Outer->GetOuter();
	}

	return nullptr;
}

//=================================================================
// 
//=================================================================
bool UDialogue::GetChoicesLimited_Internal(TArray<int32> *OutChoices, int32 &OutMaxChoices) const
{
	int32 iMax = DialogueManager->GetMaxChoices();

	if (OutChoices)
	{
		OutChoices->Empty(iMax);
	}

	if (Choices.Num()  <= iMax)
	{
		if (OutChoices)
		{
			for (int32 i=0; i<Choices.Num(); i++)
			{
				OutChoices->Add(i);
			}
		}

		OutMaxChoices = 0;
		return false;
	}

	OutMaxChoices = Choices.Num();

	//Finally add choices we want to show always
	if (OutChoices)
	{
		int32 iCount = 0;
		for (int32 i=StartChoices; i<Choices.Num() && iCount < iMax; i++)
		{
			OutChoices->Add(i);
			iCount++;
		}
	}

	return true;
}

//=================================================================
// 
//=================================================================
bool UDialogue::HasMoreChoices(int32 InDirection) const
{
	if (InDirection == 0)
		return false;

	InDirection = FMath::Clamp(InDirection, -1, 1);

	if (InDirection > 0)
	{
		int32 MaxChoices = 0;
		GetChoicesLimited_Internal(NULL, MaxChoices);
		return StartChoices + DialogueManager->GetMaxChoices() < MaxChoices;
	}

	return StartChoices > 0;
}

//=================================================================
// 
//=================================================================
bool UDialogue::GetAllActorsConst(TArray<class AActor*> &OutActors) const
{
	OutActors.Reset();

	if (IsValid(PlayerActor.Get()))
	{
		OutActors.Add(PlayerActor.Get());
	}
	if (IsValid(DialogueTarget.Get()))
	{
		OutActors.AddUnique(DialogueTarget.Get());
	}

	for (auto It = CustomSpeakers.CreateConstIterator(); It; ++It)
	{
		if (IsValid(It.Value().Get()))
		{
			OutActors.AddUnique(It.Value().Get());
		}
	}

	return OutActors.Num() > 0;
}

//=================================================================
// 
//=================================================================
bool UDialogue::GoChoicesDirection(int32 InDirection)
{
	if (!HasMoreChoices(InDirection))
		return false;

	InDirection = FMath::Clamp(InDirection, -1, 1);

	if (InDirection > 0)
	{
		int32 MaxChoices = 0;
		GetChoicesLimited_Internal(NULL, MaxChoices);

		StartChoices = FMath::Min(MaxChoices, StartChoices + DialogueManager->GetMaxChoices());
		DialogueManager->QueueDialogueUpdate();
		return true;
	}

	StartChoices = FMath::Max(0, StartChoices - DialogueManager->GetMaxChoices());
	DialogueManager->QueueDialogueUpdate();
	return true;
}

//=================================================================
// 
//=================================================================
void UDialogue::Activate()
{
	OnActivate();
}

//=================================================================
// 
//=================================================================
void UDialogue::Restore()
{
	OnRestore();
	
	DialogueManager->QueueDialogueUpdate();
}

//=================================================================
// 
//=================================================================
void UDialogue::Deactivate()
{
	class UDialogueManager *pManager = DialogueManager.Get();
	if (!IsValid(pManager) || !bIsActive)
		return;

	bool bHasLatent = HasLatentInfo();
	FLatentActionInfo LatentCopy = FinishedLatentInfo;
	FinishedLatentInfo.CallbackTarget = NULL;
	FinishedLatentInfo.ExecutionFunction = NAME_None;

	bIsActive = false;
	OnDeactivate();

	CustomSpeakers.Reset();

	ClearChoices();
	ClearDialogueBox();

	pManager->ClearDialogue();

	if (bHasLatent)
	{
		UFunction *pExecutionFunction = LatentCopy.CallbackTarget->FindFunction(LatentCopy.ExecutionFunction);
		if (pExecutionFunction)
		{
			LatentCopy.CallbackTarget->ProcessEvent(pExecutionFunction, &LatentCopy.Linkage);
		}
	}
}


//=================================================================
// 
//=================================================================
void UDialogue::Update(float DeltaTime)
{
	if (HasDuration() && !Paused)
	{
		Box_Time -= DeltaTime;
		if (Box_Time <= 0.0f)
		{
			Box_Time = 0.0f;
			Skip();
		}
	}

	if (IsActive())
		OnUpdate(DeltaTime);
}

//=================================================================
// 
//=================================================================
void UDialogue::Skip()
{
	if (!Box_IsValid || HasChoices())
		return;	

	Box_IsValid = false;

	DialogueManager->QueueDialogueUpdate();
	DialogueManager->MarkShouldUpdateSpeaker();

	DialogueManager->SetDialogueHoveredAsset(NULL);

	Box_Execute();
}

//=================================================================
// 
//=================================================================
void UDialogue::ClearDialogueBox()
{
	Box_IsValid = false;
}

//=================================================================
// 
//=================================================================
void UDialogue::Box_Execute()
{
	if (Box_IsValidFunction())
	{
		UFunction *pExecutionFunction = FindFunction(Box_ExecutionFunction);
		if (pExecutionFunction)
		{ 
			UE_LOG(LogTemp, Warning, TEXT("Executing function \"%s\" with output linkage [%d]"), *Box_ExecutionFunction.ToString(), Box_OutputLink);
			ProcessEvent(pExecutionFunction, &Box_OutputLink);
			return;
		}

		UE_LOG(LogTemp, Fatal, TEXT("WARNING! WARNING! Failed to find function %s!"), *Box_ExecutionFunction.ToString());
		Deactivate();
		return;
	}

	Deactivate();
	UE_LOG(LogTemp, Warning, TEXT("No function. Deactivating!"));
}

//=================================================================
// 
//=================================================================
FGameplayTag UDialogue::GetCustomSpeakerTag(const class AActor* const InActor) const
{
	for (auto It = CustomSpeakers.CreateConstIterator(); It; ++It)
	{
		if (It.Value().Get() == InActor)
			return It.Key();
	}

	return FGameplayTag();
}

//=================================================================
// 
//=================================================================
bool UDialogue::FindCustomSpeaker(FGameplayTag InTag, TSoftClassPtr<class AActor> InClass)
{
	if (!InTag.IsValid())
		return false;

	if (IsValid(GetCustomSpeaker(InTag)))
		return true;

	FVector Origin = IsValid(DialogueTarget.Get()) ? DialogueTarget.Get()->GetActorLocation() : PlayerActor.Get()->GetActorLocation();

	class AActor *pBest = NULL;
	float flBest = FLT_MAX;
	TArray<class AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(this, InClass.Get(), Actors);
	for (int32 i=0; i<Actors.Num(); i++)
	{
		class AActor *pActor = Actors.GetData()[i];
		if (!IsValid(pActor))
			continue;

		if (pActor == DialogueTarget.Get() || pActor == PlayerActor.Get())
			continue;

		float flDist = FVector::Dist(pActor->GetActorLocation(), Origin);
		if (flDist < flBest)
		{
			flBest = flDist;
			pBest = pActor;
		}
	}

	if (IsValid(pBest))
	{
		SetCustomSpeaker(InTag, pBest);
		return true;
	}

	if (Actors.Num() == 1)
	{
		SetCustomSpeaker(InTag, Actors.GetData()[0]);
		return true;
	}

	UE_LOG(LogTemp, Error, TEXT("Failed to find speak with tag \"%s\" from list of %d actors!"), *InTag.ToString(), Actors.Num());
	return false;
}

//=================================================================
// 
//=================================================================
void UDialogue::SetCustomSpeaker(FGameplayTag InTag, class AActor *InActor)
{
	if (InTag.IsValid())
	{
		CustomSpeakers.Emplace(InTag, InActor);
	}
}

//=================================================================
// 
//=================================================================
class AActor *UDialogue::GetCustomSpeaker(FGameplayTag InTag) const
{
	if (CustomSpeakers.Contains(InTag))
	{
		return CustomSpeakers[InTag].Get();
	}

	return NULL;
}

//=================================================================
// 
//=================================================================
void UDialogue::DialogueBox(EDialogueSpeaker Speaker, FText Text, FLatentActionInfo LatentInfo, float Duration, EDialogueExpression Expression, EDialogueEffect Effect, FGameplayTag VoiceOver, FGameplayTag InCustomTag)
{
	if (!IsActive())
		return;

	class AActor *pActor = NULL;
	switch (Speaker)
	{
	case EDialogueSpeaker::Player:
		pActor = PlayerActor.Get();
		break;
	case EDialogueSpeaker::Target:
		pActor = DialogueTarget.Get();
		break;
	case EDialogueSpeaker::Custom:
		pActor = GetCustomSpeaker(InCustomTag);
		break;
	case EDialogueSpeaker::Narrator:
		pActor = NULL;
		break;
	}

	/*
	if (!IsValid(pActor))
	{
		Deactivate();
		return;
	}
	*/

	ClearDialogueBox();

	if (Duration < -0.0f)
	{
		Duration = FMath::Max( DialogueManager->GetAdditionalTextTime() + (DialogueManager->GetTimePerLetter() * Text.ToString().Len()), DialogueManager->GetMinimumTextTime());
	}

	Box_IsValid = true;
	Box_Text = Text;
	Box_Expression = Expression;
	Box_Effect = Effect;
	Box_Actor = pActor;
	Box_Time = Box_Delay = Duration;

	DialogueManager->OnDialogue.Broadcast(pActor, Text);
	
	if (LatentInfo.Linkage == INDEX_NONE)
	{
		Box_ExecutionFunction = NAME_None;
		Box_OutputLink = INDEX_NONE;
	}
	else
	{
		Box_ExecutionFunction = LatentInfo.ExecutionFunction;
		Box_OutputLink = LatentInfo.Linkage;
	}

	//Make sure
	if (HasChoices() && Box_IsValidFunction())
	{
		UE_LOG(LogTemp, Warning, TEXT("WARNING! WARNING! Dialogue box has function defined when there's choices!"));
	}

	DialogueManager->QueueDialogueUpdate();
	DialogueManager->MarkShouldUpdateSpeaker();

	PlayVoiceOver(pActor, VoiceOver, Expression);
}

//=================================================================
// 
//=================================================================
void UDialogue::DialogueBoxNoLatent(EDialogueSpeaker Speaker, FText Text, float Duration, EDialogueExpression Expression, EDialogueEffect Effect, FGameplayTag VoiceOver, FGameplayTag InCustomTag)
{
	if (!IsActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("Not active!"));
		return;
	}

	class AActor *pActor = NULL;
	switch (Speaker)
	{
	case EDialogueSpeaker::Player:
		pActor = PlayerActor.Get();
		break;
	case EDialogueSpeaker::Target:
		pActor = DialogueTarget.Get();
		break;
	case EDialogueSpeaker::Custom:
		pActor = GetCustomSpeaker(InCustomTag);
		break;
	case EDialogueSpeaker::Narrator:
		pActor = NULL;
		break;
	}

	/*
	if (!IsValid(pActor))
	{
		Deactivate();
		return;
	}
	*/

	ClearDialogueBox();

	if (Duration < -0.0f)
	{
		Duration = FMath::Max( DialogueManager->GetAdditionalTextTime() + (DialogueManager->GetTimePerLetter() * Text.ToString().Len()), DialogueManager->GetMinimumTextTime());
	}

	Box_IsValid = true;
	Box_Text = Text;
	Box_Expression = Expression;
	Box_Effect = Effect;
	Box_Actor = pActor;
	Box_Time = Box_Delay = Duration;	
	Box_ExecutionFunction = NAME_None;
	Box_OutputLink = INDEX_NONE;

	//Make sure
	if (HasChoices() && Box_IsValidFunction())
	{
		UE_LOG(LogTemp, Warning, TEXT("WARNING! WARNING! Dialogue box has function defined when there's choices!"));
	}

	DialogueManager->QueueDialogueUpdate();
	DialogueManager->MarkShouldUpdateSpeaker();

	PlayVoiceOver(pActor, VoiceOver, Expression);
}

//=================================================================
// 
//=================================================================
void UDialogue::ClearChoices()
{
	Choices.SetNum(0);
	StartChoices = 0;
}

//=================================================================
// 
//=================================================================
void UDialogue::DialogueChoices(FText Text, FLatentActionInfo LatentInfo, bool Enabled, FName CustomChoiceName, bool DisableVisited, class UDataAsset *ChoiceAsset)
{
	if (LatentInfo.CallbackTarget != this)
		return;

	FDialogueChoice choice;
	choice.Title = Text;
	choice.Enabled = Enabled;
	choice.ExecutionFunction = LatentInfo.ExecutionFunction;
	choice.OutputLink = LatentInfo.Linkage;
	choice.OriginalIndex = Choices.Num();
	choice.ChoiceAsset = ChoiceAsset;
	if (DisableVisited)
	{
		choice.ChoiceName = NAME_None;
	}
	else
	{
		choice.ChoiceName = CustomChoiceName.IsNone() ? *FString::Printf(TEXT("%s_%d"), *LatentInfo.ExecutionFunction.ToString(), LatentInfo.Linkage) : CustomChoiceName;
	}

	Choices.Add(choice);

	const TArray<FName> &InVisitedChoices = VisitedChoices;

	//
	Choices.Sort([InVisitedChoices](const FDialogueChoice & A, const FDialogueChoice & B)
	{
		bool bVisitedA = !A.ChoiceName.IsNone() && InVisitedChoices.Contains(A.ChoiceName);
		bool bVisitedB = !B.ChoiceName.IsNone() && InVisitedChoices.Contains(B.ChoiceName);

		if (bVisitedA && !bVisitedB)
			return false;

		if (!bVisitedA && bVisitedB)
			return true;

		return A.OriginalIndex < B.OriginalIndex;
	});

	DialogueManager->QueueDialogueUpdate();
	DialogueManager->MarkShouldUpdateSpeaker();
}

//=================================================================
// 
//=================================================================
bool UDialogue::CanSelectOption(int32 Index) const
{
	if (Index < 0 || Index >= Choices.Num())
		return false;

	const FDialogueChoice &Choice = Choices[Index];
	return Choice.Enabled;
}

//=================================================================
// 
//=================================================================
bool UDialogue::HoverOption(int32 Index)
{
	if (!DialogueManager.Get())
		return false;

	if (Index < 0 || Index >= Choices.Num())
	{
		return false;
	}

	HoveredChoice = Index;

	DialogueManager->SetDialogueHoveredAsset(Choices.GetData()[Index].ChoiceAsset);

	DialogueManager->OnDialogueUpdated.Broadcast();
	return true;
}

//=================================================================
// 
//=================================================================
bool UDialogue::MoveUpDownOptions(int32 Direction)
{
	if (!DialogueManager.Get())
	{
		UE_LOG(LogTemp, Fatal, TEXT("No dialogue manager!"));
		return false;
	}

	int32 NewChoice = HoveredChoice + Direction;
	while (NewChoice >= 0 && NewChoice < Choices.Num())
	{
		if (Choices[NewChoice].Enabled)
		{
			HoveredChoice = NewChoice;
			DialogueManager->OnDialogueUpdated.Broadcast();

			UE_LOG(LogTemp, Warning, TEXT("New choice %d"), HoveredChoice);
			return true;
		}

		NewChoice = NewChoice + Direction;
	}

	UE_LOG(LogTemp, Warning, TEXT("Failed to move up and down with direction %d"), Direction);
	return false;
}

//=================================================================
// 
//=================================================================
bool UDialogue::HasVisitedChoice(int32 Index) const
{
	return Index >= 0 && Index < Choices.Num() && !Choices[Index].ChoiceName.IsNone() && VisitedChoices.Contains(Choices.GetData()[Index].ChoiceName);
}

//=================================================================
// 
//=================================================================
bool UDialogue::AreAllNextTasksVisited() const
{
	int32 iMax = FMath::Min(Choices.Num(), StartChoices + DialogueManager->GetMaxChoices() * 2);
	if (iMax <= StartChoices + DialogueManager->GetMaxChoices())
		return false;

	for (int32 i=StartChoices + DialogueManager->GetMaxChoices(); i<iMax; i++)
	{
		if (!HasVisitedChoice(i))
			return false;
	}

	return true;
}

//=================================================================
// 
//=================================================================
void UDialogue::GetEveryoneInvolved(TArray<class AActor*>& OutActors)
{
	OutActors.Reset();

	class AActor *pPlayer = PlayerActor.Get();
	if (IsValid(pPlayer))
	{
		OutActors.Add(pPlayer);
	}

	class AActor *pTarget = DialogueTarget.Get();
	if (IsValid(pTarget))
	{
		OutActors.AddUnique(pTarget);
	}

	for (auto It = CustomSpeakers.CreateConstIterator(); It; ++It)
	{
		class AActor *pCustom = It.Value().Get();
		if (IsValid(pCustom))
		{
			OutActors.AddUnique(pCustom);
		}
	}
}

//=================================================================
// 
//=================================================================
bool UDialogue::SelectOption(int32 Index)
{
	if (!DialogueManager.Get())
	{
		UE_LOG(LogTemp, Fatal, TEXT("No dialogue manager!"));
		return false;
	}

	if (Index < 0 || Index >= Choices.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid choice"));

		Skip();
		return false;
	}

	if (!Choices.GetData()[Index].Enabled)
	{
		UE_LOG(LogTemp, Warning, TEXT("Choice not enabled!"));

		return false;
	}

	if (Choices[Index].IsValidFunction())
	{
		UFunction *pExecutionFunction = FindFunction(Choices[Index].ExecutionFunction);
		if (pExecutionFunction)
		{
			LastClickedAsset = Choices.GetData()[Index].ChoiceAsset;
			LastClickedOption = Choices.GetData()[Index].ChoiceName;

			DialogueManager->SetDialogueHoveredAsset(NULL);

			if (!Choices.GetData()[Index].ChoiceName.IsNone())
			{
				VisitedChoices.AddUnique(Choices.GetData()[Index].ChoiceName);
			}

			int32 OutputLink = Choices.GetData()[Index].OutputLink;

			ClearChoices();
			ClearDialogueBox();
			DialogueManager->QueueDialogueUpdate();

			ProcessEvent(pExecutionFunction, &OutputLink);
			return true;
		}
	}

	Deactivate();
	return true;
}

#if WITH_EDITOR

//=================================================================
// 
//=================================================================
void UDialogue::PreSave(FObjectPreSaveContext ObjectSaveContext)
{
	Super::PreSave(ObjectSaveContext);

	if (ObjectSaveContext.GetTargetPlatform() == NULL)
	{
		CheckUsingStringTable();
	}
}

//=================================================================
// 
//=================================================================
bool FixTextToUseStringTable(class UStringTable *StringTable, FString Key, FText &Text, bool DontAdd)
{
	if (Text.IsEmpty() || Text.IsFromStringTable())
		return false;

	/** Get the underlying string table owned by this asset */
	FStringTable &MutableStringTable = StringTable->GetMutableStringTable().Get();

	FString SourceString = Text.ToString();
	bool bFound = false;

	StringTable->GetStringTable()->EnumerateSourceStrings([&](const FString& InKey, const FString& InSourceString)
	{
		if (SourceString.Equals(InSourceString))
		{
			bFound = true;
			Key = InKey;
			return false;
		}

		return true;
	});

	if (!bFound)
	{
		int32 iKeyIndex = 0;
		while (true)
		{
			FString NewKey = FString::Printf(TEXT("%s_%d"), *Key, iKeyIndex);
			bool bFoundKey = false;

			StringTable->GetStringTable()->EnumerateSourceStrings([&](const FString& InKey, const FString& InSourceString)
			{
				if (InKey == NewKey)
				{
					bFoundKey = true;
					return false;
				}
					
				return true;
			});

			if (bFoundKey)
			{
				iKeyIndex++;
			}
			else
			{
				Key = NewKey;
				break;
			}
		}
	}

	//Add text to string table
	if (!bFound && !DontAdd)
	{
		MutableStringTable.SetSourceString(Key, SourceString);
		StringTable->Modify();
	}

	if (bFound || !DontAdd)
	{
		Text = FText::FromStringTable(StringTable->GetStringTableId(), Key);
		return true;
	}

	return false;
}

//=================================================================
// 
//=================================================================
bool UDialogue::FixTextToUseStringTable(FText& InText, const FString &InKey, class UStringTable* InStringTable, bool InDontAdd)
{
	return ::FixTextToUseStringTable(InStringTable, InKey, InText, InDontAdd);
}

//=================================================================
// 
//=================================================================
bool UDialogue::ChangeTextInStringTable(class UStringTable* InStringTable, const FText& InOldText, FText& InNewText)
{
	if (InNewText.IsEmpty())
		return false;

	if (!InOldText.IsFromStringTable())
		return false;

	if (!IsValid(InStringTable))
		return false;

	FStringTable& MutableStringTable = InStringTable->GetMutableStringTable().Get();

	FName TableId;
	FString Key;
	FTextInspector::GetTableIdAndKey(InOldText, TableId, Key);
	if (InStringTable->GetStringTableId() != TableId)
		return false;


	MutableStringTable.SetSourceString(Key, InNewText.ToString());
	InStringTable->Modify();

	InNewText = FText::FromStringTable(InStringTable->GetStringTableId(), Key);
	return true;
}

//=================================================================
// 
//=================================================================
void UseStringTableInProperty(class UObject *Dialogue, const FString &KeyName, FTextProperty *InProperty, void *ValuePtr, bool InGetValuePtr, class UStringTable *StringTable, bool DontAdd)
{
	FText InText = InGetValuePtr ? InProperty->GetPropertyValue(ValuePtr) : InProperty->GetPropertyValue_InContainer(ValuePtr, 0);
	if (FixTextToUseStringTable(StringTable, KeyName, InText, DontAdd))
	{
		if (InGetValuePtr)
		{
			InProperty->SetPropertyValue(ValuePtr, InText);
		}
		else
		{
			InProperty->SetPropertyValue_InContainer(ValuePtr, InText, 0);
		}
	}
}

//=================================================================
// 
//=================================================================
FString GetKeyName(const FString &KeyName, FProperty *Property, int32 Index)
{
	if (KeyName.Len() > 0)
	{
		if (Index > 0)
			return FString::Printf(TEXT("%s_%s_%d"), *KeyName, *Property->GetAuthoredName(), Index);

		return FString::Printf(TEXT("%s_%s"), *KeyName, *Property->GetAuthoredName());
	}

	if (Index > 0)
		return FString::Printf(TEXT("%s_%d"), *Property->GetAuthoredName(), Index);

	return Property->GetAuthoredName();
}

//=================================================================
// 
//=================================================================
void UseStringTableInProperties(class UObject *Dialogue, FString KeyName, class UStringTable *StringTable, const UStruct *Struct, void *Address, bool DontAdd)
{
	//Go through regular text properties
	for (TFieldIterator<FTextProperty> Property(Struct); Property; ++Property)
	{
		FString NewKey = GetKeyName(KeyName, *Property, -1);

		UseStringTableInProperty(Dialogue, NewKey, *Property, Address, false, StringTable, DontAdd);
	}

	//Go through struct properties that might have text properties nested in them
	for (TFieldIterator<FStructProperty> Property(Struct); Property; ++Property)
	{
		void *StructAddress = Property->ContainerPtrToValuePtr<void>(Address, 0);

		FString NewKey = GetKeyName(KeyName, *Property, -1);

		UseStringTableInProperties(Dialogue, NewKey, StringTable, Property->Struct, StructAddress, DontAdd);
	}

	//Go through array properties that might have text properties nested in them
	for (TFieldIterator<FMapProperty> Property(Struct); Property; ++Property)
	{
		FScriptMapHelper_InContainer MapHelper(*Property, Address, 0); 

		FTextProperty *TextProperty = CastField<FTextProperty>(MapHelper.GetValueProperty());
		FStructProperty *StructProperty = CastField<FStructProperty>(MapHelper.GetValueProperty());

		if (TextProperty || StructProperty)
		{

			for (int32 SparseElementIndex = 0; SparseElementIndex < MapHelper.GetMaxIndex(); ++SparseElementIndex)
			{
				if (MapHelper.IsValidIndex(SparseElementIndex))
				{
					if (StructProperty)
					{	
						FString NewKey = GetKeyName( KeyName, *Property, SparseElementIndex);

						UseStringTableInProperties(Dialogue, NewKey, StringTable, StructProperty->Struct, MapHelper.GetValuePtr(SparseElementIndex), DontAdd);
					}
					
					if (TextProperty)
					{	
						FString NewKey = GetKeyName(KeyName, *Property, SparseElementIndex);

						UseStringTableInProperty(Dialogue, NewKey, TextProperty, MapHelper.GetValuePtr(SparseElementIndex), true, StringTable, DontAdd);
					}
				}
			}
		}
	}

	//Go through array properties that might have text properties nested in them
	for (TFieldIterator<FArrayProperty> Property(Struct); Property; ++Property)
	{
		FProperty *InnerProperty = Property->Inner;

		//If array of structs
		FStructProperty *StructProperty = CastField<FStructProperty>(InnerProperty);
		if (StructProperty)
		{
			FScriptArrayHelper_InContainer ArrayHelper(*Property, Address);
			for (int32 i = 0; i < ArrayHelper.Num(); i++)
			{
				FString NewKey = GetKeyName(KeyName, *Property, i);

				UseStringTableInProperties(Dialogue, NewKey, StringTable, StructProperty->Struct, ArrayHelper.GetRawPtr(i), DontAdd);
			}

			continue;
		}

		//If array of texts
		FTextProperty *TextProperty = CastField<FTextProperty>(InnerProperty);
		if (TextProperty)
		{
			FScriptArrayHelper_InContainer ArrayHelper(*Property, Address);
			for (int32 i = 0; i < ArrayHelper.Num(); i++)
			{
				FString NewKey = GetKeyName(KeyName, *Property, i);

				UseStringTableInProperty(Dialogue, NewKey, TextProperty, ArrayHelper.GetRawPtr(i), true, StringTable, DontAdd);
			}

			continue;
		}
	}
}

//=================================================================
// 
//=================================================================
void ClearStringTableUseInProperty(class UObject* Dialogue, FTextProperty* InProperty, void* ValuePtr, bool InGetValuePtr, class UStringTable* StringTable)
{
	FText InText = InGetValuePtr ? InProperty->GetPropertyValue(ValuePtr) : InProperty->GetPropertyValue_InContainer(ValuePtr, 0);
	
	if (!InText.IsFromStringTable())
		return;

	FName TableId;
	FString Key;
	FTextInspector::GetTableIdAndKey(InText, TableId, Key);
	if (StringTable->GetStringTableId() != TableId)
		return;

	if (InGetValuePtr)
	{
		InProperty->SetPropertyValue(ValuePtr, FText::FromString(InText.ToString()));
	}
	else
	{
		InProperty->SetPropertyValue_InContainer(ValuePtr, FText::FromString(InText.ToString()), 0);
	}
}

//=================================================================
// 
//=================================================================
void ClearStringTableUseInProperties(class UObject* Dialogue, class UStringTable* StringTable, const UStruct* Struct, void* Address)
{
	//Go through regular text properties
	for (TFieldIterator<FTextProperty> Property(Struct); Property; ++Property)
	{
		ClearStringTableUseInProperty(Dialogue, *Property, Address, false, StringTable);
	}

	//Go through struct properties that might have text properties nested in them
	for (TFieldIterator<FStructProperty> Property(Struct); Property; ++Property)
	{
		void* StructAddress = Property->ContainerPtrToValuePtr<void>(Address, 0);

		ClearStringTableUseInProperties(Dialogue, StringTable, Property->Struct, StructAddress);
	}

	//Go through array properties that might have text properties nested in them
	for (TFieldIterator<FMapProperty> Property(Struct); Property; ++Property)
	{
		FScriptMapHelper_InContainer MapHelper(*Property, Address, 0);

		FTextProperty* TextProperty = CastField<FTextProperty>(MapHelper.GetValueProperty());
		FStructProperty* StructProperty = CastField<FStructProperty>(MapHelper.GetValueProperty());

		if (TextProperty || StructProperty)
		{

			for (int32 SparseElementIndex = 0; SparseElementIndex < MapHelper.GetMaxIndex(); ++SparseElementIndex)
			{
				if (MapHelper.IsValidIndex(SparseElementIndex))
				{
					if (StructProperty)
					{
						ClearStringTableUseInProperties(Dialogue, StringTable, StructProperty->Struct, MapHelper.GetValuePtr(SparseElementIndex));
					}

					if (TextProperty)
					{
						ClearStringTableUseInProperty(Dialogue, TextProperty, MapHelper.GetValuePtr(SparseElementIndex), true, StringTable);
					}
				}
			}
		}
	}

	//Go through array properties that might have text properties nested in them
	for (TFieldIterator<FArrayProperty> Property(Struct); Property; ++Property)
	{
		FProperty* InnerProperty = Property->Inner;

		//If array of structs
		FStructProperty* StructProperty = CastField<FStructProperty>(InnerProperty);
		if (StructProperty)
		{
			FScriptArrayHelper_InContainer ArrayHelper(*Property, Address);
			for (int32 i = 0; i < ArrayHelper.Num(); i++)
			{
				ClearStringTableUseInProperties(Dialogue, StringTable, StructProperty->Struct, ArrayHelper.GetRawPtr(i));
			}

			continue;
		}

		//If array of texts
		FTextProperty* TextProperty = CastField<FTextProperty>(InnerProperty);
		if (TextProperty)
		{
			FScriptArrayHelper_InContainer ArrayHelper(*Property, Address);
			for (int32 i = 0; i < ArrayHelper.Num(); i++)
			{
				ClearStringTableUseInProperty(Dialogue, TextProperty, ArrayHelper.GetRawPtr(i), true, StringTable);
			}

			continue;
		}
	}
}



//=================================================================
// 
//=================================================================
FORCEINLINE bool DoNodesOverlap(class UEdGraphNode *Node1, class UEdGraphNode *Node2)
{
	return	Node1->NodePosX + Node1->NodeWidth >= Node2->NodePosX &&
			Node2->NodePosX + Node2->NodeWidth >= Node1->NodePosX &&
			Node1->NodePosY + Node1->NodeHeight >= Node2->NodePosY &&
			Node2->NodePosY + Node2->NodeHeight >= Node1->NodePosY;
}

//=================================================================
// 
//=================================================================
FORCEINLINE bool IsValidCommentCharacter(const wchar_t InCharacter)
{
	return (InCharacter >= L'a' && InCharacter <= L'z') || (InCharacter >= L'A' && InCharacter <= L'Z') || (InCharacter >= L'0' && InCharacter <= L'9') || InCharacter == L'_';
}

//=================================================================
// 
//=================================================================
void UseStringTableInGraph(class UObject *Dialogue, class UStringTable *StringTable, const TArray<class UEdGraph *> &Graphs, bool DontAdd)
{
	//Go through different graphs
	for (int32 i=0; i<Graphs.Num(); i++)
	{
		//
		class UEdGraph *pGraph = Graphs.GetData()[i];
		if (!pGraph)
			continue;

		//Find all comments
		TArray<class UEdGraphNode_Comment*> Comments;
		for (int32 j = 0; j < pGraph->Nodes.Num(); j++)
		{
			//
			class UEdGraphNode* pNode = pGraph->Nodes.GetData()[j];
			if (!pNode)
				continue;

			//
			class UEdGraphNode_Comment* pComment = Cast<UEdGraphNode_Comment>(pNode);
			if (IsValid(pComment))
			{
				Comments.Add(pComment);
			}
		}

		//Go through graph nodes
		for (int32 j=0; j<pGraph->Nodes.Num(); j++)
		{
			//
			class UEdGraphNode *pNode = pGraph->Nodes.GetData()[j];
			if (!pNode)
				continue;

			FString Comment;
			FString Key = Dialogue->GetClass()->GetName();
			Key.RemoveFromEnd("_C");
			Key.RemoveFromStart("BP_");

			int32 iBest = INT_MAX;

			const int32 iMaxCommentLength = 24;

			for (int32 k=0; k<Comments.Num(); k++)
			{
				class UEdGraphNode_Comment *pComment = Comments.GetData()[k];
				if (!DoNodesOverlap(pNode, pComment))
					continue;

				if (pComment->NodeComment.Len() > iMaxCommentLength)
					continue;

				//Use the size of the comment node to determine what is the comment we want
				int32 iArea = pComment->NodeWidth * pComment->NodeHeight;

				//Smallest box wins
				if (iArea >= iBest)
				{
					continue;
				}

				iBest = iArea;
				Comment = pComment->NodeComment;
			}

			//Remove disallowed characters from comment
			for (int32 k=Comment.Len()-1; k>=0; k--)
			{
				if (Comment[k] == L' ')
				{
					Comment[k] = L'_';
				}
				else if (Comment[k] == L'&')
				{
					Comment.RemoveAt(k);
					Comment.InsertAt(k, L'a');
					Comment.InsertAt(k+1, L'n');
					Comment.InsertAt(k+2, L'd');
				}
				else if (!IsValidCommentCharacter(Comment[k]))
				{
					Comment.RemoveAt(k);
				}
			}

			if (Comment.Len() > 0)
			{
				//UE_LOG(LogTemp, Error, TEXT("Comment \"%s\""), *Comment);

				Key = FString::Printf(TEXT("%s_%s"), *Key, *Comment);
			}

			//Go through pins in node
			for (int32 k=0; k<pNode->Pins.Num(); k++)
			{
				//
				class UEdGraphPin *pPin = pNode->Pins.GetData()[k];
				if (!pPin)
					continue;

				//
				if (pPin->LinkedTo.Num() > 0)
					continue;

				if (pPin->Direction != EEdGraphPinDirection::EGPD_Input)
					continue;

				//Make sure correct type
				static const FName Name_Text = TEXT("text");
				if (pPin->PinType.PinCategory != Name_Text)
				{
					continue;
				}

				FixTextToUseStringTable(StringTable, Key, pPin->DefaultTextValue, DontAdd);
			}
		}
	}
}


//=================================================================
// 
//=================================================================
void ClearStringTableUseInGraph(class UObject* Dialogue, class UStringTable* StringTable, const TArray<class UEdGraph*>& Graphs)
{
	//Go through different graphs
	for (int32 i = 0; i < Graphs.Num(); i++)
	{
		//
		class UEdGraph* pGraph = Graphs.GetData()[i];
		if (!pGraph)
			continue;

		//Go through graph nodes
		for (int32 j = 0; j < pGraph->Nodes.Num(); j++)
		{
			//
			class UEdGraphNode* pNode = pGraph->Nodes.GetData()[j];
			if (!pNode)
				continue;
			//Go through pins in node
			for (int32 k = 0; k < pNode->Pins.Num(); k++)
			{
				//
				class UEdGraphPin* pPin = pNode->Pins.GetData()[k];
				if (!pPin)
					continue;

				//
				if (pPin->LinkedTo.Num() > 0)
					continue;

				if (pPin->Direction != EEdGraphPinDirection::EGPD_Input)
					continue;

				//Make sure correct type
				static const FName Name_Text = TEXT("text");
				if (pPin->PinType.PinCategory != Name_Text)
				{
					continue;
				}

				if (!pPin->DefaultTextValue.IsFromStringTable())
					continue;

				FName TableId;
				FString Key;
				FTextInspector::GetTableIdAndKey(pPin->DefaultTextValue, TableId, Key);
				if (StringTable->GetStringTableId() != TableId)
					continue;

				pPin->DefaultTextValue = FText::FromString(pPin->DefaultTextValue.ToString());
			}
		}
	}
}

//=================================================================
// 
//=================================================================
void UDialogue::UseStringTable(const TSoftObjectPtr<class UStringTable> &InStringTable, class UObject *InObject, bool InDontAdd)
{
	class UStringTable *pDefaultStringTable = InStringTable.IsPending() ? InStringTable.LoadSynchronous() : InStringTable.Get();
	if (pDefaultStringTable)
	{
		//If object, then check if blueprint object
		class UBlueprint *pBlueprint = Cast<UBlueprint>(InObject->GetClass()->ClassGeneratedBy);
		if (pBlueprint)
		{
			UseStringTableInGraph(InObject, pDefaultStringTable, pBlueprint->UbergraphPages, InDontAdd);
			UseStringTableInGraph(InObject, pDefaultStringTable, pBlueprint->FunctionGraphs, InDontAdd);
			UseStringTableInGraph(InObject, pDefaultStringTable, pBlueprint->MacroGraphs, InDontAdd);
		}

		UseStringTableInProperties(InObject, TEXT(""), pDefaultStringTable, InObject->GetClass(), InObject, InDontAdd);

		class UDataTable *pDataTable = Cast<UDataTable>(InObject);
		if (!IsValid(pDataTable))
			return;

		const TMap<FName, uint8*>&RowMap = pDataTable->GetRowMap();
		for (auto It = RowMap.CreateConstIterator(); It; ++It)
		{
			UseStringTableInProperties(InObject, It.Key().ToString(), pDefaultStringTable, pDataTable->GetRowStruct(), It.Value(), InDontAdd);
		}
	}
}

//=================================================================
// 
//=================================================================
void UDialogue::ClearStringTableUse(const TSoftObjectPtr<class UStringTable>& InStringTable, class UObject* InObject)
{
	class UStringTable* pDefaultStringTable = InStringTable.IsPending() ? InStringTable.LoadSynchronous() : InStringTable.Get();
	if (pDefaultStringTable)
	{
		//If object, then check if blueprint object
		class UBlueprint* pBlueprint = Cast<UBlueprint>(InObject->GetClass()->ClassGeneratedBy);
		if (pBlueprint)
		{
			ClearStringTableUseInGraph(InObject, pDefaultStringTable, pBlueprint->UbergraphPages);
			ClearStringTableUseInGraph(InObject, pDefaultStringTable, pBlueprint->FunctionGraphs);
			ClearStringTableUseInGraph(InObject, pDefaultStringTable, pBlueprint->MacroGraphs);
		}

		ClearStringTableUseInProperties(InObject, pDefaultStringTable, InObject->GetClass(), InObject);
	}
}

//=================================================================
// 
//=================================================================
void UDialogue::CheckUsingStringTable()
{
	UseStringTable(DefaultStringTable, this, true);
	UseStringTable(StringTable, this, false);
}

//=================================================================
// 
//=================================================================
void UDialogue::UseStringTables(class UObject* InObject, TSoftObjectPtr<class UStringTable> InGeneralStringTable, TSoftObjectPtr<class UStringTable> InObjectSpecificStringTable)
{
	if (!IsValid(InObject))
		return;

	UseStringTable(InGeneralStringTable, InObject, true);
	UseStringTable(InObjectSpecificStringTable, InObject, false);

	InObject->Modify();
}

#endif //

//=================================================================
// 
//=================================================================
bool UDialogue::AddContext(FGameplayTag InTag, FGameplayTag InActorTag, int32 InValue)
{
	return DialogueManager->AddContext(InTag, InActorTag, InValue);
}

//=================================================================
// 
//=================================================================
bool UDialogue::RemoveContext(FGameplayTag InTag, FGameplayTag InActorTag)
{
	return DialogueManager->RemoveContext(InTag, InActorTag);
}

//=================================================================
// 
//=================================================================
bool UDialogue::HasContext(FGameplayTag InTag, FGameplayTag InActorTag) const
{
	return DialogueManager->HasContext(InTag, InActorTag);
}

//=================================================================
// 
//=================================================================
int32 UDialogue::GetContext(FGameplayTag InTag, FGameplayTag InActorTag) const
{
	return DialogueManager->GetContext(InTag, InActorTag);
}

//=================================================================
// 
//=================================================================
int32 UDialogue::MakeRandomRoll(FGameplayTag InTag, int32 InMin, int32 InMax)
{
	return DialogueManager->MakeRandomRoll(InTag, GetTargetActorTag(), InMin, InMax);
}

//=================================================================
// 
//=================================================================
bool UDialogue::IncrementContext(FGameplayTag InTag, FGameplayTag InActorTag, int32 InValue, bool AddIfMissing)
{
	return DialogueManager->IncrementContext(InTag, InActorTag, InValue, AddIfMissing);
}

//=================================================================
// 
//=================================================================
bool UDialogue::RemoveAllContextFor(FGameplayTag InActorTag)
{
	return DialogueManager->RemoveAllContextFor(InActorTag);
}

#if WITH_EDITOR

//===================================================================================================
// 
//===================================================================================================
FString GetPinId(int32 PinNum)
{
	FString Number = FString::Printf(TEXT("%d"), PinNum);

	FString Final = TEXT("");
	for (int32 i=Number.Len(); i<32; i++)
	{
		Final += TEXT("0");
	}

	return Final + Number;
}

//===================================================================================================
// 
//===================================================================================================
void UDialogue::ParseLine(const FString &InString, FString &OutStrippedText, FString &OutSpeakerName, FString &OutCustomName, FString &OutExpressionName, FString &InPreviousSpeaker)
{
	if (!InString.Split(TEXT(": "), &OutSpeakerName, &OutStrippedText))
	{
		OutStrippedText = InString;
	}

	if (OutSpeakerName.Len() == 0 && InPreviousSpeaker.Len() > 0)
	{
		OutSpeakerName = InPreviousSpeaker;
	}

	if (OutSpeakerName.Len() > 0 && OutSpeakerName != TEXT("Player") && OutSpeakerName != TEXT("Narrator") && OutSpeakerName != "Target")
	{
		OutCustomName = OutSpeakerName;
		OutSpeakerName = TEXT("Custom");
	}

	if (OutSpeakerName.Len() == 0)
	{
		OutSpeakerName = TEXT("Target");
	}

	if (OutCustomName.Len() > 0)
	{
		InPreviousSpeaker = OutCustomName;

		//OutCustomName = FString::Printf(TEXT("(TagName=\\\"Character.Name.%s\\\")"), *CustomName); //(TagName=\"Character.Abigail\")

		UE_LOG(LogTemp, Error, TEXT("Custom name value: %s"), *OutCustomName);
	}
	else
	{
		InPreviousSpeaker = OutSpeakerName;
	}

	if (OutStrippedText.Len() > 0 && OutStrippedText[0] == L'"' && OutStrippedText[OutStrippedText.Len() - 1] == L'"')
	{
		OutStrippedText.RemoveFromStart(L"\"");
		OutStrippedText.RemoveFromEnd(L"\"");
	}

	//Remove empty spaces from end
	while (OutStrippedText.Len() > 0 && OutStrippedText[OutStrippedText.Len() - 1] == L' ')
	{
		OutStrippedText.RemoveAt(OutStrippedText.Len() - 1);
	}

	//--------------------------------------------------------------------------------------
	// Figure out speaker
	//--------------------------------------------------------------------------------------

	//If last character is ")" then we might have an expression
	if (OutStrippedText.Len() > 0 && OutStrippedText[OutStrippedText.Len() - 1] == L')')
	{
		FString NewText = OutStrippedText;
		NewText.RemoveAt(NewText.Len() - 1);
		NewText.Split(TEXT("("), &NewText, &OutExpressionName);

		//Remove empty spaces from end
		while (NewText.Len() > 0 && NewText[NewText.Len() - 1] == L' ')
		{
			NewText.RemoveAt(NewText.Len() - 1);
		}

		if (NewText.Len() > 0 && NewText[0] == L'"' && NewText[NewText.Len() - 1] == L'"')
		{
			NewText.RemoveFromStart(L"\"");
			NewText.RemoveFromEnd(L"\"");
		}

		if (NewText.Len() > 0)
		{
			OutStrippedText = NewText;
		}
	}
}

//===================================================================================================
// 
//===================================================================================================
void UDialogue::GenerateNode(const FString &InString, float InDuration, int32 Index, bool HasNext, int32 &PinNum, FString &OutString, FString &PreviousSpeaker)
{
	//
	static const int XOffset = 800;
	static const int YOffset = 420;

	OutString += FString::Printf(TEXT("Begin Object Class=/Script/BlueprintGraph.K2Node_CallFunction Name=\"K2Node_CallFunction_%d\"\r\n"), Index);
	OutString += TEXT("   FunctionReference=(MemberName=\"DialogueBox\",bSelfContext=True)\r\n");

	int32 iNumberOfPins = 6;

	if (GenerateNodesVerticallyAndNotConnected)
	{
		OutString += TEXT("   NodePosX=0\r\n");
		OutString += FString::Printf(TEXT("   NodePosY=%d\r\n"), Index * YOffset);
	}
	else
	{
		OutString += FString::Printf(TEXT("   NodePosX=%d\r\n"), Index * XOffset);
		OutString += TEXT("   NodePosY=0\r\n");
	}

	if (Index > 1 && !GenerateNodesVerticallyAndNotConnected)
	{
		OutString += FString::Printf(TEXT("   CustomProperties Pin (PinId=%s,PinName=\"execute\",PinType.PinCategory=\"exec\",PinType.PinSubCategory=\"\",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CallFunction_%d %s,),bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)\r\n"), *GetPinId(PinNum), Index-1, *GetPinId(PinNum- iNumberOfPins));
		PinNum++;
	}

	if (HasNext && !GenerateNodesVerticallyAndNotConnected)
	{
		OutString += FString::Printf(TEXT("   CustomProperties Pin (PinId=%s,PinName=\"then\",PinFriendlyName=\"Completed\",Direction=\"EGPD_Output\",PinType.PinCategory=\"exec\",PinType.PinSubCategory=\"\",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,LinkedTo=(K2Node_CallFunction_%d %s,),bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)\r\n"), *GetPinId(PinNum), Index+1, *GetPinId(PinNum+ iNumberOfPins));
		PinNum++;
	}

	//--------------------------------------------------------------------------------------
	// Figure out speaker
	//--------------------------------------------------------------------------------------

	FString StrippedText;
	FString ExpressionName;
	FString SpeakerName;
	FString CustomName;
	ParseLine(InString, StrippedText, SpeakerName, CustomName, ExpressionName, PreviousSpeaker);
	CustomName = FString::Printf(TEXT("(TagName=\\\"Character.Name.%s\\\")"), *CustomName); //(TagName=\"Character.Abigail\")

	UE_LOG(LogTemp, Error, TEXT("Expression Name: %s"), *ExpressionName);

	//Text
	OutString += FString::Printf(TEXT("   CustomProperties Pin (PinId=%s,PinName=\"Text\",PinType.PinCategory=\"text\",PinType.PinSubCategory=\"\",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultTextValue=\"%s\"),bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)\r\n"), *GetPinId(PinNum), *StrippedText);
	PinNum++;

	//Duration
	OutString += FString::Printf(TEXT("   CustomProperties Pin (PinId=%s,PinName=\"Duration\",PinType.PinCategory=\"real\",PinType.PinSubCategory=\"float\",PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultValue=\"%f\"),bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)\r\n"), *GetPinId(PinNum), InDuration);
	PinNum++;

	//Expression:
	OutString += FString::Printf(TEXT("   CustomProperties Pin (PinId=%s,PinName=\"Expression\",PinType.PinCategory=\"byte\",PinType.PinSubCategory=\"\",PinType.PinSubCategoryObject=/Script/CoreUObject.Enum'\"/Script/SimpleDialogue.EDialogueExpression\"',PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultValue=\"%s\",bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)\r\n"), *GetPinId(PinNum), *ExpressionName);
	PinNum++;

	//Speaker:
	OutString += FString::Printf(TEXT("   CustomProperties Pin (PinId=%s,PinName=\"Speaker\",PinType.PinCategory=\"byte\",PinType.PinSubCategory=\"\",PinType.PinSubCategoryObject=/Script/CoreUObject.Enum'\"/Script/SimpleDialogue.EDialogueSpeaker\"',PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultValue=\"%s\",bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)\r\n"), *GetPinId(PinNum), *SpeakerName);
	PinNum++;

	//Custom:
	OutString += FString::Printf(TEXT("   CustomProperties Pin (PinId=%s,PinName=\"InCustomName\",PinType.PinCategory=\"struct\",PinType.PinSubCategory=\"\",PinType.PinSubCategoryObject =/Script/CoreUObject.ScriptStruct'\"/Script/GameplayTags.GameplayTag\"',PinType.PinValueType=(),PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,DefaultValue=\"%s\",bHidden=False,bNotConnectable=False,bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)\r\n"), *GetPinId(PinNum), *CustomName);
	PinNum++;

	OutString += TEXT("End Object\r\n");
}

//===================================================================================================
// 
//===================================================================================================
bool FindPinPropertyValue(const FString &String, const FString &PinName, FString &Value, bool IsGameplayTag = false)
{
	int32 iIndex = String.Find(PinName, ESearchCase::CaseSensitive);
	if (iIndex == INDEX_NONE)
		return false;

	iIndex = iIndex + PinName.Len() + 1;

	FString MidText = String.Mid(iIndex);
	
	if (MidText.StartsWith(TEXT("LOCTABLE")) || MidText.StartsWith(TEXT("NSLOCTEXT")))
	{
		//UE_LOG(LogTemp, Error, TEXT("Localization table!"));

		iIndex = 0;
		while (iIndex < MidText.Len())
		{
			Value.AppendChar(MidText[iIndex]);

			if (MidText[iIndex] == ')')
			{
				return Value.Len() > 0;
			}

			iIndex++;
		}

		return Value.Len() > 0;
	}

	int32 iCount = 0;
	int32 iStart = IsGameplayTag ? 3 : 1;
	int32 iEnd = iStart+1; // IsGameplayTag ? 4 : 2;
	while (iIndex < String.Len())
	{
		if (String[iIndex] == L'\"' || (IsGameplayTag && String[iIndex] == L'\\'))
		{
			iCount++;

			if (iCount < iEnd)
			{
				iIndex++;
				continue;
			}

			return Value.Len() > 0;
		}
		else if (iCount < iStart)
		{
			iIndex++;
			continue;
		}

		Value.AppendChar(String[iIndex]);
		iIndex++;
	}

	return false;
}

//===================================================================================================
// 
//===================================================================================================
bool UDialogue::ParseNodeToString(const FString& NodeString, FString& OutLine)
{
	static const FString String_Text = TEXT("Text");
	static const FString String_Speaker = TEXT("Speaker");
	static const FString String_CustomName = TEXT("InCustomName");
	static const FString String_Expression = TEXT("Expression");

	TArray<FString> Array;
	NodeString.ParseIntoArray(Array, TEXT("\n"), true);

	FString Text;
	FString Expression;
	FString Speaker;
	FString CustomSpeaker;

	for (int32 i=0; i<Array.Num(); i++)
	{
		const FString &String = Array.GetData()[i];

		FString PinName;
		FindPinPropertyValue(String, TEXT("PinName"), PinName);

		bool bIsGameplayTag = PinName.Equals(String_CustomName, ESearchCase::CaseSensitive);

		FString Value;
		if (!FindPinPropertyValue(String, TEXT("DefaultValue"), Value, bIsGameplayTag))
		{
			FindPinPropertyValue(String, TEXT("DefaultTextValue"), Value, bIsGameplayTag);
		}

		//UE_LOG(LogTemp, Error, TEXT("PinName %s value %s"), *PinName, *Value);

		if (PinName.Equals(String_Text, ESearchCase::CaseSensitive))
		{
			Text = Value;
		}
		else if (PinName.Equals(String_Speaker, ESearchCase::CaseSensitive))
		{
			Speaker = Value;
		}
		else if (PinName.Equals(String_CustomName, ESearchCase::CaseSensitive))
		{
			CustomSpeaker = Value;
		}
		else if (PinName.Equals(String_Expression, ESearchCase::CaseSensitive))
		{
			Expression = Value;
		}
	}

	if (Text.Len() == 0)
		return false;

	int32 PortFlags = 0;

	FText ImportedText;
	const FTextProperty* TextPropCDO = GetDefault<FTextProperty>();
	//TextPropCDO->ExportTextItem_Direct(Text, &ImportedText, nullptr, nullptr, PortFlags, nullptr);
	TextPropCDO->ImportText_Direct(*Text, &ImportedText, nullptr, PortFlags);

	/*
	UE_LOG(LogTemp, Error, TEXT("Text is \"%s\" and \"%s\" with type: %s"), *Text, *ImportedText.ToString(), ImportedText.IsFromStringTable() ? TEXT("From string table") : (ImportedText.IsCultureInvariant() ? TEXT("Culture invariant") : TEXT("OTHER")));
	UE_LOG(LogTemp, Error, TEXT("Expression is \"%s\""), *Expression);
	UE_LOG(LogTemp, Error, TEXT("Speaker is \"%s\""), *Speaker);
	UE_LOG(LogTemp, Error, TEXT("Custom Name is \"%s\""), *CustomSpeaker);
	*/

	if (Speaker == TEXT("Custom"))
	{
		FString SimpleName;
		if (CustomSpeaker.Split(TEXT("."), NULL, &SimpleName, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
		{
			Speaker = SimpleName;
		}
		else
		{
			Speaker = CustomSpeaker;
		}
	}

	OutLine = FString::Printf(TEXT("%s: %s"), *Speaker, *ImportedText.ToString());

	if (Expression.Len() > 0 && !Expression.Equals(TEXT("None"), ESearchCase::IgnoreCase))
	{
		OutLine += TEXT(" (") + Expression + TEXT(")");
	}

	return true;
}

//===================================================================================================9
// 
//===================================================================================================
void GatherTextPinsFromGraph(const TArray<class UEdGraph *> &Graphs, TArray<UEdGraphPin*> &OutPins)
{
	//Go through different graphs
	for (int32 i=0; i<Graphs.Num(); i++)
	{
		//
		class UEdGraph *pGraph = Graphs.GetData()[i];
		if (!pGraph)
			continue;

		//Go through graph nodes
		for (int32 j=0; j<pGraph->Nodes.Num(); j++)
		{
			//
			class UEdGraphNode *pNode = pGraph->Nodes.GetData()[j];
			if (!pNode)
				continue;

			//Go through pins in node
			for (int32 k=0; k<pNode->Pins.Num(); k++)
			{
				//
				class UEdGraphPin *pPin = pNode->Pins.GetData()[k];
				if (!pPin)
					continue;

				//
				if (pPin->LinkedTo.Num() > 0)
					continue;

				if (pPin->Direction != EEdGraphPinDirection::EGPD_Input)
					continue;

				//Make sure correct type
				static const FName Name_Text = TEXT("text");
				if (pPin->PinType.PinCategory != Name_Text)
				{
					continue;
				}

				OutPins.Add(pPin);
			}
		}
	}
}

//===================================================================================================
// 
//===================================================================================================
void GatherAllTextPins(class UObject *InObject, TArray<UEdGraphPin*> &OutPins)
{
	//If object, then check if blueprint object
	class UBlueprint *pBlueprint = Cast<UBlueprint>(InObject->GetClass()->ClassGeneratedBy);
	if (pBlueprint)
	{
		GatherTextPinsFromGraph( pBlueprint->UbergraphPages, OutPins);
		GatherTextPinsFromGraph( pBlueprint->FunctionGraphs, OutPins);
		GatherTextPinsFromGraph( pBlueprint->MacroGraphs, OutPins);
	}
}

//===================================================================================================
// 
//===================================================================================================
bool UDialogue::GatherAllTexts(TSubclassOf<class UDialogue> DialogueScript, TArray<FText>& Texts)
{
	Texts.Reset();

	TArray<UEdGraphPin*> Pins;

	//If object, then check if blueprint object
	class UBlueprint* pBlueprint = Cast<UBlueprint>(DialogueScript->ClassGeneratedBy);
	if (pBlueprint)
	{
		GatherTextPinsFromGraph(pBlueprint->UbergraphPages, Pins);
		GatherTextPinsFromGraph(pBlueprint->FunctionGraphs, Pins);
		GatherTextPinsFromGraph(pBlueprint->MacroGraphs, Pins);
	}

	for (int32 i=0; i<Pins.Num(); i++)
	{
		Texts.Add(Pins.GetData()[i]->DefaultTextValue);
	}

	return Texts.Num() > 0;
}

//===================================================================================================9
// 
//===================================================================================================
class UEdGraphPin *FindTextPinInGraph(const TArray<class UEdGraph*>& Graphs, const FString &Text)
{
	//Go through different graphs
	for (int32 i = 0; i < Graphs.Num(); i++)
	{
		//
		class UEdGraph* pGraph = Graphs.GetData()[i];
		if (!pGraph)
			continue;

		//Go through graph nodes
		for (int32 j = 0; j < pGraph->Nodes.Num(); j++)
		{
			//
			class UEdGraphNode* pNode = pGraph->Nodes.GetData()[j];
			if (!pNode)
				continue;

			//Go through pins in node
			for (int32 k = 0; k < pNode->Pins.Num(); k++)
			{
				//
				class UEdGraphPin* pPin = pNode->Pins.GetData()[k];
				if (!pPin)
					continue;

				//
				if (pPin->LinkedTo.Num() > 0)
					continue;

				if (pPin->Direction != EEdGraphPinDirection::EGPD_Input)
					continue;

				//Make sure correct type
				static const FName Name_Text = TEXT("text");
				if (pPin->PinType.PinCategory != Name_Text)
				{
					continue;
				}

				if (pPin->DefaultTextValue.ToString().Equals(Text))
					return pPin;
			}
		}
	}

	return NULL;
}

//===================================================================================================
// 
//===================================================================================================
class UEdGraphPin *UDialogue::FindPinWithText(TSubclassOf<class UDialogue> DialogueScript, const FText &Text)
{
	//If object, then check if blueprint object
	class UBlueprint* pBlueprint = Cast<UBlueprint>(DialogueScript->ClassGeneratedBy);
	if (pBlueprint)
	{
		FString String = Text.ToString();

		class UEdGraphPin *pPin;

		pPin = FindTextPinInGraph(pBlueprint->UbergraphPages, String);
		if (pPin)
			return pPin;

		pPin = FindTextPinInGraph(pBlueprint->FunctionGraphs, String);
		if (pPin)
			return pPin;

		pPin = FindTextPinInGraph(pBlueprint->MacroGraphs, String);
		if (pPin)
			return pPin;
	}

	return NULL;
}

//===================================================================================================
// 
//===================================================================================================
void GatherFloatPinsFromGraph(const TArray<class UEdGraph *> &Graphs, TArray<UEdGraphPin*> &OutPins, FName InRequiredName)
{
	//Go through different graphs
	for (int32 i=0; i<Graphs.Num(); i++)
	{
		//
		class UEdGraph *pGraph = Graphs.GetData()[i];
		if (!pGraph)
			continue;

		//Go through graph nodes
		for (int32 j=0; j<pGraph->Nodes.Num(); j++)
		{
			//
			class UEdGraphNode *pNode = pGraph->Nodes.GetData()[j];
			if (!pNode)
				continue;

			//Go through pins in node
			for (int32 k=0; k<pNode->Pins.Num(); k++)
			{
				//
				class UEdGraphPin *pPin = pNode->Pins.GetData()[k];
				if (!pPin)
					continue;

				//
				if (pPin->LinkedTo.Num() > 0)
					continue;

				if (pPin->Direction != EEdGraphPinDirection::EGPD_Input)
					continue;

				//Make sure correct type
				/*
				static const FName Name_Float = TEXT("float");
				if (pPin->PinType.PinCategory != Name_Float)
				{
					continue;
				}
				*/

				if (pPin->PinName != InRequiredName)
				{
					continue;
				}

				OutPins.Add(pPin);
			}
		}
	}
}

//===================================================================================================
// 
//===================================================================================================
void GatherAllFloatPins(class UObject *InObject, TArray<UEdGraphPin*> &OutPins, FName InRequiredName)
{
	//If object, then check if blueprint object
	class UBlueprint *pBlueprint = Cast<UBlueprint>(InObject->GetClass()->ClassGeneratedBy);
	if (pBlueprint)
	{
		GatherFloatPinsFromGraph( pBlueprint->UbergraphPages, OutPins, InRequiredName);
		GatherFloatPinsFromGraph( pBlueprint->FunctionGraphs, OutPins, InRequiredName);
		GatherFloatPinsFromGraph( pBlueprint->MacroGraphs, OutPins, InRequiredName);
	}
}

//===================================================================================================
// 
//===================================================================================================
void UDialogue::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (GenerateTextsFromClipboard)
	{
		FString Dest;
		FPlatformApplicationMisc::ClipboardPaste(Dest);

		Dest.ParseIntoArray(ClipboardTexts, TEXT("\n"), true);

		UE_LOG(LogTemp, Error, TEXT("From \"%s\" got %d texts!"), *Dest, ClipboardTexts.Num());

		for (int32 i=0; i<ClipboardTexts.Num(); i++)
		{
			ClipboardTexts.GetData()[i] = ClipboardTexts.GetData()[i].Replace(TEXT("\t"), TEXT(""), ESearchCase::IgnoreCase);
			ClipboardTexts.GetData()[i] = ClipboardTexts.GetData()[i].Replace(TEXT("\r"), TEXT(""), ESearchCase::IgnoreCase);
			ClipboardTexts.GetData()[i] = ClipboardTexts.GetData()[i].Replace(TEXT("\""), TEXT(""), ESearchCase::IgnoreCase);
		}

		GenerateTextsFromClipboard = false;
	}

	if (ParseNodesFromClipboard)
	{
		FString Dest;
		FPlatformApplicationMisc::ClipboardPaste(Dest);

		TArray<FString> Lines;

		TArray<FString> NodeStrings;
		Dest.ParseIntoArray(NodeStrings, TEXT("Begin Object"), true);
		for (int32 i=0; i< NodeStrings.Num(); i++)
		{
			FString LineString;
			if (ParseNodeToString(NodeStrings.GetData()[i], LineString))
			{
				Lines.Add(LineString);
			}
		}

		if (Lines.Num() > 0)
		{
			ClipboardTexts = Lines;
		}

		ParseNodesFromClipboard = false;
	}

	if (CopyClipboardTextArrayToClipBoard)
	{
		FString Dest;
		for (int32 i=0; i<ClipboardTexts.Num(); i++)
		{
			Dest += ClipboardTexts.GetData()[i] + TEXT("\r\n");
		}

		FPlatformApplicationMisc::ClipboardCopy(*Dest);

		CopyClipboardTextArrayToClipBoard = false;
	}

	if ( GenerateNodesFromTextsAndCopyToClipboard )
	{
		FString Text;

		FString PreviousSpeaker = TEXT("Target");

		int32 PinNum = 1;
		for (int32 i=0; i<ClipboardTexts.Num(); i++)
		{
			GenerateNode(ClipboardTexts.GetData()[i], GenerateDefaultDuration, i+1, i < ClipboardTexts.Num()-1, PinNum, Text, PreviousSpeaker);
		}

		FPlatformApplicationMisc::ClipboardCopy(*Text);

		GenerateNodesFromTextsAndCopyToClipboard = false;
	}

	if (ResetTimeInAllNodes)
	{
		ResetTimeInAllNodes = false;

		TArray<class UEdGraphPin*> Pins;
		GatherAllFloatPins(this, Pins, TEXT("Duration"));

		for (int32 i=0; i<Pins.Num(); i++)
		{
			Pins.GetData()[i]->DefaultValue = FString::Printf(TEXT("%f"), GenerateDefaultDuration);
		}
	}

	bool bDestroyStringTable = false;
	if (AreYouSure && DestroyEntriesInStringTable)
	{
		AreYouSure = false;
		DestroyEntriesInStringTable = false;
		bDestroyStringTable = true;
	}

	if (UnlinkStringTablesInAllNodes || bDestroyStringTable)
	{
		UnlinkStringTablesInAllNodes = false;

		/*
		TArray<class UEdGraphPin*> Pins;
		GatherAllTextPins(this, Pins);
		for (int32 i=0; i<Pins.Num(); i++)
		{
			Pins.GetData()[i]->DefaultTextValue = FText::FromString(Pins.GetData()[i]->DefaultTextValue.ToString());
		}
		*/

		ClearStringTableUse(StringTable, this);
		ClearStringTableUse(DefaultStringTable, this);
	}

	if (bDestroyStringTable)
	{
		class UStringTable *pStringTable = StringTable.IsPending() ? StringTable.LoadSynchronous() : StringTable.Get();
		if (pStringTable)
		{
			FStringTable &MutableStringTable = pStringTable->GetMutableStringTable().Get();

			MutableStringTable.ClearSourceStrings();
		}
	}
}

#endif //