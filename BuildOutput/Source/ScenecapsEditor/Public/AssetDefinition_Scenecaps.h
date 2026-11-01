
#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "ScenecapsAsset.h"
#include "ScenecapsLocalization.h"
#include "ScenecapsAssetCategories.h"
#include "AssetDefinition_Scenecaps.generated.h"

UCLASS()
class UAssetDefinition_Scenecaps : public UAssetDefinitionDefault
{
    GENERATED_BODY()
public:
    virtual FText GetAssetDisplayName() const override { return SC_LOCTEXT(AssetDisplayName, "Scenecaps"); }
    virtual FLinearColor GetAssetColor() const override { return FLinearColor(0.15f, 0.55f, 0.9f); }
    virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UScenecapsAsset::StaticClass(); }
    virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
    {
        static const TArray<FAssetCategoryPath> Categories = {
            FAssetCategoryPath(SC_LOCTEXT(ScenecapsCategory, "Scenecaps"))
        };
        return Categories;
    }
    virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};