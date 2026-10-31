// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsSnapshotRecorder.cpp

#include "ScenecapsSnapshotRecorder.h"
#include "ScenecapsAsset.h"
#include "ScenecapsModule.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "PackageTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/GameplayStatics.h"

UScenecapsSnapshotRecorder::UScenecapsSnapshotRecorder()
{
}

TArray<AActor*> UScenecapsSnapshotRecorder::CollectActors(UWorld* World, const FScenecapsSnapshotSettings& Settings) const
{
    TArray<AActor*> Result;
    if (!World) return Result;

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (!Actor) continue;

        if (!Settings.bIncludeHiddenActors && Actor->IsHidden()) continue;

        bool bMatchesClass = true;
        if (Settings.Filter == EScenecapsSnapshotFilter::ClassFilter && Settings.AllowedClasses.Num() > 0)
        {
            bMatchesClass = false;
            for (TSubclassOf<AActor> AllowedClass : Settings.AllowedClasses)
            {
                if (Actor->IsA(AllowedClass))
                {
                    bMatchesClass = true;
                    break;
                }
            }
        }
        if (!bMatchesClass) continue;

        if (Settings.RequiredTags.Num() > 0)
        {
            bool bHasAllRequired = true;
            for (FName RequiredTag : Settings.RequiredTags)
            {
                if (!Actor->ActorHasTag(RequiredTag))
                {
                    bHasAllRequired = false;
                    break;
                }
            }
            if (!bHasAllRequired) continue;
        }

        if (Settings.ExcludedTags.Num() > 0)
        {
            bool bHasExcluded = false;
            for (FName ExcludedTag : Settings.ExcludedTags)
            {
                if (Actor->ActorHasTag(ExcludedTag))
                {
                    bHasExcluded = true;
                    break;
                }
            }
            if (bHasExcluded) continue;
        }

        Result.Add(Actor);
    }

    return Result;
}

void UScenecapsSnapshotRecorder::BuildEntryFromActor(AActor* Actor, FScenecapsEntry& OutEntry, const FTransform& RootTransform) const
{
    if (!Actor) return;

    OutEntry.EntryId = FGuid::NewGuid();
    OutEntry.DisplayName = Actor->GetActorLabel();
    OutEntry.RelativeTransform = Actor->GetActorTransform().GetRelativeTransform(RootTransform);
    OutEntry.bVisible = !Actor->IsHidden();
    OutEntry.bLocked = false;

    TArray<UStaticMeshComponent*> MeshComps;
    Actor->GetComponents<UStaticMeshComponent>(MeshComps);
    if (MeshComps.Num() > 0 && MeshComps[0]->GetStaticMesh())
    {
        OutEntry.Asset = MeshComps[0]->GetStaticMesh();
    }

    if (MeshComps.Num() > 0)
    {
        UMaterialInterface* Mat = MeshComps[0]->GetMaterial(0);
        if (Mat)
        {
            OutEntry.MaterialOverride = Mat;
            OutEntry.MaterialSlotIndex = 0;
        }
    }

    if (MeshComps.Num() > 0)
    {
        OutEntry.bOverrideMobility = true;
        OutEntry.Mobility = MeshComps[0]->Mobility;

        OutEntry.bOverrideCollision = true;
        OutEntry.CollisionEnabled = MeshComps[0]->GetCollisionEnabled();

        OutEntry.bOverrideCastShadow = true;
        OutEntry.bCastShadow = MeshComps[0]->CastShadow;
    }
}

FGuid UScenecapsSnapshotRecorder::FindOrCreateParentId(const TArray<AActor*>& Actors, AActor* Child, const TMap<AActor*, FGuid>& ActorToId) const
{
    if (!Child) return FGuid();

    AActor* Parent = Child->GetAttachParentActor();
    if (!Parent) return FGuid();

    if (const FGuid* ParentId = ActorToId.Find(Parent))
    {
        return *ParentId;
    }

    return FindOrCreateParentId(Actors, Parent, ActorToId);
}

UScenecapsAsset* UScenecapsSnapshotRecorder::SnapshotActorsAsSceneAsset(
    const TArray<AActor*>& Actors, 
    const FScenecapsSnapshotSettings& Settings, 
    UObject* Outer)
{
    if (Actors.Num() == 0) return nullptr;

    UScenecapsAsset* Asset = NewObject<UScenecapsAsset>(Outer ? Outer : GetTransientPackage(), NAME_None, RF_Transactional);
    if (!Asset) return nullptr;

    Asset->RootId = FGuid::NewGuid();
    Asset->Entries.Reset();
    Asset->Variants.Reset();

    TMap<AActor*, FGuid> ActorToId;
    ActorToId.Reserve(Actors.Num());

    FTransform RootTransform = FTransform::Identity;
    if (Actors.Num() > 0)
    {
        FVector Center = FVector::ZeroVector;
        for (AActor* Actor : Actors) Center += Actor->GetActorLocation();
        Center /= Actors.Num();
        RootTransform = FTransform(Center);
    }

    for (AActor* Actor : Actors)
    {
        FScenecapsEntry Entry;
        BuildEntryFromActor(Actor, Entry, RootTransform);
        
        if (Settings.bPreserveHierarchy)
        {
            Entry.ParentId = FindOrCreateParentId(Actors, Actor, ActorToId);
            if (Entry.ParentId.IsValid() == false)
            {
                Entry.ParentId = Asset->RootId;
            }
        }
        else
        {
            Entry.ParentId = Asset->RootId;
        }

        ActorToId.Add(Actor, Entry.EntryId);
        Asset->Entries.Add(MoveTemp(Entry));
    }

    if (Settings.bAutoGenerateVariant && !Settings.NewVariantName.IsNone())
    {
        FScenecapsVariant Variant;
        Variant.Name = Settings.NewVariantName;
        Variant.Description = Settings.NewVariantDescription;
        Asset->Variants.Add(MoveTemp(Variant));
    }

    return Asset;
}

UScenecapsAsset* UScenecapsSnapshotRecorder::SnapshotWorldAsSceneAsset(UWorld* World, const FScenecapsSnapshotSettings& Settings)
{
    TArray<AActor*> Actors = CollectActors(World, Settings);
    return SnapshotActorsAsSceneAsset(Actors, Settings);
}

#if WITH_EDITOR
UScenecapsAsset* UScenecapsSnapshotRecorder::SnapshotAndSaveAsSceneAsset(
    UWorld* World, 
    const FScenecapsSnapshotSettings& Settings, 
    const FString& PackagePath, 
    const FString& AssetName)
{
    UScenecapsAsset* Asset = SnapshotWorldAsSceneAsset(World, Settings);
    if (!Asset) return nullptr;

    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package) return nullptr;

    Asset->Rename(*AssetName, Package, REN_DontCreateRedirectors | REN_DoNotDirty);
    Asset->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Asset);

    return Asset;
}
#endif

TArray<AActor*> UScenecapsSnapshotRecorder::SpawnSceneSnapshot(UWorld* World, UScenecapsAsset* Asset, const FTransform& WorldTransform, FName VariantName)
{
    if (!World || !Asset) return TArray<AActor*>();
    return Asset->SpawnCollection(World, WorldTransform, false, VariantName);
}
