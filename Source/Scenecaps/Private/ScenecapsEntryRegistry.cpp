#include "ScenecapsEntryRegistry.h"

FScenecapsEntryRegistry& FScenecapsEntryRegistry::Get()
{
    static FScenecapsEntryRegistry Instance;
    return Instance;
}

void FScenecapsEntryRegistry::RegisterHandler(TSharedPtr<IScenecapsEntryHandler> Handler)
{
    if (Handler.IsValid())
    {
        Handlers.AddUnique(Handler);
    }
}

void FScenecapsEntryRegistry::UnregisterHandler(const TSharedPtr<IScenecapsEntryHandler>& Handler)
{
    Handlers.Remove(Handler);
}

void FScenecapsEntryRegistry::ClearHandlers()
{
    Handlers.Reset();
}

IScenecapsEntryHandler* FScenecapsEntryRegistry::FindHandlerForAsset(const UObject* Asset) const
{
    if (!Asset)
    {
        return nullptr;
    }

    for (const TSharedPtr<IScenecapsEntryHandler>& Handler : Handlers)
    {
        if (Handler.IsValid() && Handler->CanHandleAsset(Asset))
        {
            return Handler.Get();
        }
    }

    return nullptr;
}

UObject* FScenecapsEntryRegistry::ResolveAssetForStorage(UObject* DroppedObject) const
{
    if (!DroppedObject)
    {
        return nullptr;
    }

    for (const TSharedPtr<IScenecapsEntryHandler>& Handler : Handlers)
    {
        if (Handler.IsValid())
        {
            if (UObject* Result = Handler->ResolveAssetForStorage(DroppedObject))
            {
                return Result;
            }
        }
    }

    return nullptr;
}

UObject* FScenecapsEntryRegistry::ResolveAssetFromActor(AActor* Actor) const
{
    if (!Actor)
    {
        return nullptr;
    }

    for (const TSharedPtr<IScenecapsEntryHandler>& Handler : Handlers)
    {
        if (Handler.IsValid())
        {
            if (UObject* Result = Handler->ResolveAssetFromActor(Actor))
            {
                return Result;
            }
        }
    }

    return nullptr;
}