// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PicoGS : ModuleRules
{
	public PicoGS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        //PublicDependencyModuleNames.AddRange(new string[] { "D:/Unreal Engine/UE5.3.2/Project/PICO_VR_Project/Blank_Log_Test/Player" });

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "HTTP", "HeadMountedDisplay", "VRExpansionPlugin" });

        if (Target.Type == TargetType.Editor)
        		{
           			PublicDependencyModuleNames.Add("PropertyEditor");
      		}

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
