// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsActor.h

#pragma once

#include "CoreMinimal.h"
#include "WorldPartition/DataLayer/DataLayerAsset.h"
#include "GameFramework/Actor.h"
#include "ScenecapsActor.generated.h"

class UScenecapsAsset;
class USceneComponent;
class UDataLayerAsset;
class UScenecapsModifierManager;

/** 已生成 Actor 与其来源条目的对应关系（用于精确应用变体插值 / Data Layer 清理） */
USTRUCT()
struct FScenecapsSpawnedEntry
{
    GENERATED_BODY()

    UPROPERTY()
    FGuid EntryId;

    UPROPERTY()
    TObjectPtr<AActor> Actor;

    /** 生成时的世界变换与条目相对变换，用于过渡时还原父级世界变换 */
    UPROPERTY()
    FTransform SpawnTransform;

    UPROPERTY()
    FTransform BaseRelativeTransform;

#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TObjectPtr<UDataLayerAsset> AppliedDataLayer;
#endif
};

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
#if WITH_EDITORONLY_DATA
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Collection|World Partition")
    TSoftObjectPtr<UDataLayerAsset> TargetDataLayer;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Collection|World Partition",
        meta = (EditCondition = "TargetDataLayer != nullptr"))
    bool bSpawnOnDataLayerLoaded = false;
#endif

    // ---------- Modifiers ----------

    UPROPERTY(EditAnywhere, Instanced, Category = "Scene Collection|Modifiers")
    TObjectPtr<UScenecapsModifierManager> ModifierManager;

    // ---------- API ----------

    UFUNCTION(BlueprintCallable, Category = "Scene Collection")
    void Rebuild();

    UFUNCTION(BlueprintCallable, Category = "Scene Collection")
    void Clear();

    const TArray<TObjectPtr<AActor>>& GetSpawnedActors() const { return SpawnedActors; }
    int32 GetSpawnedCount() const { return SpawnedActors.Num(); }

    /** 条目 ID -> 生成 Actor 的映射，供变体过渡精确应用 */
    const TArray<FScenecapsSpawnedEntry>& GetSpawnedEntries() const { return SpawnedEntries; }

    /** 由 SpawnCollectionAsISM/HISM 写入，供变体过渡定位批量实例容器 */
    void SetISMContainerActor(AActor* InActor);
    AActor* GetISMContainerActor() const;

    // ---------- Runtime Blueprint API ----------

    /** 获取当前引用的 Collection Asset */
    UFUNCTION(BlueprintPure, Category = "Scene Collection")
    UScenecapsAsset* GetCollectionAsset() const;

    /** 获取当前激活的变体名 */
    UFUNCTION(BlueprintPure, Category = "Scene Collection|Variants")
    FName GetActiveVariant() const;

    /** 获取所有可用变体名 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene Collection|Variants")
    TArray<FName> GetVariantNames() const;

    /** 运行时动态设置 Collection Asset（可选立即 Rebuild） */
    UFUNCTION(BlueprintCallable, Category = "Scene Collection")
    void SetCollectionAsset(UScenecapsAsset* InAsset, bool bRebuildImmediately = true);

    /** 运行时设置激活变体（可选立即 Rebuild） */
    UFUNCTION(BlueprintCallable, Category = "Scene Collection|Variants")
    void SetActiveVariant(FName InVariant, bool bRebuildImmediately = true);

/** 获取当前 Data Layer 资产 */
#if WITH_EDITOR
    UFUNCTION(BlueprintPure, Category = "Scene Collection|World Partition")
    UDataLayerAsset* GetTargetDataLayer() const;

    /** 运行时设置目标 Data Layer（可选立即应用） */
    UFUNCTION(BlueprintCallable, Category = "Scene Collection|World Partition")
    void SetTargetDataLayer(UDataLayerAsset* InDataLayer, bool bApplyImmediately = true);

    /** 运行时切换 Data Layer 激活状态 */
    UFUNCTION(BlueprintCallable, Category = "Scene Collection|World Partition")
    void SetDataLayerActive(bool bActive);

    /** 获取 Data Layer 状态文本 */
    UFUNCTION(BlueprintPure, Category = "Scene Collection|World Partition")
    FText GetDataLayerStatusText() const;
#endif

    /** 获取所有已生成 Actor 的引用（用于运行时进一步操作） */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene Collection")
    TArray<AActor*> GetSpawnedActorArray() const;

    /** 运行时按条件查找/过滤已生成 Actor */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene Collection")
    TArray<AActor*> FindSpawnedActorsByClass(TSubclassOf<AActor> ActorClass) const;

    // ---------- Modifiers API ----------

    UFUNCTION(BlueprintCallable, Category = "Scene Collection|Modifiers")
    void ApplyModifiers(EScenecapsModifierPhase Phase, FName VariantName = NAME_None, FGuid EntryId = FGuid());

    UFUNCTION(BlueprintCallable, Category = "Scene Collection|Modifiers")
    void ApplyModifiersToActor(AActor* Actor, FName VariantName = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Scene Collection|Modifiers")
    UScenecapsModifierManager* GetOrCreateModifierManager();

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

    UPROPERTY(Transient)
    TArray<FScenecapsSpawnedEntry> SpawnedEntries;

    /** 批量实例化容器，由 SpawnCollectionAsISM/HISM 关联 */
    UPROPERTY(Transient)
    TObjectPtr<AActor> ISMContainerActor;

    UPROPERTY(VisibleAnywhere, Category = "Scene Collection")
    TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
    UPROPERTY(Transient)
    TObjectPtr<UDataLayerAsset> CachedDataLayerAsset;

    void EnsureDataLayerCached();
    bool ShouldDeferRebuild() const;
    bool IsTargetDataLayerActive() const;
    void ApplyDataLayerToActor(AActor* Actor, UDataLayerAsset* LayerAsset) const;
    void RemoveDataLayerFromActor(AActor* Actor, UDataLayerAsset* LayerAsset) const;
#endif
};
