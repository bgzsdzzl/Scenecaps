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
            "AssetDefinition"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "UnrealEd",
            "EditorFramework",
            "EditorWidgets",
            "EditorStyle",
            "AdvancedPreviewScene",
            "Slate",
            "SlateCore",
            "AssetTools",
            "AssetRegistry",
            "PropertyEditor",
            "ToolMenus",
            "LevelEditor",
            "AppFramework",
            "InputCore",
            "ContentBrowser"        // ← 新增：Content Browser 右键菜单
        });
    }
}