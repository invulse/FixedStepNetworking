// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "FixedStepPlayerComponent.generated.h"

/**
 *  Currently unused, may consider using this later... 
 */

class UFixedStepManagerComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FIXEDSTEP_API UFixedStepPlayerComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	
	UFixedStepPlayerComponent();

	virtual void BeginPlay() override;

	virtual void Init(UFixedStepManagerComponent* InManager);


protected:

	UFixedStepManagerComponent* Manager;
	APlayerController* PCOwner;

public:
	FORCEINLINE bool IsInitialized() const { return Manager != nullptr; }



};
