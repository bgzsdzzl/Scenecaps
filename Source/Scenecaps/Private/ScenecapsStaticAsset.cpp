
#include "ScenecapsStaticAsset.h"

#include "ScenecapsAsset.h"
#include "ScenecapsEntryHandler.h"    // ApplyEntryOverrides
#include "ScenecapsModule.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/ObjectSaveContext.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

UScenecapsStaticAsset::UScenecapsStaticAsset()
{
    RootId = FGuid::NewGuid();
}

void UScenecapsStaticAsset::PostLoad()
{
    Super::PostLoad();
    if (!RootId.IsValid()) RootId = FGuid::NewGuid();
}

// ================================================================
// Variant helpers
// ================================================================

TArray<FName> UScenecapsStaticAsset::GetVariantNames() const
{
    TArray<FName> Out;
    Out.Reserve(Variants.Num());
    for (const FScenecapsBakedVariant& V : Variants)
        if (!V.Name.IsNone()) Out.Add(V.Name);
    return Out;
}

const FScenecapsBakedVariant* UScenecapsStaticAsset::FindVariant(FName VariantName) const
{
    if (VariantName.IsNone()) return nullptr;
    return Variants.FindByPredicate(
        [&](const FScenecapsBakedVariant& V) { return V.Name == VariantName; });
}

const FScenecapsBakedEntryOverride* UScenecapsStaticAsset::FindOverride(
    const FScenecapsBakedVariant& Variant, FGuid EntryId) const
{
    return Variant.Overrides.FindByPredicate(
        [&](const FScenecapsBakedEntryOverride& O) { return O.EntryId == EntryId; });
}

FScenecapsBakedEntry UScenecapsStaticAsset::ApplyVariantToEntry(
    const FScenecapsBakedEntry& BaseEntry, FName VariantName) const
{
    const FScenecapsBakedVariant* Variant = FindVariant(VariantName);
    if (!Variant) return BaseEntry;
    const FScenecapsBakedEntryOverride* Override = FindOverride(*Variant, BaseEntry.EntryId);
    if (!Override) return BaseEntry;

    FScenecapsBakedEntry Result = BaseEntry;
    if (Override->bOverrideVisible)   Result.bVisible = Override->bVisible;
    if (Override->bOverrideTransform) Result.RelativeTransform = Override->RelativeTransform;
    if (Override->bOverrideMaterial)
    {
        Result.MaterialOverride = Override->MaterialOverride;
        Result.MaterialSlotIndex = Override->MaterialSlotIndex;
    }
    return Result;
}

// ================================================================
// Spawn
// ================================================================

TArray<AActor*> UScenecapsStaticAsset::SpawnCollection(
    UObject* WorldContextObject, const FTransform& WorldTransform,
    bool bIncludeHidden, FName VariantName)
{
    TMap<FGuid, AActor*> Ignored;
    return SpawnCollectionEx(WorldContextObject, WorldTransform, bIncludeHidden, VariantName, Ignored);
}

TArray<AActor*> UScenecapsStaticAsset::SpawnCollectionEx(
    UObject* WorldContextObject, const FTransform& WorldTransform,
    bool bIncludeHidden, FName VariantName,
    TMap<FGuid, AActor*>& OutEntryToActor)
{
    TArray<AActor*> SpawnedActors;
    UWorld* World = GEngine
        ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull)
        : nullptr;
    if (!World) return SpawnedActors;

    AActor* WorldContextActor = Cast<AActor>(WorldContextObject);

    TMap<FGuid, AActor*>    IdToActor;
    TSet<FGuid>             Processed;

    TFunction<void(const FScenecapsBakedEntry&, const FTransform&, bool)> SpawnEntry =
        [&](const FScenecapsBakedEntry& BaseEntry, const FTransform& ParentWorld, bool bParentSkipped)
        {
            if (Processed.Contains(BaseEntry.EntryId)) return;
            Processed.Add(BaseEntry.EntryId);

            const FScenecapsBakedEntry Entry = ApplyVariantToEntry(BaseEntry, VariantName);
            const FTransform FinalWorld = Entry.RelativeTransform * ParentWorld;
            const bool bShouldSpawn = (Entry.bVisible || bIncludeHidden) && !bParentSkipped;

            if (bShouldSpawn && Entry.ActorClass)
            {
                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                Params.ObjectFlags |= RF_Transactional;

                AActor* Spawned = World->SpawnActor<AActor>(Entry.ActorClass, FinalWorld, Params);
                if (Spawned)
                {
                    // ★ 反序列化烘焙的脚本属性
                    if (Entry.SerializedActorProperties.Num() > 0)
                    {
                        FMemoryReader Reader(Entry.SerializedActorProperties, /*bIsPersistent=*/true);
                        FObjectAndNameAsStringProxyArchive Ar(Reader, /*bLoadIfFindFails=*/true);
                        Ar.ArIsSaveGame = false;
                        Ar.ArNoDelta = true;
                        Spawned->SerializeScriptProperties(Ar);
                    }

                    if (WorldContextActor && BaseEntry.ParentId == RootId)
                        Spawned->AttachToActor(WorldContextActor, FAttachmentTransformRules::KeepWorldTransform);

                    // 复用共享覆盖逻辑（Mobility/Collision/Shadow/Material）
                    FScenecapsSpawnContext Context;
                    Context.World = World;
                    Context.Transform = FinalWorld;
                    Context.DecalSize = Entry.DecalSize;
                    Context.bOverrideMobility = Entry.bOverrideMobility;
                    Context.Mobility = Entry.Mobility;
                    Context.bOverrideCollision = Entry.bOverrideCollision;
                    Context.CollisionEnabled = Entry.CollisionEnabled;
                    Context.bOverrideCastShadow = Entry.bOverrideCastShadow;
                    Context.bCastShadow = Entry.bCastShadow;
                    Context.MaterialOverride = Entry.MaterialOverride.LoadSynchronous();
                    Context.MaterialSlotIndex = Entry.MaterialSlotIndex;
                    ApplyEntryOverrides(Spawned, Context);

                    SpawnedActors.Add(Spawned);
                    IdToActor.Add(BaseEntry.EntryId, Spawned);
                    OutEntryToActor.Add(BaseEntry.EntryId, Spawned);
                }
            }

            for (const FScenecapsBakedEntry& Child : Entries)
                if (Child.ParentId == BaseEntry.EntryId)
                    SpawnEntry(Child, FinalWorld, bParentSkipped || !bShouldSpawn);
        };

    for (const FScenecapsBakedEntry& Entry : Entries)
        if (Entry.ParentId == RootId)
            SpawnEntry(Entry, WorldTransform, false);

    for (const FScenecapsBakedEntry& Entry : Entries)
        if (!Processed.Contains(Entry.EntryId))
            SpawnEntry(Entry, WorldTransform, false);

    for (const FScenecapsBakedEntry& Entry : Entries)
    {
        AActor** ChildPtr = IdToActor.Find(Entry.EntryId);
        AActor** ParentPtr = IdToActor.Find(Entry.ParentId);
        if (ChildPtr && ParentPtr && *ChildPtr && *ParentPtr && *ChildPtr != *ParentPtr)
            (*ChildPtr)->AttachToActor(*ParentPtr, FAttachmentTransformRules::KeepWorldTransform);
    }

    return SpawnedActors;
}

// ================================================================
// Bake
// ================================================================

#if WITH_EDITOR
void UScenecapsStaticAsset::BakeActor(AActor* Actor, FScenecapsBakedEntry& OutEntry)
{
    if (!Actor) return;

    OutEntry.ActorClass = Actor->GetClass();
    OutEntry.BakedClassName = Actor->GetClass()->GetName();
    OutEntry.DisplayName = Actor->GetActorLabel();
    OutEntry.RelativeTransform = Actor->GetActorTransform();
    OutEntry.bVisible = !Actor->IsHidden();

    // 序列化脚本属性（含所有 UPROPERTY、组件、软引用都通过 path 字符串保存）
    OutEntry.SerializedActorProperties.Reset();
    FMemoryWriter Writer(OutEntry.SerializedActorProperties, /*bIsPersistent=*/true);
    FObjectAndNameAsStringProxyArchive Ar(Writer, /*bInLoadIfFindFails=*/false);
    Ar.ArIsSaveGame = false;
    Ar.ArNoDelta = true;
    Actor->SerializeScriptProperties(Ar);
}

bool UScenecapsStaticAsset::BakeFromSceneAsset(
    UScenecapsAsset* SourceAsset, UScenecapsStaticAsset* TargetAsset, UWorld* World)
{
    if (!SourceAsset || !TargetAsset || !World) return false;

    TargetAsset->Modify();
    TargetAsset->RootId = FGuid::NewGuid();
    TargetAsset->Entries.Reset();
    TargetAsset->Variants.Reset();

    // 1. 在 editor world 里 spawn 一份（作为烘焙源）
    TMap<FGuid, AActor*> EntryToActor;
    TArray<AActor*> TempActors = SourceAsset->SpawnCollectionEx(
        World, FTransform::Identity, /*bIncludeHidden=*/true, NAME_None, EntryToActor);

    // 2. 逐条烘焙
    TMap<FGuid, FGuid> OldToNew;
    OldToNew.Reserve(SourceAsset->Entries.Num());

    for (const FScenecapsEntry& SrcEntry : SourceAsset->Entries)
    {
        FScenecapsBakedEntry NewEntry;

        if (AActor* const* FoundActor = EntryToActor.Find(SrcEntry.EntryId))
        {
            BakeActor(*FoundActor, NewEntry);
        }
        // 兜底：spawn 失败（不可见/加载失败），仍保留类信息
        if (!NewEntry.ActorClass)
        {
            if (UObject* Loaded = SrcEntry.Asset.LoadSynchronous())
            {
                if (UClass* Cls = Cast<UClass>(Loaded))
                    NewEntry.ActorClass = Cls;
                else if (UBlueprint* BP = Cast<UBlueprint>(Loaded))
                    NewEntry.ActorClass = BP->GeneratedClass;
            }
        }

        NewEntry.EntryId = FGuid::NewGuid();
        OldToNew.Add(SrcEntry.EntryId, NewEntry.EntryId);
        NewEntry.DisplayName = SrcEntry.DisplayName;
        NewEntry.RelativeTransform = SrcEntry.RelativeTransform;
        NewEntry.bVisible = SrcEntry.bVisible;
        NewEntry.bLocked = SrcEntry.bLocked;
        NewEntry.DecalSize = SrcEntry.DecalSize;
        NewEntry.bOverrideMobility = SrcEntry.bOverrideMobility;
        NewEntry.Mobility = SrcEntry.Mobility;
        NewEntry.bOverrideCollision = SrcEntry.bOverrideCollision;
        NewEntry.CollisionEnabled = SrcEntry.CollisionEnabled;
        NewEntry.bOverrideCastShadow = SrcEntry.bOverrideCastShadow;
        NewEntry.bCastShadow = SrcEntry.bCastShadow;
        NewEntry.MaterialOverride = SrcEntry.MaterialOverride;
        NewEntry.MaterialSlotIndex = SrcEntry.MaterialSlotIndex;
        NewEntry.DataLayer = SrcEntry.DataLayer;

        TargetAsset->Entries.Add(MoveTemp(NewEntry));
    }

    // 3. ParentId 重映射
    for (int32 i = 0; i < TargetAsset->Entries.Num(); ++i)
    {
        const FGuid SrcParent = SourceAsset->Entries[i].ParentId;
        if (SrcParent == SourceAsset->RootId)
            TargetAsset->Entries[i].ParentId = TargetAsset->RootId;
        else if (const FGuid* NewParent = OldToNew.Find(SrcParent))
            TargetAsset->Entries[i].ParentId = *NewParent;
        else
            TargetAsset->Entries[i].ParentId = TargetAsset->RootId;
    }

    // 4. 变体
    for (const FScenecapsVariant& SrcVar : SourceAsset->Variants)
    {
        FScenecapsBakedVariant NewVar;
        NewVar.Name = SrcVar.Name;
        NewVar.Description = SrcVar.Description;

        for (const FScenecapsEntryOverride& SrcOv : SrcVar.Overrides)
        {
            FScenecapsBakedEntryOverride NewOv;
            NewOv.EntryId = OldToNew.FindRef(SrcOv.EntryId);
            NewOv.DisplayName = SrcOv.DisplayName;
            NewOv.bOverrideVisible = SrcOv.bOverrideVisible;
            NewOv.bVisible = SrcOv.bVisible;
            NewOv.bOverrideTransform = SrcOv.bOverrideTransform;
            NewOv.RelativeTransform = SrcOv.RelativeTransform;
            NewOv.bOverrideMaterial = SrcOv.bOverrideMaterial;
            NewOv.MaterialOverride = SrcOv.MaterialOverride;
            NewOv.MaterialSlotIndex = SrcOv.MaterialSlotIndex;
            NewVar.Overrides.Add(MoveTemp(NewOv));
        }
        TargetAsset->Variants.Add(MoveTemp(NewVar));
    }

    // 5. 清理临时 Actor
    for (AActor* A : TempActors) if (IsValid(A)) A->Destroy();

    // 6. Metadata 复制
    TargetAsset->DisplayName = SourceAsset->DisplayName;
    TargetAsset->Description = SourceAsset->Description;
    TargetAsset->Tags = SourceAsset->Tags;

    TargetAsset->MarkPackageDirty();
    return true;
}
#endif