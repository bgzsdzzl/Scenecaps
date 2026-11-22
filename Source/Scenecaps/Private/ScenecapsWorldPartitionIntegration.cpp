// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsWorldPartitionIntegration.cpp

#include "ScenecapsWorldPartitionIntegration.h"
#include "ScenecapsModule.h"
#include "WorldPartition/DataLayer/DataLayerAsset.h"
#include "WorldPartition/DataLayer/DataLayerInstance.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

namespace
{
    // HLOD 级别映射：级别越高代表越粗糙的远景 HLOD，剔除距离越远、LOD 切换越早。
    constexpr int32 MinHLODLevel = 1;
    constexpr int32 MaxHLODLevel = 8;
    constexpr int32 HLODCullDistanceStep = 5000; // cm / 级

    int32 ResolveHLODLevel(int32 HLODLevel)
    {
        if (HLODLevel < MinHLODLevel)
        {
            UE_LOG(LogScenecaps, Warning, TEXT("[Scenecaps.WP] Invalid HLODLevel %d, clamped to %d."), HLODLevel, MinHLODLevel);
            return MinHLODLevel;
        }
        if (HLODLevel > MaxHLODLevel)
        {
            UE_LOG(LogScenecaps, Warning, TEXT("[Scenecaps.WP] HLODLevel %d exceeds %d, clamped."), HLODLevel, MaxHLODLevel);
            return MaxHLODLevel;
        }
        return HLODLevel;
    }

    void ApplyHLODSettings(UInstancedStaticMeshComponent* Component, int32 SafeLevel)
    {
        const int32 EndCullDistance = HLODCullDistanceStep * SafeLevel;
        const int32 StartCullDistance = EndCullDistance / 5; // 20% 淡出区间

        // UE5.8: 两者都是 UInstancedStaticMeshComponent 的公开 API，HISM 同样继承。
        Component->SetCullDistances(StartCullDistance, EndCullDistance);
        // InstanceLODDistanceScale 越小，LOD 切换越早（更早退化为低细节），符合 HLOD 用途。
        Component->SetLODDistanceScale(1.0f / static_cast<float>(SafeLevel));
    }
}

UScenecapsWorldPartitionIntegration::UScenecapsWorldPartitionIntegration()
{
}

void UScenecapsWorldPartitionIntegration::AutoAssignDataLayers(
    UWorld* World,
    const TArray<AActor*>& Actors,
    const FScenecapsWorldPartitionSettings& Settings)
{
    if (!World || !Settings.bAutoAssignDataLayers || Actors.Num() == 0) return;

    UDataLayerManager* DataLayerManager = UDataLayerManager::GetDataLayerManager(World);
    if (!DataLayerManager) return;

    // DistanceBased 模式的参考点：玩家 Pawn -> 玩家相机 -> Actor 列表中心
    FVector ReferencePoint = FVector::ZeroVector;
    bool bHasReferencePoint = false;

    if (APlayerController* PlayerController = World->GetFirstPlayerController())
    {
        if (APawn* PlayerPawn = PlayerController->GetPawn())
        {
            ReferencePoint = PlayerPawn->GetActorLocation();
        }
        else
        {
            FVector CameraLocation;
            FRotator CameraRotation;
            PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
            ReferencePoint = CameraLocation;
        }
        bHasReferencePoint = true;
    }

    if (!bHasReferencePoint)
    {
        FVector ActorsCenter = FVector::ZeroVector;
        int32 ValidActorCount = 0;
        for (AActor* Actor : Actors)
        {
            if (!Actor) continue;
            ActorsCenter += Actor->GetActorLocation();
            ++ValidActorCount;
        }
        if (ValidActorCount > 0)
        {
            ReferencePoint = ActorsCenter / static_cast<float>(ValidActorCount);
            bHasReferencePoint = true;
        }
        UE_LOG(LogScenecaps, Log,
            TEXT("[Scenecaps.WP] No player pawn/camera found, DistanceBased reference point falls back to actor center (%d actors): %s"),
            ValidActorCount, *ReferencePoint.ToString());
    }

    for (AActor* Actor : Actors)
    {
        if (!Actor) continue;

        for (const FScenecapsSpatialDataLayerRule& Rule : Settings.SpatialRules)
        {
            if (!Rule.DataLayer.IsValid()) continue;

            bool bMatches = true;

            if (Rule.RequiredTags.Num() > 0)
            {
                bool bHasAll = true;
                for (FName Tag : Rule.RequiredTags)
                {
                    if (!Actor->ActorHasTag(Tag))
                    {
                        bHasAll = false;
                        break;
                    }
                }
                if (!bHasAll) bMatches = false;
            }

            if (Rule.ExcludedTags.Num() > 0)
            {
                for (FName Tag : Rule.ExcludedTags)
                {
                    if (Actor->ActorHasTag(Tag))
                    {
                        bMatches = false;
                        break;
                    }
                }
            }

            if (bMatches && Rule.Mode == EScenecapsDataLayerAssignmentMode::SpatialGrid)
            {
                FVector ActorLoc = Actor->GetActorLocation();
                if (!Rule.Bounds.IsInsideOrOn(ActorLoc))
                {
                    bMatches = false;
                }
            }

            if (bMatches && Rule.Mode == EScenecapsDataLayerAssignmentMode::DistanceBased)
            {
                if (!bHasReferencePoint ||
                    FVector::Dist(Actor->GetActorLocation(), ReferencePoint) > Rule.MaxDistance)
                {
                    bMatches = false;
                }
            }

            if (bMatches)
            {
                UDataLayerAsset* DataLayer = Rule.DataLayer.LoadSynchronous();
                if (DataLayer)
                {
                    AssignDataLayerToActor(Actor, DataLayer, true);
                }
            }
        }
    }
}

void UScenecapsWorldPartitionIntegration::AssignDataLayerToActor(
    AActor* Actor,
    UDataLayerAsset* DataLayer,
    bool bActivate)
{
#if WITH_EDITOR
    if (!Actor || !DataLayer) return;

    UWorld* World = Actor->GetWorld();
    if (!World) return;

    UDataLayerManager* Manager = UDataLayerManager::GetDataLayerManager(World);
    if (!Manager) return;

    const UDataLayerInstance* Instance = Manager->GetDataLayerInstanceFromAsset(DataLayer);
    if (!Instance) return;

    Actor->AddDataLayer(const_cast<UDataLayerInstance*>(Instance));

    if (bActivate)
    {
        Manager->SetDataLayerInstanceRuntimeState(const_cast<UDataLayerInstance*>(Instance), EDataLayerRuntimeState::Activated);
    }
#endif
}

void UScenecapsWorldPartitionIntegration::RemoveDataLayerFromActor(
    AActor* Actor,
    UDataLayerAsset* DataLayer)
{
#if WITH_EDITOR
    if (!Actor || !DataLayer) return;

    UWorld* World = Actor->GetWorld();
    if (!World) return;

    UDataLayerManager* Manager = UDataLayerManager::GetDataLayerManager(World);
    if (!Manager) return;

    const UDataLayerInstance* Instance = Manager->GetDataLayerInstanceFromAsset(DataLayer);
    if (Instance)
    {
        Actor->RemoveDataLayer(const_cast<UDataLayerInstance*>(Instance));
    }
#endif
}

void UScenecapsWorldPartitionIntegration::SetDataLayerRuntimeState(
    UWorld* World,
    UDataLayerAsset* DataLayer,
    bool bActivate)
{
    if (!World || !DataLayer) return;

    UDataLayerManager* Manager = UDataLayerManager::GetDataLayerManager(World);
    if (!Manager) return;

    const UDataLayerInstance* Instance = Manager->GetDataLayerInstanceFromAsset(DataLayer);
    if (Instance)
    {
        Manager->SetDataLayerInstanceRuntimeState(const_cast<UDataLayerInstance*>(Instance), bActivate ? EDataLayerRuntimeState::Activated : EDataLayerRuntimeState::Unloaded);
    }
}

EDataLayerRuntimeState UScenecapsWorldPartitionIntegration::GetDataLayerRuntimeState(
    UWorld* World,
    UDataLayerAsset* DataLayer)
{
    if (!World || !DataLayer) return EDataLayerRuntimeState::Unloaded;

    UDataLayerManager* Manager = UDataLayerManager::GetDataLayerManager(World);
    if (!Manager) return EDataLayerRuntimeState::Unloaded;

    const UDataLayerInstance* Instance = Manager->GetDataLayerInstanceFromAsset(DataLayer);
    if (!Instance) return EDataLayerRuntimeState::Unloaded;

    return Manager->GetDataLayerInstanceRuntimeState(Instance);
}

void UScenecapsWorldPartitionIntegration::SetDataLayersRuntimeState(
    UWorld* World,
    const TArray<UDataLayerAsset*>& DataLayers,
    bool bActivate)
{
    if (!World || DataLayers.Num() == 0) return;

    UDataLayerManager* Manager = UDataLayerManager::GetDataLayerManager(World);
    if (!Manager) return;

    for (UDataLayerAsset* DataLayer : DataLayers)
    {
        if (DataLayer)
        {
            const UDataLayerInstance* Instance = Manager->GetDataLayerInstanceFromAsset(DataLayer);
            if (Instance)
            {
                Manager->SetDataLayerInstanceRuntimeState(const_cast<UDataLayerInstance*>(Instance), bActivate ? EDataLayerRuntimeState::Activated : EDataLayerRuntimeState::Unloaded);
            }
        }
    }
}

void UScenecapsWorldPartitionIntegration::RegisterISMToHLOD(
    UInstancedStaticMeshComponent* ISM,
    int32 HLODLevel)
{
    if (!ISM) return;

    const int32 SafeLevel = ResolveHLODLevel(HLODLevel);
    ApplyHLODSettings(ISM, SafeLevel);
    ISM->SetCanEverAffectNavigation(false);

    UE_LOG(LogScenecaps, Log,
        TEXT("[Scenecaps.WP] Registered ISM to HLOD level %d: cull %d/%d cm, LOD distance scale %.3f"),
        SafeLevel, HLODCullDistanceStep * SafeLevel / 5, HLODCullDistanceStep * SafeLevel, 1.0f / static_cast<float>(SafeLevel));
}

void UScenecapsWorldPartitionIntegration::RegisterHISMToHLOD(
    UHierarchicalInstancedStaticMeshComponent* HISM,
    int32 HLODLevel)
{
    if (!HISM) return;

    const int32 SafeLevel = ResolveHLODLevel(HLODLevel);
    ApplyHLODSettings(HISM, SafeLevel);
    HISM->SetCanEverAffectNavigation(false);

    // 近景级别（1）同步构建保证立即可见；远景级别异步构建避免构建集群树时卡顿。
    HISM->BuildTreeIfOutdated(/*Async=*/SafeLevel > MinHLODLevel, /*ForceUpdate=*/false);

    UE_LOG(LogScenecaps, Log,
        TEXT("[Scenecaps.WP] Registered HISM to HLOD level %d: cull %d/%d cm, LOD distance scale %.3f, async tree rebuild %s"),
        SafeLevel, HLODCullDistanceStep * SafeLevel / 5, HLODCullDistanceStep * SafeLevel,
        1.0f / static_cast<float>(SafeLevel), SafeLevel > MinHLODLevel ? TEXT("true") : TEXT("false"));
}
