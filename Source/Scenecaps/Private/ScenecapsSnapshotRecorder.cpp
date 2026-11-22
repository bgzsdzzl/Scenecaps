// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsSnapshotRecorder.cpp

#include "ScenecapsSnapshotRecorder.h"
#include "ScenecapsAsset.h"
#include "ScenecapsModule.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "PackageTools.h"
#endif
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
#if WITH_EDITOR
    OutEntry.DisplayName = Actor->GetActorLabel();
#else
    OutEntry.DisplayName = Actor->GetName();
#endif
    OutEntry.RelativeTransform = Actor->GetActorTransform().GetRelativeTransform(RootTransform);
    OutEntry.bVisible = !Actor->IsHidden();
    OutEntry.bLocked = false;

    TArray<UStaticMeshComponent*> MeshComps;
    Actor->GetComponents<UStaticMeshComponent>(MeshComps);

    // 找到第一个真正持有 StaticMesh 的组件，避免 MeshComps[0] 为空网格时漏掉资产
    UStaticMeshComponent* MeshComp = nullptr;
    for (UStaticMeshComponent* Candidate : MeshComps)
    {
        if (Candidate && Candidate->GetStaticMesh())
        {
            MeshComp = Candidate;
            break;
        }
    }

    if (MeshComp)
    {
        OutEntry.Asset = Cast<UObject>(MeshComp->GetStaticMesh());

        OutEntry.bOverrideMobility = true;
        OutEntry.Mobility = MeshComp->Mobility;

        OutEntry.bOverrideCollision = true;
        OutEntry.CollisionEnabled = MeshComp->GetCollisionEnabled();

        OutEntry.bOverrideCastShadow = true;
        OutEntry.bCastShadow = MeshComp->CastShadow;
    }

    // 材质覆盖同理：遍历所有组件的材质槽，取第一个有效材质，而不是硬编码 MeshComps[0] 的第 0 槽
    for (UStaticMeshComponent* Candidate : MeshComps)
    {
        if (!Candidate) continue;
        for (int32 SlotIndex = 0; SlotIndex < Candidate->GetNumMaterials(); ++SlotIndex)
        {
            UMaterialInterface* Mat = Candidate->GetMaterial(SlotIndex);
            if (Mat)
            {
                OutEntry.MaterialOverride = Mat;
                OutEntry.MaterialSlotIndex = SlotIndex;
                break;
            }
        }
        if (!OutEntry.MaterialOverride.IsNull()) break;
    }
}

FGuid UScenecapsSnapshotRecorder::FindOrCreateParentId(AActor* Child, const TMap<AActor*, FGuid>& ActorToId) const
{
    if (!Child) return FGuid();

    AActor* Parent = Child->GetAttachParentActor();
    if (!Parent) return FGuid();

    if (const FGuid* ParentId = ActorToId.Find(Parent))
    {
        return *ParentId;
    }

    return FindOrCreateParentId(Parent, ActorToId);
}

UScenecapsAsset* UScenecapsSnapshotRecorder::SnapshotActorsAsSceneAsset(
    const TArray<AActor*>& Actors, 
    const FScenecapsSnapshotSettings& Settings, 
    UObject* Outer)
{
    if (Actors.Num() == 0) return nullptr;

#if WITH_EDITOR
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
            Entry.ParentId = FindOrCreateParentId(Actor, ActorToId);
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

        // 为每个 Entry 生成稀疏覆盖，真正记录快照此刻的状态，而不是留一个空变体
        Variant.Overrides.Reserve(Asset->Entries.Num());
        for (const FScenecapsEntry& Entry : Asset->Entries)
        {
            FScenecapsEntryOverride Override;
            Override.EntryId = Entry.EntryId;
            Override.DisplayName = Entry.DisplayName;

            Override.bOverrideTransform = true;
            Override.RelativeTransform = Entry.RelativeTransform;

            Override.bOverrideVisible = true;
            Override.bVisible = Entry.bVisible;

            if (!Entry.MaterialOverride.IsNull())
            {
                Override.bOverrideMaterial = true;
                Override.MaterialOverride = Entry.MaterialOverride;
                Override.MaterialSlotIndex = Entry.MaterialSlotIndex;
            }

            if (Entry.FloatParams.Num() > 0)
            {
                Override.bOverrideFloatParams = true;
                Override.FloatParams = Entry.FloatParams;
            }
            if (Entry.VectorParams.Num() > 0)
            {
                Override.bOverrideVectorParams = true;
                Override.VectorParams = Entry.VectorParams;
            }
            if (Entry.ColorParams.Num() > 0)
            {
                Override.bOverrideColorParams = true;
                Override.ColorParams = Entry.ColorParams;
            }
            if (Entry.BoolParams.Num() > 0)
            {
                Override.bOverrideBoolParams = true;
                Override.BoolParams = Entry.BoolParams;
            }
            if (Entry.ProceduralParams.IsValid())
            {
                Override.bOverrideProceduralParams = true;
                Override.ProceduralParams = Entry.ProceduralParams;
            }

            Variant.Overrides.Add(MoveTemp(Override));
        }

        Asset->Variants.Add(MoveTemp(Variant));
    }

    return Asset;
#else
    return nullptr;
#endif
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
