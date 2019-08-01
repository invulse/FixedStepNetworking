// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "FixedStepManagerComponent.h"
#include "FixedStepComponent.generated.h"

namespace EHistoryProp
{
	enum Type
	{
		Default,
		Bool,
		Float,
		Int,
		Vector,
		Rotator,
	};
}

namespace EHistoryType
{
	enum Type
	{
		State,
		Input
	};
}

class FFixedStepCmd
{
public:
	UProperty* Property;
	UProperty* ErrorOffset;
	UProperty* VisualOffset;
	UObject* Owner;
	int32 Offset;
	EHistoryType::Type Type;
	EHistoryProp::Type PropertyType;
	float ErrorBlend;
	float ErrorTolerance;

	FFixedStepCmd() : 
		Property(nullptr), ErrorOffset(nullptr), VisualOffset(nullptr), Owner(nullptr), 
		Offset(INDEX_NONE), Type(EHistoryType::State), PropertyType(EHistoryProp::Default),
		ErrorBlend(0.0f), ErrorTolerance(0.0f)
	{}

	FFixedStepCmd(UObject* InOwner, UProperty* InProp, EHistoryType::Type InType, UProperty* InErrorProp = NULL, UProperty* InVisualProp = NULL, float InErrorBlend = -1.f, float InErrorTolerance = -1.f) :
		Property(InProp), ErrorOffset(InErrorProp), VisualOffset(InVisualProp), Owner(InOwner),
		Offset(INDEX_NONE), Type(InType), PropertyType(EHistoryProp::Default),
		ErrorBlend(InErrorBlend), ErrorTolerance(InErrorTolerance)
	{}
};

#define FIXEDSTEPINPUT(c,v) \
{ \
	static UProperty* sp##v = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,v)); \
	OutCmds.Add( FFixedStepCmd( this, sp##v, EHistoryType::Input ) );	\
}

#define FIXEDSTEPSTATE(c,v) \
{ \
	static UProperty* sp##v = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,v)); \
	OutCmds.Add( FFixedStepCmd( this, sp##v, EHistoryType::State ) );	\
}

#define FIXEDSTEPSTATE_ERROR(c,v,e) \
{ \
	static UProperty* sp##v = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,v)); \
	static UProperty* sp##e = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,e)); \
	OutCmds.Add( FFixedStepCmd( this, sp##v, EHistoryType::State, sp##e ) );	\
}

#define FIXEDSTEPSTATE_VISUAL(c,v,vis) \
{ \
	static UProperty* sp##v = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,v)); \
	static UProperty* sp##vis = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,vis)); \
	OutCmds.Add( FFixedStepCmd( this, sp##v, EHistoryType::State, NULL, sp##vis ) );	\
}

#define FIXEDSTEPSTATE_ERRORVISUAL(c,v,e,vis) \
{ \
	static UProperty* sp##v = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,v)); \
	static UProperty* sp##e = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,e)); \
	static UProperty* sp##vis = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,vis)); \
	OutCmds.Add( FFixedStepCmd( this, sp##v, EHistoryType::State, sp##e, sp##vis ) );	\
}

#define FIXEDSTEPSTATE_ERRORVISUAL_CUSTOMBLEND(c,v,e,vis,blend) \
{ \
	static UProperty* sp##v = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,v)); \
	static UProperty* sp##e = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,e)); \
	static UProperty* sp##vis = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,vis)); \
	OutCmds.Add( FFixedStepCmd( this, sp##v, EHistoryType::State, sp##e, sp##vis, blend ) );	\
}

#define FIXEDSTEPSTATE_ERRORVISUAL_CUSTOMBLEND_CUSTOMTOLERANCE(c,v,e,vis,blend,tol) \
{ \
	static UProperty* sp##v = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,v)); \
	static UProperty* sp##e = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,e)); \
	static UProperty* sp##vis = FindFieldChecked<UProperty>(c::StaticClass(),GET_MEMBER_NAME_CHECKED(c,vis)); \
	OutCmds.Add( FFixedStepCmd( this, sp##v, EHistoryType::State, sp##e, sp##vis, blend, tol ) );	\
}


USTRUCT()
struct FFixedStepEventBase
{
	GENERATED_BODY()
};

class FFixedStepEventCmd
{
public:

	//The owning object events of this type will come from.
	UObject* Owner;

	//The data type of this event.  Should be a pointer to the static struct type
	UScriptStruct* Type;

	//The call back function to invoke when receiving an event like this.
	UFunction* Callback;

	//The generated id given to this command on building of history.
	uint8 Id;

	FFixedStepEventCmd() : Owner(NULL), Type(NULL), Callback(NULL), Id(INDEX_NONE)
	{}

	FFixedStepEventCmd(UObject* InOwner, UScriptStruct* InType, UFunction* InCallback) : Owner(InOwner), Type(InType), Callback(InCallback), Id(INDEX_NONE)
	{
	}
};

USTRUCT()
struct FFixedStepEventData
{
	GENERATED_BODY()

	int32 FrameNum;
	uint8 Id; //Events support a max of 255 types per actor
	TArray<uint8> Data;
	UScriptStruct* Type;

	FFixedStepEventData()
	{}

	FFixedStepEventData(UScriptStruct* InType, int32 InFrameNum, int32 InId) : FrameNum(InFrameNum), Id(InId), Type(InType)
	{}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FFixedStepEventData> : public TStructOpsTypeTraitsBase2<FFixedStepEventData>
{
	enum
	{
		WithNetSerializer = true
	};
};

#define FIXEDSTEPEVENT(evt,callback) \
{ \
	OutEvents.Add( FFixedStepEventCmd(this, evt::StaticStruct(),  FindFunctionChecked(STATIC_FUNCTION_FNAME(TEXT(#callback)))) ); \
}

class FFixedStepFrame
{
public:
	int32 FrameNumber;
	TArray<uint8> StateData;
	TArray<uint8> InputData;		

	int32 ClientInterpDelta;

	//Set to true when the input data for this frame was set via a client rpc.
	bool bClientInput;

	//Change this at some point, we need a way without frame number to determine if the data here is set.
	bool bSet;
	
	FFixedStepFrame() : FrameNumber(INDEX_NONE), ClientInterpDelta(0), bClientInput(false), bSet(false) {}

	FFixedStepFrame(int32 StateSize, int32 InputSize) : FFixedStepFrame()
	{
		StateData.Init(0, StateSize);
		InputData.Init(0, InputSize);
	}

	void Copy(FFixedStepFrame& From, bool CopyFrameNum = true)
	{
		if (CopyFrameNum)
		{
			FrameNumber = From.FrameNumber;
		}
		FMemory::Memcpy(StateData.GetData(), From.StateData.GetData(), StateData.Num());
		FMemory::Memcpy(InputData.GetData(), From.InputData.GetData(), InputData.Num());
	}
};

class FFixedStepHistory
{
public:
	TArray<FFixedStepFrame> Buffer;
	TArray<FFixedStepCmd> Commands;
	TMap<uint8, FFixedStepEventCmd> Events;
	int32 StateSize;
	int32 InputSize;

	FFixedStepFrame PendingFrame;

	FFixedStepFrame PreRollbackState;

	FFixedStepFrame InterpFrom;
	FFixedStepFrame InterpTo;
	
	FFixedStepHistory() : StateSize(0), InputSize(0) 
	{
	}

	FFixedStepFrame& GetFrame(int32 FrameNum)
	{
		return Buffer[FrameNum%Buffer.Num()];
	}

	const FFixedStepFrame& GetFrameConst(int32 FrameNum) const
	{
		return Buffer[FrameNum%Buffer.Num()];
	}

	bool HasFrame(int32 FrameNum) const
	{
		return GetFrameConst(FrameNum).FrameNumber == FrameNum;
	}

	FFixedStepFrame& GetFrameOrBestBefore(int32 FrameNum)
	{				
		for (int32 F = FrameNum; F >= FMath::Max(0, FrameNum - Buffer.Num()); F--)
		{
			if (HasFrame(F))
			{
				return GetFrame(F);
			}
		}

		return GetFrame(FrameNum);
	}

	FFixedStepFrame& GetFrameOrBestAfter(int32 FrameNum)
	{
		for (int32 F = FrameNum; F < FrameNum + Buffer.Num(); F++)
		{
			if (HasFrame(F))
			{
				return GetFrame(F);
			}
		}

		return GetFrame(FrameNum);
	}
};


USTRUCT()
struct FFixedStepRepData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<uint8> StateData;

	UPROPERTY()
	TArray<uint8> InputData;
	
	FFixedStepFrame* LatestFrame;

	FFixedStepHistory* History;
	
	UPROPERTY()
	int32 FrameNum;
	
	UPROPERTY()
	int32 ClientInputFrame;

	UPROPERTY()
	uint32 Checksum;

	FFixedStepRepData() : FrameNum(INDEX_NONE) {};

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FFixedStepRepData> : public TStructOpsTypeTraitsBase2<FFixedStepRepData>
{
	enum 
	{
		WithNetSerializer = true
	};
};

UENUM()
namespace EFixedStepNetworkType
{
	enum Type
	{
		Simulate,
		Interpolate
	};
}

UINTERFACE(MinimalAPI, BlueprintType)
class UFixedStepListenerInterface : public UInterface
{
	GENERATED_BODY()
};

class FIXEDSTEP_API IFixedStepListenerInterface
{
	GENERATED_BODY()

public:
		
	virtual void Simulate(int32 FrameNum, float DeltaTime, bool bRollback) {}
	virtual void PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha) {}
	virtual void SetFrame(int32 FrameNum) {}
	virtual void GatherInputs(int32 FrameNum) {}
	virtual void RollbackStart(int32 FrameNum) {}
	virtual void RollbackComplete(int32 FrameNum) {}
	virtual void Interpolate(int32 FromFrameNum, int32 ToFrameNum, float Alpha, float DeltaTime) {}
	
	virtual void GetFixedStepCommands(TArray<FFixedStepCmd>& OutCmds) {}
	virtual void GetFixedStepEvents(TArray<FFixedStepEventCmd>& OutEvents) {}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FIXEDSTEP_API UFixedStepComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFixedStepComponent();

	// The network type to use for autonomous proxies of this object
	UPROPERTY(EditAnywhere)
	TEnumAsByte<EFixedStepNetworkType::Type> AutonomousType;

	// The network type to use for simulated proxies of this object
	UPROPERTY(VisibleAnywhere)
	TEnumAsByte<EFixedStepNetworkType::Type> SimulatedType;

	//Reference to owning manager
	UPROPERTY()
	UFixedStepManagerComponent* Manager;

	UPROPERTY(ReplicatedUsing = OnRep_FrameData)
	FFixedStepRepData FrameData;

	UPROPERTY(EditAnywhere)
	float DefaultErrorBlendOut;

	//How many redundant inputs to send each rpc
	UPROPERTY(EditAnywhere)
	int32 InputRedundancy;

	UPROPERTY(EditAnywhere)
	int32 MaxInputsToSend;

	TMap<EHistoryProp::Type, float> DefaultErrorTolerance;

	//Server only array of buffered inputs from clients 
	TArray<FFixedStepFrame> BufferedInputs;
	
	TArray<FFixedStepEventData> PendingEvents;

	bool bFillingBufferedInputs;
	
protected:
		
	// Called when the game starts
	virtual void BeginPlay() override;

	void TryFindManager();

	FFixedStepHistory History;

	void BuildHistory();

	void BuildHistoryForObject(IFixedStepListenerInterface* Object, TArray<FFixedStepEventCmd>& OutEvents);

	void RecordFrame(FFixedStepFrame& Frame, int32 FrameNum);

	void RecordHistory(int32 FrameNum);

public:

	void SetToFrame(FFixedStepFrame& Frame, bool bInputOnly = false);

	void SetToFrame(int32 FrameNum, bool bInputOnly = false);

	void SetToHistory(int32 FrameNum, bool bInputOnly = false);

	//Forces any visual interpolation to snap to current frame.
	void SnapVisualInterpolation();

	void RecordCurrentFrame();
	
protected:

	UFUNCTION()
	void OnRep_FrameData();
		
	TArray<IFixedStepListenerInterface*> Responders;

	int32 LastSimulatedFrame;

	FFixedStepFrame LastClientInputFrame;

	int32 QueuedInputs;

	UPROPERTY(BlueprintReadOnly)
	bool bHasInputs;
	
	float LastReceivedInputTime;

	int32 SnappedVisualInterpolationFrame;

public:	
	
	virtual void Init(UFixedStepManagerComponent* InManager);

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	UFUNCTION(BlueprintCallable, Category = FixedStep)
	void RegisterObserver(UObject* Responder);

	UFUNCTION(BlueprintCallable, Category = FixedStep)
	void UnregisterObserver(UObject* Responder);
	
	bool IsInitialized() { return Manager != NULL; }

	TEnumAsByte<ENetRole> GetNetRole() const;

	TEnumAsByte<ENetRole> GetNetRemoteRole() const;

	const AController* GetOwningController() const;

	UFUNCTION(BlueprintCallable, Category = FixedStep)
	TEnumAsByte<EFixedStepNetworkType::Type> GetNetworkType() const;

	UFUNCTION(BlueprintCallable, Category = FixedStep)
	int32 GetCurrentFrame() const;
	
	FORCEINLINE int32 GetLastClientInputFrame() const 
	{
		return LastClientInputFrame.FrameNumber;
	}

	FORCEINLINE int32 GetLastClientInterpDelta() const
	{
		return LastClientInputFrame.ClientInterpDelta;
	}

	bool IsLocallyControlled() const;

	bool WantSendInput() const;

	bool HasInputForFrame(int32 FrameNum) const;

	FORCEINLINE bool HasInputs() const { return bHasInputs; }

	UFUNCTION()
	virtual void GatherInputs(int32 FrameNum, float DeltaTime, bool bRollback);

	UFUNCTION()
	virtual void Simulate(int32 FrameNum, float DeltaTime, bool bRollback);

	UFUNCTION()
	virtual void PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha);
		
	UFUNCTION()
	virtual void RollbackStart(int32 FrameNum);

	UFUNCTION()
	virtual void RollbackComplete();

	UFUNCTION()
	virtual void LagCompensationStart(int32 FrameNum);

	UFUNCTION()
	virtual void LagCompensationEnd();

	UFUNCTION()
	virtual void Interpolate(int32 FromFrameNum, int32 ToFrameNum, float Alpha, float DeltaTime);
	
	void QueueEvent(UObject* Owner, UScriptStruct* Type, const FFixedStepEventBase* Event);

	UFUNCTION(Unreliable, NetMulticast)
	void MulticastSendEvent(int32 FrameNum, uint8 EventId, FFixedStepEventData Event);
	void MulticastSendEvent_Implementation(int32 FrameNum, uint8 EventId, FFixedStepEventData Event);

	void DispatchEvents(int32 FromFrame, int32 ToFrame);

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	virtual void InternalSimulate(int32 FrameNum, float DeltaTime, bool bRollback);

	void SendInputs();

	UFUNCTION(Unreliable, Server, WithValidation)
	void ServerSendInputs(uint8 NumInputs, const TArray<uint8>& Data, int32 FrameNum, int32 InterpDelta, uint32 Checksum);
	void ServerSendInputs_Implementation(uint8 NumInputs, const TArray<uint8>& Data, int32 FrameNum, int32 InterpDelta, uint32 Checksum);
	bool ServerSendInputs_Validate(uint8 NumInputs, const TArray<uint8>& Data, int32 FrameNum, int32 InterpDelta, uint32 Checksum);

public:
	
	int32 DebugBufferedInputFrames;
	int32 DebugBufferedInputSize;


protected:
	
	EHistoryProp::Type GetPropertyType(UProperty* Prop) const;

	void InterpProperty(const FFixedStepCmd& Cmd, FFixedStepFrame& From, FFixedStepFrame& To, float Alpha, void* OutData);
	void GetOffset(const FFixedStepCmd& Cmd, uint8* From, uint8* To, void* OutData);
	void GetError(const FFixedStepCmd& Cmd, FFixedStepFrame& PrevFrame, FFixedStepFrame& CurFrame);
	void BlendOutError(const FFixedStepCmd& Cmd);
	bool WithinTolerance(const FFixedStepCmd& Cmd, FFixedStepFrame& From, FFixedStepFrame& To);
};

class FFixedStepOp
{
public:
	
	//Interpolation

	//Non templated interp just sets from or to depending on alpha
	static void Interp(const FFixedStepCmd& Cmd, uint8* From, uint8* To, float Alpha, void* Out)
	{
		Cmd.Property->CopyCompleteValue(Out, Alpha < 0.5f ? From : To);
	}
	
	template<typename InType>
	static void Interp(const FFixedStepCmd& Cmd, uint8* From, uint8* To, float Alpha, void* Out) 
	{		
		InType FromValue;
		InType ToValue;
		InType Value;

		Cmd.Property->CopyCompleteValue(&FromValue, From);
		Cmd.Property->CopyCompleteValue(&ToValue, To);
		Value = FMath::Lerp(FromValue, ToValue, Alpha);
		Cmd.Property->CopyCompleteValue(Out, &Value);

	}
		
	template<>
	static void Interp<FRotator>(const FFixedStepCmd& Cmd, uint8* From, uint8* To, float Alpha, void*  Out)
	{
		FRotator FromValue;
		FRotator ToValue;
		FQuat Value;
	

		Cmd.Property->CopyCompleteValue(&FromValue, From);
		Cmd.Property->CopyCompleteValue(&ToValue, To);
		Value = FQuat::Slerp(FromValue.Quaternion(), ToValue.Quaternion(), Alpha);
		Value.EnforceShortestArcWith(ToValue.Quaternion());
		FRotator RotValue = Value.Rotator();

		Cmd.Property->CopyCompleteValue(Out, &RotValue);
	}

	//Offsets
	template<typename InType>
	static void GetOffset(const FFixedStepCmd& Cmd, uint8* From, uint8* To, void* Out) 
	{
		InType FromValue;
		InType ToValue;

		Cmd.Property->CopyCompleteValue(&FromValue, From);
		Cmd.Property->CopyCompleteValue(&ToValue, To);

		InType Offset = ToValue - FromValue;
		Cmd.Property->CopyCompleteValue(Out, &Offset);
	}

	template<>
	static void GetOffset<FRotator>(const FFixedStepCmd& Cmd, uint8* From, uint8* To, void* Out)
	{
		FRotator FromValue;
		FRotator ToValue;

		Cmd.Property->CopyCompleteValue(&FromValue, From);
		Cmd.Property->CopyCompleteValue(&ToValue, To);

		FRotator OffsetRot = (ToValue.Quaternion() * FromValue.Quaternion().Inverse()).Rotator();
		
		Cmd.Property->CopyCompleteValue(Out, &OffsetRot);
	}

	//Accumulate errors
	template<typename InType>
	static void GetError(const FFixedStepCmd& Cmd, uint8* Prev, uint8* Cur, void* Out) 
	{
		InType PrevValue;
		InType OldErrorValue;
		InType CurValue;

		Cmd.Property->CopyCompleteValue(&PrevValue, Prev);
		Cmd.Property->CopyCompleteValue(&CurValue, Cur);
		Cmd.Property->CopyCompleteValue(&OldErrorValue, Out);

		InType Error = PrevValue + OldErrorValue - CurValue;
		Cmd.ErrorOffset->CopyCompleteValue(Out, &Error);
	}

	//Error tolerances
	static bool WithinTolerance(const FFixedStepCmd& Cmd, uint8* From, uint8* To)
	{
		return Cmd.Property->Identical(From, To);
	}

	template<typename InType>
	static bool WithinTolerance(const FFixedStepCmd& Cmd, uint8* From, uint8* To)
	{
		return Cmd.Property->Identical(From, To);
	}

	template<>
	static bool WithinTolerance<int32>(const FFixedStepCmd& Cmd, uint8* From, uint8* To)
	{
		int32 Offset;
		GetOffset<int32>(Cmd, From, To, &Offset);
		return FMath::Abs(Offset) <= Cmd.ErrorTolerance;
	}

	template<>
	static bool WithinTolerance<uint8>(const FFixedStepCmd& Cmd, uint8* From, uint8* To)
	{
		uint8 Offset;
		GetOffset<uint8>(Cmd, From, To, &Offset);
		return FMath::Abs(Offset) <= Cmd.ErrorTolerance;
	}

	template<>
	static bool WithinTolerance<float>(const FFixedStepCmd& Cmd, uint8* From, uint8* To)
	{
		float Offset;
		GetOffset<float>(Cmd, From, To, &Offset);
		return FMath::Abs(Offset) <= Cmd.ErrorTolerance;
	}

	template<>
	static bool WithinTolerance<FVector>(const FFixedStepCmd& Cmd, uint8* From, uint8* To)
	{
		FVector Offset;
		GetOffset<FVector>(Cmd, From, To, &Offset);
		return FMath::Abs(Offset.X) <= Cmd.ErrorTolerance && FMath::Abs(Offset.Y) <= Cmd.ErrorTolerance && FMath::Abs(Offset.Z) <= Cmd.ErrorTolerance;
	}

	template<>
	static bool WithinTolerance<FRotator>(const FFixedStepCmd& Cmd, uint8* From, uint8* To)
	{
		FRotator Offset;
		GetOffset<FRotator>(Cmd, From, To, &Offset);
		return FMath::Abs(Offset.Pitch) <= Cmd.ErrorTolerance && FMath::Abs(Offset.Yaw) <= Cmd.ErrorTolerance && FMath::Abs(Offset.Roll) <= Cmd.ErrorTolerance;
	}

	template<>
	static void GetError<FRotator>(const FFixedStepCmd& Cmd, uint8* Prev, uint8* Cur, void* Out)
	{
		FRotator PrevValue;
		FRotator OldErrorValue;
		FRotator TargetValue;

		Cmd.Property->CopyCompleteValue(&PrevValue, Prev);
		Cmd.Property->CopyCompleteValue(&TargetValue, Cur);
		Cmd.Property->CopyCompleteValue(&OldErrorValue, Out);
		
		FRotator Error = ((PrevValue.Quaternion() * OldErrorValue.Quaternion()) * TargetValue.Quaternion().Inverse()).Rotator();

		Cmd.Property->CopyCompleteValue(Out, &Error);
	}


	//Blend out errors
	template<typename InType>
	static void BlendOutError(const FFixedStepCmd& Cmd, void* Out) 
	{
		InType Value;

		Cmd.Property->CopyCompleteValue(&Value, Out);
		Value = Value * Cmd.ErrorBlend;
		Cmd.Property->CopyCompleteValue(Out, &Value);
	}

	template<>
	static void BlendOutError<int32>(const FFixedStepCmd& Cmd, void* Out)
	{
		int32 Value;

		Cmd.Property->CopyCompleteValue(&Value, Out);
		Value = FMath::FloorToInt((float)Value * Cmd.ErrorBlend);
		if (Value <= 1)
		{
			Value = 0;
		}
		Cmd.Property->CopyCompleteValue(Out, &Value);
	}

	template<>
	static void BlendOutError<uint8>(const FFixedStepCmd& Cmd, void* Out)
	{
		int32 Value;

		Cmd.Property->CopyCompleteValue(&Value, Out);
		Value = FMath::FloorToInt((float)Value * Cmd.ErrorBlend);
		if (Value <= 1)
		{
			Value = 0;
		}
		Cmd.Property->CopyCompleteValue(Out, &Value);
	}

	template<>
	static void BlendOutError<FRotator>(const FFixedStepCmd& Cmd, void* Out)
	{
		FRotator Value;

		Cmd.Property->CopyCompleteValue(&Value, Out);
		Value = FQuat::Slerp(Value.Quaternion(), FRotator::ZeroRotator.Quaternion(), Cmd.ErrorBlend).Rotator();
		Cmd.Property->CopyCompleteValue(Out, &Value);
	}


};



struct FIXEDSTEP_API FScopedLagCompensation : private FNoncopyable
{

public:

	UFixedStepComponent* Comp;

	FScopedLagCompensation(UFixedStepComponent* ForComponent) : Comp(NULL)
	{
		//Only allow lag compensation on authority for non authority controlled comps.  Any other call of this should just do nothing.
		if (ForComponent->GetNetRole() == ROLE_Authority && ForComponent->GetNetRemoteRole() < ROLE_Authority)
		{
			Comp = ForComponent;
			Comp->Manager->LagCompensationStart(Comp->GetCurrentFrame() - Comp->GetLastClientInterpDelta(), Comp->GetOwner()->GetNetConnection());
		}
	}

	~FScopedLagCompensation()
	{
		if (Comp)
		{
			Comp->Manager->LagCompensationEnd(Comp->GetOwner()->GetNetConnection());
		}
	}
};
