// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsViewportExtension.cpp

#include "ScenecapsViewportExtension.h"
#include "ScenecapsEditorToolkit.h"
#include "ToolMenus.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "PrimitiveDrawInterface.h"
#include "EditorViewportClient.h"
#include "Editor.h"
#include "Engine/Engine.h"

FScenecapsViewportExtension::FScenecapsViewportExtension()
{
}

FScenecapsViewportExtensionManager::FScenecapsViewportExtensionManager()
{
}

FScenecapsViewportExtensionManager::~FScenecapsViewportExtensionManager()
{
}

FScenecapsViewportExtensionManager& FScenecapsViewportExtensionManager::Get()
{
    static FScenecapsViewportExtensionManager Instance;
    return Instance;
}

void FScenecapsViewportExtensionManager::RegisterExtension(FScenecapsViewportExtension* Extension)
{
    if (!Extension) return;
    if (RegisteredExtensions.ContainsByPredicate([&](FScenecapsViewportExtension* E) { return E == Extension; }))
    {
        return;
    }
    RegisteredExtensions.Add(Extension);
    RegisteredExtensions.Sort([](FScenecapsViewportExtension& A, FScenecapsViewportExtension& B)
    {
        return A.Priority < B.Priority;
    });
}

void FScenecapsViewportExtensionManager::UnregisterExtension(FScenecapsViewportExtension* Extension)
{
    if (!Extension) return;
    RegisteredExtensions.RemoveAll([&](FScenecapsViewportExtension* E) { return E == Extension; });
}

void FScenecapsViewportExtensionManager::UnregisterExtensionByName(FName ExtensionName)
{
    RegisteredExtensions.RemoveAll([&](FScenecapsViewportExtension* E) { return E && E->ExtensionName == ExtensionName; });
}

TArray<FScenecapsViewportExtension*> FScenecapsViewportExtensionManager::GetExtensionsForPoint(EScenecapsViewportExtensionPoint Point) const
{
    TArray<FScenecapsViewportExtension*> Result;
    for (FScenecapsViewportExtension* Ext : RegisteredExtensions)
    {
        if (Ext && Ext->bEnabled && Ext->ExtensionPoint == Point)
        {
            Result.Add(Ext);
        }
    }
    return Result;
}

void FScenecapsViewportExtensionManager::ApplyExtensionsToToolkit(FScenecapsEditorToolkit* Toolkit)
{
    if (!Toolkit) return;

    // Toolbar
    if (UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar"))
    {
        for (FScenecapsViewportExtension* Ext : GetExtensionsForPoint(EScenecapsViewportExtensionPoint::Toolbar))
        {
            if (Ext && Ext->IsAvailableForAsset(Toolkit))
            {
                Ext->ExtendToolbar(Toolbar, Toolkit);
            }
        }
    }

    // Note: Context menu and other extensions are applied dynamically when menu opens
}

void FScenecapsViewportExtensionManager::RemoveExtensionsFromToolkit(FScenecapsEditorToolkit* Toolkit)
{
    // Cleanup handled by toolbar/menu system automatically
}