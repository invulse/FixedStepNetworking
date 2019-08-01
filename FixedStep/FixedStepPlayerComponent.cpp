// Fill out your copyright notice in the Description page of Project Settings.

#include "FixedStep.h"
#include "FixedStepPlayerComponent.h"
#include "FixedStepManagerComponent.h"

#include "GameFramework/HUD.h"
#include "DisplayDebugHelpers.h"

UFixedStepPlayerComponent::UFixedStepPlayerComponent()
{
	Manager = NULL;
	PCOwner = NULL;
}

void UFixedStepPlayerComponent::BeginPlay()
{
	Super::BeginPlay();

	PCOwner = Cast<APlayerController>(GetOwner());
	ensure(PCOwner);

	//Search the world for manager	
	if (!IsInitialized())
	{
		for (FActorIterator ActorIt(GetWorld()); ActorIt; ++ActorIt)
		{
			AActor* Actor = *ActorIt;
			if (!Actor || Actor->IsPendingKill()) continue;

			for (UActorComponent* Comp : Actor->GetComponentsByClass(UFixedStepManagerComponent::StaticClass()))
			{
				UFixedStepManagerComponent* ManagerComp = Cast<UFixedStepManagerComponent>(Comp);
				if (ManagerComp)
				{
					ManagerComp->AddPlayerComponent(this);
				}
			}
		}
	}
}

void UFixedStepPlayerComponent::Init(UFixedStepManagerComponent* InManager)
{
	Manager = InManager;
}

