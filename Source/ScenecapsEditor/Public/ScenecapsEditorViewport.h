// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsEditorViewport.h

#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "ScenecapsViewportConfig.h"

class FScenecapsEditorToolkit;
class FScenecapsEditorViewportClient;
class UScenecapsViewportSettings;

class SScenecapsEditorViewport : public SEditorViewport
{
public:
    SLATE_BEGIN_ARGS(SScenecapsEditorViewport) {}
        SLATE_ARGUMENT(TWeakPtr<FScenecapsEditorToolkit>, Toolkit)
        SLATE_ARGUMENT(TObjectPtr<UScenecapsViewportSettings>, ViewportSettings)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

    TSharedPtr<FScenecapsEditorViewportClient> GetViewportClient() const { return ViewportClient; }

    // Apply settings to viewport
    void ApplySettings(TObjectPtr<UScenecapsViewportSettings> InSettings);

    // Get current settings
    TObjectPtr<UScenecapsViewportSettings> GetSettings() const { return ViewportSettings; }

    // Camera bookmarks
    void AddCameraBookmark(const FScenecapsCameraBookmark& Bookmark);
    void RemoveCameraBookmark(FName Name);
    void GotoCameraBookmark(FName Name);
    TArray<FScenecapsCameraBookmark> GetCameraBookmarks() const;

    // Custom toolbar buttons
    void AddToolbarButton(const FScenecapsViewportToolbarButton& Button);
    void RemoveToolbarButton(FName ActionName);
    TArray<FScenecapsViewportToolbarButton> GetToolbarButtons() const;

    // Viewport extensions
    void ApplyExtensions();
    void RemoveExtensions();

private:
    TWeakPtr<FScenecapsEditorToolkit> ToolkitPtr;
    TSharedPtr<FScenecapsEditorViewportClient> ViewportClient;
    TObjectPtr<UScenecapsViewportSettings> ViewportSettings;
    TArray<FScenecapsCameraBookmark> CameraBookmarks;
    TArray<FScenecapsViewportToolbarButton> ToolbarButtons;
};
