// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "Engine/DataAsset.h"
#include "DialogueChoice.h"
#include "GameplayTagContainer.h"
#include "Dialogue.generated.h"

//=============================================================================================================================================================================================================
//
//=============================================================================================================================================================================================================
UCLASS(Blueprintable, Abstract, Category = "Dialogue")
class SIMPLEDIALOGUE_API UDialogue : public UObject
{
public:
	GENERATED_BODY()

	//=============================================================================================================================================================================================================
	//
	//=============================================================================================================================================================================================================
public:

	//
	UFUNCTION(BlueprintCallable, meta = (CallableWithoutContext = true))
	static class TSubclassOf<class UDialogue> LoadDialogue(TSoftClassPtr<class UDialogue> InClass);

	FORCEINLINE bool IsActive() const { return bIsActive; }

	virtual void Init(class UDialogueManager *InDialogueManager, class APlayerController *InPlayerController, class AActor *InPlayer, class AActor *InActor, FGameplayTag InSpeakContext, FLatentActionInfo InLatentInfo);

	//
	virtual void Activate();

	UFUNCTION(BlueprintCallable)
	virtual void Deactivate();

	void Restore();

	void Update(float DeltaTime);

	virtual void Skip();

	UFUNCTION(BlueprintNativeEvent)
	void OnUpdate(float DeltaTime);
	virtual void OnUpdate_Implementation(float DeltaTime) { }

	UFUNCTION(BlueprintNativeEvent)
	void OnActivate();
	virtual void OnActivate_Implementation() { Deactivate(); }

	UFUNCTION(BlueprintNativeEvent)
	void OnDeactivate();
	virtual void OnDeactivate_Implementation() { }

	UFUNCTION(BlueprintNativeEvent)
	void OnRestore();
	virtual void OnRestore_Implementation() { }

	//
	class UWorld *GetWorld() const override;

private:

	mutable TWeakObjectPtr<UWorld> CachedWorld;

private:

	//
	UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	TWeakObjectPtr<class APlayerController> PlayerController;

	//
	UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	TWeakObjectPtr<class UDialogueManager> DialogueManager;

	UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	bool bIsActive;

	//Runtime
	UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	FGameplayTag SpeakContext;

	//=============================================================================================================================================================================================================
	// DIALOGUE
	//=============================================================================================================================================================================================================

public:

	//Find and set a custom speaker
	UFUNCTION(BlueprintCallable)
	bool FindCustomSpeaker(UPARAM(meta = (Categories = "Character,Guard,Enemy,Player,Item")) FGameplayTag InTag, TSoftClassPtr<class AActor> InClass);

	//Set custom speaker
	UFUNCTION(BlueprintCallable)
	void SetCustomSpeaker(UPARAM(meta = (Categories = "Character,Guard,Enemy,Player,Item")) FGameplayTag InTag, class AActor *InActor);

	//
	UFUNCTION(BlueprintPure)
	class AActor *GetCustomSpeaker(UPARAM(meta = (Categories = "Character,Guard,Enemy,Player,Item")) FGameplayTag InTag) const;

	//
	UFUNCTION(Blueprintpure)
	FGameplayTag GetCustomSpeakerTag(const class AActor * const InActor) const;

	//
	UFUNCTION(BlueprintNativeEvent)
	void PlayVoiceOver(class AActor *Actor, FGameplayTag VoiceOver, EDialogueExpression Expression);
	virtual void PlayVoiceOver_Implementation(class AActor* Actor, FGameplayTag VoiceOver, EDialogueExpression Expression) { }

	//Add dialogue. If you want this to be accompanied by Choices then do this last
	UFUNCTION(BlueprintCallable, meta = (BlueprintProtected, Latent, LatentInfo = "LatentInfo", MultiLine = true), DisplayName = "Dialogue")
	void DialogueBox(	EDialogueSpeaker Speaker,
						UPARAM(meta=(MultiLine=true)) FText Text, 
						FLatentActionInfo LatentInfo, 
						float Duration = 0.0f /*If negative the delay is automatically calculated. If zero the text will stay on screen until skipped. */,
						EDialogueExpression Expression = EDialogueExpression::None,
						EDialogueEffect Effect = EDialogueEffect::None,
						UPARAM(meta=(Categories="VoiceOver")) FGameplayTag VoiceOver = FGameplayTag(),
						UPARAM(meta=(Categories="Character,Guard,Enemy,Player")) FGameplayTag InCustomName = FGameplayTag());

	//Add dialogue. If you want this to be accompanied by Choices then do this last
	UFUNCTION(BlueprintCallable, meta = (BlueprintProtected, MultiLine = true), DisplayName = "Dialogue During Choices")
	void DialogueBoxNoLatent(	EDialogueSpeaker Speaker,
								FText Text, 
								float Duration = 0.0f /*If negative the delay is automatically calculated. If zero the text will stay on screen until skipped. */,
								EDialogueExpression Expression = EDialogueExpression::None,
								EDialogueEffect Effect = EDialogueEffect::None,
								UPARAM(meta = (Categories = "VoiceOver")) FGameplayTag VoiceOver = FGameplayTag(),
								UPARAM(meta = (Categories = "Character,Guard,Enemy,Player")) FGameplayTag InCustomName = FGameplayTag() );

	//
	void ClearDialogueBox();

	FORCEINLINE bool HasDialogue() const { return Box_IsValid || HasChoices(); }
	FORCEINLINE const FText &GetText() const { return Box_IsValid ? Box_Text : FText::GetEmpty(); }
	FORCEINLINE class AActor *GetActor() const { return Box_IsValid ? Box_Actor.Get() : NULL; }
	FORCEINLINE EDialogueEffect GetEffect() const { return Box_IsValid ? Box_Effect : EDialogueEffect::None; }
	FORCEINLINE EDialogueExpression GetExpression() const { return Box_IsValid ? Box_Expression : EDialogueExpression::None; }
	FORCEINLINE bool IsPaused() const { return Paused; }
	FORCEINLINE bool HasDuration() const { return Box_IsValid && !HasChoices() ? Box_HasDelay() : false; }
	FORCEINLINE float GetTimeFraction() const { return Box_IsValid && !HasChoices() ? Box_GetTimeFraction() : 1.0f; }
	
	//
	UFUNCTION(BlueprintPure)
	bool HasChoices() const;
	FORCEINLINE class AActor *GetPlayerActor() const { return PlayerActor.Get(); }
	FORCEINLINE class AActor *GetDialogueTarget() const { return DialogueTarget.Get(); }
	FORCEINLINE class APlayerController *GetPlayerController() const { return PlayerController.Get(); }

	FORCEINLINE void SetPaused(bool NewPaused) { Paused = NewPaused; }

	FORCEINLINE int32 GetOverrideMaxChoices() const { return OverrideMaxChoices; }

public:

	void Box_Execute();

	//
	FORCEINLINE bool Box_IsValidFunction() const { return !Box_ExecutionFunction.IsNone() && Box_OutputLink != INDEX_NONE; }

	/*
	FORCEINLINE const FText &GetText() const { return Text; }
	FORCEINLINE class AActor *GetActor() const { return Actor.Get(); }
	FORCEINLINE EDialogueEffect GetEffect() const { return Effect; }
	*/

	FORCEINLINE bool Box_HasDelay() const { return Box_Delay > 0.0f; }
	FORCEINLINE float Box_GetTimeFraction() const { return Box_Time / Box_Delay; }

	/*
	bool UpdateTime(float DeltaTime);
	*/

private:

	UPROPERTY(SaveGame)
	bool Box_IsValid;

	UPROPERTY(SaveGame)
	FText Box_Text = FText::GetEmpty();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime", SaveGame)
	TWeakObjectPtr<class AActor> Box_Actor = NULL;

	UPROPERTY(SaveGame)
	FName Box_ExecutionFunction = NAME_None;

	UPROPERTY(SaveGame)
	int32 Box_OutputLink = INDEX_NONE;

private:

	UPROPERTY(SaveGame)
	EDialogueEffect Box_Effect = EDialogueEffect::None;

	UPROPERTY(SaveGame)
	EDialogueExpression Box_Expression = EDialogueExpression::None;

	UPROPERTY(SaveGame)
	float Box_Delay = 0;

	UPROPERTY(SaveGame)
	float Box_Time = 0;

	//=============================================================================================================================================================================================================
	// DIALOGUE CHOICES
	//=============================================================================================================================================================================================================

public:

	//Add a dialogue choice. Use Sequence to have multiple.
	UFUNCTION(BlueprintCallable, meta = (BlueprintProtected, Latent, LatentInfo = "LatentInfo", AdvancedDisplay="DisableVisited,CustomChoiceName,ShowAlways,ChoiceAsset"), DisplayName = "Add Choice")
	void DialogueChoices(FText Text, FLatentActionInfo LatentInfo, bool Enabled = true, FName CustomChoiceName = NAME_None /*Used to determine if choice has been visited yet*/, bool DisableVisited = false, class UDataAsset *ChoiceAsset = NULL);

	//
	FORCEINLINE bool HasLatentInfo() const { return FinishedLatentInfo.CallbackTarget != NULL && !FinishedLatentInfo.ExecutionFunction.IsNone(); }

	//
	FORCEINLINE const TArray<FDialogueChoice> &GetChoices() const { return Choices; }

	UFUNCTION(BlueprintPure)
	bool CanSelectOption(int32 Index) const;

	UFUNCTION(BlueprintPure)
	class UDataAsset *GetChoiceAsset(int32 Index) const;

	UFUNCTION(BlueprintCallable)
	bool SelectOption(int32 Index);

	//
	UFUNCTION(BlueprintCallable)
	bool HoverOption(int32 Index);

	//
	UFUNCTION(BlueprintCallable)
	bool MoveUpDownOptions(int32 Direction = 1);

	//
	UFUNCTION(BlueprintPure)
	int32 GetNumChoices() const { return Choices.Num(); }

	//
	UFUNCTION(BlueprintCallable)
	bool GetChoicesLimited(TArray<int32> &OutChoices, int32 &OutMaxChoices);
	bool GetChoicesLimited_Internal(TArray<int32> *OutChoices, int32 &OutMaxChoices) const;

	//
	UFUNCTION(BlueprintPure)
	bool HasMoreChoices(int32 InDirection=1) const;

	//
	UFUNCTION(BlueprintCallable)
	bool GetAllActors(TArray<class AActor*>& OutActors);
	bool GetAllActorsConst(TArray<class AActor*>& OutActors) const;

	UFUNCTION(BlueprintCallable)
	bool GoChoicesDirection(int32 InDirection=1);

	//
	FORCEINLINE int32 GetHoveredChoice() const { return HoveredChoice; }

	//
	UFUNCTION(BlueprintPure)
	bool HasVisitedChoice(int32 Index) const;

	UFUNCTION(BlueprintPure)
	bool AreAllNextTasksVisited() const;

private:

	void ClearChoices();

protected:

	UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	class UDataAsset *LastClickedAsset = NULL;

	UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	FName LastClickedOption = NAME_None;

private:

	UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	TArray<FDialogueChoice> Choices;

	UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	int32 HoveredChoice = 0;

	//
	UPROPERTY(SaveGame, VisibleInstanceOnly, Category = "Runtime")
	TArray<FName> VisitedChoices;

	UPROPERTY(SaveGame, VisibleInstanceOnly, Category = "Runtime")
	int32 StartChoices;

	UPROPERTY(EditAnywhere, Category="Settings", meta = (ClampMin=0))
	int32 OverrideMaxChoices = 0;

	//=============================================================================================================================================================================================================
	// STRING TABLE
	//=============================================================================================================================================================================================================
public:

#if WITH_EDITOR
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;

	//
	static void UseStringTable(const TSoftObjectPtr<class UStringTable>& InStringTable, class UObject* InObject, bool InDontAdd);
	static void ClearStringTableUse(const TSoftObjectPtr<class UStringTable>& InStringTable, class UObject* InObject);
	void CheckUsingStringTable();

	UFUNCTION(BlueprintCallable, Category = "Test", meta = (CallableWithoutWorldContext = true, DevelopmentOnly = true))
	static void UseStringTables(class UObject *InObject, TSoftObjectPtr<class UStringTable> InGeneralStringTable, TSoftObjectPtr<class UStringTable> InObjectSpecificStringTable);

#endif

private:

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category="String Table")
	TSoftObjectPtr<class UStringTable> StringTable;

	UPROPERTY(EditAnywhere, Category="String Table")
	TSoftObjectPtr<class UStringTable> DefaultStringTable;
#endif //

public:

	//
	UFUNCTION(BlueprintCallable)
	void GetEveryoneInvolved(TArray<class AActor*> &OutActors);

	//=============================================================================================================================================================================================================
	//
	//=============================================================================================================================================================================================================
protected:

	//
	UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	TWeakObjectPtr<class AActor> PlayerActor;

	//
	UPROPERTY(SaveGame, VisibleInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	TWeakObjectPtr<class AActor> DialogueTarget;

	//
	UPROPERTY(SaveGame, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	TMap<FGameplayTag, TWeakObjectPtr<class AActor>> CustomSpeakers;

	//Not valid during DoesPass
	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	bool					Paused;

	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Runtime")
	FLatentActionInfo FinishedLatentInfo;

	//==============================================================================================================
	// CONTEXT
	//==============================================================================================================
public:

	//
	UFUNCTION(BlueprintCallable)
	bool AddContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag, int32 InValue);

	//
	UFUNCTION(BlueprintCallable)
	bool IncrementContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag = FGameplayTag(), int32 InValue = 1, bool AddIfMissing = true);

	UFUNCTION(BlueprintCallable)
	bool RemoveContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag = FGameplayTag());

	//
	UFUNCTION(BlueprintPure)
	bool HasContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag = FGameplayTag()) const;

	//Gave the value for context
	UFUNCTION(BlueprintPure)
	int32 GetContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag = FGameplayTag()) const;

	//
	UFUNCTION(BlueprintCallable)
	bool AddContextTarget(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, int32 InValue);

	//
	UFUNCTION(BlueprintCallable)
	bool IncrementContextTarget(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, int32 InValue = 1, bool AddIfMissing = true);

	UFUNCTION(BlueprintCallable)
	bool RemoveContextTarget(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag);

	//
	UFUNCTION(BlueprintPure)
	bool HasContextTarget(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag) const;

	//Gave the value for context
	UFUNCTION(BlueprintPure)
	int32 GetContextTarget(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag) const;

	//
	UFUNCTION(BlueprintCallable)
	bool AddGlobalContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, int32 InValue);

	//
	UFUNCTION(BlueprintCallable)
	bool IncrementGlobalContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, int32 InValue = 1, bool AddIfMissing = true);

	UFUNCTION(BlueprintCallable)
	bool RemoveGlobalContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag);

	//
	UFUNCTION(BlueprintPure)
	bool HasGlobalContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag) const;

	//Gave the value for context
	UFUNCTION(BlueprintPure)
	int32 GetGlobalContext(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag) const;


	//Saves the result so that if used again will give the same result
	UFUNCTION(BlueprintCallable)
	int32 MakeRandomRoll(UPARAM(meta = (Categories = "Context,Docks")) FGameplayTag InTag, int32 InMin, int32 InMax);

	//
	UFUNCTION(BlueprintCallable)
	bool RemoveAllContextFor(UPARAM(meta = (Categories = "Character.Name")) FGameplayTag InActorTag);

	//
	UFUNCTION(BlueprintCallable)
	bool RemoveAllContextForTarget();

	//
	UFUNCTION(BlueprintCallable)
	bool RemoveAllGlobalContext();

	//
	virtual const FGameplayTag &GetTargetActorTag() const { return FGameplayTag::EmptyTag; }

	//==============================================================================================================
	// CLIPBOARD GENERATOR
	//==============================================================================================================
public:

#if WITH_EDITOR
	//
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	//
	void GenerateNode(const FString &InString, float InDuration, int32 Index, bool HasNext, int32 &PinNum, FString &OutString, FString &PreviousSpeaker);
	bool ParseNodeToString(const FString& NodeString, FString& OutLine);

	//
#endif //

#if WITH_EDITORONLY_DATA

	//Select nodes, hit Ctrl + C and then toggle this value enabled. ClipboardTexts array should get populated with the values of the selected nodes
	UPROPERTY(EditAnywhere, Category = "Nodes to Clipboard")
	bool ParseNodesFromClipboard;

	//Copy from ClipboardText array to clipboard and allow pasting back to text editor program
	UPROPERTY(EditAnywhere, Category = "Nodes to Clipboard")
	bool CopyClipboardTextArrayToClipBoard;

	//
	UPROPERTY(EditAnywhere, Category="Clipboard")
	TArray<FString> ClipboardTexts;

	//
	UPROPERTY(EditAnywhere, Category="Clipboard")
	bool GenerateTextsFromClipboard;

	//
	UPROPERTY(EditAnywhere, Category="Clipboard")
	float GenerateDefaultDuration = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Clipboard")
	bool GenerateNodesVerticallyAndNotConnected;

	UPROPERTY(EditAnywhere, Category="Clipboard")
	bool GenerateNodesFromTextsAndCopyToClipboard;

	//Resets every parameter in graphs that is titled "Duration" to the GenerateDefaultDuration value
	UPROPERTY(EditAnywhere, Category="Reset")
	bool ResetTimeInAllNodes;

	//Unlink all the string tables
	UPROPERTY(EditAnywhere, Category="Reset")
	bool UnlinkStringTablesInAllNodes;

	//Are you sure you want to destroy all that stuff in string tables. Make sure other blueprints don't use the same string table!!!!
	UPROPERTY(EditAnywhere, Category="Reset", meta=(EditCondition="AreYouSure"))
	bool DestroyEntriesInStringTable;

	//Are you sure you want to destroy all that stuff in string tables. Make sure other blueprints don't use the same string table!!!!
	UPROPERTY(EditAnywhere, Category="Reset", meta=(InlineEditConditionToggle))
	bool AreYouSure;

#endif //
};

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE TSubclassOf<class UDialogue> UDialogue::LoadDialogue(TSoftClassPtr<class UDialogue> InClass)
{
	return InClass.IsPending() ? InClass.LoadSynchronous() : InClass.Get();
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::AddContextTarget(FGameplayTag InTag, int32 InValue)
{
	return AddContext(InTag, GetTargetActorTag(), InValue);
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::IncrementContextTarget(FGameplayTag InTag, int32 InValue, bool AddIfMissing)
{
	return IncrementContext(InTag, GetTargetActorTag(), InValue, AddIfMissing);
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::RemoveContextTarget(FGameplayTag InTag)
{
	return RemoveContext(InTag, GetTargetActorTag());
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::HasContextTarget(FGameplayTag InTag) const
{
	return HasContext(InTag, GetTargetActorTag());
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE int32 UDialogue::GetContextTarget(FGameplayTag InTag) const
{
	return GetContext(InTag, GetTargetActorTag());
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::AddGlobalContext(FGameplayTag InTag, int32 InValue)
{
	return AddContext(InTag, FGameplayTag::EmptyTag, InValue);
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::IncrementGlobalContext(FGameplayTag InTag, int32 InValue, bool AddIfMissing)
{
	return IncrementContext(InTag, FGameplayTag::EmptyTag, InValue, AddIfMissing);
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::RemoveGlobalContext(FGameplayTag InTag)
{
	return RemoveContext(InTag, FGameplayTag::EmptyTag);
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::HasGlobalContext(FGameplayTag InTag) const
{
	return HasContext(InTag, FGameplayTag::EmptyTag);
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE int32 UDialogue::GetGlobalContext(FGameplayTag InTag) const
{
	return GetContext(InTag, FGameplayTag::EmptyTag);
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::RemoveAllContextForTarget()
{
	return RemoveAllContextFor(GetTargetActorTag());
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::RemoveAllGlobalContext()
{
	return RemoveAllContextFor(FGameplayTag::EmptyTag);
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::GetChoicesLimited(TArray<int32> &OutChoices, int32 &OutMaxChoices)
{
	return GetChoicesLimited_Internal(&OutChoices, OutMaxChoices);
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::HasChoices() const 
{ 
	return Choices.Num() > 0; 
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE class UDataAsset* UDialogue::GetChoiceAsset(int32 Index) const
{
	return Choices.GetData()[Index].ChoiceAsset;
}

//===================================================================================================
// 
//===================================================================================================
FORCEINLINE bool UDialogue::GetAllActors(TArray<class AActor*>& OutActors)
{
	return GetAllActorsConst(OutActors);
}