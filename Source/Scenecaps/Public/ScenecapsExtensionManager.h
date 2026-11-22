#pragma once

#include "CoreMinimal.h"

class SCENECAPS_API FScenecapsExtensionManager
{
public:
    static FScenecapsExtensionManager& Get();

    void Initialize();
    void Shutdown();

private:
    FScenecapsExtensionManager() = default;

    // 从 <ProjectDir>/Plugins/Scenecaps/Extensions/ 加载的 DLL 句柄，Shutdown 时按加载顺序释放
    TArray<void*> LoadedModuleHandles;
};
