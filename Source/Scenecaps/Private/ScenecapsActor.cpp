#include "ScenecapsActor.h"

#include "ScenecapsAsset.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "ScenecapsModifierManager.h"

#if WITH_EDITOR
#include "WorldPartition/DataLayer/DataLayerAsset.h"
#include "WorldPartition/DataLayer/DataLayerInstance.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"
#endif

AScenecapsActor::AScenecapsActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    PrimaryActorTick.TickInterval = 0.25f;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
}

// ---------- Data Layer helpers ----------
#if WITH_EDITOR

void AScenecapsActor::EnsureDataLayerCached()
{
    if (!CachedDataLayerAsset && !TargetDataLayer.IsNull())
    {
        CachedDataLayerAsset = TargetDataLayer.LoadSynchronous();
    }
}

#endif

// ---------- Rebuild / Clear ----------

void AScenecapsActor::Rebuild()
{
#if WITH_EDITOR
    Modify();
#endif

    Clear();
#if WITH_EDITOR
    EnsureDataLayerCached();
    if (ShouldDeferRebuild()) return;
#endif

    UScenecapsAsset* Asset = CollectionAsset.LoadSynchronous();
    if (!Asset) return;

    UWorld* World = GetWorld();
    if (!World) return;

    const FTransform BaseTransform = GetActorTransform();
    TMap<FGuid, AActor*> EntryToActor;
    TArray<AActor*> NewActors =
        Asset->SpawnCollectionEx(this, BaseTransform, bIncludeHidden, ActiveVariant, EntryToActor);

    SpawnedActors.Reserve(NewActors.Num());

    for (AActor* Actor : NewActors)
    {
        if (!Actor) continue;
#if WITH_EDITOR
        Actor->SetIsSpatiallyLoaded(false);
#endif
        if (Actor->GetAttachParentActor() == nullptr)
        {
            Actor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        }
        SpawnedActors.Add(Actor);
    }

#if WITH_EDITOR
    for (const TPair<FGuid, AActor*>& Pair : EntryToActor)
    {
        AActor* Actor = Pair.Value;
        if (!Actor) continue;

        UDataLayerAsset* LayerToUse = nullptr;
        if (const FScenecapsEntry* Entry = Asset->Entries.FindByPredicate(
            [&](const FScenecapsEntry& X) { return X.EntryId == Pair.Key; }))
        {
            if (!Entry->DataLayer.IsNull())
            {
                LayerToUse = Entry->DataLayer.LoadSynchronous();
            }
        }
        if (!LayerToUse) LayerToUse = CachedDataLayerAsset;
        if (LayerToUse) ApplyDataLayerToActor(Actor, LayerToUse);
    }
#endif
}

void AScenecapsActor::Clear()
{
    for (TObjectPtr<AActor>& Ptr : SpawnedActors)
    {
        if (AActor* Actor = Ptr.Get())
        {
            if (IsValid(Actor))
            {
#if WITH_EDITOR
                UDataLayerAsset* LayerToRemove = CachedDataLayerAsset;
                if (!LayerToRemove && !TargetDataLayer.IsNull())
                {
                    LayerToRemove = TargetDataLayer.LoadSynchronous();
                }
                if (LayerToRemove)
                {
                    RemoveDataLayerFromActor(Actor, LayerToRemove);
                }
#endif
                Actor->Destroy();
            }
        }
    }
    SpawnedActors.Empty();
}

// ---------- AActor lifecycle ----------

void AScenecapsActor::Destroyed()
{
    Clear();
    Super::Destroyed();
}

void AScenecapsActor::PostDuplicate(bool bDuplicateForPIE)
{
    Super::PostDuplicate(bDuplicateForPIE);
    SpawnedActors.Empty();
    Rebuild();
}

void AScenecapsActor::BeginPlay()
{
    Super::BeginPlay();
#if WITH_EDITOR
    EnsureDataLayerCached();

    if (bSpawnOnDataLayerLoaded && !TargetDataLayer.IsNull())
    {
        SetActorTickEnabled(true);
    }

    if (!ShouldDeferRebuild())
    {
        Rebuild();
    }
#endif
}

void AScenecapsActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SetActorTickEnabled(false);
    Super::EndPlay(EndPlayReason);
}

void AScenecapsActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if WITH_EDITOR
    if (!bSpawnOnDataLayerLoaded || TargetDataLayer.IsNull()) return;

    const bool bActive = IsTargetDataLayerActive();
    const bool bExists = SpawnedActors.Num() > 0;

    if (bActive && !bExists) Rebuild();
    else if (!bActive && bExists) Clear();
#endif
}
#if WITH_EDITOR

void AScenecapsActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropName = PropertyChangedEvent.GetPropertyName();
    if (PropName == GET_MEMBER_NAME_CHECKED(AScenecapsActor, CollectionAsset) ||
        PropName == GET_MEMBER_NAME_CHECKED(AScenecapsActor, bIncludeHidden) ||
        PropName == GET_MEMBER_NAME_CHECKED(AScenecapsActor, ActiveVariant))
    {
        Rebuild();
    }
    else if (PropName == GET_MEMBER_NAME_CHECKED(AScenecapsActor, TargetDataLayer) ||
        PropName == GET_MEMBER_NAME_CHECKED(AScenecapsActor, bSpawnOnDataLayerLoaded))
    {
        CachedDataLayerAsset = TargetDataLayer.IsNull()
            ? nullptr : TargetDataLayer.LoadSynchronous();

        SetActorTickEnabled(bSpawnOnDataLayerLoaded && !TargetDataLayer.IsNull());
        Rebuild();
    }
}
#endif

#if WITH_EDITOR

// ---------- WP state query ----------

bool AScenecapsActor::IsTargetDataLayerActive() const
{
    if (TargetDataLayer.IsNull()) return true;
    UWorld* World = GetWorld();
    if (!World) return true;
    UDataLayerManager* Manager = UDataLayerManager::GetDataLayerManager(World);
    if (!Manager) return true;

    UDataLayerAsset* Asset = CachedDataLayerAsset
        ? CachedDataLayerAsset.Get()
        : TargetDataLayer.LoadSynchronous();
    if (!Asset) return true;

    const UDataLayerInstance* Instance = Manager->GetDataLayerInstanceFromAsset(Asset);
    if (!Instance) return true;

    return Manager->GetDataLayerInstanceRuntimeState(Instance) == EDataLayerRuntimeState::Activated;
}

bool AScenecapsActor::ShouldDeferRebuild() const
{
    if (!bSpawnOnDataLayerLoaded) return false;
    if (TargetDataLayer.IsNull()) return false;
#if WITH_EDITOR
    if (GIsEditor && GetWorld() && GetWorld()->WorldType == EWorldType::Editor) return false;
#endif
    return !IsTargetDataLayerActive();
}

FText AScenecapsActor::GetDataLayerStatusText() const
{
    if (TargetDataLayer.IsNull()) return FText::FromString(TEXT("N/A (no Data Layer)"));
    UWorld* World = GetWorld();
    if (!World) return FText::FromString(TEXT("No World"));
    UDataLayerManager* M = UDataLayerManager::GetDataLayerManager(World);
    if (!M) return FText::FromString(TEXT("No DataLayerManager"));

    UDataLayerAsset* Asset = TargetDataLayer.LoadSynchronous();
    if (!Asset) return FText::FromString(TEXT("Asset missing"));
    const UDataLayerInstance* Instance = M->GetDataLayerInstanceFromAsset(Asset);
    if (!Instance) return FText::FromString(TEXT("Instance not registered"));

    switch (M->GetDataLayerInstanceRuntimeState(Instance))
    {
    case EDataLayerRuntimeState::Activated: return FText::FromString(TEXT("Activated"));
    case EDataLayerRuntimeState::Loaded:    return FText::FromString(TEXT("Loaded"));
    case EDataLayerRuntimeState::Unloaded:  return FText::FromString(TEXT("Unloaded"));
    default:                                return FText::FromString(TEXT("Unknown"));
    }
}

UDataLayerAsset* AScenecapsActor::GetTargetDataLayer() const
{
    return TargetDataLayer.LoadSynchronous();
}

void AScenecapsActor::SetTargetDataLayer(UDataLayerAsset* InDataLayer, bool bApplyImmediately)
{
    if (TargetDataLayer.Get() == InDataLayer) return;
    TargetDataLayer = InDataLayer;
    CachedDataLayerAsset = InDataLayer;
    if (bApplyImmediately) Rebuild();
}

void AScenecapsActor::SetDataLayerActive(bool bActive)
{
    if (!TargetDataLayer.IsNull())
    {
        if (UDataLayerManager* Manager = UDataLayerManager::GetDataLayerManager(GetWorld()))
        {
            if (UDataLayerAsset* Asset = TargetDataLayer.LoadSynchronous())
            {
                if (const UDataLayerInstance* Instance = Manager->GetDataLayerInstanceFromAsset(Asset))
                {
                    Manager->SetDataLayerInstanceRuntimeState(const_cast<UDataLayerInstance*>(Instance), bActive ? EDataLayerRuntimeState::Activated : EDataLayerRuntimeState::Unloaded);
                }
            }
        }
    }
}

void AScenecapsActor::ApplyDataLayerToActor(AActor* Actor, UDataLayerAsset* LayerAsset) const
{
    if (!Actor || !LayerAsset) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UDataLayerManager* Manager = UDataLayerManager::GetDataLayerManager(World);
    if (!Manager) return;
    if (const UDataLayerInstance* Instance = Manager->GetDataLayerInstanceFromAsset(LayerAsset))
    {
        Actor->AddDataLayer(Instance);
    }
}

void AScenecapsActor::RemoveDataLayerFromActor(AActor* Actor, UDataLayerAsset* LayerAsset) const
{
    if (!Actor || !LayerAsset) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UDataLayerManager* Manager = UDataLayerManager::GetDataLayerManager(World);
    if (!Manager) return;
    if (const UDataLayerInstance* Instance = Manager->GetDataLayerInstanceFromAsset(LayerAsset))
    {
        Actor->RemoveDataLayer(Instance);
    }
}

#endif

// ---------- Runtime Blueprint API ----------

UScenecapsAsset* AScenecapsActor::GetCollectionAsset() const
{
    return CollectionAsset.LoadSynchronous();
}

FName AScenecapsActor::GetActiveVariant() const
{
    return ActiveVariant;
}

TArray<FName> AScenecapsActor::GetVariantNames() const
{
    TArray<FName> Names;
    if (!CollectionAsset.IsNull())
    {
        if (UScenecapsAsset* Asset = CollectionAsset.LoadSynchronous())
            Names = Asset->GetVariantNames();
    }
    return Names;
}

void AScenecapsActor::SetCollectionAsset(UScenecapsAsset* InAsset, bool bRebuildImmediately)
{
    if (CollectionAsset.Get() == InAsset) return;
    CollectionAsset = InAsset;
    if (bRebuildImmediately) Rebuild();
}

void AScenecapsActor::SetActiveVariant(FName InVariant, bool bRebuildImmediately)
{
    if (ActiveVariant == InVariant) return;
    ActiveVariant = InVariant;
    if (bRebuildImmediately) Rebuild();
}

TArray<AActor*> AScenecapsActor::GetSpawnedActorArray() const
{
    TArray<AActor*> Result;
    Result.Reserve(SpawnedActors.Num());
    for (const TObjectPtr<AActor>& Ptr : SpawnedActors)
    {
        if (AActor* Actor = Ptr.Get())
            Result.Add(Actor);
    }
    return Result;
}

TArray<AActor*> AScenecapsActor::FindSpawnedActorsByClass(TSubclassOf<AActor> ActorClass) const
{
    TArray<AActor*> Result;
    for (const TObjectPtr<AActor>& Ptr : SpawnedActors)
    {
        if (AActor* Actor = Ptr.Get())
        {
            if (Actor->IsA(ActorClass))
                Result.Add(Actor);
        }
    }
    return Result;
}

UScenecapsModifierManager* AScenecapsActor::GetOrCreateModifierManager()
{
    if (!ModifierManager)
    {
        ModifierManager = NewObject<UScenecapsModifierManager>(this);
    }
    return ModifierManager;
}

void AScenecapsActor::ApplyModifiers(EScenecapsModifierPhase Phase, FName VariantName, FGuid EntryId)
{
    if (UScenecapsModifierManager* Mgr = GetOrCreateModifierManager())
    {
        Mgr->ExecuteModifiers(CollectionAsset.LoadSynchronous(), Phase, VariantName, EntryId, this);
    }
}

void AScenecapsActor::ApplyModifiersToActor(AActor* Actor, FName VariantName)
{
    if (!Actor) return;
    if (UScenecapsModifierManager* Mgr = GetOrCreateModifierManager())
    {
        Mgr->ExecuteModifiersOnActor(Actor, CollectionAsset.LoadSynchronous(), VariantName);
    }
}