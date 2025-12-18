// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "DialogueInspectorEditor.h"
#include "SimpleDialogueEditor.h"
#include "Toolkits/IToolkitHost.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "EdGraphUtilities.h"
#include "SNodePanel.h"
#include "SoundCueGraphEditorCommands.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/Commands/GenericCommands.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EditorSupportDelegates.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EdGraph/EdGraph.h"
#include "FileHelpers.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/Text/STextBlock.h"
#include "Engine.h"
#include "Widgets/Layout/SScrollBox.h"
#include "DetailLayoutBuilder.h"
#include "Widgets/Input/SEditableTextBox.h"

#include "Editor/PropertyEditor/Public/ISinglePropertyView.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "UnrealEd/Public/Subsystems/AssetEditorSubsystem.h"
#include "Misc/MessageDialog.h"
#include "Editor/EditorStyle/Public/EditorStyleSet.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Dialogue/DialogueInspectorAsset.h"
#include "Dialogue/Dialogue.h"
#include "Runtime/Engine/Public/Internationalization/StringTable.h"
#include "Editor/Kismet/Public/BlueprintEditorModule.h"
#include "EdGraph/EdGraphPin.h"

#define LOCTEXT_NAMESPACE "FDialogueInspectorEditor" 

//===========================================================================================================================
// 
//===========================================================================================================================
const FName InspectorEditorAppName = FName(TEXT("SimpleDialogueInspectorApp"));

//===========================================================================================================================
// 
//===========================================================================================================================
struct FDialogueInspectorEditorTabs
{
	static const FName TextBoxID;
	static const FName ChangesID;
	static const FName DetailsID;
};

//===========================================================================================================================
// 
//===========================================================================================================================
const FName FDialogueInspectorEditorTabs::TextBoxID(TEXT("TextBox"));
const FName FDialogueInspectorEditorTabs::ChangesID(TEXT("Changes"));
const FName FDialogueInspectorEditorTabs::DetailsID(TEXT("Details"));

//===========================================================================================================================
// 
//===========================================================================================================================
FName FDialogueInspectorEditor::GetToolkitFName() const
{
	return FName("Simple Dialogue Inspector Editor");
}

//===========================================================================================================================
// 
//===========================================================================================================================
FText FDialogueInspectorEditor::GetBaseToolkitName() const
{
	return LOCTEXT("DialogueInspectorEditorAppLabel", "Dialogue Inspector Editor");
}

//===========================================================================================================================
// 
//===========================================================================================================================
FText FDialogueInspectorEditor::GetToolkitName() const
{
	const bool bDirtyState = PropBeingEdited->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("InspectorAssetName"), FText::FromString(PropBeingEdited->GetName()));
	return FText::Format(LOCTEXT("DialogueInspectorEditorAppLabel", "{InspectorAssetName}"), Args);
}

//===========================================================================================================================
// 
//===========================================================================================================================
FString FDialogueInspectorEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("DialogueInspectorEditor");
}

//===========================================================================================================================
// 
//===========================================================================================================================
FLinearColor FDialogueInspectorEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::UpdatePropertyEditor()
{
	//
	PropertyEditor->ClearChildren();

	//FMatrixAssetEditorEdMode* EdMode = FAssetEditorToolkit::GetEditorMode();
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs; //(false, false, false, FDetailsViewArgs::HideNameArea, true, this);
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.NotifyHook = this;

	TSharedPtr<IDetailsView> PropertyEditorRef = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	PropertyEditorRef->SetObject(PropBeingEdited);

	PropertyEditor->AddSlot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Fill)
		.Padding(2.0f)
		[
			PropertyEditorRef.ToSharedRef()
		];
}

//===========================================================================================================================
// 
//===========================================================================================================================
TSharedRef<SDockTab> FDialogueInspectorEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	PropertyEditor = SNew(SScrollBox).OnUserScrolled(this, &FDialogueInspectorEditor::OnWidgetsScrollChanged);

	UpdatePropertyEditor();

	// Spawn the tab
	return 
	SNew(SDockTab)
	.Label(LOCTEXT("DetailsTab_Title", "Details"))
	[
		PropertyEditor.ToSharedRef()
	];
}

//===========================================================================================================================
// 
//===========================================================================================================================
TSharedRef<SDockTab> FDialogueInspectorEditor::SpawnTab_TextBox(const FSpawnTabArgs& Args)
{
	TextEditor = SNew(SMultiLineEditableTextBox)
		.BackgroundColor(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)))
		.Text(this, &FDialogueInspectorEditor::GetEditorText)
		.Font(TextStyle.Font)
		.OnTextChanged(this, &FDialogueInspectorEditor::SetEditorText, ETextCommit::Default, false)
		.OnVScrollBarUserScrolled(this, &FDialogueInspectorEditor::OnScrollChanged);

	// Spawn the tab
	return
	SNew(SDockTab)
	.Label(LOCTEXT("TextBoxTab_Title", "Text Box"))
	[
		TextEditor.ToSharedRef()
	];
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::OnScrollChanged(float Value)
{
	if (!bChangingScroll && TextWidgetsScroll.IsValid() && TextEditor.IsValid())
	{
		TSharedPtr<const SScrollBar> Bar = TextEditor->GetVScrollBar();

		float flNewValue = Bar->DistanceFromTop() / (Bar->DistanceFromTop() + Bar->DistanceFromBottom());

		//UE_LOG(LogTemp, Display, TEXT("Value: %.2f New value %.2f"), Value, flNewValue);

		bChangingScroll = true;
		TextWidgetsScroll->SetScrollOffset(flNewValue * TextWidgetsScroll->GetScrollOffsetOfEnd());
		bChangingScroll = false;
	}
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::OnWidgetsScrollChanged(float Value)
{
	if (!bChangingScroll && TextWidgetsScroll.IsValid() && TextEditor.IsValid())
	{
		float flNewValue = TextWidgetsScroll->GetScrollOffset() / TextWidgetsScroll->GetScrollOffsetOfEnd();

		UE_LOG(LogTemp, Display, TEXT("Value: %.2f"), flNewValue);

		bChangingScroll = true;
		//TextEditor->GetVScrollBar()->SetState(flNewValue, 0.0f);
		bChangingScroll = false;

		//TextWidgetsScroll->SetScrollOffset(flNewValue * TextWidgetsScroll->GetScrollOffsetOfEnd());
	}
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::SetEditorText(const FText& Text, ETextCommit::Type Type, bool bInCommit)
{
	EditorText = Text;

	if (!PropBeingEdited)
		return;

	TArray<FString> Strings;
	Text.ToString().ParseIntoArray(Strings, TEXT("\n"), true);

	TArray<int32> NewIndices;
	TArray<int32> OldIndices;
	NewIndices.Reserve(Strings.Num());
	OldIndices.Reserve(Texts.Num());

	TArray<FString> OldStrings;
	OldStrings.SetNum(Texts.Num());
	for (int32 i=0; i<Texts.Num(); i++)
	{
		OldStrings.GetData()[i] = Texts.GetData()[i].ToString();
		OldIndices.Add(i);
	}

	for (int32 i=0; i<Strings.Num(); i++)
	{
		Strings.GetData()[i].RemoveFromEnd(TEXT("\n"));
		Strings.GetData()[i].RemoveFromEnd(TEXT("\r"));

		FString Line = Strings.GetData()[i];
		FString Speaker;
		FString CustomName;
		FString Expression;
		FString PreviousSpeaker;
		UDialogue::ParseLine(Line, Strings.GetData()[i], Speaker, CustomName, Expression, PreviousSpeaker);

		NewIndices.Add(i);
	}

	TArray<FText> NewTexts;
	NewTexts.SetNum(Strings.Num());
	for (int32 i=0; i<Strings.Num(); i++)
	{
		NewTexts.GetData()[i] = FText::FromString(Strings.GetData()[i]);

		bool bFound = false;
		for (int32 j=0; j< OldStrings.Num(); j++)
		{
			if (!OldIndices.Contains(j))
				continue;

			if (Strings.GetData()[i].Equals(OldStrings.GetData()[j], ESearchCase::CaseSensitive))
			{
				bFound = true;
				NewTexts.GetData()[i] = Texts.GetData()[j];

				NewIndices.Remove(i);
				OldIndices.Remove(j);
				break;
			}
		}

		if (bFound)
			continue;

		FString FakeKey;

		if (UDialogue::FixTextToUseStringTable(NewTexts.GetData()[i], FakeKey, PropBeingEdited->DefaultStringTable, true))
		{
			NewIndices.Remove(i);
			continue;
		}

		if (UDialogue::FixTextToUseStringTable(NewTexts.GetData()[i], FakeKey, PropBeingEdited->StringTable, true))
		{
			NewIndices.Remove(i);
			continue;
		}
	}

	if (OldIndices.Num() > 0 && NewIndices.Num() > 0)
	{
		if (NewIndices.Num() == 1 && OldIndices.Num() == 1)
		{
			int32 iOld = OldIndices.GetData()[0];
			int32 iNew = NewIndices.GetData()[0];
			UE_LOG(LogTemp, Display, TEXT("From %d to %d. Updating text in stringtable from \"%s\" to \"%s\""), iOld, iNew, *Texts.GetData()[iOld].ToString(), *NewTexts.GetData()[iNew].ToString());
			UDialogue::ChangeTextInStringTable(PropBeingEdited->StringTable, Texts.GetData()[OldIndices.GetData()[0]], NewTexts.GetData()[NewIndices.GetData()[0]]);
		}
		else if (NewIndices.Num() == OldIndices.Num())
		{
			bool bAllIndicesMatch = true;
			for (int32 i=0; i<NewIndices.Num(); i++)
			{
				if (NewIndices.GetData()[i] != OldIndices.GetData()[i])
				{
					bAllIndicesMatch = false;
					break;
				}
			}
			
			if (bAllIndicesMatch)
			{
				for (int32 i = 0; i < NewIndices.Num(); i++)
				{
					int32 iIndex = NewIndices.GetData()[i];

					UE_LOG(LogTemp, Display, TEXT("Index %d. Updating text in stringtable from \"%s\" to \"%s\""), iIndex, *Texts.GetData()[iIndex].ToString(), *NewTexts.GetData()[iIndex].ToString());
					UDialogue::ChangeTextInStringTable(PropBeingEdited->StringTable, Texts.GetData()[iIndex], NewTexts.GetData()[iIndex]);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to update %d texts!"), NewIndices.Num());
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to update %d texts!"), NewIndices.Num());
		}
	}

	Texts = NewTexts;

	UpdateTextWidgets();
}

//===========================================================================================================================
// 
//===========================================================================================================================
TSharedRef<SDockTab> FDialogueInspectorEditor::SpawnTab_Changes(const FSpawnTabArgs& Args)
{
	TextWidgetsScroll = SNew(SScrollBox);

	// Spawn the tab
	return
		SNew(SDockTab)
		.Label(LOCTEXT("ChangesTab_Title", "Changes"))
		[
			TextWidgetsScroll.ToSharedRef()
		];
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::UpdateTextWidgets()
{
	if (TextWidgets.Num() != Texts.Num())
	{
		TextWidgetsScroll->ClearChildren();

		//Destroy excess ones
		while (TextWidgets.Num() > Texts.Num())
		{
			TextWidgets.RemoveAt(TextWidgets.Num()-1);
		}

		//Create new ones
		while (TextWidgets.Num() < Texts.Num())
		{
			FTextInspectorData NewData;
			NewData.TextWidget = SNew(STextBlock).TextStyle(&MediumTextStyle);
			NewData.SpeakerWidget = SNew(STextBlock).TextStyle(&MediumTextStyle);
			NewData.KeyNameWidget = SNew(STextBlock).TextStyle(&SmallTextStyle);
			NewData.NamespaceWidget = SNew(STextBlock).TextStyle(&SmallTextStyle);
			NewData.OpenButtonWidget = SNew(SButton).Text(FText::FromString(TEXT("Open"))).TextStyle(&TextStyle);
			NewData.HorizontalBox = SNew(SHorizontalBox)

			//
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.Padding(5, 0)
			[
				NewData.OpenButtonWidget.ToSharedRef()
			]

			//
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				[
					SNew(SHorizontalBox)

					//
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					[
						NewData.SpeakerWidget.ToSharedRef()
					]

					//
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					[
						NewData.TextWidget.ToSharedRef()
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				[
					SNew(SHorizontalBox)

					//
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					[
						NewData.NamespaceWidget.ToSharedRef()
					]

					//
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					[
						SNew(STextBlock).Font(TextStyle.Font).Text(FText::FromString(TEXT(".")))
					]

					//
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					[
						NewData.KeyNameWidget.ToSharedRef()
					]
				]
			];

			TextWidgets.Add(NewData);
		}

		//Add them to the scroll box
		for (int32 i=0; i<TextWidgets.Num(); i++)
		{
			TextWidgetsScroll->AddSlot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Fill)
				.Padding(0.0f)
				[
					TextWidgets.GetData()[i].HorizontalBox.ToSharedRef()
				];
		}
	}

	for (int32 i=0; i<TextWidgets.Num(); i++)
	{
		FName TableId;
		FString Key;
		FTextInspector::GetTableIdAndKey(Texts.GetData()[i], TableId, Key);

		class UEdGraphPin *pPin = UDialogue::FindPinWithText(PropBeingEdited->DialogueScript, Texts.GetData()[i]);

		FString Speaker;
		FString CustomName;

		if (pPin && pPin->GetOwningNode())
		{
			//Go through pins in node
			for (int32 k = 0; k < pPin->GetOwningNode()->Pins.Num(); k++)
			{
				//
				class UEdGraphPin* pOtherPin = pPin->GetOwningNode()->Pins.GetData()[k];
				if (!pOtherPin)
					continue;

				//
				if (pOtherPin->LinkedTo.Num() > 0)
					continue;

				if (pOtherPin->Direction != EEdGraphPinDirection::EGPD_Input)
					continue;

				static const FName Name_Speaker = TEXT("Speaker");
				if (pOtherPin->PinName == Name_Speaker)
				{
					Speaker = pOtherPin->DefaultValue;
				}

				static const FName Name_CustomName = TEXT("InCustomName");
				if (pOtherPin->PinName == Name_CustomName)
				{
					//CustomName = pOtherPin->DefaultValue;

					if (pOtherPin->DefaultValue.Split(TEXT("."), NULL, &CustomName, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
					{
						CustomName.Split(TEXT("\""), &CustomName, NULL, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
					}
				}
			}
		}

		if (Speaker == TEXT("Custom"))
		{
			TextWidgets.GetData()[i].SpeakerWidget->SetText(FText::FromString(FString::Printf(TEXT("%s: "), *CustomName)));
		}
		else if (Speaker.Len() > 0)
		{
			TextWidgets.GetData()[i].SpeakerWidget->SetText(FText::FromString(FString::Printf(TEXT("%s: "), *Speaker)));
		}
		else
		{
			TextWidgets.GetData()[i].SpeakerWidget->SetText(FText::GetEmpty());
		}

		TextWidgets.GetData()[i].OpenButtonWidget->SetOnClicked(FOnClicked::CreateSP(this, &FDialogueInspectorEditor::OnClick, pPin));
		TextWidgets.GetData()[i].OpenButtonWidget->SetEnabled(pPin != NULL);

		TextWidgets.GetData()[i].TextWidget->SetText(Texts.GetData()[i]);

		TextWidgets.GetData()[i].NamespaceWidget->SetText(FText::FromName(TableId));
		TextWidgets.GetData()[i].KeyNameWidget->SetText(FText::FromString(Key));
	}
}

//===========================================================================================================================
// 
//===========================================================================================================================
FReply FDialogueInspectorEditor::OnClick(class UEdGraphPin* InPin)
{
	//UE_LOG(LogTemp, Error, TEXT("Pin %s"), InPin != NULL ? *InPin->GetName() : TEXT("NULL"));

	if (InPin != NULL)
	{
		static const FName BpEditorModuleName("Kismet");
		FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>(BpEditorModuleName);

		//If object, then check if blueprint object
		class UBlueprint* pBlueprint = Cast<UBlueprint>(PropBeingEdited->DialogueScript->ClassGeneratedBy);
		if (pBlueprint)
		{
			TSharedRef< IBlueprintEditor > NewKismetEditor = BlueprintEditorModule.CreateBlueprintEditor(EToolkitMode::Standalone, false, pBlueprint, false);
			NewKismetEditor->JumpToPin(InPin);
		}
	}

	return FReply::Handled();
}

//===========================================================================================================================
// 
//===========================================================================================================================
bool FDialogueInspectorEditor::IsTickableInEditor() const
{
	return false;
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::Tick(float DeltaTime)
{
	if (bDialogueInspectorChanged)
	{
		bDialogueInspectorChanged = false;
	}
}

//===========================================================================================================================
// 
//===========================================================================================================================
bool FDialogueInspectorEditor::IsTickable() const
{
	return true;
}

//===========================================================================================================================
// 
//===========================================================================================================================
TStatId FDialogueInspectorEditor::GetStatId() const
{
	return TStatId();
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	CheckImportText();
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::CheckImportText()
{
	if (!IsValid(PropBeingEdited))
		return;

	const class UDialogue* pDialogue = PropBeingEdited->DialogueScript != NULL ? PropBeingEdited->DialogueScript->GetDefaultObject<UDialogue>() : NULL;
	if (!IsValid(pDialogue))
		return;

	PropBeingEdited->StringTable = pDialogue->GetStringTable().IsPending() ? pDialogue->GetStringTable().LoadSynchronous() : pDialogue->GetStringTable().Get();
	PropBeingEdited->DefaultStringTable = pDialogue->GetDefaultStringTable() ? pDialogue->GetDefaultStringTable().LoadSynchronous() : pDialogue->GetDefaultStringTable().Get();

	if (!PropBeingEdited->ImportDialogueFromScript)
		return;

	UDialogue::GatherAllTexts(PropBeingEdited->DialogueScript, Texts);

	//UE_LOG(LogTemp, Error, TEXT("Number of texts: %d"), Texts.Num());

	FString String;
	for (int32 i = 0; i < Texts.Num(); i++)
	{
		String += Texts.GetData()[i].ToString() + TEXT("\r\n");
	}

	EditorText = FText::FromString(String);

	PropBeingEdited->ImportDialogueFromScript = false;

	UpdateTextWidgets();
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged)
{
	CheckImportText();
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::SaveAsset_Execute()
{
	UPackage* Package = PropBeingEdited->GetOutermost();
	if (Package)
	{
		TArray<UPackage*> PackagesToSave;
		PackagesToSave.Add(Package);
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
	}
}

//===========================================================================================================================
// 
//===========================================================================================================================
FDialogueInspectorEditor::~FDialogueInspectorEditor()
{

}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManagerReg)
{
	WorkspaceMenuCategory = TabManagerReg->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_DialogueInspectorEditor", "Dialogue Inspector Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(TabManagerReg);

	TabManagerReg->RegisterTabSpawner(FDialogueInspectorEditorTabs::TextBoxID, FOnSpawnTab::CreateSP(this, &FDialogueInspectorEditor::SpawnTab_TextBox))
		.SetDisplayName(LOCTEXT("TextBoxTabLabel", "Text Box"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.TextBox"));

	TabManagerReg->RegisterTabSpawner(FDialogueInspectorEditorTabs::ChangesID, FOnSpawnTab::CreateSP(this, &FDialogueInspectorEditor::SpawnTab_Changes))
		.SetDisplayName(LOCTEXT("ChangesTabLabel", "Changes"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Changes"));

	TabManagerReg->RegisterTabSpawner(FDialogueInspectorEditorTabs::DetailsID, FOnSpawnTab::CreateSP(this, &FDialogueInspectorEditor::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTabLabel", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManagerReg)
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManagerReg);

	TabManagerReg->UnregisterTabSpawner(FDialogueInspectorEditorTabs::TextBoxID);
	TabManagerReg->UnregisterTabSpawner(FDialogueInspectorEditorTabs::ChangesID);
	TabManagerReg->UnregisterTabSpawner(FDialogueInspectorEditorTabs::DetailsID);
}

#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush(Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::InitInspectorEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject *PropData)
{
	TextStyle = FTextBlockStyle()
	.SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14))
	.SetColorAndOpacity(FLinearColor::White)
	.SetShadowOffset(FVector2D::ZeroVector)
	.SetShadowColorAndOpacity(FLinearColor::Black)
	.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f));

	MediumTextStyle = FTextBlockStyle()
		.SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11))
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black)
		.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f));

	SmallTextStyle = FTextBlockStyle()
		.SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		.SetColorAndOpacity(FLinearColor(0.8,0.8,0.8,1.0))
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black)
		.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f));

	//.SetHighlightShape(BOX_BRUSH("Common/TextBlockHighlightShape", FMargin(3.f /8.f)));

	FSlateBrush Translucent;
	Translucent.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));

	TranslucentButton = FButtonStyle()
		.SetNormal(Translucent)
		.SetHovered(Translucent)
		.SetPressed(Translucent)
		.SetNormalPadding(FMargin(0.0f))
		.SetPressedPadding(FMargin(0.0f));

	// Initialize the asset editor and spawn nothing (dummy layout)
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseOtherEditors(PropData, this);

	PropBeingEdited = Cast<UDialogueInspectorAsset>(PropData);

	// Default layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("DialogueInspectorEditor_Layout_v3")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->SetSizeCoefficient(0.8f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.35f)
					->SetHideTabWell(true)
					->AddTab(FDialogueInspectorEditorTabs::TextBoxID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.45f)
					->SetHideTabWell(true)
					->AddTab(FDialogueInspectorEditorTabs::ChangesID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->SetHideTabWell(true)
					->AddTab(FDialogueInspectorEditorTabs::DetailsID, ETabState::OpenedTab)
				)
			)
		);

	// Initialize the asset editor and spawn nothing (dummy layout)
	InitAssetEditor(Mode, InitToolkitHost, InspectorEditorAppName, StandaloneDefaultLayout, /*bCreateDefaultStandaloneMenu=*/ true, /*bCreateDefaultToolbar=*/ true, PropData);

	bDialogueInspectorChanged = true;

	ShowObjectDetails(PropBeingEdited);
	CheckImportText();
	UpdateTextWidgets();
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FDialogueInspectorEditor::ShowObjectDetails(UObject* ObjectProperties)
{
	PropBeingEdited = Cast<UDialogueInspectorAsset>(ObjectProperties);
	UpdatePropertyEditor();
}

#undef LOCTEXT_NAMESPACE

#undef BOX_BRUSH