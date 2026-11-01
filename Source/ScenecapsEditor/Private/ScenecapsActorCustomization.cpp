// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsActorCustomization.cpp

#include "ScenecapsActorCustomization.h"
#include "ScenecapsLocalization.h"

#include "ScenecapsActor.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "ScopedTransaction.h"
#include "PropertyEditorModule.h"
#include "IDetailPropertyRow.h"


TSharedRef<IDetailCustomization> FScenecapsActorCustomization::MakeInstance()
{
    return MakeShareable(new FScenecapsActorCustomization());
}

void FScenecapsActorCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    TArray<TWeakObjectPtr<UObject>> Objects;
    DetailBuilder.GetObjectsBeingCustomized(Objects);
    if (Objects.Num() != 1)
    {
        return;
    }

    CachedActor = Cast<AScenecapsActor>(Objects[0].Get());
    if (!CachedActor.IsValid())
    {
        return;
    }

    IDetailCategoryBuilder& Cat = DetailBuilder.EditCategory(
        TEXT("Scene Collection"), FText::GetEmpty(), ECategoryPriority::Important);

    // 只读计数
    Cat.AddCustomRow(SC_LOCTEXT(SpawnedCount, "Spawned"))
        .NameContent()
        [
            SNew(STextBlock)
                .Text(SC_LOCTEXT(SpawnedCountLabel, "Spawned Actors"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
        ]
        .ValueContent()
        [
            SNew(STextBlock)
                .Text(this, &FScenecapsActorCustomization::GetSpawnedCountText)
                .Font(IDetailLayoutBuilder::GetDetailFont())
        ];

    // ★ 新增：Data Layer 状态只读行
    Cat.AddCustomRow(SC_LOCTEXT(DataLayerStatus, "DataLayer"))
        .NameContent()
        [
            SNew(STextBlock)
                .Text(SC_LOCTEXT(DataLayerStatusLabel, "Data Layer Status"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
        ]
        .ValueContent()
        [
            SNew(STextBlock)
                .Text(this, &FScenecapsActorCustomization::GetDataLayerStatusTextSafe)
                .Font(IDetailLayoutBuilder::GetDetailFont())
        ];

    // 按钮行
    Cat.AddCustomRow(SC_LOCTEXT(Actions, "Actions"))
        .WholeRowContent()
        [
            SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                .Padding(2.f)
                .AutoWidth()
                [
                    SNew(SButton)
                        .Text(SC_LOCTEXT(SelectAll, "Select All"))
                        .ToolTipText(SC_LOCTEXT(SelectAllTip, "Select every Actor spawned by this container"))
                        .OnClicked(this, &FScenecapsActorCustomization::OnSelectAllSpawned)
                ]

                + SHorizontalBox::Slot()
                .Padding(2.f)
                .AutoWidth()
                [
                    SNew(SButton)
                        .Text(SC_LOCTEXT(FocusOn, "Focus"))
                        .ToolTipText(SC_LOCTEXT(FocusOnTip, "Focus the viewport on this container and its contents"))
                        .OnClicked(this, &FScenecapsActorCustomization::OnFocusOnContainer)
                ]

                + SHorizontalBox::Slot()
                .Padding(2.f)
                .AutoWidth()
                [
                    SNew(SButton)
                        .Text(SC_LOCTEXT(Rebuild, "Rebuild"))
                        .ToolTipText(SC_LOCTEXT(RebuildTip, "Clear and regenerate the container contents"))
                        .OnClicked(this, &FScenecapsActorCustomization::OnRebuildClicked)
                ]

                + SHorizontalBox::Slot()
                .Padding(2.f)
                .AutoWidth()
                [
                    SNew(SButton)
                        .Text(SC_LOCTEXT(Clear, "Clear"))
                        .ToolTipText(SC_LOCTEXT(ClearTip, "Destroy every Actor spawned by this container"))
                        .OnClicked(this, &FScenecapsActorCustomization::OnClearClicked)
                ]
        ];
}

FReply FScenecapsActorCustomization::OnSelectAllSpawned()
{
    AScenecapsActor* Actor = CachedActor.Get();
    if (!Actor || !GEditor)
    {
        return FReply::Handled();
    }

    GEditor->SelectNone(false, true);

    for (const TObjectPtr<AActor>& Ptr : Actor->GetSpawnedActors())
    {
        if (AActor* Spawned = Ptr.Get())
        {
            if (IsValid(Spawned))
            {
                GEditor->SelectActor(Spawned, true, true);
            }
        }
    }

    GEditor->NoteSelectionChange();
    return FReply::Handled();
}

FReply FScenecapsActorCustomization::OnFocusOnContainer()
{
    AScenecapsActor* Actor = CachedActor.Get();
    if (!Actor || !GEditor)
    {
        return FReply::Handled();
    }

    FBox Bounds(ForceInit);
    Bounds += Actor->GetComponentsBoundingBox(true);

    for (const TObjectPtr<AActor>& Ptr : Actor->GetSpawnedActors())
    {
        if (AActor* Spawned = Ptr.Get())
        {
            if (IsValid(Spawned))
            {
                Bounds += Spawned->GetComponentsBoundingBox(true);
            }
        }
    }

    if (Bounds.IsValid)
    {
        GEditor->MoveViewportCamerasToBox(Bounds, false);
    }

    return FReply::Handled();
}

FReply FScenecapsActorCustomization::OnRebuildClicked()
{
    if (AScenecapsActor* Actor = CachedActor.Get())
    {
        const FScopedTransaction Transaction(
            SC_LOCTEXT(RebuildScenecaps, "Rebuild Scene Collection"));

        Actor->Modify();
        Actor->Rebuild();
    }
    return FReply::Handled();
}

FReply FScenecapsActorCustomization::OnClearClicked()
{
    if (AScenecapsActor* Actor = CachedActor.Get())
    {
        const FScopedTransaction Transaction(
            SC_LOCTEXT(ClearScenecaps, "Clear Scene Collection"));

        Actor->Modify();
        Actor->Clear();
    }
    return FReply::Handled();
}

FText FScenecapsActorCustomization::GetSpawnedCountText() const
{
    if (AScenecapsActor* A = CachedActor.Get())
    {
        return FText::AsNumber(A->GetSpawnedCount());
    }
    return FText::AsNumber(0);
}

FText FScenecapsActorCustomization::GetDataLayerStatusTextSafe() const
{
    if (AScenecapsActor* A = CachedActor.Get())
    {
        return A->GetDataLayerStatusText();
    }
    return FText::GetEmpty();
}