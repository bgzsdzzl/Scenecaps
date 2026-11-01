// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsModifierLibrary.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ScenecapsModifierLibrary.generated.h"

class UScenecapsAsset;
class UScenecapsModifierBase;
class UScenecapsModifierManager;
class UScenecapsModifierRegistry;
class AActor;

UCLASS()
class SCENECAPS_API UScenecapsModifierLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ---- Global Registry ----

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Modifier", meta = (WorldContext = "WorldContextObject"))
    static UScenecapsModifierRegistry* GetModifierRegistry();

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    static void RegisterGlobalModifier(UScenecapsModifierBase* Modifier);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    static void UnregisterGlobalModifier(UScenecapsModifierBase* Modifier);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    static void UnregisterGlobalModifierByName(FName ModifierName);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Modifier")
    static TArray<UScenecapsModifierBase*> GetAllGlobalModifiers();

    // ---- Asset Modifier Manager ----

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Modifier")
    static UScenecapsModifierManager* GetOrCreateModifierManager(UScenecapsAsset* Asset);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    static void ApplyModifiersToAsset(UScenecapsAsset* Asset, EScenecapsModifierPhase Phase, FName VariantName = NAME_None, FGuid EntryId = FGuid());

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    static void ApplyModifiersToActor(UScenecapsAsset* Asset, AActor* Actor, FName VariantName = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    static void RegisterModifierToAsset(UScenecapsAsset* Asset, UScenecapsModifierBase* Modifier);

UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    static void UnregisterModifierFromAsset(UScenecapsAsset* Asset, UScenecapsModifierBase* Modifier);

    // ---- Built-in Modifier Creation ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier|Builtin")
    static class UScenecapsTransformModifier* CreateTransformModifier(FVector LocationOffset = FVector::ZeroVector, FRotator RotationOffset = FRotator::ZeroRotator, FVector ScaleMultiplier = FVector(1.0f));

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier|Builtin")
    static class UScenecapsMaterialModifier* CreateMaterialModifier(TSoftObjectPtr<class UMaterialInterface> Material, int32 SlotIndex = 0);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier|Builtin")
    static class UScenecapsVisibilityModifier* CreateVisibilityModifier(bool bVisible = true, FName RequiredTag = NAME_None, bool bInvertTag = false);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier|Builtin")
    static class UScenecapsCustomParamsModifier* CreateCustomParamsModifier(TMap<FName, float> FloatParams, TMap<FName, FVector> VectorParams, TMap<FName, FLinearColor> ColorParams, TMap<FName, bool> BoolParams);

    // ---- Runtime Application ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    static void ApplyModifiersAtRuntime(UScenecapsAsset* Asset, EScenecapsModifierPhase Phase, FName VariantName = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    static void ApplyModifiersToSpawnedActor(UScenecapsAsset* Asset, AActor* Actor, FName VariantName = NAME_None);
};
