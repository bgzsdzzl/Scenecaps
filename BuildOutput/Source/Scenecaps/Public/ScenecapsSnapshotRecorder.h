// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsSnapshotRecorder.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ScenecapsTypes.h"
#include "ScenecapsSnapshotRecorder.generated.h"

class AActor;
class UWorld;
class UScenecapsAsset;

UENUM(BlueprintType)
enum class EScenecapsSnapshotMode : uint8
{
    SceneAsset,       // 生成 UScenecapsAsset (引用资产)
};

UENUM(BlueprintType)
enum class EScenecapsSnapshotFilter : uint8
{
    All,              // 所有 Actor
    SelectedOnly,     // 仅选中的 Actor
    ClassFilter,      // 按类过滤
    TagFilter,        // 按标签过滤
    DataLayerFilter,  // 按数据层过滤
};

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsSnapshotSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    EScenecapsSnapshotMode Mode = EScenecapsSnapshotMode::SceneAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    EScenecapsSnapshotFilter Filter = EScenecapsSnapshotFilter::All;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    TArray<TSubclassOf<AActor>> AllowedClasses;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    TArray<FName> RequiredTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    TArray<FName> ExcludedTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    TSoftObjectPtr<class UDataLayerAsset> DataLayerFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    bool bIncludeHiddenActors = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    bool bIncludeNonSpatiallyLoaded = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    bool bPreserveHierarchy = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    bool bAutoGenerateVariant = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    FName NewVariantName = TEXT("Snapshot");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snapshot")
    FText NewVariantDescription;
};

UCLASS(BlueprintType, Blueprintable)
class SCENECAPS_API UScenecapsSnapshotRecorder : public UObject
{
    GENERATED_BODY()

public:
    UScenecapsSnapshotRecorder();

    // ---- 核心快照功能 ----

    /** 对当前 World 进行快照，生成 Asset */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Snapshot")
    UScenecapsAsset* SnapshotWorldAsSceneAsset(UWorld* World, const FScenecapsSnapshotSettings& Settings);

    /** 对指定 Actor 列表进行快照 */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Snapshot")
    UScenecapsAsset* SnapshotActorsAsSceneAsset(const TArray<AActor*>& Actors, const FScenecapsSnapshotSettings& Settings, UObject* Outer = nullptr);

    // ---- 编辑器专用：保存到 Content Browser ----

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Snapshot")
    UScenecapsAsset* SnapshotAndSaveAsSceneAsset(UWorld* World, const FScenecapsSnapshotSettings& Settings, const FString& PackagePath, const FString& AssetName);
#endif

    // ---- 运行时动态增删 ----

    /** 将快照结果直接 Spawn 到当前 World */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Snapshot", meta = (DisplayName = "Spawn Snapshot")) TArray<AActor*> SpawnSceneSnapshot(UWorld* World, UScenecapsAsset* Asset, const FTransform& WorldTransform, FName VariantName = NAME_None);

protected:
    // 内部实现
    TArray<AActor*> CollectActors(UWorld* World, const FScenecapsSnapshotSettings& Settings) const;
    void BuildEntryFromActor(AActor* Actor, FScenecapsEntry& OutEntry, const FTransform& RootTransform) const;
    FGuid FindOrCreateParentId(const TArray<AActor*>& Actors, AActor* Child, const TMap<AActor*, FGuid>& ActorToId) const;
};
