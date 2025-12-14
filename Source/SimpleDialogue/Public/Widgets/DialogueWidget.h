// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DialogueWidget.generated.h"

//==============================================================================================================
//
//==============================================================================================================
UCLASS(Blueprintable)
class SIMPLEDIALOGUE_API UDialogueWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	//
	virtual void NativeDestruct() override;

	//
	UFUNCTION(BlueprintCallable)
	virtual void Setup(class UDialogueManager *InManager);

	//
	UFUNCTION(BlueprintNativeEvent)
	void OnSetup();
	virtual void OnSetup_Implementation() { }

	//
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdate();
	virtual void OnUpdate_Implementation() { }

	//
	FORCEINLINE class UDialogueManager *GetManager() const { return Manager.Get(); }

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true), Category="Runtime")
	TWeakObjectPtr<class UDialogueManager> Manager;
};
