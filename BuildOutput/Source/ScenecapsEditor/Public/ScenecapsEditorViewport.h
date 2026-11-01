// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsEditorViewport.h

#include "CoreMinimal.h"
#include "SEditorViewport.h"
#include "EditorViewportClient.h"
#include "ScenecapsViewportConfig.h"

class FScenecapsEditorToolkit;
class FEditorViewportClient;
class FScenecapsEditorViewportClient;

class SScenecapsEditorViewport : public SEditorViewport
{
public:
    SLATE_BEGIN_ARGS(SScenecapsEditorViewport) {}
        SLATE_ARGUMENT(TWeakPtr<FScenecapsEditorToolkit>, Toolkit)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

    TSharedPtr<FEditorViewportClient> GetViewportClient() const { return ViewportClient; }
    TSharedPtr<FScenecapsEditorViewportClient> GetScenecapsViewportClient() const { return StaticCastSharedPtr<FScenecapsEditorViewportClient>(ViewportClient); }

    // Camera bookmarks
    void AddCameraBookmark(const FScenecapsCameraBookmark& Bookmark);
    void RemoveCameraBookmark(FName Name);
    void GotoCameraBookmark(FName Name);
    TArray<FScenecapsCameraBookmark> GetCameraBookmarks() const;

    // Custom toolbar buttons
    void AddToolbarButton(const FScenecapsViewportToolbarButton& Button);
    void RemoveToolbarButton(FName ActionName);
    TArray<FScenecapsViewportToolbarButton> GetToolbarButtons() const;

protected:
    virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

private:
    TWeakPtr<FScenecapsEditorToolkit> ToolkitPtr;
    TSharedPtr<FEditorViewportClient> ViewportClient;
    TArray<FScenecapsCameraBookmark> CameraBookmarks;
    TArray<FScenecapsViewportToolbarButton> ToolbarButtons;
};