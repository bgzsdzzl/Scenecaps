// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorToolkit.cpp

#include "ScenecapsEditorToolkit.h"

#include "ScenecapsEditorViewport.h"
#include "ScenecapsEditorViewportClient.h"
#include "ScenecapsBatchEdit.h"
#include "ScenecapsEditorModule.h"
#include "ScenecapsLocalization.h"
#include "ScenecapsViewportConfig.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Engine/TextureCube.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SMenuAnchor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "UObject/Package.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/SkyLight.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
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
#include "ScopedTransaction.h"
#include "Editor.h"
#include "AssetRegistry/AssetData.h"
#include "SceneView.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInterface.h"

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
    CVS.bDefaultLighting = false;  // Disable default lighting completely
    CVS.LightBrightness = 0.0f;
    CVS.SkyBrightness = 0.0f;
    PreviewScene = MakeUnique<FPreviewScene>(CVS);
    PreviewScene->SetLightDirection(FRotator(-45.f, -30.f, 0.f));
    PreviewScene->SetLightColor(FColor::White);

    // Aggressively disable all global lighting - no sky, no directional light, no sky light, no sky atmosphere
    if (UWorld* World = PreviewScene->GetWorld())
    {
        // Disable Sky Light completely
        if (USkyLightComponent* SkyLight = PreviewScene->SkyLight)
        {
            SkyLight->SetIntensity(0.0f);
            SkyLight->SetVisibility(false);
        }

        // Disable Directional Light completely
        if (UDirectionalLightComponent* LightComp = PreviewScene->DirectionalLight)
        {
            LightComp->SetIntensity(0.0f);
            LightComp->SetVisibility(false);
        }

        // Disable Sky Atmosphere if any
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            AActor* Actor = *It;
            if (Actor && (Actor->IsA(ASkyLight::StaticClass()) || Actor->IsA(ADirectionalLight::StaticClass()) || Actor->IsA(ASkyAtmosphere::StaticClass())))
            {
                Actor->Destroy();
            }
        }
    }

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
    ApplyHDRISkyboxToPreview();
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

    // 视口顶部工具条：HDRI 天空盒默认关闭，必须手动选择 HDRI 贴图后才会启用
    TSharedRef<SWidget> HDRIToolbar =
        SNew(SBorder)
        .Padding(FMargin(4.f, 3.f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([this]() -> ECheckBoxState
                    {
                        return IsHDRISkyboxEnabled() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                    })
                .OnCheckStateChanged_Lambda([this](ECheckBoxState State)
                    {
                        if (State == ECheckBoxState::Checked && !GetHDRISkyboxTexture())
                        {
                            // 尚未选择 HDRI：打开选择器而不启用
                            if (HDRIPickerAnchor.IsValid()) HDRIPickerAnchor->SetIsOpen(true);
                            return;
                        }
                        SetHDRISkyboxEnabled(State == ECheckBoxState::Checked);
                    })
                [
                    SNew(STextBlock).Text(SC_LOCTEXT(HDRIEnableLabel, "HDRI Skybox"))
                ]
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f, 0.f, 0.f)
            [
                SAssignNew(HDRIPickerAnchor, SMenuAnchor)
                .Placement(MenuPlacement_BelowAnchor)
                .OnGetMenuContent_Lambda([this]() { return MakeHDRIAssetPicker(); })
                [
                    SNew(SButton)
                    .OnClicked_Lambda([this]() -> FReply
                        {
                            if (HDRIPickerAnchor.IsValid()) HDRIPickerAnchor->SetIsOpen(true);
                            return FReply::Handled();
                        })
                    [
                        SNew(STextBlock).Text(SC_LOCTEXT(HDRISelectLabel, "Select HDRI..."))
                    ]
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1.f)
        ];

    TSharedRef<SWidget> ViewportWithToolbar =
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight() [ HDRIToolbar ]
        + SVerticalBox::Slot().FillHeight(1.f) [ Viewport ];

    TSharedRef<SAssetDropTarget> DropTarget =
        SNew(SAssetDropTarget)
        .bSupportsMultiDrop(true)
        .OnAssetsDropped_Lambda([this](const FDragDropEvent& E, TArrayView<FAssetData> InAssets)
            { OnAssetsDropped(E, InAssets); })
        [ViewportWithToolbar];

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

// ---------- 视口 HDRI 天空盒 ----------

UScenecapsViewportSettings* FScenecapsEditorToolkit::GetViewportSettings() const
{
    return GetMutableDefault<UScenecapsViewportSettings>();
}

bool FScenecapsEditorToolkit::IsHDRISkyboxEnabled() const
{
    const UScenecapsViewportSettings* Settings = GetViewportSettings();
    return Settings->bEnableHDRISkybox && !Settings->HDRISkyboxTexture.IsNull();
}

UTextureCube* FScenecapsEditorToolkit::GetHDRISkyboxTexture() const
{
    return GetViewportSettings()->HDRISkyboxTexture.LoadSynchronous();
}

void FScenecapsEditorToolkit::SetHDRISkyboxEnabled(bool bEnabled)
{
    UScenecapsViewportSettings* Settings = GetViewportSettings();
    Settings->bEnableHDRISkybox = bEnabled && !Settings->HDRISkyboxTexture.IsNull();
    Settings->SaveConfig();
    ApplyHDRISkyboxToPreview();
}

void FScenecapsEditorToolkit::SetHDRISkyboxTexture(UTextureCube* InTexture)
{
    UScenecapsViewportSettings* Settings = GetViewportSettings();
    Settings->HDRISkyboxTexture = InTexture;

    // 没有 HDRI 时强制关闭；选择有效 HDRI 后自动启用
    Settings->bEnableHDRISkybox = (InTexture != nullptr);
    Settings->SaveConfig();
    ApplyHDRISkyboxToPreview();
}

void FScenecapsEditorToolkit::EnsureHDRISkySphere()
{
    if (!PreviewScene.IsValid() || HDRISkySphere.IsValid()) return;

    // 与引擎 FAdvancedPreviewScene 一致：反法线球 + bIsSky 材质，使 HDRI 作为可见天空背景
    UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Engine/EditorMeshes/AssetViewer/Sphere_inversenormals.Sphere_inversenormals"));
    UMaterial* SkyMaterial = LoadObject<UMaterial>(nullptr,
        TEXT("/Engine/EditorMaterials/AssetViewer/M_SkyBox.M_SkyBox"));
    if (!SphereMesh || !SkyMaterial) return;

    UMaterialInstanceConstant* MaterialInstance = NewObject<UMaterialInstanceConstant>(GetTransientPackage());
    MaterialInstance->Parent = SkyMaterial;
    MaterialInstance->PostLoad();

    UStaticMeshComponent* SphereComponent = NewObject<UStaticMeshComponent>(GetTransientPackage());
    SphereComponent->SetStaticMesh(SphereMesh);
    SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SphereComponent->CastShadow = false;
    SphereComponent->bCastDynamicShadow = false;
    SphereComponent->SetMaterial(0, MaterialInstance);

    // 原点、2000 倍缩放，与引擎资产查看器一致
    const FTransform SphereTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(2000));
    PreviewScene->AddComponent(SphereComponent, SphereTransform);
    SphereComponent->SetVisibility(false, true);

    HDRISkySphere = SphereComponent;
    HDRISkyMaterialInstance = MaterialInstance;
}

void FScenecapsEditorToolkit::ApplyHDRISkyboxToPreview()
{
    const UScenecapsViewportSettings* Settings = GetViewportSettings();
    const bool bEnabled = IsHDRISkyboxEnabled();
    UTextureCube* HDRI = bEnabled ? Settings->HDRISkyboxTexture.LoadSynchronous() : nullptr;

    if (!PreviewScene.IsValid()) return;

    EnsureHDRISkySphere();

    // 可见天空背景：通过天空球材质的 cubemap 参数切换，而非 SkyLighting show flag
    UMaterialInstanceConstant* SkyMat = HDRISkyMaterialInstance.Get();
    if (SkyMat)
    {
        static const FName SkyBoxParam(TEXT("SkyBox"));
        UTexture* CurrentTexture = nullptr;
        SkyMat->GetTextureParameterValue(SkyBoxParam, CurrentTexture);
        if (CurrentTexture != static_cast<UTexture*>(HDRI))
        {
            SkyMat->SetTextureParameterValueEditorOnly(SkyBoxParam, HDRI);
            SkyMat->SetScalarParameterValueEditorOnly(FName(TEXT("CubemapRotation")), 0.0f);
            SkyMat->SetScalarParameterValueEditorOnly(FName(TEXT("Intensity")), Settings->HDRISkyboxIntensity);
            SkyMat->PostEditChange();
        }
    }

    UStaticMeshComponent* SkySphere = HDRISkySphere.Get();
    if (SkySphere)
    {
        SkySphere->SetVisibility(HDRI != nullptr, true);
    }

    // IBL 照明：bDefaultLighting=false 时预览场景没有 SkyLight，需要自建一个
    if (!PreviewScene->SkyLight)
    {
        USkyLightComponent* NewSkyLight = NewObject<USkyLightComponent>(GetTransientPackage(), NAME_None, RF_Transient);
        NewSkyLight->Mobility = EComponentMobility::Movable;
        NewSkyLight->bLowerHemisphereIsBlack = false;
        NewSkyLight->SourceType = SLS_SpecifiedCubemap;
        NewSkyLight->Intensity = 0.0f;
        PreviewScene->SkyLight = NewSkyLight;
        PreviewScene->AddComponent(NewSkyLight, FTransform::Identity);
    }

    if (USkyLightComponent* SkyLight = PreviewScene->SkyLight)
    {
        SkyLight->SetVisibility(HDRI != nullptr);
        SkyLight->SetIntensity(HDRI ? Settings->HDRISkyboxIntensity : 0.0f);
        SkyLight->SourceType = SLS_SpecifiedCubemap;
        if (HDRI) SkyLight->SetCubemap(HDRI);
        SkyLight->SetCaptureIsDirty();
        SkyLight->MarkRenderStateDirty();
    }

    // 驱动一次天空捕获，避免 capture 长时间处于 dirty/未初始化状态导致帧间闪烁
    PreviewScene->UpdateCaptureContents();

    if (TSharedPtr<SScenecapsEditorViewport> Viewport = ViewportWidget.Pin())
    {
        if (TSharedPtr<FScenecapsEditorViewportClient> Client = Viewport->GetScenecapsViewportClient())
        {
            Client->RequestRedraw();
        }
    }
}

TSharedRef<SWidget> FScenecapsEditorToolkit::MakeHDRIAssetPicker()
{
    FAssetPickerConfig Config;
    Config.Filter.ClassPaths.Add(UTextureCube::StaticClass()->GetClassPathName());
    Config.SelectionMode = ESelectionMode::Single;
    Config.OnAssetSelected = FOnAssetSelected::CreateSP(this, &FScenecapsEditorToolkit::HandleHDRIAssetSelected);

    return FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get().CreateAssetPicker(Config);
}

void FScenecapsEditorToolkit::HandleHDRIAssetSelected(const FAssetData& AssetData)
{
    SetHDRISkyboxTexture(Cast<UTextureCube>(AssetData.GetAsset()));
    if (HDRIPickerAnchor.IsValid())
    {
        HDRIPickerAnchor->SetIsOpen(false);
    }
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
        if (auto Client = Viewport->GetViewportClient())
        {
            FViewport* VP = Client->Viewport;
            if (VP)
            {
                const FGeometry& ViewportGeometry = Viewport->GetTickSpaceGeometry();
                const FVector2D LocalPos = ViewportGeometry.AbsoluteToLocal(ScreenPos) * ViewportGeometry.Scale;

                FSceneViewFamily::ConstructionValues CVS(VP, PreviewScene->GetWorld()->Scene, Client->EngineShowFlags);
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
    AActor* Actor = AssetProxy.IsValid() ? AssetProxy->SpawnPreviewEntry(Effective, World) : nullptr;
    
    // Add a small point light to each preview actor for lighting
    if (Actor && World)
    {
        UPointLightComponent* PointLight = NewObject<UPointLightComponent>(Actor, TEXT("PreviewPointLight"));
        PointLight->RegisterComponent();
        PointLight->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        PointLight->SetRelativeLocation(FVector(0, 0, 50));
        PointLight->SetIntensity(150.0f);
        PointLight->SetAttenuationRadius(300.0f);
        PointLight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.9f));
        PointLight->SetSourceRadius(10.0f);
        PointLight->SetSoftSourceRadius(5.0f);
        PointLight->SetCastShadows(false);
        PointLight->bUseInverseSquaredFalloff = true;
        PointLight->SetMobility(EComponentMobility::Movable);
    }
    
    return Actor;
}

void FScenecapsEditorToolkit::RequestRedraw()
{
    if (TSharedPtr<SScenecapsEditorViewport> Viewport = ViewportWidget.Pin())
        if (auto Client = Viewport->GetViewportClient())
            Client->Invalidate();
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
        if (auto Client = Viewport->GetScenecapsViewportClient())
        {
            Client->SetSelectedActors({ Actor });
            Client->FocusViewportOnBox(Actor->GetComponentsBoundingBox(true), true);
            Client->Invalidate();
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
            FScenecapsEditorEntryView* ScopedView =
                reinterpret_cast<FScenecapsEditorEntryView*>(DetailsStructScope->GetStructMemory());
            *ScopedView = V;
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