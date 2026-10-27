// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorCommands.cpp
#include "ScenecapsEditorCommands.h"

#include "ScenecapsEntryRegistry.h"
#include "ScenecapsAsset.h"
#include "ScenecapsActor.h"
#include "ScenecapsStaticAsset.h"
#include "ScenecapsStaticActor.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "GameFramework/Actor.h"
#include "Engine/Blueprint.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"

void FScenecapsEditorCommands::GatherFromSelection(UScenecapsAsset* Asset)
{
    if (!GEditor || !Asset) return;
    TArray<AActor*> Selected;
    if (USelection* Selection = GEditor->GetSelectedActors())
    {
        Selection->GetSelectedObjects<AActor>(Selected);
    }
    GatherFromActors(Asset, Selected);
}

void FScenecapsEditorCommands::PlaceIntoCurrentLevel(UScenecapsAsset* Asset)
{
    if (!GEditor || !Asset) return;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return;

    const FScopedTransaction Transaction(FText::FromString(TEXT("Place Scene Collection")));
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transactional;

    if (AScenecapsActor* Container = World->SpawnActor<AScenecapsActor>(
        AScenecapsActor::StaticClass(), FTransform::Identity, Params))
    {
        Container->CollectionAsset = Asset;
        Container->Rebuild();
    }
}

void FScenecapsEditorCommands::GatherFromActors(UScenecapsAsset* Asset, const TArray<AActor*>& Actors)
{
    if (!Asset || Actors.Num() == 0) return;

    const FScopedTransaction Transaction(FText::FromString(TEXT("Gather Scene Collection")));
    Asset->Modify();

    TSet<AActor*> SelectedSet(Actors);
    TArray<AActor*> Roots;
    for (AActor* A : Actors)
    {
        bool bIsRoot = true;
        for (AActor* P = A->GetAttachParentActor(); P; P = P->GetAttachParentActor())
        {
            if (SelectedSet.Contains(P)) { bIsRoot = false; break; }
        }
        if (bIsRoot) Roots.Add(A);
    }

    Asset->Entries.Reset();

    TFunction<void(AActor*, const FGuid&, const FTransform&)> Recurse =
        [&](AActor* Actor, const FGuid& ParentId, const FTransform& ParentWorld)
        {
            FScenecapsEntry Entry;
            Entry.DisplayName = Actor->GetActorLabel();
            Entry.ParentId = ParentId;

            if (UObject* StoredAsset = FScenecapsEntryRegistry::Get().ResolveAssetFromActor(Actor))
                Entry.Asset = StoredAsset;
            else
                Entry.Asset = Actor->GetClass();

            const FTransform WorldXform = Actor->GetActorTransform();
            Entry.RelativeTransform = WorldXform.GetRelativeTransform(ParentWorld);

            const FGuid MyId = Entry.EntryId;
            Asset->Entries.Add(Entry);

            TArray<AActor*> Children;
            Actor->GetAttachedActors(Children);
            for (AActor* Child : Children)
            {
                if (SelectedSet.Contains(Child))
                    Recurse(Child, MyId, WorldXform);
            }
        };

    for (AActor* Root : Roots)
        Recurse(Root, Asset->RootId, FTransform::Identity);

    Asset->MarkPackageDirty();
}

// ================================================================
// Static
// ================================================================

void FScenecapsEditorCommands::PlaceStaticIntoCurrentLevel(UScenecapsStaticAsset* Asset)
{
    if (!GEditor || !Asset) return;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return;

    const FScopedTransaction Transaction(FText::FromString(TEXT("Place Static Scene Collection")));
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transactional;

    if (AScenecapsStaticActor* Container = World->SpawnActor<AScenecapsStaticActor>(
        AScenecapsStaticActor::StaticClass(), FTransform::Identity, Params))
    {
        Container->CollectionAsset = Asset;
        Container->Rebuild();
    }
}

void FScenecapsEditorCommands::GatherStaticFromSelection(UScenecapsStaticAsset* Asset)
{
    if (!GEditor || !Asset) return;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return;

    TArray<AActor*> Selected;
    if (USelection* Selection = GEditor->GetSelectedActors())
    {
        Selection->GetSelectedObjects<AActor>(Selected);
    }
    if (Selected.Num() == 0) return;

    const FScopedTransaction Transaction(FText::FromString(TEXT("Gather Static Scene Collection")));
    Asset->Modify();

    TSet<AActor*> SelectedSet(Selected);
    TArray<AActor*> Roots;
    for (AActor* A : Selected)
    {
        bool bIsRoot = true;
        for (AActor* P = A->GetAttachParentActor(); P; P = P->GetAttachParentActor())
        {
            if (SelectedSet.Contains(P)) { bIsRoot = false; break; }
        }
        if (bIsRoot) Roots.Add(A);
    }

    Asset->Entries.Reset();

    TFunction<void(AActor*, const FGuid&, const FTransform&)> Recurse =
        [&](AActor* Actor, const FGuid& ParentId, const FTransform& ParentWorld)
        {
            FScenecapsBakedEntry Entry;
            UScenecapsStaticAsset::BakeActor(Actor, Entry);
            Entry.ParentId = ParentId;
            Entry.RelativeTransform = Actor->GetActorTransform().GetRelativeTransform(ParentWorld);

            const FGuid MyId = Entry.EntryId;
            Asset->Entries.Add(MoveTemp(Entry));

            TArray<AActor*> Children;
            Actor->GetAttachedActors(Children);
            for (AActor* Child : Children)
            {
                if (SelectedSet.Contains(Child))
                    Recurse(Child, MyId, Actor->GetActorTransform());
            }
        };

    for (AActor* Root : Roots)
        Recurse(Root, Asset->RootId, FTransform::Identity);

    Asset->MarkPackageDirty();
}

UScenecapsStaticAsset* FScenecapsEditorCommands::ConvertToStatic(UScenecapsAsset* SourceAsset)
{
    if (!GEditor || !SourceAsset) return nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) return nullptr;

    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    const FString SrcPath = SourceAsset->GetOutermost()->GetName();
    const FString SrcName = SourceAsset->GetName();
    const FString PackagePath = FPackageName::GetLongPackagePath(SrcPath);
    const FString BaseName = SrcName + TEXT("_Static");

    FString NewName = BaseName;
    FString NewPackageName = PackagePath / NewName;
    int32 Suffix = 1;
    while (FindObject<UObject>(nullptr, *NewPackageName) != nullptr ||
        FPackageName::DoesPackageExist(NewPackageName))
    {
        NewName = FString::Printf(TEXT("%s_%d"), *BaseName, Suffix++);
        NewPackageName = PackagePath / NewName;
    }

    const FScopedTransaction Transaction(FText::FromString(TEXT("Convert to Static Scenecaps")));

    UObject* Created = AssetTools.CreateAsset(NewName, PackagePath, UScenecapsStaticAsset::StaticClass(), nullptr);
    UScenecapsStaticAsset* NewAsset = Cast<UScenecapsStaticAsset>(Created);
    if (!NewAsset) return nullptr;

    UScenecapsStaticAsset::BakeFromSceneAsset(SourceAsset, NewAsset, World);
    FAssetRegistryModule::AssetCreated(NewAsset);
    NewAsset->MarkPackageDirty();
    return NewAsset;
}