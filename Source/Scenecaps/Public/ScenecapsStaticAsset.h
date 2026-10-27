
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ScenecapsStaticTypes.h"
#include "ScenecapsStaticAsset.generated.h"

class UScenecapsAsset;
class AActor;
class UWorld;


UCLASS(BlueprintType, meta = (DisplayName = "Scene Collection (Static)"))
class SCENECAPS_API UScenecapsStaticAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UScenecapsStaticAsset();

    UPROPERTY(VisibleAnywhere, Category = "Scenecaps")
    FGuid RootId;

    UPROPERTY(EditAnywhere, Category = "Scenecaps", meta = (TitleProperty = "DisplayName"))
    TArray<FScenecapsBakedEntry> Entries;

    UPROPERTY(EditAnywhere, Category = "Scenecaps|Variants", meta = (TitleProperty = "Name"))
    TArray<FScenecapsBakedVariant> Variants;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metadata")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metadata", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metadata")
    TArray<FString> Tags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metadata")
    FLinearColor EditorColor = FLinearColor(0.85f, 0.35f, 0.15f, 1.0f);

    // ---------- Variant API ----------
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Variants")
    TArray<FName> GetVariantNames() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Variants")
    FScenecapsBakedEntry ApplyVariantToEntry(const FScenecapsBakedEntry& BaseEntry, FName VariantName) const;

    const FScenecapsBakedVariant* FindVariant(FName VariantName) const;
    const FScenecapsBakedEntryOverride* FindOverride(const FScenecapsBakedVariant& Variant, FGuid EntryId) const;

    // ---------- Spawn ----------
    UFUNCTION(BlueprintCallable, Category = "Scenecaps",
        meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bIncludeHidden,VariantName"))
    TArray<AActor*> SpawnCollection(UObject* WorldContextObject,
        const FTransform& WorldTransform,
        bool bIncludeHidden = false,
        FName VariantName = NAME_None);

    TArray<AActor*> SpawnCollectionEx(
        UObject* WorldContextObject,
        const FTransform& WorldTransform,
        bool bIncludeHidden,
        FName VariantName,
        TMap<FGuid, AActor*>& OutEntryToActor);

    virtual void PostLoad() override;

    // ---------- Bake ----------
#if WITH_EDITOR
    /** 把世界里的 Actor 序列化为一个 BakedEntry（不修改 asset）。 */
    static void BakeActor(AActor* Actor, FScenecapsBakedEntry& OutEntry);

    /** 把现有 ScenecapsAsset 转成本 StaticAsset（保留层级/变体/覆盖）。 */
    static bool BakeFromSceneAsset(UScenecapsAsset* SourceAsset, UScenecapsStaticAsset* TargetAsset, UWorld* World);
#endif
};