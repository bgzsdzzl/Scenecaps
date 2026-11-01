// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsModifierRegistry.cpp

#include "ScenecapsModifierRegistry.h"
#include "ScenecapsModifierManager.h"
#include "ScenecapsAsset.h"
#include "Engine/World.h"

UScenecapsModifierRegistry::UScenecapsModifierRegistry()
{
}

UScenecapsModifierRegistry* UScenecapsModifierRegistry::Get()
{
    static UScenecapsModifierRegistry* Instance = nullptr;
    if (!Instance)
    {
        Instance = NewObject<UScenecapsModifierRegistry>();
        Instance->AddToRoot();
    }
    return Instance;
}

void UScenecapsModifierRegistry::RegisterModifier(UScenecapsModifierBase* Modifier)
{
    RegisterModifierInternal(Modifier);
}

void UScenecapsModifierRegistry::UnregisterModifier(UScenecapsModifierBase* Modifier)
{
    if (!Modifier) return;
    GlobalModifiers.RemoveAll([&](TObjectPtr<UScenecapsModifierBase> M) { return M.Get() == Modifier; });
}

void UScenecapsModifierRegistry::UnregisterModifierByName(FName ModifierName)
{
    GlobalModifiers.RemoveAll([&](TObjectPtr<UScenecapsModifierBase> M) { return M && M->GetModifierName() == ModifierName; });
}

void UScenecapsModifierRegistry::ClearAllModifiers()
{
    GlobalModifiers.Empty();
}

TArray<UScenecapsModifierBase*> UScenecapsModifierRegistry::GetAllModifiers() const
{
    TArray<UScenecapsModifierBase*> Result;
    for (TObjectPtr<UScenecapsModifierBase> M : GlobalModifiers)
    {
        if (M) Result.Add(M.Get());
    }
    return Result;
}

void UScenecapsModifierRegistry::RegisterModifierInternal(UScenecapsModifierBase* Modifier)
{
    if (!Modifier) return;
    if (GlobalModifiers.ContainsByPredicate([&](TObjectPtr<UScenecapsModifierBase> M) { return M.Get() == Modifier; }))
    {
        return;
    }
    GlobalModifiers.Add(Modifier);
    SortGlobalModifiers();
}

void UScenecapsModifierRegistry::SortGlobalModifiers()
{
    // Manual bubble sort
    int32 N = GlobalModifiers.Num();
    for (int32 i = 0; i < N - 1; ++i)
    {
        for (int32 j = 0; j < N - i - 1; ++j)
        {
            UScenecapsModifierBase* A = GlobalModifiers[j].Get();
            UScenecapsModifierBase* B = GlobalModifiers[j + 1].Get();
            if (A && B && A->GetPriority() > B->GetPriority())
            {
                GlobalModifiers.Swap(j, j + 1);
            }
        }
    }
}

class UScenecapsModifierManager* UScenecapsModifierRegistry::GetOrCreateAssetModifierManager(class UScenecapsAsset* Asset)
{
    if (!Asset) return nullptr;
    return Asset->GetOrCreateModifierManager();
}
