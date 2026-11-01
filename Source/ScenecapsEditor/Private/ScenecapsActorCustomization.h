#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Framework/Commands/UIAction.h"
#include "Widgets/Input/SButton.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/ObjectPtr.h"
#include "InputCoreTypes.h"

class AScenecapsActor;

class FScenecapsActorCustomization : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    FReply OnSelectAllSpawned();
    FReply OnFocusOnContainer();
    FReply OnRebuildClicked();
    FReply OnClearClicked();

    TWeakObjectPtr<AScenecapsActor> CachedActor;
};