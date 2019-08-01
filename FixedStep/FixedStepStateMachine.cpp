// Fill out your copyright notice in the Description page of Project Settings.

#include "FixedStep.h"
#include "FixedStepStateMachine.h"


UFixedStepStateMachineComponent::UFixedStepStateMachineComponent()
{
	bReplicates = false;
}

void UFixedStepStateMachineComponent::Simulate(int32 FrameNum, float DeltaTime, bool bRollback)
{
	HistoryStateName = ActiveState->Name;
	HistoryActiveTime += DeltaTime;

	if (UFixedStepState* FixedState = Cast<UFixedStepState>(ActiveState))
	{
		FixedState->Simulate(FrameNum, DeltaTime, bRollback);
	}
}

void UFixedStepStateMachineComponent::PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha)
{
	if (UFixedStepState* FixedState = Cast<UFixedStepState>(ActiveState))
	{
		FixedState->PostSimulate(StartFrame, EndFrame, Alpha);
	}
}

void UFixedStepStateMachineComponent::SetFrame(int32 FrameNum)
{
	if ((!ActiveState || HistoryStateName != ActiveState->Name) && StateMap.Contains(HistoryStateName))
	{
		//float PreActiveTime = HistoryActiveTime; //Need to use this to restore the time after switching states as this causes an automatic time reset
		GotoStateName(HistoryStateName, true);
		//HistoryActiveTime = PreActiveTime;
	}
}

void UFixedStepStateMachineComponent::Interpolate(int32 FromFrame, int32 ToFrame, float Alpha, float DeltaTime)
{
	if ((!ActiveState || HistoryStateName != ActiveState->Name) && StateMap.Contains(HistoryStateName))
	{
		GotoStateName(HistoryStateName, true);
	}

	if (UFixedStepState* FixedState = Cast<UFixedStepState>(ActiveState))
	{
		FixedState->Interpolate(FromFrame, ToFrame, Alpha, DeltaTime);
	}
}

bool UFixedStepStateMachineComponent::GotoState(UStateX* ToState, bool bForce)
{
	if (Super::GotoState(ToState, bForce))
	{
		if(!bForce)
		{
			HistoryActiveTime = 0;
		}
		return true;
	}

	return false;
}

void UFixedStepStateMachineComponent::GetFixedStepCommands(TArray<FFixedStepCmd>& OutCmds)
{
	FIXEDSTEPSTATE(UFixedStepStateMachineComponent, HistoryStateName);
	FIXEDSTEPSTATE_VISUAL(UFixedStepStateMachineComponent, HistoryActiveTime, HistoryVisualActiveTime);
}

void UFixedStepState::Simulate(int32 FrameNum, float DeltaTime, bool bRollback)
{
}

void UFixedStepState::PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha)
{
}

void UFixedStepState::Interpolate(int32 FromFrame, int32 ToFrame, float Alpha, float DeltaTime)
{
}
