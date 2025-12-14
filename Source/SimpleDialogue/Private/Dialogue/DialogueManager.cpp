// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Dialogue/DialogueManager.h"
#include "Dialogue/Dialogue.h"
#include "Internationalization/TextFormatter.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

#if WITH_EDITOR
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"
#endif //WITH_EDITOR
#include "Dialogue/OneLineDialogue.h"
#include "GameplayTagsManager.h"

//==============================================================================================================
//
//==============================================================================================================
UDialogueManager::UDialogueManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

//==============================================================================================================
//
//==============================================================================================================
bool FContextAndValue::CheckHasContext(class UDialogueManager* InManager, const FGameplayTag& InActorTag) const
{
	if (!Tag.IsValid())
		return true;

	if (bUseValue)
		return InManager->GetContext(Tag, InActorTag) == Value;

	return InManager->HasContext(Tag, InActorTag);
}

//==============================================================================================================
//
//==============================================================================================================
bool FContextAndValue::CheckDoesntHaveContext(class UDialogueManager* InManager, const FGameplayTag& InActorTag) const
{
	if (!Tag.IsValid())
		return true;

	if (bUseValue)
		return InManager->GetContext(Tag, InActorTag) != Value;

	return !InManager->HasContext(Tag, InActorTag);
}

//==============================================================================================================
//
//==============================================================================================================
bool FContextAndValue::ChangeContext(class UDialogueManager* InManager, const FGameplayTag& InActorTag) const
{
	if (!Tag.IsValid())
		return false;

	if (bUseValue)
		return InManager->AddContext(Tag, InActorTag, Value);

	if (!InManager->HasContext(Tag, InActorTag))
	{
		InManager->AddContext(Tag, InActorTag, Value);
		return true;
	}

	return false;
}

//==============================================================================================================
//
//==============================================================================================================
bool FContextAndValue::RemoveContext(class UDialogueManager* InManager, const FGameplayTag& InActorTag) const
{
	if (!Tag.IsValid())
		return false;

	if (bUseValue)
		return false;

	if (!InManager->HasContext(Tag, InActorTag))
	{
		InManager->RemoveContext(Tag, InActorTag);
		return true;
	}

	return false;
}

//==============================================================================================================
//
//==============================================================================================================
void UDialogueManager::GetAllContextTags(TArray<FGameplayTag>& OutTags)
{
	FGameplayTag ContextTag = FGameplayTag::RequestGameplayTag(TEXT("Context"), false);
	FGameplayTagContainer Container = UGameplayTagsManager::Get().RequestGameplayTagChildren(ContextTag);
	Container.GetGameplayTagArray(OutTags);

	FGameplayTag DocksTag = FGameplayTag::RequestGameplayTag(TEXT("Docks"), false);
	FGameplayTagContainer DocksContainer = UGameplayTagsManager::Get().RequestGameplayTagChildren(DocksTag);
	TArray<FGameplayTag> OutDocksTags;
	DocksContainer.GetGameplayTagArray(OutDocksTags);
	OutTags.Append(OutDocksTags);
}

//==============================================================================================================
//
//==============================================================================================================
void UDialogueManager::Initialize(class APlayerController *InController)
{
	PlayerController = InController;
	QueueDialogueUpdate();

	for (int32 i=Context.Num()-1; i>=0; i--)
	{
		AddContext(Context.GetData()[i].Tag, Context.GetData()[i].ActorTag, Context.GetData()[i].Value);
	}
	Context.Reset();
}

//==============================================================================================================
//
//==============================================================================================================
class AActor *UDialogueManager::GetPlayerActor() const
{
	if (IsValid(Dialogue))
	{
		return Dialogue->GetPlayerActor();
	}

	return NULL;
}

//
//==============================================================================================================
class AActor *UDialogueManager::GetDialogueTarget() const
{
	if (IsValid(Dialogue))
	{
		return Dialogue->GetDialogueTarget();
	}

	return NULL;
}

//==============================================================================================================
//
//==============================================================================================================
int32 UDialogueManager::GetMaxChoices() const
{
	if (IsValid(Dialogue) && Dialogue->GetOverrideMaxChoices() > 0)
	{
		return Dialogue->GetOverrideMaxChoices();
	}

	return MaxChoices;
}

//==============================================================================================================
//
//==============================================================================================================
void UDialogueManager::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Dialogue)
		Dialogue->Update(DeltaTime);

	if (ShouldUpdateDialogue)
	{
		OnDialogueUpdated.Broadcast();
		ShouldUpdateDialogue = false;
	}

	if (ShouldUpdateSpeaker)
	{
		class AActor *pPrevious = PreviousSpeaker.Get();
		class AActor *pCurrent = InDialogue() && !HasDialogueOptions() ? Dialogue->GetActor() : NULL;
		EDialogueExpression NewExpression = InDialogue() ? Dialogue->GetExpression() : EDialogueExpression::None;
		
		if (pPrevious != pCurrent)
		{
			OnSpeakerChanged.Broadcast(pPrevious, pCurrent);
		}

		if (IsValid(pCurrent) && (pPrevious != pCurrent || NewExpression != PreviousExpression))
		{
			OnExpressionChanged.Broadcast(pCurrent, NewExpression);
		}

		PreviousSpeaker = pCurrent;

		ShouldUpdateSpeaker = false;
	}

	if (!InDialogue())
	{
		PrimaryComponentTick.SetTickFunctionEnable(false);
	}
}

//==============================================================================================================
//
//==============================================================================================================
void UDialogueManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearDialogue();

	Super::EndPlay(EndPlayReason);
}

//=================================================================
// 
//=================================================================
void UDialogueManager::UpdateDialogueMode()
{

}

//=================================================================
// 
//=================================================================
void UDialogueManager::SetDialogueHoveredAsset(class UDataAsset *InAsset)
{
	if (HoveredAsset != InAsset)
	{
		HoveredAsset = InAsset;
		OnDialogueAssetHovered.Broadcast(HoveredAsset);
	}
}

//=================================================================
// 
//=================================================================
void UDialogueManager::ClearDialogue()
{
	SetDialogueHoveredAsset(NULL);

	if (Dialogue)
	{
		ShouldUpdateSpeaker = true;

#if WITH_EDITOR
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(Dialogue);
#endif

		Dialogue->Deactivate();
		Dialogue = NULL;

		QueueDialogueUpdate();
		UpdateDialogueMode();
	}
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::SkipDialogue(bool All)
{
	if (!Dialogue || Dialogue->HasChoices())
	{ 
		UE_LOG(LogTemp, Warning, TEXT("No dialogue!"));
		return false;
	}

	if (All)
	{
		while (Dialogue && Dialogue->HasDialogue() && !Dialogue->HasChoices())
		{
			Dialogue->Skip();
		}

		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("Skipping dialogue!"));
	Dialogue->Skip();
	return true;
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::HandleDialogueChoiceMoveUpDown(int32 Direction)
{
	if (!Dialogue || !Dialogue->HasChoices())
	{ 
		UE_LOG(LogTemp, Warning, TEXT("No dialogue!"));
		return false;
	}

	return Dialogue->MoveUpDownOptions(Direction);
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::HandleDialogueChoiceMoveRightLeft(int32 Direction)
{
	if (!Dialogue || !Dialogue->HasChoices())
	{ 
		UE_LOG(LogTemp, Warning, TEXT("No dialogue!"));
		return false;
	}

	return Dialogue->GoChoicesDirection(-Direction);
}


//=================================================================
// 
//=================================================================
bool UDialogueManager::HandleSelectDialogueChoice()
{
	if (!Dialogue || !Dialogue->HasChoices())
	{ 
		UE_LOG(LogTemp, Warning, TEXT("No dialogue!"));
		return false;
	}

	return Dialogue->SelectOption(Dialogue->GetHoveredChoice());
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::InOneLineDialogue() const
{
	return Dialogue != NULL && Dialogue->GetClass()->IsChildOf(UOneLineDialogue::StaticClass());
}

//=================================================================
// 
//=================================================================
void UDialogueManager::SpeakOneLineDialogueLatent(FText InText, class AActor *InPlayer, class AActor *InActor, EDialogueSpeaker InSpeaker, EDialogueExpression InExpression, EDialogueEffect InEffect, float InDuration, bool InOverrideCurrent, FLatentActionInfo LatentInfo)
{
#if WITH_EDITOR
	if (!IsValid(this))
	{
		UE_LOG(LogTemp, Fatal, TEXT("NULL dialogue manager!"));
	}
#endif //

	if (InText.IsEmpty() || (!InOverrideCurrent && InDialogue()) || !IsValid(InPlayer))
	{
		UFunction* pExecutionFunction = IsValid(LatentInfo.CallbackTarget) ? LatentInfo.CallbackTarget->FindFunction(LatentInfo.ExecutionFunction) : NULL;
		if (pExecutionFunction)
		{
			LatentInfo.CallbackTarget->ProcessEvent(pExecutionFunction, &LatentInfo.Linkage);
		}

		return;
	}

	ClearDialogue();

	Dialogue = NewObject<UDialogue>(this, UOneLineDialogue::StaticClass());
	class UOneLineDialogue *pOneLine = Cast<UOneLineDialogue>(Dialogue);
	if (pOneLine)
	{
		pOneLine->Text = InText;
		pOneLine->Speaker = InSpeaker;
		pOneLine->Expression = InExpression;
		pOneLine->Effect = InEffect;
		pOneLine->Duration = InDuration;

		//UE_LOG(LogTemp, Error, TEXT("Starting dialogue..."));

		UpdateDialogueMode();

		Dialogue->Init(this, PlayerController.Get(), InPlayer, InActor, FGameplayTag(), LatentInfo);
		Dialogue->Activate();
		QueueDialogueUpdate();
		UpdateDialogueMode();

		PrimaryComponentTick.SetTickFunctionEnable(true);
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("Failed to speak dialogue \"%s\""), *InText.ToString());
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::IsActorInvolved(const class AActor* const InActor) const
{
	if (!IsValid(InActor))
		return false;

	if (!IsValid(Dialogue))
		return false;

	if (Dialogue->GetDialogueTarget() == InActor)
		return true;

	if (Dialogue->GetPlayerActor() == InActor)
		return true;

	if (Dialogue->GetCustomSpeakerTag(InActor).IsValid())
		return true;

	return false;
}

//=================================================================
// 
//=================================================================
void UDialogueManager::SpeakDialogueLatent(TSoftClassPtr<UDialogue> NewDialogue, class AActor *InPlayer, class AActor *InActor, FGameplayTag InSpeakContext, bool InWaitForActivation, FLatentActionInfo LatentInfo)
{
	if (!IsValid(InPlayer))
	{
		return;
	}

	TSubclassOf<UDialogue> DialogueClass = UDialogue::LoadDialogue(NewDialogue);
	if (!DialogueClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid dialogue class \"%s\""), *NewDialogue.ToString());
		return;
	}

	ClearDialogue();

	Dialogue = NewObject<UDialogue>(this, DialogueClass);
	if (Dialogue)
	{
		//UE_LOG(LogTemp, Error, TEXT("Starting dialogue..."));

		UpdateDialogueMode();

		Dialogue->Init(this, PlayerController.Get(), InPlayer, InActor, InSpeakContext, LatentInfo);

		if (InWaitForActivation)
		{
			WaitingForActivation = true;
		}
		else
		{
			WaitingForActivation = false;
			Dialogue->Activate();
		}

		QueueDialogueUpdate();
		UpdateDialogueMode();

		PrimaryComponentTick.SetTickFunctionEnable(true);
		return;
	}

	UE_LOG(LogTemp, Error, TEXT("Failed to speak dialogue \"%s\""), NewDialogue != NULL ? *NewDialogue->GetName() : TEXT("NULL"));
}

//=================================================================
// 
//=================================================================
void UDialogueManager::ActivateDialogue()
{
	if (Dialogue && WaitingForActivation)
	{
		WaitingForActivation = false;
		Dialogue->Activate();
	}
}

//=================================================================
// 
//=================================================================
void UDialogueManager::TogglePause()
{
	if (Dialogue)
		Dialogue->SetPaused(!Dialogue->IsPaused());
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::CanSelectDialogueOption(int32 Index) const
{
	if (!Dialogue)
		return false;

	return Dialogue->CanSelectOption(Index);
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::SelectDialogueOption(int32 Index)
{
	if (Dialogue)
		return Dialogue->SelectOption(Index);

	return false;
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::HasDialogueOptions() const
{
	return Dialogue && Dialogue->HasChoices();
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::IsPaused() const
{
	return Dialogue && Dialogue->IsPaused();
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::HasDuration() const
{
	return Dialogue != NULL ? Dialogue->HasDuration() : false;
}

//=================================================================
// 
//=================================================================
float UDialogueManager::GetTimeFraction() const
{
	return Dialogue != NULL ? Dialogue->GetTimeFraction() : 0.0f;
}

//=================================================================
// 
//=================================================================
FText UDialogueManager::GetText() const
{
	if (Dialogue)
	{
		//UDialogueManager *pController = const_cast<UDialogueManager*>(this);

		//TArray<FFormatArgumentData> InArgs;

		/*
		FFormatArgumentData attack;
		attack.ArgumentName = TEXT("Attack");
		attack.ArgumentValue = GetKeyDisplayName( pController->FindActionKey( TEXT("Attack"), pController->GetUsingGamepad() ) );
		attack.ArgumentValueType = EFormatArgumentType::Text;
		InArgs.Add(attack);

		FFormatArgumentData use;
		use.ArgumentName = TEXT("Use");
		use.ArgumentValue = GetKeyDisplayName( pController->FindActionKey( TEXT("Use"), pController->GetUsingGamepad() ) );
		use.ArgumentValueType = EFormatArgumentType::Text;
		InArgs.Add(use);
		*/

		return Dialogue->GetText(); //return FTextFormatter::Format(Dialogue->GetText(), MoveTemp(InArgs), false, false);
	}

	return FText::GetEmpty();
}

//=================================================================
// 
//=================================================================
class AActor *UDialogueManager::GetSpeaker() const
{
	return Dialogue != NULL && IsValid(Dialogue->GetActor()) ? Dialogue->GetActor() : NULL;
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::IsSpeakerPlayer() const
{
	return Dialogue != NULL ? GetPlayerActor() == Dialogue->GetActor() : true;
}

//=================================================================
// 
//=================================================================
EDialogueEffect UDialogueManager::GetEffect() const
{
	return Dialogue != NULL ? Dialogue->GetEffect() : EDialogueEffect::None;
}


//=================================================================
// 
//=================================================================
bool UDialogueManager::AddContext(FGameplayTag InTag, FGameplayTag InActorTag, int32 InValue)
{
	if (!InTag.IsValid())
		return false;

#if WITH_EDITOR
	if (!IsValid(this))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Invalid dialogue manager!"));
	}
#endif //

	FContextMapType* pData = FindContextMap(InActorTag);
	if (!pData)
	{
		FSavedContextMap NewMap;

		ActorContext.Add(InActorTag, NewMap);
		pData = &ActorContext[InActorTag].Values;
	}

	int32 *pValue = pData->Find(InTag);
	if (pValue != NULL)
	{
		if (*pValue == InValue)
			return false;

		*pValue = InValue;

		if (IsGlobalContext(InActorTag))
		{
			OnGlobalContextChanged.Broadcast(InTag, InValue);
		}

		return true;
	}

	/*
	int32 i = FindContext(InTag, InActorTag);
	if (i != INDEX_NONE)
	{
		if (Context.GetData()[i].Value != InValue)
		{
			Context.GetData()[i].Value = InValue;

			if (!InActorTag.IsValid())
			{
				OnGlobalContextChanged.Broadcast(InTag, InValue);
			}
			return true;
		}

		return false;
	}
	*/

	pData->Emplace(InTag, InValue);

	/*
	FSavedContext context;
	context.Tag = InTag;
	context.ActorTag = InActorTag;
	context.Value = InValue;
	Context.Add(context);
	*/

	if (IsGlobalContext(InActorTag))
	{
		OnGlobalContextChanged.Broadcast(InTag, InValue);
	}
	return true;
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::RemoveContext(FGameplayTag InTag, FGameplayTag InActorTag)
{
#if WITH_EDITOR
	if (!IsValid(this))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Invalid dialogue manager!"));
	}
#endif //

	FContextMapType *pData = FindContextMap(InActorTag);
	if (!pData)
		return false;

	if (!pData->Contains(InTag))
		return false;

	pData->Remove(InTag);

	if (IsGlobalContext(InActorTag))
	{
		OnGlobalContextChanged.Broadcast(InTag, 0);
	}

	/*
	int32 i = FindContext(InTag, InActorTag);
	if (i != INDEX_NONE)
	{
		//UE_LOG(LogTemp, Fatal, TEXT("Removing context %s for %s"), *InTag.ToString(), InActor != NULL ? *InActor->GetName() : TEXT("NULL"));
		Context.RemoveAt(i);
		return true;
	}
	*/

	return false;
}

//=================================================================
// 
//=================================================================
int32 UDialogueManager::MakeRandomRoll(FGameplayTag InTag, FGameplayTag InActorTag, int32 InMin, int32 InMax)
{
	/*
	int32 i = FindContext(InTag, InActorTag);
	if (i != INDEX_NONE)
		return FMath::Clamp(Context.GetData()[i].Value, InMin, InMax);
		*/

	FContextMapType *pData = FindContextMap(InActorTag);
	int32 *pValue = pData != NULL ? pData->Find(InTag) : NULL;
	if (pValue != NULL)
	{
		return FMath::Clamp(*pValue, InMin, InMax);
	}

	int32 iRandom = FMath::RandRange(InMin, InMax);
	AddContext(InTag, InActorTag, iRandom);
	return iRandom;
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::IncrementContext(FGameplayTag InTag, FGameplayTag InActorTag, int32 InValue, bool AddIfMissing)
{
	if (!AddIfMissing && !HasContext(InTag, InActorTag))
	{
		return false;
	}

	int32 iValue = GetContext(InTag, InActorTag);
	return AddContext(InTag, InActorTag, iValue + InValue);
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::IncreaseContextToLimit(FGameplayTag InTag, FGameplayTag InActorTag, int32& OutQuetient, int32 InValue, int32 InLimit)
{
	int32 iValue = GetContext(InTag, InActorTag) + InValue;
	if (iValue >= InLimit)
	{
		OutQuetient = iValue / InLimit;
		iValue = iValue - (OutQuetient * InLimit);

		AddContext(InTag, InActorTag, iValue);
		return true;
	}

	AddContext(InTag, InActorTag, iValue);
	return false;
}

//=================================================================
// 
//=================================================================
bool UDialogueManager::RemoveAllContextFor(FGameplayTag InActorTag)
{
	bool bSuccess = false;

	/*
	if (!IsValid(InActor))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Removing all global context!"));
	}
	*/

	FContextMapType *pData = FindContextMap(InActorTag);
	if (!pData || pData->Num() == 0)
		return false;

	pData->Reset();

	/*
	//
	for (int32 i=Context.Num()-1; i>=0; i--)
	{
		if (!InActorTag.IsValid())
		{
			if (Context.GetData()[i].ActorTag.IsValid())
				continue;
		}
		else
		{
			if (!Context.GetData()[i].ActorTag.IsValid())
				continue;

			if (!Context.GetData()[i].ActorTag.MatchesTag(InActorTag))
				continue;
		}

		//UE_LOG(LogTemp, Fatal, TEXT("Removing context %s for %s"), *Context.GetData()[i].Tag.ToString(), InActor != NULL ? *InActor->GetName() : TEXT("NULL"));
		Context.RemoveAt(i);
	}
	*/

	return bSuccess;
}