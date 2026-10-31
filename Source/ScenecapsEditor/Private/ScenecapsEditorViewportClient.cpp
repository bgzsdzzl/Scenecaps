// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorViewportClient.cpp

#include "ScenecapsEditorViewportClient.h"
#include "ScenecapsEditorToolkit.h"
#include "EditorModeManager.h"

FScenecapsEditorViewportClient::FScenecapsEditorViewportClient(
    FEditorModeTools* InModeTools,
    FPreviewScene* InPreviewScene,
    const TSharedRef<SEditorViewport>& InViewport,
    const TWeakPtr<FScenecapsEditorToolkit>& InToolkit)
    : FEditorViewportClient(nullptr, InPreviewScene, InViewport)
    , ToolkitPtr(InToolkit)
{
    // 像关卡编辑器一样初始化：开启网格、实时渲染、黑色背景、相机位置
    EngineShowFlags.SetGrid(true);
    SetRealtime(true);
    
    // 设置相机位置：Z=500 俯视，网格在 Z=0 可见
    SetViewLocation(FVector(0.0f, 0.0f, 500.0f));
    SetViewRotation(FRotator(-90.0f, 0.0f, 0.0f));
    
    // 像关卡编辑器一样设置视图模式和显示标志
    EngineShowFlags.SetGrid(true);
    SetRealtime(true);
    EngineShowFlags.SetCompositeEditorPrimitives(true);
    EngineShowFlags.SetSelectionOutline(true);
    
    // 设置网格大小（通过编辑器设置）
    // 注意：GridSettings 是受保护成员，子类可访问
    // GridSettings.GridColor = FLinearColor(0.3f, 0.3f, 0.3f, 0.5f);
    // GridSettings.GridSize = 100.0f;
}

void FScenecapsEditorViewportClient::Tick(float DeltaSeconds)
{
    FEditorViewportClient::Tick(DeltaSeconds);
}

FLinearColor FScenecapsEditorViewportClient::GetBackgroundColor() const
{
    return FLinearColor(0.0f, 0.0f, 0.0f, 1.0f); // 黑色背景
}

bool FScenecapsEditorViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
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