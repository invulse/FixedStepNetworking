// Fill out your copyright notice in the Description page of Project Settings.

#include "FixedStep.h"
#include "FixedStepManagerComponent.h"
#include "FixedStepComponent.h"
#include "FixedStepPlayerComponent.h"

#include "GameFramework/HUD.h"
#include "DisplayDebugHelpers.h"

// Sets default values for this component's properties
UFixedStepManagerComponent::UFixedStepManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	// ...

	TickRate = 0.016f; //60Hz
	MaxDeltaTime = 0.2f;
	MaxRollback = 0.5f;
	NetDirtyFrame = INDEX_NONE;
	PingFudgeFactor = 0.25f;
	CurrentFrameNumber = 0;
	MaxExtraOffset = 4;
	InterpolatedFrames = 0;
	InterpRatio = 2.f;
	NetFrameMaxDelta = 0.1f;
	JitterMinFrames = 2;
	JitterStdDevs = 3;
}


// Called when the game starts
void UFixedStepManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	//Search the world for all fixed step components and players	
	for (FActorIterator ActorIt(GetWorld()); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		if (!Actor || Actor->IsPendingKill()) continue;

		for (UActorComponent* Comp : Actor->GetComponentsByClass(UFixedStepComponent::StaticClass()))
		{
			UFixedStepComponent* FixedComp = Cast<UFixedStepComponent>(Comp);
			if(FixedComp && !FixedComp->IsInitialized())
			{
				AddComponent(FixedComp);
			}
		}

		for (UActorComponent* Comp : Actor->GetComponentsByClass(UFixedStepPlayerComponent::StaticClass()))
		{
			UFixedStepPlayerComponent* PlayerComp = Cast<UFixedStepPlayerComponent>(Comp);
			if (PlayerComp && !PlayerComp->IsInitialized())
			{
				AddPlayerComponent(PlayerComp);
			}
		}
	}
	
	AHUD::OnShowDebugInfo.AddUObject(this, &UFixedStepManagerComponent::OnDisplayDebug);
}


// Called every frame
void UFixedStepManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	{
		//Lock this from allowing components array to be changed during all this ticking
		FScopedFixedStepLock Lock(this);

		if (GetOwner()->Role < ROLE_Authority)
		{
			if (NetDirtyFrame > INDEX_NONE)
			{
				int32 ToFrame = GetCurrentFrame();

				for (UFixedStepComponent* Comp : AllComponents)
				{
					Comp->RollbackStart(NetDirtyFrame);
				}

				int32 CurFrame = NetDirtyFrame + 1;

				while (CurFrame <= ToFrame)
				{
					for (UFixedStepComponent* Comp : AllComponents)
					{
						Comp->GatherInputs(CurFrame, TickRate, true);
					}

					for (UFixedStepComponent* Comp : AllComponents)
					{
						Comp->Simulate(CurFrame, TickRate, true);
					}

					CurFrame++;
				}

				for (UFixedStepComponent* Comp : AllComponents)
				{
					Comp->RollbackComplete();
				}

				NetDirtyFrame = INDEX_NONE;
			}
		}


		float InterpAlpha = Accumulator / TickRate;

		if (GetOwner()->Role < ROLE_Authority)
		{
			//TODO: Improve both the calculation and growing/shrinking of this interpolated frames.
			int32 NewInterpFrames = FMath::CeilToInt(((InterpBucket.Average() + InterpBucket.StdDev()) * InterpRatio) / TickRate);

			if (NewInterpFrames > InterpolatedFrames * 1.5f || NewInterpFrames < InterpolatedFrames * 0.5f)
			{
				InterpolatedFrames = NewInterpFrames;
			}
			else if (NewInterpFrames < InterpolatedFrames / 2 + 1)
			{
				InterpolatedFrames = NewInterpFrames;
			}

			int32 InterpolateFrameFrom = CurrentFrameNumber - InterpolatedFrames - 1;

			UE_LOG(LogFixedStep, VeryVerbose, TEXT("Interpolating From: %d - To: %d Alpha: %f  CurFrame: %d"), InterpolateFrameFrom, InterpolateFrameFrom + 1, InterpAlpha, CurrentFrameNumber);

			if (InterpolateFrameFrom > 0)
			{
				for (UFixedStepComponent* Comp : AllComponents)
				{
					Comp->Interpolate(InterpolateFrameFrom, InterpolateFrameFrom + 1, InterpAlpha, DeltaTime);
				}
			}
		}


		Accumulator += DeltaTime;
		
		int32 FramesSimulated = 0;

		while (Accumulator >= TickRate)
		{
			CurrentFrameNumber++;
			FramesSimulated++;
			Accumulator -= TickRate;

			for (UFixedStepComponent* Comp : AllComponents)
			{
				Comp->GatherInputs(CurrentFrameNumber, TickRate, false);
			}

			for (UFixedStepComponent* Comp : AllComponents)
			{
				Comp->Simulate(CurrentFrameNumber, TickRate, false);
			}

			UE_LOG(LogFixedStep, VeryVerbose, TEXT("Simulated frame=%d client=%d"), CurrentFrameNumber, GetOwner()->Role != ROLE_Authority);
		}

		float Alpha = Accumulator / TickRate;

		for (UFixedStepComponent* Comp : AllComponents)
		{
			Comp->PostSimulate(CurrentFrameNumber - FramesSimulated, CurrentFrameNumber, Alpha);
		}
	}

	for (UFixedStepComponent* Comp : DeferredAddComponents)
	{
		AddComponent(Comp);
	}
	DeferredAddComponents.Empty();

	for (UFixedStepComponent* Comp : DeferredRemoveComponents)
	{
		RemoveComponent(Comp);
	}
	DeferredRemoveComponents.Empty();
}

void UFixedStepManagerComponent::AddComponent(UFixedStepComponent* Comp)
{
	if (bIteratingComponents)
	{
		DeferredAddComponents.Add(Comp);
		return;
	}

	AllComponents.AddUnique(Comp);
	Comp->Init(this);
}

void UFixedStepManagerComponent::RemoveComponent(UFixedStepComponent* Comp)
{
	if (bIteratingComponents)
	{
		DeferredRemoveComponents.Add(Comp);
		return;
	}

	AllComponents.Remove(Comp);
}

void UFixedStepManagerComponent::AddPlayerComponent(UFixedStepPlayerComponent* Comp)
{
	AllPlayers.AddUnique(Comp);
	Comp->Init(this);
}

void UFixedStepManagerComponent::RemovePlayerComponent(UFixedStepPlayerComponent* Comp)
{
	AllPlayers.Remove(Comp);
}

int32 UFixedStepManagerComponent::GetCurrentFrame() const
{
	return CurrentFrameNumber;
}

bool UFixedStepManagerComponent::IsInRollback() const
{
	return bInRollback;
}

void UFixedStepManagerComponent::SetNetDirtyFrame(int32 FrameNum)
{	
	UE_LOG(LogFixedStep, Verbose, TEXT("Set Net Dirty Frame - DirtyFrame: %d CurFrame %d"), FrameNum, GetCurrentFrame());

	if (FrameNum > CurrentFrameNumber)
	{
		//ensure(false);
	}
	else
	{
		NetDirtyFrame = FrameNum;
	}
}

void UFixedStepManagerComponent::SetLastInputFrame(UNetConnection* ForConnection, int32 FrameNum)
{
	int32& Frame = PlayerInputFrame.FindOrAdd(ForConnection);
	Frame = FrameNum;
}

int32 UFixedStepManagerComponent::GetLastInputFrame(UNetConnection* ForConnection)
{
	return PlayerInputFrame.FindOrAdd(ForConnection);
}

UFixedStepPlayerComponent* UFixedStepManagerComponent::GetPlayerForOwner(const AActor* OwningActor) const
{
	for (UFixedStepPlayerComponent* PlayerComp : AllPlayers)
	{
		if (PlayerComp->GetOwner() == OwningActor)
		{
			return PlayerComp;
		}
	}

	check(false);
	return NULL;
}

void UFixedStepManagerComponent::ReceivedInterpFrame()
{
	if (GetWorld()->GetRealTimeSeconds() > LastReceivedInterpFrameTime)
	{
		float Delta = FMath::Clamp(GetWorld()->GetRealTimeSeconds() - LastReceivedInterpFrameTime, 0.f, NetFrameMaxDelta);
		LastReceivedInterpFrameTime = GetWorld()->GetRealTimeSeconds();

		InterpBucket.Add(Delta);

		UE_LOG(LogFixedStepInterp, Verbose, TEXT("Received InterpFrame - %d, Delta: %f"), GetCurrentFrame(), Delta);
	}
}

void UFixedStepManagerComponent::LagCompensationStart(int32 FrameNum, UNetConnection* ForConnection)
{
	check(GetOwner()->Role == ROLE_Authority);

	for (UFixedStepComponent* Comp : AllComponents)
	{
		if (Comp->GetOwner()->GetNetConnection() != ForConnection)
		{
			Comp->LagCompensationStart(FrameNum);
		}
	}
}

void UFixedStepManagerComponent::LagCompensationEnd(UNetConnection* ForConnection)
{
	check(GetOwner()->Role == ROLE_Authority);

	for (UFixedStepComponent* Comp : AllComponents)
	{
		if (Comp->GetOwner()->GetNetConnection() != ForConnection)
		{
			Comp->LagCompensationEnd();
		}
	}
}


void UFixedStepManagerComponent::AddJitterDelta(UNetConnection* ForConnection, float DeltaTime)
{
	TBucket<float, 64>& JitterBucket = PlayerJitterBucket.FindOrAdd(ForConnection);
	JitterBucket.Add(FMath::Min(DeltaTime, NetFrameMaxDelta));
}

int32 UFixedStepManagerComponent::GetJitterFrames(UNetConnection* ForConnection)
{
	TBucket<float, 64>& JitterBucket = PlayerJitterBucket.FindOrAdd(ForConnection);

	return FMath::Max(
		FMath::RoundToInt((JitterBucket.Average() + JitterBucket.StdDev() * JitterStdDevs)/TickRate) + 1, 
		JitterMinFrames);
}

void UFixedStepManagerComponent::OnDisplayDebug(AHUD* HUD, UCanvas* Canvas, const class FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	if (DebugDisplay.IsDisplayOn(TEXT("FixedStep")))
	{
		DisplayDebug(Canvas, DebugDisplay, YL, YPos);
	}
}

void UFixedStepManagerComponent::DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{	
	Canvas->SetDrawColor(FColor::White);
	UFont* RenderFont = GEngine->GetSmallFont();
	FString T = FString::Printf(TEXT(""));

	T = FString::Printf(TEXT("FixedStep %s"), GetOwner()->Role == ROLE_Authority ? TEXT("Server") : TEXT("Client"));
	Canvas->DrawText(GEngine->GetLargeFont(), T, 4.0f, YPos);
	YPos += YL;

	T = FString::Printf(TEXT("Current Frame: %d"), GetCurrentFrame());
	Canvas->DrawText(RenderFont, T, 4.0f, YPos);
	YPos += YL;
	
	if (GetOwner()->Role < ROLE_Authority)
	{

		T = FString::Printf(TEXT("Interpolation - Frames: %d DeltMin: %f Max: %f Ave: %f StdDev: %f"), GetInterpolatedFrames(), InterpBucket.Min(), InterpBucket.Max(), InterpBucket.Average(), InterpBucket.StdDev());
		Canvas->DrawText(RenderFont, T, 4.0f, YPos);
		YPos += YL;
	}
	else
	{
		for (UFixedStepComponent* Comp : AllComponents)
		{
			if (Comp->HasInputs() && Comp->GetOwner()->GetNetConnection())
			{
				if (!PlayerJitterBucket.Contains(Comp->GetOwner()->GetNetConnection())) continue;
				TBucket<float, 64>& JitterBucket = PlayerJitterBucket[Comp->GetOwner()->GetNetConnection()];
				T = FString::Printf(TEXT("Comp %s - Buffered Inputs: %d / %d AveDelta: %f StdDev: %f"), *Comp->GetOwner()->GetName(), Comp->DebugBufferedInputFrames, Comp->DebugBufferedInputSize, JitterBucket.Average(), JitterBucket.StdDev());
				Canvas->DrawText(RenderFont, T, 4.0f, YPos);
				YPos += YL;
			}
		}
	}
	
	YPos += YL * 2;
}