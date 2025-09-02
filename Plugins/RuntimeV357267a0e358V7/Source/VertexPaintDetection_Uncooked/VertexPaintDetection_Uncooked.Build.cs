// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

using UnrealBuildTool;

public class VertexPaintDetection_Uncooked : ModuleRules
{
	public VertexPaintDetection_Uncooked(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		if (Target.bBuildEditor)
		{
			bUseUnity = false;
		}

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
            }
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
            }
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
				// ... add other public dependencies that you statically link with here ...
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"UnrealEd",
                "BlueprintGraph",
                "KismetCompiler",
				"Kismet",
                "VertexPaintDetectionPlugin"
				// ... add private dependencies that you statically link with here ...	
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
