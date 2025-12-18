// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimpleDialogueEditor.h"
#include "Modules/ModuleManager.h"
#include "DetailCustomizations/ContextAndValueDetails.h"
#include "Assets/AssetTypeActions_DialogueInspectorAsset.h"

#define LOCTEXT_NAMESPACE "FSimpleDialogueEditorModule"

//==============================================================================================================
//
//==============================================================================================================
void FSimpleDialogueEditorModule::StartupModule()
{	
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();


	// Register asset category
	ObjectiveCategoryBit = AssetTools.RegisterAdvancedAssetCategory(SIMPLEDIALOGUE_MENU_CATEGORY_KEY, SIMPLEDIALOGUE_MENU_CATEGORY_KEY_TEXT);

	// Register asset types
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_DialogueInspectorAsset(ObjectiveCategoryBit)));

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
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
		}
	}
	CreatedAssetTypeActions.Empty();	
	
	// Unregister the details customization
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("ContextAndValue");
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

//==============================================================================================================
//
//==============================================================================================================
void FSimpleDialogueEditorModule::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSimpleDialogueEditorModule, SimpleDialogueEditor)