
#include "ScenecapsStaticActor.h"

#include "ScenecapsStaticAsset.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "WorldPartition/DataLayer/DataLayerAsset.h"
#include "WorldPartition/DataLayer/DataLayerInstance.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"

AScenecapsStaticActor::AScenecapsStaticActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    PrimaryActorTick.TickInterval = 0.25f;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
}

void AScenecapsStaticActor::EnsureDataLayerCached()
{
    if (!CachedDataLayerAsset && !TargetDataLayer.IsNull())
        CachedDataLayerAsset = TargetDataLayer.LoadSynchronous();
}

void AScenecapsStaticActor::ApplyDataLayerToActor(AActor* Actor, UDataLayerAsset* LayerAsset) const
{
    if (!Actor || !LayerAsset) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UDataLayerManager* M = UDataLayerManager::GetDataLayerManager(World);
    if (!M) return;
    if (const UDataLayerInstance* Instance = M->GetDataLayerInstanceFromAsset(LayerAsset))
        Actor->AddDataLayer(Instance);
}

void AScenecapsStaticActor::RemoveDataLayerFromActor(AActor* Actor, UDataLayerAsset* LayerAsset) const
{
    if (!Actor || !LayerAsset) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UDataLayerManager* M = UDataLayerManager::GetDataLayerManager(World);
    if (!M) return;
    if (const UDataLayerInstance* Instance = M->GetDataLayerInstanceFromAsset(LayerAsset))
        Actor->RemoveDataLayer(Instance);
}

void AScenecapsStaticActor::Rebuild()
{
#if WITH_EDITOR
    Modify();
#endif
    Clear();
    EnsureDataLayerCached();
    if (ShouldDeferRebuild()) return;

    UScenecapsStaticAsset* Asset = CollectionAsset.LoadSynchronous();
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
        Actor->SetIsSpatiallyLoaded(false);
        if (Actor->GetAttachParentActor() == nullptr)
            Actor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        SpawnedActors.Add(Actor);
    }

    for (const TPair<FGuid, AActor*>& Pair : EntryToActor)
    {
        AActor* Actor = Pair.Value;
        if (!Actor) continue;

        UDataLayerAsset* LayerToUse = nullptr;
        if (const FScenecapsBakedEntry* Entry = Asset->Entries.FindByPredicate(
            [&](const FScenecapsBakedEntry& X) { return X.EntryId == Pair.Key; }))
        {
            if (!Entry->DataLayer.IsNull())
                LayerToUse = Entry->DataLayer.LoadSynchronous();
        }
        if (!LayerToUse) LayerToUse = CachedDataLayerAsset;
        if (LayerToUse) ApplyDataLayerToActor(Actor, LayerToUse);
    }
}

void AScenecapsStaticActor::Clear()
{
    UDataLayerAsset* LayerToRemove = CachedDataLayerAsset;
    if (!LayerToRemove && !TargetDataLayer.IsNull())
        LayerToRemove = TargetDataLayer.LoadSynchronous();

    if (LayerToRemove)
        for (TObjectPtr<AActor>& Ptr : SpawnedActors)
            if (AActor* Actor = Ptr.Get())
                if (IsValid(Actor)) RemoveDataLayerFromActor(Actor, LayerToRemove);

    for (TObjectPtr<AActor>& Ptr : SpawnedActors)
        if (AActor* Actor = Ptr.Get())
            if (IsValid(Actor)) Actor->Destroy();

    SpawnedActors.Empty();
}

void AScenecapsStaticActor::SetActiveVariant(FName InVariant)
{
    if (ActiveVariant == InVariant) return;
    ActiveVariant = InVariant;
    Rebuild();
}

void AScenecapsStaticActor::Destroyed() { Clear(); Super::Destroyed(); }
void AScenecapsStaticActor::PostDuplicate(bool bDuplicateForPIE)
{
    Super::PostDuplicate(bDuplicateForPIE);
    SpawnedActors.Empty();
    Rebuild();
}

void AScenecapsStaticActor::BeginPlay()
{
    Super::BeginPlay();
    EnsureDataLayerCached();
    if (bSpawnOnDataLayerLoaded && !TargetDataLayer.IsNull())
        SetActorTickEnabled(true);
    if (!ShouldDeferRebuild()) Rebuild();
}

void AScenecapsStaticActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SetActorTickEnabled(false);
    Super::EndPlay(EndPlayReason);
}

void AScenecapsStaticActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bSpawnOnDataLayerLoaded || TargetDataLayer.IsNull()) return;
    const bool bActive = IsTargetDataLayerActive();
    const bool bExists = SpawnedActors.Num() > 0;
    if (bActive && !bExists) Rebuild();
    else if (!bActive && bExists) Clear();
}

#if WITH_EDITOR
void AScenecapsStaticActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    const FName PropName = PropertyChangedEvent.GetPropertyName();
    if (PropName == GET_MEMBER_NAME_CHECKED(AScenecapsStaticActor, CollectionAsset) ||
        PropName == GET_MEMBER_NAME_CHECKED(AScenecapsStaticActor, bIncludeHidden) ||
        PropName == GET_MEMBER_NAME_CHECKED(AScenecapsStaticActor, ActiveVariant))
    {
        Rebuild();
    }
    else if (PropName == GET_MEMBER_NAME_CHECKED(AScenecapsStaticActor, TargetDataLayer) ||
        PropName == GET_MEMBER_NAME_CHECKED(AScenecapsStaticActor, bSpawnOnDataLayerLoaded))
    {
        CachedDataLayerAsset = TargetDataLayer.IsNull() ? nullptr : TargetDataLayer.LoadSynchronous();
        SetActorTickEnabled(bSpawnOnDataLayerLoaded && !TargetDataLayer.IsNull());
        Rebuild();
    }
}
#endif

bool AScenecapsStaticActor::IsTargetDataLayerActive() const
{
    if (TargetDataLayer.IsNull()) return true;
    UWorld* World = GetWorld();
    if (!World) return true;
    UDataLayerManager* M = UDataLayerManager::GetDataLayerManager(World);
    if (!M) return true;

    UDataLayerAsset* Asset = CachedDataLayerAsset
        ? CachedDataLayerAsset.Get()
        : TargetDataLayer.LoadSynchronous();
    if (!Asset) return true;

    const UDataLayerInstance* Instance = M->GetDataLayerInstanceFromAsset(Asset);
    if (!Instance) return true;

    return M->GetDataLayerInstanceRuntimeState(Instance) == EDataLayerRuntimeState::Activated;
}

bool AScenecapsStaticActor::ShouldDeferRebuild() const
{
    if (!bSpawnOnDataLayerLoaded) return false;
    if (TargetDataLayer.IsNull()) return false;
#if WITH_EDITOR
    if (GIsEditor && GetWorld() && GetWorld()->WorldType == EWorldType::Editor) return false;
#endif
    return !IsTargetDataLayerActive();
}

FText AScenecapsStaticActor::GetDataLayerStatusText() const
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