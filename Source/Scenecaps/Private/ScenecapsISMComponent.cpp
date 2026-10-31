// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsISMComponent.cpp

#include "ScenecapsISMComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

UScenecapsISMComponent::UScenecapsISMComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UScenecapsISMComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UScenecapsISMComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RegisteredISMs.Empty();
    RegisteredHISMs.Empty();
    Super::EndPlay(EndPlayReason);
}

void UScenecapsISMComponent::RegisterContainerActor(AActor* ContainerActor)
{
    if (!ContainerActor) return;

    RegisteredISMs.Empty();
    RegisteredHISMs.Empty();

    ContainerActor->GetComponents<UInstancedStaticMeshComponent>(RegisteredISMs);
    ContainerActor->GetComponents<UHierarchicalInstancedStaticMeshComponent>(RegisteredHISMs);
}

TArray<UInstancedStaticMeshComponent*> UScenecapsISMComponent::GetRegisteredISMs() const
{
    return RegisteredISMs;
}

TArray<UHierarchicalInstancedStaticMeshComponent*> UScenecapsISMComponent::GetRegisteredHISMs() const
{
    return RegisteredHISMs;
}

UInstancedStaticMeshComponent* UScenecapsISMComponent::FindISMByMesh(UStaticMesh* Mesh) const
{
    if (!Mesh) return nullptr;
    for (UInstancedStaticMeshComponent* ISM : RegisteredISMs)
    {
        if (ISM && ISM->GetStaticMesh() == Mesh)
            return ISM;
    }
    return nullptr;
}

UHierarchicalInstancedStaticMeshComponent* UScenecapsISMComponent::FindHISMByMesh(UStaticMesh* Mesh) const
{
    if (!Mesh) return nullptr;
    for (UHierarchicalInstancedStaticMeshComponent* HISM : RegisteredHISMs)
    {
        if (HISM && HISM->GetStaticMesh() == Mesh)
            return HISM;
    }
    return nullptr;
}

// ---- ISM 基础操作 ----

int32 UScenecapsISMComponent::AddInstanceToISM(UInstancedStaticMeshComponent* ISM, const FTransform& WorldTransform)
{
    if (!ISM) return -1;
    return ISM->AddInstance(WorldTransform, true);
}

TArray<int32> UScenecapsISMComponent::AddInstancesToISM(UInstancedStaticMeshComponent* ISM, const TArray<FTransform>& WorldTransforms)
{
    TArray<int32> Indices;
    if (!ISM) return Indices;

    Indices.Reserve(WorldTransforms.Num());
    for (const FTransform& T : WorldTransforms)
    {
        Indices.Add(ISM->AddInstance(T, true));
    }
    return Indices;
}

bool UScenecapsISMComponent::RemoveInstanceFromISM(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, bool bSwapRemove)
{
    if (!ISM) return false;
    if (InstanceIndex < 0 || InstanceIndex >= ISM->GetInstanceCount()) return false;

    if (bSwapRemove)
    {
        return ISM->RemoveInstance(InstanceIndex);
    }
    else
    {
        int32 Count = ISM->GetInstanceCount();
        for (int32 i = InstanceIndex; i < Count - 1; ++i)
        {
            FTransform T;
            if (ISM->GetInstanceTransform(i + 1, T, true))
            {
                ISM->UpdateInstanceTransform(i, T, true, false);
            }
        }
        return ISM->RemoveInstance(Count - 1);
    }
}

bool UScenecapsISMComponent::UpdateInstanceTransformISM(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, const FTransform& NewWorldTransform)
{
    if (!ISM) return false;
    if (InstanceIndex < 0 || InstanceIndex >= ISM->GetInstanceCount()) return false;

    ISM->UpdateInstanceTransform(InstanceIndex, NewWorldTransform, true, true);
    return true;
}

bool UScenecapsISMComponent::GetInstanceTransformISM(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, FTransform& OutWorldTransform) const
{
    if (!ISM) return false;
    if (InstanceIndex < 0 || InstanceIndex >= ISM->GetInstanceCount()) return false;

    return ISM->GetInstanceTransform(InstanceIndex, OutWorldTransform, true);
}

int32 UScenecapsISMComponent::GetInstanceCountISM(UInstancedStaticMeshComponent* ISM) const
{
    return ISM ? ISM->GetInstanceCount() : 0;
}

// ---- HISM 基础操作 ----

int32 UScenecapsISMComponent::AddInstanceToHISM(UHierarchicalInstancedStaticMeshComponent* HISM, const FTransform& WorldTransform)
{
    if (!HISM) return -1;
    return HISM->AddInstance(WorldTransform, true);
}

TArray<int32> UScenecapsISMComponent::AddInstancesToHISM(UHierarchicalInstancedStaticMeshComponent* HISM, const TArray<FTransform>& WorldTransforms)
{
    TArray<int32> Indices;
    if (!HISM) return Indices;

    Indices.Reserve(WorldTransforms.Num());
    for (const FTransform& T : WorldTransforms)
    {
        Indices.Add(HISM->AddInstance(T, true));
    }
    return Indices;
}

bool UScenecapsISMComponent::RemoveInstanceFromHISM(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, bool bSwapRemove)
{
    if (!HISM) return false;
    if (InstanceIndex < 0 || InstanceIndex >= HISM->GetInstanceCount()) return false;

    if (bSwapRemove)
    {
        return HISM->RemoveInstance(InstanceIndex);
    }
    else
    {
        int32 Count = HISM->GetInstanceCount();
        for (int32 i = InstanceIndex; i < Count - 1; ++i)
        {
            FTransform T;
            if (HISM->GetInstanceTransform(i + 1, T, true))
            {
                HISM->UpdateInstanceTransform(i, T, true, false);
            }
        }
        return HISM->RemoveInstance(Count - 1);
    }
}

bool UScenecapsISMComponent::UpdateInstanceTransformHISM(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, const FTransform& NewWorldTransform)
{
    if (!HISM) return false;
    if (InstanceIndex < 0 || InstanceIndex >= HISM->GetInstanceCount()) return false;

    HISM->UpdateInstanceTransform(InstanceIndex, NewWorldTransform, true, true);
    return true;
}

bool UScenecapsISMComponent::GetInstanceTransformHISM(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, FTransform& OutWorldTransform) const
{
    if (!HISM) return false;
    if (InstanceIndex < 0 || InstanceIndex >= HISM->GetInstanceCount()) return false;

    return HISM->GetInstanceTransform(InstanceIndex, OutWorldTransform, true);
}

int32 UScenecapsISMComponent::GetInstanceCountHISM(UHierarchicalInstancedStaticMeshComponent* HISM) const
{
    return HISM ? HISM->GetInstanceCount() : 0;
}

// ---- Per-Instance Custom Data ----

bool UScenecapsISMComponent::SetInstanceCustomDataISM(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, int32 DataIndex, float Value)
{
    if (!ISM) return false;
    if (InstanceIndex < 0 || InstanceIndex >= ISM->GetInstanceCount()) return false;
    if (DataIndex < 0 || DataIndex >= ISM->NumCustomDataFloats) return false;

    ISM->SetCustomDataValue(InstanceIndex, DataIndex, Value, true);
    return true;
}

bool UScenecapsISMComponent::SetInstanceCustomDataHISM(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, int32 DataIndex, float Value)
{
    if (!HISM) return false;
    if (InstanceIndex < 0 || InstanceIndex >= HISM->GetInstanceCount()) return false;
    if (DataIndex < 0 || DataIndex >= HISM->NumCustomDataFloats) return false;

    HISM->SetCustomDataValue(InstanceIndex, DataIndex, Value, true);
    return true;
}

void UScenecapsISMComponent::SetInstanceCustomDataArrayISM(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, const TArray<float>& Values)
{
    if (!ISM) return;
    if (InstanceIndex < 0 || InstanceIndex >= ISM->GetInstanceCount()) return;

    int32 MaxIdx = FMath::Min(Values.Num(), ISM->NumCustomDataFloats);
    for (int32 i = 0; i < MaxIdx; ++i)
    {
        ISM->SetCustomDataValue(InstanceIndex, i, Values[i], true);
    }
}

void UScenecapsISMComponent::SetInstanceCustomDataArrayHISM(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, const TArray<float>& Values)
{
    if (!HISM) return;
    if (InstanceIndex < 0 || InstanceIndex >= HISM->GetInstanceCount()) return;

    int32 MaxIdx = FMath::Min(Values.Num(), HISM->NumCustomDataFloats);
    for (int32 i = 0; i < MaxIdx; ++i)
    {
        HISM->SetCustomDataValue(InstanceIndex, i, Values[i], true);
    }
}

// ---- 高层封装 ----

int32 UScenecapsISMComponent::AddInstanceByMesh(UStaticMesh* Mesh, const FTransform& WorldTransform)
{
    if (UInstancedStaticMeshComponent* ISM = FindISMByMesh(Mesh))
    {
        return AddInstanceToISM(ISM, WorldTransform);
    }
    if (UHierarchicalInstancedStaticMeshComponent* HISM = FindHISMByMesh(Mesh))
    {
        return AddInstanceToHISM(HISM, WorldTransform);
    }
    return -1;
}

bool UScenecapsISMComponent::RemoveInstanceByMesh(UStaticMesh* Mesh, int32 InstanceIndex)
{
    if (UInstancedStaticMeshComponent* ISM = FindISMByMesh(Mesh))
    {
        return RemoveInstanceFromISM(ISM, InstanceIndex);
    }
    if (UHierarchicalInstancedStaticMeshComponent* HISM = FindHISMByMesh(Mesh))
    {
        return RemoveInstanceFromHISM(HISM, InstanceIndex);
    }
    return false;
}





