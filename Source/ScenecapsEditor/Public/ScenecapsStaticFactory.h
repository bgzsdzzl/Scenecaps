
#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ScenecapsStaticFactory.generated.h"

UCLASS()
class UScenecapsStaticFactory : public UFactory
{
    GENERATED_BODY()
public:
    UScenecapsStaticFactory();
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
        EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    virtual UClass* ResolveSupportedClass() override;
    virtual FText GetDisplayName() const override;
    virtual uint32 GetMenuCategories() const override;
    virtual bool ShouldShowInNewMenu() const override { return true; }
};