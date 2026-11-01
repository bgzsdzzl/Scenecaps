#pragma once

#include "CoreMinimal.h"

/**
 * Scenecaps 插件的轻量本地化。
 *
 * 不走 UE 的 GatherText / CompileText 管线，直接读 JSON：
 *   - 插件内置：<Plugin>/Localization/<culture>.json
 *   - 项目覆盖：<Project>/Saved/Scenecaps/Localization/<culture>.json
 *
 * 用法：
 *   FText T = SC_LOCTEXT(AssetDisplayName, "Scene Collection");
 *   // 若 zh-Hans.json 里有 "AssetDisplayName": "场景集合"，
 *   // 则 T 是 "场景集合"；否则是 "Scene Collection"。
 *
 * JSON 格式（扁平 map）：
 *   {
 *       "AssetDisplayName": "场景集合",
 *       "ViewportTabLabel": "视口"
 *   }
 */
class SCENECAPS_API FScenecapsLocalization
{
public:
    static FScenecapsLocalization& Get();

    /** 根据当前编辑器/游戏语言重新加载。可在 StartupModule 或用户手动调用。 */
    void Reload();

    /** 查询翻译。找不到 key 时返回 DefaultText。 */
    FText GetText(const TCHAR* Key, const TCHAR* DefaultText) const;

    /** 当前加载的语言代码，例如 "zh-Hans"。仅诊断用。 */
    const FString& GetCurrentCulture() const { return CurrentCulture; }

private:
    FScenecapsLocalization() = default;

    void LoadFromFile(const FString& FilePath);

    TMap<FString, FString> Translations;
    FString CurrentCulture;
    bool bLoaded = false;
};

/**
 * 用法与 UE 的 LOCTEXT 一致：
 *   SC_LOCTEXT(AssetDisplayName, "Scene Collection")
 *
 * Key 必须是裸标识符（不能用引号包）。DefaultText 是字符串字面量。
 */
#define SC_LOCTEXT(Key, DefaultText) \
    FScenecapsLocalization::Get().GetText(TEXT(#Key), TEXT(DefaultText))