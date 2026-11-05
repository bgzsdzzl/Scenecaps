using UnrealBuildTool;

public class ScenecapsEditor : ModuleRules
{
    public ScenecapsEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.NoPCHs;

        PublicDependencyModuleNames.AddRange(new string[] 
        { 
            "Core",
            "CoreUObject",
            "Engine",
            "Scenecaps",
            "AssetDefinition",
            "Slate",
            "SlateCore",
            "Renderer",
            "AssetRegistry"
        });

        PrivateDependencyModuleNames.AddRange(new string[] 
        { 
            "UnrealEd", 
            "EditorFramework", 
            "EditorWidgets", 
            "EditorStyle", 
            "AdvancedPreviewScene",
            "AssetTools",
            "PropertyEditor",
            "ToolMenus", 
            "LevelEditor", 
            "AppFramework", 
            "InputCore", 
            "ContentBrowser"
        });
    }
}
