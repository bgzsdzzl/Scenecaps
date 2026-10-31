// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorViewport.cpp

#include "ScenecapsEditorViewport.h"
#include "ScenecapsEditorViewportClient.h"
#include "ScenecapsEditorToolkit.h"
#include "ScenecapsViewportExtension.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"

void SScenecapsEditorViewport::Construct(const FArguments& InArgs)
{
    ToolkitPtr = InArgs._Toolkit;
    ViewportSettings = InArgs._ViewportSettings;

    if (!ViewportSettings)
    {
        ViewportSettings = NewObject<UScenecapsViewportSettings>(GetTransientPackage());
    }

    SEditorViewport::Construct(SEditorViewport::FArguments());

    ApplySettings(ViewportSettings);
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

    // Apply settings to viewport client
    if (ViewportClient.IsValid() && ViewportSettings)
    {
        ViewportClient->EngineShowFlags.SetRendering(ViewportSettings->DefaultRenderMode == EScenecapsViewportRenderMode::Lit);
        ViewportClient->SetRealtime(ViewportSettings->bEnableRealtime);
    }

    return ViewportClient.ToSharedRef();
}

void SScenecapsEditorViewport::ApplySettings(TObjectPtr<UScenecapsViewportSettings> InSettings)
{
    ViewportSettings = InSettings;

    if (!ViewportSettings)
    {
        ViewportSettings = NewObject<UScenecapsViewportSettings>(GetTransientPackage());
    }

    if (ViewportClient.IsValid())
    {
        // Render mode
        ViewportClient->EngineShowFlags.SetRendering(ViewportSettings->DefaultRenderMode == EScenecapsViewportRenderMode::Lit);

        // Show flags
        ViewportClient->EngineShowFlags.SetGrid(ViewportSettings->bShowGrid);
        ViewportClient->EngineShowFlags.SetBounds(ViewportSettings->bShowBounds);
        ViewportClient->EngineShowFlags.SetPivot(ViewportSettings->bShowPivot);

        // Realtime
        ViewportClient->SetRealtime(ViewportSettings->bEnableRealtime);

        // Camera
        ViewportClient->SetViewLocation(FVector::ZeroVector);
        ViewportClient->SetViewRotation(FRotator::ZeroRotator);
    }
}

void SScenecapsEditorViewport::AddCameraBookmark(const FScenecapsCameraBookmark& Bookmark)
{
    CameraBookmarks.RemoveAll([&](const FScenecapsCameraBookmark& B) { return B.Name == Bookmark.Name; });
    CameraBookmarks.Add(Bookmark);
    if (ViewportSettings)
    {
        ViewportSettings->CameraBookmarks = CameraBookmarks;
    }
}

void SScenecapsEditorViewport::RemoveCameraBookmark(FName Name)
{
    CameraBookmarks.RemoveAll([&](const FScenecapsCameraBookmark& B) { return B.Name == Name; });
    if (ViewportSettings)
    {
        ViewportSettings->CameraBookmarks = CameraBookmarks;
    }
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

void SScenecapsEditorViewport::ApplyExtensions()
{
    if (auto Toolkit = ToolkitPtr.Pin())
    {
        FScenecapsViewportExtensionManager::Get().ApplyExtensionsToToolkit(Toolkit.Get());
    }
}

void SScenecapsEditorViewport::RemoveExtensions()
{
    if (auto Toolkit = ToolkitPtr.Pin())
    {
        FScenecapsViewportExtensionManager::Get().RemoveExtensionsFromToolkit(Toolkit.Get());
    }
}