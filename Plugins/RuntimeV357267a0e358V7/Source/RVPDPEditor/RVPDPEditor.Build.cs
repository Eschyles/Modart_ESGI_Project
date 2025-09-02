// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

using System.IO;
using UnrealBuildTool;

public class RVPDPEditor : ModuleRules
{
	public RVPDPEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		if (Target.bBuildEditor)
		{
			bUseUnity = false;
		}

		PublicIncludePaths.AddRange(
			new string[] {
                Path.Combine(ModuleDirectory, "") // Necessary other we got Package issue if including stuff in the root
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
				"Slate",
				"SlateCore",
				"EditorStyle",
				"InputCore",
				"RenderCore",
				"UnrealEd",
				"EditorSubsystem",
				"AssetRegistry",
				"DeveloperSettings",
				"Blutility",
				"UMG",
				"EngineSettings",
				"WebBrowser",
				"Projects",
                "VertexPaintDetectionPlugin",
                "GameplayTags",
                "GameplayTagsEditor"
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
