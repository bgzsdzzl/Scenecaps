// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorToolkit.cpp
#include "ScenecapsEditorToolkit.h"

#include "ScenecapsEditorViewport.h"
#include "ScenecapsEditorViewportClient.h"
#include "ScenecapsEditorOutliner.h"
#include "ScenecapsBatchEdit.h"
#include "ScenecapsEditorModule.h"
#include "ScenecapsLocalization.h"

#include "DragAndDrop/AssetDragDropOp.h"
#include "Engine/World.h"
#include "Framework/Docking/TabManager.h"
#include "GameFramework/Actor.h"
#include "IDetailsView.h"
#include "IStructureDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "SAssetDropTarget.h"
#include "EditorViewportClient.h"
#include "UObject/StructOnScope.h"
#include "Components/PrimitiveComponent.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ScopedTransaction.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "Scenecaps"

const FName FScenecapsEditorToolkit::ViewportTabId(TEXT("ScenecapsEditor_Viewport"));
const FName FScenecapsEditorToolkit::OutlinerTabId(TEXT("ScenecapsEditor_Outliner"));
const FName FScenecapsEditorToolkit::DetailsTabId(TEXT("ScenecapsEditor_Details"));

void FScenecapsEditorToolkit::InitEditor(const EToolkitMode::Type Mode,
    const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* InAsset)
{
    AssetProxy = MakeEditorAssetProxy(InAsset);
    if (!AssetProxy.IsValid()) return;

    FVector SavedLoc; FRotator SavedRot;
    if (AssetProxy->GetSavedEditorView(SavedLoc, SavedRot))
    {
        CachedEditorViewLocation = SavedLoc;
        CachedEditorViewRotation = SavedRot;
    }

    FPreviewScene::ConstructionValues CVS;
    CVS.bAllowAudioPlayback = false;
    CVS.bCreatePhysicsScene = true;
    CVS.LightBrightness = 3.14f;
    CVS.SkyBrightness = 1.0f;
    PreviewScene = MakeUnique<FPreviewScene>(CVS);
    PreviewScene->SetLightDirection(FRotator(-45.f, -30.f, 0.f));
    PreviewScene->SetLightColor(FColor::White);

    const TSharedRef<FTabManager::FLayout> Layout =
        FTabManager::NewLayout("ScenecapsEditor_Layout_v1")
        ->AddArea
        (
            FTabManager::NewPrimaryArea()
            ->SetOrientation(Orient_Horizontal)
            ->Split(FTabManager::NewStack()->SetSizeCoefficient(0.20f)->AddTab(OutlinerTabId, ETabState::OpenedTab))
            ->Split(FTabManager::NewStack()->SetSizeCoefficient(0.55f)->AddTab(ViewportTabId, ETabState::OpenedTab))
            ->Split(FTabManager::NewStack()->SetSizeCoefficient(0.25f)->AddTab(DetailsTabId, ETabState::OpenedTab))
        );

    InitAssetEditor(Mode, InitToolkitHost, TEXT("ScenecapsEditorApp"), Layout, true, true, InAsset);

    if (FScenecapsEditorModule* M = FModuleManager::GetModulePtr<FScenecapsEditorModule>("ScenecapsEditor"))
        M->RegisterToolkit(SharedThis(this));

    RebuildPreviewScene();
}

FName        FScenecapsEditorToolkit::GetToolkitFName() const { return FName(TEXT("ScenecapsEditor")); }
FText        FScenecapsEditorToolkit::GetBaseToolkitName() const { return SC_LOCTEXT(ToolkitName, "Scenecaps Editor"); }
FString      FScenecapsEditorToolkit::GetWorldCentricTabPrefix() const { return TEXT("Scenecaps "); }
FLinearColor FScenecapsEditorToolkit::GetWorldCentricTabColorScale() const { return FLinearColor(0.15f, 0.55f, 0.9f, 0.5f); }

void FScenecapsEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
    InTabManager->RegisterTabSpawner(ViewportTabId, FOnSpawnTab::CreateSP(this, &FScenecapsEditorToolkit::SpawnTab_Viewport))
        .SetDisplayName(SC_LOCTEXT(ViewportTabLabel, "Viewport"));
    InTabManager->RegisterTabSpawner(OutlinerTabId, FOnSpawnTab::CreateSP(this, &FScenecapsEditorToolkit::SpawnTab_Outliner))
        .SetDisplayName(SC_LOCTEXT(OutlinerTabLabel, "Outliner"));
    InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FScenecapsEditorToolkit::SpawnTab_Details))
        .SetDisplayName(SC_LOCTEXT(DetailsTabLabel, "Details"));
}

void FScenecapsEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
    InTabManager->UnregisterTabSpawner(ViewportTabId);
    InTabManager->UnregisterTabSpawner(OutlinerTabId);
    InTabManager->UnregisterTabSpawner(DetailsTabId);
}

TSharedRef<SDockTab> FScenecapsEditorToolkit::SpawnTab_Viewport(const FSpawnTabArgs&)
{
    if (!AssetProxy.IsValid())
    {
        return MakeTab(ViewportTabId, SC_LOCTEXT(ViewportTabLabel, "Viewport"), SNew(STextBlock).Text(SC_LOCTEXT(InvalidAsset, "Invalid Asset")));
    }

    TSharedRef<SScenecapsEditorViewport> Viewport = SNew(SScenecapsEditorViewport)
        .Toolkit(SharedThis(this));
    ViewportWidget = Viewport;

    TSharedRef<SAssetDropTarget> DropTarget =
        SNew(SAssetDropTarget)
        .bSupportsMultiDrop(true)
        .OnAssetsDropped_Lambda([this](const FDragDropEvent& E, TArrayView<FAssetData> InAssets)
            { OnAssetsDropped(E, InAssets); })
        [Viewport];

    return MakeTab(ViewportTabId, SC_LOCTEXT(ViewportTabLabel, "Viewport"), DropTarget);
}

TSharedRef<SDockTab> FScenecapsEditorToolkit::SpawnTab_Outliner(const FSpawnTabArgs&)
{
    if (!AssetProxy.IsValid())
    {
        return MakeTab(OutlinerTabId, SC_LOCTEXT(OutlinerTabLabel, "Outliner"), SNew(STextBlock).Text(SC_LOCTEXT(InvalidAsset, "Invalid Asset")));
    }
    TSharedRef<SScenecapsEditorOutliner> Outliner = SNew(SScenecapsEditorOutliner).Toolkit(SharedThis(this));
    OutlinerWidget = Outliner;
    return MakeTab(OutlinerTabId, SC_LOCTEXT(OutlinerTabLabel, "Outliner"), Outliner);
}

TSharedRef<SDockTab> FScenecapsEditorToolkit::SpawnTab_Details(const FSpawnTabArgs&)
{
    if (!AssetProxy.IsValid())
    {
        return MakeTab(DetailsTabId, SC_LOCTEXT(DetailsTabLabel, "Details"), SNew(STextBlock).Text(SC_LOCTEXT(InvalidAsset, "Invalid Asset")));
    }
    FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs DetailsArgs;
    DetailsArgs.bAllowSearch = true;
    DetailsArgs.bHideSelectionTip = true;
    DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    FStructureDetailsViewArgs StructDetailsArgs;
    TSharedRef<IStructureDetailsView> StructDetails = PropModule.CreateStructureDetailView(DetailsArgs, StructDetailsArgs, nullptr);
    StructDetails->GetOnFinishedChangingPropertiesDelegate().AddSP(this, &FScenecapsEditorToolkit::OnDetailsPropertyCommitted);
    StructureDetailsView = StructDetails;
    DetailsEntryWidget = StructDetails->GetWidget();

    TSharedRef<IDetailsView> AssetView = PropModule.CreateDetailView(DetailsArgs);
    AssetView->OnFinishedChangingProperties().AddSP(this, &FScenecapsEditorToolkit::OnAssetPropertyCommitted);
    AssetView->SetObject(GetEditingAssetObject());
    AssetDetailsView = AssetView;
    DetailsAssetWidget = StaticCastSharedRef<SWidget>(AssetView);

    SAssignNew(DetailsHost, SBox)[DetailsAssetWidget.ToSharedRef()];
    return MakeTab(DetailsTabId, SC_LOCTEXT(DetailsTabLabel, "Details"), DetailsHost.ToSharedRef());
}

TSharedRef<SDockTab> FScenecapsEditorToolkit::MakeTab(const FName&, const FText& Label, TSharedRef<SWidget> Content)
{
    return SNew(SDockTab).TabRole(ETabRole::PanelTab).Label(Label)[Content];
}

void FScenecapsEditorToolkit::SetPreviewVariant(FName InVariant)
{
    if (PreviewVariant == InVariant) return;
    PreviewVariant = InVariant;
    RebuildPreviewScene();
    RefreshOutliner();
    RequestRedraw();
}

void FScenecapsEditorToolkit::OnAssetsDropped(const FDragDropEvent& DragDropEvent, TArrayView<FAssetData> InAssets)
{
    if (!AssetProxy.IsValid() || !PreviewScene.IsValid() || InAssets.Num() == 0) return;

    const FVector2D ScreenPos = DragDropEvent.GetScreenSpacePosition();
    FVector WorldLocation = FVector::ZeroVector;
    FRotator WorldRotation = FRotator::ZeroRotator;

    if (TSharedPtr<SScenecapsEditorViewport> Viewport = ViewportWidget.Pin())
    {
        if (TSharedPtr<FScenecapsEditorViewportClient> Client = Viewport->GetViewportClient())
        {
            FViewport* VP = Client->Viewport;
            if (VP)
            {
                const FGeometry& ViewportGeometry = Viewport->GetTickSpaceGeometry();
                const FVector2D LocalPos = ViewportGeometry.AbsoluteToLocal(ScreenPos) * ViewportGeometry.Scale;

                FSceneViewFamily::ConstructionValues CVS(VP, PreviewScene->GetWorld()->Scene, *Client->GetEngineShowFlagsPublic());
                FSceneViewFamilyContext ViewFamily(CVS);
                FSceneView* View = Client->CalcSceneView(&ViewFamily);

                if (View)
                {
                    FVector RayOrigin, RayDirection;
                    View->DeprojectFVector2D(LocalPos, RayOrigin, RayDirection);

                    FHitResult HitResult;
                    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ScenecapsDrop), true);
                    if (PreviewScene->GetWorld()->LineTraceSingleByChannel(HitResult, RayOrigin,
                        RayOrigin + RayDirection * 100000.f, ECC_Visibility, QueryParams))
                    {
                        WorldLocation = HitResult.Location;
                    }
                    else
                    {
                        const FPlane GroundPlane(FVector::ZeroVector, FVector::UpVector);
                        WorldLocation = FMath::RayPlaneIntersection(RayOrigin, RayDirection, GroundPlane);
                    }
                }
            }
        }
    }

    for (const FAssetData& AssetData : InAssets)
        if (UObject* Loaded = AssetData.GetAsset())
            AddEntryFromDroppedObject(Loaded, WorldLocation, WorldRotation);
}

void FScenecapsEditorToolkit::AddEntryFromDroppedObject(UObject* DroppedObject, const FVector& WorldLocation, const FRotator& WorldRotation)
{
    if (!AssetProxy.IsValid() || !DroppedObject) return;

    ShowAssetDetails();

    const FTransform Xform(WorldRotation, WorldLocation);
    AssetProxy->Modify();
    const FGuid NewId = AssetProxy->AddEntryFromObject(DroppedObject, Xform, AssetProxy->GetRootId());
    if (NewId.IsValid())
    {
        RebuildPreviewScene();
        RefreshOutliner();
    }
}

void FScenecapsEditorToolkit::ClearPreviewScene()
{
    for (const TPair<FGuid, TWeakObjectPtr<AActor>>& Pair : PreviewActors)
        if (AActor* Actor = Pair.Value.Get()) Actor->Destroy();
    PreviewActors.Empty();
}

void FScenecapsEditorToolkit::RebuildPreviewScene()
{
    ClearPreviewScene();
    if (!AssetProxy.IsValid() || !PreviewScene.IsValid()) return;
    UWorld* World = PreviewScene->GetWorld();
    if (!World) return;

    const FGuid RootId = AssetProxy->GetRootId();
    if (!RootId.IsValid()) return;

    TSet<FGuid> Spawned;

    TFunction<void(const FGuid&, const FTransform&)> SpawnRecursive =
        [&](const FGuid& EntryId, const FTransform& ParentWorld)
        {
            if (Spawned.Contains(EntryId)) return;
            if (!AssetProxy->HasEntry(EntryId)) return;

            const FScenecapsEditorEntryView Eff = AssetProxy->GetEffectiveView(EntryId, PreviewVariant);
            if (!Eff.EntryId.IsValid()) return;
            
            const FTransform FinalWorld = Eff.RelativeTransform * ParentWorld;

            AActor* Actor = SpawnPreviewEntry(Eff, World);
            if (!Actor) return;

            Actor->SetActorTransform(FinalWorld);
            Actor->SetIsTemporarilyHiddenInEditor(!Eff.bVisible);

            TInlineComponentArray<UPrimitiveComponent*> Prims;
            Actor->GetComponents(Prims);
            for (UPrimitiveComponent* P : Prims) if (P) P->SetVisibility(Eff.bVisible);

            PreviewActors.Add(EntryId, Actor);
            Spawned.Add(EntryId);

            TArray<FGuid> Children;
            AssetProxy->GetEntryChildren(EntryId, Children);
            for (const FGuid& Child : Children) SpawnRecursive(Child, FinalWorld);
        };

    TArray<FGuid> Roots;
    AssetProxy->GetEntryChildren(AssetProxy->GetRootId(), Roots);
    for (const FGuid& Root : Roots) SpawnRecursive(Root, FTransform::Identity);

    AssetProxy->ForEachEntry([&](const FScenecapsEditorEntryView& V)
        {
            if (!Spawned.Contains(V.EntryId)) SpawnRecursive(V.EntryId, FTransform::Identity);
        });

    AssetProxy->ForEachEntry([&](const FScenecapsEditorEntryView& V)
        {
            AActor* Child = PreviewActors.FindRef(V.EntryId).Get();
            AActor* Parent = PreviewActors.FindRef(V.ParentId).Get();
            if (Child && Parent && Child != Parent)
                Child->AttachToActor(Parent, FAttachmentTransformRules::KeepWorldTransform);
        });

    RequestRedraw();
}

AActor* FScenecapsEditorToolkit::SpawnPreviewEntry(const FScenecapsEditorEntryView& Effective, UWorld* World)
{
    return AssetProxy.IsValid() ? AssetProxy->SpawnPreviewEntry(Effective, World) : nullptr;
}

void FScenecapsEditorToolkit::RequestRedraw()
{
    if (TSharedPtr<SScenecapsEditorViewport> Viewport = ViewportWidget.Pin())
        if (TSharedPtr<FScenecapsEditorViewportClient> Client = Viewport->GetViewportClient())
            Client->RequestRedraw();
}

void FScenecapsEditorToolkit::RefreshLocalizedUI()
{
    if (TSharedPtr<FTabManager> TM = GetTabManager())
    {
        if (auto Tab = TM->FindExistingLiveTab(ViewportTabId)) Tab->SetLabel(SC_LOCTEXT(ViewportTabLabel, "Viewport"));
        if (auto Tab = TM->FindExistingLiveTab(OutlinerTabId)) Tab->SetLabel(SC_LOCTEXT(OutlinerTabLabel, "Outliner"));
        if (auto Tab = TM->FindExistingLiveTab(DetailsTabId)) Tab->SetLabel(SC_LOCTEXT(DetailsTabLabel, "Details"));
    }
    if (TSharedPtr<SScenecapsEditorOutliner> Outliner = OutlinerWidget.Pin())
        Outliner->RefreshTree();
}

void FScenecapsEditorToolkit::RefreshOutliner()
{
    if (TSharedPtr<SScenecapsEditorOutliner> Outliner = OutlinerWidget.Pin())
        Outliner->RefreshTree();

    if (DetailsEntryIds.Num() > 0)
    {
        TArray<FGuid> Valid;
        for (const FGuid& Id : DetailsEntryIds)
            if (AssetProxy.IsValid() && AssetProxy->HasEntry(Id)) Valid.Add(Id);
        if (Valid.Num() > 0) ShowEntriesDetails(Valid);
        else ShowAssetDetails();
    }
}

void FScenecapsEditorToolkit::SelectPreviewActor(const FGuid& EntryId)
{
    AActor* Actor = PreviewActors.FindRef(EntryId).Get();
    ShowEntriesDetails({ EntryId });
    if (!Actor) return;
    if (TSharedPtr<SScenecapsEditorViewport> Viewport = ViewportWidget.Pin())
    {
        if (TSharedPtr<FScenecapsEditorViewportClient> Client = Viewport->GetViewportClient())
        {
            Client->SetSelectedActors({ Actor });
            Client->FocusViewportOnBox(Actor->GetComponentsBoundingBox(true), true);
            Client->RequestRedraw();
        }
    }
}

void FScenecapsEditorToolkit::UpdatePreviewActorVisibility(const FGuid& EntryId, bool) { UpdatePreviewActor(EntryId); }

void FScenecapsEditorToolkit::UpdatePreviewActor(const FGuid& EntryId)
{
    if (!AssetProxy.IsValid()) return;
    AActor* PreviewActor = PreviewActors.FindRef(EntryId).Get();
    if (!PreviewActor) return;

    FScenecapsEditorEntryView V;
    if (!AssetProxy->GetEntryView(EntryId, V)) return;

    FTransform ParentWorld = FTransform::Identity;
    if (AActor* ParentActor = PreviewActors.FindRef(V.ParentId).Get())
        ParentWorld = ParentActor->GetActorTransform();

    const FScenecapsEditorEntryView Eff = AssetProxy->GetEffectiveView(EntryId, PreviewVariant);

    PreviewActor->SetActorTransform(Eff.RelativeTransform * ParentWorld);
    PreviewActor->SetIsTemporarilyHiddenInEditor(!Eff.bVisible);

    TInlineComponentArray<UPrimitiveComponent*> Prims;
    PreviewActor->GetComponents(Prims);
    for (UPrimitiveComponent* P : Prims)
    {
        if (!P) continue;
        P->SetVisibility(Eff.bVisible);
        if (UMeshComponent* Mesh = Cast<UMeshComponent>(P))
        {
            UMaterialInterface* Mat = Eff.MaterialOverride.LoadSynchronous();
            if (Mat)
            {
                if (Eff.MaterialSlotIndex < 0)
                {
                    int32 N = Mesh->GetNumMaterials();
                    for (int32 i = 0; i < N; ++i) Mesh->SetMaterial(i, Mat);
                }
                else Mesh->SetMaterial(Eff.MaterialSlotIndex, Mat);
            }
        }
    }
    RequestRedraw();
}

void FScenecapsEditorToolkit::ReparentEntry(const FGuid& ChildId, const FGuid& NewParentId)
{
    ReparentEntries({ ChildId }, NewParentId);
}

void FScenecapsEditorToolkit::ReparentEntries(const TArray<FGuid>& ChildIds, const FGuid& NewParentId)
{
    if (!AssetProxy.IsValid() || ChildIds.Num() == 0) return;

    FTransform NewParentWorld = FTransform::Identity;
    if (NewParentId != AssetProxy->GetRootId())
        if (AActor* ParentActor = PreviewActors.FindRef(NewParentId).Get())
            NewParentWorld = ParentActor->GetActorTransform();

    AssetProxy->Modify();
    for (const FGuid& ChildId : ChildIds)
    {
        if (ChildId == NewParentId) continue;
        FScenecapsEditorEntryView V;
        if (!AssetProxy->GetEntryView(ChildId, V)) continue;
        if (V.bLocked) continue;

        FTransform ChildWorld = V.RelativeTransform;
        if (AActor* ChildActor = PreviewActors.FindRef(ChildId).Get())
            ChildWorld = ChildActor->GetActorTransform();

        AssetProxy->SetEntryParent(ChildId, NewParentId);
        AssetProxy->SetEntryRelativeTransform(ChildId, ChildWorld.GetRelativeTransform(NewParentWorld));
    }
    AssetProxy->MarkDirty();
    RebuildPreviewScene();
    RefreshOutliner();
}

void FScenecapsEditorToolkit::HandleViewportActorSelected(AActor* Actor)
{
    if (!Actor || !AssetProxy.IsValid()) return;
    for (const TPair<FGuid, TWeakObjectPtr<AActor>>& Pair : PreviewActors)
    {
        if (Pair.Value.Get() == Actor)
        {
            FScenecapsEditorEntryView V;
            if (AssetProxy->GetEntryView(Pair.Key, V) && V.bLocked) { ShowAssetDetails(); return; }
            if (TSharedPtr<SScenecapsEditorOutliner> Outliner = OutlinerWidget.Pin())
                Outliner->SelectEntry(Pair.Key);
            ShowEntriesDetails({ Pair.Key });
            return;
        }
    }
}

void FScenecapsEditorToolkit::OnDetailsPropertyCommitted(const FPropertyChangedEvent&)
{
    if (!AssetProxy.IsValid()) return;

    if (DetailsEntryIds.Num() > 1 && BatchDetailsScope.IsValid())
    {
        FScenecapsBatchEdit* Data = reinterpret_cast<FScenecapsBatchEdit*>(BatchDetailsScope->GetStructMemory());
        if (!Data) return;

        const FScopedTransaction Transaction(SC_LOCTEXT(EditEntriesTx, "Edit Scene Entries"));
        AssetProxy->Modify();
        bool bChanged = false;
        for (const FGuid& Id : DetailsEntryIds)
        {
            FScenecapsEditorEntryView V;
            if (!AssetProxy->GetEntryView(Id, V) || V.bLocked) continue;
            if (Data->bApplyVisible && V.bVisible != Data->Visible) { AssetProxy->SetEntryVisible(Id, Data->Visible); bChanged = true; }
            if (Data->bApplyTransform) { AssetProxy->SetEntryRelativeTransform(Id, Data->RelativeTransform); bChanged = true; }
        }
        if (bChanged) AssetProxy->MarkDirty();
        RebuildPreviewScene();
        RefreshOutliner();
        RequestRedraw();
        return;
    }

    if (DetailsEntryIds.Num() == 0) return;

    FScenecapsEditorEntryView Current;
    if (!AssetProxy->GetEntryView(DetailsEntryIds[0], Current) || Current.bLocked) return;

    if (DetailsStructScope.IsValid())
    {
        const FScenecapsEditorEntryView* Edited =
            reinterpret_cast<const FScenecapsEditorEntryView*>(DetailsStructScope->GetStructMemory());
        if (Edited)
        {
            const FScopedTransaction Transaction(SC_LOCTEXT(EditEntryTx, "Edit Scene Entry"));
            AssetProxy->Modify();
            AssetProxy->ApplyViewToEntry(DetailsEntryIds[0], *Edited);
            AssetProxy->MarkDirty();
            UpdatePreviewActor(DetailsEntryIds[0]);
            RequestRedraw();
        }
    }
}

void FScenecapsEditorToolkit::OnAssetPropertyCommitted(const FPropertyChangedEvent& Event)
{
    if (!AssetProxy.IsValid()) return;
    AssetProxy->MarkDirty();
    const FName PropName = Event.GetPropertyName();
    if (PropName.IsNone() ||
        PropName == FName(TEXT("Entries")) ||
        PropName == FName(TEXT("Variants")))
    {
        RebuildPreviewScene();
        RefreshOutliner();
        RequestRedraw();
    }
    else
    {
        RefreshOutliner();
    }
}

void FScenecapsEditorToolkit::ShowEntryDetails(FGuid EntryId) { ShowEntriesDetails({ EntryId }); }

void FScenecapsEditorToolkit::ShowEntriesDetails(const TArray<FGuid>& EntryIds)
{
    TSharedPtr<IStructureDetailsView> Details = StructureDetailsView.Pin();
    if (!AssetProxy.IsValid() || !Details.IsValid()) return;

    if (EntryIds.Num() == 0) { ShowAssetDetails(); return; }

    if (EntryIds.Num() == 1)
    {
        FScenecapsEditorEntryView V;
        if (AssetProxy->GetEntryView(EntryIds[0], V))
        {
            DetailsEntryIds = EntryIds;
            BatchDetailsScope.Reset();
            DetailsStructScope = MakeShared<FStructOnScope>(FScenecapsEditorEntryView::StaticStruct());
            FMemory::Memcpy(DetailsStructScope->GetStructMemory(), &V, sizeof(FScenecapsEditorEntryView));
            Details->SetStructureData(DetailsStructScope);
        }
        else { ShowAssetDetails(); return; }
    }
    else
    {
        DetailsEntryIds = EntryIds;
        DetailsStructScope.Reset();
        BatchDetailsScope = MakeShared<FStructOnScope>(FScenecapsBatchEdit::StaticStruct());
        Details->SetStructureData(BatchDetailsScope);
    }

    if (DetailsHost.IsValid() && DetailsEntryWidget.IsValid())
        DetailsHost->SetContent(DetailsEntryWidget.ToSharedRef());
}

void FScenecapsEditorToolkit::ShowAssetDetails()
{
    DetailsEntryIds.Reset();
    DetailsStructScope.Reset();
    BatchDetailsScope.Reset();

    if (TSharedPtr<IStructureDetailsView> D = StructureDetailsView.Pin()) D->SetStructureData(nullptr);
    if (TSharedPtr<IDetailsView> V = AssetDetailsView.Pin()) V->SetObject(GetEditingAssetObject());
    if (DetailsHost.IsValid() && DetailsAssetWidget.IsValid()) DetailsHost->SetContent(DetailsAssetWidget.ToSharedRef());
}

void FScenecapsEditorToolkit::UpdateCachedEditorView(const FVector& Loc, const FRotator& Rot)
{
    if (!CachedEditorViewLocation.Equals(Loc, 1.0f) || !CachedEditorViewRotation.Equals(Rot, 0.1f))
    {
        CachedEditorViewLocation = Loc;
        CachedEditorViewRotation = Rot;
        bEditorViewDirty = true;
    }
}

void FScenecapsEditorToolkit::SaveEditorViewToAsset()
{
    if (!bEditorViewDirty || !AssetProxy.IsValid()) return;
    AssetProxy->SaveEditorView(CachedEditorViewLocation, CachedEditorViewRotation);
    bEditorViewDirty = false;
}

bool FScenecapsEditorToolkit::OnRequestClose(EAssetEditorCloseReason CloseReason)
{
    SaveEditorViewToAsset();
    return FAssetEditorToolkit::OnRequestClose(CloseReason);
}

void FScenecapsEditorToolkit::DeleteSelectedViewportActors(const TArray<TWeakObjectPtr<AActor>>& ActorsToDelete)
{
    if (!AssetProxy.IsValid()) return;

    TSet<FGuid> ToRemove;
    for (const TWeakObjectPtr<AActor>& Weak : ActorsToDelete)
    {
        if (AActor* Actor = Weak.Get())
        {
            for (const auto& Pair : PreviewActors)
            {
                if (Pair.Value.Get() == Actor)
                {
                    TFunction<void(const FGuid&)> Rec = [&](const FGuid& Id)
                        {
                            ToRemove.Add(Id);
                            TArray<FGuid> Ch; AssetProxy->GetEntryChildren(Id, Ch);
                            for (const FGuid& C : Ch) Rec(C);
                        };
                    Rec(Pair.Key);
                    break;
                }
            }
        }
    }

    if (ToRemove.Num() == 0) return;

    for (const FGuid& Id : ToRemove)
    {
        FScenecapsEditorEntryView V;
        if (AssetProxy->GetEntryView(Id, V) && V.bLocked) return;
    }

    AssetProxy->Modify();
    AssetProxy->DeleteEntries(ToRemove);
    AssetProxy->MarkDirty();
    RebuildPreviewScene();
    RefreshOutliner();
    ShowAssetDetails();
}

void FScenecapsEditorToolkit::BeginTransaction()
{
    if (bTransactionActive || !AssetProxy.IsValid() || !GEditor) return;
    GEditor->BeginTransaction(SC_LOCTEXT(MoveActorTx, "Move Scenecaps Actor"));
    AssetProxy->Modify();
    bTransactionActive = true;
}

void FScenecapsEditorToolkit::EndTransactionAndUpdateTransforms(const TArray<TWeakObjectPtr<AActor>>& ModifiedActors)
{
    if (AssetProxy.IsValid())
    {
        bool bChanged = false;
        for (const TWeakObjectPtr<AActor>& Weak : ModifiedActors)
        {
            if (AActor* Actor = Weak.Get())
            {
                for (const auto& Pair : PreviewActors)
                {
                    if (Pair.Value.Get() == Actor)
                    {
                        FScenecapsEditorEntryView V;
                        if (AssetProxy->GetEntryView(Pair.Key, V) && !V.bLocked)
                        {
                            FTransform ParentWorld = FTransform::Identity;
                            if (AActor* ParentActor = PreviewActors.FindRef(V.ParentId).Get())
                                ParentWorld = ParentActor->GetActorTransform();
                            AssetProxy->SetEntryRelativeTransform(Pair.Key,
                                Actor->GetActorTransform().GetRelativeTransform(ParentWorld));
                            bChanged = true;
                        }
                        break;
                    }
                }
            }
        }
        if (bChanged)
        {
            AssetProxy->MarkDirty();
            if (TSharedPtr<SScenecapsEditorOutliner> Outliner = OutlinerWidget.Pin())
                Outliner->RefreshTree();
            if (DetailsEntryIds.Num() > 0) ShowEntriesDetails(DetailsEntryIds);
        }
    }

    if (bTransactionActive && GEditor) GEditor->EndTransaction();
    bTransactionActive = false;
}

#undef LOCTEXT_NAMESPACE