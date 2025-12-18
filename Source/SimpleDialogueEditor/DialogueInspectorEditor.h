// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "Toolkits/IToolkit.h"
#include "Misc/NotifyHook.h"
#include "EditorUndoClient.h"
#include "IDetailsView.h"
#include "Tickable.h"
#include "AssetThumbnail.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

//===========================================================================================================================
//
//===========================================================================================================================
struct FTextInspectorData
{
	FText OldText;
	TSharedPtr<SHorizontalBox> HorizontalBox;
	TSharedPtr<STextBlock> SpeakerWidget;
	TSharedPtr<STextBlock> TextWidget;
	TSharedPtr<STextBlock> NamespaceWidget;
	TSharedPtr<STextBlock> KeyNameWidget;
	TSharedPtr<SButton> OpenButtonWidget;
};

//===========================================================================================================================
// Editor for dialogue inspector assets 
//===========================================================================================================================
class SIMPLEDIALOGUEEDITOR_API FDialogueInspectorEditor : public FAssetEditorToolkit, public FNotifyHook, public FTickableGameObject
{
public:
	~FDialogueInspectorEditor();
	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	// End of IToolkit interface

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	// End of FAssetEditorToolkit

	// FTickableGameObject Interface
	virtual bool IsTickableInEditor() const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	// End of FTickableGameObject Interface

	void InitInspectorEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UObject *PropData);

	class UObject *GetObject() const { return (class UObject*)PropBeingEdited; }

	void ShowObjectDetails(UObject* ObjectProperties);

	//
	void UpdatePropertyEditor();

	//
	FText GetEditorText() const { return EditorText; }
	void SetEditorText(const FText& Text, ETextCommit::Type Type, bool bInCommit);

	//===========================================================================================================================
	// 
	//===========================================================================================================================

	//
	void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged) override;

	void CheckImportText();

	void UpdateTextWidgets();

	void OnScrollChanged(float Value);
	void OnWidgetsScrollChanged(float Value);

	FReply OnClick(class UEdGraphPin *InPin);

private:

	TArray<FText> Texts;

	//===========================================================================================================================
	// 
	//===========================================================================================================================

protected:
	/** Called when "Save" is clicked for this asset */
	virtual void SaveAsset_Execute() override;

protected:
	TSharedPtr<FUICommandList> DialogueInspectorEditorCommands;

	TSharedPtr<SMultiLineEditableTextBox> TextEditor;
	TSharedPtr<SScrollBox> PropertyEditor;
	TSharedPtr<SScrollBox> TextWidgetsScroll;
	TArray<FTextInspectorData> TextWidgets;

	class UDialogueInspectorAsset *PropBeingEdited = NULL;

	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_TextBox(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Changes(const FSpawnTabArgs& Args);

	bool bDialogueInspectorChanged;

	FTextBlockStyle TextStyle;
	FTextBlockStyle MediumTextStyle;
	FTextBlockStyle SmallTextStyle;

	FText EditorText;

	FButtonStyle TranslucentButton;

	bool bChangingScroll = false;
};

