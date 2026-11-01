// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsEditorCommands.h
#pragma once

#include "CoreMinimal.h"

class UScenecapsAsset;
class AActor;

class FScenecapsEditorCommands
{
public:
    static void GatherFromSelection(UScenecapsAsset* Asset);
    static void PlaceIntoCurrentLevel(UScenecapsAsset* Asset);
    static void GatherFromActors(UScenecapsAsset* Asset, const TArray<AActor*>& Actors);

    // 自定义二进制导入/导出（.scnb）
    static bool ExportToCustomBinary(UScenecapsAsset* Asset);
    static bool ImportFromCustomBinary(UScenecapsAsset* Asset);
};
