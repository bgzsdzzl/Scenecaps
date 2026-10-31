// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorViewportClient.cpp

#include "ScenecapsEditorViewportClient.h"
#include "ScenecapsEditorToolkit.h"
#include "PrimitiveDrawInterface.h"
#include "InputCoreTypes.h"
#include "Engine/Engine.h"
#include "EditorModeManager.h"

FScenecapsEditorViewportClient::FScenecapsEditorViewportClient(
    FEditorModeTools* InModeTools,
    FPreviewScene* InPreviewScene,
    const TSharedRef<SEditorViewport>& InViewport,
    const TWeakPtr<FScenecapsEditorToolkit>& InToolkit)
    : FEditorViewportClient(InModeTools, InPreviewScene, InViewport)
    , ToolkitPtr(InToolkit)
{
}

void FScenecapsEditorViewportClient::Tick(float DeltaSeconds)
{
    FEditorViewportClient::Tick(DeltaSeconds);
}

FLinearColor FScenecapsEditorViewportClient::GetBackgroundColor() const
{
    return FEditorViewportClient::GetBackgroundColor();
}

bool FScenecapsEditorViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
    return FEditorViewportClient::InputKey(EventArgs);
}

void FScenecapsEditorViewportClient::TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge)
{
    FEditorViewportClient::TrackingStarted(InInputState, bIsDragging, bNudge);
}

void FScenecapsEditorViewportClient::TrackingStopped()
{
    FEditorViewportClient::TrackingStopped();
}

void FScenecapsEditorViewportClient::RequestRedraw()
{
    Invalidate();
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