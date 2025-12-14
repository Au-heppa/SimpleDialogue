// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "Dialogue/Dialogue.h"
#include "OneLineDialogue.generated.h"

//==============================================================================================================
//
//==============================================================================================================
UCLASS()
class SIMPLEDIALOGUE_API UOneLineDialogue : public UDialogue
{
	GENERATED_BODY()
	
public:

	//
	virtual void OnActivate_Implementation() override;

	//
	void Skip() override;


public:

	//
	UPROPERTY(VisibleAnywhere, Category="Runtime")
	FText Text;

	//
	UPROPERTY(VisibleAnywhere, Category="Runtime")
	EDialogueSpeaker Speaker;

	//
	UPROPERTY(VisibleAnywhere, Category="Runtime")
	EDialogueEffect Effect;

	//
	UPROPERTY(VisibleAnywhere, Category = "Runtime")
	EDialogueExpression Expression;

	//
	UPROPERTY(VisibleAnywhere, Category="Runtime")
	float Duration;
};
