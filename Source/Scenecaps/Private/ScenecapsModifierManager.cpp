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
    if (RegisteredModifiers.Contains(Modifier)) return;
    RegisteredModifiers.AddUnique(Modifier);
    SortModifiersByPriority();
}

void UScenecapsModifierManager::UnregisterModifier(UScenecapsModifierBase* Modifier)
{
    if (!Modifier) return;
    RegisteredModifiers.Remove(Modifier);
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
    for (UScenecapsModifierBase* M : RegisteredModifiers)
    {
        if (M) Result.Add(M);
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
            UScenecapsModifierBase* A = RegisteredModifiers[j];
            UScenecapsModifierBase* B = RegisteredModifiers[j + 1];
            if (A && B && A->GetPriority() > B->GetPriority())
            {
                RegisteredModifiers[j] = B;
                RegisteredModifiers[j + 1] = A;
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
    Context.World = Asset->GetWorld();

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

    for (UScenecapsModifierBase* Modifier : RegisteredModifiers)
    {
        if (!Modifier) continue;
        if (!Modifier->CanExecute(Context)) continue;

        Modifier->Execute(Context);
        OnModifierExecuted(Modifier, UEnum::GetValueAsName(Context.Phase), Context.VariantName);
    }
}

void UScenecapsModifierManager::OnModifierExecuted_Implementation(UScenecapsModifierBase* Modifier, FName Phase, FName VariantName)
{
}
