// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsModifierRegistry.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ScenecapsModifier.h"
#include "ScenecapsModifierRegistry.generated.h"

class UScenecapsModifierBase;
class UScenecapsModifierManager;
class UScenecapsAsset;

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Scenecaps Global Modifier Registry"))
class SCENECAPS_API UScenecapsModifierRegistry : public UObject
{
    GENERATED_BODY()

public:
    UScenecapsModifierRegistry();

    // ---- Global Singleton ----

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Modifier Registry")
    static UScenecapsModifierRegistry* Get();

    // ---- External Module Registration ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier Registry")
    void RegisterModifier(UScenecapsModifierBase* Modifier);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier Registry")
    void UnregisterModifier(UScenecapsModifierBase* Modifier);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier Registry")
    void UnregisterModifierByName(FName ModifierName);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier Registry")
    void ClearAllModifiers();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Modifier Registry")
    TArray<UScenecapsModifierBase*> GetAllModifiers() const;

    // ---- Per-Asset Modifier Manager Access ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier Registry")
    class UScenecapsModifierManager* GetOrCreateAssetModifierManager(class UScenecapsAsset* Asset);

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UScenecapsModifierBase>> GlobalModifiers;

    void RegisterModifierInternal(UScenecapsModifierBase* Modifier);
    void SortGlobalModifiers();
};
