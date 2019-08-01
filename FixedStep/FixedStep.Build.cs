// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class FixedStep : ModuleRules
{
	public FixedStep(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "StateMachineX"
        });

		PrivateDependencyModuleNames.AddRange(
            new string[] {
        });
	}
}
