#pragma once

#include "AssetTypeActions_Base.h"

//===========================================================================================================================
// 
//===========================================================================================================================
class FAssetTypeActions_DialogueInspectorAsset : public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_DialogueInspectorAsset(EAssetTypeCategories::Type InAssetCategory) : AssetCategory(InAssetCategory) {}

	// IAssetTypeActions interface
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	uint32 GetCategories() override { return AssetCategory; }
	// End of IAssetTypeActions interface

private:
	/** Indicates the category used for Dialogues */
	EAssetTypeCategories::Type AssetCategory;
};

