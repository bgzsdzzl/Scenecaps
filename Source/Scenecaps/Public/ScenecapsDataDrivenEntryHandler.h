// Plugins/Scenecaps/Source/Scenecaps/Public/ScenecapsDataDrivenEntryHandler.h

#pragma once

#include "CoreMinimal.h"
#include "ScenecapsEntryHandler.h"
#include "ScenecapsExtensionFormat.h"

class SCENECAPS_API FScenecapsDataDrivenEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
    virtual UObject* ResolveAssetFromActor(AActor* Actor) const override;

    /** 进程级单例。 */
    static FScenecapsDataDrivenEntryHandler& Get();

    /** 从配置目录加载所有 .scx 文件。 */
    void LoadAll();

    /** 清空并重新加载。 */
    void ReloadAll();

    int32 GetEntryCount() const;
    const TArray<FScenecapsSCXEntry>& GetEntries() const { return Entries; }
    TArray<FString> GetLoadedFiles() const;
    TArray<FString> GetErrors() const;

private:
    FScenecapsDataDrivenEntryHandler() = default;

    void  InternalLoadAll(bool bClearFirst);
    bool  LoadFile(const FString& FilePath, TArray<FScenecapsSCXEntry>& OutEntries);
    bool  ParseFile(const TArray<uint8>& Data, const FString& FilePath, TArray<FScenecapsSCXEntry>& OutEntries);
    void  ResolveClass(FScenecapsSCXEntry& Entry);
    void  SortEntries();

    const FScenecapsSCXEntry* FindMatchingEntry(const UObject* Asset) const;

    void  ApplyPropertyOverride(UObject* Target, const FScenecapsSCXProperty& Prop) const;
    bool  ApplyValueToProperty(UObject* Target, FProperty* Property, const FScenecapsSCXProperty& Prop) const;

    mutable FCriticalSection Lock;
    TArray<FScenecapsSCXEntry> Entries;
    TArray<FString> LoadedFiles;
    TArray<FString> Errors;
};