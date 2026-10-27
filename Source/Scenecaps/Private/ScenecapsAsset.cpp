// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsAsset.cpp

#include "ScenecapsAsset.h"

#include "ScenecapsEntryRegistry.h"
#include "ScenecapsModule.h"

#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

UScenecapsAsset::UScenecapsAsset()
{
    RootId = FGuid::NewGuid();
}

void UScenecapsAsset::PostLoad()
{
    Super::PostLoad();
    if (!RootId.IsValid())
    {
        RootId = FGuid::NewGuid();
    }
}

// ---------- Variant helpers ----------

TArray<FName> UScenecapsAsset::GetVariantNames() const
{
    TArray<FName> Out;
    Out.Reserve(Variants.Num());
    for (const FScenecapsVariant& V : Variants)
    {
        if (!V.Name.IsNone())
        {
            Out.Add(V.Name);
        }
    }
    return Out;
}

const FScenecapsVariant* UScenecapsAsset::FindVariant(FName VariantName) const
{
    if (VariantName.IsNone()) return nullptr;
    return Variants.FindByPredicate(
        [&](const FScenecapsVariant& V) { return V.Name == VariantName; });
}

const FScenecapsEntryOverride* UScenecapsAsset::FindOverride(
    const FScenecapsVariant& Variant, FGuid EntryId) const
{
    return Variant.Overrides.FindByPredicate(
        [&](const FScenecapsEntryOverride& O) { return O.EntryId == EntryId; });
}

FScenecapsEntry UScenecapsAsset::ApplyVariantToEntry(
    const FScenecapsEntry& BaseEntry, FName VariantName) const
{
    const FScenecapsVariant* Variant = FindVariant(VariantName);
    if (!Variant) return BaseEntry;

    const FScenecapsEntryOverride* Override = FindOverride(*Variant, BaseEntry.EntryId);
    if (!Override) return BaseEntry;

    FScenecapsEntry Result = BaseEntry;

    if (Override->bOverrideVisible)
    {
        Result.bVisible = Override->bVisible;
    }
    if (Override->bOverrideTransform)
    {
        Result.RelativeTransform = Override->RelativeTransform;
    }
    if (Override->bOverrideMaterial)
    {
        Result.MaterialOverride = Override->MaterialOverride;
        Result.MaterialSlotIndex = Override->MaterialSlotIndex;
    }

    return Result;
}

// ---------- Spawn ----------

TArray<AActor*> UScenecapsAsset::SpawnCollection(
    UObject* WorldContextObject,
    const FTransform& WorldTransform,
    bool bIncludeHidden,
    FName VariantName)
{
    TMap<FGuid, AActor*> Ignored;
    return SpawnCollectionEx(WorldContextObject, WorldTransform, bIncludeHidden, VariantName, Ignored);
}

TArray<AActor*> UScenecapsAsset::SpawnCollectionEx(
    UObject* WorldContextObject,
    const FTransform& WorldTransform,
    bool bIncludeHidden,
    FName VariantName,
    TMap<FGuid, AActor*>& OutEntryToActor)
{
    TArray<AActor*> SpawnedActors;

    UWorld* World = GEngine
        ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)
        : nullptr;
    if (!World) return SpawnedActors;

    static thread_local TArray<const UScenecapsAsset*> GRecursionStack;
    constexpr int32 GMaxNestingDepth = 16;

    if (GRecursionStack.Contains(this))
    {
        UE_LOG(LogScenecaps, Error,
            TEXT("Circular reference detected: %s, skipping spawn"), *GetName());
        return SpawnedActors;
    }
    if (GRecursionStack.Num() >= GMaxNestingDepth)
    {
        UE_LOG(LogScenecaps, Error,
            TEXT("Nesting depth exceeds %d, skipping: %s"), GMaxNestingDepth, *GetName());
        return SpawnedActors;
    }

    GRecursionStack.Push(this);
    struct FRecursionGuard
    {
        ~FRecursionGuard() { GRecursionStack.Pop(); }
    } RecursionGuard;

    AActor* WorldContextActor = Cast<AActor>(WorldContextObject);

    TMap<FGuid, AActor*>    IdToActor;
    TMap<FGuid, FTransform> IdToWorld;
    TSet<FGuid>             Processed;

    TFunction<void(const FScenecapsEntry&, const FTransform&, bool)> SpawnEntry =
        [&](const FScenecapsEntry& BaseEntry, const FTransform& ParentWorld, bool bParentSkipped)
        {
            if (Processed.Contains(BaseEntry.EntryId))
            {
                return;
            }
            Processed.Add(BaseEntry.EntryId);

            // ★ 变体应用：所有下游都基于实际使用的 Entry
            const FScenecapsEntry Entry = ApplyVariantToEntry(BaseEntry, VariantName);

            const FTransform FinalWorld = Entry.RelativeTransform * ParentWorld;
            const bool bShouldSpawn = (Entry.bVisible || bIncludeHidden) && !bParentSkipped;

            if (bShouldSpawn && !Entry.Asset.IsNull())
            {
                if (UObject* Loaded = Entry.Asset.LoadSynchronous())
                {
                    FScenecapsSpawnContext Context;
                    Context.World = World;
                    Context.Transform = FinalWorld;
                    Context.bIsPreview = false;
                    Context.bIncludeHidden = bIncludeHidden;

                    Context.DecalSize = Entry.DecalSize;
                    Context.bOverrideMobility = Entry.bOverrideMobility;
                    Context.Mobility = Entry.Mobility;
                    Context.bOverrideCollision = Entry.bOverrideCollision;
                    Context.CollisionEnabled = Entry.CollisionEnabled;
                    Context.bOverrideCastShadow = Entry.bOverrideCastShadow;
                    Context.bCastShadow = Entry.bCastShadow;

                    Context.MaterialOverride = Entry.MaterialOverride.LoadSynchronous();
                    Context.MaterialSlotIndex = Entry.MaterialSlotIndex;

                    IScenecapsEntryHandler* Handler =
                        FScenecapsEntryRegistry::Get().FindHandlerForAsset(Loaded);

                    if (AActor* Spawned = Handler ? Handler->SpawnActor(Context, Loaded) : nullptr)
                    {
                        if (WorldContextActor && BaseEntry.ParentId == RootId)
                        {
                            Spawned->AttachToActor(
                                WorldContextActor,
                                FAttachmentTransformRules::KeepWorldTransform);
                        }

                        SpawnedActors.Add(Spawned);
                        IdToActor.Add(BaseEntry.EntryId, Spawned);
                        IdToWorld.Add(BaseEntry.EntryId, FinalWorld);
                        OutEntryToActor.Add(BaseEntry.EntryId, Spawned);
                    }
                }
            }

            for (const FScenecapsEntry& Child : Entries)
            {
                if (Child.ParentId == BaseEntry.EntryId)
                {
                    SpawnEntry(Child, FinalWorld, bParentSkipped || !bShouldSpawn);
                }
            }
        };

    for (const FScenecapsEntry& Entry : Entries)
    {
        if (Entry.ParentId == RootId)
        {
            SpawnEntry(Entry, WorldTransform, false);
        }
    }

    for (const FScenecapsEntry& Entry : Entries)
    {
        if (!Processed.Contains(Entry.EntryId))
        {
            SpawnEntry(Entry, WorldTransform, false);
        }
    }

    for (const FScenecapsEntry& Entry : Entries)
    {
        AActor** ChildPtr = IdToActor.Find(Entry.EntryId);
        AActor** ParentPtr = IdToActor.Find(Entry.ParentId);
        if (ChildPtr && ParentPtr && *ChildPtr && *ParentPtr && *ChildPtr != *ParentPtr)
        {
            (*ChildPtr)->AttachToActor(*ParentPtr, FAttachmentTransformRules::KeepWorldTransform);
        }
    }

    return SpawnedActors;
}