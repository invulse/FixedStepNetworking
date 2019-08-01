// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/ProjectileMovementComponent.h"
#include "FixedStepComponent.h"
#include "FixedStepProjectileMovement.generated.h"

/**
 * 
 */
UCLASS()
class FIXEDSTEP_API UFixedStepProjectileMovement : public UProjectileMovementComponent, public IFixedStepListenerInterface
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY()
	FVector ReplicatedVelocity;

	UPROPERTY()
	FRotator ReplicatedRotation;

	UPROPERTY()
	FRotator ReplicatedRotationError;

	UPROPERTY()
	FRotator VisualRotation;

	UPROPERTY()
	FVector ReplicatedLocation;

	UPROPERTY()
	FVector ReplicatedLocationError;

	UPROPERTY()
	FVector VisualLocation;

	UPROPERTY()
	bool bProjectileActive;
		
	UFixedStepProjectileMovement();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	
	virtual void StartSimulating();
	virtual void StopSimulating();
	virtual void StopSimulating(const FHitResult& HitResult) override;
	
	virtual void Simulate(int32 FrameNum, float DeltaTime, bool bRollback) override;
	virtual void SetFrame(int32 FrameNum) override;
	virtual void Interpolate(int32 FromFrameNum, int32 ToFrameNum, float Alpha, float DeltaTime) override;

	virtual void RollbackStart(int32 FrameNum) override;
	virtual void RollbackComplete(int32 FrameNum) override;
	
	virtual void GetFixedStepCommands(TArray<FFixedStepCmd>& OutCmds);

	//Allow this component to ignore rollbacks before a certain frame number. It will ignore set frame and it will ignore simulation before this
	void SetIgnoreRollbacksBefore(int32 InMinRollbackFrame);

protected:

	UPROPERTY(BlueprintReadOnly)
	bool bInRollback;
};
