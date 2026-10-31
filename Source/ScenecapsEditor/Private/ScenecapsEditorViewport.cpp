// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorViewport.cpp

#include "ScenecapsEditorViewport.h"
#include "ScenecapsEditorViewportClient.h"
#include "ScenecapsEditorToolkit.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"

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

    ViewportClient = MakeShared<FScenecapsEditorViewportClient>(
        nullptr,
        PreviewScene,
        SharedThis(this),
        ToolkitPtr);

    // Apply default viewport settings: black background, grid enabled
    if (ViewportClient.IsValid())
    {
        ViewportClient->EngineShowFlags.SetGrid(true);
        ViewportClient->SetRealtime(true);
    }

    return ViewportClient.ToSharedRef();
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