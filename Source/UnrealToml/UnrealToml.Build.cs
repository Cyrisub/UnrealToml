// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealToml : ModuleRules
{
	public UnrealToml(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		bEnableExceptions = false;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
