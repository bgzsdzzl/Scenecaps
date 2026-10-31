// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsEditorViewport.h

#pragma once

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "LevelEditorViewport.h"
#include "ScenecapsViewportConfig.h"

class FScenecapsEditorToolkit;
class FScenecapsEditorViewportClient;

class SScenecapsEditorViewport : public SEditorViewport
{
public:
    SLATE_BEGIN_ARGS(SScenecapsEditorViewport) {}
        SLATE_ARGUMENT(TWeakPtr<FScenecapsEditorToolkit>, Toolkit)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

    TSharedPtr<FScenecapsEditorViewportClient> GetViewportClient() const { return ViewportClient; }

    // Camera bookmarks
    void AddCameraBookmark(const FScenecapsCameraBookmark& Bookmark);
    void RemoveCameraBookmark(FName Name);
    void GotoCameraBookmark(FName Name);
    TArray<FScenecapsCameraBookmark> GetCameraBookmarks() const;

    // Custom toolbar buttons
    void AddToolbarButton(const FScenecapsViewportToolbarButton& Button);
    void RemoveToolbarButton(FName ActionName);
    TArray<FScenecapsViewportToolbarButton> GetToolbarButtons() const;

private:
    TWeakPtr<FScenecapsEditorToolkit> ToolkitPtr;
    TSharedPtr<FScenecapsEditorViewportClient> ViewportClient;
    TArray<FScenecapsCameraBookmark> CameraBookmarks;
    TArray<FScenecapsViewportToolbarButton> ToolbarButtons;
};