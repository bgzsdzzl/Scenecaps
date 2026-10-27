#include "ScenecapsExtensionManager.h"
#include "ScenecapsModule.h"

FScenecapsExtensionManager& FScenecapsExtensionManager::Get()
{
    static FScenecapsExtensionManager Instance;
    return Instance;
}

void FScenecapsExtensionManager::Initialize()
{
    // 暂时留空，或者在这里写扫描外部 DLL 的逻辑
    UE_LOG(LogScenecaps, Log, TEXT("ScenecapsExtensionManager Initialized."));
}

void FScenecapsExtensionManager::Shutdown()
{
    LoadedModuleHandles.Empty();
}