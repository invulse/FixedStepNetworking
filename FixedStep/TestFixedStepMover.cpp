// Fill out your copyright notice in the Description page of Project Settings.

#include "FixedStep.h"
#include "TestFixedStepMover.h"
#include "FixedStepComponent.h"


// Sets default values
ATestFixedStepMover::ATestFixedStepMover()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	FixedStepComp = CreateDefaultSubobject<UFixedStepComponent>(TEXT("FixedStep"));
	bReplicates = true;
	bAlwaysRelevant = true;
}

// Called when the game starts or when spawned
void ATestFixedStepMover::BeginPlay()
{
	Super::BeginPlay();
	
	StateLocation = GetActorLocation();

	FixedStepComp->RegisterObserver(this);
}

void ATestFixedStepMover::GatherInputs(int32 FrameNum)
{

}

void ATestFixedStepMover::Simulate(int32 FrameNum, float DeltaTime, bool bRollback)
{
	if (Role == ROLE_Authority)
	{
		if (FrameNum % 100 == 0)
		{
			Input = FMath::VRand();
			Input.Z = 0;
		}
	}

	StateLocation += Input * Speed * DeltaTime;
	SetActorLocation(StateLocation);
}

void ATestFixedStepMover::PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha)
{

}

void ATestFixedStepMover::SetFrame(int32 FrameNum)
{
	SetActorLocation(StateLocation);
}

void ATestFixedStepMover::Interpolate(int32 FromFrame, int32 ToFrame, float Alpha, float DeltaTime)
{
	SetActorLocation(StateLocation);
}

void ATestFixedStepMover::RollbackStart(int32 FrameNum)
{

}

void ATestFixedStepMover::RollbackComplete(int32 FrameNum)
{

}

void ATestFixedStepMover::GetFixedStepCommands(TArray<FFixedStepCmd>& OutCmds)
{	
	FIXEDSTEPINPUT(ATestFixedStepMover, Input);
	FIXEDSTEPSTATE_ERROR(ATestFixedStepMover, StateLocation, StateLocationError);
}

// Called every frame
void ATestFixedStepMover::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

