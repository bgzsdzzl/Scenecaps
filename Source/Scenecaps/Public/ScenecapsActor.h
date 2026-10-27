// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScenecapsActor.generated.h"

class UScenecapsAsset;
class USceneComponent;
class UDataLayerAsset;

UCLASS(BlueprintType, meta = (DisplayName = "Scene Collection"))
class SCENECAPS_API AScenecapsActor : public AActor
{
    GENERATED_BODY()

public:
    AScenecapsActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Collection")
    TSoftObjectPtr<UScenecapsAsset> CollectionAsset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Collection", meta = (AdvancedDisplay = "1"))
    bool bIncludeHidden = false;

    /**
     * 当前激活的变体。NAME_None = 使用基础 Entries。
     * 变体不存在时静默降级为 Base。
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Collection|Variants")
    FName ActiveVariant = NAME_None;

    // ---------- World Partition ----------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Collection|World Partition")
    TSoftObjectPtr<UDataLayerAsset> TargetDataLayer;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Collection|World Partition",
        meta = (EditCondition = "TargetDataLayer != nullptr"))
    bool bSpawnOnDataLayerLoaded = false;

    // ---------- API ----------

    UFUNCTION(BlueprintCallable, Category = "Scene Collection")
    void Rebuild();

    UFUNCTION(BlueprintCallable, Category = "Scene Collection")
    void Clear();

    /** 切换变体并触发 Rebuild。 */
    UFUNCTION(BlueprintCallable, Category = "Scene Collection|Variants")
    void SetActiveVariant(FName InVariant);

    const TArray<TObjectPtr<AActor>>& GetSpawnedActors() const { return SpawnedActors; }
    int32 GetSpawnedCount() const { return SpawnedActors.Num(); }

    UFUNCTION(BlueprintPure, Category = "Scene Collection|World Partition")
    FText GetDataLayerStatusText() const;

    // ---------- AActor ----------

    virtual void Destroyed() override;
    virtual void PostDuplicate(bool bDuplicateForPIE) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    UPROPERTY()
    TArray<TObjectPtr<AActor>> SpawnedActors;

    UPROPERTY(VisibleAnywhere, Category = "Scene Collection")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(Transient)
    TObjectPtr<UDataLayerAsset> CachedDataLayerAsset;

    void EnsureDataLayerCached();
    bool ShouldDeferRebuild() const;
    bool IsTargetDataLayerActive() const;
    void ApplyDataLayerToActor(AActor* Actor, UDataLayerAsset* LayerAsset) const;
    void RemoveDataLayerFromActor(AActor* Actor, UDataLayerAsset* LayerAsset) const;
};