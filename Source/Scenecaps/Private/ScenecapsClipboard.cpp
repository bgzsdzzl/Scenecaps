#include "ScenecapsClipboard.h"

namespace
{
    TArray<FScenecapsEntry>      GClipboard;
    TArray<FScenecapsBakedEntry> GBakedClipboard;
}

namespace ScenecapsClipboard
{
    bool  HasData() { return GClipboard.Num() > 0; }
    int32 Num() { return GClipboard.Num(); }
    void  Clear() { GClipboard.Reset(); }
    void  Set(TArray<FScenecapsEntry>&& In) { GClipboard = MoveTemp(In); }
    const TArray<FScenecapsEntry>& Get() { return GClipboard; }

    bool  HasBakedData() { return GBakedClipboard.Num() > 0; }
    int32 NumBaked() { return GBakedClipboard.Num(); }
    void  ClearBaked() { GBakedClipboard.Reset(); }
    void  SetBaked(TArray<FScenecapsBakedEntry>&& In) { GBakedClipboard = MoveTemp(In); }
    const TArray<FScenecapsBakedEntry>& GetBaked() { return GBakedClipboard; }
}

namespace ScenecapsEntryUtils
{
    FTransform AccumulateWorldTransform(
        const TArray<FScenecapsEntry>& Entries,
        const FGuid& TargetEntryId, const FGuid& RootId)
    {
        if (!TargetEntryId.IsValid() || TargetEntryId == RootId) return FTransform::Identity;
        TArray<const FScenecapsEntry*> Chain;
        FGuid Cur = TargetEntryId;
        int32 Guard = 0;
        while (Cur.IsValid() && Cur != RootId && Guard++ < 256)
        {
            const FScenecapsEntry* E = Entries.FindByPredicate(
                [&](const FScenecapsEntry& X) { return X.EntryId == Cur; });
            if (!E) break;
            Chain.Insert(E, 0);
            Cur = E->ParentId;
        }
        FTransform Result = FTransform::Identity;
        for (const FScenecapsEntry* E : Chain) Result = E->RelativeTransform * Result;
        return Result;
    }

    FTransform AccumulateBakedWorldTransform(
        const TArray<FScenecapsBakedEntry>& Entries,
        const FGuid& TargetEntryId, const FGuid& RootId)
    {
        if (!TargetEntryId.IsValid() || TargetEntryId == RootId) return FTransform::Identity;
        TArray<const FScenecapsBakedEntry*> Chain;
        FGuid Cur = TargetEntryId;
        int32 Guard = 0;
        while (Cur.IsValid() && Cur != RootId && Guard++ < 256)
        {
            const FScenecapsBakedEntry* E = Entries.FindByPredicate(
                [&](const FScenecapsBakedEntry& X) { return X.EntryId == Cur; });
            if (!E) break;
            Chain.Insert(E, 0);
            Cur = E->ParentId;
        }
        FTransform Result = FTransform::Identity;
        for (const FScenecapsBakedEntry* E : Chain) Result = E->RelativeTransform * Result;
        return Result;
    }
}