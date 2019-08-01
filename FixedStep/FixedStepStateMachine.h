// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StateMachineXComponent.h"
#include "StateX.h"
#include "FixedStepComponent.h"
#include "FixedStepStateMachine.generated.h"

/**
 * 
 */
UCLASS()
class FIXEDSTEP_API UFixedStepStateMachineComponent : public UStateMachineXComponent, public IFixedStepListenerInterface
{
	GENERATED_BODY()
	
public:

	UFixedStepStateMachineComponent();
	
	
	UPROPERTY()
	FName HistoryStateName;

	UPROPERTY()
	float HistoryActiveTime;

	UPROPERTY()
	float HistoryVisualActiveTime;
		
	UFUNCTION()
	virtual void Simulate(int32 FrameNum, float DeltaTime, bool bRollback);
		
	UFUNCTION()
	virtual void PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha);

	UFUNCTION()
	virtual void SetFrame(int32 FrameNum);

	UFUNCTION()
	virtual void Interpolate(int32 FromFrame, int32 ToFrame, float Alpha, float DeltaTime);
		
	virtual bool GotoState(UStateX* ToState, bool bForce = false) override;

	UFUNCTION(BlueprintPure, Category=StateX)
	FORCEINLINE float GetVisualActiveTime() const
	{
		return HistoryActiveTime + HistoryVisualActiveTime;
	}

	UFUNCTION(BlueprintPure, Category=StateX)
	FORCEINLINE float GetActiveTime() const
	{
		return HistoryActiveTime;
	}

	virtual void GetFixedStepCommands(TArray<FFixedStepCmd>& OutCmds);
};


UCLASS()
class FIXEDSTEP_API UFixedStepState : public UStateX
{

	GENERATED_BODY()

public:
	
	UFUNCTION()
	virtual void Simulate(int32 FrameNum, float DeltaTime, bool bRollback);

	UFUNCTION()
	virtual void PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha);

	UFUNCTION()
	virtual void Interpolate(int32 FromFrame, int32 ToFrame, float Alpha, float DeltaTime);

	virtual float GetActiveTime() const
	{
		return Cast<UFixedStepStateMachineComponent>(GetStateMachine())->GetActiveTime();
	}

	UFUNCTION(BlueprintPure, Category = StateX)
		FORCEINLINE float GetVisualActiveTime() const
	{
		return Cast<UFixedStepStateMachineComponent>(GetStateMachine())->GetVisualActiveTime();
	}
};