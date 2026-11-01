#pragma once

#include "CoreMinimal.h"
#include "ScenecapsEntryHandler.h"

class SCENECAPS_API FScenecapsEntryRegistry
{
public:
    static FScenecapsEntryRegistry& Get();

    void RegisterHandler(TSharedPtr<IScenecapsEntryHandler> Handler);
    void UnregisterHandler(const TSharedPtr<IScenecapsEntryHandler>& Handler);
    void ClearHandlers();

    IScenecapsEntryHandler* FindHandlerForAsset(const UObject* Asset) const;
    UObject* ResolveAssetForStorage(UObject* DroppedObject) const;
    UObject* ResolveAssetFromActor(AActor* Actor) const;

private:
    TArray<TSharedPtr<IScenecapsEntryHandler>> Handlers;
};