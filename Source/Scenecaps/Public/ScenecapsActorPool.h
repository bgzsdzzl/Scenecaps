// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsActorPool.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScenecapsActorPool.generated.h"

class AActor;
class UClass;
class UWorld;

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsPooledActor
{
    GENERATED_BODY()

    UPROPERTY()
    AActor* Actor = nullptr;

    UPROPERTY()
    bool bInUse = false;

    UPROPERTY()
    FName PoolKey;

    UPROPERTY()
    double LastUsedTime = 0.0;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent, DisplayName = "Scenecaps Actor Pool"))
class SCENECAPS_API UScenecapsActorPoolComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UScenecapsActorPoolComponent();

    // ---- 池配置 ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
    TMap<FName, TSubclassOf<AActor>> PoolClasses;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "0"))
    int32 MaxPoolSizePerClass = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "0"))
    float MaxIdleTimeSeconds = 300.0f; // 5分钟自动清理

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
    bool bAutoExpand = true;

    // ---- 运行时状态 ----

    UPROPERTY(Transient)
    TArray<FScenecapsPooledActor> PooledActors;

    UPROPERTY(Transient)
    TMap<FName, int32> ActiveCountPerClass;

    // ---- 核心接口 ----

    /** 从池中获取 Actor，如果池为空则自动创建（需配置 PoolClasses） */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ActorPool")
    AActor* AcquireActor(FName PoolKey, const FTransform& SpawnTransform, AActor* Owner = nullptr);

    /** 归还 Actor 到池 */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ActorPool")
    void ReleaseActor(AActor* Actor, FName PoolKey);

    /** 预热池：预先创建指定数量的 Actor */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ActorPool")
    void PrewarmPool(FName PoolKey, int32 Count);

    /** 清理空闲超时的 Actor */
    UFUNCTION(BlueprintCallable, Category = "Scenecaps|ActorPool")
    void CleanupIdleActors();

    /** 获取池统计信息 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|ActorPool")
    int32 GetPoolSize(FName PoolKey) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|ActorPool")
    int32 GetActiveCount(FName PoolKey) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|ActorPool")
    int32 GetTotalPoolSize() const;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    AActor* CreatePooledActor(FName PoolKey, const FTransform& SpawnTransform, AActor* Owner);
    void ResetActor(AActor* Actor);
};
