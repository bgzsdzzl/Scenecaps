// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsTypes.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ScenecapsProceduralTypes.h"
#include "ScenecapsTypes.generated.h"

class UDataLayerAsset;
class UMaterialInterface;
class UTexture;

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
		meta = (EditCondition = "MaterialOverride != nullptr", ClampMin = "-1"))
	int32 MaterialSlotIndex = 0;

	UPROPERTY(EditAnywhere, Category = "Scenecaps|World Partition")
	TSoftObjectPtr<UDataLayerAsset> DataLayer;

	// ---- 新增：自定义参数（可在变体中覆盖、插值）----
	UPROPERTY(EditAnywhere, Category = "Scenecaps|Custom Params", meta = (DisplayName = "Float Params"))
	TMap<FName, float> FloatParams;

	UPROPERTY(EditAnywhere, Category = "Scenecaps|Custom Params", meta = (DisplayName = "Vector Params"))
	TMap<FName, FVector> VectorParams;

	UPROPERTY(EditAnywhere, Category = "Scenecaps|Custom Params", meta = (DisplayName = "Color Params"))
	TMap<FName, FLinearColor> ColorParams;

	UPROPERTY(EditAnywhere, Category = "Scenecaps|Custom Params", meta = (DisplayName = "Bool Params"))
	TMap<FName, bool> BoolParams;

	// ---- 新增：程序化生成参数 ----
	UPROPERTY(EditAnywhere, Category = "Scenecaps|Procedural", meta = (ShowOnlyInnerProperties))
	FScenecapsProceduralParams ProceduralParams;
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

	// ---- 新增：自定义参数覆盖 ----
	UPROPERTY(EditAnywhere, Category = "Custom Params")
	bool bOverrideFloatParams = false;

	UPROPERTY(EditAnywhere, Category = "Custom Params", meta = (EditCondition = "bOverrideFloatParams"))
	TMap<FName, float> FloatParams;

	UPROPERTY(EditAnywhere, Category = "Custom Params")
	bool bOverrideVectorParams = false;

	UPROPERTY(EditAnywhere, Category = "Custom Params", meta = (EditCondition = "bOverrideVectorParams"))
	TMap<FName, FVector> VectorParams;

	UPROPERTY(EditAnywhere, Category = "Custom Params")
	bool bOverrideColorParams = false;

	UPROPERTY(EditAnywhere, Category = "Custom Params", meta = (EditCondition = "bOverrideColorParams"))
	TMap<FName, FLinearColor> ColorParams;

	UPROPERTY(EditAnywhere, Category = "Custom Params")
	bool bOverrideBoolParams = false;

	UPROPERTY(EditAnywhere, Category = "Custom Params", meta = (EditCondition = "bOverrideBoolParams"))
	TMap<FName, bool> BoolParams;

	// ---- 新增：程序化参数覆盖 ----
	UPROPERTY(EditAnywhere, Category = "Procedural")
	bool bOverrideProceduralParams = false;

	UPROPERTY(EditAnywhere, Category = "Procedural", meta = (EditCondition = "bOverrideProceduralParams", ShowOnlyInnerProperties))
	FScenecapsProceduralParams ProceduralParams;

	/** 至少有一个字段被覆盖时为 true（UI 用来显示"有效覆盖"）。 */
	bool HasAnyOverride() const
	{
		return bOverrideVisible || bOverrideTransform || bOverrideMaterial
			|| bOverrideFloatParams || bOverrideVectorParams || bOverrideColorParams || bOverrideBoolParams
			|| bOverrideProceduralParams;
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

// ================================================================
// Variant Interpolation (Runtime)
// ================================================================

/** 插值缓动类型 */
UENUM(BlueprintType)
enum class EScenecapsInterpolationEase : uint8
{
	Linear,
	QuadIn,
	QuadOut,
	QuadInOut,
	CubicIn,
	CubicOut,
	CubicInOut,
	SineIn,
	SineOut,
	SineInOut,
	ExpoIn,
	ExpoOut,
	ExpoInOut
};

/** 变体过渡运行时状态 */
USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsVariantTransition
{
	GENERATED_BODY()

	/** 起始变体（NAME_None = Base） */
	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
	FName FromVariant = NAME_None;

	/** 目标变体 */
	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
	FName ToVariant = NAME_None;

	/** 总时长（秒） */
	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
	float Duration = 1.0f;

	/** 已用时间 */
	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
	float ElapsedTime = 0.0f;

	/** 缓动函数 */
	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
	EScenecapsInterpolationEase EaseType = EScenecapsInterpolationEase::CubicInOut;

	/** 当前归一化进度 [0,1] */
	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
	float Alpha = 0.0f;

	/** 是否正在过渡 */
	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
	bool bIsActive = false;

	/** 是否循环 */
	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
	bool bLoop = false;

	/** 是否反向播放（到达终点后自动反向） */
	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps|Transition")
	bool bPingPong = false;

	/** 重置过渡 */
	void Reset()
	{
		FromVariant = NAME_None;
		ToVariant = NAME_None;
		Duration = 1.0f;
		ElapsedTime = 0.0f;
		EaseType = EScenecapsInterpolationEase::CubicInOut;
		Alpha = 0.0f;
		bIsActive = false;
		bLoop = false;
		bPingPong = false;
	}

	/** 更新过渡，返回当前 Alpha；过渡结束返回 >=1 或 <=0 */
	float Update(float DeltaTime);

	/** 根据缓动类型计算 Alpha */
	static float EaseAlpha(float InAlpha, EScenecapsInterpolationEase EaseType);
};

/** 插值后的条目结果（运行时生成，不存盘） */
USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsInterpolatedEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps")
	FGuid EntryId;

	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps")
	bool bVisible = true;

	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps")
	FTransform RelativeTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps")
	TSoftObjectPtr<UMaterialInterface> MaterialOverride;

	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps")
	int32 MaterialSlotIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps")
	TMap<FName, float> FloatParams;

	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps")
	TMap<FName, FVector> VectorParams;

	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps")
	TMap<FName, FLinearColor> ColorParams;

	UPROPERTY(BlueprintReadOnly, Category = "Scenecaps")
	TMap<FName, bool> BoolParams;
};
