// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsProceduralBiome.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ScenecapsProceduralTypes.h"
#include "ScenecapsProceduralBiome.generated.h"

class UStaticMesh;
class UMaterialInterface;
class UTexture2D;
class ALandscape;
class UWorld;

UENUM(BlueprintType)
enum class EScenecapsBiomeBlendMode : uint8
{
    Replace,      // 完全替换
    Additive,     // 叠加
    Weighted,     // 权重混合
};

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsBiomeEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Biome Entry")
    TSoftObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, Category = "Biome Entry")
    TSoftObjectPtr<UMaterialInterface> MaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Biome Entry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Weight = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Biome Entry")
    FScenecapsProceduralParams ProceduralParams;

    UPROPERTY(EditAnywhere, Category = "Biome Entry", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinSlope = 0.0f;        // 适用最小坡度 (度)

    UPROPERTY(EditAnywhere, Category = "Biome Entry", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float MaxSlope = 90.0f;       // 适用最大坡度 (度)

    UPROPERTY(EditAnywhere, Category = "Biome Entry")
    float MinHeight = -10000.0f;  // 适用最小高度

    UPROPERTY(EditAnywhere, Category = "Biome Entry")
    float MaxHeight = 10000.0f;   // 适用最大高度

    UPROPERTY(EditAnywhere, Category = "Biome Entry")
    TArray<TSoftObjectPtr<UTexture2D>> MaskTextures; // 遮罩贴图

    UPROPERTY(EditAnywhere, Category = "Biome Entry")
    EScenecapsBiomeBlendMode BlendMode = EScenecapsBiomeBlendMode::Weighted;
};

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsBiomeLayer
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Biome Layer")
    FName LayerName;

    UPROPERTY(EditAnywhere, Category = "Biome Layer", meta = (TitleProperty = "Mesh"))
    TArray<FScenecapsBiomeEntry> Entries;

    UPROPERTY(EditAnywhere, Category = "Biome Layer", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GlobalWeight = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Biome Layer")
    bool bEnabled = true;
};

UCLASS(BlueprintType, meta = (DisplayName = "Scenecaps Biome Asset"))
class SCENECAPS_API UScenecapsBiomeAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Biome")
    TArray<FScenecapsBiomeLayer> Layers;

    UPROPERTY(EditAnywhere, Category = "Biome")
    FScenecapsProceduralParams GlobalProceduralParams;

    // 根据位置查询适用的条目权重
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Biome")
    TArray<FScenecapsBiomeEntry> QueryEntriesAtLocation(
        const FVector& WorldLocation,
        ALandscape* Landscape = nullptr,
        UWorld* World = nullptr) const;
};

// 运行时 Biome 采样器组件
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "Scenecaps Biome Sampler"))
class SCENECAPS_API UScenecapsBiomeSamplerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UScenecapsBiomeSamplerComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
    TSoftObjectPtr<UScenecapsBiomeAsset> BiomeAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
    TSoftObjectPtr<ALandscape> TargetLandscape;

    // 在编辑器中预览生成
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Biome")
    TArray<FTransform> PreviewGenerate(
        const FTransform& WorldTransform,
        int32 MaxInstances = 1000);

// 运行时生成到 ISM/HISM (Not implemented yet)
    UInstancedStaticMeshComponent* GenerateToISM(
        UWorld* World,
        const FTransform& WorldTransform,
        AActor* ContainerActor = nullptr);

    UHierarchicalInstancedStaticMeshComponent* GenerateToHISM(
        UWorld* World,
        const FTransform& WorldTransform,
        AActor* ContainerActor = nullptr);
};

