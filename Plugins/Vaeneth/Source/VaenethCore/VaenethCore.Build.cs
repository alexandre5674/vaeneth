// Copyright 2026 Vaeneth. All Rights Reserved.
// Build revision: 2

using UnrealBuildTool;

public class VaenethCore : ModuleRules
{
	public VaenethCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"AssetTools",
				"UnrealEd"
			});
		}
	}
}
