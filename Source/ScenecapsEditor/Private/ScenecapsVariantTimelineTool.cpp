// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsVariantTimelineTool.cpp

#include "ScenecapsVariantTimelineTool.h"
#include "ScenecapsAsset.h"
#include "SlateBasics.h"
#include "Framework/Application/SlateApplication.h"

void SScenecapsVariantTimelineTool::Construct(const FArguments& InArgs)
{
    // Create child widgets
    TSharedRef<STextBlock> StatusText = SNew(STextBlock)
        .Text(this, &SScenecapsVariantTimelineTool::GetStatusText)
        .ColorAndOpacity(FLinearColor::Yellow);

    TSharedRef<SSlider> AlphaSlider = SNew(SSlider)
        .Value(this, &SScenecapsVariantTimelineTool::GetPreviewAlpha)
        .OnValueChanged(this, &SScenecapsVariantTimelineTool::OnAlphaChanged)
        .StepSize(0.01f);

    TSharedRef<STextBlock> AlphaText = SNew(STextBlock)
        .Text(this, &SScenecapsVariantTimelineTool::GetAlphaText)
        .MinDesiredWidth(50);

    TSharedRef<SSpinBox<float>> DurationSpinBox = SNew(SSpinBox<float>)
        .Value(this, &SScenecapsVariantTimelineTool::GetPreviewDuration)
        .OnValueChanged(this, &SScenecapsVariantTimelineTool::OnDurationChanged)
        .MinValue(0.01f)
        .MaxValue(60.0f)
        .Delta(0.1f);

    TSharedRef<STextBlock> EaseText = SNew(STextBlock)
        .Text(this, &SScenecapsVariantTimelineTool::GetEaseText);

    TSharedRef<STextBlock> FromVariantText = SNew(STextBlock)
        .Text(this, &SScenecapsVariantTimelineTool::GetFromVariantText);

    TSharedRef<STextBlock> ToVariantText = SNew(STextBlock)
        .Text(this, &SScenecapsVariantTimelineTool::GetToVariantText);

    ChildSlot
    [
        SNew(SBox)
        .Padding(10)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 10)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Scenecaps Variant Timeline Preview")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 10)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(STextBlock).Text(FText::FromString(TEXT("From:"))).MinDesiredWidth(60)
                ]
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    FromVariantText
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(10, 0, 0, 0)
                [
                    SNew(STextBlock).Text(FText::FromString(TEXT("To:"))).MinDesiredWidth(60)
                ]
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    ToVariantText
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 10)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(STextBlock).Text(FText::FromString(TEXT("Duration:"))).MinDesiredWidth(80)
                ]
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    DurationSpinBox
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(10, 0, 0, 0)
                [
                    SNew(STextBlock).Text(FText::FromString(TEXT("Ease:"))).MinDesiredWidth(50)
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    EaseText
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 10)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(STextBlock).Text(FText::FromString(TEXT("Alpha:"))).MinDesiredWidth(60)
                ]
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    AlphaSlider
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    AlphaText
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10, 0, 0)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Play")))
                    .OnClicked(this, &SScenecapsVariantTimelineTool::OnPlayClicked)
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(5, 0, 0, 0)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Stop")))
                    .OnClicked(this, &SScenecapsVariantTimelineTool::OnStopClicked)
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10, 0, 0)
            [
                StatusText
            ]
        ]
    ];

    UpdateVariantOptions();
}

void SScenecapsVariantTimelineTool::SetTargetAsset(UObject* InAsset)
{
    TargetAsset = InAsset;
    SceneAsset = Cast<UScenecapsAsset>(InAsset);
        UpdateVariantOptions();
    PreviewAlpha = 0.0f;
}

void SScenecapsVariantTimelineTool::Refresh()
{
    UpdateVariantOptions();
}

void SScenecapsVariantTimelineTool::UpdateVariantOptions()
{
    VariantNames.Empty();
    if (SceneAsset.IsValid()) VariantNames = SceneAsset->GetVariantNames();
    
    
    if (SelectedFromVariant.IsNone() && VariantNames.Num() > 0) SelectedFromVariant = VariantNames[0];
    if (SelectedToVariant.IsNone() && VariantNames.Num() > 1) SelectedToVariant = VariantNames[1];
}

float SScenecapsVariantTimelineTool::GetPreviewAlpha() const { return PreviewAlpha; }

void SScenecapsVariantTimelineTool::OnAlphaChanged(float InValue)
{
    PreviewAlpha = InValue;
    ApplyPreview(PreviewAlpha);
}

FText SScenecapsVariantTimelineTool::GetAlphaText() const
{
    return FText::FromString(FString::Printf(TEXT("%.2f"), PreviewAlpha));
}

FReply SScenecapsVariantTimelineTool::OnPlayClicked()
{
    if (!TargetAsset.IsValid() || SelectedFromVariant == SelectedToVariant) return FReply::Handled();
    bIsPlaying = true;
    PlayStartTime = FPlatformTime::Seconds();
    return FReply::Handled();
}

FReply SScenecapsVariantTimelineTool::OnStopClicked()
{
    bIsPlaying = false;
    PreviewAlpha = 0.0f;
    ApplyPreview(0.0f);
    return FReply::Handled();
}

void SScenecapsVariantTimelineTool::ApplyPreview(float Alpha)
{
    // Preview logic here
}

FText SScenecapsVariantTimelineTool::GetFromVariantText() const
{
    return SelectedFromVariant.IsNone() ? FText::FromString(TEXT("<Base>")) : FText::FromString(SelectedFromVariant.ToString());
}

FText SScenecapsVariantTimelineTool::GetToVariantText() const
{
    return SelectedToVariant.IsNone() ? FText::FromString(TEXT("<Base>")) : FText::FromString(SelectedToVariant.ToString());
}

FText SScenecapsVariantTimelineTool::GetStatusText() const
{
    if (!TargetAsset.IsValid()) return FText::FromString(TEXT("No asset selected"));
    if (bIsPlaying) return FText::FromString(FString::Printf(TEXT("Playing... Alpha: %.2f"), PreviewAlpha));
    return FText::FromString(FString::Printf(TEXT("Ready. Alpha: %.2f"), PreviewAlpha));
}

float SScenecapsVariantTimelineTool::GetPreviewDuration() const { return PreviewDuration; }

void SScenecapsVariantTimelineTool::OnDurationChanged(float InValue) { PreviewDuration = InValue; }

FText SScenecapsVariantTimelineTool::GetEaseText() const
{
    static const TArray<FString> EaseNames = {
        TEXT("Linear"),
        TEXT("Quad In"), TEXT("Quad Out"), TEXT("Quad InOut"),
        TEXT("Cubic In"), TEXT("Cubic Out"), TEXT("Cubic InOut"),
        TEXT("Sine In"), TEXT("Sine Out"), TEXT("Sine InOut"),
        TEXT("Expo In"), TEXT("Expo Out"), TEXT("Expo InOut")
    };
    return FText::FromString(EaseNames.IsValidIndex(PreviewEaseIndex) ? EaseNames[PreviewEaseIndex] : TEXT("Unknown"));
}

