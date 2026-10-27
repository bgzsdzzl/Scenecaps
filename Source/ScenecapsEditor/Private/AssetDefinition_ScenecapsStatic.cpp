// Plugins/Scenecaps/Source/ScenecapsEditor/Private/AssetDefinition_ScenecapsStatic.cpp
#include "AssetDefinition_ScenecapsStatic.h"
#include "ScenecapsEditorToolkit.h"

EAssetCommandResult UAssetDefinition_ScenecapsStatic::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
    TArray<UObject*> LoadedObjects = OpenArgs.LoadObjects<UObject>();

    for (UObject* Obj : LoadedObjects)
    {
        if (!Cast<UScenecapsStaticAsset>(Obj)) continue;

        TSharedRef<FScenecapsEditorToolkit> Toolkit = MakeShareable(new FScenecapsEditorToolkit());
        Toolkit->InitEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Obj);
    }

    return EAssetCommandResult::Handled;
}