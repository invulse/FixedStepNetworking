// Fill out your copyright notice in the Description page of Project Settings.

#include "FixedStep.h"
#include "FixedStepGameState.h"
#include "FixedStepManagerComponent.h"

AFixedStepGameState::AFixedStepGameState()
{
	Manager = CreateDefaultSubobject<UFixedStepManagerComponent>(TEXT("Manager"));
}