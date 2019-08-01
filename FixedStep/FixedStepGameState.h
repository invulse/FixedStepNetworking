// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/GameState.h"
#include "FixedStepGameState.generated.h"

class UFixedStepManagerComponent;


/**
 * 
 */
UCLASS()
class FIXEDSTEP_API AFixedStepGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	AFixedStepGameState();

	UPROPERTY(EditAnywhere)
	UFixedStepManagerComponent* Manager;
	
};
