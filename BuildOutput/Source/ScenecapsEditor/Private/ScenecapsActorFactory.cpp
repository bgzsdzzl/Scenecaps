#include "ScenecapsActorFactory.h"

#include "ScenecapsAsset.h"
#include "ScenecapsActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UScenecapsActorFactory::UScenecapsActorFactory()
{
    NewActorClass = AScenecapsActor::StaticClass();
    bUsePlacementExtent = false;
}

bool UScenecapsActorFactory::CanCreateActorFrom(
    const FAssetData& AssetData, FText& OutErrorMsg)
{
    if (!AssetData.IsValid())
    {
        OutErrorMsg = FText::FromString(TEXT("Invalid asset data"));
        return false;
    }

    if (!AssetData.AssetClassPath.IsNull())
    {
        static const FTopLevelAssetPath ScenecapsClassPath =
            UScenecapsAsset::StaticClass()->GetClassPathName();

        if (AssetData.AssetClassPath == ScenecapsClassPath)
        {
            return true;
        }
    }

    if (UObject* Asset = AssetData.GetAsset())
    {
        if (Asset->IsA<UScenecapsAsset>())
        {
            return true;
        }
    }

    if (const UClass* Class = AssetData.GetClass())
    {
        if (Class->IsChildOf(UScenecapsAsset::StaticClass()))
        {
            return true;
        }
    }

    OutErrorMsg = FText::FromString(TEXT("Not a Scene Collection asset"));
    return false;
}

void UScenecapsActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
    Super::PostSpawnActor(Asset, NewActor);

    UScenecapsAsset* SceneAsset = Cast<UScenecapsAsset>(Asset);
    AScenecapsActor* Container = Cast<AScenecapsActor>(NewActor);
    if (!SceneAsset || !Container) return;

    Container->CollectionAsset = SceneAsset;
    Container->Rebuild();

#if WITH_EDITOR
    Container->SetActorLabel(SceneAsset->GetName());
#endif
}