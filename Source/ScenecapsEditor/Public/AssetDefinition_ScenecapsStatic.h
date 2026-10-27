// Plugins/Scenecaps/Source/ScenecapsEditor/Public/AssetDefinition_ScenecapsStatic.h
#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "ScenecapsStaticAsset.h"
#include "ScenecapsLocalization.h"
#include "ScenecapsAssetCategories.h"
#include "AssetDefinition_ScenecapsStatic.generated.h"

UCLASS()
class UAssetDefinition_ScenecapsStatic : public UAssetDefinitionDefault
{
    GENERATED_BODY()

public:
    virtual FText GetAssetDisplayName() const override
    {
        return SC_LOCTEXT(StaticAssetDisplayName, "Scenecaps (Static)");
    }

    virtual FLinearColor GetAssetColor() const override
    {
        return FLinearColor(0.85f, 0.35f, 0.15f);
    }

    virtual TSoftClassPtr<UObject> GetAssetClass() const override
    {
        return UScenecapsStaticAsset::StaticClass();
    }

    virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
    {
        static const TArray<FAssetCategoryPath> Categories =
        {
            FAssetCategoryPath(SC_LOCTEXT(ScenecapsCategory, "Scenecaps"))
        };
        return Categories;
    }

    virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
};