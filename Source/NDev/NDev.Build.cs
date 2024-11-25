// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NDev : ModuleRules
{
	public NDev(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        IWYUSupport = IWYUSupport.None;

        if (Target.Type == TargetType.Editor || Target.Type == TargetType.Game || Target.Type == TargetType.Client)
        {
            PrivateIncludePathModuleNames.AddRange(new string[]
            {
            "CoreUObject",
            "Engine",
            "TargetPlatform",
            "TextureCompressor",
            });

            PrivateDependencyModuleNames.AddRange(new string[]
            {
            "Core",
            "CoreUObject",
            "Engine",
            "ImageCore",
            "ImageWrapper",
            "TextureBuild",
            "TextureCompressor",
            "Slate",
            "SlateCore"
            });

            PublicIncludePathModuleNames.AddRange(new string[]
            {
            "CoreUObject",
            "Engine",
            "TargetPlatform",
            "TextureCompressor",
            });

            PublicDependencyModuleNames.AddRange(new string[]
            {
            "Core",
            "CoreUObject",
            "Engine",
            "ImageCore",
            "ImageWrapper",
            "TextureBuild",
            "TextureCompressor",
            });

            DynamicallyLoadedModuleNames.Add("TextureFormatDXT");

            //add this for DXT module
            AddEngineThirdPartyPrivateStaticDependencies(Target, "nvTextureTools");
        }
        else
        {
            PublicDependencyModuleNames.AddRange(new string[]
            {
            "Core",
             });

            PrivateDependencyModuleNames.AddRange(new string[]
            {
            "CoreUObject",
            "Engine",
            });
        }
    }
}
