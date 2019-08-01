// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "FixedStepComponent.h"
#include "FixedStepCharacterMovement.generated.h"

class AFixedStepCharacter;

/**
 * 
 */
UCLASS()
class FIXEDSTEP_API UFixedStepCharacterMovement : public UCharacterMovementComponent, public IFixedStepListenerInterface
{
	GENERATED_BODY()

public:

	UFixedStepCharacterMovement();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override {};
	virtual void SendClientAdjustment() override {};

	UFUNCTION(BlueprintCallable, Category = "Pawn|Components|CharacterMovement")
	AFixedStepCharacter* GetFixedStepCharacterOwner() const;

protected:

	UPROPERTY()
	FVector ReplicatedMovementInput;

	UPROPERTY()
	FVector ReplicatedLocation;

	UPROPERTY()
	FVector ReplicatedLocationError;

	UPROPERTY()
	FVector VisualLocation;

	UPROPERTY()
	FRotator ReplicatedRotation;

	UPROPERTY()
	FRotator ReplicatedRotationError;

	UPROPERTY()
	FRotator VisualRotation;

	UPROPERTY()
	FVector ReplicatedVelocity;
	
	UPROPERTY()
	FVector VisualVelocity;

	UPROPERTY()
	uint8 ReplicatedMovementMode;
	
	UFUNCTION()
	void GatherInputs(int32 FrameNum);

	UFUNCTION()
	void Simulate(int32 FrameNum, float DeltaTime, bool bRollback);
	
	UFUNCTION()
	void PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha);

	UFUNCTION()
	void SetFrame(int32 FrameNum);

	UFUNCTION()
	void Interpolate(int32 FromFrame, int32 ToFrame, float Alpha, float DeltaTime);

	UFUNCTION()
	void RollbackStart(int32 FrameNum);

	UFUNCTION()
	void RollbackComplete(int32 FrameNum);

	void GetFixedStepCommands(TArray<FFixedStepCmd>& OutCmds);
};
