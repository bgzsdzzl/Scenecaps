// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsEntryHandler.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

class AActor;
class UObject;
class UWorld;
class UMaterialInterface;

struct SCENECAPS_API FScenecapsSpawnContext
{
    UWorld* World = nullptr;
    FTransform Transform = FTransform::Identity;
    bool bIsPreview = false;

    /** 贴花尺寸，仅贴花材质使用。 */
    FVector DecalSize = FVector(128.f, 128.f, 256.f);

    /** 条目级覆盖：Mobility。 */
    bool bOverrideMobility = false;
    TEnumAsByte<EComponentMobility::Type> Mobility = EComponentMobility::Movable;

    /** 条目级覆盖：Collision。 */
    bool bOverrideCollision = false;
    TEnumAsByte<ECollisionEnabled::Type> CollisionEnabled = ECollisionEnabled::QueryAndPhysics;

    /** 条目级覆盖：CastShadow。 */
    bool bOverrideCastShadow = false;
    bool bCastShadow = true;

    /** 是否把不可见的条目也生成出来。 */
    bool bIncludeHidden = false;

    /** 条目级材质覆盖。为 nullptr 时不覆盖。 */
    UMaterialInterface* MaterialOverride = nullptr;

    /** 材质覆盖槽位。-1 = 全部槽位。 */
    int32 MaterialSlotIndex = 0;

    // ---- 新增：自定义参数（传递给 Handler/Blueprint）----
    TMap<FName, float> FloatParams;
    TMap<FName, FVector> VectorParams;
    TMap<FName, FLinearColor> ColorParams;
    TMap<FName, bool> BoolParams;
};

class SCENECAPS_API IScenecapsEntryHandler
{
public:
    virtual ~IScenecapsEntryHandler() = default;

    virtual bool CanHandleAsset(const UObject* Asset) const = 0;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const = 0;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const { return nullptr; }
    virtual UObject* ResolveAssetFromActor(AActor* Actor) const { return nullptr; }
};

/** 把 Context 里的覆盖字段应用到 Actor 的所有 PrimitiveComponent 上。 */
SCENECAPS_API void ApplyEntryOverrides(AActor* Actor, const FScenecapsSpawnContext& Context);

class SCENECAPS_API FScenecapsMaterialEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
};
