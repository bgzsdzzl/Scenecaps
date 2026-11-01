// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsProceduralTypes.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ScenecapsProceduralTypes.generated.h"

/** 程序化散布规则类型 */
UENUM(BlueprintType)
enum class EScenecapsProceduralType : uint8
{
	None,
	PoissonDisk,      // 泊松圆盘采样
	Grid,             // 规则网格
	NoiseCluster,     // 噪声聚类
	Radial,           // 径向分布
	Custom            // 自定义（通过 delegate）
};

/** 程序化散布参数 */
USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsProceduralParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Procedural")
	EScenecapsProceduralType Type = EScenecapsProceduralType::None;

	UPROPERTY(EditAnywhere, Category = "Procedural", meta = (EditCondition = "Type != EScenecapsProceduralType::None"))
	float Density = 0.01f;                    // 单位面积实例数

	UPROPERTY(EditAnywhere, Category = "Procedural", meta = (EditCondition = "Type != EScenecapsProceduralType::None"))
	float MinDistance = 100.0f;               // PoissonDisk: 最小距离

	UPROPERTY(EditAnywhere, Category = "Procedural", meta = (EditCondition = "Type == EScenecapsProceduralType::Grid"))
	float GridSpacing = 200.0f;               // Grid: 网格间距

	UPROPERTY(EditAnywhere, Category = "Procedural", meta = (EditCondition = "Type == EScenecapsProceduralType::NoiseCluster"))
	float NoiseScale = 0.1f;                  // NoiseCluster: 噪声缩放

	UPROPERTY(EditAnywhere, Category = "Procedural", meta = (EditCondition = "Type == EScenecapsProceduralType::NoiseCluster"))
	float ClusterRadius = 500.0f;             // NoiseCluster: 聚类半径

	UPROPERTY(EditAnywhere, Category = "Procedural", meta = (EditCondition = "Type == EScenecapsProceduralType::Radial"))
	float RadialMinRadius = 0.0f;             // Radial: 内半径

	UPROPERTY(EditAnywhere, Category = "Procedural", meta = (EditCondition = "Type == EScenecapsProceduralType::Radial"))
	float RadialMaxRadius = 1000.0f;          // Radial: 外半径

	UPROPERTY(EditAnywhere, Category = "Procedural", meta = (EditCondition = "Type == EScenecapsProceduralType::Radial"))
	int32 RadialSegments = 8;                 // Radial: 扇区数

	UPROPERTY(EditAnywhere, Category = "Procedural")
	int32 Seed = 12345;                       // 随机种子

	UPROPERTY(EditAnywhere, Category = "Procedural|Transform Randomization")
	FVector MinScale = FVector(0.8f);         // 缩放随机范围

	UPROPERTY(EditAnywhere, Category = "Procedural|Transform Randomization")
	FVector MaxScale = FVector(1.2f);

	UPROPERTY(EditAnywhere, Category = "Procedural|Transform Randomization")
	FRotator MaxRotationOffset = FRotator(0, 180, 0); // 旋转随机范围

	UPROPERTY(EditAnywhere, Category = "Procedural|Transform Randomization")
	FVector LocationJitter = FVector(10.0f);  // 位置抖动

	UPROPERTY(EditAnywhere, Category = "Procedural|Bounds")
	FBox Bounds = FBox(FVector(-500), FVector(500)); // 生成包围盒

	UPROPERTY(EditAnywhere, Category = "Procedural|Bounds")
	bool bAlignToSurface = true;              // 贴地

	UPROPERTY(EditAnywhere, Category = "Procedural|Bounds")
	TEnumAsByte<ETraceTypeQuery> SurfaceTraceChannel = ETraceTypeQuery::TraceTypeQuery1;

	/** 验证参数有效性 */
	bool IsValid() const
	{
		return Type != EScenecapsProceduralType::None && Density > 0.0f;
	}
};
