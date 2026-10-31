// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsVariantTimelineTool.h

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ScenecapsAsset.h"

// WARNING: Timeline Tool is event-only, ApplyPreview is a stub - not implemented yet
class SScenecapsVariantTimelineTool : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SScenecapsVariantTimelineTool) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void SetTargetAsset(UObject* InAsset);
    void Refresh();

private:
    TWeakObjectPtr<UObject> TargetAsset;
    TWeakObjectPtr<UScenecapsAsset> SceneAsset;
    
    FName SelectedFromVariant = NAME_None;
    FName SelectedToVariant = NAME_None;
    float PreviewAlpha = 0.0f;
    bool bIsPlaying = false;
    double PlayStartTime = 0.0;
    float PreviewDuration = 1.0f;
    int32 PreviewEaseIndex = 2; // CubicInOut

    TArray<FName> VariantNames;

    void UpdateVariantOptions();
    FReply OnPlayClicked();
    FReply OnStopClicked();
    void OnAlphaChanged(float InValue);
    void TickPlayback();
    // WARNING: Not implemented - preview only shows UI, no visual interpolation
    void ApplyPreview(float Alpha);

    // Getters for Slate delegates
    float GetPreviewAlpha() const;
    FText GetAlphaText() const;
    FText GetFromVariantText() const;
    FText GetToVariantText() const;
    FText GetStatusText() const;
    float GetPreviewDuration() const;
    void OnDurationChanged(float InValue);
    FText GetEaseText() const;
};


