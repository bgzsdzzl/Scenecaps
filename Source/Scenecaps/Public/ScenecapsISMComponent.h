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

/** 条目到 ISM 实例的绑定，供变体过渡精确定位并还原父级世界变换 */
USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsISMEntryBinding
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|ISM")
    FGuid EntryId;

    UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|ISM")
    TObjectPtr<UInstancedStaticMeshComponent> ISM;

    UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|ISM")
    int32 InstanceIndex = INDEX_NONE;

    /** 实例生成时的世界变换与条目相对变换，用于过渡时反推父级世界 */
    UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|ISM")
    FTransform SpawnTransform = FTransform::Identity;

    UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|ISM")
    FTransform BaseRelativeTransform = FTransform::Identity;

    /** 生成时实例上的 CustomPrimitiveData，过渡时作为基线恢复 */
    UPROPERTY()
    TArray<float> BaseCustomData;
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

    /** 注册条目 -> 实例绑定（由批量生成流程调用） */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    void RegisterEntryInstance(FGuid EntryId, UInstancedStaticMeshComponent* ISM, int32 InstanceIndex,
        const FTransform& SpawnTransform, const FTransform& BaseRelativeTransform, const TArray<float>& BaseCustomData);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Runtime")
    int32 GetEntryBindingCount() const;

    const TArray<FScenecapsISMEntryBinding>& GetEntryBindings() const { return EntryBindings; }

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

    UPROPERTY(Transient)
    TArray<FScenecapsISMEntryBinding> EntryBindings;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
