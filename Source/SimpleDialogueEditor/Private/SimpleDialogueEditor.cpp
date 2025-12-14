// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimpleDialogueEditor.h"
#include "Modules/ModuleManager.h"
#include "DetailCustomizations/ContextAndValueDetails.h"

#define LOCTEXT_NAMESPACE "FSimpleDialogueEditorModule"

//==============================================================================================================
//
//==============================================================================================================
void FSimpleDialogueEditorModule::StartupModule()
{	
	// Register the details customizer
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout("ContextAndValue", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FContextAndValueDetails::MakeInstance));
	PropertyModule.NotifyCustomizationModuleChanged();
}

//==============================================================================================================
//
//==============================================================================================================
void FSimpleDialogueEditorModule::ShutdownModule()
{
	// Unregister the details customization
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("ContextAndValue");
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSimpleDialogueEditorModule, SimpleDialogueEditor)