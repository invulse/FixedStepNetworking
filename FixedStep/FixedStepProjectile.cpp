// Fill out your copyright notice in the Description page of Project Settings.

#include "FixedStep.h"
#include "FixedStepProjectile.h"


// Sets default values
AFixedStepProjectile::AFixedStepProjectile()
{
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(CollisionComp);

	FixedStepComp = CreateDefaultSubobject<UFixedStepComponent>(TEXT("FixedStep"));

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(GetRootComponent());

	MovementComp = CreateDefaultSubobject<UFixedStepProjectileMovement>(TEXT("Movement"));
	
	bReplicateMovement = false;
	bReplicates = true;
}

// Called when the game starts or when spawned
void AFixedStepProjectile::BeginPlay()
{
	Super::BeginPlay();

	FixedStepComp->RegisterObserver(MovementComp);
	FixedStepComp->RegisterObserver(this);
}

void AFixedStepProjectile::PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha)
{
	VisualRoot->SetWorldLocationAndRotation(GetActorLocation() + MovementComp->VisualLocation, GetActorRotation().Quaternion() * MovementComp->VisualRotation.Quaternion());
}

void AFixedStepProjectile::Interpolate(int32 FromFrameNum, int32 ToFrameNum, float Alpha, float DeltaTime)
{
	VisualRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FQuat::Identity);
}

