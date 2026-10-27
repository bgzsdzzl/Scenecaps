#pragma once

#include "CoreMinimal.h"

namespace ScenecapsAssetCategories
{
    inline FName CategoryKey() { return FName(TEXT("Scenecaps")); }
    extern SCENECAPSEDITOR_API uint32 GCategoryBit;
}