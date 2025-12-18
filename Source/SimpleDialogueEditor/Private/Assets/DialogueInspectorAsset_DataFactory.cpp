// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "DialogueInspectorAsset_DataFactory.h"
#include "Dialogue/DialogueInspectorAsset.h"

//===========================================================================================================================
// 
//===========================================================================================================================
UDialogueInspectorAsset_DataFactory::UDialogueInspectorAsset_DataFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SupportedClass = UDialogueInspectorAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

//===========================================================================================================================
// 
//===========================================================================================================================
bool UDialogueInspectorAsset_DataFactory::CanCreateNew() const
{
	return true;
}

//===========================================================================================================================
// 
//===========================================================================================================================
UObject *UDialogueInspectorAsset_DataFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UDialogueInspectorAsset*NewAsset = NewObject<UDialogueInspectorAsset>(InParent, Class, Name, Flags | RF_Transactional);
	return NewAsset;
}
