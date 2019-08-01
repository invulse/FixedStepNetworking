// Fill out your copyright notice in the Description page of Project Settings.

#include "FixedStep.h"
#include "FixedStepCharacterMovement.h"
#include "FixedStepCharacter.h"

UFixedStepCharacterMovement::UFixedStepCharacterMovement()
{
	bUseAccelerationForPaths = true;
}

void UFixedStepCharacterMovement::BeginPlay()
{
	Super::BeginPlay();
}

AFixedStepCharacter* UFixedStepCharacterMovement::GetFixedStepCharacterOwner() const
{
	return Cast<AFixedStepCharacter>(GetCharacterOwner());
}

void UFixedStepCharacterMovement::GatherInputs(int32 FrameNum)
{
	ReplicatedMovementInput = CharacterOwner->GetPendingMovementInputVector(); //ConsumeInputVector();
}

void UFixedStepCharacterMovement::Simulate(int32 FrameNum, float DeltaTime, bool bRollback)
{	
	//Acceleration = Acceleration.GetClampedToMaxSize(GetMaxAcceleration());
	Acceleration = ScaleInputAcceleration(ConstrainInputAcceleration(ReplicatedMovementInput));
	AnalogInputModifier = ComputeAnalogInputModifier();

	//ApplyNetworkMovementMode(ReplicatedMovementMode);

	FTransform FixedStepRootMotion = FTransform::Identity;
	if (GetFixedStepCharacterOwner()->GetFixedStepRootMotion(DeltaTime, FixedStepRootMotion))
	{
		FTransform MeshTransform = CharacterOwner->GetMesh()->GetComponentTransform();
		
		FVector RootMotionVelocity = CalcAnimRootMotionVelocity(MeshTransform.TransformVector(FixedStepRootMotion.GetTranslation()), DeltaTime, Velocity);
		Velocity = ConstrainAnimRootMotionVelocity(RootMotionVelocity, Velocity);
		StartNewPhysics(DeltaTime, 0);
		UpdateCharacterStateAfterMovement(DeltaTime);

		const FQuat OldActorRotationQuat = UpdatedComponent->GetComponentQuat();
		const FQuat RootMotionRotationQuat = FixedStepRootMotion.GetRotation();
		if (!RootMotionRotationQuat.IsIdentity())
		{
			const FQuat NewActorRotationQuat = RootMotionRotationQuat * OldActorRotationQuat;
			MoveUpdatedComponent(FVector::ZeroVector, NewActorRotationQuat, true);
		}
	}
	else
	{		
		PerformMovement(DeltaTime);
	}
	
	ReplicatedLocation = CharacterOwner->GetActorLocation();
	ReplicatedRotation = CharacterOwner->GetActorRotation();
	ReplicatedVelocity = Velocity;
	ReplicatedMovementMode = PackNetworkMovementMode();
}

void UFixedStepCharacterMovement::PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha)
{
	GetFixedStepCharacterOwner()->VisualRoot->SetWorldLocationAndRotation(ReplicatedLocation + VisualLocation, ReplicatedRotation.Quaternion() * VisualRotation.Quaternion());
	ConsumeInputVector();
}

void UFixedStepCharacterMovement::SetFrame(int32 FrameNum)
{
	ApplyNetworkMovementMode(ReplicatedMovementMode);
	CharacterOwner->SetActorLocation(ReplicatedLocation);
	CharacterOwner->SetActorRotation(ReplicatedRotation);
	Velocity = ReplicatedVelocity;
}

void UFixedStepCharacterMovement::Interpolate(int32 FromFrame, int32 ToFrame, float Alpha, float DeltaTime)
{
	ApplyNetworkMovementMode(ReplicatedMovementMode);
	CharacterOwner->SetActorLocation(ReplicatedLocation);
	CharacterOwner->SetActorRotation(ReplicatedRotation);
	Velocity = ReplicatedVelocity;
	GetFixedStepCharacterOwner()->VisualRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FQuat::Identity);
}

void UFixedStepCharacterMovement::RollbackStart(int32 FrameNum)
{
}

void UFixedStepCharacterMovement::RollbackComplete(int32 FrameNum)
{
}

void UFixedStepCharacterMovement::GetFixedStepCommands(TArray<FFixedStepCmd>& OutCmds)
{
	FIXEDSTEPINPUT(UFixedStepCharacterMovement, ReplicatedMovementInput);
	FIXEDSTEPSTATE_ERRORVISUAL(UFixedStepCharacterMovement, ReplicatedLocation, ReplicatedLocationError, VisualLocation);
	FIXEDSTEPSTATE_ERRORVISUAL(UFixedStepCharacterMovement, ReplicatedRotation, ReplicatedRotationError, VisualRotation);
	FIXEDSTEPSTATE_VISUAL(UFixedStepCharacterMovement, ReplicatedVelocity, VisualVelocity);
	FIXEDSTEPSTATE(UFixedStepCharacterMovement, ReplicatedMovementMode);
}

