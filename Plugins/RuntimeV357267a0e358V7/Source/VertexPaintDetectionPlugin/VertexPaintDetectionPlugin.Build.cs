// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

using System.IO;
using UnrealBuildTool;

public class VertexPaintDetectionPlugin : ModuleRules
{
    public VertexPaintDetectionPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        if (Target.bBuildEditor)
        {
            bUseUnity = false;
        }
        
        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "Components"),
                Path.Combine(ModuleDirectory, "DataAssets"),
                Path.Combine(ModuleDirectory, "Async"),
                Path.Combine(ModuleDirectory, "Prerequisites"),
                Path.Combine(ModuleDirectory, "Utility"),
                Path.Combine(ModuleDirectory, "FunctionLibs"),
                Path.Combine(ModuleDirectory, "Subsystems"),
                Path.Combine(ModuleDirectory, "") // Necessary otherwise we got Package issue if including stuff in the root
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
                "RVPDPRapidJson"
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
                "RenderCore",
                "PhysicsCore",
                "AssetRegistry",
                "DeveloperSettings",
                "ChaosCloth",
                "ClothingSystemRuntimeInterface",
                "ClothingSystemRuntimeCommon",
                "Landscape",
                "Json",
                "GameplayTags",
                "AnimationCore",
                "Chaos"
                // ... add private dependencies that you statically link with here ...	
            }
            );


        // For UE5 Only so the Plugin can compile for UE4 that doesn't have these
        if (Target.Version.MajorVersion == 5)
        {
            // Had an issue with PrivateDependencyModuleNames where I couldn't include GeometryScriptTypes & MeshVertexColorFunctions in functionlibrary
            // but with Public it got resolved, eventhough it's the same module. Had to include them since forward decleration wasn't enough for
            // SetAndEnableDynamicMeshConstantVertexColor()
            PublicDependencyModuleNames.AddRange(new string[]
            {
                "GeometryFramework",
                "GeometryScriptingCore",
                "GeometryCore",
                "GeometryCollectionEngine"
            });
        }


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
