// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FracturedPlaneClientCore : ModuleRules
{
	public FracturedPlaneClientCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", 
			"EnhancedInput", "Networking", "Sockets", "FPCoreLibrary", "DeveloperSettings" });

		PrivateDependencyModuleNames.AddRange(new string[] { "ProceduralMeshComponent" });

		PublicIncludePaths.Add("FracturedPlaneClientCore");
		
		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
