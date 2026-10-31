// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsEditorAssetProxy.h

#pragma once

#include "CoreMinimal.h"
#include "ScenecapsTypes.h"
#include "ScenecapsEditorAssetProxy.generated.h"

class UScenecapsAsset;
class UWorld;
class AActor;
class UDataLayerAsset;
class UMaterialInterface;

USTRUCT()
struct FScenecapsEditorEntryView
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "Scenecaps") FGuid EntryId;
    UPROPERTY(VisibleAnywhere, Category = "Scenecaps") FGuid ParentId;
    UPROPERTY(EditAnywhere, Category = "Scenecaps")     FString DisplayName;
    UPROPERTY(EditAnywhere, Category = "Scenecaps")     bool bVisible = true;
    UPROPERTY(EditAnywhere, Category = "Scenecaps")     bool bLocked = false;
    UPROPERTY(EditAnywhere, Category = "Scenecaps")     FTransform RelativeTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, Category = "Scenecaps")     FVector DecalSize = FVector(128.f, 128.f, 256.f);
    UPROPERTY(EditAnywhere, Category = "Scenecaps")     bool bOverrideMobility = false;
    UPROPERTY(EditAnywhere, Category = "Scenecaps", meta = (EditCondition = "bOverrideMobility"))
    TEnumAsByte<EComponentMobility::Type> Mobility = EComponentMobility::Movable;
    UPROPERTY(EditAnywhere, Category = "Scenecaps")     bool bOverrideCollision = false;
    UPROPERTY(EditAnywhere, Category = "Scenecaps", meta = (EditCondition = "bOverrideCollision"))
    TEnumAsByte<ECollisionEnabled::Type> CollisionEnabled = ECollisionEnabled::QueryAndPhysics;
    UPROPERTY(EditAnywhere, Category = "Scenecaps")     bool bOverrideCastShadow = false;
    UPROPERTY(EditAnywhere, Category = "Scenecaps", meta = (EditCondition = "bOverrideCastShadow"))
    bool bCastShadow = true;

    UPROPERTY(EditAnywhere, Category = "Material Override")
    TSoftObjectPtr<UMaterialInterface> MaterialOverride;
    UPROPERTY(EditAnywhere, Category = "Material Override",
        meta = (EditCondition = "MaterialOverride != nullptr", ClampMin = "-1"))
    int32 MaterialSlotIndex = 0;

    UPROPERTY(EditAnywhere, Category = "World Partition")
    TSoftObjectPtr<UDataLayerAsset> DataLayer;

    TSoftObjectPtr<UObject> IconAsset;
    TSoftClassPtr<AActor>   IconClass;
};

class SCENECAPSEDITOR_API FScenecapsEditorAssetProxy
{
public:
    virtual ~FScenecapsEditorAssetProxy() = default;

    virtual UObject* GetAssetObject() const = 0;
    virtual FString   GetAssetName()   const = 0;
    virtual FGuid     GetRootId()      const = 0;
    virtual FLinearColor GetEditorColor() const = 0;
    virtual void      Modify() = 0;
    virtual void      MarkDirty() = 0;

    virtual bool GetSavedEditorView(FVector& OutLoc, FRotator& OutRot) const = 0;
    virtual void SaveEditorView(const FVector& Loc, const FRotator& Rot) = 0;

    virtual TArray<FName> GetVariantNames() const = 0;
    virtual int32 GetVariantCount() const = 0;
    virtual FName GetVariantNameAt(int32 Index) const = 0;

    virtual int32 GetEntryCount() const = 0;
    virtual bool  HasEntry(FGuid EntryId) const = 0;
    virtual bool  GetEntryView(FGuid EntryId, FScenecapsEditorEntryView& Out) const = 0;
    virtual void  ForEachEntry(TFunctionRef<void(const FScenecapsEditorEntryView&)> Fn) const = 0;
    virtual void  GetEntryChildren(FGuid ParentId, TArray<FGuid>& OutChildren) const = 0;
    virtual FScenecapsEditorEntryView GetEffectiveView(FGuid EntryId, FName Variant) const = 0;

    virtual void SetEntryVisible(FGuid EntryId, bool bVisible) = 0;
    virtual void SetEntryLocked(FGuid EntryId, bool bLocked) = 0;
    virtual void SetEntryDisplayName(FGuid EntryId, const FString& Name) = 0;
    virtual void SetEntryRelativeTransform(FGuid EntryId, const FTransform& Xform) = 0;
    virtual void SetEntryParent(FGuid EntryId, FGuid NewParentId) = 0;
    virtual void ApplyViewToEntry(FGuid EntryId, const FScenecapsEditorEntryView& View) = 0;

    virtual FGuid AddEntryFromObject(UObject* Object, const FTransform& WorldTransform, FGuid ParentId) = 0;
    virtual void  DeleteEntries(const TSet<FGuid>& EntryIds) = 0;

    virtual bool CopyEntriesToClipboard(const TSet<FGuid>& EntryIds) = 0;
    virtual bool CanPasteFromClipboard() const = 0;
    virtual void PasteFromClipboard(FGuid TargetParentId) = 0;

    virtual void AddEntriesToVariant(const TSet<FGuid>& EntryIds, FName VariantName) = 0;
    virtual void RemoveEntriesFromVariant(const TSet<FGuid>& EntryIds, FName VariantName) = 0;

    virtual AActor* SpawnPreviewEntry(const FScenecapsEditorEntryView& Effective, UWorld* World) const = 0;
    virtual FTransform GetWorldTransform(FGuid EntryId) const = 0;
};

SCENECAPSEDITOR_API TSharedPtr<FScenecapsEditorAssetProxy> MakeEditorAssetProxy(UObject* Asset);
