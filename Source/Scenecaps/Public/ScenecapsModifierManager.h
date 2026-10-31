// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsModifierManager.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ScenecapsModifier.h"
#include "ScenecapsModifierManager.generated.h"

class UScenecapsAsset;
class UScenecapsModifierBase;
class AActor;
class UWorld;

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "Scenecaps Modifier Manager"))
class SCENECAPS_API UScenecapsModifierManager : public UObject
{
    GENERATED_BODY()

public:
    UScenecapsModifierManager();

    // ---- 修改器注册 ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    void RegisterModifier(UScenecapsModifierBase* Modifier);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    void UnregisterModifier(UScenecapsModifierBase* Modifier);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    void UnregisterModifierByName(FName ModifierName);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    void ClearAllModifiers();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenecaps|Modifier")
    TArray<UScenecapsModifierBase*> GetRegisteredModifiers() const;

    // ---- 执行接口 ----

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    void ExecuteModifiers(
        UScenecapsAsset* Asset,
        EScenecapsModifierPhase Phase,
        FName VariantName = NAME_None,
        FGuid EntryId = FGuid(),
        AActor* Actor = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Scenecaps|Modifier")
    void ExecuteModifiersOnActor(
        AActor* Actor,
        UScenecapsAsset* Asset,
        FName VariantName);

    // ---- 蓝图扩展点 ----

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scenecaps|Modifier")
    void OnModifierExecuted(UScenecapsModifierBase* Modifier, FName Phase, FName VariantName);

    // ---- 内部存储 ----

protected:
    TArray<UScenecapsModifierBase*> RegisteredModifiers;

    void SortModifiersByPriority();
    void ExecuteModifiersInternal(FScenecapsModifierContext& Context);
};


