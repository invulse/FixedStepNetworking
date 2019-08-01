// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "FixedStepManagerComponent.generated.h"

template<typename InType, uint8 Size>
class TBucket
{		
private:
	InType Bucket[Size];
	int32 Pos;

public:
	TBucket() : Pos(0)
	{
	}

	void Add(InType Value)
	{
		Bucket[Pos % Size] = Value;
		Pos++;
	}

	int32 Count() const
	{
		return FMath::Min((int32)Size, Pos);
	}

	InType Average() const
	{
		if (Count() == 0) return 0;

		InType Total = 0;

		for (int32 Idx = 0; Idx < Count(); Idx++)
		{
			Total += Bucket[Idx];
		}

		return Total / Count();
	}

	InType StdDev() const
	{
		if (Count() == 0) return 0.f;

		InType Ave = Average();
		InType Var = 0;

		for (int32 Idx = 0; Idx < Count(); Idx++)
		{
			Var += FMath::Square(Bucket[Idx] - Ave);
		}

		Var /= Count();

		return FMath::Sqrt(Var);
	}
	
	InType Min() const
	{
		InType M = 0;

		for (int32 Idx = 0; Idx < Count(); Idx++)
		{
			M = FMath::Min(Bucket[Idx], M);
		}

		return M;
	}

	InType Max() const
	{
		InType M = 0;
		
		for (int32 Idx = 0; Idx < Count(); Idx++)
		{
			M = FMath::Max(Bucket[Idx], M);
		}

		return M;
	}

	bool Filled() const 
	{
		return Count() >= Size;
	}
};

class UFixedStepComponent;
class UFixedStepPlayerComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FIXEDSTEP_API UFixedStepManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	//The fixed step tick rate that will be used. 
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float TickRate;

	//How long in seconds we are allowed to rewind. Anything further will be clamped.  This is essentially your max supported ping.
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	float MaxRollback;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	int32 CurrentFrameNumber;

	//How many frames we believe we are ahead of the server
	UPROPERTY()
	int32 FramesAhead;
	
	//The maximum delta time we will allow before clamping.  This will limit the number of steps the manager can do in a single frame
	UPROPERTY(EditAnywhere)
	float MaxDeltaTime;

	//How much we will add to the half rtt to make sure we are slightly ahead of the server
	UPROPERTY(EditAnywhere)
	float PingFudgeFactor;

	// If we exceed this amount of extra offset from the expected server frame on client, we should slowly roll back how far we are ahead of the server to match this value.
	UPROPERTY(EditAnywhere)
	int32 MaxExtraOffset;

	//Our ratio average time + stddev between received frames from the server and the time we should be interpolating comps at
	UPROPERTY(EditAnywhere)
	float InterpRatio;

	//The max allowed delta time we will record for receiving net frames (client and server)
	UPROPERTY(EditAnywhere)
	float NetFrameMaxDelta;
	
	UPROPERTY(EditAnywhere)
	int32 JitterMinFrames;
	
	UPROPERTY(EditAnywhere)
	float JitterStdDevs;

	UPROPERTY(EditAnywhere)
	bool bDisableErrorCorrection;

	UPROPERTY(EditAnywhere)
	bool bDisableVisualInterpolation;

	// Sets default values for this component's properties
	UFixedStepManagerComponent();
	
	TBucket<float, 8> InterpBucket;

	TMap<UNetConnection*, TBucket<float, 64>> PlayerJitterBucket;

	//Map of last input frame processed by the server for a given player
	TMap<UNetConnection*, int32> PlayerInputFrame;

protected:
	
	UPROPERTY()
	float Accumulator;
	
	UPROPERTY()
	bool bInRollback;
	
	//If assigned this is the furthest back a new net frame was received by the client.
	UPROPERTY()
	int32 NetDirtyFrame;
		
	UPROPERTY()
	int32 InterpolatedFrames;
	
	UPROPERTY()
	float LastReceivedInterpFrameTime;

	UPROPERTY()
	TArray<UFixedStepComponent*> AllComponents;

	TArray<UFixedStepComponent*> DeferredAddComponents;
	TArray<UFixedStepComponent*> DeferredRemoveComponents;

	UPROPERTY()
	TArray<UFixedStepPlayerComponent*> AllPlayers;

	bool bIteratingComponents;

	// Called when the game starts
	virtual void BeginPlay() override;



public:	

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AddComponent(UFixedStepComponent* Comp);

	void RemoveComponent(UFixedStepComponent* Comp);

	void AddPlayerComponent(UFixedStepPlayerComponent* Comp);

	void RemovePlayerComponent(UFixedStepPlayerComponent* Comp);

	UFUNCTION(BlueprintCallable, Category = FixedStep)
	int32 GetCurrentFrame() const;

	UFUNCTION(BlueprintCallable, Category = FixedStep)
	FORCEINLINE int32 GetInterpolatedFrames() const { return InterpolatedFrames; }

	UFUNCTION(BlueprintCallable, Category = FixedStep)
	bool IsInRollback() const;
	
	UFUNCTION(BlueprintCallable, Category = FixedStep)
	FORCEINLINE int32 GetMaxRollbackFrames() const { return FMath::FloorToInt(MaxRollback / TickRate); };

	void SetNetDirtyFrame(int32 FrameNum);

	void SetLastInputFrame(UNetConnection* ForConnection, int32 FrameNum);
	int32 GetLastInputFrame(UNetConnection* ForConnection);

	UFixedStepPlayerComponent* GetPlayerForOwner(const AActor* OwningActor) const;

	void ReceivedInterpFrame();

	void LagCompensationStart(int32 FrameNum, UNetConnection* ForConnection);

	void LagCompensationEnd(UNetConnection* ForConnection);
	
	void AddJitterDelta(UNetConnection* ForConnection, float DeltaTime);
	int32 GetJitterFrames(UNetConnection* ForConnection);


protected:

	virtual void OnDisplayDebug(class AHUD* HUD, class UCanvas* Canvas, const class FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos);
	virtual void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos);

	friend class FScopedFixedStepLock;
};

class FScopedFixedStepLock
{
	UFixedStepManagerComponent* Comp;

public:
	FScopedFixedStepLock(UFixedStepManagerComponent* InComp)
	{
		Comp = InComp;
		check(!Comp->bIteratingComponents)
		Comp->bIteratingComponents = true;
	}

	~FScopedFixedStepLock()
	{
		Comp->bIteratingComponents = false;
	}
};