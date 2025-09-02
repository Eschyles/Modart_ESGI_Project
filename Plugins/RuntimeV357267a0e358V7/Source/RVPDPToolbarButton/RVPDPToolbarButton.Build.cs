// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

using UnrealBuildTool;

public class RVPDPToolbarButton : ModuleRules
{
	public RVPDPToolbarButton(ReadOnlyTargetRules Target) : base(Target)
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
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"LevelEditor",
				"Blutility",
				"UMG",
				"UMGEditor"
				// ... add private dependencies that you statically link with here ...	
			}
			);


#if UE_5_0_OR_LATER

PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"EditorFramework"
				// ... add private dependencies that you statically link with here ...	
			}
			);

#else


#endif


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
