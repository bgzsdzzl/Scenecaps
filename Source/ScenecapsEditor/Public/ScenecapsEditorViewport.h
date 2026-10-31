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
        SLATE_ARGUMENT(TSharedPtr<FScenecapsEditorToolkit>, Toolkit)
        SLATE_ARGUMENT(TSharedPtr<UScenecapsViewportSettings>, ViewportSettings)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

    TSharedPtr<FScenecapsEditorViewportClient> GetViewportClient() const { return ViewportClient; }

    // Apply settings to viewport
    void ApplySettings(TSharedPtr<UScenecapsViewportSettings> InSettings);

    // Get current settings
    TSharedPtr<UScenecapsViewportSettings> GetSettings() const { return ViewportSettings; }

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
    TSharedPtr<UScenecapsViewportSettings> ViewportSettings;
    TArray<FScenecapsCameraBookmark> CameraBookmarks;
    TArray<FScenecapsViewportToolbarButton> ToolbarButtons;
};
