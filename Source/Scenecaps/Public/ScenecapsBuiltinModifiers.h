// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsBuiltinModifiers.h

#pragma once

#include "CoreMinimal.h"
#include "ScenecapsModifier.h"
#include "ScenecapsBuiltinModifiers.generated.h"

class UScenecapsAsset;
class AActor;
class UMaterialInterface;
class UStaticMeshComponent;

UCLASS(BlueprintType, meta = (DisplayName = "Transform Modifier"))
class SCENECAPS_API UScenecapsTransformModifier : public UScenecapsModifierBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    FVector LocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    FRotator RotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    FVector ScaleMultiplier = FVector(1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    bool bApplyToChildren = true;

    virtual void ExecuteImpl(FScenecapsModifierContext& Context) override;
    void ApplyTransform(FScenecapsEntry& Entry);
};

UCLASS(BlueprintType, meta = (DisplayName = "Material Override Modifier"))
class SCENECAPS_API UScenecapsMaterialModifier : public UScenecapsModifierBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    TSoftObjectPtr<UMaterialInterface> MaterialOverride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    int32 MaterialSlotIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    bool bApplyToAllSlots = false;

    virtual void ExecuteImpl(FScenecapsModifierContext& Context) override;
};

UCLASS(BlueprintType, meta = (DisplayName = "Visibility Modifier"))
class SCENECAPS_API UScenecapsVisibilityModifier : public UScenecapsModifierBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility")
    bool bVisible = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility")
    FName RequiredTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility")
    bool bInvertTag = false;

    virtual void ExecuteImpl(FScenecapsModifierContext& Context) override;
};

UCLASS(BlueprintType, meta = (DisplayName = "Custom Params Modifier"))
class SCENECAPS_API UScenecapsCustomParamsModifier : public UScenecapsModifierBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Params")
    TMap<FName, float> FloatParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Params")
    TMap<FName, FVector> VectorParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Params")
    TMap<FName, FLinearColor> ColorParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Params")
    TMap<FName, bool> BoolParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Params")
    EScenecapsInterpolationEase InterpolationEase = EScenecapsInterpolationEase::CubicInOut;

    virtual void ExecuteImpl(FScenecapsModifierContext& Context) override;
    void ApplyParams(FScenecapsEntry& Entry);
};

UCLASS(BlueprintType, meta = (DisplayName = "Actor Property Modifier"))
class SCENECAPS_API UScenecapsActorPropertyModifier : public UScenecapsModifierBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Properties")
    TMap<FName, FString> PropertyOverrides;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Properties")
    TArray<FName> ComponentTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Properties")
    bool bApplyToAllComponents = false;

    virtual void ExecuteImpl(FScenecapsModifierContext& Context) override;
};
