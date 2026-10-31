// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsViewportConfig.cpp

#include "ScenecapsViewportConfig.h"

UScenecapsViewportSettings::UScenecapsViewportSettings()
    : DefaultRenderMode(EScenecapsViewportRenderMode::Lit)
    , ShowFlagsPreset(EScenecapsViewportShowFlagsPreset::Default)
    , bShowGrid(true)
    , bShowBounds(false)
    , bShowPivot(false)
    , bShowAxes(true)
    , BackgroundColor(FLinearColor(0.15f, 0.15f, 0.15f, 1.0f))
    , DefaultFOV(90.0f)
    , DefaultOrthoZoom(1.0f)
    , CameraSpeed(1.0f)
    , bEnableRealtime(true)
    , bAllowCinematicControl(false)
    , ExposureCompensation(0.0f)
{
}