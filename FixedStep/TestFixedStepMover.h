// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "FixedStepComponent.h"
#include "TestFixedStepMover.generated.h"


UCLASS()
class FIXEDSTEP_API ATestFixedStepMover : public AActor, public IFixedStepListenerInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATestFixedStepMover();

	UPROPERTY(EditAnywhere)
	UFixedStepComponent* FixedStepComp;

	UPROPERTY(BlueprintReadOnly)
	FVector StateLocation;

	UPROPERTY(BlueprintReadOnly)
	FVector StateLocationError;

	UPROPERTY(BlueprintReadOnly)
	FVector Input;

	UPROPERTY(EditAnywhere)
	float Speed;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
	
};
