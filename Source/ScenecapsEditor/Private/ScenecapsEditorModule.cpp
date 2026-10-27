// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorModule.cpp
#include "ScenecapsEditorModule.h"

#include "ScenecapsActorFactory.h"
#include "ScenecapsFactory.h"
#include "ScenecapsStaticFactory.h"
#include "ScenecapsStaticActorFactory.h"
#include "AssetDefinition_Scenecaps.h"
#include "AssetDefinition_ScenecapsStatic.h"
#include "AssetDefinitionRegistry.h"
#include "ScenecapsAsset.h"
#include "ScenecapsStaticAsset.h"
#include "ScenecapsActor.h"
#include "ScenecapsStaticActor.h"
#include "ScenecapsEditorCommands.h"
#include "ScenecapsEditorToolkit.h"
#include "ScenecapsLocalization.h"
#include "ScenecapsModule.h"
#include "ScenecapsAssetCategories.h"

#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "ContentBrowserMenuContexts.h"
#include "ToolMenus.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "ScenecapsEditorModule"

void FScenecapsEditorModule::StartupModule()
{
    (void)UScenecapsActorFactory::StaticClass();
    (void)UScenecapsFactory::StaticClass();
    (void)UScenecapsStaticFactory::StaticClass();
    (void)UScenecapsStaticActorFactory::StaticClass();
    (void)UAssetDefinition_Scenecaps::StaticClass();
    (void)UAssetDefinition_ScenecapsStatic::StaticClass();

    if (UAssetDefinitionRegistry* Registry = UAssetDefinitionRegistry::Get())
    {
        Registry->RegisterAssetDefinition(GetMutableDefault<UAssetDefinition_Scenecaps>());
        Registry->RegisterAssetDefinition(GetMutableDefault<UAssetDefinition_ScenecapsStatic>());
    }

    if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
    {
        IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
        ScenecapsAssetCategories::GCategoryBit = AssetTools.RegisterAdvancedAssetCategory(
            ScenecapsAssetCategories::CategoryKey(),
            SC_LOCTEXT(ScenecapsCategory, "Scenecaps"));
    }

    RegisterContentBrowserMenus();
    RegisterConsoleCommands();
}

void FScenecapsEditorModule::ShutdownModule()
{
    ScenecapsAssetCategories::GCategoryBit = 0;
    for (IConsoleObject* Cmd : RegisteredConsoleCommands)
    {
        if (Cmd) IConsoleManager::Get().UnregisterConsoleObject(Cmd);
    }
    RegisteredConsoleCommands.Reset();
}

void FScenecapsEditorModule::RegisterToolkit(const TSharedRef<FScenecapsEditorToolkit>& Toolkit)
{
    OpenToolkits.RemoveAll([](const TWeakPtr<FScenecapsEditorToolkit>& T) { return !T.IsValid(); });
    OpenToolkits.AddUnique(Toolkit);
}

void FScenecapsEditorModule::RefreshAllToolkitsLocalizedUI()
{
    OpenToolkits.RemoveAll([](const TWeakPtr<FScenecapsEditorToolkit>& T) { return !T.IsValid(); });
    for (const TWeakPtr<FScenecapsEditorToolkit>& Weak : OpenToolkits)
    {
        if (TSharedPtr<FScenecapsEditorToolkit> Toolkit = Weak.Pin())
            Toolkit->RefreshLocalizedUI();
    }
}

void FScenecapsEditorModule::RegisterContentBrowserMenus()
{
    FToolMenuOwnerScoped OwnerScoped(FName(TEXT("ScenecapsEditor")));

    UToolMenu* AssetMenu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
    FToolMenuSection& AssetSection = AssetMenu->FindOrAddSection(
        "GetAssetActions", NSLOCTEXT("ContentBrowser", "GetAssetActions", "Get Asset Actions"));

    AssetSection.AddDynamicEntry("ScenecapsActions", FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
        {
            const UContentBrowserAssetContextMenuContext* Context =
                InSection.FindContext<UContentBrowserAssetContextMenuContext>();
            if (!Context) return;

            TArray<UScenecapsAsset*> SceneAssets;
            TArray<UScenecapsStaticAsset*> StaticAssets;
            for (const FAssetData& Data : Context->SelectedAssets)
            {
                if (UScenecapsAsset* A = Cast<UScenecapsAsset>(Data.GetAsset()))
                    SceneAssets.Add(A);
                else if (UScenecapsStaticAsset* S = Cast<UScenecapsStaticAsset>(Data.GetAsset()))
                    StaticAssets.Add(S);
            }

            if (SceneAssets.Num() > 0)
            {
                InSection.AddMenuEntry("Scenecaps_PlaceIntoLevel",
                    SC_LOCTEXT(PlaceIntoLevel, "Place into Level"),
                    SC_LOCTEXT(PlaceIntoLevelTip, "Spawn a Scenecaps container in the current level"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateLambda([SceneAssets]()
                        {
                            for (UScenecapsAsset* A : SceneAssets)
                                FScenecapsEditorCommands::PlaceIntoCurrentLevel(A);
                        })));

                InSection.AddMenuEntry("Scenecaps_GatherFromSelection",
                    SC_LOCTEXT(GatherFromSelection, "Gather from Selection"),
                    SC_LOCTEXT(GatherFromSelectionTip, "Replace this asset's entries with currently selected Actors"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateLambda([SceneAssets]()
                        {
                            for (UScenecapsAsset* A : SceneAssets)
                                FScenecapsEditorCommands::GatherFromSelection(A);
                        })));

                InSection.AddMenuEntry("Scenecaps_ConvertToStatic",
                    SC_LOCTEXT(ConvertToStatic, "Convert to Static"),
                    SC_LOCTEXT(ConvertToStaticTip, "Bake this Scene Collection into a self-contained Static asset"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateLambda([SceneAssets]()
                        {
                            for (UScenecapsAsset* A : SceneAssets)
                                FScenecapsEditorCommands::ConvertToStatic(A);
                        })));
            }

            if (StaticAssets.Num() > 0)
            {
                InSection.AddMenuEntry("Scenecaps_PlaceStaticIntoLevel",
                    FText::FromString(TEXT("Place into Level (Static)")),
                    SC_LOCTEXT(PlaceStaticIntoLevelTip, "Spawn a Static Scenecaps container in the current level"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateLambda([StaticAssets]()
                        {
                            for (UScenecapsStaticAsset* A : StaticAssets)
                                FScenecapsEditorCommands::PlaceStaticIntoCurrentLevel(A);
                        })));

                InSection.AddMenuEntry("Scenecaps_GatherStaticFromSelection",
                    FText::FromString(TEXT("Gather from Selection (Static)")),
                    SC_LOCTEXT(GatherStaticFromSelectionTip, "Bake selected Actors directly into this Static asset"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateLambda([StaticAssets]()
                        {
                            for (UScenecapsStaticAsset* A : StaticAssets)
                                FScenecapsEditorCommands::GatherStaticFromSelection(A);
                        })));

                // [Export to Custom Binary menu entry]
                InSection.AddMenuEntry("Scenecaps_ExportCustomBinary",
                    FText::FromString(TEXT("Export to Custom Binary")),
                    FText::FromString(TEXT("Export this Static Scene Collection to a custom binary format")),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateLambda([StaticAssets]()
                        {
                            for (UScenecapsStaticAsset* A : StaticAssets)
                                FScenecapsEditorCommands::ExportToCustomBinary(A);
                        })));
            }
        }));

    }

void FScenecapsEditorModule::RegisterConsoleCommands()
{
    IConsoleManager& CM = IConsoleManager::Get();

    {
        IConsoleObject* Cmd = CM.RegisterConsoleCommand(
            TEXT("Scenecaps.ReloadLocalization"),
            TEXT("Reload localization JSON and refresh open editors."),
            FConsoleCommandDelegate::CreateLambda([]()
                {
                    FScenecapsLocalization::Get().Reload();
                    if (FScenecapsEditorModule* M = FModuleManager::GetModulePtr<FScenecapsEditorModule>("ScenecapsEditor"))
                        M->RefreshAllToolkitsLocalizedUI();
                }),
            ECVF_Default);
        if (Cmd) RegisteredConsoleCommands.Add(Cmd);
    }

    {
        IConsoleObject* Cmd = CM.RegisterConsoleCommand(
            TEXT("Scenecaps.DumpWorldPartition"),
            TEXT("Dump all ScenecapsActors / ScenecapsStaticActors with their Data Layer status."),
            FConsoleCommandDelegate::CreateLambda([]()
                {
                    if (!GEditor) return;
                    UWorld* World = GEditor->GetEditorWorldContext().World();
                    if (!World) return;

                    int32 Count = 0;

                    for (TActorIterator<AScenecapsActor> It(World); It; ++It)
                    {
                        AScenecapsActor* Actor = *It;
                        if (!Actor) continue;
                        ++Count;
                        UE_LOG(LogScenecaps, Log, TEXT("[Scene] %s Spawned=%d DL=%s Status=%s"),
                            *Actor->GetActorNameOrLabel(),
                            Actor->GetSpawnedCount(),
                            Actor->TargetDataLayer.IsNull() ? TEXT("<none>") : *Actor->TargetDataLayer.GetAssetName(),
                            *Actor->GetDataLayerStatusText().ToString());
                    }

                    for (TActorIterator<AScenecapsStaticActor> It(World); It; ++It)
                    {
                        AScenecapsStaticActor* Actor = *It;
                        if (!Actor) continue;
                        ++Count;
                        UE_LOG(LogScenecaps, Log, TEXT("[Static] %s Spawned=%d DL=%s Status=%s"),
                            *Actor->GetActorNameOrLabel(),
                            Actor->GetSpawnedCount(),
                            Actor->TargetDataLayer.IsNull() ? TEXT("<none>") : *Actor->TargetDataLayer.GetAssetName(),
                            *Actor->GetDataLayerStatusText().ToString());
                    }

                    UE_LOG(LogScenecaps, Log, TEXT("Total ScenecapsActors: %d"), Count);
                }),
            ECVF_Default);
        if (Cmd) RegisteredConsoleCommands.Add(Cmd);
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FScenecapsEditorModule, ScenecapsEditor)