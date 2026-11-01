// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsBuiltinModifiers.cpp

#include "ScenecapsBuiltinModifiers.h"
#include "ScenecapsAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "UObject/PropertyPortFlags.h"

void UScenecapsTransformModifier::ApplyTransform(FScenecapsEntry& Entry)
{
    Entry.RelativeTransform.SetLocation(Entry.RelativeTransform.GetLocation() + LocationOffset);
    Entry.RelativeTransform.SetRotation((Entry.RelativeTransform.GetRotation() * RotationOffset.Quaternion()).GetNormalized());
    Entry.RelativeTransform.SetScale3D(Entry.RelativeTransform.GetScale3D() * ScaleMultiplier);
}

void UScenecapsTransformModifier::ExecuteImpl(FScenecapsModifierContext& Context)
{
    if (Context.Phase != EScenecapsModifierPhase::PreSpawn &&
        Context.Phase != EScenecapsModifierPhase::PostVariant)
    {
        return;
    }

    if (!Context.Asset) return;

    if (Context.EntryId.IsValid())
    {
        FScenecapsEntry* Entry = Context.Asset->Entries.FindByPredicate([&](const FScenecapsEntry& E) { return E.EntryId == Context.EntryId; });
        if (Entry)
        {
            ApplyTransform(*Entry);
        }
    }
    else
    {
        for (FScenecapsEntry& E : Context.Asset->Entries)
        {
            ApplyTransform(E);
        }
    }
}

void UScenecapsMaterialModifier::ExecuteImpl(FScenecapsModifierContext& Context)
{
    if (Context.Phase != EScenecapsModifierPhase::PreSpawn &&
        Context.Phase != EScenecapsModifierPhase::PostSpawn &&
        Context.Phase != EScenecapsModifierPhase::PostVariant)
    {
        return;
    }

    if (!MaterialOverride.IsValid()) return;

    if (!Context.Asset) return;

    UMaterialInterface* Mat = MaterialOverride.LoadSynchronous();
    if (!Mat) return;

    if (Context.EntryId.IsValid())
    {
        FScenecapsEntry* Entry = Context.Asset->Entries.FindByPredicate([&](const FScenecapsEntry& E) { return E.EntryId == Context.EntryId; });
        if (Entry)
        {
            Entry->MaterialOverride = MaterialOverride;
            Entry->MaterialSlotIndex = MaterialSlotIndex;
        }
    }
    else
    {
        for (FScenecapsEntry& E : Context.Asset->Entries)
        {
            E.MaterialOverride = MaterialOverride;
            E.MaterialSlotIndex = MaterialSlotIndex;
        }
    }
}

void UScenecapsVisibilityModifier::ExecuteImpl(FScenecapsModifierContext& Context)
{
    if (Context.Phase != EScenecapsModifierPhase::PreSpawn &&
        Context.Phase != EScenecapsModifierPhase::PostVariant)
    {
        return;
    }

    if (!Context.Asset) return;

    if (Context.EntryId.IsValid())
    {
        FScenecapsEntry* Entry = Context.Asset->Entries.FindByPredicate([&](const FScenecapsEntry& E) { return E.EntryId == Context.EntryId; });
        if (Entry)
        {
            bool bShouldHide = bVisible;
            if (!RequiredTag.IsNone())
            {
                bool bHasTag = false;
                if (Context.Actor)
                {
                    bHasTag = Context.Actor->ActorHasTag(RequiredTag);
                }
                bShouldHide = bInvertTag ? !bHasTag : bHasTag;
            }
            Entry->bVisible = bShouldHide;
        }
    }
    else
    {
        for (FScenecapsEntry& E : Context.Asset->Entries)
        {
            bool bShouldHide = bVisible;
            if (!RequiredTag.IsNone() && Context.Actor)
            {
                bool bHasTag = Context.Actor->ActorHasTag(RequiredTag);
                bShouldHide = bInvertTag ? !bHasTag : bHasTag;
            }
            E.bVisible = bShouldHide;
        }
    }
}

void UScenecapsCustomParamsModifier::ApplyParams(FScenecapsEntry& Entry)
{
    Entry.FloatParams.Append(FloatParams);
    Entry.VectorParams.Append(VectorParams);
    Entry.ColorParams.Append(ColorParams);
    Entry.BoolParams.Append(BoolParams);
}

void UScenecapsCustomParamsModifier::ExecuteImpl(FScenecapsModifierContext& Context)
{
    if (Context.Phase != EScenecapsModifierPhase::PreSpawn &&
        Context.Phase != EScenecapsModifierPhase::PostVariant)
    {
        return;
    }

    if (!Context.Asset) return;

    if (Context.EntryId.IsValid())
    {
        FScenecapsEntry* Entry = Context.Asset->Entries.FindByPredicate([&](const FScenecapsEntry& E) { return E.EntryId == Context.EntryId; });
        if (Entry)
        {
            ApplyParams(*Entry);
        }
    }
    else
    {
        for (FScenecapsEntry& E : Context.Asset->Entries)
        {
            ApplyParams(E);
        }
    }
}

void UScenecapsActorPropertyModifier::ExecuteImpl(FScenecapsModifierContext& Context)
{
    if (Context.Phase != EScenecapsModifierPhase::PostSpawn)
    {
        return;
    }

    if (!Context.Actor || !Context.Asset) return;

    if (PropertyOverrides.Num() == 0) return;

    // TODO: Implement property override using UE 5.8 property system
    // FProperty doesn't have ImportText in UE 5.8
}
