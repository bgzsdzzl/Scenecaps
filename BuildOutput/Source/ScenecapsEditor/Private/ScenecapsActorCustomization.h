#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Widgets/Input/SButton.h"
#include "UObject/WeakObjectPtr.h"
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

    FText GetSpawnedCountText() const;
    FText GetDataLayerStatusTextSafe() const;

    TWeakObjectPtr<AScenecapsActor> CachedActor;
};