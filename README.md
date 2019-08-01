# FixedStepNetworking

A first attempt at fixed step simulation and networking in UE4.  Provides a standard way to predict autonomous proxy clients and interpolate simulated clients while maintaining full server authority.  

The system automatically handles gathering inputs/state data, packing then replicating to the authority, running inputs, then replicating back result data to all clients.

FixedStep State/Input variables can exist on an actor or any components within the actor, and by implementing a single interface, the FixedStepComponent will be able to gather inputs and state data from any number of actor+component combos without needing to write any additional code to glue them together.

Autonomous clients will predict their own result and automatically handle error correct/rollback/reprediction when they receive state data.  

Simulated clients will automatically interpolate state data to ensure smooth looking movement while only receiving limited snapshots of data.


Basics:

FixedStepComponents - Added to any actor that has constant state data that is changing and/or inputs from clients that need to be predicted.

FixedStepManagerComponent - Added to the GameState.  Controls the flow of the system (fixed step ticking, interpolation amount, roll backs, lag compensation, etc)

FixedStepPlayerComponent - Added to each PlayerState

IFixedStepListenerInterface - Any actor/component wanting to use the FixedStep system will need to inherit from this interface.  

    GetFixedStepCommands() - Similar to GetLifetimeReplicatedProps(), all fixed step state or input variables will need to be initialized here.
  
        FIXEDSTEPINPUT(Class,Var) - Initialize an input variable for authority/autonomous clients.
        FIXEDSTEPSTATE(Class,Var) - Initializes a state variable for authority/autonomouse & simulated clients
        FIXEDSTEPSTATE_ERROR(Class,Var,Err) - Initialized a state variable as well as an automatically calculated error value (smoothed out over time)
        FIXEDSTEPSTATE_ERRORVISUAL(Class,Var,Err,Vis) - Initializes a state variable as well as error and visual value (visual is base val + error)
  
    Simulate() - Any network relevant simulation should happen in this function. This will ensure that all relevant simulations are deterministic.  This will be called on authority and autonomous proxies who are predicting, as well as rolling back and resimulating.
  
    PostSimulate() - Any clean up can be done here (adjusting visual locations of meshes to account for error).  This will only run once per Unreal frame, while Simulate() may run many times per Unreal frame.
  
    GatherInputs() - Any input variables should be updated here to ensure they have the correct value when Simulate() is run.
  
    RollbackStart()/RollbackComplete() 
  
    Interpolate() - This will only occur on simulated clients.  The state variables will already have their interpolated values set here allowing visual or cosmetic things to occur in response here.
  
  



