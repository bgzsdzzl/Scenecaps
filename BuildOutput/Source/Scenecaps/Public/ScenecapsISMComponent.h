// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsISMComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScenecapsISMComponent.generated.h"

class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsInstanceData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Scenecaps|ISM")
    int32 InstanceIndex = -1;

    UPROPERTY(BlueprintReadWrite, Category = "Scenecaps|ISM")
    FTransform Transform = FTransform::Identity;

    UPROPERTY(BlueprintReadWrite, Category = "Scenecaps|ISM")
    TMap<FName, float> CustomData;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "Scenecaps ISM Manager"))
class SCENECAPS_API UScenecapsISMComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UScenecapsISMComponent();

    // ---- ISM 操作 ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    int32 AddInstanceToISM(UInstancedStaticMeshComponent* ISM, const FTransform& WorldTransform);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    TArray<int32> AddInstancesToISM(UInstancedStaticMeshComponent* ISM, const TArray<FTransform>& WorldTransforms);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    bool RemoveInstanceFromISM(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, bool bSwapRemove = true);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    bool UpdateInstanceTransformISM(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, const FTransform& NewWorldTransform);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    bool GetInstanceTransformISM(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, FTransform& OutWorldTransform) const;

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    int32 GetInstanceCountISM(UInstancedStaticMeshComponent* ISM) const;

    // ---- HISM 操作 ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    int32 AddInstanceToHISM(UHierarchicalInstancedStaticMeshComponent* HISM, const FTransform& WorldTransform);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    TArray<int32> AddInstancesToHISM(UHierarchicalInstancedStaticMeshComponent* HISM, const TArray<FTransform>& WorldTransforms);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    bool RemoveInstanceFromHISM(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, bool bSwapRemove = true);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    bool UpdateInstanceTransformHISM(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, const FTransform& NewWorldTransform);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    bool GetInstanceTransformHISM(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, FTransform& OutWorldTransform) const;

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    int32 GetInstanceCountHISM(UHierarchicalInstancedStaticMeshComponent* HISM) const;

    // ---- Per-Instance Custom Data (材质参数) ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    bool SetInstanceCustomDataISM(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, int32 DataIndex, float Value);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    bool SetInstanceCustomDataHISM(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, int32 DataIndex, float Value);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    void SetInstanceCustomDataArrayISM(UInstancedStaticMeshComponent* ISM, int32 InstanceIndex, const TArray<float>& Values);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    void SetInstanceCustomDataArrayHISM(UHierarchicalInstancedStaticMeshComponent* HISM, int32 InstanceIndex, const TArray<float>& Values);

    // ---- 容器管理 ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    void RegisterContainerActor(AActor* ContainerActor);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    TArray<UInstancedStaticMeshComponent*> GetRegisteredISMs() const;

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    TArray<UHierarchicalInstancedStaticMeshComponent*> GetRegisteredHISMs() const;

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    UInstancedStaticMeshComponent* FindISMByMesh(UStaticMesh* Mesh) const;

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    UHierarchicalInstancedStaticMeshComponent* FindHISMByMesh(UStaticMesh* Mesh) const;

    // ---- 高层封装：按 Mesh + 变换增删 ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    int32 AddInstanceByMesh(UStaticMesh* Mesh, const FTransform& WorldTransform);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    bool RemoveInstanceByMesh(UStaticMesh* Mesh, int32 InstanceIndex);

protected:
    UPROPERTY(Transient)
    TArray<UInstancedStaticMeshComponent*> RegisteredISMs;

    UPROPERTY(Transient)
    TArray<UHierarchicalInstancedStaticMeshComponent*> RegisteredHISMs;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
