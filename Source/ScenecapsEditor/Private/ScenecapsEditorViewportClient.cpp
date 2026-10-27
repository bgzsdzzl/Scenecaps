// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorViewportClient.cpp
#include "ScenecapsEditorViewportClient.h"
#include "ScenecapsEditorToolkit.h"
#include "ScenecapsEditorAssetProxy.h"
#include "Engine/World.h"
#include "PreviewScene.h"
#include "SceneView.h"
#include "CollisionQueryParams.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"
#include "DrawDebugHelpers.h"

FScenecapsEditorViewportClient::FScenecapsEditorViewportClient(
    FPreviewScene* InPreviewScene,
    const TSharedRef<SEditorViewport>& InViewport,
    const TSharedPtr<FScenecapsEditorToolkit>& InToolkit)
    : FEditorViewportClient(nullptr, InPreviewScene, InViewport)
    , ToolkitPtr(InToolkit)
{
    SetViewMode(VMI_Lit);
    DrawHelper.bDrawGrid = true;
    DrawHelper.bDrawPivot = false;
    DrawHelper.bDrawWorldBox = false;
    DrawHelper.GridColorAxis = FColor(80, 80, 80);
    DrawHelper.GridColorMajor = FColor(55, 55, 55);
    DrawHelper.GridColorMinor = FColor(35, 35, 35);
    SetRealtime(false);

    bool bRestored = false;
    if (TSharedPtr<FScenecapsEditorToolkit> Toolkit = ToolkitPtr.Pin())
    {
        if (TSharedPtr<FScenecapsEditorAssetProxy> Proxy = Toolkit->GetAssetProxy())
        {
            FVector Loc; FRotator Rot;
            if (Proxy->GetSavedEditorView(Loc, Rot))
            {
                SetViewLocation(Loc);
                SetViewRotation(Rot);
                bRestored = true;
            }
        }
    }
    if (!bRestored) FocusViewportOnBox(FBox(FVector(-100.f), FVector(100.f)));
}

void FScenecapsEditorViewportClient::Tick(float DeltaSeconds)
{
    FEditorViewportClient::Tick(DeltaSeconds);
    if (TSharedPtr<FScenecapsEditorToolkit> Toolkit = ToolkitPtr.Pin())
        Toolkit->UpdateCachedEditorView(GetViewLocation(), GetViewRotation());

    // 选中条目后，在视口里画 XYZ 坐标轴箭头 + 包围盒块
    if (SelectedActors.Num() > 0)
    {
        if (TSharedPtr<FScenecapsEditorToolkit> Toolkit = ToolkitPtr.Pin())
            if (FPreviewScene* Scene = Toolkit->GetPreviewScene())
                if (UWorld* World = Scene->GetWorld())
                {
                    const float AxisLength = 60.f;
                    const float Thickness = 2.f;
                    for (const TWeakObjectPtr<AActor>& ActorPtr : SelectedActors)
                    {
                        AActor* Actor = ActorPtr.Get();
                        if (!Actor || !IsValid(Actor)) continue;

                        const FVector Loc = Actor->GetActorLocation();
                        const FRotator Rot = Actor->GetActorRotation();

                        const FVector XDir = Rot.RotateVector(FVector::ForwardVector);
                        const FVector YDir = Rot.RotateVector(FVector::RightVector);
                        const FVector ZDir = Rot.RotateVector(FVector::UpVector);

                        DrawDebugDirectionalArrow(World, Loc, Loc + XDir * AxisLength, 8.f, FColor::Red, false, 0.f, 0, Thickness);
                        DrawDebugDirectionalArrow(World, Loc, Loc + YDir * AxisLength, 8.f, FColor::Green, false, 0.f, 0, Thickness);
                        DrawDebugDirectionalArrow(World, Loc, Loc + ZDir * AxisLength, 8.f, FColor::Blue, false, 0.f, 0, Thickness);

                        const FBox Box = Actor->GetComponentsBoundingBox(true);
                        if (Box.IsValid)
                        {
                            DrawDebugBox(World, Box.GetCenter(), Box.GetExtent(), FColor::Yellow, false, 0.f, 0, 2.f);
                        }
                        else
                        {
                            DrawDebugBox(World, Loc, FVector(15.f), FColor::Yellow, false, 0.f, 0, 2.f);
                        }
                    }
                }
    }
}

void FScenecapsEditorViewportClient::RequestRedraw() { Invalidate(); }

FLinearColor FScenecapsEditorViewportClient::GetBackgroundColor() const
{
    return FLinearColor(0.035f, 0.035f, 0.035f, 1.0f);
}

void FScenecapsEditorViewportClient::SetSelectedActors(const TArray<AActor*>& InActors)
{
    SelectedActors.Empty();
    for (AActor* A : InActors) SelectedActors.Add(A);
    if (GEditor)
    {
        GEditor->SelectNone(false, true);
        for (AActor* A : InActors) GEditor->SelectActor(A, true, true);
        GEditor->NoteSelectionChange();
    }
    Invalidate();
}

bool FScenecapsEditorViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
    const bool bHandled = FEditorViewportClient::InputKey(EventArgs);

    if (EventArgs.Key == EKeys::LeftMouseButton && EventArgs.Event == IE_Pressed && EventArgs.Viewport)
    {
        const FModifierKeysState Mod = FSlateApplication::Get().GetModifierKeys();
        if (!Mod.IsShiftDown() && !Mod.IsControlDown() && !Mod.IsAltDown())
        {
            if (TSharedPtr<FScenecapsEditorToolkit> Toolkit = ToolkitPtr.Pin())
                if (FPreviewScene* Scene = Toolkit->GetPreviewScene())
                    if (UWorld* World = Scene->GetWorld())
                    {
                        FSceneViewFamily::ConstructionValues CVS(EventArgs.Viewport, World->Scene, *GetEngineShowFlagsPublic());
                        FSceneViewFamilyContext ViewFamily(CVS);
                        FSceneView* View = CalcSceneView(&ViewFamily);
                        if (View)
                        {
                            FIntPoint MousePos(0, 0);
                            EventArgs.Viewport->GetMousePos(MousePos, true);
                            FVector RayOrigin, RayDir;
                            View->DeprojectFVector2D(FVector2D(MousePos), RayOrigin, RayDir);
                            FHitResult Hit;
                            FCollisionQueryParams QP(SCENE_QUERY_STAT(ScenecapsSelect), true);
                            if (World->LineTraceSingleByChannel(Hit, RayOrigin,
                                RayOrigin + RayDir * 100000.f, ECC_Visibility, QP))
                            {
                                SetSelectedActors({ Hit.GetActor() });
                                Toolkit->HandleViewportActorSelected(Hit.GetActor());
                            }
                            else
                            {
                                SetSelectedActors({});
                                Toolkit->ShowAssetDetails();
                            }
                        }
                    }
        }
    }
    else if (EventArgs.Key == EKeys::Delete && EventArgs.Event == IE_Pressed)
    {
        if (TSharedPtr<FScenecapsEditorToolkit> Toolkit = ToolkitPtr.Pin())
            if (SelectedActors.Num() > 0)
            {
                Toolkit->DeleteSelectedViewportActors(SelectedActors);
                SetSelectedActors({});
                return true;
            }
    }
    return bHandled;
}

void FScenecapsEditorViewportClient::TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge)
{
    FEditorViewportClient::TrackingStarted(InInputState, bIsDragging, bNudge);
    if (bIsDragging && SelectedActors.Num() > 0)
        if (TSharedPtr<FScenecapsEditorToolkit> Toolkit = ToolkitPtr.Pin())
        {
            Toolkit->BeginTransaction();
            bIsTransacting = true;
        }
}

void FScenecapsEditorViewportClient::TrackingStopped()
{
    FEditorViewportClient::TrackingStopped();
    if (bIsTransacting)
    {
        if (TSharedPtr<FScenecapsEditorToolkit> Toolkit = ToolkitPtr.Pin())
            Toolkit->EndTransactionAndUpdateTransforms(SelectedActors);
        bIsTransacting = false;
    }
}