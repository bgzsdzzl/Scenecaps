// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsViewportExtension.h

#pragma once

#include "CoreMinimal.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SBox.h"
#include "PrimitiveDrawInterface.h"
#include "InputCoreTypes.h"
#include "EditorViewportClient.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Editor.h"
#include "Engine/EngineTypes.h"

class FToolMenu;
class FViewport;
class FCanvas;
class SWidget;
class SBox;
class FScenecapsEditorToolkit;
class FScenecapsEditorViewportClient;
class UWorld;

enum class EScenecapsViewportExtensionPoint : uint8
{
    Toolbar,
    ContextMenu,
    ViewportOverlay,
    StatusBar,
    ViewportTab,
    CameraControls,
    StatusOverlay,
    Custom
};

class FScenecapsViewportExtension
{
public:
    FScenecapsViewportExtension();

    FName ExtensionName;
    int32 Priority = 0;
    EScenecapsViewportExtensionPoint ExtensionPoint = EScenecapsViewportExtensionPoint::Toolbar;
    bool bEnabled = true;

    virtual void OnRegister(class FScenecapsEditorToolkit* Toolkit) {}
    virtual void OnUnregister() {}
    
    virtual void ExtendToolbar(class UToolMenu* Toolbar, class FScenecapsEditorToolkit* Toolkit) {}
    virtual void ExtendContextMenu(class FToolMenu* Menu, class FScenecapsEditorToolkit* Toolkit, const TArray<AActor*>& SelectedActors) {}
    virtual void DrawViewportOverlay(class FScenecapsEditorViewportClient* ViewportClient, class FPrimitiveDrawInterface* PDI) {}
    virtual void TickViewportOverlay(class FScenecapsEditorViewportClient* ViewportClient, float DeltaSeconds) {}
    virtual bool HandleViewportInput(const FInputEvent& InEvent, class FScenecapsEditorViewportClient* ViewportClient) { return false; }
    virtual void ExtendStatusBar(class SBox* StatusBarContainer, class FScenecapsEditorToolkit* Toolkit) {}
    virtual void ExtendCameraControls(class SBox* Container, class FScenecapsEditorToolkit* Toolkit) {}
    virtual void DrawHUD(class FScenecapsEditorViewportClient* ViewportClient, class FViewport* Viewport, class FCanvas* Canvas) {}
    virtual class UObject* CreateCustomTabContent(class FScenecapsEditorToolkit* Toolkit) { return nullptr; }
    
    virtual bool IsAvailableForAsset(class FScenecapsEditorToolkit* Toolkit) const { return true; }
    virtual bool IsAvailableForMode(uint8 Mode) const { return true; }
};

class FScenecapsViewportExtensionManager
{
public:
    FScenecapsViewportExtensionManager();
    ~FScenecapsViewportExtensionManager();

    static FScenecapsViewportExtensionManager& Get();

    void RegisterExtension(FScenecapsViewportExtension* Extension);
    void UnregisterExtension(FScenecapsViewportExtension* Extension);
    void UnregisterExtensionByName(FName ExtensionName);
    TArray<FScenecapsViewportExtension*> GetExtensionsForPoint(EScenecapsViewportExtensionPoint Point) const;
    void ApplyExtensionsToToolkit(FScenecapsEditorToolkit* Toolkit);
    void RemoveExtensionsFromToolkit(FScenecapsEditorToolkit* Toolkit);

protected:
    TArray<FScenecapsViewportExtension*> RegisteredExtensions;
};