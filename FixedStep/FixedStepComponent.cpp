// Fill out your copyright notice in the Description page of Project Settings.

#include "FixedStep.h"
#include "FixedStepComponent.h"
#include "FixedStepPlayerComponent.h"

// Sets default values for this component's properties
UFixedStepComponent::UFixedStepComponent()
{
	bReplicates = true;
	LastSimulatedFrame = 0;
	DefaultErrorBlendOut = 0.9f;
	InputRedundancy = 2;
	MaxInputsToSend = 6;
	QueuedInputs = 0;
	AutonomousType = EFixedStepNetworkType::Simulate;
	SimulatedType = EFixedStepNetworkType::Interpolate;
	LastReceivedInputTime = -1.f;
	bFillingBufferedInputs = true;
	SnappedVisualInterpolationFrame = INDEX_NONE;

	DefaultErrorTolerance.Add(EHistoryProp::Default, 0.f);
	DefaultErrorTolerance.Add(EHistoryProp::Bool, 0.f);
	DefaultErrorTolerance.Add(EHistoryProp::Int, 0.f);
	DefaultErrorTolerance.Add(EHistoryProp::Float, 0.01f);
	DefaultErrorTolerance.Add(EHistoryProp::Vector, 1.f);
	DefaultErrorTolerance.Add(EHistoryProp::Rotator, 1.f);


	DebugBufferedInputSize = 0;
	DebugBufferedInputFrames = 0;
}

// Called when the game starts
void UFixedStepComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (!IsInitialized())
	{
		TryFindManager();
	}
}

void UFixedStepComponent::TryFindManager()
{
	if (GetWorld() && GetWorld()->GetGameState())
	{
		UFixedStepManagerComponent* Comp = Cast<UFixedStepManagerComponent>(GetWorld()->GetGameState()->GetComponentByClass(UFixedStepManagerComponent::StaticClass()));

		if (Comp)
		{
			Comp->AddComponent(this);
		}
	}
}

void UFixedStepComponent::BuildHistory()
{
	TArray<FFixedStepEventCmd> OutEvents;

	BuildHistoryForObject(Cast<IFixedStepListenerInterface>(GetOwner()), OutEvents);

	for (UActorComponent* Comp : GetOwner()->GetComponents())
	{
		IFixedStepListenerInterface* FComp = Cast<IFixedStepListenerInterface>(Comp);
		
		if (FComp)
		{
			BuildHistoryForObject(FComp, OutEvents);
		}
	}

	for (FFixedStepCmd& Cmd : History.Commands)
	{
		ensure(Cmd.Property && Cmd.Owner);
		
		Cmd.PropertyType = GetPropertyType(Cmd.Property);

		if (Cmd.ErrorBlend < 0.f)
		{
			Cmd.ErrorBlend = DefaultErrorBlendOut;
		}
		
		if (Cmd.ErrorTolerance < 0.f)
		{
			Cmd.ErrorTolerance = DefaultErrorTolerance[Cmd.PropertyType];
		}

		if (Cmd.ErrorOffset)
		{
			check(Cmd.ErrorOffset->StaticClass() == Cmd.Property->StaticClass()); //Can't have error offset that is different type
		}

		if (Cmd.VisualOffset)
		{
			check(Cmd.VisualOffset->StaticClass() == Cmd.Property->StaticClass()); //Can't have visual property that is different type
		}

		if (Cmd.Type == EHistoryType::State)
		{
			Cmd.Offset = History.StateSize;
			History.StateSize += Cmd.Property->ElementSize;
		}
		else if(Cmd.Type == EHistoryType::Input)
		{
			Cmd.Offset = History.InputSize;
			History.InputSize += Cmd.Property->ElementSize;
			bHasInputs = true;
		}
	}
		
	UE_LOG(LogFixedStep, Verbose, TEXT("History Size=%d Cmds=%d"), History.StateSize, History.Commands.Num());

	for (FFixedStepEventCmd& Event : OutEvents)
	{
		Event.Id = History.Events.Num();
		History.Events.Add(Event.Id, Event);
	}


	for (uint8 Idx = 0; Idx < Manager->GetMaxRollbackFrames(); Idx++)
	{
		History.Buffer.Emplace(History.StateSize, History.InputSize);
	}

	History.PendingFrame = FFixedStepFrame(History.StateSize, History.InputSize);
	History.PreRollbackState = FFixedStepFrame(History.StateSize, History.InputSize);
	History.InterpFrom = FFixedStepFrame(History.StateSize, History.InputSize);
	History.InterpTo = FFixedStepFrame(History.StateSize, History.InputSize);

	FrameData.History = &History;
}

void UFixedStepComponent::BuildHistoryForObject(IFixedStepListenerInterface* Object, TArray<FFixedStepEventCmd>& OutEvents)
{
	check(Object);
	Object->GetFixedStepCommands(History.Commands);
	Object->GetFixedStepEvents(OutEvents);
}

void UFixedStepComponent::RecordFrame(FFixedStepFrame& Frame, int32 FrameNum)
{
	Frame.FrameNumber = FrameNum;

	for (const FFixedStepCmd& Cmd : History.Commands)
	{
		uint8* PropData = Cmd.Property->ContainerPtrToValuePtr<uint8>(Cmd.Owner);
		uint8* ToData = Cmd.Type == EHistoryType::Input ? Frame.InputData.GetData() : Frame.StateData.GetData();

		Cmd.Property->CopyCompleteValue(ToData + Cmd.Offset, PropData);
	}

	if (WantSendInput())
	{
		Frame.bClientInput = true;
	}
}

void UFixedStepComponent::RecordHistory(int32 FrameNum)
{
	FFixedStepFrame& Frame = History.GetFrame(FrameNum);	
	RecordFrame(Frame, FrameNum);
}

void UFixedStepComponent::SetToFrame(FFixedStepFrame& Frame, bool bInputOnly)
{
	for (const FFixedStepCmd& Cmd : History.Commands)
	{
		if (bInputOnly && Cmd.Type != EHistoryType::Input) continue;

		uint8* PropData = Cmd.Property->ContainerPtrToValuePtr<uint8>(Cmd.Owner);
		uint8* FromData = Cmd.Type == EHistoryType::Input ? Frame.InputData.GetData() : Frame.StateData.GetData();

		Cmd.Property->CopyCompleteValue(PropData, FromData + Cmd.Offset);
	}
	
	//We should probably split out set state and set input instead of containing in a single function like this.
	if (!bInputOnly)
	{
		for (IFixedStepListenerInterface* Responder : Responders)
		{
			Responder->SetFrame(Frame.FrameNumber);
		}
	}
}

void UFixedStepComponent::SetToFrame(int32 FrameNum, bool bInputOnly)
{
	if (History.HasFrame(FrameNum))
	{
		SetToFrame(History.GetFrame(FrameNum), bHasInputs);
	}
	else
	{
		UE_LOG(LogFixedStep, Warning, TEXT("Failed to SetToFrame - frame %d not found."), FrameNum);
	}
}

void UFixedStepComponent::SetToHistory(int32 FrameNum, bool bInputOnly)
{
	FFixedStepFrame& Frame = History.GetFrame(FrameNum);

	//check(Frame->FrameNumber == FrameNum);

	if (Frame.FrameNumber < FrameNum && GetOwner()->Role < ROLE_Authority) //Don't copy forward for server.
	{
		int32 MinFrame = FMath::Max(FrameNum - Manager->GetMaxRollbackFrames() - 1, 0);

		//If we try to set to a history frame that hasn't been written yet (client advances forward but hasnt received any frames for this yet), copy last simulated frame forward and backward until we get to this frame
		for (int32 PrevFrame = FrameNum - 1; PrevFrame >= MinFrame; PrevFrame--)
		{
			if (History.HasFrame(PrevFrame) || PrevFrame == MinFrame)
			{
				FFixedStepFrame& MostRecentFrame = History.GetFrame(PrevFrame);
				MostRecentFrame.FrameNumber = PrevFrame;

				for (int32 NextFrame = MinFrame; NextFrame <= FrameNum; NextFrame++)
				{
					if (NextFrame == PrevFrame) continue;

					History.GetFrame(NextFrame).Copy(MostRecentFrame, false);
					History.GetFrame(NextFrame).FrameNumber = NextFrame;
				}

				break;
			}
		}
	}
	
	SetToFrame(Frame, bInputOnly);
}

void UFixedStepComponent::SnapVisualInterpolation()
{
	SnappedVisualInterpolationFrame = GetCurrentFrame();
}

void UFixedStepComponent::RecordCurrentFrame()
{
	RecordHistory(GetCurrentFrame());
}

void UFixedStepComponent::Init(UFixedStepManagerComponent* InManager)
{
	Manager = InManager;
		
	BuildHistory();

	if (GetOwner()->Role < ROLE_Authority && FrameData.FrameNum > INDEX_NONE)
	{
		OnRep_FrameData();
	}
}

void UFixedStepComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);

	if (Manager)
	{
		Manager->RemoveComponent(this);
	}
}

void UFixedStepComponent::RegisterObserver(UObject* Responder)
{
	IFixedStepListenerInterface* IResponder = Cast<IFixedStepListenerInterface>(Responder);
	ensure(IResponder);

	Responders.AddUnique(IResponder);
}

void UFixedStepComponent::UnregisterObserver(UObject* Responder)
{
	IFixedStepListenerInterface* IResponder = Cast <IFixedStepListenerInterface>(Responder);
	ensure(IResponder);

	Responders.Remove(IResponder);
}

TEnumAsByte<ENetRole> UFixedStepComponent::GetNetRole() const
{
	//Currently we consider non-replicated actors to always be authority, but we should maybe rethink this logic
	if (!GetOwner()->GetIsReplicated())
	{
		return ROLE_Authority;
	}

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (Pawn->Controller)
		{
			return Pawn->Controller->Role;
		}
	}

	if (GetOwner()->GetNetOwner())
	{
		return GetOwner()->GetNetOwner()->Role;
	}
	else
	{
		return GetOwner()->Role;
	}
}

TEnumAsByte<ENetRole> UFixedStepComponent::GetNetRemoteRole() const
{
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (Pawn->Controller)
		{
			return Pawn->Controller->GetRemoteRole();
		}
	}

	if (GetOwner()->GetNetOwner())
	{
		return GetOwner()->GetNetOwner()->GetRemoteRole();
	}
	else
	{
		return GetOwner()->GetRemoteRole();
	}
}

const AController* UFixedStepComponent::GetOwningController() const
{
	const AActor* NetOwner = GetOwner()->GetNetOwner();

	if (const APawn* PawnOwner = Cast<const APawn>(NetOwner))
	{
		return PawnOwner->Controller;
	}

	return Cast<const AController>(NetOwner);
}

TEnumAsByte<EFixedStepNetworkType::Type> UFixedStepComponent::GetNetworkType() const
{
	switch (GetNetRole())
	{
	case ROLE_Authority:
		return EFixedStepNetworkType::Simulate;
	case ROLE_AutonomousProxy:
		return AutonomousType;
	case ROLE_SimulatedProxy:
		return SimulatedType;
	}

	return EFixedStepNetworkType::Simulate;
}

int32 UFixedStepComponent::GetCurrentFrame() const
{
	if (Manager)
	{
		return Manager->GetCurrentFrame();
	}

	return 0;
}

bool UFixedStepComponent::IsLocallyControlled() const
{
	const ENetMode NetMode = GetNetMode();

	//Currently non-replicated owners should always be considered locally controller, but we may want to rethink this logic as with the netrole above
	if (!GetOwner()->GetIsReplicated())
	{
		return true;
	}

	if (NetMode == NM_Standalone)
	{
		// Not networked.
		return true;
	}

	if (NetMode == NM_Client && GetNetRole() == ROLE_AutonomousProxy)
	{
		// Networked client in control.
		return true;
	}

	if (GetNetRemoteRole() != ROLE_AutonomousProxy && GetNetRole() == ROLE_Authority)
	{
		// Local authority in control.
		return true;
	}
	
	return false;
}

bool UFixedStepComponent::WantSendInput() const
{	
	return bHasInputs && IsLocallyControlled() && GetNetRole() != ROLE_Authority;
}

void UFixedStepComponent::OnRep_FrameData()
{
	if (!IsInitialized()) return;
			
	if (GetNetworkType() == EFixedStepNetworkType::Simulate)
	{
		//TEMP- need a better way of marking pending frames as valid
		History.PendingFrame.bSet = true;

		//If we are the "owner" of this predicted replicated actor we can assume that any frame we received from the server includes the last input frame number the server has processed from us.
		if (IsLocallyControlled() && FrameData.ClientInputFrame > 0)
		{
			//See if we require a rollback
			FFixedStepFrame& OldFrame = History.GetFrame(FrameData.ClientInputFrame);
			//ensure(FrameData.ClientInputFrame == OldFrame.FrameNumber); //We should probably do something if our response from the server was too old.

			for (FFixedStepCmd& Cmd : History.Commands)
			{
				if (Cmd.Type == EHistoryType::State && !WithinTolerance(Cmd, OldFrame, History.PendingFrame))
				{
					UE_LOG(LogFixedStepError, Verbose, TEXT("%s property %s was not within error tolerance."), *GetOwner()->GetName(), *Cmd.Property->GetName());
					Manager->SetNetDirtyFrame(FrameData.ClientInputFrame);
					break;
				}
			}
		}
	}
	else if(GetNetworkType() == EFixedStepNetworkType::Interpolate)
	{
		FFixedStepFrame& Frame = History.GetFrame(GetCurrentFrame());
		Frame.Copy(History.PendingFrame, false);
		Frame.FrameNumber = GetCurrentFrame();
		Manager->ReceivedInterpFrame();
		
		UE_LOG(LogFixedStepInterp, Verbose, TEXT("%s Interp Received Frame: %d"), *GetOwner()->GetName(), Frame.FrameNumber);
	}

	//Manager->SetNetDirtyFrame(FrameData.FrameNum);
}

bool UFixedStepComponent::HasInputForFrame(int32 FrameNum) const
{
	return History.HasFrame(FrameNum) && History.GetFrameConst(FrameNum).bClientInput;
}

void UFixedStepComponent::GatherInputs(int32 FrameNum, float DeltaTime, bool bRollback)
{
	if (GetNetworkType() != EFixedStepNetworkType::Simulate) return;
	if (!bHasInputs) return;

	//Only gather inputs for owners of this
	if (!bRollback)
	{
		if (!IsLocallyControlled() && GetNetRole() == ROLE_Authority)
		{
			//Server needs to set the clients inputs it received on simulation
			const int32 BufferSize = Manager->GetJitterFrames(GetOwner()->GetNetConnection());

			if (!bFillingBufferedInputs && BufferedInputs.Num() > 0 || BufferedInputs.Num() > BufferSize) //If the buffer is not full only grab every other input until its full
			{
				bFillingBufferedInputs = false;
				if (BufferedInputs.Num() > BufferSize + 1) //the + 1 stops the some fluctuation of being over/under buffer size.  Probably should improve this
				{
					UE_LOG(LogFixedStepInput, Log, TEXT("Buffer was too large - %d / %d - dropping an input"), BufferedInputs.Num(), BufferSize);
					//Trim oldest inputs if out buffer is too large. Currently only trim one frame at a time so we dont cut off huge amounts of inputs at once.
					//TODO: this might be too aggressive in removing inputs at times.  For times when the buffer is only slightly too large consider overwatch dialation technique
					BufferedInputs.RemoveAt(0);
				}

				LastClientInputFrame = BufferedInputs[0];
				SetToFrame(LastClientInputFrame, true);
				BufferedInputs.RemoveAt(0);
			}
			else if (LastClientInputFrame.FrameNumber > INDEX_NONE)
			{
				UE_LOG(LogFixedStepInput, Log, TEXT("Filling buffer - %d / %d"), BufferedInputs.Num(), BufferSize);
				LastClientInputFrame.FrameNumber++; //Not sure where this should be
				bFillingBufferedInputs = true;
			}

			DebugBufferedInputSize = BufferSize;
			DebugBufferedInputFrames = BufferedInputs.Num();

			Manager->SetLastInputFrame(GetOwner()->GetNetConnection(), LastClientInputFrame.FrameNumber);
		}
		else if (IsLocallyControlled() && GetNetRole() == ROLE_Authority)
		{
			LastClientInputFrame.FrameNumber = GetCurrentFrame();
		}

		if (IsLocallyControlled())
		{
			for (IFixedStepListenerInterface* Responder : Responders)
			{
				Responder->GatherInputs(FrameNum);
			}
		}
	}
	else if (HasInputForFrame(FrameNum))
	{
		//Rollbacks we should set our input if it exists. Non-owning clients may or may not have input for this frame. 
		SetToFrame(FrameNum, true);
	}
}

void UFixedStepComponent::Simulate(int32 FrameNum, float DeltaTime, bool bRollback)
{	
	if (GetNetworkType() != EFixedStepNetworkType::Simulate) return;
	
	//TODO, make new frame recording part of a system that using FScopedFixedStepFrame that automatically does all of this. Or maybe make a BeginNewFrame, EndNewFrame type of system so simulate is not tied to recording.
		
	InternalSimulate(FrameNum, DeltaTime, bRollback);

	//Record history after simulating
	RecordHistory(FrameNum);

	if (WantSendInput() && !bRollback)
	{
		QueuedInputs++;
	}
		
	if (!bRollback)
	{
		//Blend out any errors
		for (const FFixedStepCmd& Cmd : History.Commands)
		{
			if (Cmd.ErrorOffset)
			{
				BlendOutError(Cmd);
			}
		}
	}
}

void UFixedStepComponent::InternalSimulate(int32 FrameNum, float DeltaTime, bool bRollback)
{	
	check(IsInitialized());
		
	for (IFixedStepListenerInterface* Responder : Responders)
	{
		Responder->Simulate(FrameNum, DeltaTime, bRollback);
	}
		
	LastSimulatedFrame = FrameNum;
}

void UFixedStepComponent::PostSimulate(int32 StartFrame, int32 EndFrame, float Alpha)
{
	if (GetNetworkType() != EFixedStepNetworkType::Simulate) return;
	
	//Alpha blend
	FFixedStepFrame* CurFrame = &History.GetFrame(EndFrame);
	FFixedStepFrame* PrevFrame = CurFrame;
	
	if (EndFrame > 0 && EndFrame > SnappedVisualInterpolationFrame && History.HasFrame(EndFrame - 1) && !Manager->bDisableVisualInterpolation)
	{
		PrevFrame = &History.GetFrame(EndFrame - 1);
	}

	for (const FFixedStepCmd& Cmd : History.Commands)
	{
		if (Cmd.VisualOffset)
		{
			uint8* PropertyData = Cmd.Property->ContainerPtrToValuePtr<uint8>(Cmd.Owner);
			uint8* VisualData = Cmd.VisualOffset->ContainerPtrToValuePtr<uint8>(Cmd.Owner);
			InterpProperty(Cmd, *PrevFrame, *CurFrame, Alpha, VisualData);
			GetOffset(Cmd, PropertyData, VisualData, VisualData);
			//If we have an error property as well we can combine the error with the visual property to get a final visual value

			if (Cmd.ErrorOffset)
			{
				uint8* ErrorData = Cmd.ErrorOffset->ContainerPtrToValuePtr<uint8>(Cmd.Owner);

				switch (Cmd.PropertyType)
				{
					case EHistoryProp::Int:
					{
						int32 Visual;
						int32 Error;
						int32 Combined;
						
						Cmd.VisualOffset->CopyCompleteValue(&Visual, VisualData);
						Cmd.ErrorOffset->CopyCompleteValue(&Error, ErrorData);
						Combined = Visual + Error;
						Cmd.VisualOffset->CopyCompleteValue(VisualData, &Combined);
						break;
					}
					case EHistoryProp::Float:
					{
						float Visual;
						float Error;
						float Combined;

						Cmd.VisualOffset->CopyCompleteValue(&Visual, VisualData);
						Cmd.ErrorOffset->CopyCompleteValue(&Error, ErrorData);
						Combined = Visual + Error;
						Cmd.VisualOffset->CopyCompleteValue(VisualData, &Combined);
						break;
					}
					case EHistoryProp::Vector:
					{
						FVector Visual;
						FVector Error;
						FVector Combined;

						Cmd.VisualOffset->CopyCompleteValue(&Visual, VisualData);
						Cmd.ErrorOffset->CopyCompleteValue(&Error, ErrorData);
						Combined = Visual + Error;
						Cmd.VisualOffset->CopyCompleteValue(VisualData, &Combined);
						break;
					}
					case EHistoryProp::Rotator:
					{
						FRotator Visual;
						FRotator Error;
						FRotator Combined;

						Cmd.VisualOffset->CopyCompleteValue(&Visual, VisualData);
						Cmd.ErrorOffset->CopyCompleteValue(&Error, ErrorData);
						Combined = (Visual.Quaternion() * Error.Quaternion()).Rotator();
						Cmd.VisualOffset->CopyCompleteValue(VisualData, &Combined);
						break;
					}
				}
			}
		
		}
	}
		
	DispatchEvents(StartFrame, EndFrame);

	if (WantSendInput())
	{
		SendInputs();
	}


	for (IFixedStepListenerInterface* Responder : Responders)
	{
		Responder->PostSimulate(StartFrame, EndFrame, Alpha);
	}
}

void UFixedStepComponent::RollbackStart(int32 FrameNum)
{
	if (GetNetworkType() != EFixedStepNetworkType::Simulate) return;
	

	//If there is a pending net dirty frame in our history set it to the incoming frame in the buffer before applying so we have the latest server frame
	FFixedStepFrame& OldFrame = History.GetFrame(FrameNum);

	if (History.PendingFrame.bSet)
	{
		OldFrame.FrameNumber = FrameNum;
		OldFrame.Copy(History.PendingFrame, false);
		History.PendingFrame.bSet = false; //Clear this so we don't attempt to use it again
	}

	//Actually set our frame values.
	SetToFrame(OldFrame);

	//Get our most recently simulated frame and store as a pre-rollback state so we can check for errors after resimulating
	FFixedStepFrame& CurState = History.GetFrame(LastSimulatedFrame);
	ensure(CurState.FrameNumber == LastSimulatedFrame);
	History.PreRollbackState.Copy(CurState);
	
	//Alert we've started rollback
	for (IFixedStepListenerInterface* Responder : Responders)
	{
		Responder->RollbackStart(FrameNum);
	}
}

void UFixedStepComponent::RollbackComplete()
{
	if (GetNetworkType() != EFixedStepNetworkType::Simulate) return;

	FFixedStepFrame& PrevState = History.PreRollbackState;
	FFixedStepFrame& CurState = History.GetFrame(LastSimulatedFrame);

	if (PrevState.FrameNumber == CurState.FrameNumber && !Manager->bDisableErrorCorrection)
	{
		//Find errors
		for (const FFixedStepCmd& Cmd : History.Commands)
		{
			if (Cmd.ErrorOffset)
			{
				GetError(Cmd, PrevState, CurState);
			}
		}
	}


	for (IFixedStepListenerInterface* Responder : Responders)
	{
		Responder->RollbackComplete(LastSimulatedFrame);
	}
}

void UFixedStepComponent::LagCompensationStart(int32 FrameNum)
{
	ensure(FrameNum >= LastSimulatedFrame - Manager->GetMaxRollbackFrames());
	SetToFrame(History.GetFrame(FrameNum));
}

void UFixedStepComponent::LagCompensationEnd()
{
	SetToFrame(History.GetFrame(LastSimulatedFrame));
}

void UFixedStepComponent::Interpolate(int32 FromFrameNum, int32 ToFrameNum, float Alpha, float DeltaTime)
{
	if (GetNetworkType() != EFixedStepNetworkType::Interpolate) return;

	FFixedStepFrame& FromFrame = History.GetFrameOrBestBefore(FromFrameNum);
	FFixedStepFrame& ToFrame = History.GetFrameOrBestAfter(ToFrameNum);
	
	if (FromFrame.FrameNumber != INDEX_NONE && FromFrame.FrameNumber <= FromFrameNum && ToFrame.FrameNumber >= ToFrameNum)
	{
		float ActualAlpha = ((float)FromFrameNum - (float)FromFrame.FrameNumber + Alpha) / ((float)ToFrame.FrameNumber - (float)FromFrame.FrameNumber);

		for (FFixedStepCmd& Cmd : History.Commands)
		{
			uint8* Data = Cmd.Property->ContainerPtrToValuePtr<uint8>(Cmd.Owner);
			InterpProperty(Cmd, FromFrame, ToFrame, ActualAlpha, Data);
		}

		UE_LOG(LogFixedStepInterp, Verbose, TEXT("%s Interp from found frames (%d, %d) - (%d, %d) with alpha: (%f, %f)"), *GetOwner()->GetName(), FromFrameNum, FromFrame.FrameNumber, ToFrameNum, ToFrame.FrameNumber, Alpha, ActualAlpha);
	}
	else if (FromFrame.FrameNumber != INDEX_NONE && FromFrame.FrameNumber <= FromFrameNum)
	{
		int32 MissingFrames = ToFrameNum - FromFrame.FrameNumber;

		SetToFrame(FromFrame);

		int32 SimulatedFrame = FromFrame.FrameNumber + 1;
				
		while (SimulatedFrame <= ToFrameNum)
		{			
			InternalSimulate(SimulatedFrame, Manager->TickRate, false);
			
			if (SimulatedFrame + 1 == ToFrameNum)
			{
				RecordFrame(History.InterpFrom, SimulatedFrame);
			}
			else if (SimulatedFrame == ToFrameNum)
			{
				RecordFrame(History.InterpTo, SimulatedFrame);
			}			
			
			SimulatedFrame++;	
		}
		
		check(History.InterpTo.FrameNumber == ToFrameNum);

		for (FFixedStepCmd& Cmd : History.Commands)
		{
			uint8* Data = Cmd.Property->ContainerPtrToValuePtr<uint8>(Cmd.Owner);
			InterpProperty(Cmd, MissingFrames == 1 ? FromFrame : History.InterpFrom, History.InterpTo, Alpha, Data);
		}
		
		UE_LOG(LogFixedStepInterp, Verbose, TEXT("%s Interp failed to find valid 'to frame', simulating to catch up - ExpectedFrom: %d FoundFrom: %d, To:%d"), *GetOwner()->GetName(), FromFrameNum, FromFrame.FrameNumber, ToFrameNum);
	}
	else
	{
		UE_LOG(LogFixedStepInterp, Verbose, TEXT("%s Interp failed to found valid frames for %d - %d"), *GetOwner()->GetName(), FromFrameNum, ToFrameNum);
	}
	
	for (IFixedStepListenerInterface* Responder : Responders)
	{
		Responder->Interpolate(FromFrameNum, ToFrameNum, Alpha, DeltaTime);
	}

	//Fire off events from between the interpolation frames...not sure if this should be the from or to frame for events?
	DispatchEvents(FromFrameNum, ToFrameNum);
}

void UFixedStepComponent::QueueEvent(UObject* Owner, UScriptStruct* Type, const FFixedStepEventBase* Event)
{
	check(GetNetRole() >= ROLE_AutonomousProxy);

	for (auto EventPair : History.Events) 
	{
		FFixedStepEventCmd& Cmd = EventPair.Value;

		if (Cmd.Owner == Owner && Cmd.Type == Type)
		{
			//Build event data
			FFixedStepEventData PendingEvent(Cmd.Type, GetCurrentFrame(), Cmd.Id);
			PendingEvent.Data.SetNumZeroed(Cmd.Type->GetStructureSize());
			Cmd.Type->CopyScriptStruct(PendingEvent.Data.GetData(), Event);
			
			//Server will send this event out to all clients
			if (GetNetRole() == ROLE_Authority)
			{
				/*FNetBitWriter Writer(NULL, 0); //Need to explore new ways to serialize this data as the net writer will fail on FNames and UObject references since the package map is null
				
				Cmd.Type->SerializeItem(Writer, (void*)Event, NULL);

				TArray<uint8> EventData;
				EventData.SetNumUninitialized(Writer.GetNumBytes());
				FMemory::Memcpy(EventData.GetData(), Writer.GetData(), Writer.GetNumBytes());
				
				MulticastSendEvent(GetCurrentFrame(), Cmd.Id, EventData);*/
				MulticastSendEvent(GetCurrentFrame(), Cmd.Id, PendingEvent);
			}

			PendingEvents.Add(PendingEvent);
		}
	}
}

void UFixedStepComponent::MulticastSendEvent_Implementation(int32 FrameNum, uint8 EventId, FFixedStepEventData Event)
{
	if (GetNetRole() > ROLE_SimulatedProxy)
	{
		return; // Currently we only need multicast events for interolated/non-controlled clients as they are not really simulating
	}

	Event.Id = EventId;
	Event.FrameNum = GetCurrentFrame();

	PendingEvents.Add(Event);
}

void UFixedStepComponent::DispatchEvents(int32 FromFrame, int32 ToFrame)
{	
	for (FFixedStepEventData& EventData : PendingEvents)
	{
		if (EventData.FrameNum < FromFrame || EventData.FrameNum > ToFrame) continue;
		
		FFixedStepEventCmd& Cmd = History.Events[EventData.Id];
			
		FFrame NewStack(Cmd.Owner, Cmd.Callback, EventData.Data.GetData(), NULL, Cmd.Callback->Children);
		Cmd.Owner->CallFunction(NewStack, nullptr, Cmd.Callback);	
	}
	
	PendingEvents.RemoveAll([ToFrame](FFixedStepEventData& Data)
	{
		return Data.FrameNum <= ToFrame;
	});
}

void UFixedStepComponent::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	//Server records for replication
	if (GetOwner()->Role == ROLE_Authority && IsInitialized())
	{
		FrameData.LatestFrame = &History.GetFrame(LastSimulatedFrame);
		FrameData.FrameNum = GetCurrentFrame();
		FrameData.ClientInputFrame = Manager->GetLastInputFrame(GetOwner()->GetNetConnection()); //LastClientInputFrame.FrameNumber; //Set the frame num value to the last processed client input frame number.  The client who receives this frame data will know that we the server processed this input and can resimulate locally from there
	}

	Super::PreReplication(ChangedPropertyTracker);
}

void UFixedStepComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFixedStepComponent, FrameData);
}

void UFixedStepComponent::SendInputs()
{
	check(GetOwner()->Role == ROLE_AutonomousProxy);
	check(History.InputSize > 0);

	QueuedInputs = FMath::Min(QueuedInputs, MaxInputsToSend);

	uint8 ActualInputs = 0;

	FNetBitWriter Writer = FNetBitWriter(NULL, 0);

	for (int32 FrameNum = LastSimulatedFrame; FrameNum > LastSimulatedFrame - QueuedInputs; FrameNum--)
	{
		FFixedStepFrame& Frame = History.GetFrame(FrameNum);
		if (Frame.FrameNumber != FrameNum)
		{
			//We don't have an input for a required frame.  We can just skip this replication
			return;
		}

		for (const FFixedStepCmd& Cmd : History.Commands)
		{
			if (Cmd.Type == EHistoryType::Input)
			{
				uint8* Data = Cmd.Property->ContainerPtrToValuePtr<uint8>(Cmd.Owner);
				Cmd.Property->NetSerializeItem(Writer, NULL, Data);
			}
		}

		//This is probably wrong...this could unsync our inputs from client to server...
		ActualInputs++;
	}

	TArray<uint8> InputData;
	InputData.SetNumUninitialized(Writer.GetNumBytes());
	FMemory::Memcpy(InputData.GetData(), Writer.GetData(), Writer.GetNumBytes());
	
	ServerSendInputs(ActualInputs, InputData, LastSimulatedFrame, Manager->GetInterpolatedFrames(), FCrc::MemCrc32(InputData.GetData(), sizeof(uint8) * InputData.Num()));

	QueuedInputs = 0; //Maybe use redundancy here
}

void UFixedStepComponent::ServerSendInputs_Implementation(uint8 NumInputs, const TArray<uint8>& Data, int32 FrameNum, int32 InterpDelta, uint32 Checksum)
{
	if (FrameNum - (NumInputs - 1) < LastClientInputFrame.FrameNumber)
	{
		UE_LOG(LogFixedStepInput, Warning, TEXT("Received client inputs that are older than the last one we processed - Received: %d to %d LastProcessed: %d"), FrameNum - (NumInputs - 1), FrameNum, LastClientInputFrame.FrameNumber);
	}
	
	FNetBitReader Reader(NULL, (uint8*)Data.GetData(), Data.Num() * 8);
		
	for (int32 Idx = 0; Idx < NumInputs; Idx++)
	{
		FFixedStepFrame& Frame = BufferedInputs[BufferedInputs.Emplace(0, History.InputSize)];
		Frame.FrameNumber = FrameNum - (NumInputs - 1) + Idx;
		Frame.bClientInput = true;
		Frame.ClientInterpDelta = InterpDelta;
		uint8* InputData = Frame.InputData.GetData();

		for (const FFixedStepCmd& Cmd : History.Commands)
		{
			if (Cmd.Type == EHistoryType::Input)
			{
				Cmd.Property->NetSerializeItem(Reader, NULL, InputData + Cmd.Offset);
			}
		}
	}

	BufferedInputs.Sort([](const FFixedStepFrame& A, const FFixedStepFrame& B)
	{
		return A.FrameNumber < B.FrameNumber;
	});

	if (LastReceivedInputTime > 0.f)
	{
		Manager->AddJitterDelta(GetOwner()->GetNetConnection(), GetWorld()->GetRealTimeSeconds() - LastReceivedInputTime);	
	}

	LastReceivedInputTime = GetWorld()->GetRealTimeSeconds();

	UE_LOG(LogFixedStepInput, Verbose, TEXT("Received client inputs - ClientFrame=%d NumInputs=%d BufferedInputs=%d"), FrameNum, NumInputs, BufferedInputs.Num());
}

bool UFixedStepComponent::ServerSendInputs_Validate(uint8 NumInputs, const TArray<uint8>& Data, int32 FrameNum, int32 InterpDelta, uint32 Checksum)
{
	return true;
}

EHistoryProp::Type UFixedStepComponent::GetPropertyType(UProperty* Prop) const
{
	if (Prop->IsA( UFloatProperty::StaticClass()))
	{
		return EHistoryProp::Float;
	}
	else if (Prop->IsA(UIntProperty::StaticClass()))
	{
		return EHistoryProp::Int;
	}
	else if (Prop->IsA(UBoolProperty::StaticClass()))
	{
		return EHistoryProp::Bool;
	}
	else if (Prop->IsA(UStructProperty::StaticClass()))
	{
		UStructProperty * StructProp = Cast< UStructProperty >(Prop);
		UScriptStruct * Struct = StructProp->Struct;

		if (Struct->GetFName() == NAME_Vector)
		{
			return EHistoryProp::Vector;
		}
		else if (Struct->GetFName() == NAME_Rotator)
		{
			return EHistoryProp::Rotator;
		}
	}

	return EHistoryProp::Default;
}

void UFixedStepComponent::InterpProperty(const FFixedStepCmd& Cmd, FFixedStepFrame& From, FFixedStepFrame& To, float Alpha, void* OutData)
{
	uint8* FromData = Cmd.Type == EHistoryType::Input ? From.InputData.GetData() : From.StateData.GetData();
	FromData += Cmd.Offset;

	uint8* ToData = Cmd.Type == EHistoryType::Input ? To.InputData.GetData() : To.StateData.GetData();
	ToData += Cmd.Offset;

	switch (Cmd.PropertyType)
	{
	case EHistoryProp::Int:
		FFixedStepOp::Interp<int32>(Cmd, FromData, ToData, Alpha, OutData);
		break;
	case EHistoryProp::Float:		
		FFixedStepOp::Interp<float>(Cmd, FromData, ToData, Alpha, OutData);
		break;
	case EHistoryProp::Vector:
		FFixedStepOp::Interp<FVector>(Cmd, FromData, ToData, Alpha, OutData);
		break;
	case EHistoryProp::Rotator:
		FFixedStepOp::Interp<FRotator>(Cmd, FromData, ToData, Alpha, OutData);
		break;
	default:
		FFixedStepOp::Interp(Cmd, FromData, ToData, Alpha, OutData);
	}

	
}

void UFixedStepComponent::GetOffset(const FFixedStepCmd& Cmd, uint8* From, uint8* To, void* OutData)
{
	switch (Cmd.PropertyType)
	{
	case EHistoryProp::Int:
		FFixedStepOp::GetOffset<int32>(Cmd, From, To, OutData);
		break;
	case EHistoryProp::Float:
		FFixedStepOp::GetOffset<float>(Cmd, From, To, OutData);
		break;
	case EHistoryProp::Vector:
		FFixedStepOp::GetOffset<FVector>(Cmd, From, To, OutData);
		break;
	case EHistoryProp::Rotator:
		FFixedStepOp::GetOffset<FRotator>(Cmd, From, To, OutData);
		break;
	}
}

void UFixedStepComponent::GetError(const FFixedStepCmd& Cmd, FFixedStepFrame& PrevFrame, FFixedStepFrame& CurFrame)
{
	uint8* PrevData = PrevFrame.StateData.GetData();
	PrevData += Cmd.Offset;

	uint8* CurData = CurFrame.StateData.GetData();
	CurData += Cmd.Offset;

	uint8* ErrorData = Cmd.ErrorOffset->ContainerPtrToValuePtr<uint8>(Cmd.Owner);

	if (Cmd.ErrorOffset)
	{
		switch (Cmd.PropertyType)
		{
		case EHistoryProp::Int:
			FFixedStepOp::GetError<int32>(Cmd, PrevData, CurData, ErrorData);
			break;
		case EHistoryProp::Float:
			FFixedStepOp::GetError<float>(Cmd, PrevData, CurData, ErrorData);
			break;
		case EHistoryProp::Vector:
			FFixedStepOp::GetError<FVector>(Cmd, PrevData, CurData, ErrorData);
			break;
		case EHistoryProp::Rotator:
			FFixedStepOp::GetError<FRotator>(Cmd, PrevData, CurData, ErrorData);
			break;
		}
	}
}

void UFixedStepComponent::BlendOutError(const FFixedStepCmd& Cmd)
{
	uint8* ErrorData = Cmd.ErrorOffset->ContainerPtrToValuePtr<uint8>(Cmd.Owner);

	switch (Cmd.PropertyType)
	{
	case EHistoryProp::Int:
		FFixedStepOp::BlendOutError<int32>(Cmd, ErrorData);
		break;
	case EHistoryProp::Float:
		FFixedStepOp::BlendOutError<float>(Cmd, ErrorData);
		break;
	case EHistoryProp::Vector:
		FFixedStepOp::BlendOutError<FVector>(Cmd, ErrorData);
		break;
	case EHistoryProp::Rotator:
		FFixedStepOp::BlendOutError<FRotator>(Cmd, ErrorData);
		break;
	default:
		//Forgot to implement this type
		check(false);
	}
}

bool UFixedStepComponent::WithinTolerance(const FFixedStepCmd& Cmd, FFixedStepFrame& From, FFixedStepFrame& To)
{
	uint8* FromData = Cmd.Type == EHistoryType::State ? From.StateData.GetData() + Cmd.Offset : From.InputData.GetData() + Cmd.Offset;
	uint8* ToData = Cmd.Type == EHistoryType::State ? To.StateData.GetData() + Cmd.Offset : To.InputData.GetData() + Cmd.Offset;

	switch (Cmd.PropertyType)
	{
	case EHistoryProp::Int:
		return FFixedStepOp::WithinTolerance<int32>(Cmd, FromData, ToData);
	case EHistoryProp::Float:
		return FFixedStepOp::WithinTolerance<float>(Cmd, FromData, ToData);
	case EHistoryProp::Vector:
		return FFixedStepOp::WithinTolerance<FVector>(Cmd, FromData, ToData);
	case EHistoryProp::Rotator:
		return FFixedStepOp::WithinTolerance<FRotator>(Cmd, FromData, ToData);
	default:
		return FFixedStepOp::WithinTolerance(Cmd, FromData, ToData);
	}
}

bool FFixedStepRepData::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	if (Ar.IsSaving())
	{
		if (LatestFrame)
		{
			//Record for replication
			//FrameNum = LatestFrame->FrameNumber; //Dont record frame number...we set this value to the last client input frame used which does not equal our history frame number

			FNetBitWriter InputWriter(Map, 0);
			FNetBitWriter StateWriter(Map, 0);
			
			for (const FFixedStepCmd& Cmd : History->Commands)
			{
				uint8* Data = Cmd.Property->ContainerPtrToValuePtr<uint8>(Cmd.Owner);
				Cmd.Property->NetSerializeItem(Cmd.Type == EHistoryType::Input ? InputWriter : StateWriter, Map, Data);
			}

			StateData.SetNumUninitialized(StateWriter.GetNumBytes(), true);
			FMemory::Memcpy(StateData.GetData(), StateWriter.GetData(), StateWriter.GetNumBytes());


			InputData.SetNumUninitialized(InputWriter.GetNumBytes(), true);
			FMemory::Memcpy(InputData.GetData(), InputWriter.GetData(), InputWriter.GetNumBytes());

			Checksum = FCrc::MemCrc32(StateData.GetData(), sizeof(uint8) * StateData.Num());
		}

	}

	Ar << FrameNum;
	Ar << ClientInputFrame;
	SafeNetSerializeTArray_Default<1024>(Ar, StateData);
	SafeNetSerializeTArray_Default<1024>(Ar, InputData);
	Ar << Checksum;

	if (Ar.IsLoading())
	{
		//Validate - this eventually needs to be fixed somehow as this is a bug https://issues.unrealengine.com/issue/UE-38148
		if (Checksum != FCrc::MemCrc32(StateData.GetData(), sizeof(uint8) * StateData.Num()))
		{
			return true;
		}

		//Cant really do anything until we have a history
		if (!History) return true;

		FFixedStepFrame& Frame = History->PendingFrame;
		Frame.FrameNumber = ClientInputFrame; //Right now we are setting this to the client input frame sent from the server...this doesn't really have much use outside of clients determining rollback state for predicted components.

		FNetBitReader InputReader(Map, InputData.GetData(), InputData.Num() * 8);
		FNetBitReader StateReader(Map, StateData.GetData(), StateData.Num() * 8);

		for (const FFixedStepCmd& Cmd : History->Commands)
		{
			uint8* Data = Cmd.Type == EHistoryType::Input ? Frame.InputData.GetData() : Frame.StateData.GetData();
			Data += Cmd.Offset;
			Cmd.Property->NetSerializeItem(Cmd.Type == EHistoryType::Input ? InputReader : StateReader, Map, Data);
		}
	}

	return true;	
}

bool FFixedStepEventData::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Type;

	if (Type)
	{
		if (Ar.IsLoading())
		{
			Data.SetNumUninitialized(Type->GetStructureSize());
		}

		uint8* DataPtr = Data.GetData();

		if (Type->StructFlags & STRUCT_NetSerializeNative)
		{
			Type->GetCppStructOps()->NetSerialize(Ar, Map, bOutSuccess, DataPtr);
		}
		else
		{
			// This won't work since UStructProperty::NetSerializeItem is deprecrated.
			//	1) we have to manually crawl through the topmost struct's fields since we don't have a UStructProperty for it (just the UScriptProperty)
			//	2) if there are any UStructProperties in the topmost struct's fields, we will assert in UStructProperty::NetSerializeItem.

			for (TFieldIterator<UProperty> It(Type); It; ++It)
			{
				if (It->PropertyFlags & CPF_RepSkip)
				{
					continue;
				}

				void* PropertyData = It->ContainerPtrToValuePtr<void*>(DataPtr);

				It->NetSerializeItem(Ar, Map, PropertyData);
			}
		}
	}

	return bOutSuccess;
}
