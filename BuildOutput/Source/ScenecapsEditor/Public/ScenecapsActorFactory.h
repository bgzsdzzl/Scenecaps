#pragma once

#include "CoreMinimal.h"
#include "ActorFactories/ActorFactory.h"
#include "ScenecapsActorFactory.generated.h"

UCLASS()
class SCENECAPSEDITOR_API UScenecapsActorFactory : public UActorFactory
{
    GENERATED_BODY()

public:
    UScenecapsActorFactory();

    virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
    virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
};