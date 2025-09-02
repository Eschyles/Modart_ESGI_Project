// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

using UnrealBuildTool;
using System.IO;

public class RVPDPRapidJson : ModuleRules
{
    public RVPDPRapidJson(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Json"});

        PrivateDependencyModuleNames.AddRange(new string[] { });

        // Add the public include paths
        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "ThirdParty/rapidjson"),
                Path.Combine(ModuleDirectory, "") // Necessary other we got Package issue if including stuff in the root
            }
        );
    }
}
