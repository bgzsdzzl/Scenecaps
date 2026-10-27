// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsEditorCommands.h
#pragma once

#include "CoreMinimal.h"

class UScenecapsAsset;
class UScenecapsStaticAsset;
class AActor;

class FScenecapsEditorCommands
{
public:
    static void GatherFromSelection(UScenecapsAsset* Asset);
    static void PlaceIntoCurrentLevel(UScenecapsAsset* Asset);
    static void GatherFromActors(UScenecapsAsset* Asset, const TArray<AActor*>& Actors);

    // Static 版
    static void PlaceStaticIntoCurrentLevel(UScenecapsStaticAsset* Asset);
    static void GatherStaticFromSelection(UScenecapsStaticAsset* Asset);

    // 引用 → 静态
    static UScenecapsStaticAsset* ConvertToStatic(UScenecapsAsset* SourceAsset);

    // 导出为自定义二进制格式（.scnb）
    static bool ExportToCustomBinary(UScenecapsStaticAsset* Asset);
};