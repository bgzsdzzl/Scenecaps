// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsModifierManager.cpp

#include "ScenecapsModifierManager.h"
#include "ScenecapsAsset.h"
#include "ScenecapsActor.h"
#include "Engine/World.h"

UScenecapsModifierManager::UScenecapsModifierManager()
{
}

void UScenecapsModifierManager::RegisterModifier(UScenecapsModifierBase* Modifier)
{
    if (!Modifier) return;
    if (RegisteredModifiers.ContainsByPredicate([&](TObjectPtr<UScenecapsModifierBase> M) { return M.Get() == Modifier; })) return;
    RegisteredModifiers.AddUnique(Modifier);
    SortModifiersByPriority();
}

void UScenecapsModifierManager::UnregisterModifier(UScenecapsModifierBase* Modifier)
{
    if (!Modifier) return;
    RegisteredModifiers.RemoveAll([&](TObjectPtr<UScenecapsModifierBase> M) { return M.Get() == Modifier; });
}

void UScenecapsModifierManager::UnregisterModifierByName(FName ModifierName)
{
    RegisteredModifiers.RemoveAll([&](UScenecapsModifierBase* M) { return M && M->GetModifierName() == ModifierName; });
}

void UScenecapsModifierManager::ClearAllModifiers()
{
    RegisteredModifiers.Empty();
}

TArray<UScenecapsModifierBase*> UScenecapsModifierManager::GetRegisteredModifiers() const
{
    TArray<UScenecapsModifierBase*> Result;
    for (TObjectPtr<UScenecapsModifierBase> M : RegisteredModifiers)
    {
        if (M) Result.Add(M.Get());
    }
    return Result;
}

void UScenecapsModifierManager::SortModifiersByPriority()
{
    // Manual bubble sort to avoid TArray::Sort template issues
    int32 N = RegisteredModifiers.Num();
    for (int32 i = 0; i < N - 1; ++i)
    {
        for (int32 j = 0; j < N - i - 1; ++j)
        {
            UScenecapsModifierBase* A = RegisteredModifiers[j].Get();
            UScenecapsModifierBase* B = RegisteredModifiers[j + 1].Get();
            if (A && B && A->GetPriority() > B->GetPriority())
            {
                RegisteredModifiers.Swap(j, j + 1);
            }
        }
    }
}

void UScenecapsModifierManager::ExecuteModifiers(
    UScenecapsAsset* Asset,
    EScenecapsModifierPhase Phase,
    FName VariantName,
    FGuid EntryId,
    AActor* Actor)
{
    if (!Asset) return;

    FScenecapsModifierContext Context;
    Context.Asset = Asset;
    Context.Phase = Phase;
    Context.VariantName = VariantName;
    Context.EntryId = EntryId;
    Context.Actor = Actor;
    Context.World = Actor ? Actor->GetWorld() : nullptr;

    ExecuteModifiersInternal(Context);
}

void UScenecapsModifierManager::ExecuteModifiersOnActor(
    AActor* Actor,
    UScenecapsAsset* Asset,
    FName VariantName)
{
    if (!Actor || !Asset) return;

    FScenecapsModifierContext Context;
    Context.Asset = Asset;
    Context.Phase = EScenecapsModifierPhase::PostSpawn;
    Context.VariantName = VariantName;
    Context.Actor = Actor;
    Context.World = Actor->GetWorld();

    ExecuteModifiersInternal(Context);
}

void UScenecapsModifierManager::ExecuteModifiersInternal(FScenecapsModifierContext& Context)
{
    SortModifiersByPriority();

    for (TObjectPtr<UScenecapsModifierBase> ModifierPtr : RegisteredModifiers)
    {
        UScenecapsModifierBase* Modifier = ModifierPtr.Get();
        if (!Modifier) continue;
        if (!Modifier->CanExecute(Context)) continue;

        Modifier->Execute(Context);
        OnModifierExecuted(Modifier, UEnum::GetValueAsName(Context.Phase), Context.VariantName);
    }
}

void UScenecapsModifierManager::OnModifierExecuted_Implementation(UScenecapsModifierBase* Modifier, FName Phase, FName VariantName)
{
}
