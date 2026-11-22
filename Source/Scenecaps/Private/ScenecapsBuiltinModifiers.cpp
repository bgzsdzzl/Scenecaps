// Plugins/Scenecaps/Source/Scenecaps/Private/ScenecapsBuiltinModifiers.cpp

#include "ScenecapsBuiltinModifiers.h"
#include "ScenecapsAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Components/ActorComponent.h"
#include "ScenecapsModule.h"
#include "UObject/UnrealType.h"
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

namespace
{
    // 将一条文本覆盖应用到指定对象（Actor 或组件）的某个属性上
    bool ApplyPropertyTextToObject(UObject* Target, FName PropertyName, const FString& ValueText)
    {
        if (!Target || PropertyName.IsNone()) return false;

        FProperty* Property = FindFProperty<FProperty>(Target->GetClass(), PropertyName);
        if (!Property)
        {
            UE_LOG(LogScenecaps, Warning,
                TEXT("[Scenecaps.PropertyModifier] 对象 %s 上未找到属性 '%s'，跳过"),
                *Target->GetName(), *PropertyName.ToString());
            return false;
        }

        // UE 5.8 中 FProperty 没有 ImportText，使用 ImportText_InContainer。
        // 容器为 UObject 本身时，Container 与 OwnerObject 均为 Target。
        const TCHAR* Result = Property->ImportText_InContainer(*ValueText, Target, Target, 0);
        if (!Result)
        {
            UE_LOG(LogScenecaps, Warning,
                TEXT("[Scenecaps.PropertyModifier] 对象 %s 的属性 '%s' 解析文本失败: %s"),
                *Target->GetName(), *PropertyName.ToString(), *ValueText);
            return false;
        }

        UE_LOG(LogScenecaps, Log,
            TEXT("[Scenecaps.PropertyModifier] 已应用属性 %s.%s = %s"),
            *Target->GetName(), *PropertyName.ToString(), *ValueText);
        return true;
    }
}

void UScenecapsActorPropertyModifier::ExecuteImpl(FScenecapsModifierContext& Context)
{
    // 仅在 Actor 生成后执行
    if (Context.Phase != EScenecapsModifierPhase::PostSpawn)
    {
        return;
    }

    if (!Context.Actor) return;

    if (PropertyOverrides.Num() == 0) return;

    AActor* Actor = Context.Actor;

    // 一次性收集全部组件，供按组件名 / 组件标签定位使用
    TInlineComponentArray<UActorComponent*> Components;
    Actor->GetComponents(Components);

    for (const TPair<FName, FString>& Override : PropertyOverrides)
    {
        const FString FullName = Override.Key.ToString();

        // 支持 "组件名.属性名" 形式；未指定组件名时作用于 Actor 本身
        FString ComponentNameStr;
        FString PropertyNameStr = FullName;
        int32 DotIndex = INDEX_NONE;
        if (FullName.FindChar(TEXT('.'), DotIndex))
        {
            ComponentNameStr = FullName.Left(DotIndex);
            PropertyNameStr = FullName.Mid(DotIndex + 1);
        }

        const FName PropertyName(*PropertyNameStr);
        const FName ComponentName = ComponentNameStr.IsEmpty() ? NAME_None : FName(*ComponentNameStr);

        // 无效属性名（空、或仅有点号）安全跳过
        if (PropertyName.IsNone())
        {
            UE_LOG(LogScenecaps, Warning,
                TEXT("[Scenecaps.PropertyModifier] 无效属性名 '%s'，跳过"),
                *FullName);
            continue;
        }

        bool bApplied = false;

        if (!ComponentName.IsNone())
        {
            // 按组件名定位组件（比较 GetFName）
            UActorComponent* TargetComp = nullptr;
            for (UActorComponent* Comp : Components)
            {
                if (Comp && Comp->GetFName() == ComponentName)
                {
                    TargetComp = Comp;
                    break;
                }
            }

            if (TargetComp)
            {
                bApplied = ApplyPropertyTextToObject(TargetComp, PropertyName, Override.Value);
            }
            else
            {
                UE_LOG(LogScenecaps, Warning,
                    TEXT("[Scenecaps.PropertyModifier] 在 %s 上未找到组件 '%s'，跳过属性 '%s'"),
                    *Actor->GetName(), *ComponentName.ToString(), *FullName);
            }
        }
        else
        {
            // 未指定组件名：作用于 Actor 本身
            bApplied = ApplyPropertyTextToObject(Actor, PropertyName, Override.Value);
        }

        // bApplyToAllComponents：对带有指定 ComponentTags 的组件也应用
        if (bApplyToAllComponents && ComponentTags.Num() > 0)
        {
            for (UActorComponent* Comp : Components)
            {
                if (!Comp) continue;

                bool bHasTag = false;
                for (const FName& Tag : ComponentTags)
                {
                    if (Comp->ComponentHasTag(Tag))
                    {
                        bHasTag = true;
                        break;
                    }
                }
                if (!bHasTag) continue;

                // 已按组件名应用过的组件不再重复设置
                if (Comp->GetFName() == ComponentName) continue;

                if (ApplyPropertyTextToObject(Comp, PropertyName, Override.Value))
                {
                    bApplied = true;
                }
            }
        }

        if (!bApplied)
        {
            UE_LOG(LogScenecaps, Warning,
                TEXT("[Scenecaps.PropertyModifier] 属性 '%s' 未成功应用到 %s"),
                *FullName, *Actor->GetName());
        }
    }
}
