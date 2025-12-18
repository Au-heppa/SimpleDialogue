// Copyright Tero "Au-heppa" Knuutinen 2024.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DialogueInspectorAsset.generated.h"

//==============================================================================================================
// Editor only asset for inspecting string tables
//==============================================================================================================
UCLASS()
class SIMPLEDIALOGUE_API UDialogueInspectorAsset : public UObject
{
	GENERATED_BODY()
	
public:

	//
	UPROPERTY(EditAnywhere, Category = "Dialogue Script")
	TSubclassOf<class UDialogue> DialogueScript = NULL;

	//
	UPROPERTY(EditAnywhere, Category = "Dialogue Script")
	bool ImportDialogueFromScript = false;

	//
	UPROPERTY(EditAnywhere, Category = "String Table")
	class UStringTable *StringTable = NULL;

	//
	UPROPERTY(EditAnywhere, Category = "String Table")
	class UStringTable *DefaultStringTable = NULL;
};
