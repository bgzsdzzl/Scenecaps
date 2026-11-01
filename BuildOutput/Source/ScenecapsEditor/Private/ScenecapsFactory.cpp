#include "ScenecapsFactory.h"
#include "ScenecapsAsset.h"
#include "ScenecapsAssetCategories.h"
#include "ScenecapsLocalization.h"
#include "AssetTypeCategories.h"

UScenecapsFactory::UScenecapsFactory()
{
    SupportedClass = UScenecapsAsset::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UScenecapsFactory::FactoryCreateNew(
    UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
    UObject* /*Context*/, FFeedbackContext* /*Warn*/)
{
    return NewObject<UScenecapsAsset>(InParent, InClass, InName, Flags);
}

UClass* UScenecapsFactory::ResolveSupportedClass()
{
    return UScenecapsAsset::StaticClass();
}

FText UScenecapsFactory::GetDisplayName() const
{
    return SC_LOCTEXT(AssetDisplayName, "Scenecaps");
}

uint32 UScenecapsFactory::GetMenuCategories() const
{
    // 读取我们在 Module 里注册的独立分类位掩码
    const uint32 Bit = ScenecapsAssetCategories::GCategoryBit;
    return Bit != 0 ? Bit : EAssetTypeCategories::Misc;
}