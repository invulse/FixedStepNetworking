// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FixedStepProjectile.h"
#include "FixedStepPredictedProjectile.generated.h"

/**
 * 
 */
UCLASS()
class FIXEDSTEP_API AFixedStepPredictedProjectile : public AFixedStepProjectile
{
	GENERATED_BODY()

public:
	
	AFixedStepPredictedProjectile();

	virtual void BeginPlay() override;
	
	virtual void SetFrame(int32 FrameNum) override;
	virtual void Simulate(int32 FrameNum, float DeltaTime, bool bRollback) override;
	virtual void RollbackStart(int32 FrameNum) override;
	virtual void Interpolate(int32 FromFrameNum, int32 ToFrameNum, float Alpha, float DeltaTime) override;
		
	void SetProjectileActive(bool bInActive, bool bForce = false);

	UFUNCTION(BlueprintImplementableEvent, Category = Projectile)
	void OnProjectileActive();
	
	UFUNCTION(BlueprintImplementableEvent, Category = Projectile)
	void OnProjectileInactive();
	
protected:

	bool bInternalActive;

	ECollisionEnabled::Type InitCollisionFlags;
};
