// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsTypes.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ScenecapsTypes.generated.h"

class UDataLayerAsset;
class UMaterialInterface;

/**
 * A single entry inside a scene collection.
 */
USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsEntry
{
	GENERATED_BODY()

	FScenecapsEntry()
		: EntryId(FGuid::NewGuid())
		, ParentId(FGuid())
	{}
	UPROPERTY(VisibleAnywhere, Category = "Scenecaps", meta = (IgnoreForMemberInitializationTest))
	FGuid EntryId;

	UPROPERTY(EditAnywhere, Category = "Scenecaps")
	FGuid ParentId;

	UPROPERTY(EditAnywhere, Category = "Scenecaps")
	FString DisplayName;

	UPROPERTY(EditAnywhere, Category = "Scenecaps")
	TSoftObjectPtr<UObject> Asset;

	UPROPERTY(EditAnywhere, Category = "Scenecaps")
	FTransform RelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, Category = "Scenecaps")
	bool bVisible = true;

	UPROPERTY(EditAnywhere, Category = "Scenecaps")
	bool bLocked = false;

	UPROPERTY(EditAnywhere, Category = "Scenecaps")
	FVector DecalSize = FVector(128.f, 128.f, 256.f);

	UPROPERTY(EditAnywhere, Category = "Scenecaps")
	bool bOverrideMobility = false;

	UPROPERTY(EditAnywhere, Category = "Scenecaps", meta = (EditCondition = "bOverrideMobility"))
	TEnumAsByte<EComponentMobility::Type> Mobility = EComponentMobility::Movable;

	UPROPERTY(EditAnywhere, Category = "Scenecaps")
	bool bOverrideCollision = false;

	UPROPERTY(EditAnywhere, Category = "Scenecaps", meta = (EditCondition = "bOverrideCollision"))
	TEnumAsByte<ECollisionEnabled::Type> CollisionEnabled = ECollisionEnabled::QueryAndPhysics;

	UPROPERTY(EditAnywhere, Category = "Scenecaps")
	bool bOverrideCastShadow = false;

	UPROPERTY(EditAnywhere, Category = "Scenecaps", meta = (EditCondition = "bOverrideCastShadow"))
	bool bCastShadow = true;

	UPROPERTY(EditAnywhere, Category = "Scenecaps|Material Override")
	TSoftObjectPtr<UMaterialInterface> MaterialOverride;

	UPROPERTY(EditAnywhere, Category = "Scenecaps|Material Override",
		meta = (EditCondition = "!MaterialOverride.IsNull()", ClampMin = "-1"))
	int32 MaterialSlotIndex = 0;

	UPROPERTY(EditAnywhere, Category = "Scenecaps|World Partition")
	TSoftObjectPtr<UDataLayerAsset> DataLayer;
};

// ================================================================
// Variant system
// ================================================================

/**
 * 单个条目在某个变体上的稀疏覆盖。
 * 每个 bOverrideXxx 开关独立控制是否应用对应字段。
 */
USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsEntryOverride
{
	GENERATED_BODY()

	/** 目标条目（必须对应 Entries 里现有的 EntryId）。 */
	UPROPERTY(VisibleAnywhere, Category = "Scenecaps")
	FGuid EntryId;

	/** 冗余的显示名，仅供 UI 显示（Add to Variant 时自动填充）。 */
	UPROPERTY(VisibleAnywhere, Category = "Scenecaps")
	FString DisplayName;

	// ---- Visible ----
	UPROPERTY(EditAnywhere, Category = "Visibility")
	bool bOverrideVisible = false;

	UPROPERTY(EditAnywhere, Category = "Visibility", meta = (EditCondition = "bOverrideVisible"))
	bool bVisible = true;

	// ---- Transform ----
	UPROPERTY(EditAnywhere, Category = "Transform")
	bool bOverrideTransform = false;

	UPROPERTY(EditAnywhere, Category = "Transform", meta = (EditCondition = "bOverrideTransform"))
	FTransform RelativeTransform = FTransform::Identity;

	// ---- Material ----
	UPROPERTY(EditAnywhere, Category = "Material")
	bool bOverrideMaterial = false;

	UPROPERTY(EditAnywhere, Category = "Material", meta = (EditCondition = "bOverrideMaterial"))
	TSoftObjectPtr<UMaterialInterface> MaterialOverride;

	UPROPERTY(EditAnywhere, Category = "Material", meta = (EditCondition = "bOverrideMaterial"))
	int32 MaterialSlotIndex = 0;

	/** 至少有一个字段被覆盖时为 true（UI 用来显示“有效覆盖”）。 */
	bool HasAnyOverride() const
	{
		return bOverrideVisible || bOverrideTransform || bOverrideMaterial;
	}
};

/**
 * 一个场景变体（如 Day / Night）。
 * Overrides 稀疏表，为空则整个变体等价于 Base。
 */
USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsVariant
{
	GENERATED_BODY()

	/** 变体名（NAME_None 保留给 Base，不能使用）。 */
	UPROPERTY(EditAnywhere, Category = "Scenecaps")
	FName Name;

	UPROPERTY(EditAnywhere, Category = "Scenecaps", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, Category = "Scenecaps", meta = (TitleProperty = "DisplayName"))
	TArray<FScenecapsEntryOverride> Overrides;
};