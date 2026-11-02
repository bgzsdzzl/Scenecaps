// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorViewportClient.cpp

#include "ScenecapsEditorViewportClient.h"
#include "ScenecapsEditorToolkit.h"
#include "Engine/World.h"
#include "PreviewScene.h"
#include "SceneView.h"
#include "CollisionQueryParams.h"
#include "Framework/Application/SlateApplication.h"
#include "ScenecapsAsset.h"

FScenecapsEditorViewportClient::FScenecapsEditorViewportClient(
    FPreviewScene* InPreviewScene,
    const TSharedRef<SEditorViewport>& InViewport,
    const TSharedPtr<FScenecapsEditorToolkit>& InToolkit)
    : FEditorViewportClient(nullptr, InPreviewScene, InViewport)
    , ToolkitPtr(InToolkit)
{
    SetViewMode(VMI_Lit);

    // Grid settings using DrawHelper (like the old working version)
    DrawHelper.bDrawGrid = true;
    DrawHelper.bDrawPivot = false;
    DrawHelper.bDrawWorldBox = false;
    DrawHelper.GridColorAxis = FColor(80, 80, 80);
    DrawHelper.GridColorMajor = FColor(55, 55, 55);
    DrawHelper.GridColorMinor = FColor(35, 35, 35);

    SetRealtime(false);

    // Disable sky/atmosphere rendering in the viewport
    EngineShowFlags.SetSkyLighting(false);

    SetRealtime(false);

    // Frame the default scene bounds (like old version)
    FocusViewportOnBox(FBox(FVector(-100.f), FVector(100.f)));
}

void FScenecapsEditorViewportClient::Tick(float DeltaSeconds)
{
    FEditorViewportClient::Tick(DeltaSeconds);
}

FLinearColor FScenecapsEditorViewportClient::GetBackgroundColor() const
{
    return FLinearColor(0.035f, 0.035f, 0.035f, 1.0f); // Dark gray like old version
}

bool FScenecapsEditorViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
    const bool bHandled = FEditorViewportClient::InputKey(EventArgs);

    // Left mouse click selection (like old version)
    if (EventArgs.Key == EKeys::LeftMouseButton &&
        EventArgs.Event == IE_Pressed &&
        EventArgs.Viewport)
    {
        const FModifierKeysState ModifierKeys = FSlateApplication::Get().GetModifierKeys();
        if (!ModifierKeys.IsShiftDown() && !ModifierKeys.IsControlDown() && !ModifierKeys.IsAltDown())
        {
            if (TSharedPtr<FScenecapsEditorToolkit> Toolkit = ToolkitPtr.Pin())
            {
                if (FPreviewScene* Scene = Toolkit->GetPreviewScene())
                {
                    if (UWorld* World = Scene->GetWorld())
                    {
                        FSceneViewFamily::ConstructionValues CVS(EventArgs.Viewport, World->Scene, *GetEngineShowFlagsPublic());
                        FSceneViewFamilyContext ViewFamily(CVS);
                        FSceneView* View = CalcSceneView(&ViewFamily);

                        if (View)
                        {
                            FIntPoint MousePos(0, 0);
                            EventArgs.Viewport->GetMousePos(MousePos, true);

                            FVector RayOrigin = FVector::ZeroVector;
                            FVector RayDirection = FVector::ForwardVector;
                            View->DeprojectFVector2D(FVector2D(MousePos), RayOrigin, RayDirection);

                            FHitResult HitResult;
                            FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ScenecapsSelect), true);

                            if (World->LineTraceSingleByChannel(HitResult, RayOrigin, RayOrigin + RayDirection * 100000.f, ECC_Visibility, QueryParams))
                            {
                                SetSelectedActors({ HitResult.GetActor() });
                                Toolkit->HandleViewportActorSelected(HitResult.GetActor());
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
        }
    }
    // Delete key
    else if (EventArgs.Key == EKeys::Delete && EventArgs.Event == IE_Pressed)
    {
        if (TSharedPtr<FScenecapsEditorToolkit> Toolkit = ToolkitPtr.Pin())
        {
            if (SelectedActors.Num() > 0)
            {
                Toolkit->DeleteSelectedViewportActors(SelectedActors);
                SetSelectedActors({});
                return true;
            }
        }
    }

    return bHandled;
}

void FScenecapsEditorViewportClient::TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge)
{
    FEditorViewportClient::TrackingStarted(InInputState, bIsDragging, bNudge);
    if (bIsDragging && SelectedActors.Num() > 0)
    {
        if (TSharedPtr<FScenecapsEditorToolkit> Toolkit = ToolkitPtr.Pin())
        {
            Toolkit->BeginTransaction();
            bIsTransacting = true;
        }
    }
}

void FScenecapsEditorViewportClient::TrackingStopped()
{
    FEditorViewportClient::TrackingStopped();
    if (bIsTransacting)
    {
        if (TSharedPtr<FScenecapsEditorToolkit> Toolkit = ToolkitPtr.Pin())
        {
            Toolkit->EndTransactionAndUpdateTransforms(SelectedActors);
        }
        bIsTransacting = false;
    }
}

void FScenecapsEditorViewportClient::FocusViewportOnBox(const FBox& Box, bool bInstant)
{
    FEditorViewportClient::FocusViewportOnBox(Box, bInstant);
}

void FScenecapsEditorViewportClient::SetSelectedActors(const TArray<AActor*>& InActors)
{
    SelectedActors.Empty();
    for (AActor* Actor : InActors)
    {
        if (Actor)
        {
            SelectedActors.Add(Actor);
        }
    }
}