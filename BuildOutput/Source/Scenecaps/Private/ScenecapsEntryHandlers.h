#pragma once

#include "CoreMinimal.h"
#include "ScenecapsEntryHandler.h"

class SCENECAPS_API FScenecapsBlueprintEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
    virtual UObject* ResolveAssetFromActor(AActor* Actor) const override;
};

class SCENECAPS_API FScenecapsClassEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
    virtual UObject* ResolveAssetFromActor(AActor* Actor) const override;
};

class SCENECAPS_API FScenecapsStaticMeshEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
};

class SCENECAPS_API FScenecapsSoundEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
    virtual UObject* ResolveAssetFromActor(AActor* Actor) const override;
};

class SCENECAPS_API FScenecapsNestedEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
    virtual UObject* ResolveAssetFromActor(AActor* Actor) const override;
};

#if SCENE_COLLECTION_WITH_NIAGARA
class SCENECAPS_API FScenecapsNiagaraEntryHandler : public IScenecapsEntryHandler
{
public:
    virtual bool CanHandleAsset(const UObject* Asset) const override;
    virtual AActor* SpawnActor(const FScenecapsSpawnContext& Context, UObject* Asset) const override;
    virtual UObject* ResolveAssetForStorage(UObject* DroppedObject) const override;
    virtual UObject* ResolveAssetFromActor(AActor* Actor) const override;
};
#endif