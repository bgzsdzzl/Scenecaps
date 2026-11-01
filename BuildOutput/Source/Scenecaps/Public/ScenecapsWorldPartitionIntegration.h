// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsWorldPartitionIntegration.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ScenecapsWorldPartitionIntegration.generated.h"

class UWorld;
class UDataLayerAsset;
class UDataLayerInstance;
class AActor;
class UDataLayerManager;

UENUM(BlueprintType)
enum class EScenecapsDataLayerAssignmentMode : uint8
{
    Manual,           // 手动指定
    SpatialGrid,      // 空间网格自动分配
    DistanceBased,    // 基于距离（摄像机/玩家）
    BiomeBased,       // 基于 Biome
    Custom,           // 自定义回调
};

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsSpatialDataLayerRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Data Layer Rule")
    FName RuleName;

    UPROPERTY(EditAnywhere, Category = "Data Layer Rule")
    TSoftObjectPtr<UDataLayerAsset> DataLayer;

    UPROPERTY(EditAnywhere, Category = "Data Layer Rule")
    FBox Bounds;

    UPROPERTY(EditAnywhere, Category = "Data Layer Rule", meta = (ClampMin = "0", UIMin = "0"))
    int32 GridSize = 1000; // 网格单元大小

    UPROPERTY(EditAnywhere, Category = "Data Layer Rule")
    EScenecapsDataLayerAssignmentMode Mode = EScenecapsDataLayerAssignmentMode::SpatialGrid;

    UPROPERTY(EditAnywhere, Category = "Data Layer Rule")
    float MaxDistance = 5000.0f; // DistanceBased 模式的最大距离

    UPROPERTY(EditAnywhere, Category = "Data Layer Rule")
    TArray<FName> RequiredTags; // Actor 需要包含的标签

    UPROPERTY(EditAnywhere, Category = "Data Layer Rule")
    TArray<FName> ExcludedTags;
};

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsWorldPartitionSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "World Partition")
    bool bAutoAssignDataLayers = true;

    UPROPERTY(EditAnywhere, Category = "World Partition")
    bool bEnableStreaming = true;

    UPROPERTY(EditAnywhere, Category = "World Partition")
    bool bUseHLOD = true;

    UPROPERTY(EditAnywhere, Category = "World Partition")
    TArray<FScenecapsSpatialDataLayerRule> SpatialRules;

    UPROPERTY(EditAnywhere, Category = "World Partition")
    float StreamingDistance = 10000.0f;

    UPROPERTY(EditAnywhere, Category = "World Partition")
    int32 HLODLevel = 1;
};

UCLASS(BlueprintType, Blueprintable)
class SCENECAPS_API UScenecapsWorldPartitionIntegration : public UObject
{
    GENERATED_BODY()

public:
    UScenecapsWorldPartitionIntegration();

    // ---- 数据层自动分配 ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|World Partition")
    static void AutoAssignDataLayers(
        UWorld* World,
        const TArray<AActor*>& Actors,
        const FScenecapsWorldPartitionSettings& Settings);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|World Partition")
    static void AssignDataLayerToActor(
        AActor* Actor,
        UDataLayerAsset* DataLayer,
        bool bActivate = true);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|World Partition")
    static void RemoveDataLayerFromActor(
        AActor* Actor,
        UDataLayerAsset* DataLayer);

    // ---- 运行时流式控制 ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|World Partition")
    static void SetDataLayerRuntimeState(
        UWorld* World,
        UDataLayerAsset* DataLayer,
        bool bActivate);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|World Partition")
    static EDataLayerRuntimeState GetDataLayerRuntimeState(
        UWorld* World,
        UDataLayerAsset* DataLayer);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|World Partition")
    static void SetDataLayersRuntimeState(
        UWorld* World,
        const TArray<UDataLayerAsset*>& DataLayers,
        bool bActivate);

    // ---- HLOD 集成 ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|World Partition")
    static void RegisterISMToHLOD(
        class UInstancedStaticMeshComponent* ISM,
        int32 HLODLevel = 1);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|World Partition")
    static void RegisterHISMToHLOD(
        class UHierarchicalInstancedStaticMeshComponent* HISM,
        int32 HLODLevel = 1);


};

