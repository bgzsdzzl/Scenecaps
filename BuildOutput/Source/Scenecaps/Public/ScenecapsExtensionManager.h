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

    // 可以在这里记录加载的 DLL 句柄等
    TArray<void*> LoadedModuleHandles;
};