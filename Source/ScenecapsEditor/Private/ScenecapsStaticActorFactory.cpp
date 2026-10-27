
#include "ScenecapsStaticActorFactory.h"
#include "ScenecapsStaticAsset.h"
#include "ScenecapsStaticActor.h"

UScenecapsStaticActorFactory::UScenecapsStaticActorFactory()
{
    NewActorClass = AScenecapsStaticActor::StaticClass();
    bUsePlacementExtent = false;
}
bool UScenecapsStaticActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
    if (UObject* Obj = AssetData.GetAsset())
        if (Obj->IsA<UScenecapsStaticAsset>()) return true;
    OutErrorMsg = FText::FromString(TEXT("Not a Static Scene Collection asset"));
    return false;
}
void UScenecapsStaticActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
    Super::PostSpawnActor(Asset, NewActor);
    if (auto* SA = Cast<UScenecapsStaticAsset>(Asset))
        if (auto* Container = Cast<AScenecapsStaticActor>(NewActor))
        {
            Container->CollectionAsset = SA;
            Container->Rebuild();
#if WITH_EDITOR
            Container->SetActorLabel(SA->GetName());
#endif
        }
}