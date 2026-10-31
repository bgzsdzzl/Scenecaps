// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsModifierLibrary.cpp

#include "ScenecapsModifierLibrary.h"
#include "ScenecapsModifierRegistry.h"
#include "ScenecapsModifierManager.h"
#include "ScenecapsAsset.h"
#include "ScenecapsBuiltinModifiers.h"
#include "Engine/World.h"

UScenecapsModifierRegistry* UScenecapsModifierLibrary::GetModifierRegistry()
{
    return UScenecapsModifierRegistry::Get();
}

void UScenecapsModifierLibrary::RegisterGlobalModifier(UScenecapsModifierBase* Modifier)
{
    if (Modifier)
    {
        UScenecapsModifierRegistry::Get()->RegisterModifier(Modifier);
    }
}

void UScenecapsModifierLibrary::UnregisterGlobalModifier(UScenecapsModifierBase* Modifier)
{
    if (Modifier)
    {
        UScenecapsModifierRegistry::Get()->UnregisterModifier(Modifier);
    }
}

void UScenecapsModifierLibrary::UnregisterGlobalModifierByName(FName ModifierName)
{
    UScenecapsModifierRegistry::Get()->UnregisterModifierByName(ModifierName);
}

TArray<UScenecapsModifierBase*> UScenecapsModifierLibrary::GetAllGlobalModifiers()
{
    return UScenecapsModifierRegistry::Get()->GetAllModifiers();
}

UScenecapsModifierManager* UScenecapsModifierLibrary::GetOrCreateModifierManager(UScenecapsAsset* Asset)
{
    if (!Asset) return nullptr;
    return Asset->GetOrCreateModifierManager();
}

void UScenecapsModifierLibrary::ApplyModifiersToAsset(UScenecapsAsset* Asset, EScenecapsModifierPhase Phase, FName VariantName, FGuid EntryId)
{
    if (Asset)
    {
        Asset->ApplyModifiers(Phase, VariantName, EntryId);
    }
}

void UScenecapsModifierLibrary::ApplyModifiersToActor(UScenecapsAsset* Asset, AActor* Actor, FName VariantName)
{
    if (Asset && Actor)
    {
        Asset->ApplyModifiersToActor(Actor, VariantName);
    }
}

void UScenecapsModifierLibrary::RegisterModifierToAsset(UScenecapsAsset* Asset, UScenecapsModifierBase* Modifier)
{
    if (Asset && Modifier)
    {
        UScenecapsModifierManager* Mgr = Asset->GetOrCreateModifierManager();
        if (Mgr)
        {
            Mgr->RegisterModifier(Modifier);
        }
    }
}

void UScenecapsModifierLibrary::UnregisterModifierFromAsset(UScenecapsModifierBase* Modifier)
{
    // Note: Need to know which asset's manager to unregister from
    // This would require tracking which asset the modifier is registered to
}

UScenecapsTransformModifier* UScenecapsModifierLibrary::CreateTransformModifier(FVector LocationOffset, FRotator RotationOffset, FVector ScaleMultiplier)
{
    UScenecapsTransformModifier* Modifier = NewObject<UScenecapsTransformModifier>();
    Modifier->LocationOffset = LocationOffset;
    Modifier->RotationOffset = RotationOffset;
    Modifier->ScaleMultiplier = ScaleMultiplier;
    return Modifier;
}

UScenecapsMaterialModifier* UScenecapsModifierLibrary::CreateMaterialModifier(TSoftObjectPtr<class UMaterialInterface> Material, int32 SlotIndex)
{
    UScenecapsMaterialModifier* Modifier = NewObject<UScenecapsMaterialModifier>();
    Modifier->MaterialOverride = Material;
    Modifier->MaterialSlotIndex = SlotIndex;
    return Modifier;
}

UScenecapsVisibilityModifier* UScenecapsModifierLibrary::CreateVisibilityModifier(bool bVisible, FName RequiredTag, bool bInvertTag)
{
    UScenecapsVisibilityModifier* Modifier = NewObject<UScenecapsVisibilityModifier>();
    Modifier->bVisible = bVisible;
    Modifier->RequiredTag = RequiredTag;
    Modifier->bInvertTag = bInvertTag;
    return Modifier;
}

UScenecapsCustomParamsModifier* UScenecapsModifierLibrary::CreateCustomParamsModifier(TMap<FName, float> FloatParams, TMap<FName, FVector> VectorParams, TMap<FName, FLinearColor> ColorParams, TMap<FName, bool> BoolParams)
{
    UScenecapsCustomParamsModifier* Modifier = NewObject<UScenecapsCustomParamsModifier>();
    Modifier->FloatParams = FloatParams;
    Modifier->VectorParams = VectorParams;
    Modifier->ColorParams = ColorParams;
    Modifier->BoolParams = BoolParams;
    return Modifier;
}

void UScenecapsModifierLibrary::ApplyModifiersAtRuntime(UScenecapsAsset* Asset, EScenecapsModifierPhase Phase, FName VariantName)
{
    if (Asset)
    {
        Asset->ApplyModifiers(Phase, VariantName);
    }
}

void UScenecapsModifierLibrary::ApplyModifiersToSpawnedActor(UScenecapsAsset* Asset, AActor* Actor, FName VariantName)
{
    if (Asset && Actor)
    {
        Asset->ApplyModifiersToActor(Actor, VariantName);
    }
}
