#pragma once

#include "CoreMinimal.h"
#include "ScenecapsBatchEdit.generated.h"

/**
 * 多选条目时用于 Details 面板的“批量编辑”结构。
 * 每个字段都带一个 bApplyXxx 开关，勾选时才把对应值应用到所有选中条目。
 */
USTRUCT()
struct FScenecapsBatchEdit
{
    GENERATED_BODY()

    /** 统一设置可见性。 */
    UPROPERTY(EditAnywhere, Category = "Visibility")
    bool bApplyVisible = false;

    UPROPERTY(EditAnywhere, Category = "Visibility", meta = (EditCondition = "bApplyVisible"))
    bool Visible = true;

    /** 统一设置相对变换。 */
    UPROPERTY(EditAnywhere, Category = "Transform")
    bool bApplyTransform = false;

    UPROPERTY(EditAnywhere, Category = "Transform", meta = (EditCondition = "bApplyTransform"))
    FTransform RelativeTransform = FTransform::Identity;
};