#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "ScenecapsFactory.generated.h"

UCLASS()
class UScenecapsFactory : public UFactory
{
    GENERATED_BODY()

public:
    UScenecapsFactory();

    virtual UObject* FactoryCreateNew(
        UClass* InClass,
        UObject* InParent,
        FName InName,
        EObjectFlags Flags,
        UObject* Context,
        FFeedbackContext* Warn) override;

    virtual UClass* ResolveSupportedClass() override;
    virtual FText GetDisplayName() const override;
    virtual uint32 GetMenuCategories() const override;
    virtual bool ShouldShowInNewMenu() const override { return true; }
};