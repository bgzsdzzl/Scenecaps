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
#include "ScenecapsModule.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "UObject/SoftObjectPath.h"

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

bool FScenecapsEditorCommands::ExportToCustomBinary(UScenecapsStaticAsset* Asset)
{
    if (!Asset || !GEditor) return false;

    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform) return false;

    void* ParentWindowHandle = nullptr;
    if (FSlateApplication::IsInitialized())
    {
        if (TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow())
            if (ActiveWindow->GetNativeWindow().IsValid())
                ParentWindowHandle = ActiveWindow->GetNativeWindow()->GetOSWindowHandle();
    }

    const FString DefaultPath = FPaths::ProjectContentDir();
    const FString DefaultFile = Asset->GetName() + TEXT(".scnb");
    const FString FileTypes = TEXT("Scenecaps Binary (*.scnb)|*.scnb");

    FString OutFilename;
    TArray<FString> OutFilenames;
    if (!DesktopPlatform->SaveFileDialog(
        ParentWindowHandle,
        FString::Printf(TEXT("Export %s to Custom Binary"), *Asset->GetName()),
        DefaultPath,
        DefaultFile,
        FileTypes,
        0,
        OutFilenames))
    {
        return false;
    }
    OutFilename = OutFilenames.IsEmpty() ? FString() : OutFilenames[0];
    if (OutFilename.IsEmpty()) return false;

    FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*OutFilename);
    if (!FileWriter) return false;

    bool bSuccess = true;
    auto Write = [&](auto&& Value) {
        FileWriter->Serialize(&Value, sizeof(Value));
    };

    const char Magic[4] = { 'S', 'C', 'N', 'B' };
    FileWriter->Serialize((void*)Magic, 4);

    uint32 Version = 1;
    FileWriter->Serialize(&Version, sizeof(Version));
    FileWriter->Serialize(&Asset->RootId, sizeof(Asset->RootId));

    int32 Count = Asset->Entries.Num();
    FileWriter->Serialize(&Count, sizeof(Count));

    auto WriteString = [FileWriter](const FString& Str) {
        int32 Len = Str.Len();
        FileWriter->Serialize(&Len, sizeof(Len));
        if (Len > 0)
        {
            FileWriter->Serialize((void*)*Str, Len * sizeof(TCHAR));
        }
    };

    for (const FScenecapsBakedEntry& E : Asset->Entries)
    {
        FScenecapsBakedEntry Entry = E;

        FileWriter->Serialize(&Entry.EntryId, sizeof(Entry.EntryId));
        FileWriter->Serialize(&Entry.ParentId, sizeof(Entry.ParentId));

        uint8 Flags = 0;
        if (Entry.bVisible) Flags |= 0x01;
        if (Entry.bLocked) Flags |= 0x02;
        FileWriter->Serialize(&Flags, sizeof(Flags));

        WriteString(Entry.DisplayName);
        FileWriter->Serialize(&Entry.RelativeTransform, sizeof(Entry.RelativeTransform));

        UClass* Cls = Entry.ActorClass.Get();
        FString ClassPath = Cls ? FSoftObjectPath(Cls).ToString() : FString();
        WriteString(ClassPath);

        FString MatPath = Entry.MaterialOverride.IsNull() ? FString() : Entry.MaterialOverride.ToSoftObjectPath().ToString();
        WriteString(MatPath);

        FileWriter->Serialize(&Entry.MaterialSlotIndex, sizeof(Entry.MaterialSlotIndex));

        const TArray<uint8>& Props = Entry.SerializedActorProperties;
        int32 PropsNum = Props.Num();
        FileWriter->Serialize(&PropsNum, sizeof(PropsNum));
        if (PropsNum > 0)
        {
            FileWriter->Serialize((void*)Props.GetData(), PropsNum);
        }

        FString DLPath = Entry.DataLayer.IsNull() ? FString() : Entry.DataLayer.ToSoftObjectPath().ToString();
        WriteString(DLPath);
    }

    if (FileWriter->GetError())
    {
        bSuccess = false;
    }

    delete FileWriter;

    if (bSuccess)
    {
        UE_LOG(LogScenecaps, Log, TEXT("[Scenecaps] Exported static asset '%s' to custom binary: %s"), *Asset->GetName(), *OutFilename);
    }
    else
    {
        UE_LOG(LogScenecaps, Error, TEXT("[Scenecaps] Failed to write custom binary: %s"), *OutFilename);
    }

return bSuccess;
}

// PCG 导出暂时禁用

// PCG 导入暂时禁用

bool FScenecapsEditorCommands::ImportFromCustomBinary(UScenecapsStaticAsset* Asset)
{
    if (!Asset || !GEditor) return false;

    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform) return false;

    void* ParentWindowHandle = nullptr;
    if (FSlateApplication::IsInitialized())
    {
        if (TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow())
            if (ActiveWindow->GetNativeWindow().IsValid())
                ParentWindowHandle = ActiveWindow->GetNativeWindow()->GetOSWindowHandle();
    }

    const FString DefaultPath = FPaths::ProjectContentDir();
    const FString FileTypes = TEXT("Scenecaps Binary (*.scnb)|*.scnb");

    TArray<FString> OutFilenames;
    if (!DesktopPlatform->OpenFileDialog(
        ParentWindowHandle,
        FString(TEXT("Import Scenecaps Static Asset from Custom Binary")),
        DefaultPath,
        FString(),
        FileTypes,
        0,
        OutFilenames))
    {
        return false;
    }
    if (OutFilenames.IsEmpty()) return false;

    const FString& InFilename = OutFilenames[0];

    FArchive* FileReader = IFileManager::Get().CreateFileReader(*InFilename);
    if (!FileReader) return false;

    bool bSuccess = true;
    auto Read = [&](auto&& Value) {
        FileReader->Serialize(&Value, sizeof(Value));
    };
    auto ReadString = [&](FString& OutStr) {
        int32 Len = 0;
        FileReader->Serialize(&Len, sizeof(Len));
        if (Len > 0)
        {
            int64 Need = static_cast<int64>(Len) * sizeof(TCHAR);
            TArray<uint8> Bytes;
            Bytes.AddUninitialized(Need);
            FileReader->Serialize(Bytes.GetData(), Need);
            OutStr = FString((TCHAR*)Bytes.GetData(), Len);
        }
        else
        {
            OutStr.Empty();
        }
    };

    char Magic[4] = { 0 };
    FileReader->Serialize(Magic, 4);
    if (Magic[0] != 'S' || Magic[1] != 'C' || Magic[2] != 'N' || Magic[3] != 'B')
    {
        UE_LOG(LogScenecaps, Error, TEXT("[Scenecaps] Invalid magic in custom binary: %s"), *InFilename);
        delete FileReader;
        return false;
    }

    uint32 Version = 0;
    FileReader->Serialize(&Version, sizeof(Version));
    if (Version != 1)
    {
        UE_LOG(LogScenecaps, Error, TEXT("[Scenecaps] Unsupported custom binary version %u (expected 1)"), Version);
        delete FileReader;
        return false;
    }

    FGuid RootId;
    FileReader->Serialize(&RootId, sizeof(RootId));

    int32 Count = 0;
    FileReader->Serialize(&Count, sizeof(Count));

    TArray<FScenecapsBakedEntry> NewEntries;
    NewEntries.Reserve(Count);

    for (int32 i = 0; i < Count; ++i)
    {
        FScenecapsBakedEntry Entry;

        FileReader->Serialize(&Entry.EntryId, sizeof(Entry.EntryId));
        FileReader->Serialize(&Entry.ParentId, sizeof(Entry.ParentId));

        uint8 Flags = 0;
        FileReader->Serialize(&Flags, sizeof(Flags));
        Entry.bVisible = (Flags & 0x01) != 0;
        Entry.bLocked = (Flags & 0x02) != 0;

        ReadString(Entry.DisplayName);
        FileReader->Serialize(&Entry.RelativeTransform, sizeof(Entry.RelativeTransform));

        FString ClassPath;
        ReadString(ClassPath);
        if (!ClassPath.IsEmpty())
        {
            Entry.ActorClass = LoadObject<UClass>(nullptr, *ClassPath);
        }

        FString MatPath;
        ReadString(MatPath);
        if (!MatPath.IsEmpty())
        {
            Entry.MaterialOverride = FSoftObjectPath(MatPath);
        }

        FileReader->Serialize(&Entry.MaterialSlotIndex, sizeof(Entry.MaterialSlotIndex));

        int32 PropsNum = 0;
        FileReader->Serialize(&PropsNum, sizeof(PropsNum));
        if (PropsNum > 0)
        {
            Entry.SerializedActorProperties.AddUninitialized(PropsNum);
            FileReader->Serialize(Entry.SerializedActorProperties.GetData(), PropsNum);
        }

        FString DLPath;
        ReadString(DLPath);
        if (!DLPath.IsEmpty())
        {
            Entry.DataLayer = FSoftObjectPath(DLPath);
        }

        if (!bSuccess) break;

        NewEntries.Add(MoveTemp(Entry));
    }

    if (FileReader->GetError())
    {
        bSuccess = false;
    }

    delete FileReader;

    if (bSuccess)
    {
        Asset->Modify();
        Asset->RootId = RootId;
        Asset->Entries = MoveTemp(NewEntries);
        Asset->MarkPackageDirty();
        UE_LOG(LogScenecaps, Log, TEXT("[Scenecaps] Imported static asset '%s' from custom binary: %s (%d entries)"), *Asset->GetName(), *InFilename, Asset->Entries.Num());
    }
    else
    {
        UE_LOG(LogScenecaps, Error, TEXT("[Scenecaps] Failed to read custom binary: %s"), *InFilename);
    }

    return bSuccess;
}
