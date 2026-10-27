// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsEditorViewportClient.h
#pragma once

#include "CoreMinimal.h"
#include "EditorViewportClient.h"

class FPreviewScene;
class FScenecapsEditorToolkit;

class FScenecapsEditorViewportClient : public FEditorViewportClient
{
public:
    FScenecapsEditorViewportClient(
        FPreviewScene* InPreviewScene,
        const TSharedRef<SEditorViewport>& InViewport,
        const TSharedPtr<FScenecapsEditorToolkit>& InToolkit);

    virtual void Tick(float DeltaSeconds) override;
    virtual FLinearColor GetBackgroundColor() const override;
    virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
    virtual void TrackingStarted(const struct FInputEventState& InInputState, bool bIsDragging, bool bNudge) override;
    virtual void TrackingStopped() override;

    FEngineShowFlags* GetEngineShowFlagsPublic() const { return const_cast<FEngineShowFlags*>(&EngineShowFlags); }
    void RequestRedraw();
    void SetSelectedActors(const TArray<AActor*>& InActors);
    const TArray<TWeakObjectPtr<AActor>>& GetSelectedActors() const { return SelectedActors; }

private:
    TWeakPtr<FScenecapsEditorToolkit> ToolkitPtr;
    TArray<TWeakObjectPtr<AActor>> SelectedActors;
    bool bIsTransacting = false;
};