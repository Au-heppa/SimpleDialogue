// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "DialogueSystemEnums.h"
#include "DialogueChoice.generated.h"

//==============================================================================================================
// 
//==============================================================================================================
USTRUCT(BlueprintType)
struct FDialogueChoice
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly, VisibleAnywhere)
	FText Title = FText::GetEmpty();

	UPROPERTY(SaveGame, BlueprintReadOnly, VisibleAnywhere)
	bool Enabled = false;

	UPROPERTY(SaveGame, BlueprintReadOnly, VisibleAnywhere)
	FName ChoiceName = NAME_None;

	UPROPERTY(SaveGame, BlueprintReadOnly, VisibleAnywhere)
	int32 OriginalIndex = INDEX_NONE;

	UPROPERTY(SaveGame, BlueprintReadOnly, VisibleAnywhere)
	class UDataAsset *ChoiceAsset = NULL;

	UPROPERTY(SaveGame)
	FName ExecutionFunction = NAME_None;

	UPROPERTY(SaveGame)
	int32 OutputLink = INDEX_NONE;

	FORCEINLINE bool IsValidFunction() const { return !ExecutionFunction.IsNone() && OutputLink != INDEX_NONE; }
};

