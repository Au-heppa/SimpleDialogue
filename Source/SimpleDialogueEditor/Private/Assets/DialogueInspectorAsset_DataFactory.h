// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once
#include "Engine/DataAsset.h"
#include "Factories/Factory.h"
#include "DialogueInspectorAsset_DataFactory.generated.h"

//===========================================================================================================================
// 
//===========================================================================================================================
UCLASS()
class SIMPLEDIALOGUEEDITOR_API UDialogueInspectorAsset_DataFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	// End of UFactory interface
};