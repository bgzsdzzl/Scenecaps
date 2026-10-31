// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorViewportClient.cpp

#include "ScenecapsEditorViewportClient.h"
#include "ScenecapsEditorToolkit.h"
#include "ScenecapsViewportExtension.h"
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
    // Default overlay configs
    OverlayConfigs.Add(FScenecapsViewportOverlayConfig
    {
        .Type = EScenecapsViewportOverlayType::Grid,
        .bEnabled = true,
        .Color = FLinearColor::White,
        .Scale = 1.0f
    });

    OverlayConfigs.Add(FScenecapsViewportOverlayConfig
    {
        .Type = EScenecapsViewportOverlayType::Axes,
        .bEnabled = true,
        .Color = FLinearColor::White,
        .Scale = 1.0f
    });
}

void FScenecapsEditorViewportClient::Tick(float DeltaSeconds)
{
    FEditorViewportClient::Tick(DeltaSeconds);

    // Tick extensions
    TickExtensions(DeltaSeconds);
}

FLinearColor FScenecapsEditorViewportClient::GetBackgroundColor() const
{
    if (ToolkitPtr.IsValid())
    {
        if (auto Toolkit = ToolkitPtr.Pin())
        {
            if (auto Settings = Toolkit->GetViewportSettings())
            {
                return Settings->BackgroundColor;
            }
        }
    }
    return FEditorViewportClient::GetBackgroundColor();
}

bool FScenecapsEditorViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
    // Try extensions first
    if (HandleCustomInput(FInputEvent()))
    {
        return true;
    }

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

void FScenecapsEditorViewportClient::DrawCustomOverlays(FPrimitiveDrawInterface* PDI)
{
    // Draw grid
    for (const FScenecapsViewportOverlayConfig& Config : OverlayConfigs)
    {
        if (!Config.bEnabled) continue;

        switch (Config.Type)
        {
        case EScenecapsViewportOverlayType::Grid:
            if (Config.bEnabled)
            {
                DrawGrid(PDI, Config);
            }
            break;
        case EScenecapsViewportOverlayType::Axes:
            if (Config.bEnabled)
            {
                DrawAxes(PDI, Config);
            }
            break;
        case EScenecapsViewportOverlayType::Bounds:
            if (Config.bEnabled)
            {
                DrawBounds(PDI, Config);
            }
            break;
        case EScenecapsViewportOverlayType::Pivot:
            if (Config.bEnabled)
            {
                DrawPivot(PDI, Config);
            }
            break;
        case EScenecapsViewportOverlayType::Custom:
            if (Config.bEnabled)
            {
                DrawCustom(PDI, Config);
            }
            break;
        default:
            break;
        }
    }
}

void FScenecapsEditorViewportClient::DrawGrid(FPrimitiveDrawInterface* PDI, const FScenecapsViewportOverlayConfig& Config)
{
    const float GridSize = 100.0f * Config.Scale;
    const int32 NumLines = 20;
    const FLinearColor GridColor = Config.Color;

    for (int32 i = -NumLines; i <= NumLines; ++i)
    {
        float X = i * GridSize;
        PDI->DrawLine(FVector(X, -NumLines * GridSize, 0), FVector(X, NumLines * GridSize, 0), GridColor, SDPG_World);
        PDI->DrawLine(FVector(-NumLines * GridSize, X, 0), FVector(NumLines * GridSize, X, 0), GridColor, SDPG_World);
    }
}

void FScenecapsEditorViewportClient::DrawAxes(FPrimitiveDrawInterface* PDI, const FScenecapsViewportOverlayConfig& Config)
{
    const float AxisLength = 100.0f * Config.Scale;
    const FVector Origin = FVector::ZeroVector;

    PDI->DrawLine(Origin, FVector(AxisLength, 0, 0), FLinearColor::Red, SDPG_World, 2.0f);
    PDI->DrawLine(Origin, FVector(0, AxisLength, 0), FLinearColor::Green, SDPG_World, 2.0f);
    PDI->DrawLine(Origin, FVector(0, 0, AxisLength), FLinearColor::Blue, SDPG_World, 2.0f);

    // Labels
    PDI->DrawPoint(FVector(AxisLength * 1.1f, 0, 0), FLinearColor::Red, 5.0f, SDPG_World);
    PDI->DrawPoint(FVector(0, AxisLength * 1.1f, 0), FLinearColor::Green, 5.0f, SDPG_World);
    PDI->DrawPoint(FVector(0, 0, AxisLength * 1.1f), FLinearColor::Blue, 5.0f, SDPG_World);
}

void FScenecapsEditorViewportClient::DrawBounds(FPrimitiveDrawInterface* PDI, const FScenecapsViewportOverlayConfig& Config)
{
    // Draw bounds for selected actors
    for (const TWeakObjectPtr<AActor>& ActorPtr : SelectedActors)
    {
        if (AActor* Actor = ActorPtr.Get())
        {
            FVector Origin, Extent;
            Actor->GetActorBounds(true, Origin, Extent);
            DrawWireBox(PDI, FMatrix::Identity, FBox(Origin - Extent, Origin + Extent), FColor::Cyan, SDPG_World);
        }
    }
}

void FScenecapsEditorViewportClient::DrawPivot(FPrimitiveDrawInterface* PDI, const FScenecapsViewportOverlayConfig& Config)
{
    for (const TWeakObjectPtr<AActor>& ActorPtr : SelectedActors)
    {
        if (AActor* Actor = ActorPtr.Get())
        {
            FVector Location = Actor->GetActorLocation();
            PDI->DrawPoint(Location, FColor::Yellow, 10.0f, SDPG_World);
            PDI->DrawLine(Location, Location + FVector(0, 0, 100), FColor::Yellow, SDPG_World, 2.0f);
        }
    }
}

void FScenecapsEditorViewportClient::DrawCustom(FPrimitiveDrawInterface* PDI, const FScenecapsViewportOverlayConfig& Config)
{
    // Custom overlay - extensions can override this
}

bool FScenecapsEditorViewportClient::HandleCustomInput(const FInputEvent& InEvent)
{
    for (FScenecapsViewportExtension* Ext : FScenecapsViewportExtensionManager::Get().GetExtensionsForPoint(EScenecapsViewportExtensionPoint::ViewportOverlay))
    {
        if (Ext && Ext->bEnabled)
        {
            if (Ext->HandleViewportInput(InEvent, this))
            {
                return true;
            }
        }
    }
    return false;
}

void FScenecapsEditorViewportClient::DrawCustomOverlaysWithExtensions(FPrimitiveDrawInterface* PDI)
{
    // Draw built-in overlays
    DrawCustomOverlays(PDI);

    // Draw extension overlays
    for (FScenecapsViewportExtension* Ext : FScenecapsViewportExtensionManager::Get().GetExtensionsForPoint(EScenecapsViewportExtensionPoint::ViewportOverlay))
    {
        if (Ext && Ext->bEnabled)
        {
            Ext->DrawViewportOverlay(this, PDI);
        }
    }
}

void FScenecapsEditorViewportClient::TickExtensions(float DeltaSeconds)
{
    for (FScenecapsViewportExtension* Ext : FScenecapsViewportExtensionManager::Get().GetExtensionsForPoint(EScenecapsViewportExtensionPoint::ViewportOverlay))
    {
        if (Ext && Ext->bEnabled)
        {
            Ext->TickViewportOverlay(this, DeltaSeconds);
        }
    }
}

void FScenecapsEditorViewportClient::AddOverlayConfig(const FScenecapsViewportOverlayConfig& Config)
{
    OverlayConfigs.RemoveAll([&](const FScenecapsViewportOverlayConfig& C) { return C.Type == Config.Type; });
    OverlayConfigs.Add(Config);
}

void FScenecapsEditorViewportClient::RemoveOverlayConfig(EScenecapsViewportOverlayType Type)
{
    OverlayConfigs.RemoveAll([&](const FScenecapsViewportOverlayConfig& C) { return C.Type == Type; });
}

void FScenecapsEditorViewportClient::ClearOverlayConfigs()
{
    OverlayConfigs.Empty();
}