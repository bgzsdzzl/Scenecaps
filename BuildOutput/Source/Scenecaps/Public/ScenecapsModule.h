#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

SCENECAPS_API DECLARE_LOG_CATEGORY_EXTERN(LogScenecaps, Log, All);

class FScenecapsModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};