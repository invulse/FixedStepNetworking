// Fill out your copyright notice in the Description page of Project Settings.

#include "FixedStep.h"
#include "FixedStepPredictedProjectile.h"

AFixedStepPredictedProjectile::AFixedStepPredictedProjectile()
{
	bInternalActive = false;
	MovementComp->bProjectileActive = false;
}

void AFixedStepPredictedProjectile::BeginPlay()
{
	Super::BeginPlay();
		
	InitCollisionFlags = CollisionComp->GetCollisionEnabled();
	SetProjectileActive(MovementComp->bProjectileActive, true);
}

void AFixedStepPredictedProjectile::SetFrame(int32 FrameNum)
{
	Super::SetFrame(FrameNum);

	//SetProjectileActive(MovementComp->bProjectileActive);
}

void AFixedStepPredictedProjectile::Simulate(int32 FrameNum, float DeltaTime, bool bRollback)
{
	SetProjectileActive(MovementComp->bProjectileActive);
	if (!bInternalActive)	return;
	
	Super::Simulate(FrameNum, DeltaTime, bRollback);
}

void AFixedStepPredictedProjectile::RollbackStart(int32 FrameNum)
{
}

void AFixedStepPredictedProjectile::Interpolate(int32 FromFrameNum, int32 ToFrameNum, float Alpha, float DeltaTime)
{
	SetProjectileActive(MovementComp->bProjectileActive);
	if (!bInternalActive)	return;

	Super::Interpolate(FromFrameNum, ToFrameNum, Alpha, DeltaTime);
}

void AFixedStepPredictedProjectile::SetProjectileActive(bool bInActive, bool bForce)
{
	if (bInternalActive == bInActive && !bForce) return;
	
	if (bInActive)
	{
		CollisionComp->SetCollisionEnabled(InitCollisionFlags);
		RootComponent->SetVisibility(true, true);
		OnProjectileActive();
	}
	else
	{
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RootComponent->SetVisibility(false, true);
		OnProjectileInactive();
	}
}

