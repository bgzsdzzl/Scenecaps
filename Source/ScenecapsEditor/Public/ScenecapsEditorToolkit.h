// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsEditorToolkit.h
#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "PreviewScene.h"
#include "ScenecapsTypes.h"
#include "ScenecapsEditorAssetProxy.h"
#include "AssetRegistry/AssetData.h"
#include "Containers/ArrayView.h"

class AActor;
class SScenecapsEditorViewport;
class SScenecapsEditorOutliner;
class FScenecapsEditorViewportClient;
class IDetailsView;
class IStructureDetailsView;
class FStructOnScope;
class SBox;
class UScenecapsViewportSettings;

class SCENECAPSEDITOR_API FScenecapsEditorToolkit : public FAssetEditorToolkit
{
public:
    void InitEditor(const EToolkitMode::Type Mode,
        const TSharedPtr<IToolkitHost>& InitToolkitHost,
        UObject* InAsset);

    virtual FName        GetToolkitFName() const override;
    virtual FText        GetBaseToolkitName() const override;
    virtual FString      GetWorldCentricTabPrefix() const override;
    virtual FLinearColor GetWorldCentricTabColorScale() const override;
    virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
    virtual bool OnRequestClose(EAssetEditorCloseReason CloseReason) override;

    FPreviewScene* GetPreviewScene() const { return PreviewScene.Get(); }
    TSharedPtr<FScenecapsEditorAssetProxy> GetAssetProxy() const { return AssetProxy; }
    UObject* GetEditingAssetObject() const { return AssetProxy.IsValid() ? AssetProxy->GetAssetObject() : nullptr; }

    TWeakPtr<SScenecapsEditorViewport> GetViewportWidget() const { return ViewportWidget; }
    TObjectPtr<UScenecapsViewportSettings> GetViewportSettings() const { return ViewportSettings; }

    FName GetPreviewVariant() const { return PreviewVariant; }
    void  SetPreviewVariant(FName InVariant);

    void OnAssetsDropped(const FDragDropEvent& DragDropEvent, TArrayView<FAssetData> InAssets);

    void ClearPreviewScene();
    void RebuildPreviewScene();
    void RefreshOutliner();
    void SelectPreviewActor(const FGuid& EntryId);
    void UpdatePreviewActorVisibility(const FGuid& EntryId, bool bVisible);
    void ReparentEntry(const FGuid& ChildId, const FGuid& NewParentId);
    void ReparentEntries(const TArray<FGuid>& ChildIds, const FGuid& NewParentId);
    void HandleViewportActorSelected(AActor* Actor);

    void OnDetailsPropertyCommitted(const FPropertyChangedEvent& PropertyChangedEvent);
    void OnAssetPropertyCommitted(const FPropertyChangedEvent& PropertyChangedEvent);

    void RefreshLocalizedUI();

    void ShowEntryDetails(FGuid EntryId);
    void ShowEntriesDetails(const TArray<FGuid>& EntryIds);
    void ShowAssetDetails();

    void UpdateCachedEditorView(const FVector& InLocation, const FRotator& InRotation);
    void SaveEditorViewToAsset();

    void DeleteSelectedViewportActors(const TArray<TWeakObjectPtr<AActor>>& ActorsToDelete);
    void BeginTransaction();
    void EndTransactionAndUpdateTransforms(const TArray<TWeakObjectPtr<AActor>>& ModifiedActors);

    void UpdatePreviewActor(const FGuid& EntryId);

private:
    TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnTab_Outliner(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
    TSharedRef<SDockTab> MakeTab(const FName& TabId, const FText& Label, TSharedRef<SWidget> Content);

    void AddEntryFromDroppedObject(UObject* DroppedObject, const FVector& WorldLocation, const FRotator& WorldRotation);
    AActor* SpawnPreviewEntry(const FScenecapsEditorEntryView& Effective, UWorld* World);
    void RequestRedraw();

private:
    TSharedPtr<FScenecapsEditorAssetProxy> AssetProxy;
    TUniquePtr<FPreviewScene> PreviewScene;
    TWeakPtr<SScenecapsEditorViewport> ViewportWidget;
    TWeakPtr<SScenecapsEditorOutliner> OutlinerWidget;

    TMap<FGuid, TWeakObjectPtr<AActor>> PreviewActors;

    TWeakPtr<IStructureDetailsView> StructureDetailsView;
    TWeakPtr<IDetailsView>          AssetDetailsView;

    TSharedPtr<SBox>    DetailsHost;
    TSharedPtr<SWidget> DetailsEntryWidget;
    TSharedPtr<SWidget> DetailsAssetWidget;

    TSharedPtr<FStructOnScope> DetailsStructScope;
    TSharedPtr<FStructOnScope> BatchDetailsScope;
    TArray<FGuid> DetailsEntryIds;

    FVector CachedEditorViewLocation = FVector::ZeroVector;
    FRotator CachedEditorViewRotation = FRotator::ZeroRotator;
    bool bEditorViewDirty = false;

    TObjectPtr<UScenecapsViewportSettings> ViewportSettings;

    static const FName ViewportTabId;
    static const FName OutlinerTabId;
    static const FName DetailsTabId;
    bool bTransactionActive = false;

    FName PreviewVariant = NAME_None;
};
