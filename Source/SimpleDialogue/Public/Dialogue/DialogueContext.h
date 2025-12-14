// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "DialogueSystemEnums.h"
#include "GameplayTagContainer.h"
#include "DialogueContext.generated.h"

//==============================================================================================================
// 
//==============================================================================================================
USTRUCT(BlueprintType)
struct FSavedContext
{
	GENERATED_USTRUCT_BODY()

	//
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FGameplayTag Tag;

	//
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	int32 Value = 0;

	//
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	FGameplayTag ActorTag;
};

//==============================================================================================================
// 
//==============================================================================================================
USTRUCT(BlueprintType, meta=(ShowOnlyInnerProperties=true))
struct FSavedContextMap
{
	GENERATED_USTRUCT_BODY()

	//
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TMap<FGameplayTag, int32> Values;
};

#define FContextMapType TMap<FGameplayTag, int32>

//==============================================================================================================
// 
//==============================================================================================================
USTRUCT(BlueprintType)
struct FContextAndValue
{
	GENERATED_USTRUCT_BODY()

public:

	//
	SIMPLEDIALOGUE_API bool CheckHasContext(class UDialogueManager *InManager, const FGameplayTag &InActorTag = FGameplayTag::EmptyTag) const;

	//
	SIMPLEDIALOGUE_API bool CheckDoesntHaveContext(class UDialogueManager* InManager, const FGameplayTag& InActorTag = FGameplayTag::EmptyTag) const;

	//
	SIMPLEDIALOGUE_API bool ChangeContext(class UDialogueManager* InManager, const FGameplayTag& InActorTag = FGameplayTag::EmptyTag) const;

	//
	SIMPLEDIALOGUE_API bool RemoveContext(class UDialogueManager* InManager, const FGameplayTag& InActorTag = FGameplayTag::EmptyTag) const;

	//
	FORCEINLINE bool Matches(const FContextAndValue &Other) const
	{
		return Tag.MatchesTagExact(Other.Tag) && bUseValue == Other.bUseValue && Value == Other.Value;
	}

	//
#if WITH_EDITOR
	FORCEINLINE const FString GetTagDebug() const { return Tag.ToString(); }

	FORCEINLINE int32 GetValueDebug() const { return Value; }
#endif //

	//
	FORCEINLINE const FGameplayTag &GetTag() const { return Tag; }
	FORCEINLINE int32 GetValue() const { return bUseValue ? Value : 1; }

private:

	//
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta=(AllowPrivateAccess=true, Categories="Context,Dialogue,Docks"))
	FGameplayTag Tag;

	//If should only check for existence / or should only change the existence of the tag and ignore what value it is
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess = true))
	bool bUseValue = true;

	//
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess = true))
	int32 Value = 1;
};