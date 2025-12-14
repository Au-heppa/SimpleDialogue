// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "ContextAndValueDetails.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Engine/GameViewportClient.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"
#include "Widgets/Text/STextBlock.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "IPropertyUtilities.h"
#include "Engine.h"
#include "Editor/PropertyEditor/Public/IPropertyRowGenerator.h"
#include "Editor/PropertyEditor/Public/IDetailTreeNode.h"
#include "GameplayTagContainer.h"
#include "Dialogue/DialogueContext.h"

#define LOCTEXT_NAMESPACE "MissionLevelSelectorDetails"

//===========================================================================================================================
// 
//===========================================================================================================================
TSharedRef<IPropertyTypeCustomization> FContextAndValueDetails::MakeInstance()
{
	return MakeShareable(new FContextAndValueDetails);
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FContextAndValueDetails::CreateGeneratorForStruct(TSharedPtr< class IPropertyRowGenerator >& OutGenerator, class UScriptStruct* StructType, TSharedPtr<class IPropertyHandle> StructPropertyHandle)
{
	if (OutGenerator.IsValid())
		return;

	void* Data = NULL;
	StructPropertyHandle->GetValueData(Data);
	TSharedPtr<FStructOnScope> StructOnScope = MakeShareable(new FStructOnScope(StructType, (uint8*)Data));

	//Create property editor
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FPropertyRowGeneratorArgs Args;
	Args.bAllowMultipleTopLevelObjects = false;
	Args.bShouldShowHiddenProperties = false;
	Args.bAllowEditingClassDefaultObjects = false;

	OutGenerator = PropertyEditorModule.CreatePropertyRowGenerator(Args);
	OutGenerator->SetStructure(StructOnScope);
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FContextAndValueDetails::GetWidgetForProperty(TSharedPtr< class IPropertyRowGenerator >& OutGenerator,  TSharedPtr<class IPropertyHandle> ThePropertyHandle, TSharedPtr<SWidget>& OutWidget)
{
	const TArray<TSharedRef<IDetailTreeNode>>& Nodes = OutGenerator->GetRootTreeNodes();

	TSharedPtr<IDetailTreeNode> Node;

	int32 iTotal = 0;

	//
	for (int32 j = 0; j < Nodes.Num(); j++)
	{
		TArray<TSharedRef<IDetailTreeNode>> Children;
		Nodes.GetData()[j]->GetChildren(Children);

		iTotal++;

		TSharedPtr<IPropertyHandle> FirstPropertyHandle = Nodes.GetData()[j]->CreatePropertyHandle();
		if (FirstPropertyHandle.IsValid() && FirstPropertyHandle->GetProperty() == ThePropertyHandle->GetProperty())
		{
			Node = Nodes.GetData()[j];
			break;
		}

		for (int32 k = 0; k < Children.Num(); k++)
		{
			iTotal++;

			TSharedPtr<IPropertyHandle> NewPropertyHandle = Children.GetData()[k]->CreatePropertyHandle();
			if (NewPropertyHandle.IsValid() && NewPropertyHandle->GetProperty() == ThePropertyHandle->GetProperty())
			{
				Node = Children.GetData()[k];
				break;
			}
		}

		if (Node.IsValid())
			break;
	}

	if (Node.IsValid())
	{
		FNodeWidgets Widgets = Node->CreateNodeWidgets();
		OutWidget = Widgets.ValueWidget;
	}
	else
	{
		UE_LOG(LogTemp, Fatal, TEXT("Failed to get node from %d children and %d total!"), Nodes.Num(), iTotal);
	}
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FContextAndValueDetails::CreateStructCustom(	TSharedPtr< class IPropertyRowGenerator >& OutGenerator,
													class UScriptStruct* StructType,
													TSharedPtr<class IPropertyHandle> StructPropertyHandle,
													TSharedPtr<class IPropertyHandle> ThePropertyHandle,
													TSharedPtr<SWidget>& OutWidget)
{
	CreateGeneratorForStruct(OutGenerator, StructType, StructPropertyHandle);
	GetWidgetForProperty(OutGenerator, ThePropertyHandle, OutWidget);
}


//===========================================================================================================================
// 
//===========================================================================================================================
void FContextAndValueDetails::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	PropertyHandle = StructPropertyHandle;

	if (!PropertyHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to get property handle!"));
		return;
	}

	TagPropertyHandle = StructPropertyHandle->GetChildHandle(TEXT("Tag"));
	if (!TagPropertyHandle.IsValid())
	{

		UE_LOG(LogTemp, Warning, TEXT("Failed to get \"Tag\" property handle!"));
		return;
	}

	ValuePropertyHandle = StructPropertyHandle->GetChildHandle(TEXT("Value"));
	if (!ValuePropertyHandle.IsValid())
	{

		UE_LOG(LogTemp, Warning, TEXT("Failed to get \"Value\" property handle!"));
		return;
	}

	UseValuePropertyHandle = StructPropertyHandle->GetChildHandle(TEXT("bUseValue"));
	if (!UseValuePropertyHandle.IsValid())
	{

		UE_LOG(LogTemp, Warning, TEXT("Failed to get \"bUseValue\" property handle!"));
		return;
	}

	TSharedPtr<SWidget> TagWidget;
	CreateStructCustom(Generator, FContextAndValue::StaticStruct(), StructPropertyHandle, TagPropertyHandle, TagWidget);
	if (!TagWidget.IsValid())
	{
		return;
	}

	FString HintString;

	FString PropertyName;
	FArrayProperty *pArray = CastField<FArrayProperty>(PropertyHandle->GetProperty()->GetOwnerProperty());
	if (pArray)
	{
		PropertyName = pArray->GetName();
	}
	else
	{
		PropertyName = PropertyHandle->GetProperty()->GetName();
	}

	bIsRequirement =	PropertyName.Contains(TEXT("Require")) || PropertyName.Contains(TEXT("Include"));
	bIsDisallowed	=	PropertyName.Contains(TEXT("Disallow")) || PropertyName.Contains(TEXT("Exclude"));
	bIsChange =			PropertyName.Contains(TEXT("Change")) ||
						PropertyName.Contains(TEXT("Add")) ||
						PropertyName.Contains(TEXT("Completion")) ||
						PropertyName.Contains(TEXT("Start")) ||
						PropertyName.Contains(TEXT("Set"));


	if (bIsRequirement)
	{
		HintString = TEXT("Require Context DOES exist only");
	}
	else if (bIsDisallowed)
	{
		HintString = TEXT("Require Context does NOT exist only");
	}
	else if (bIsChange)
	{
		HintString = TEXT("Only change value if it doesn't exist");
	}

	HeaderRow
	.NameContent()
	.VAlign(VAlign_Top)
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MaxDesiredWidth(0.0f)
	.MinDesiredWidth(125.0f)
	.VAlign(VAlign_Top)
	.HAlign(HAlign_Left)
	[
		SNew(SBorder)
		.BorderBackgroundColor(this, &FContextAndValueDetails::GetBackgroundColor)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				TagWidget.ToSharedRef()
			]

			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(2.0f)
			[
				UseValuePropertyHandle->CreatePropertyValueWidget(true)
			]

			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(FText::FromString(TEXT(" == ")))
				.Visibility(this, &FContextAndValueDetails::IsVisible)
			]

			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SBox)
				.MinDesiredWidth(100.0f)
				.IsEnabled(this, &FContextAndValueDetails::CanEdit)
				.Visibility(this, &FContextAndValueDetails::IsVisible)
				[
					ValuePropertyHandle->CreatePropertyValueWidget(true)
				]
			]

			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(FText::FromString(HintString))
				.Visibility(this, &FContextAndValueDetails::IsVisibleInverted)
			]
		]
	];	
}

//===========================================================================================================================
// 
//===========================================================================================================================
FSlateColor FContextAndValueDetails::GetBackgroundColor() const
{
	static const FSlateColor GreenColor = FLinearColor(0, 1, 0, 0.2);
	static const FSlateColor RedColor = FLinearColor(1, 0, 0, 0.2);
	static const FSlateColor BlueColor = FLinearColor(0, 1, 1, 0.2);
	static const FSlateColor Transparent = FLinearColor(0,0,0,0);

	if (!CanEdit())
		return Transparent;

	if (bIsRequirement)
		return GreenColor;

	if (bIsDisallowed)
		return RedColor;

	if (bIsChange)
		return BlueColor;

	return Transparent;
}

//===========================================================================================================================
// 
//===========================================================================================================================
EVisibility FContextAndValueDetails::IsVisible() const
{
	bool bValue;
	UseValuePropertyHandle->GetValue(bValue);
	if (bValue)
		return EVisibility::Visible;

	if (bIsRequirement || bIsDisallowed)
		return EVisibility::Collapsed;

	if (bIsChange)
		return EVisibility::Visible;

	return EVisibility::Collapsed;
}

//===========================================================================================================================
// 
//===========================================================================================================================
EVisibility FContextAndValueDetails::IsVisibleInverted() const
{
	bool bValue;
	UseValuePropertyHandle->GetValue(bValue);
	if (bValue)
		return EVisibility::Collapsed;

	return EVisibility::Visible;
}

//===========================================================================================================================
// 
//===========================================================================================================================
bool FContextAndValueDetails::CanEdit() const
{
	if (!TagPropertyHandle.IsValid())
		return false;

	void* pValue = NULL;
	TagPropertyHandle->GetValueData(pValue);
	if (pValue == NULL)
		return false;

	FGameplayTag Tag = *((FGameplayTag*)pValue);
	return Tag.IsValid();
}

//===========================================================================================================================
// 
//===========================================================================================================================
void FContextAndValueDetails::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	
}


#undef LOCTEXT_NAMESPACE