#pragma once

#include "CoreMinimal.h"
#include "ActorFactories/ActorFactory.h"
#include "ScenecapsStaticActorFactory.generated.h"

UCLASS()
class SCENECAPSEDITOR_API UScenecapsStaticActorFactory : public UActorFactory
{
    GENERATED_BODY()

public:
    UScenecapsStaticActorFactory();

    virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
    virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
};