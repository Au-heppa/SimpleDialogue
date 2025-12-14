// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Widgets/DialogueWidget.h"
#include "Dialogue/DialogueManager.h"

//==============================================================================================================
//
//==============================================================================================================
void UDialogueWidget::Setup(class UDialogueManager *InManager)
{
	if (IsValid(InManager) && Manager.Get() != InManager)
	{
		Manager = InManager;

		InManager->OnDialogueUpdated.AddDynamic(this, &UDialogueWidget::OnUpdate);

		OnSetup();
		OnUpdate();
	}
}

//==============================================================================================================
//
//==============================================================================================================
void UDialogueWidget::NativeDestruct()
{
	if (IsValid(Manager.Get()))
	{
		Manager->ClearAllEvents(this);
	}

	Super::NativeDestruct();

	Manager = NULL;
}