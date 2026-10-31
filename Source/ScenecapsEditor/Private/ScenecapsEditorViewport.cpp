// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorViewport.cpp

#include "ScenecapsEditorViewport.h"
#include "ScenecapsEditorToolkit.h"
#include "ScenecapsEditorViewportClient.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"
#include "AssetRegistry/AssetData.h"
#include "DragAndDrop/AssetDragDropOp.h"

void SScenecapsEditorViewport::Construct(const FArguments& InArgs)
{
    ToolkitPtr = InArgs._Toolkit;

    SEditorViewport::Construct(SEditorViewport::FArguments());
}

TSharedRef<FEditorViewportClient> SScenecapsEditorViewport::MakeEditorViewportClient()
{
    FPreviewScene* PreviewScene = nullptr;
    
    if (ToolkitPtr.IsValid())
    {
        if (auto Toolkit = ToolkitPtr.Pin())
        {
            PreviewScene = Toolkit->GetPreviewScene();
        }
    }

    // Use custom viewport client that extends FEditorViewportClient with additional methods
    // Pass the toolkit like the old working version
    ViewportClient = MakeShared<FScenecapsEditorViewportClient>(
        PreviewScene,
        SharedThis(this),
        ToolkitPtr.Pin());

    return ViewportClient.ToSharedRef();
}

FReply SScenecapsEditorViewport::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
    if (!ToolkitPtr.IsValid())
    {
        return FReply::Unhandled();
    }

    if (auto Toolkit = ToolkitPtr.Pin())
    {
        // Extract asset data from drag drop event
        TArray<FAssetData> DroppedAssets;
        if (TSharedPtr<FAssetDragDropOp> AssetDragDropOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>())
        {
            DroppedAssets = AssetDragDropOp->GetAssets();
        }
        
        if (DroppedAssets.Num() > 0)
        {
            TArrayView<FAssetData> AssetsView(DroppedAssets);
            Toolkit->OnAssetsDropped(DragDropEvent, AssetsView);
        }
    }

    return FReply::Handled();
}

void SScenecapsEditorViewport::AddCameraBookmark(const FScenecapsCameraBookmark& Bookmark)
{
    CameraBookmarks.RemoveAll([&](const FScenecapsCameraBookmark& B) { return B.Name == Bookmark.Name; });
    CameraBookmarks.Add(Bookmark);
}

void SScenecapsEditorViewport::RemoveCameraBookmark(FName Name)
{
    CameraBookmarks.RemoveAll([&](const FScenecapsCameraBookmark& B) { return B.Name == Name; });
}

void SScenecapsEditorViewport::GotoCameraBookmark(FName Name)
{
    const FScenecapsCameraBookmark* Bookmark = CameraBookmarks.FindByPredicate([&](const FScenecapsCameraBookmark& B) { return B.Name == Name; });
    if (Bookmark && ViewportClient.IsValid())
    {
        ViewportClient->SetViewLocation(Bookmark->Location);
        ViewportClient->SetViewRotation(Bookmark->Rotation);
        if (Bookmark->bIsOrthographic)
        {
            ViewportClient->SetOrthoZoom(Bookmark->OrthoZoom);
        }
    }
}

TArray<FScenecapsCameraBookmark> SScenecapsEditorViewport::GetCameraBookmarks() const
{
    return CameraBookmarks;
}

void SScenecapsEditorViewport::AddToolbarButton(const FScenecapsViewportToolbarButton& Button)
{
    ToolbarButtons.RemoveAll([&](const FScenecapsViewportToolbarButton& B) { return B.ActionName == Button.ActionName; });
    ToolbarButtons.Add(Button);
}

void SScenecapsEditorViewport::RemoveToolbarButton(FName ActionName)
{
    ToolbarButtons.RemoveAll([&](const FScenecapsViewportToolbarButton& B) { return B.ActionName == ActionName; });
}

TArray<FScenecapsViewportToolbarButton> SScenecapsEditorViewport::GetToolbarButtons() const
{
    return ToolbarButtons;
}