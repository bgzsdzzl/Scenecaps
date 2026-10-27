// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsStaticFactory.cpp
#include "ScenecapsStaticFactory.h"
#include "ScenecapsStaticAsset.h"
#include "ScenecapsAssetCategories.h"
#include "ScenecapsLocalization.h"
#include "AssetTypeCategories.h"

UScenecapsStaticFactory::UScenecapsStaticFactory()
{
    SupportedClass = UScenecapsStaticAsset::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UScenecapsStaticFactory::FactoryCreateNew(
    UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
    UObject* /*Context*/, FFeedbackContext* /*Warn*/)
{
    return NewObject<UScenecapsStaticAsset>(InParent, InClass, InName, Flags);
}

UClass* UScenecapsStaticFactory::ResolveSupportedClass()
{
    return UScenecapsStaticAsset::StaticClass();
}

FText UScenecapsStaticFactory::GetDisplayName() const
{
    return SC_LOCTEXT(StaticAssetDisplayName, "Scenecaps (Static)");
}

uint32 UScenecapsStaticFactory::GetMenuCategories() const
{
    const uint32 Bit = ScenecapsAssetCategories::GCategoryBit;
    return Bit != 0 ? Bit : EAssetTypeCategories::Misc;
}