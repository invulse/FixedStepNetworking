// Fill out your copyright notice in the Description page of Project Settings.

#include "FixedStep.h"
#include "FixedStepProjectileMovement.h"




UFixedStepProjectileMovement::UFixedStepProjectileMovement()
{
	bProjectileActive = true;
}

void UFixedStepProjectileMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{

}

void UFixedStepProjectileMovement::StartSimulating()
{
	bProjectileActive = true;
}

void UFixedStepProjectileMovement::StopSimulating()
{
	Velocity = FVector::ZeroVector;
	bProjectileActive = false;
}

void UFixedStepProjectileMovement::StopSimulating(const FHitResult& HitResult)
{
	if (bInRollback) return;

	StopSimulating();
	OnProjectileStop.Broadcast(HitResult);
}

void UFixedStepProjectileMovement::Simulate(int32 FrameNum, float DeltaTime, bool bRollback)
{	
	if (!bProjectileActive) return;

	// skip if don't want component updated when not rendered or updated component can't move
	if (ShouldSkipUpdate(DeltaTime))
	{
		return;
	}
	
	if (!IsValid(UpdatedComponent))
	{
		return;
	}

	AActor* ActorOwner = UpdatedComponent->GetOwner();
	if (!ActorOwner || !CheckStillInWorld())
	{
		return;
	}

	if (UpdatedComponent->IsSimulatingPhysics())
	{
		return;
	}
	
	float RemainingTime = DeltaTime;
	uint32 NumBounces = 0;
	int32 Iterations = 0;
	FHitResult Hit(1.f);

	while (RemainingTime >= MIN_TICK_TIME && !ActorOwner->IsPendingKill() && bProjectileActive)
	{
		const float TimeTick = FMath::Min(DeltaTime, RemainingTime);
		RemainingTime -= TimeTick;

		Hit.Time = 1.f;
		const FVector OldVelocity = Velocity;
		const FVector MoveDelta = ComputeMoveDelta(OldVelocity, TimeTick);

		const FQuat NewRotation = (bRotationFollowsVelocity && !OldVelocity.IsNearlyZero(0.01f)) ? OldVelocity.ToOrientationQuat() : UpdatedComponent->GetComponentQuat();

		// Move the component
		if (bShouldBounce)
		{
			// If we can bounce, we are allowed to move out of penetrations, so use SafeMoveUpdatedComponent which does that automatically.
			SafeMoveUpdatedComponent(MoveDelta, NewRotation, true, Hit);
		}
		else
		{
			// If we can't bounce, then we shouldn't adjust if initially penetrating, because that should be a blocking hit that causes a hit event and stop simulation.
			TGuardValue<EMoveComponentFlags> ScopedFlagRestore(MoveComponentFlags, MoveComponentFlags | MOVECOMP_NeverIgnoreBlockingOverlaps);
			MoveUpdatedComponent(MoveDelta, NewRotation, true, &Hit);
		}

		// If we hit a trigger that destroyed us, abort.
		if (ActorOwner->IsPendingKill() || !bProjectileActive)
		{
			return;
		}

		// Handle hit result after movement
		if (!Hit.bBlockingHit)
		{
			PreviousHitTime = 1.f;
			bIsSliding = false;

			// Only calculate new velocity if events didn't change it during the movement update.
			if (Velocity == OldVelocity)
			{
				Velocity = ComputeVelocity(Velocity, TimeTick);
			}
		}
		else
		{
			// Only calculate new velocity if events didn't change it during the movement update.
			if (Velocity == OldVelocity)
			{
				// re-calculate end velocity for partial time
				Velocity = (Hit.Time > KINDA_SMALL_NUMBER) ? ComputeVelocity(OldVelocity, TimeTick * Hit.Time) : OldVelocity;
			}

			// Handle blocking hit
			float SubTickTimeRemaining = DeltaTime * (1.f - Hit.Time);
			const EHandleBlockingHitResult HandleBlockingResult = HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);
			if (HandleBlockingResult == EHandleBlockingHitResult::Abort || !bProjectileActive)
			{
				//break;
			}
			else if (HandleBlockingResult == EHandleBlockingHitResult::Deflect)
			{
				NumBounces++;
				HandleDeflection(Hit, OldVelocity, NumBounces, SubTickTimeRemaining);
				PreviousHitTime = Hit.Time;
				PreviousHitNormal = ConstrainNormalToPlane(Hit.Normal);
			}
			else if (HandleBlockingResult == EHandleBlockingHitResult::AdvanceNextSubstep)
			{
				// Reset deflection logic to ignore this hit
				PreviousHitTime = 1.f;
			}
			else
			{
				// Unhandled EHandleBlockingHitResult
				checkNoEntry();
			}
		}
	}	

	UpdateComponentVelocity();

	if (UpdatedComponent)
	{
		ReplicatedLocation = UpdatedComponent->GetComponentLocation();
		ReplicatedRotation = UpdatedComponent->GetComponentRotation();
		ReplicatedVelocity = Velocity;
	}
}

void UFixedStepProjectileMovement::SetFrame(int32 FrameNum)
{	
	Velocity = ReplicatedVelocity;
	UpdateComponentVelocity();

	if (UpdatedComponent)
	{
		UpdatedComponent->SetWorldLocationAndRotation(ReplicatedLocation, ReplicatedRotation.Quaternion(), false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UFixedStepProjectileMovement::Interpolate(int32 FromFrameNum, int32 ToFrameNum, float Alpha, float DeltaTime)
{
	Velocity = ReplicatedVelocity;

	if (UpdatedComponent)
	{
		UpdatedComponent->SetWorldLocationAndRotation(ReplicatedLocation, ReplicatedRotation.Quaternion(), false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UFixedStepProjectileMovement::RollbackStart(int32 FrameNum)
{
	bInRollback = true;
}

void UFixedStepProjectileMovement::RollbackComplete(int32 FrameNum)
{
	bInRollback = false;
}

void UFixedStepProjectileMovement::GetFixedStepCommands(TArray<FFixedStepCmd>& OutCmds)
{
	FIXEDSTEPSTATE(UFixedStepProjectileMovement, ReplicatedVelocity);
	FIXEDSTEPSTATE_ERRORVISUAL(UFixedStepProjectileMovement, ReplicatedRotation, ReplicatedRotationError, VisualRotation);
	FIXEDSTEPSTATE_ERRORVISUAL(UFixedStepProjectileMovement, ReplicatedLocation, ReplicatedLocationError, VisualLocation);
	FIXEDSTEPSTATE(UFixedStepProjectileMovement, bProjectileActive);
}
