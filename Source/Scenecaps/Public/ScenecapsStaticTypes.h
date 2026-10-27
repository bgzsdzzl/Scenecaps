// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsStaticTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ScenecapsStaticTypes.generated.h"

class UDataLayerAsset;
class UMaterialInterface;
class AActor;

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsBakedEntry
{
    GENERATED_BODY()

    FScenecapsBakedEntry()
        : EntryId(FGuid::NewGuid()), ParentId(FGuid()) {}

    UPROPERTY(VisibleAnywhere, Category = "Scenecaps", meta = (IgnoreForMemberInitializationTest))
    FGuid EntryId;

    UPROPERTY(EditAnywhere, Category = "Scenecaps")
    FGuid ParentId;

    UPROPERTY(EditAnywhere, Category = "Scenecaps")
    FString DisplayName;

    UPROPERTY(EditAnywhere, Category = "Scenecaps")
    TSubclassOf<AActor> ActorClass;

    UPROPERTY()
    TArray<uint8> SerializedActorProperties;

    UPROPERTY(VisibleAnywhere, Category = "Scenecaps")
    FString BakedClassName;

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

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsBakedEntryOverride
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Scenecaps")
    FGuid EntryId;

    UPROPERTY(VisibleAnywhere, Category = "Scenecaps")
    FString DisplayName;

    UPROPERTY(EditAnywhere, Category = "Visibility")
    bool bOverrideVisible = false;

    UPROPERTY(EditAnywhere, Category = "Visibility", meta = (EditCondition = "bOverrideVisible"))
    bool bVisible = true;

    UPROPERTY(EditAnywhere, Category = "Transform")
    bool bOverrideTransform = false;

    UPROPERTY(EditAnywhere, Category = "Transform", meta = (EditCondition = "bOverrideTransform"))
    FTransform RelativeTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, Category = "Material")
    bool bOverrideMaterial = false;

    UPROPERTY(EditAnywhere, Category = "Material", meta = (EditCondition = "bOverrideMaterial"))
    TSoftObjectPtr<UMaterialInterface> MaterialOverride;

    UPROPERTY(EditAnywhere, Category = "Material", meta = (EditCondition = "bOverrideMaterial"))
    int32 MaterialSlotIndex = 0;
};

USTRUCT(BlueprintType)
struct SCENECAPS_API FScenecapsBakedVariant
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Scenecaps")
    FName Name;

    UPROPERTY(EditAnywhere, Category = "Scenecaps", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, Category = "Scenecaps", meta = (TitleProperty = "DisplayName"))
    TArray<FScenecapsBakedEntryOverride> Overrides;
};