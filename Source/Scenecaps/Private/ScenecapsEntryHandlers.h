#pragma once

#include "CoreMinimal.h"
#include "ScenecapsEntryHandler.h"

class FScenecapsBlueprintEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
    virtual UObject* ResolveAssetFromActor(AActor* Actor) const override;
};

class FScenecapsClassEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
    virtual UObject* ResolveAssetFromActor(AActor* Actor) const override;
};

class FScenecapsStaticMeshEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
};

class FScenecapsSoundEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
    virtual UObject* ResolveAssetFromActor(AActor* Actor) const override;
};

class FScenecapsNestedEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
    virtual UObject* ResolveAssetFromActor(AActor* Actor) const override;
};

#if SCENE_COLLECTION_WITH_NIAGARA
class FScenecapsNiagaraEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
    virtual UObject* ResolveAssetFromActor(AActor* Actor) const override;
};
#endif