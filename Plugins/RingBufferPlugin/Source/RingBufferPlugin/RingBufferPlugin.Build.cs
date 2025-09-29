// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RingBufferPlugin : ModuleRules
{
	public RingBufferPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"SlateCore",
			}
		);

		// No third-party deps; we use <atomic> and <array> from STL.
		bUseUnity = true;
	}
}
