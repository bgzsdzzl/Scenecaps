// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsISMConversion.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"
#include "ScenecapsISMConversion.generated.h"

class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class AActor;
class UStaticMesh;
class UWorld;
class UScenecapsActorPoolComponent;

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsISMToActorSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
    bool bUseActorPool = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
    FName PoolKey = TEXT("ISMConverted");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
    bool bPreserveTransform = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
    bool bPreserveMaterial = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
    bool bPreserveCustomData = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
    TSubclassOf<AActor> OverrideActorClass;
};

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsActorToISMSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
    bool bMergeSameMesh = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
    bool bBuildHISM = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
    int32 NumLODs = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
    float LODScreenSize = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
    bool bDestroySourceActors = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion")
    bool bPreserveCustomData = true;
};

UCLASS(BlueprintType, Blueprintable)
class SCENECAPS_API UScenecapsISMConversion : public UObject
{
    GENERATED_BODY()

public:
    UScenecapsISMConversion();

    // ---- ISM -> Actor (拆解实例为独立 Actor) ----

    /** 将 ISM 的实例拆解为独立 Actor */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Conversion")
    static TArray<AActor*> ConvertISMToActors(
        UInstancedStaticMeshComponent* ISM,
        const FScenecapsISMToActorSettings& Settings,
        UWorld* World,
        UScenecapsActorPoolComponent* Pool = nullptr);

    /** 将 HISM 的实例拆解为独立 Actor */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Conversion")
    static TArray<AActor*> ConvertHISMToActors(
        UHierarchicalInstancedStaticMeshComponent* HISM,
        const FScenecapsISMToActorSettings& Settings,
        UWorld* World,
        UScenecapsActorPoolComponent* Pool = nullptr);

    // ---- Actor -> ISM (合并 Actor 为实例) ----

    /** 将一组 Actor 合并为 ISM */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Conversion")
    static UInstancedStaticMeshComponent* ConvertActorsToISM(
        const TArray<AActor*>& Actors,
        const FScenecapsActorToISMSettings& Settings,
        UWorld* World,
        AActor* ContainerActor = nullptr);

    /** 将一组 Actor 合并为 HISM（大场景推荐） */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Conversion")
    static UHierarchicalInstancedStaticMeshComponent* ConvertActorsToHISM(
        const TArray<AActor*>& Actors,
        const FScenecapsActorToISMSettings& Settings,
        UWorld* World,
        AActor* ContainerActor = nullptr);

    // ---- 实用工具 ----

    /** 获取 ISM 实例数量 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|ISM Conversion")
    static int32 GetISMInstanceCount(UInstancedStaticMeshComponent* ISM);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|ISM Conversion")
    static int32 GetHISMInstanceCount(UHierarchicalInstancedStaticMeshComponent* HISM);

    /** 获取指定索引的实例变换 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|ISM Conversion")
    static bool GetISMInstanceTransform(UInstancedStaticMeshComponent* ISM, int32 Index, FTransform& OutTransform);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|ISM Conversion")
    static bool GetHISMInstanceTransform(UHierarchicalInstancedStaticMeshComponent* HISM, int32 Index, FTransform& OutTransform);

    /** 设置实例变换 */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Conversion")
    static bool SetISMInstanceTransform(UInstancedStaticMeshComponent* ISM, int32 Index, const FTransform& Transform);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM Conversion")
    static bool SetHISMInstanceTransform(UHierarchicalInstancedStaticMeshComponent* HISM, int32 Index, const FTransform& Transform);
};
