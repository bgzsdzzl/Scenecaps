// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsAsset.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ScenecapsTypes.h"
#include "ScenecapsAsset.generated.h"

class AActor;
class UWorld;

UCLASS(BlueprintType, meta = (DisplayName = "Scene Collection"))
class SCENECAPS_API UScenecapsAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UScenecapsAsset();

	UPROPERTY(VisibleAnywhere, Category = "Scenecaps")
	FGuid RootId;

	UPROPERTY(EditAnywhere, Category = "Scenecaps", meta = (TitleProperty = "DisplayName"))
	TArray<FScenecapsEntry> Entries;

	// ----- Variants -----

	/** 场景变体列表。NAME_None 视作 Base（无覆盖）。 */
	UPROPERTY(EditAnywhere, Category = "Scenecaps|Variants",
		meta = (TitleProperty = "Name"))
	TArray<FScenecapsVariant> Variants;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Variants")
	TArray<FName> GetVariantNames() const;

	/** 把变体应用到 BaseEntry 的副本上并返回。变体不存在时返回 BaseEntry。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Variants")
	FScenecapsEntry ApplyVariantToEntry(const FScenecapsEntry& BaseEntry, FName VariantName) const;

	const FScenecapsVariant* FindVariant(FName VariantName) const;
	const FScenecapsEntryOverride* FindOverride(const FScenecapsVariant& Variant, FGuid EntryId) const;

	// ----- Asset metadata -----

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metadata")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metadata", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metadata")
	TArray<FString> Tags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metadata")
	FLinearColor EditorColor = FLinearColor(0.15f, 0.55f, 0.9f, 1.0f);

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Editor", meta = (IgnoreForMemberInitializationTest))
	bool bHasSavedEditorView = false;

	UPROPERTY(VisibleAnywhere, Category = "Editor", meta = (IgnoreForMemberInitializationTest))
	FVector EditorViewLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Editor", meta = (IgnoreForMemberInitializationTest))
	FRotator EditorViewRotation = FRotator::ZeroRotator;
#endif

	// ----- Spawn -----

	UFUNCTION(BlueprintCallable, Category = "Scenecaps",
		meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bIncludeHidden,VariantName"))
	TArray<AActor*> SpawnCollection(UObject* WorldContextObject,
		const FTransform& WorldTransform,
		bool bIncludeHidden = false,
		FName VariantName = NAME_None);

	/** C++ 专用：额外返回 EntryId → SpawnedActor 映射，用于 Data Layer 分配。 */
	TArray<AActor*> SpawnCollectionEx(
		UObject* WorldContextObject,
		const FTransform& WorldTransform,
		bool bIncludeHidden,
		FName VariantName,
		TMap<FGuid, AActor*>& OutEntryToActor);

	virtual void PostLoad() override;
};