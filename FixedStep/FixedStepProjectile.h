// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "FixedStepComponent.h"
#include "FixedStepProjectileMovement.h"
#include "FixedStepProjectile.generated.h"

UCLASS()
class FIXEDSTEP_API AFixedStepProjectile : public AActor, public IFixedStepListenerInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFixedStepProjectile();

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UFixedStepComponent* FixedStepComp;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UPrimitiveComponent* CollisionComp;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	USceneComponent* VisualRoot;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UFixedStepProjectileMovement* MovementComp;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:

	virtual void PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha) override;
	virtual void Interpolate(int32 FromFrameNum, int32 ToFrameNum, float Alpha, float DeltaTime) override;
	
};
