// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FPCoreLibrary : ModuleRules
{
	public FPCoreLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		Type = ModuleType.CPlusPlus;
		
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "Engine", "Projects"});
	}
}
