using UnrealBuildTool;

public class TeleportLogistics : ModuleRules
{
    public TeleportLogistics(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "FactoryGame", "SML", "DummyHeaders",
            "InputCore", "Slate", "SlateCore", "UMG", "NetCore", "PhysicsCore",
            "DeveloperSettings", "GeometryCollectionEngine", "AnimGraphRuntime", "AssetRegistry",
            "NavigationSystem", "AIModule", "GameplayTasks", "RenderCore", "CinematicCamera",
            "Foliage", "GameplayTags"
        });
    }
}
