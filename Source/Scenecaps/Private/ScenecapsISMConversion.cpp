// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsISMConversion.cpp

#include "ScenecapsISMConversion.h"
#include "ScenecapsActorPool.h"
#include "Engine/StaticMeshActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

UScenecapsISMConversion::UScenecapsISMConversion()
{
}

// ================================================================
// ISM -> Actor
// ================================================================

TArray<AActor*> UScenecapsISMConversion::ConvertISMToActors(
    UInstancedStaticMeshComponent* ISM,
    const FScenecapsISMToActorSettings& Settings,
    UWorld* World,
    UScenecapsActorPoolComponent* Pool)
{
    TArray<AActor*> Result;
    if (!ISM || !World || !ISM->GetStaticMesh()) return Result;

    UStaticMesh* Mesh = ISM->GetStaticMesh();
    int32 InstanceCount = ISM->GetInstanceCount();
    Result.Reserve(InstanceCount);

    TSubclassOf<AActor> ActorClass = Settings.OverrideActorClass;
    if (!ActorClass)
    {
        ActorClass = AStaticMeshActor::StaticClass();
    }

    for (int32 i = 0; i < InstanceCount; ++i)
    {
        FTransform InstanceTransform;
        if (!ISM->GetInstanceTransform(i, InstanceTransform, true)) continue;

        AActor* Actor = nullptr;

        if (Settings.bUseActorPool && Pool)
        {
            Actor = Pool->AcquireActor(Settings.PoolKey, InstanceTransform);
        }
        else
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            SpawnParams.ObjectFlags |= RF_Transactional;
            Actor = World->SpawnActor<AActor>(ActorClass, InstanceTransform, SpawnParams);
        }

        if (!Actor) continue;

        if (AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Actor))
        {
            StaticMeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
        }
        else if (UStaticMeshComponent* MeshComp = Actor->FindComponentByClass<UStaticMeshComponent>())
        {
            MeshComp->SetStaticMesh(Mesh);
        }

        Result.Add(Actor);
    }

    return Result;
}

TArray<AActor*> UScenecapsISMConversion::ConvertHISMToActors(
    UHierarchicalInstancedStaticMeshComponent* HISM,
    const FScenecapsISMToActorSettings& Settings,
    UWorld* World,
    UScenecapsActorPoolComponent* Pool)
{
    TArray<AActor*> Result;
    if (!HISM || !World || !HISM->GetStaticMesh()) return Result;

    UStaticMesh* Mesh = HISM->GetStaticMesh();
    int32 InstanceCount = HISM->GetInstanceCount();
    Result.Reserve(InstanceCount);

    TSubclassOf<AActor> ActorClass = Settings.OverrideActorClass;
    if (!ActorClass)
    {
        ActorClass = AStaticMeshActor::StaticClass();
    }

    for (int32 i = 0; i < InstanceCount; ++i)
    {
        FTransform InstanceTransform;
        if (!HISM->GetInstanceTransform(i, InstanceTransform, true)) continue;

        AActor* Actor = nullptr;

        if (Settings.bUseActorPool && Pool)
        {
            Actor = Pool->AcquireActor(Settings.PoolKey, InstanceTransform);
        }
        else
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            SpawnParams.ObjectFlags |= RF_Transactional;
            Actor = World->SpawnActor<AActor>(ActorClass, InstanceTransform, SpawnParams);
        }

        if (!Actor) continue;

        if (AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Actor))
        {
            StaticMeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
        }
        else if (UStaticMeshComponent* MeshComp = Actor->FindComponentByClass<UStaticMeshComponent>())
        {
            MeshComp->SetStaticMesh(Mesh);
        }

        Result.Add(Actor);
    }

    return Result;
}

// ================================================================
// Actor -> ISM
// ================================================================

UInstancedStaticMeshComponent* UScenecapsISMConversion::ConvertActorsToISM(
    const TArray<AActor*>& Actors,
    const FScenecapsActorToISMSettings& Settings,
    UWorld* World,
    AActor* ContainerActor)
{
    if (!World || Actors.Num() == 0) return nullptr;

    TMap<UStaticMesh*, TArray<FTransform>> MeshToTransforms;

    for (AActor* Actor : Actors)
    {
        if (!Actor) continue;

        TArray<UStaticMeshComponent*> MeshComps;
        Actor->GetComponents<UStaticMeshComponent>(MeshComps);

        for (UStaticMeshComponent* MeshComp : MeshComps)
        {
            if (UStaticMesh* Mesh = MeshComp->GetStaticMesh())
            {
                MeshToTransforms.FindOrAdd(Mesh).Add(MeshComp->GetComponentTransform());
            }
        }
    }

    if (MeshToTransforms.Num() == 0) return nullptr;

    if (!ContainerActor)
    {
        ContainerActor = World->SpawnActor<AActor>();
        ContainerActor->SetActorLabel(TEXT("ISMContainer_Converted"));
    }

    USceneComponent* Root = ContainerActor->GetRootComponent();
    if (!Root)
    {
        Root = NewObject<USceneComponent>(ContainerActor, TEXT("ISMRoot"));
        ContainerActor->SetRootComponent(Root);
    }

    UInstancedStaticMeshComponent* FirstISM = nullptr;

    for (auto& Pair : MeshToTransforms)
    {
        UStaticMesh* Mesh = Pair.Key;
        const TArray<FTransform>& Transforms = Pair.Value;

        UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(ContainerActor);
        ISM->SetStaticMesh(Mesh);
        ISM->RegisterComponentWithWorld(World);
        ISM->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);

        ISM->SetNumCustomDataFloats(4);

        for (int32 i = 0; i < Transforms.Num(); ++i)
        {
            ISM->AddInstance(Transforms[i], true);
        }

        // Lumen 设置
        ISM->SetAffectDistanceFieldLighting(true);
        ISM->SetAffectDynamicIndirectLighting(true);
        ISM->SetCastShadow(true);
        ISM->SetLightingChannels(false, true, false);
        ISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        ISM->SetGenerateOverlapEvents(false);
        ISM->SetCanEverAffectNavigation(false);
        ISM->bHasPerInstanceHitProxies = false;

        if (!FirstISM) FirstISM = ISM;
    }

    if (Settings.bDestroySourceActors)
    {
        for (AActor* Actor : Actors)
        {
            if (Actor && Actor->IsValidLowLevel())
            {
                Actor->Destroy();
            }
        }
    }

    return FirstISM;
}

UHierarchicalInstancedStaticMeshComponent* UScenecapsISMConversion::ConvertActorsToHISM(
    const TArray<AActor*>& Actors,
    const FScenecapsActorToISMSettings& Settings,
    UWorld* World,
    AActor* ContainerActor)
{
    if (!World || Actors.Num() == 0) return nullptr;

    TMap<UStaticMesh*, TArray<FTransform>> MeshToTransforms;

    for (AActor* Actor : Actors)
    {
        if (!Actor) continue;

        TArray<UStaticMeshComponent*> MeshComps;
        Actor->GetComponents<UStaticMeshComponent>(MeshComps);

        for (UStaticMeshComponent* MeshComp : MeshComps)
        {
            if (UStaticMesh* Mesh = MeshComp->GetStaticMesh())
            {
                MeshToTransforms.FindOrAdd(Mesh).Add(MeshComp->GetComponentTransform());
            }
        }
    }

    if (MeshToTransforms.Num() == 0) return nullptr;

    if (!ContainerActor)
    {
        ContainerActor = World->SpawnActor<AActor>();
        ContainerActor->SetActorLabel(TEXT("HISMContainer_Converted"));
    }

    USceneComponent* Root = ContainerActor->GetRootComponent();
    if (!Root)
    {
        Root = NewObject<USceneComponent>(ContainerActor, TEXT("HISMRoot"));
        ContainerActor->SetRootComponent(Root);
    }

    UHierarchicalInstancedStaticMeshComponent* FirstHISM = nullptr;

    for (auto& Pair : MeshToTransforms)
    {
        UStaticMesh* Mesh = Pair.Key;
        const TArray<FTransform>& Transforms = Pair.Value;

        UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(ContainerActor);
        HISM->SetStaticMesh(Mesh);
        HISM->RegisterComponentWithWorld(World);
        HISM->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);

        // HISM LOD 设置（API 可能不同，使用注释保留）
        // HISM->SetNumLODs(FMath::Clamp(Settings.NumLODs, 1, 8));
        // HISM->SetLODScreenSize(Settings.LODScreenSize);
        // HISM->SetEnableAutoLODs(true);
        HISM->bEnableDensityScaling = true;

        for (int32 i = 0; i < Transforms.Num(); ++i)
        {
            HISM->AddInstance(Transforms[i], true);
        }

        // Lumen 设置
        HISM->SetAffectDistanceFieldLighting(true);
        HISM->SetAffectDynamicIndirectLighting(true);
        HISM->SetCastShadow(true);
        HISM->SetLightingChannels(false, true, false);
        HISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        HISM->SetGenerateOverlapEvents(false);
        HISM->SetCanEverAffectNavigation(false);
        HISM->bHasPerInstanceHitProxies = false;

        HISM->BuildTreeIfOutdated(true, false);

        if (!FirstHISM) FirstHISM = HISM;
    }

    if (Settings.bDestroySourceActors)
    {
        for (AActor* Actor : Actors)
        {
            if (Actor && Actor->IsValidLowLevel())
            {
                Actor->Destroy();
            }
        }
    }

    return FirstHISM;
}

// ================================================================
// 实用工具
// ================================================================

int32 UScenecapsISMConversion::GetISMInstanceCount(UInstancedStaticMeshComponent* ISM)
{
    return ISM ? ISM->GetInstanceCount() : 0;
}

int32 UScenecapsISMConversion::GetHISMInstanceCount(UHierarchicalInstancedStaticMeshComponent* HISM)
{
    return HISM ? HISM->GetInstanceCount() : 0;
}

bool UScenecapsISMConversion::GetISMInstanceTransform(UInstancedStaticMeshComponent* ISM, int32 Index, FTransform& OutTransform)
{
    if (!ISM) return false;
    return ISM->GetInstanceTransform(Index, OutTransform, true);
}

bool UScenecapsISMConversion::GetHISMInstanceTransform(UHierarchicalInstancedStaticMeshComponent* HISM, int32 Index, FTransform& OutTransform)
{
    if (!HISM) return false;
    return HISM->GetInstanceTransform(Index, OutTransform, true);
}

bool UScenecapsISMConversion::SetISMInstanceTransform(UInstancedStaticMeshComponent* ISM, int32 Index, const FTransform& Transform)
{
    if (!ISM) return false;
    ISM->UpdateInstanceTransform(Index, Transform, true, true);
    return true;
}

bool UScenecapsISMConversion::SetHISMInstanceTransform(UHierarchicalInstancedStaticMeshComponent* HISM, int32 Index, const FTransform& Transform)
{
    if (!HISM) return false;
    HISM->UpdateInstanceTransform(Index, Transform, true, true);
    return true;
}

