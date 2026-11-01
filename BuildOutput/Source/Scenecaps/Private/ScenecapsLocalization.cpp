#include "ScenecapsLocalization.h"
#include "ScenecapsModule.h"

#include "Dom/JsonObject.h"
#include "Internationalization/Culture.h"              // ← 新增：FCulture 完整定义
#include "Internationalization/Internationalization.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FScenecapsLocalization& FScenecapsLocalization::Get()
{
    static FScenecapsLocalization Instance;
    return Instance;
}

void FScenecapsLocalization::Reload()
{
    Translations.Reset();
    bLoaded = false;

    // 当前文化代码。UE 里通常返回 "en"、"zh-Hans"、"zh-Hant"、"ja" 等。
    CurrentCulture = TEXT("en");
    if (const FInternationalization& I18N = FInternationalization::Get(); I18N.IsInitialized())
    {
        if (const FCulturePtr Culture = I18N.GetCurrentCulture())
        {
            CurrentCulture = Culture->GetName();
        }
    }

    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Scenecaps"));
    if (Plugin.IsValid())
    {
        const FString PluginFile = FPaths::Combine(
            Plugin->GetBaseDir(), TEXT("Localization"), CurrentCulture + TEXT(".json"));
        LoadFromFile(PluginFile);
    }

    const FString ProjectFile = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("Scenecaps"), TEXT("Localization"),
        CurrentCulture + TEXT(".json"));
    LoadFromFile(ProjectFile);

    bLoaded = true;
}

void FScenecapsLocalization::LoadFromFile(const FString& FilePath)
{
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
    {
        return;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogScenecaps, Warning,
            TEXT("[Scenecaps] 无法解析本地化文件: %s"), *FilePath);
        return;
    }

    for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
    {
        FString Value;
        if (Pair.Value.IsValid() && Pair.Value->TryGetString(Value))
        {
            Translations.Add(Pair.Key, Value);
        }
    }
}

FText FScenecapsLocalization::GetText(const TCHAR* Key, const TCHAR* DefaultText) const
{
    if (!bLoaded)
    {
        const_cast<FScenecapsLocalization*>(this)->Reload();
    }

    if (const FString* Found = Translations.Find(Key))
    {
        return FText::FromString(*Found);
    }
    return FText::FromString(DefaultText);
}