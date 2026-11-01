// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsExtensionFormat.h

#pragma once

#include "CoreMinimal.h"

#define SCENECAPS_SCX_MAGIC   0x31584353  // "SCX1"
#define SCENECAPS_SCX_VERSION 1

enum class EScenecapsSCXValueType : uint8
{
    Bool = 0,
    Int32 = 1,
    Float = 2,
    String = 3,
    Vector = 4,
    Rotator = 5,
    Transform = 6,
    ObjectPath = 7,
};

struct FScenecapsSCXProperty
{
    FString Path;
    EScenecapsSCXValueType ValueType = EScenecapsSCXValueType::Bool;
    TArray<uint8> ValueBytes;

    // ★ 新增：预计算的缓存
    FName CachedComponentName = NAME_None;
    FName CachedPropertyName = NAME_None;
    FProperty* CachedProperty = nullptr;
};

struct FScenecapsSCXEntry
{
    FString AssetClassPath;
    FString ActorClassPath;
    int32   Priority = 0;
    FString Description;
    TArray<FScenecapsSCXProperty> Properties;

    // 解析后缓存
    UClass* ResolvedAssetClass = nullptr;
    UClass* ResolvedActorClass = nullptr;
    FString SourceFile;
};