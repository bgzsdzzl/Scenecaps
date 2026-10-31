// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsClipboard.h

#pragma once

#include "CoreMinimal.h"
#include "ScenecapsTypes.h"

class UScenecapsAsset;

namespace ScenecapsClipboard
{
    SCENECAPS_API bool  HasData();
    SCENECAPS_API int32 Num();
    SCENECAPS_API void  Clear();
    SCENECAPS_API void  Set(TArray<FScenecapsEntry>&& InEntries);
    SCENECAPS_API const TArray<FScenecapsEntry>& Get();
}

namespace ScenecapsEntryUtils
{
    SCENECAPS_API FTransform AccumulateWorldTransform(
        const TArray<FScenecapsEntry>& Entries,
        const FGuid& TargetEntryId,
        const FGuid& RootId);
}
