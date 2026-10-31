// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorAssetProxy.cpp

#include "ScenecapsEditorAssetProxy.h"

#include "ScenecapsAsset.h"
#include "ScenecapsClipboard.h"
#include "ScenecapsEntryHandler.h"
#include "ScenecapsEntryRegistry.h"

#include "Engine/World.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Editor.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

namespace ScenecapsProxyInternal
{
    static FScenecapsEditorEntryView MakeView(const FScenecapsEntry& E)
    {
        FScenecapsEditorEntryView V;
        V.EntryId = E.EntryId;
        V.ParentId = E.ParentId;
        V.DisplayName = E.DisplayName;
        V.bVisible = E.bVisible;
        V.bLocked = E.bLocked;
        V.RelativeTransform = E.RelativeTransform;
        V.DecalSize = E.DecalSize;
        V.bOverrideMobility = E.bOverrideMobility;
        V.Mobility = E.Mobility;
        V.bOverrideCollision = E.bOverrideCollision;
        V.CollisionEnabled = E.CollisionEnabled;
        V.bOverrideCastShadow = E.bOverrideCastShadow;
        V.bCastShadow = E.bCastShadow;
        V.MaterialOverride = E.MaterialOverride;
        V.MaterialSlotIndex = E.MaterialSlotIndex;
        V.DataLayer = E.DataLayer;
        return V;
    }

    static void CopyViewCommon(const FScenecapsEditorEntryView& V, FScenecapsEntry& Out)
    {
        Out.DisplayName = V.DisplayName;
        Out.bVisible = V.bVisible;
        Out.bLocked = V.bLocked;
        Out.RelativeTransform = V.RelativeTransform;
        Out.DecalSize = V.DecalSize;
        Out.bOverrideMobility = V.bOverrideMobility;
        Out.Mobility = V.Mobility;
        Out.bOverrideCollision = V.bOverrideCollision;
        Out.CollisionEnabled = V.CollisionEnabled;
        Out.bOverrideCastShadow = V.bOverrideCastShadow;
        Out.bCastShadow = V.bCastShadow;
        Out.MaterialOverride = V.MaterialOverride;
        Out.MaterialSlotIndex = V.MaterialSlotIndex;
        Out.DataLayer = V.DataLayer;
    }
}

class FScenecapsSceneAssetProxy : public FScenecapsEditorAssetProxy
{
public:
    explicit FScenecapsSceneAssetProxy(UScenecapsAsset* InAsset) : Asset(InAsset) {}

    virtual UObject* GetAssetObject() const override { return Asset.Get(); }
    virtual FString   GetAssetName()   const override { return Asset.IsValid() ? Asset->GetName() : FString(); }
    virtual FGuid     GetRootId()      const override { return Asset.IsValid() ? Asset->RootId : FGuid(); }
    virtual FLinearColor GetEditorColor() const override { return Asset.IsValid() ? Asset->EditorColor : FLinearColor::White; }

    virtual void Modify() override { if (Asset.IsValid()) Asset->Modify(); }
    virtual void MarkDirty() override { if (Asset.IsValid()) Asset->MarkPackageDirty(); }

    virtual bool GetSavedEditorView(FVector& OutLoc, FRotator& OutRot) const override
    {
#if WITH_EDITORONLY_DATA
        if (Asset.IsValid() && Asset->bHasSavedEditorView)
        {
            OutLoc = Asset->EditorViewLocation;
            OutRot = Asset->EditorViewRotation;
            return true;
        }
#endif
        return false;
    }
    virtual void SaveEditorView(const FVector& Loc, const FRotator& Rot) override
    {
#if WITH_EDITORONLY_DATA
        if (Asset.IsValid())
        {
            Asset->EditorViewLocation = Loc;
            Asset->EditorViewRotation = Rot;
            Asset->bHasSavedEditorView = true;
            Asset->MarkPackageDirty();
        }
#endif
    }

    virtual TArray<FName> GetVariantNames() const override
    {
        TArray<FName> Out;
        if (!Asset.IsValid()) return Out;
        for (const FScenecapsVariant& V : Asset->Variants)
            if (!V.Name.IsNone()) Out.Add(V.Name);
        return Out;
    }
    virtual int32 GetVariantCount() const override { return Asset.IsValid() ? Asset->Variants.Num() : 0; }
    virtual FName GetVariantNameAt(int32 Index) const override
    {
        if (Asset.IsValid() && Asset->Variants.IsValidIndex(Index)) return Asset->Variants[Index].Name;
        return NAME_None;
    }

    virtual int32 GetEntryCount() const override { return Asset.IsValid() ? Asset->Entries.Num() : 0; }
    virtual bool  HasEntry(FGuid Id) const override
    {
        return Asset.IsValid() && Asset->Entries.ContainsByPredicate(
            [&](const FScenecapsEntry& E) { return E.EntryId == Id; });
    }
    virtual bool GetEntryView(FGuid Id, FScenecapsEditorEntryView& Out) const override
    {
        if (!Asset.IsValid()) return false;
        const FScenecapsEntry* E = Asset->Entries.FindByPredicate(
            [&](const FScenecapsEntry& X) { return X.EntryId == Id; });
        if (!E) return false;
        Out = ScenecapsProxyInternal::MakeView(*E);
        Out.IconAsset = E->Asset;
        return true;
    }
    virtual void ForEachEntry(TFunctionRef<void(const FScenecapsEditorEntryView&)> Fn) const override
    {
        if (!Asset.IsValid()) return;
        for (const FScenecapsEntry& E : Asset->Entries)
        {
            FScenecapsEditorEntryView V = ScenecapsProxyInternal::MakeView(E);
            V.IconAsset = E.Asset;
            Fn(V);
        }
    }
    virtual void GetEntryChildren(FGuid ParentId, TArray<FGuid>& OutChildren) const override
    {
        OutChildren.Reset();
        if (!Asset.IsValid()) return;
        for (const FScenecapsEntry& E : Asset->Entries)
            if (E.ParentId == ParentId) OutChildren.Add(E.EntryId);
    }
    virtual FScenecapsEditorEntryView GetEffectiveView(FGuid Id, FName Variant) const override
    {
        FScenecapsEditorEntryView V;
        if (!Asset.IsValid()) return V;
        const FScenecapsEntry* E = Asset->Entries.FindByPredicate(
            [&](const FScenecapsEntry& X) { return X.EntryId == Id; });
        if (!E) return V;
        FScenecapsEntry Eff = Asset->ApplyVariantToEntry(*E, Variant);
        V = ScenecapsProxyInternal::MakeView(Eff);
        V.IconAsset = Eff.Asset;
        return V;
    }

    virtual void SetEntryVisible(FGuid Id, bool bVisible) override
    {
        if (FScenecapsEntry* E = FindEntry(Id)) E->bVisible = bVisible;
    }
    virtual void SetEntryLocked(FGuid Id, bool bLocked) override
    {
        if (FScenecapsEntry* E = FindEntry(Id)) E->bLocked = bLocked;
    }
    virtual void SetEntryDisplayName(FGuid Id, const FString& Name) override
    {
        if (FScenecapsEntry* E = FindEntry(Id)) E->DisplayName = Name;
    }
    virtual void SetEntryRelativeTransform(FGuid Id, const FTransform& Xf) override
    {
        if (FScenecapsEntry* E = FindEntry(Id)) E->RelativeTransform = Xf;
    }
    virtual void SetEntryParent(FGuid Id, FGuid NewParentId) override
    {
        if (FScenecapsEntry* E = FindEntry(Id)) E->ParentId = NewParentId;
    }
    virtual void ApplyViewToEntry(FGuid Id, const FScenecapsEditorEntryView& V) override
    {
        if (FScenecapsEntry* E = FindEntry(Id)) ScenecapsProxyInternal::CopyViewCommon(V, *E);
    }

    virtual FGuid AddEntryFromObject(UObject* Object, const FTransform& WorldTransform, FGuid ParentId) override
    {
        if (!Asset.IsValid() || !Object) return FGuid();

        FScenecapsEntry NewEntry;
        NewEntry.ParentId = ParentId;
        NewEntry.RelativeTransform = WorldTransform;

        if (UClass* Cls = Cast<UClass>(Object))
        {
            if (!Cls->IsChildOf(AActor::StaticClass())) return FGuid();
            NewEntry.Asset = Cls;
        }
        else if (UBlueprint* BP = Cast<UBlueprint>(Object))
        {
            if (!BP->GeneratedClass || !BP->GeneratedClass->IsChildOf(AActor::StaticClass())) return FGuid();
            NewEntry.Asset = Object;
        }
        else if (Object->IsA<AActor>())
        {
            NewEntry.Asset = Object->GetClass();
        }
        else
        {
            NewEntry.Asset = Object;
        }

        if (NewEntry.Asset.IsNull()) return FGuid();
        NewEntry.DisplayName = NewEntry.Asset.GetAssetName();

        Asset->Modify();
        Asset->Entries.Add(NewEntry);
        Asset->MarkPackageDirty();
        return NewEntry.EntryId;
    }

    virtual void DeleteEntries(const TSet<FGuid>& EntryIds) override
    {
        if (!Asset.IsValid() || EntryIds.Num() == 0) return;
        Asset->Modify();
        Asset->Entries.RemoveAll([&](const FScenecapsEntry& E) { return EntryIds.Contains(E.EntryId); });
        for (FScenecapsVariant& Vari : Asset->Variants)
            Vari.Overrides.RemoveAll([&](const FScenecapsEntryOverride& O) { return EntryIds.Contains(O.EntryId); });
        Asset->MarkPackageDirty();
    }

    virtual bool CopyEntriesToClipboard(const TSet<FGuid>& EntryIds) override
    {
        if (!Asset.IsValid() || EntryIds.Num() == 0) return false;

        TMap<FGuid, FGuid> Remap;
        for (const FGuid& Id : EntryIds) Remap.Add(Id, FGuid::NewGuid());

        TArray<FScenecapsEntry> Out;
        Out.Reserve(EntryIds.Num());
        for (const FScenecapsEntry& Entry : Asset->Entries)
        {
            if (!EntryIds.Contains(Entry.EntryId)) continue;

            FScenecapsEntry Copy = Entry;
            Copy.EntryId = Remap[Entry.EntryId];
            if (const FGuid* NewParent = Remap.Find(Entry.ParentId))
            {
                Copy.ParentId = *NewParent;
            }
            else
            {
                Copy.ParentId = FGuid();
                const FTransform ParentWorld = ScenecapsEntryUtils::AccumulateWorldTransform(
                    Asset->Entries, Entry.ParentId, Asset->RootId);
                Copy.RelativeTransform = Copy.RelativeTransform * ParentWorld;
            }
            Out.Add(MoveTemp(Copy));
        }
        ScenecapsClipboard::Set(MoveTemp(Out));
        return true;
    }

    virtual bool CanPasteFromClipboard() const override { return ScenecapsClipboard::HasData(); }

    virtual void PasteFromClipboard(FGuid TargetParentId) override
    {
        if (!Asset.IsValid() || !ScenecapsClipboard::HasData()) return;
        const TArray<FScenecapsEntry>& Clip = ScenecapsClipboard::Get();
        const FTransform TargetParentWorld = ScenecapsEntryUtils::AccumulateWorldTransform(
            Asset->Entries, TargetParentId, Asset->RootId);

        TMap<FGuid, FGuid> Remap;
        for (const FScenecapsEntry& C : Clip) Remap.Add(C.EntryId, FGuid::NewGuid());

        Asset->Modify();
        for (const FScenecapsEntry& C : Clip)
        {
            FScenecapsEntry New = C;
            New.EntryId = Remap[C.EntryId];
            if (C.ParentId.IsValid() && Remap.Contains(C.ParentId))
            {
                New.ParentId = Remap[C.ParentId];
            }
            else
            {
                New.ParentId = TargetParentId;
                New.RelativeTransform = C.RelativeTransform.GetRelativeTransform(TargetParentWorld);
            }
            Asset->Entries.Add(MoveTemp(New));
        }
        Asset->MarkPackageDirty();
    }

    virtual void AddEntriesToVariant(const TSet<FGuid>& EntryIds, FName VariantName) override
    {
        if (!Asset.IsValid() || VariantName.IsNone()) return;
        FScenecapsVariant* Vari = Asset->Variants.FindByPredicate(
            [&](const FScenecapsVariant& V) { return V.Name == VariantName; });
        if (!Vari) return;
        Asset->Modify();
        for (const FGuid& Id : EntryIds)
        {
            if (Vari->Overrides.ContainsByPredicate(
                [&](const FScenecapsEntryOverride& O) { return O.EntryId == Id; }))
                continue;
            const FScenecapsEntry* E = Asset->Entries.FindByPredicate(
                [&](const FScenecapsEntry& X) { return X.EntryId == Id; });
            if (!E) continue;
            FScenecapsEntryOverride Ov;
            Ov.EntryId = Id;
            Ov.DisplayName = E->DisplayName;
            Vari->Overrides.Add(MoveTemp(Ov));
        }
        Asset->MarkPackageDirty();
    }

    virtual void RemoveEntriesFromVariant(const TSet<FGuid>& EntryIds, FName VariantName) override
    {
        if (!Asset.IsValid() || VariantName.IsNone()) return;
        FScenecapsVariant* Vari = Asset->Variants.FindByPredicate(
            [&](const FScenecapsVariant& V) { return V.Name == VariantName; });
        if (!Vari) return;
        Asset->Modify();
        Vari->Overrides.RemoveAll([&](const FScenecapsEntryOverride& O) { return EntryIds.Contains(O.EntryId); });
        Asset->MarkPackageDirty();
    }

    virtual AActor* SpawnPreviewEntry(const FScenecapsEditorEntryView& Effective, UWorld* World) const override
    {
        if (!World) return nullptr;

        UObject* Loaded = Effective.IconAsset.LoadSynchronous();
        if (!Loaded) return nullptr;

        FScenecapsSpawnContext Context;
        Context.World = World;
        Context.Transform = FTransform::Identity;
        Context.bIsPreview = true;
        Context.DecalSize = Effective.DecalSize;
        Context.bOverrideMobility = Effective.bOverrideMobility;
        Context.Mobility = Effective.Mobility;
        Context.bOverrideCollision = Effective.bOverrideCollision;
        Context.CollisionEnabled = Effective.CollisionEnabled;
        Context.bOverrideCastShadow = Effective.bOverrideCastShadow;
        Context.bCastShadow = Effective.bCastShadow;
        Context.MaterialOverride = Effective.MaterialOverride.LoadSynchronous();
        Context.MaterialSlotIndex = Effective.MaterialSlotIndex;

        IScenecapsEntryHandler* Handler = FScenecapsEntryRegistry::Get().FindHandlerForAsset(Loaded);
        return Handler ? Handler->SpawnActor(Context, Loaded) : nullptr;
    }

    virtual FTransform GetWorldTransform(FGuid EntryId) const override
    {
        if (!Asset.IsValid()) return FTransform::Identity;
        return ScenecapsEntryUtils::AccumulateWorldTransform(Asset->Entries, EntryId, Asset->RootId);
    }

private:
    FScenecapsEntry* FindEntry(FGuid Id) const
    {
        if (!Asset.IsValid()) return nullptr;
        return Asset->Entries.FindByPredicate([&](const FScenecapsEntry& E) { return E.EntryId == Id; });
    }

    TWeakObjectPtr<UScenecapsAsset> Asset;
};

TSharedPtr<FScenecapsEditorAssetProxy> MakeEditorAssetProxy(UObject* Asset)
{
    if (UScenecapsAsset* Scene = Cast<UScenecapsAsset>(Asset))
        return MakeShared<FScenecapsSceneAssetProxy>(Scene);
    return nullptr;
}
