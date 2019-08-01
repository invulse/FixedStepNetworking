// Fill out your copyright notice in the Description page of Project Settings.

#include "FixedStep.h"
#include "FixedStepCharacter.h"
#include "FixedStepCharacterMovement.h"

// Sets default values
AFixedStepCharacter::AFixedStepCharacter(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFixedStepCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	FixedStepComp = ObjectInitializer.CreateDefaultSubobject<UFixedStepComponent>(this, TEXT("FixedStep"));

	VisualRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(GetRootComponent());
	GetMesh()->SetupAttachment(VisualRoot);
	
	bReplicateMovement = false;

	FixedStepMovement = Cast<UFixedStepCharacterMovement>(GetMovementComponent());
}

// Called when the game starts or when spawned
void AFixedStepCharacter::BeginPlay()
{
	Super::BeginPlay();	

	//Disable use of root motion because we will manually extract and apply separately
	if (GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}

	FixedStepComp->RegisterObserver(this);
	FixedStepComp->RegisterObserver(FixedStepMovement);
}

// Called every frame
void AFixedStepCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AFixedStepCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AFixedStepCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (GetMesh())
	{
		GetMesh()->bOnlyAllowAutonomousTickPose = false; //Hack 
	}
}

void AFixedStepCharacter::GatherInputs(int32 FrameNum)
{
	ReplicatedControlRotationInput = GetControlRotation();
}

void AFixedStepCharacter::Simulate(int32 FrameNum, float DeltaTime, bool bRollback)
{
	if (bUseControllerRotationYaw || bUseControllerRotationPitch || bUseControllerRotationRoll)
	{		
		FRotator TargetRot(0.f);

		if (bUseControllerRotationYaw)
		{
			TargetRot.Yaw = ReplicatedControlRotationInput.Yaw;
		}

		if (bUseControllerRotationPitch)
		{
			TargetRot.Pitch = ReplicatedControlRotationInput.Pitch;
		}

		if (bUseControllerRotationRoll)
		{
			TargetRot.Roll = ReplicatedControlRotationInput.Roll;
		}

		SetActorRotation(TargetRot);
	}
}

void AFixedStepCharacter::GetFixedStepCommands(TArray<FFixedStepCmd>& OutCmds)
{
	FIXEDSTEPINPUT(AFixedStepCharacter, ReplicatedControlRotationInput);
}

bool AFixedStepCharacter::GetFixedStepRootMotion(float DeltaTime, FTransform& OutRootMotion) const
{
	return false;
}