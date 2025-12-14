// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DialogueContext.h"
#include "DialogueSystemEnums.h"
#include "GameplayTagContainer.h"
#include "DialogueManager.generated.h"

//==============================================================================================================
//
//==============================================================================================================
UCLASS( ClassGroup=(Heroine), meta=(BlueprintSpawnableComponent) )
class SIMPLEDIALOGUE_API UDialogueManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDialogueManager();

	//
	virtual void Initialize(class APlayerController *InController);

	//
	void EndPlay(const EEndPlayReason::Type EndPlayReason);

	//
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	//
	FORCEINLINE const FText &GetUnknownName() const { return UnknownNameText; }

	//
	FORCEINLINE const FText &GetNarratorNameText() const { return NarratorNameText; }

	//==============================================================================================================
	// DIALOGUE
	//==============================================================================================================
public:

	//
	void UpdateDialogueMode();

	//
	UFUNCTION(BlueprintCallable)
	void SetDialogueHoveredAsset(class UDataAsset* InAsset);
	FORCEINLINE class UDataAsset *GetHoveredAsset() const { return HoveredAsset; }

	//
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool InDialogue() const;

	//
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool InOneLineDialogue() const;

	UFUNCTION(BluePrintCallable, Category = "Dialogue")
	void ClearDialogue();

	//Go to next dialogue
	UFUNCTION(BluePrintCallable, Category = "Dialogue")
	bool SkipDialogue(bool All);

	//
	//
	UFUNCTION(BlueprintCallable)
	bool SpeakOneLineDialogue(FText InText, class AActor *InPlayer, class AActor *InActor, EDialogueSpeaker InSpeaker, EDialogueExpression Expression, EDialogueEffect InEffect, float InDuration, bool InOverrideCurrent);
	
	//
	UFUNCTION(BlueprintCallable, meta = (Latent, LatentInfo = "LatentInfo"))
	void SpeakOneLineDialogueLatent(FText InText, class AActor *InPlayer, class AActor *InActor, EDialogueSpeaker InSpeaker, EDialogueExpression Expression, EDialogueEffect InEffect, float InDuration, bool InOverrideCurrent, FLatentActionInfo LatentInfo);

	//
	UFUNCTION(BlueprintCallable)
	bool SpeakDialogue(TSoftClassPtr<class UDialogue> NewDialogue, class AActor *InPlayer, class AActor *InActor, UPARAM(meta=(Categories="Dialogue,Context")) FGameplayTag InSpeakContext, bool InWaitForActivation);

	//
	UFUNCTION(BlueprintCallable, meta = (Latent, LatentInfo = "LatentInfo"))
	void SpeakDialogueLatent(TSoftClassPtr<UDialogue> NewDialogue, class AActor *InPlayer, class AActor *InActor, UPARAM(meta = (Categories = "Dialogue,Context")) FGameplayTag InSpeakContext, bool InWaitForActivation, FLatentActionInfo LatentInfo);

	//
	UFUNCTION(BlueprintCallable)
	void ActivateDialogue();

	//
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void QueueDialogueUpdate() { ShouldUpdateDialogue = true; }

	//
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool CanSelectDialogueOption(int32 Index) const;

	//
	UFUNCTION(BluePrintCallable, Category = "Dialogue")
	bool SelectDialogueOption(int32 Index);

	//
	bool HandleDialogueChoiceMoveUpDown(int32 Direction);

	//
	bool HandleDialogueChoiceMoveRightLeft(int32 Direction);

	//
	bool HandleSelectDialogueChoice();

	//Toggle pause
	UFUNCTION(BluePrintCallable, Category = "Dialogue")
	void TogglePause();

	//Is Paused
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsPaused() const;

	//
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool HasDialogueOptions() const;

	//
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool HasDuration() const;

	//Get time fraction
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	float GetTimeFraction() const;

	//Get dialogue text
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FText GetText() const;

	//
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	class AActor *GetSpeaker() const;

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsSpeakerPlayer() const;

	//
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	EDialogueEffect GetEffect() const;

	//Are we in dialogue mode
	FORCEINLINE float GetTimePerLetter() const { return TimePerLetter; }
	FORCEINLINE float GetAdditionalTextTime() const { return AdditionalTextTime; }
	FORCEINLINE float GetMinimumTextTime() const { return MinimumTextTime; }

	//
	class AActor *GetPlayerActor() const;

	//
	class AActor *GetDialogueTarget() const;

	//
	UFUNCTION(BlueprintPure)
	bool IsActorInvolved(const class AActor * const InActor) const;

	//
	FORCEINLINE const class UDialogue * const GetDialogue() const { return Dialogue; }

	//
	void MarkShouldUpdateSpeaker() { ShouldUpdateSpeaker = true; }

private:

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Dialogue")
	class UDialogue					*Dialogue = NULL;

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Dialogue")
	bool WaitingForActivation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Dialogue")
	class UDataAsset *HoveredAsset = NULL;

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	TWeakObjectPtr<class APlayerController> PlayerController;

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	TWeakObjectPtr<class AActor> PreviousSpeaker;

	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	EDialogueExpression PreviousExpression;

	//Dialogue can change several times in one frame. Sometimes native events wont fire if they have already been fired that frame.
	//Using Should Update Dialogue we make sure that the HUD gets updated to the most up to date data.
	//If dialogue is changed after the components ComponentTick the HUD will be updated next frame
	UPROPERTY(Transient)
	bool ShouldUpdateDialogue;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Dialogue")
	float							TimePerLetter = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Dialogue")
	float							AdditionalTextTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Dialogue")
	float							MinimumTextTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin=0), Category = "Dialogue")
	int32 MaxChoices = 4;

	UPROPERTY(Transient)
	bool ShouldUpdateSpeaker;

public:

	//
	UFUNCTION(BlueprintPure)
	int32 GetMaxChoices() const;

	//==============================================================================================================
	// DELEGATES
	//==============================================================================================================
public:

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDialogueManagerEvent);
	
	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueContextEvent, FGameplayTag, Tag, int32, Value);

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueActorEvent, class AActor *, PreviousSpeaker, class AActor*, CurrentSpeaker);

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueSpeakerEvent, class AActor*, Speaker, EDialogueExpression, Expression);

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueAssetEvent, class UDataAsset *, DataAsset);

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueEvent, class AActor*, Speaker, FText, Text);

	//
	UPROPERTY(BlueprintAssignable)
	FDialogueManagerEvent OnDialogueUpdated;

	//
	UPROPERTY(BlueprintAssignable)
	FDialogueContextEvent OnGlobalContextChanged;

	//
	UPROPERTY(BlueprintAssignable)
	FDialogueActorEvent OnSpeakerChanged;

	//
	UPROPERTY(BlueprintAssignable)
	FDialogueSpeakerEvent OnExpressionChanged;

	//
	UPROPERTY(BlueprintAssignable)
	FDialogueAssetEvent OnDialogueAssetHovered;

	//
	UPROPERTY(BlueprintAssignable)
	FDialogueEvent OnDialogue;

	//
	UFUNCTION(BlueprintCallable)
	void ClearAllEvents(class UObject *Object);

	//==============================================================================================================
	// CONTEXT
	//==============================================================================================================
public:

	//
	UFUNCTION(BlueprintCallable, meta=(CallableWithoutWorldContext=true))
	static void GetAllContextTags(TArray<FGameplayTag> &OutTags);

	//
	UFUNCTION(BlueprintCallable)
	bool AddContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag, int32 InValue);

	//Increase context value by Value, if new total value is higher or equal to Limit then value is set to Value modulos Limit and outputs the quetient
	//Returns true only if reached limit. 
	//Note: If value is currently 1, limit 10, and value added is 10 then, quetient is 1 and true is returned, but the context value doesn't change (it was 1 before and is 1 after).
	//In cases like these OnGlobalContextChanged wont be triggered.
	UFUNCTION(BlueprintCallable)
	bool IncreaseContextToLimit(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag, int32& OutQuetient, UPARAM(meta = (ClampMin = 1)) int32 InValue = 1, UPARAM(meta=(ClampMin=1)) int32 InLimit = 10);

	//
	UFUNCTION(BlueprintCallable)
	bool IncrementContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag, int32 InValue = 1, bool AddIfMissing = true);

	UFUNCTION(BlueprintCallable)
	bool RemoveContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag = FGameplayTag());

	//
	UFUNCTION(BlueprintPure)
	bool HasContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag = FGameplayTag()) const;

	//
	UFUNCTION(BlueprintPure)
	int32 GetContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag = FGameplayTag()) const;

	//
	//int32 FindContext(const FGameplayTag &InTag, const FGameplayTag &InActorTag) const;

	//
	FContextMapType *FindContextMap(const FGameplayTag &InActorTag) const;

	//
	UFUNCTION(BlueprintCallable)
	int32 MakeRandomRoll(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag, int32 InMin, int32 InMax);

	//
	UFUNCTION(BlueprintCallable)
	bool RemoveAllContextFor(UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag);

	//
	FORCEINLINE bool IsGlobalContext(const FGameplayTag &InActorTag) const { return !InActorTag.IsValid(); }

private:

	//
	UPROPERTY(EditAnywhere, Category="Runtime")
	TArray<FSavedContext> Context;

	//
	UPROPERTY(SaveGame, EditAnywhere, Category = "Runtime", SimpleDisplay)
	TMap<FGameplayTag, int32> GlobalContext;

	//
	UPROPERTY(SaveGame, EditAnywhere, Category = "Runtime", SimpleDisplay, meta = (ShowOnlyInnerProperties = true))
	TMap<FGameplayTag, FSavedContextMap> ActorContext;

private:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true), Category="Settings")
	FText UnknownNameText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true), Category="Settings")
	FText NarratorNameText;
};

//==============================================================================================================
// 
//==============================================================================================================
FORCEINLINE void UDialogueManager::ClearAllEvents(class UObject *Object)
{
	OnDialogueUpdated.RemoveAll(Object);
	OnGlobalContextChanged.RemoveAll(Object);
	OnSpeakerChanged.RemoveAll(Object);
	OnExpressionChanged.RemoveAll(Object);
	OnDialogueAssetHovered.RemoveAll(Object);
	OnDialogue.RemoveAll(Object);
}

//==============================================================================================================
// 
//==============================================================================================================
FORCEINLINE bool UDialogueManager::InDialogue() const
{
	return Dialogue != NULL;
}

//=================================================================
// 
//=================================================================
FORCEINLINE bool UDialogueManager::HasContext(FGameplayTag InTag, FGameplayTag InActorTag) const
{
#if WITH_EDITOR
	if (!IsValid(this))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Invalid dialogue manager!"));
	}
#endif //

	FContextMapType* pData = FindContextMap(InActorTag);
	return pData && pData->Contains(InTag);

	//return FindContext(InTag, InActorTag) != INDEX_NONE;
}

//=================================================================
// 
//=================================================================
FORCEINLINE int32 UDialogueManager::GetContext(FGameplayTag InTag, FGameplayTag InActorTag) const
{
#if WITH_EDITOR
	if (!IsValid(this))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Invalid dialogue manager!"));
	}
#endif //

	FContextMapType *pData = FindContextMap(InActorTag);
	const int32 *pValue = pData != NULL ? pData->Find(InTag) : NULL;
	if (pValue)
		return *pValue;

	return 0;
}

//=================================================================
// 
//=================================================================
FORCEINLINE FContextMapType* UDialogueManager::FindContextMap(const FGameplayTag& InActorTag) const
{
	if (IsGlobalContext(InActorTag))
		return &(const_cast<UDialogueManager*>(this)->GlobalContext);

	FSavedContextMap *pData = const_cast<UDialogueManager*>(this)->ActorContext.Find(InActorTag);
	if (pData != NULL)
		return &pData->Values;

	return NULL;
}

/*
//=================================================================
// 
//=================================================================
FORCEINLINE int32 UDialogueManager::FindContext(const FGameplayTag &InTag, const FGameplayTag &InActorTag) const
{
	for (int32 i=0; i<Context.Num(); i++)
	{
		if (!Context.GetData()[i].Tag.MatchesTag(InTag))
			continue;

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

		return i;
	}

	return INDEX_NONE;
}
*/

//=================================================================
// 
//=================================================================
FORCEINLINE bool UDialogueManager::SpeakOneLineDialogue(FText InText, class AActor *InPlayer, class AActor *InActor, EDialogueSpeaker InSpeaker, EDialogueExpression InExpression, EDialogueEffect InEffect, float InDuration, bool InOverrideCurrent)
{
	SpeakOneLineDialogueLatent(InText, InPlayer, InActor, InSpeaker, InExpression, InEffect, InDuration, InOverrideCurrent, FLatentActionInfo());
	return InDialogue();
}

//=================================================================
// 
//=================================================================
FORCEINLINE bool UDialogueManager::SpeakDialogue(TSoftClassPtr<class UDialogue> NewDialogue, class AActor *InPlayer, class AActor *InActor, FGameplayTag InSpeakContext, bool InWaitForActivation)
{
	SpeakDialogueLatent(NewDialogue, InPlayer, InActor, InSpeakContext, InWaitForActivation, FLatentActionInfo());
	return InDialogue();
}