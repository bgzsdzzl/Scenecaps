// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsWorldPartitionIntegration.cpp

#include "ScenecapsWorldPartitionIntegration.h"
#include "WorldPartition/DataLayer/DataLayerAsset.h"
#include "WorldPartition/DataLayer/DataLayerInstance.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "Editor.h"

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
                // 需要参考点（如玩家/摄像机位置）
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
}

void UScenecapsWorldPartitionIntegration::RemoveDataLayerFromActor(
    AActor* Actor,
    UDataLayerAsset* DataLayer)
{
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
    ISM->SetCanEverAffectNavigation(false);
}

void UScenecapsWorldPartitionIntegration::RegisterHISMToHLOD(
    UHierarchicalInstancedStaticMeshComponent* HISM,
    int32 HLODLevel)
{
    if (!HISM) return;
    HISM->SetCanEverAffectNavigation(false);
    HISM->BuildTreeIfOutdated(true, false);
}

// Editor function disabled for compilation



