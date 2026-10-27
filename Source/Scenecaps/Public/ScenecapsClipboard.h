#pragma once

#include "CoreMinimal.h"
#include "ScenecapsTypes.h"
#include "ScenecapsStaticTypes.h"

class UScenecapsAsset;

namespace ScenecapsClipboard
{
    SCENECAPS_API bool  HasData();
    SCENECAPS_API int32 Num();
    SCENECAPS_API void  Clear();
    SCENECAPS_API void  Set(TArray<FScenecapsEntry>&& InEntries);
    SCENECAPS_API const TArray<FScenecapsEntry>& Get();

    SCENECAPS_API bool  HasBakedData();
    SCENECAPS_API int32 NumBaked();
    SCENECAPS_API void  ClearBaked();
    SCENECAPS_API void  SetBaked(TArray<FScenecapsBakedEntry>&& InEntries);
    SCENECAPS_API const TArray<FScenecapsBakedEntry>& GetBaked();
}

namespace ScenecapsEntryUtils
{
    SCENECAPS_API FTransform AccumulateWorldTransform(
        const TArray<FScenecapsEntry>& Entries,
        const FGuid& TargetEntryId,
        const FGuid& RootId);

    SCENECAPS_API FTransform AccumulateBakedWorldTransform(
        const TArray<FScenecapsBakedEntry>& Entries,
        const FGuid& TargetEntryId,
        const FGuid& RootId);
}