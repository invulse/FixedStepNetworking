// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Pawn.h"
#include "FixedStepComponent.h"
#include "TestFixedStepPawn.generated.h"

UCLASS()
class FIXEDSTEP_API ATestFixedStepPawn : public APawn, public IFixedStepListenerInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ATestFixedStepPawn();

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UFixedStepComponent* FixedStepComponent;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UMeshComponent* MeshComponent;

	UPROPERTY(BlueprintReadWrite)
	FVector Input;

	UPROPERTY(BlueprintReadWrite)
	FVector ReplicatedLocation;

	UPROPERTY(BlueprintReadOnly)
	FVector RepLocationOffset;

	UPROPERTY(BlueprintReadWrite)
	FRotator ReplicatedRotation;

	UPROPERTY(BlueprintReadOnly)
	FRotator RepRotationOffset;
		
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float Speed;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float RotationSpeed;

	FVector PendingInput;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION()
	void MoveForward(float Value);
	
	UFUNCTION()
	void MoveSide(float Value);

	UFUNCTION()
	virtual void GatherInputs(int32 FrameNum);

	UFUNCTION()
	virtual void Simulate(int32 FrameNum, float DeltaTime, bool bRollback);
		
	UFUNCTION()
	virtual void PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha);

	UFUNCTION()
	virtual void SetFrame(int32 FrameNum);

	UFUNCTION()
	virtual void Interpolate(int32 FromFrame, int32 ToFrame, float Alpha, float DeltaTime);

	UFUNCTION()
	virtual void RollbackStart(int32 FrameNum);

	UFUNCTION()
	virtual void RollbackComplete(int32 FrameNum);

	virtual void GetFixedStepCommands(TArray<FFixedStepCmd>& OutCmds);
	
};
