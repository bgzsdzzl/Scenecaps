
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScenecapsStaticActor.generated.h"

class UScenecapsStaticAsset;
class USceneComponent;
class UDataLayerAsset;

UCLASS(BlueprintType, meta = (DisplayName = "Scene Collection (Static)"))
class SCENECAPS_API AScenecapsStaticActor : public AActor
{
    GENERATED_BODY()
public:
    AScenecapsStaticActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Collection")
    TSoftObjectPtr<UScenecapsStaticAsset> CollectionAsset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Collection", meta = (AdvancedDisplay = "1"))
    bool bIncludeHidden = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Collection|Variants")
    FName ActiveVariant = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Collection|World Partition")
    TSoftObjectPtr<UDataLayerAsset> TargetDataLayer;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scene Collection|World Partition",
        meta = (EditCondition = "TargetDataLayer != nullptr"))
    bool bSpawnOnDataLayerLoaded = false;

    UFUNCTION(BlueprintCallable, Category = "Scene Collection")
    void Rebuild();

    UFUNCTION(BlueprintCallable, Category = "Scene Collection")
    void Clear();

    UFUNCTION(BlueprintCallable, Category = "Scene Collection|Variants")
    void SetActiveVariant(FName InVariant);

    const TArray<TObjectPtr<AActor>>& GetSpawnedActors() const { return SpawnedActors; }
    int32 GetSpawnedCount() const { return SpawnedActors.Num(); }

    UFUNCTION(BlueprintPure, Category = "Scene Collection|World Partition")
    FText GetDataLayerStatusText() const;

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