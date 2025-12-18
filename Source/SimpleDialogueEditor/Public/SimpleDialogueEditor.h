// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Developer/AssetTools/Public/IAssetTools.h"

const FName SIMPLEDIALOGUE_MENU_CATEGORY_KEY(TEXT("SimpleDialogue"));
const FText SIMPLEDIALOGUE_MENU_CATEGORY_KEY_TEXT(NSLOCTEXT("SimpleDialogueEditor", "SimpleDialogueCategory", "Simple Dialogue"));

class FSimpleDialogueEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:
	// The menu for Mission asset
	EAssetTypeCategories::Type ObjectiveCategoryBit;

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action);

	/** All created asset type actions.  Cached here so that we can unregister them during shutdown. */
	TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;	
};
