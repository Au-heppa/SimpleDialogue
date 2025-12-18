#include "AssetTypeActions_DialogueInspectorAsset.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "../DialogueInspectorEditor.h"
#include "Dialogue/DialogueInspectorAsset.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_DialogueInspectorAsset"

//===========================================================================================================================
// 
//===========================================================================================================================
FText FAssetTypeActions_DialogueInspectorAsset::GetName() const
{
	return LOCTEXT("FAssetTypeActions_DialogueInspectorAsset", "Dialogue Inspector Asset");
}

//===========================================================================================================================
// 
//===========================================================================================================================
FColor FAssetTypeActions_DialogueInspectorAsset::GetTypeColor() const
{
	return FColor(128, 128, 128);
}

//===========================================================================================================================
// 
//===========================================================================================================================
UClass* FAssetTypeActions_DialogueInspectorAsset::GetSupportedClass() const
{
	return UDialogueInspectorAsset::StaticClass();
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FAssetTypeActions_DialogueInspectorAsset::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	//Get crime time editor module
	FSimpleDialogueEditorModule& ObjectivesEditorModule = FModuleManager::GetModuleChecked<FSimpleDialogueEditorModule>("SimpleDialogueEditor");

	//Get crime time editor module
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		UObject *PropData = *ObjIt;
		if (PropData) 
		{
			TSharedRef<FDialogueInspectorEditor> NewEventEditor(new FDialogueInspectorEditor());
			NewEventEditor->InitInspectorEditor(Mode, EditWithinLevelEditor, PropData);
		}
	}
}

#undef LOCTEXT_NAMESPACE
