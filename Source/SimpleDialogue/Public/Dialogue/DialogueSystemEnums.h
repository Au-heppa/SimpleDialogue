// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

//===============================================================================================================================
// 
//===============================================================================================================================
UENUM(BlueprintType)
enum class EDialogueEffect : uint8
{
	None					UMETA(DisplayName = "None"),
	Shake 					UMETA(DisplayName = "Shake"),
	Bend					UMETA(DisplayName = "Bend"),
	Blur					UMETA(DisplayName = "Blur"),
	Damage					UMETA(DisplayName = "Damage"),
};

//===============================================================================================================================
// 
//===============================================================================================================================
UENUM(BlueprintType)
enum class EDialogueExpression : uint8
{
	None					UMETA(DisplayName = "None"),
	Happy,
	Angry,
	Curious,
	Shy,
	Annoyed,
	Scared,
	Smug,
	Proud,
	Disgusted,
	Worried,
	Crying,
	Sad,
	Smirk,
};

//===============================================================================================================================
// 
//===============================================================================================================================
UENUM(BlueprintType)
enum class EDialogueSpeaker : uint8
{
	Player,
	Target,
	Narrator,
	Custom,
};

//===============================================================================================================================
// 
//===============================================================================================================================
UENUM(BlueprintType)
enum class EDialogueArray : uint8
{
	Start,
	Victory,
	Failure,
};