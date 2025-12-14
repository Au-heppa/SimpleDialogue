// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "UnrealClient.h"
#include "Misc/NotifyHook.h"

#include "IPropertyTypeCustomization.h"
#include "Editor/PropertyEditor/Public/IPropertyRowGenerator.h"

class IPropertyHandle;


//=================================================================
// 
//=================================================================
class FContextAndValueDetails : public IPropertyTypeCustomization, public FNotifyHook
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	SIMPLEDIALOGUEEDITOR_API static void CreateGeneratorForStruct(TSharedPtr< class IPropertyRowGenerator >& OutGenerator, class UScriptStruct* StructType, TSharedPtr<class IPropertyHandle> StructPropertyHandle);
	SIMPLEDIALOGUEEDITOR_API static void GetWidgetForProperty(TSharedPtr< class IPropertyRowGenerator >& OutGenerator, TSharedPtr<class IPropertyHandle> ThePropertyHandle, TSharedPtr<SWidget>& OutWidget);
	SIMPLEDIALOGUEEDITOR_API static void CreateStructCustom(TSharedPtr< class IPropertyRowGenerator >& OutGenerator, class UScriptStruct* StructType, TSharedPtr<class IPropertyHandle> StructPropertyHandle, TSharedPtr<class IPropertyHandle> PropertyHandle, TSharedPtr<SWidget>& OutWidget);
	
	/** IPropertyTypeCustomization interface */
	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	/** Respond to a selection change event from the combo box */
	void UpdateProperty();

	//
	bool CanEdit() const;

	//
	EVisibility IsVisible() const;

	//
	EVisibility IsVisibleInverted() const;

	FSlateColor GetBackgroundColor() const;

	//
	bool bIsRequirement;
	bool bIsDisallowed;
	bool bIsChange;

private:

	TSharedPtr<IPropertyHandle> PropertyHandle;
	TSharedPtr<IPropertyHandle> TagPropertyHandle;
	TSharedPtr<IPropertyHandle> ValuePropertyHandle;
	TSharedPtr<IPropertyHandle> UseValuePropertyHandle;

	/** property utils */
	class IPropertyUtilities* PropUtils;

	//
	/** Row generator applied on detailed object */
	TSharedPtr< class IPropertyRowGenerator > Generator;
};
