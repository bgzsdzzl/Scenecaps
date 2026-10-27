// Plugins/Scenecaps/Source/Scenecaps/Scenecaps.Build.cs

using UnrealBuildTool;

public class Scenecaps : ModuleRules
{
    public Scenecaps(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine"        // ← WorldPartition / DataLayer 的所有类型都在这里
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Json",
            "JsonUtilities",
            "Projects"
        });

        bool bEnableNiagaraHandler = true;

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