// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Character.h"
#include "FixedStepComponent.h"
#include "FixedStepCharacter.generated.h"

UCLASS()
class FIXEDSTEP_API AFixedStepCharacter : public ACharacter, public IFixedStepListenerInterface
{
	GENERATED_UCLASS_BODY()

public:
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UFixedStepComponent* FixedStepComp;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	USceneComponent* VisualRoot;

	UPROPERTY(BlueprintReadOnly)
	FRotator ReplicatedControlRotationInput;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnRep_ReplicatedMovement() override {};

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual void PossessedBy(AController* NewController) override;
	
//Fixed step interface
protected:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	class UFixedStepCharacterMovement* FixedStepMovement;

	UFUNCTION()
	virtual void GatherInputs(int32 FrameNum);

	UFUNCTION()
	virtual void Simulate(int32 FrameNum, float DeltaTime, bool bRollback);

	virtual void GetFixedStepCommands(TArray<FFixedStepCmd>& OutCmds);

//Root motion
public:
		
	virtual bool GetFixedStepRootMotion(float DeltaTime, FTransform& OutRootMotion) const;
};
