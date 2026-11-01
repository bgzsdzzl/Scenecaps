// Plugins/Scenecaps/Source/Scenecaps/Public/IScenecapsModifierModule.h

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "ScenecapsModifier.h"

class IScenecapsModifierModule : public IModuleInterface
{
public:
    virtual ~IScenecapsModifierModule() = default;

    virtual void RegisterModifiers(class UScenecapsModifierRegistry* Registry) = 0;
    virtual void UnregisterModifiers(class UScenecapsModifierRegistry* Registry) = 0;
};
