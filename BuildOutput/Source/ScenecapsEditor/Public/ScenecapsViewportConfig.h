// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsViewportConfig.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Commands/UIAction.h"
#include "InputCoreTypes.h"
#include "Framework/Commands/InputChord.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Commands/UICommandList.h"
#include "Textures/SlateIcon.h"
#include "ScenecapsViewportConfig.generated.h"

class UWorld;
class AActor;
class UMaterialInterface;
class UTextureRenderTarget2D;

// Viewport render mode
UENUM(BlueprintType)
enum class EScenecapsViewportRenderMode : uint8
{
    Lit,
    Unlit,
    Wireframe,
    DetailLighting,
    LightingOnly,
    LightComplexity,
    ShaderComplexity,
    StationaryLightOverlap,
    Custom
};

// Viewport show flags preset
UENUM(BlueprintType)
enum class EScenecapsViewportShowFlagsPreset : uint8
{
    Default,
    Game,
    Editor,
    Custom
};

// Camera bookmark
USTRUCT()
struct FScenecapsCameraBookmark
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "CameraBookmark")
    FName Name;

    UPROPERTY(EditAnywhere, Category = "CameraBookmark")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "CameraBookmark")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, Category = "CameraBookmark")
    float OrthoZoom = 1.0f;

    UPROPERTY(EditAnywhere, Category = "CameraBookmark")
    bool bIsOrthographic = false;
};

// Viewport toolbar button
USTRUCT()
struct FScenecapsViewportToolbarButton
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "ToolbarButton")
    FName ActionName;

    UPROPERTY(EditAnywhere, Category = "ToolbarButton")
    FText Label;

    UPROPERTY(EditAnywhere, Category = "ToolbarButton")
    FText ToolTip;

    // FSlateIcon and FUIAction are not USTRUCTs, so they can't be UPROPERTY
    FSlateIcon Icon;
    FUIAction Action;
};

// Viewport gizmo/widget overlay
UENUM(BlueprintType)
enum class EScenecapsViewportOverlayType : uint8
{
    None,
    Grid,
    Axes,
    Bounds,
    Pivot,
    Custom
};

// Viewport overlay config
USTRUCT()
struct FScenecapsViewportOverlayConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Overlay")
    EScenecapsViewportOverlayType Type = EScenecapsViewportOverlayType::None;

    UPROPERTY(EditAnywhere, Category = "Overlay")
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, Category = "Overlay")
    FLinearColor Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, Category = "Overlay")
    float Scale = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Overlay")
    TMap<FName, FString> CustomParams;
};

// Viewport layout section
USTRUCT()
struct FScenecapsViewportLayoutSection
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Layout")
    FName SectionName;

    UPROPERTY(EditAnywhere, Category = "Layout")
    TArray<FName> TabIds;

    UPROPERTY(EditAnywhere, Category = "Layout")
    float SizeCoefficient = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Layout")
    TEnumAsByte<EOrientation> Orientation = Orient_Horizontal;
};

// Per-asset viewport settings
UCLASS(BlueprintType, Blueprintable, config = EditorPerProjectUserSettings, meta = (DisplayName = "Scenecaps Viewport Settings"))
class UScenecapsViewportSettings : public UObject
{
    GENERATED_BODY()

public:
    // ---- Render ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render", config)
    EScenecapsViewportRenderMode DefaultRenderMode = EScenecapsViewportRenderMode::Lit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render", config)
    EScenecapsViewportShowFlagsPreset ShowFlagsPreset = EScenecapsViewportShowFlagsPreset::Default;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render", config)
    bool bShowGrid = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render", config)
    bool bShowBounds = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render", config)
    bool bShowPivot = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render", config)
    bool bShowAxes = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Render", config)
    FLinearColor BackgroundColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);

    // ---- Camera ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", config)
    float DefaultFOV = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", config)
    float DefaultOrthoZoom = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", config)
    float CameraSpeed = 1.0f;

UPROPERTY(EditAnywhere, config, Category = "Camera")
    TArray<FScenecapsCameraBookmark> CameraBookmarks;

    // ---- Overlays ----

    UPROPERTY(EditAnywhere, config, Category = "Overlays")
    TArray<FScenecapsViewportOverlayConfig> OverlayConfigs;

    // ---- Layout ----

    UPROPERTY(EditAnywhere, config, Category = "Layout")
    TArray<FScenecapsViewportLayoutSection> LayoutSections;

    // ---- Toolbar ----

UPROPERTY(EditAnywhere, config, Category = "Toolbar")
    TArray<FScenecapsViewportToolbarButton> CustomToolbarButtons;

    // ---- Advanced ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", config)
    bool bEnableRealtime = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", config)
    bool bAllowCinematicControl = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", config)
    float ExposureCompensation = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced", config)
    TMap<FName, FString> CustomSettings;
};
