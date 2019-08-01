// Fill out your copyright notice in the Description page of Project Settings.

#include "FixedStep.h"
#include "TestFixedStepPawn.h"

// Sets default values
ATestFixedStepPawn::ATestFixedStepPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	FixedStepComponent = CreateDefaultSubobject<UFixedStepComponent>(TEXT("FixedStep"));
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(RootComponent);

	PendingInput = FVector::ZeroVector;
	Speed = 200.0f;
}

// Called when the game starts or when spawned
void ATestFixedStepPawn::BeginPlay()
{
	Super::BeginPlay();

	ReplicatedLocation = GetActorLocation();

	FixedStepComponent->RegisterObserver(this);
}

// Called every frame
void ATestFixedStepPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ATestFixedStepPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ATestFixedStepPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveSide", this, &ATestFixedStepPawn::MoveSide);

}

void ATestFixedStepPawn::MoveForward(float Value)
{
	PendingInput.X = Value;
}

void ATestFixedStepPawn::MoveSide(float Value)
{
	PendingInput.Y = Value;
}

void ATestFixedStepPawn::GatherInputs(int32 FrameNum)
{
	Input = PendingInput;
}

void ATestFixedStepPawn::Simulate(int32 FrameNum, float DeltaTime, bool bRollback)
{
	ReplicatedLocation += Input * Speed * DeltaTime;
	SetActorLocation(ReplicatedLocation);
	MeshComponent->SetWorldLocation(GetActorLocation() + RepLocationOffset);

	ReplicatedRotation = Input.Rotation();
	SetActorRotation(ReplicatedRotation);
	MeshComponent->SetWorldRotation(GetActorRotation().Quaternion() * RepRotationOffset.Quaternion());
}

void ATestFixedStepPawn::PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha)
{

}

void ATestFixedStepPawn::SetFrame(int32 FrameNum)
{
	SetActorLocation(ReplicatedLocation);
	MeshComponent->SetWorldLocation(GetActorLocation() + RepLocationOffset);

	SetActorRotation(ReplicatedRotation);
	MeshComponent->SetWorldRotation(GetActorRotation().Quaternion() * RepRotationOffset.Quaternion());
}

void ATestFixedStepPawn::Interpolate(int32 FromFrame, int32 ToFrame, float Alpha, float DeltaTime)
{

}

void ATestFixedStepPawn::RollbackStart(int32 FrameNum)
{

}

void ATestFixedStepPawn::RollbackComplete(int32 FrameNum)
{

}

void ATestFixedStepPawn::GetFixedStepCommands(TArray<FFixedStepCmd>& OutCmds)
{
	FIXEDSTEPINPUT(ATestFixedStepPawn, Input);
	FIXEDSTEPSTATE_ERROR(ATestFixedStepPawn, ReplicatedLocation, RepLocationOffset);
	FIXEDSTEPSTATE_ERROR(ATestFixedStepPawn, ReplicatedRotation, RepRotationOffset);
}

