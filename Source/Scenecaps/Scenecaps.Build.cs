using UnrealBuildTool;

public class Scenecaps : ModuleRules
{
    public Scenecaps(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.NoPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core",
            "CoreUObject",
            "Engine",
            "SlateCore", "Slate",
            "Landscape" });

        PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities", "Projects" });

        // Niagara 集成默认启用，可通过环境变量 SCENECAPS_WITH_NIAGARA=0 关闭
        bool bEnableNiagaraHandler = true;
        string NiagaraSetting = System.Environment.GetEnvironmentVariable("SCENECAPS_WITH_NIAGARA");
        if (!string.IsNullOrEmpty(NiagaraSetting))
        {
            bool bDisabled = NiagaraSetting == "0" ||
                NiagaraSetting.Equals("false", System.StringComparison.OrdinalIgnoreCase);
            bEnableNiagaraHandler = !bDisabled;
        }

        if (bEnableNiagaraHandler)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "Niagara",
                "NiagaraCore"
            });
            PublicDefinitions.Add("SCENE_COLLECTION_WITH_NIAGARA=1");
        }
        else
        {
            PublicDefinitions.Add("SCENE_COLLECTION_WITH_NIAGARA=0");
        }
    }
}
