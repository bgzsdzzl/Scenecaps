// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsAsset.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ScenecapsTypes.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ScenecapsAsset.generated.h"

class AActor;
class UWorld;
class UScenecapsModifierManager;

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

	// ---- 新增：变体插值/过渡 ----
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Variants")
	FScenecapsInterpolatedEntry InterpolateEntry(
		const FScenecapsEntry& BaseEntry,
		FName FromVariant,
		FName ToVariant,
		float Alpha) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Variants")
	TArray<FScenecapsInterpolatedEntry> InterpolateAllEntries(
		FName FromVariant,
		FName ToVariant,
		float Alpha) const;

	UFUNCTION(BlueprintCallable, Category = "Scenecaps|Variants")
	FScenecapsVariantTransition BeginTransition(
		FName FromVariant,
		FName ToVariant,
		float Duration,
		EScenecapsInterpolationEase EaseType = EScenecapsInterpolationEase::CubicInOut,
		bool bLoop = false,
		bool bPingPong = false) const;

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

	// ----- Modifiers -----

	UPROPERTY(EditAnywhere, Instanced, Category = "Scenecaps|Modifiers")
	TObjectPtr<UScenecapsModifierManager> ModifierManager;

	UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifiers")
	void ApplyModifiers(EScenecapsModifierPhase Phase, FName VariantName = NAME_None, FGuid EntryId = FGuid());

	UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifiers")
	void ApplyModifiersToActor(AActor* Actor, FName VariantName = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifiers")
	UScenecapsModifierManager* GetOrCreateModifierManager();

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

	// ---- 新增：程序化生成 ----
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Procedural")
	TArray<FTransform> GenerateProceduralTransforms(
		const FScenecapsEntry& Entry,
		const FTransform& WorldTransform,
		FName VariantName = NAME_None) const;

	// ---- 新增：ISM/HISM 批量生成 ----
	UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM")
	UInstancedStaticMeshComponent* SpawnCollectionAsISM(
		UObject* WorldContextObject,
		const FTransform& WorldTransform,
		FName VariantName = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Scenecaps|ISM")
	UHierarchicalInstancedStaticMeshComponent* SpawnCollectionAsHISM(
		UObject* WorldContextObject,
		const FTransform& WorldTransform,
		FName VariantName = NAME_None, int32 NumLODs = 3,
		float LODScreenSize = 0.5f);

	virtual void PostLoad() override;
};
