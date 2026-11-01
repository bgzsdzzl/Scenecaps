
#include "AssetDefinition_Scenecaps.h"
#include "ScenecapsEditorToolkit.h"

EAssetCommandResult UAssetDefinition_Scenecaps::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
    TArray<UObject*> LoadedObjects = OpenArgs.LoadObjects<UObject>();
    for (UObject* Obj : LoadedObjects)
    {
        if (!Cast<UScenecapsAsset>(Obj)) continue;
        TSharedRef<FScenecapsEditorToolkit> Toolkit = MakeShareable(new FScenecapsEditorToolkit());
        Toolkit->InitEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Obj);
    }
    return EAssetCommandResult::Handled;
}