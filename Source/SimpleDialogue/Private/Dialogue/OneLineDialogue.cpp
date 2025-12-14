// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Dialogue/OneLineDialogue.h"

//==============================================================================================================
//
//==============================================================================================================
void UOneLineDialogue::OnActivate_Implementation()
{
	DialogueBoxNoLatent( Speaker, Text, Duration, Expression, Effect );
}

//==============================================================================================================
//
//==============================================================================================================
void UOneLineDialogue::Skip()
{
	Deactivate();
}