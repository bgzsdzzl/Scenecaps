using UnrealBuildTool;

public class ScenecapsEditor : ModuleRules
{
    public ScenecapsEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] 
        { 
            "Core",
            "CoreUObject",
            "Engine",
            "Scenecaps",
            "AssetDefinition",
            "Slate",
            "SlateCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[] 
        { 
            "UnrealEd", 
            "EditorFramework", 
            "EditorWidgets", 
            "EditorStyle", 
            "AdvancedPreviewScene", 
            "AssetTools", 
            "AssetRegistry", 
            "PropertyEditor", 
            "ToolMenus", 
            "LevelEditor", 
            "AppFramework", 
            "InputCore", 
            "ContentBrowser" 
        });

        // Ensure public headers are accessible
        PublicIncludePaths.AddRange(new string[] 
        {
            ModuleDirectory + "/Public"
        });
    }
}
