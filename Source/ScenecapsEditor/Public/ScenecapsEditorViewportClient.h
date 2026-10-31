// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsEditorViewportClient.h

#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"
#include "ScenecapsViewportConfig.h"

class FPreviewScene;
class FScenecapsEditorToolkit;
class UScenecapsViewportSettings;
class SScenecapsEditorViewport;

class FScenecapsEditorViewportClient : public FEditorViewportClient
{
public:
    FScenecapsEditorViewportClient(
        FEditorModeTools* InModeTools,
        FPreviewScene* InPreviewScene,
        const TSharedRef<SEditorViewport>& InViewport,
        const TWeakPtr<FScenecapsEditorToolkit>& InToolkit);

    virtual void Tick(float DeltaSeconds) override;
    virtual FLinearColor GetBackgroundColor() const override;
    virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
    virtual void TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge) override;
    virtual void TrackingStopped() override;

    FEngineShowFlags* GetEngineShowFlagsPublic() const { return const_cast<FEngineShowFlags*>(&EngineShowFlags); }
    void RequestRedraw();
    void SetSelectedActors(const TArray<AActor*>& InActors);
    const TArray<TWeakObjectPtr<AActor>>& GetSelectedActors() const { return SelectedActors; }

    // ---- Custom Overlays ----
    virtual void DrawCustomOverlays(FPrimitiveDrawInterface* PDI);
    void AddOverlayConfig(const FScenecapsViewportOverlayConfig& Config);
    void RemoveOverlayConfig(EScenecapsViewportOverlayType Type);
    void ClearOverlayConfigs();
    TArray<FScenecapsViewportOverlayConfig> GetOverlayConfigs() const { return OverlayConfigs; }

    // ---- Extensions ----
    virtual bool HandleCustomInput(const FInputEvent& InEvent);
    virtual void DrawCustomOverlaysWithExtensions(FPrimitiveDrawInterface* PDI);
    void TickExtensions(float DeltaSeconds);

protected:
    // Overlay drawing helpers
    virtual void DrawGrid(FPrimitiveDrawInterface* PDI, const FScenecapsViewportOverlayConfig& Config);
    virtual void DrawAxes(FPrimitiveDrawInterface* PDI, const FScenecapsViewportOverlayConfig& Config);
    virtual void DrawBounds(FPrimitiveDrawInterface* PDI, const FScenecapsViewportOverlayConfig& Config);
    virtual void DrawPivot(FPrimitiveDrawInterface* PDI, const FScenecapsViewportOverlayConfig& Config);
    virtual void DrawCustom(FPrimitiveDrawInterface* PDI, const FScenecapsViewportOverlayConfig& Config);

private:
    TWeakPtr<FScenecapsEditorToolkit> ToolkitPtr;
    TArray<TWeakObjectPtr<AActor>> SelectedActors;
    bool bIsTransacting = false;

    TArray<FScenecapsViewportOverlayConfig> OverlayConfigs;
};