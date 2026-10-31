// Plugins/Scenecaps/Source/ScenecapsEditor/Private/ScenecapsEditorOutliner.cpp
#include "ScenecapsEditorOutliner.h"
#include "ScenecapsLocalization.h"
#include "ScenecapsAsset.h"
#include "ScenecapsEditorToolkit.h"
#include "ScenecapsEditorAssetProxy.h"
#include "ScenecapsClipboard.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Framework/Application/SlateApplication.h"
#include "ScopedTransaction.h"

// ---------- Drag op ----------
TSharedPtr<SWidget> FScenecapsOutlinerDragDropOp::GetDefaultDecorator() const
{
    FText Label = DraggedEntryIds.Num() == 1
        ? SC_LOCTEXT(DragOne, "Scene Entry")
        : FText::Format(SC_LOCTEXT(DragMany, "{0} Scene Entries"), FText::AsNumber(DraggedEntryIds.Num()));
    return SNew(STextBlock).Text(Label);
}

// ---------- Helpers ----------

static TSharedPtr<FScenecapsEditorAssetProxy> GetProxy(TWeakPtr<FScenecapsEditorToolkit> ToolkitPtr)
{
    if (auto T = ToolkitPtr.Pin()) return T->GetAssetProxy();
    return nullptr;
}

static FTransform AccumulateWorld(TSharedPtr<FScenecapsEditorAssetProxy> Proxy, const FGuid& Target, const FGuid& RootId)
{
    if (!Proxy.IsValid() || Target == RootId) return FTransform::Identity;

    TArray<FTransform> Chain;
    FGuid Cur = Target;
    int32 Guard = 0;
    while (Cur.IsValid() && Cur != RootId && Guard++ < 256)
    {
        FScenecapsEditorEntryView V;
        if (!Proxy->GetEntryView(Cur, V)) break;
        Chain.Insert(V.RelativeTransform, 0);
        Cur = V.ParentId;
    }
    FTransform Result = FTransform::Identity;
    for (const FTransform& X : Chain) Result = X * Result;
    return Result;
}

// ---------- Construct ----------

void SScenecapsEditorOutliner::Construct(const FArguments& InArgs)
{
    ToolkitPtr = InArgs._Toolkit;

    TSharedRef<SVerticalBox> RootBox = SNew(SVerticalBox);

    RootBox->AddSlot().AutoHeight().Padding(4.0f, 4.0f, 4.0f, 2.0f)
        [
            SAssignNew(SearchBoxWidget, SSearchBox)
                .HintText(SC_LOCTEXT(SearchHint, "Search entries..."))
                .OnTextChanged(this, &SScenecapsEditorOutliner::OnFilterChanged)
        ];

    SAssignNew(VariantComboBox, SComboBox<TSharedPtr<FName>>)
        .OptionsSource(&VariantOptions)
        .OnGenerateWidget(this, &SScenecapsEditorOutliner::OnGenerateVariantWidget)
        .OnSelectionChanged(this, &SScenecapsEditorOutliner::OnVariantSelected)
        [SNew(STextBlock).Text(this, &SScenecapsEditorOutliner::GetVariantComboLabel)];

    RootBox->AddSlot().AutoHeight().Padding(4.0f, 0.0f, 4.0f, 2.0f)
        [
            SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
                [SNew(STextBlock).Text(SC_LOCTEXT(VariantLabel, "Variant:"))]
                + SHorizontalBox::Slot().FillWidth(1.0f)[VariantComboBox.ToSharedRef()]
        ];

    TSharedRef<SHorizontalBox> Toolbar = SNew(SHorizontalBox);
    Toolbar->AddSlot().AutoWidth().Padding(0.f, 0.f, 2.f, 0.f)
        [SNew(SButton).Text(SC_LOCTEXT(SelectAllBtn, "All")).OnClicked(this, &SScenecapsEditorOutliner::OnSelectAllClicked)];
    Toolbar->AddSlot().AutoWidth().Padding(0.f, 0.f, 2.f, 0.f)
        [SNew(SButton).Text(SC_LOCTEXT(ExpandBtn, "Expand")).OnClicked(this, &SScenecapsEditorOutliner::OnExpandAllClicked)];
    Toolbar->AddSlot().AutoWidth().Padding(0.f, 0.f, 2.f, 0.f)
        [SNew(SButton).Text(SC_LOCTEXT(CollapseBtn, "Collapse")).OnClicked(this, &SScenecapsEditorOutliner::OnCollapseAllClicked)];
    Toolbar->AddSlot().AutoWidth().Padding(0.f, 0.f, 2.f, 0.f)
        [SNew(SButton).Text(SC_LOCTEXT(HideBtn, "Hide")).OnClicked(this, &SScenecapsEditorOutliner::OnHideSelectedClicked)];
    Toolbar->AddSlot().AutoWidth().Padding(0.f, 0.f, 2.f, 0.f)
        [SNew(SButton).Text(SC_LOCTEXT(ShowBtn, "Show")).OnClicked(this, &SScenecapsEditorOutliner::OnShowSelectedClicked)];
    Toolbar->AddSlot().AutoWidth()
        [SNew(SButton).Text(SC_LOCTEXT(DeleteBtn, "Delete")).OnClicked(this, &SScenecapsEditorOutliner::OnDeleteSelectedClicked)];

    RootBox->AddSlot().AutoHeight().Padding(4.0f, 0.0f, 4.0f, 2.0f)[Toolbar];

    RootBox->AddSlot().FillHeight(1.0f)
        [
            SNew(SBorder).Padding(4.0f)
                [
                    SAssignNew(TreeView, STreeView<TSharedPtr<FScenecapsOutlinerNode>>)
                        .TreeItemsSource(&RootNodes)
                        .SelectionMode(ESelectionMode::Multi)
                        .OnGenerateRow(this, &SScenecapsEditorOutliner::OnGenerateRow)
                        .OnGetChildren(this, &SScenecapsEditorOutliner::OnGetChildren)
                        .OnSelectionChanged(this, &SScenecapsEditorOutliner::OnSelectionChanged)
                ]
        ];

    ChildSlot[RootBox];
    RefreshVariantOptions();
    RefreshTree();
}

// ---------- Variant combo ----------

void SScenecapsEditorOutliner::RefreshVariantOptions()
{
    VariantOptions.Reset();
    VariantOptions.Add(MakeShared<FName>(NAME_None));

    if (auto Proxy = GetProxy(ToolkitPtr))
    {
        for (const FName& N : Proxy->GetVariantNames())
            if (!N.IsNone()) VariantOptions.Add(MakeShared<FName>(N));
    }

    if (VariantComboBox.IsValid())
    {
        FName Current = NAME_None;
        if (auto T = ToolkitPtr.Pin()) Current = T->GetPreviewVariant();
        for (const TSharedPtr<FName>& Item : VariantOptions)
            if (Item.IsValid() && *Item == Current) { VariantComboBox->SetSelectedItem(Item); break; }
        VariantComboBox->RefreshOptions();
    }
}

TSharedRef<SWidget> SScenecapsEditorOutliner::OnGenerateVariantWidget(TSharedPtr<FName> Item)
{
    FText Label = (!Item.IsValid() || Item->IsNone()) ? SC_LOCTEXT(VariantBase, "Base") : FText::FromName(*Item);
    return SNew(STextBlock).Text(Label);
}

FText SScenecapsEditorOutliner::GetVariantComboLabel() const
{
    FName Current = NAME_None;
    if (auto T = ToolkitPtr.Pin()) Current = T->GetPreviewVariant();
    if (Current.IsNone()) return SC_LOCTEXT(VariantBase, "Base");
    return FText::FromName(Current);
}

void SScenecapsEditorOutliner::OnVariantSelected(TSharedPtr<FName> NewItem, ESelectInfo::Type SelectInfo)
{
    if (SelectInfo == ESelectInfo::Direct || !NewItem.IsValid()) return;
    if (auto T = ToolkitPtr.Pin()) T->SetPreviewVariant(*NewItem);
}

// ---------- Variant add/remove ----------

void SScenecapsEditorOutliner::AddSelectedToVariant(FName VariantName)
{
    auto Proxy = GetProxy(ToolkitPtr);
    if (!Proxy.IsValid() || VariantName.IsNone()) return;

    TArray<FGuid> Sel = GetSelectedEntryIds();
    if (Sel.Num() == 0) return;

    TSet<FGuid> ToAdd;
    for (const FGuid& Id : Sel) CollectEntryAndDescendants(Id, ToAdd);

    const FScopedTransaction Transaction(SC_LOCTEXT(AddToVariantTx, "Add Entry to Variant"));
    Proxy->Modify();
    Proxy->AddEntriesToVariant(ToAdd, VariantName);
    Proxy->MarkDirty();
    RefreshTree();
}

void SScenecapsEditorOutliner::RemoveSelectedFromVariant(FName VariantName)
{
    auto Proxy = GetProxy(ToolkitPtr);
    if (!Proxy.IsValid() || VariantName.IsNone()) return;

    TArray<FGuid> Sel = GetSelectedEntryIds();
    if (Sel.Num() == 0) return;

    TSet<FGuid> ToRemove;
    for (const FGuid& Id : Sel) CollectEntryAndDescendants(Id, ToRemove);

    const FScopedTransaction Transaction(SC_LOCTEXT(RemoveFromVariantTx, "Remove Entry from Variant"));
    Proxy->Modify();
    Proxy->RemoveEntriesFromVariant(ToRemove, VariantName);
    Proxy->MarkDirty();
    RefreshTree();
}

// ---------- Filtering / tree ----------

void SScenecapsEditorOutliner::OnFilterChanged(const FText& NewText)
{
    FilterText = NewText.ToString();
    RefreshTree();
}

TSet<FGuid> SScenecapsEditorOutliner::ComputeVisibleSet() const
{
    TSet<FGuid> Visible;
    auto Proxy = GetProxy(ToolkitPtr);
    if (!Proxy.IsValid()) return Visible;

    if (FilterText.IsEmpty())
    {
        Proxy->ForEachEntry([&](const FScenecapsEditorEntryView& V) { Visible.Add(V.EntryId); });
        return Visible;
    }

    TMap<FGuid, FScenecapsEditorEntryView> Map;
    Proxy->ForEachEntry([&](const FScenecapsEditorEntryView& V) { Map.Add(V.EntryId, V); });

    const FGuid RootId = Proxy->GetRootId();
    Proxy->ForEachEntry([&](const FScenecapsEditorEntryView& V)
        {
            const FString AssetName = V.IconAsset.IsNull() ? V.IconClass.GetAssetName() : V.IconAsset.GetAssetName();
            const bool bMatch = V.DisplayName.Contains(FilterText) || AssetName.Contains(FilterText);
            if (!bMatch) return;

            FGuid Cur = V.EntryId;
            int32 Guard = 0;
            while (Cur.IsValid() && Guard++ <= Map.Num())
            {
                Visible.Add(Cur);
                if (!Map.Contains(Cur)) break;
                if (Map[Cur].ParentId == RootId) break;
                Cur = Map[Cur].ParentId;
            }
        });
    return Visible;
}

void SScenecapsEditorOutliner::RefreshTree()
{
    RefreshVariantOptions();

    FGuid PrevSelection;
    if (TreeView.IsValid())
    {
        TArray<TSharedPtr<FScenecapsOutlinerNode>> Sel = TreeView->GetSelectedItems();
        if (Sel.Num() > 0 && Sel[0].IsValid()) PrevSelection = Sel[0]->EntryId;
    }

    RootNodes.Reset();
    AllNodes.Reset();

    auto Proxy = GetProxy(ToolkitPtr);
    if (!Proxy.IsValid())
    {
        if (TreeView.IsValid()) TreeView->RequestTreeRefresh();
        return;
    }

    const TSet<FGuid> VisibleSet = ComputeVisibleSet();

    TMap<FGuid, TSharedPtr<FScenecapsOutlinerNode>> NodeMap;
    Proxy->ForEachEntry([&](const FScenecapsEditorEntryView& V)
        {
            if (!VisibleSet.Contains(V.EntryId)) return;
            auto Node = MakeShared<FScenecapsOutlinerNode>(V.EntryId);
            NodeMap.Add(V.EntryId, Node);
            AllNodes.Add(Node);
        });

    Proxy->ForEachEntry([&](const FScenecapsEditorEntryView& V)
        {
            if (!VisibleSet.Contains(V.EntryId)) return;
            auto Node = NodeMap.FindRef(V.EntryId);
            if (!Node.IsValid()) return;
            auto* ParentPtr = NodeMap.Find(V.ParentId);
            if (ParentPtr && ParentPtr->IsValid()) (*ParentPtr)->Children.Add(Node);
            else RootNodes.Add(Node);
        });

    if (TreeView.IsValid())
    {
        TreeView->RequestTreeRefresh();
        if (PrevSelection.IsValid())
        {
            if (auto Node = FindNode(PrevSelection))
            {
                bSuppressFocusOnSelection = true;
                TreeView->SetSelection(Node);
                bSuppressFocusOnSelection = false;
            }
        }
    }
}

void SScenecapsEditorOutliner::SelectEntry(const FGuid& EntryId)
{
    if (!TreeView.IsValid()) return;
    auto Node = FindNode(EntryId);
    if (!Node.IsValid()) return;

    bSuppressFocusOnSelection = true;
    FScenecapsEditorEntryView V;
    if (auto Proxy = GetProxy(ToolkitPtr))
    {
        if (Proxy->GetEntryView(EntryId, V))
        {
            FGuid Cur = V.ParentId;
            const FGuid RootId = Proxy->GetRootId();
            while (Cur.IsValid())
            {
                if (auto P = FindNode(Cur)) TreeView->SetItemExpansion(P, true);
                if (Cur == RootId) break;
                FScenecapsEditorEntryView PV;
                if (!Proxy->GetEntryView(Cur, PV)) break;
                Cur = PV.ParentId;
            }
        }
    }
    TreeView->SetSelection(Node);
    bSuppressFocusOnSelection = false;
    TreeView->RequestScrollIntoView(Node);
}

const FSlateBrush* SScenecapsEditorOutliner::GetEntryIcon(const FGuid& EntryId) const
{
    auto Proxy = GetProxy(ToolkitPtr);
    if (!Proxy.IsValid()) return FAppStyle::GetBrush("ClassIcon.Object");

    FScenecapsEditorEntryView V;
    if (!Proxy->GetEntryView(EntryId, V)) return FAppStyle::GetBrush("ClassIcon.Object");

    FName ClassName = NAME_None;
    if (!V.IconAsset.IsNull())
    {
        if (UObject* L = V.IconAsset.Get()) ClassName = L->GetClass()->GetFName();
        else
        {
            const FSoftObjectPath P = V.IconAsset.ToSoftObjectPath();
            if (P.IsValid())
            {
                FAssetRegistryModule& RM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
                const FAssetData D = RM.Get().GetAssetByObjectPath(P);
                if (D.IsValid()) if (UClass* C = D.GetClass()) ClassName = C->GetFName();
            }
        }
    }
    else if (!V.IconClass.IsNull())
    {
        if (UClass* C = V.IconClass.Get()) ClassName = C->GetFName();
    }

    if (!ClassName.IsNone())
    {
        const FName IconName(*FString::Printf(TEXT("ClassIcon.%s"), *ClassName.ToString()));
        if (const FSlateBrush* B = FAppStyle::GetBrush(IconName)) return B;
    }
    return FAppStyle::GetBrush("ClassIcon.Object");
}

void SScenecapsEditorOutliner::SRowContextMenuWrapper::Construct(const FArguments& InArgs)
{
    Outliner = InArgs._Outliner;
    ChildSlot[InArgs._Content.Widget];
}

FReply SScenecapsEditorOutliner::SRowContextMenuWrapper::OnMouseButtonUp(
    const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (auto Pin = Outliner.Pin())
            if (TSharedPtr<SWidget> Menu = Pin->OnRowContextMenuOpening())
            {
                FSlateApplication::Get().PushMenu(SharedThis(this), FWidgetPath(), Menu.ToSharedRef(),
                    MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect::ContextMenu);
                return FReply::Handled();
            }
    }
    return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

TSharedRef<ITableRow> SScenecapsEditorOutliner::OnGenerateRow(
    TSharedPtr<FScenecapsOutlinerNode> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    const FGuid EntryId = Item.IsValid() ? Item->EntryId : FGuid();
    TSharedRef<FScenecapsOutlinerNode> ItemRef = Item.ToSharedRef();
    TSharedRef<SHorizontalBox> RowBox = SNew(SHorizontalBox);

    {
        TSharedRef<SButton> Btn = SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "NoBorder")
            .IsEnabled_Lambda([this, EntryId]() { return !IsEntryLocked(EntryId); })
            .OnClicked(FOnClicked::CreateSP(this, &SScenecapsEditorOutliner::OnToggleVisibility, Item));
        Btn->SetContent(SNew(SImage).Image(FAppStyle::GetBrush(IsEntryVisible(EntryId) ? "Icons.Visible" : "Icons.Hidden")));
        RowBox->AddSlot().AutoWidth().Padding(2.f, 0.f)[Btn];
    }
    {
        TSharedRef<SButton> Btn = SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "NoBorder")
            .OnClicked(FOnClicked::CreateSP(this, &SScenecapsEditorOutliner::OnToggleLock, Item));
        Btn->SetContent(SNew(SImage).Image(FAppStyle::GetBrush("Icons.Lock"))
            .ColorAndOpacity_Lambda([this, EntryId]()
                { return IsEntryLocked(EntryId)
                ? FLinearColor(1.f, 0.75f, 0.2f, 1.f)
                : FLinearColor(0.35f, 0.35f, 0.35f, 0.4f); }));
        RowBox->AddSlot().AutoWidth().Padding(2.f, 0.f)[Btn];
    }
    RowBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f)
        [SNew(SBox).WidthOverride(4.f).HeightOverride(16.f)
        [SNew(SImage).Image(FAppStyle::GetBrush("WhiteBrush"))
        .ColorAndOpacity_Lambda([this]()
            {
                if (auto Proxy = GetProxy(ToolkitPtr)) return Proxy->GetEditorColor();
                return FLinearColor(0.15f, 0.55f, 0.9f, 1.f);
            })]];
    RowBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f)
        [SNew(SBox).WidthOverride(16.f).HeightOverride(16.f)
        [SNew(SImage).Image_Lambda([this, EntryId]() { return GetEntryIcon(EntryId); })]];
    RowBox->AddSlot().FillWidth(1.f).VAlign(VAlign_Center)
        [SNew(SInlineEditableTextBlock)
        .Text(GetEntryDisplayName(EntryId))
        .OnTextCommitted(this, &SScenecapsEditorOutliner::OnNameCommitted, Item)
        .IsReadOnly_Lambda([this, EntryId]() { return IsEntryLocked(EntryId); })];
    {
        TSharedRef<SButton> Btn = SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "NoBorder")
            .IsEnabled_Lambda([this, EntryId]() { return !IsEntryLocked(EntryId); })
            .OnClicked(FOnClicked::CreateSP(this, &SScenecapsEditorOutliner::OnDeleteEntry, Item));
        Btn->SetContent(SNew(SImage).Image(FAppStyle::GetBrush("Icons.Delete")));
        RowBox->AddSlot().AutoWidth().Padding(2.f, 0.f)[Btn];
    }

    TSharedRef<SRowContextMenuWrapper> Wrapper = SNew(SRowContextMenuWrapper).Outliner(SharedThis(this));
    Wrapper->SetWrapperContent(RowBox);

    TSharedRef<STableRow<TSharedPtr<FScenecapsOutlinerNode>>> Row =
        SNew(STableRow<TSharedPtr<FScenecapsOutlinerNode>>, OwnerTable)
        .OnDragDetected(this, &SScenecapsEditorOutliner::OnRowDragDetected, ItemRef)
        .OnCanAcceptDrop_Lambda([this](const FDragDropEvent& E, EItemDropZone Z, TSharedPtr<FScenecapsOutlinerNode> T)
            { return OnCanAcceptDrop(E, Z, T); })
        .OnAcceptDrop_Lambda([this](const FDragDropEvent& E, EItemDropZone Z, TSharedPtr<FScenecapsOutlinerNode> T)
            { return OnAcceptDrop(E, Z, T); });
    Row->SetContent(Wrapper);
    return Row;
}

void SScenecapsEditorOutliner::OnGetChildren(
    TSharedPtr<FScenecapsOutlinerNode> Item, TArray<TSharedPtr<FScenecapsOutlinerNode>>& OutChildren)
{
    if (Item.IsValid()) OutChildren = Item->Children;
}

void SScenecapsEditorOutliner::OnSelectionChanged(
    TSharedPtr<FScenecapsOutlinerNode> Item, ESelectInfo::Type SelectInfo)
{
    if (bSuppressFocusOnSelection || SelectInfo == ESelectInfo::Direct || !TreeView.IsValid()) return;
    auto T = ToolkitPtr.Pin();
    if (!T.IsValid()) return;

    TArray<TSharedPtr<FScenecapsOutlinerNode>> Sel = TreeView->GetSelectedItems();
    if (Sel.Num() == 0) { T->ShowAssetDetails(); return; }

    TArray<FGuid> Ids;
    for (const auto& N : Sel) if (N.IsValid() && !IsEntryLocked(N->EntryId)) Ids.Add(N->EntryId);

    if (Ids.Num() == 0) { T->ShowAssetDetails(); return; }
    if (Ids.Num() == 1) T->SelectPreviewActor(Ids[0]);
    else T->ShowEntriesDetails(Ids);
}

// ---------- Context menu ----------

TSharedPtr<SWidget> SScenecapsEditorOutliner::OnRowContextMenuOpening()
{
    const TArray<FGuid> Sel = GetSelectedEntryIds();
    const bool bHas = Sel.Num() > 0;
    const bool bSingle = Sel.Num() == 1;

    auto Proxy = GetProxy(ToolkitPtr);
    FMenuBuilder MenuBuilder(true, nullptr);

    MenuBuilder.BeginSection("ScenecapsEntry", SC_LOCTEXT(EntrySection, "Entry"));
    {
        MenuBuilder.AddMenuEntry(SC_LOCTEXT(CtxDuplicate, "Duplicate"), SC_LOCTEXT(CtxDuplicateTip, "Duplicate (Ctrl+D)"),
            FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]() { OnDuplicateSelected(); }),
                FCanExecuteAction::CreateLambda([bHas]() { return bHas; })));
        MenuBuilder.AddMenuEntry(SC_LOCTEXT(CtxCopy, "Copy"), SC_LOCTEXT(CtxCopyTip, "Copy (Ctrl+C)"),
            FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]() { OnCopySelected(); }),
                FCanExecuteAction::CreateLambda([bHas]() { return bHas; })));
        MenuBuilder.AddMenuEntry(SC_LOCTEXT(CtxPaste, "Paste"), SC_LOCTEXT(CtxPasteTip, "Paste (Ctrl+V)"),
            FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]() { OnPasteEntries(); }),
                FCanExecuteAction::CreateLambda([Proxy]() { return Proxy.IsValid() && Proxy->CanPasteFromClipboard(); })));
        MenuBuilder.AddMenuEntry(SC_LOCTEXT(CtxDelete, "Delete"), SC_LOCTEXT(CtxDeleteTip, "Delete selected"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Delete"),
            FUIAction(FExecuteAction::CreateLambda([this]() { OnDeleteSelectedClicked(); }),
                FCanExecuteAction::CreateLambda([bHas]() { return bHas; })));
        MenuBuilder.AddMenuEntry(SC_LOCTEXT(CtxMoveToRoot, "Move to Root"), SC_LOCTEXT(CtxMoveToRootTip, "Detach from parent"),
            FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]() { OnMoveSelectedToRootClicked(); }),
                FCanExecuteAction::CreateLambda([bHas]() { return bHas; })));
    }
    MenuBuilder.EndSection();

    if (Proxy.IsValid() && Proxy->GetVariantCount() > 0 && bHas)
    {
        MenuBuilder.BeginSection("ScenecapsVariant", SC_LOCTEXT(VariantSection, "Variant"));
        {
            MenuBuilder.AddSubMenu(SC_LOCTEXT(CtxAddToVariant, "Add to Variant"), SC_LOCTEXT(CtxAddToVariantTip, "Add"),
                FNewMenuDelegate::CreateLambda([this, Proxy](FMenuBuilder& Sub)
                    {
                        for (const FName& N : Proxy->GetVariantNames())
                        {
                            if (N.IsNone()) continue;
                            const FName Name = N;
                            Sub.AddMenuEntry(FText::FromName(Name), FText::GetEmpty(), FSlateIcon(),
                                FUIAction(FExecuteAction::CreateLambda([this, Name]() { AddSelectedToVariant(Name); })));
                        }
                    }));
            MenuBuilder.AddSubMenu(SC_LOCTEXT(CtxRemoveFromVariant, "Remove from Variant"), SC_LOCTEXT(CtxRemoveFromVariantTip, "Remove"),
                FNewMenuDelegate::CreateLambda([this, Proxy](FMenuBuilder& Sub)
                    {
                        for (const FName& N : Proxy->GetVariantNames())
                        {
                            if (N.IsNone()) continue;
                            const FName Name = N;
                            Sub.AddMenuEntry(FText::FromName(Name), FText::GetEmpty(), FSlateIcon(),
                                FUIAction(FExecuteAction::CreateLambda([this, Name]() { RemoveSelectedFromVariant(Name); })));
                        }
                    }));
        }
        MenuBuilder.EndSection();
    }

    MenuBuilder.BeginSection("ScenecapsView", SC_LOCTEXT(ViewSection, "View"));
    {
        MenuBuilder.AddMenuEntry(SC_LOCTEXT(CtxFocus, "Focus"), SC_LOCTEXT(CtxFocusTip, "Focus"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Focus"),
            FUIAction(FExecuteAction::CreateLambda([this]() { OnFocusSelected(); }),
                FCanExecuteAction::CreateLambda([bSingle]() { return bSingle; })));
        MenuBuilder.AddMenuEntry(SC_LOCTEXT(CtxExpandAll, "Expand All"), SC_LOCTEXT(CtxExpandAllTip, "Expand"),
            FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]() { OnExpandAllClicked(); })));
        MenuBuilder.AddMenuEntry(SC_LOCTEXT(CtxCollapseAll, "Collapse All"), SC_LOCTEXT(CtxCollapseAllTip, "Collapse"),
            FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]() { OnCollapseAllClicked(); })));
    }
    MenuBuilder.EndSection();

    MenuBuilder.BeginSection("ScenecapsVisibility", SC_LOCTEXT(VisibilitySection, "Visibility"));
    {
        MenuBuilder.AddMenuEntry(SC_LOCTEXT(CtxHide, "Hide"), SC_LOCTEXT(CtxHideTip, "Hide"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Hidden"),
            FUIAction(FExecuteAction::CreateLambda([this]() { OnHideSelectedClicked(); }),
                FCanExecuteAction::CreateLambda([bHas]() { return bHas; })));
        MenuBuilder.AddMenuEntry(SC_LOCTEXT(CtxShow, "Show"), SC_LOCTEXT(CtxShowTip, "Show"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Visible"),
            FUIAction(FExecuteAction::CreateLambda([this]() { OnShowSelectedClicked(); }),
                FCanExecuteAction::CreateLambda([bHas]() { return bHas; })));
    }
    MenuBuilder.EndSection();

    MenuBuilder.BeginSection("ScenecapsSelection", SC_LOCTEXT(SelectionSection, "Selection"));
    {
        MenuBuilder.AddMenuEntry(SC_LOCTEXT(CtxSelectAll, "Select All"), SC_LOCTEXT(CtxSelectAllTip, "Select all"),
            FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]() { OnSelectAllClicked(); })));
    }
    MenuBuilder.EndSection();

    return MenuBuilder.MakeWidget();
}

// ---------- Batch ops ----------

TArray<FGuid> SScenecapsEditorOutliner::GetSelectedEntryIds() const
{
    TArray<FGuid> Result;
    if (!TreeView.IsValid()) return Result;
    for (const auto& N : TreeView->GetSelectedItems())
        if (N.IsValid() && !IsEntryLocked(N->EntryId)) Result.Add(N->EntryId);
    return Result;
}

void SScenecapsEditorOutliner::CollectEntryAndDescendants(const FGuid& EntryId, TSet<FGuid>& OutSet) const
{
    if (OutSet.Contains(EntryId)) return;
    OutSet.Add(EntryId);
    if (auto Proxy = GetProxy(ToolkitPtr))
    {
        TArray<FGuid> Children; Proxy->GetEntryChildren(EntryId, Children);
        for (const FGuid& C : Children) CollectEntryAndDescendants(C, OutSet);
    }
}

void SScenecapsEditorOutliner::DeleteEntries(const TSet<FGuid>& EntryIds)
{
    auto Proxy = GetProxy(ToolkitPtr);
    if (!Proxy.IsValid() || EntryIds.Num() == 0) return;

    for (const FGuid& Id : EntryIds)
    {
        FScenecapsEditorEntryView V;
        if (Proxy->GetEntryView(Id, V) && V.bLocked) return;
    }

    const FScopedTransaction Transaction(SC_LOCTEXT(DeleteEntriesTx, "Delete Scene Entries"));
    Proxy->Modify();
    Proxy->DeleteEntries(EntryIds);
    Proxy->MarkDirty();

    if (auto T = ToolkitPtr.Pin())
    {
        T->RebuildPreviewScene();
        T->RefreshOutliner();
    }
}

void SScenecapsEditorOutliner::SetEntriesVisible(const TArray<FGuid>& EntryIds, bool bVisible)
{
    auto Proxy = GetProxy(ToolkitPtr);
    if (!Proxy.IsValid() || EntryIds.Num() == 0) return;

    const FScopedTransaction Transaction(bVisible
        ? SC_LOCTEXT(ShowEntriesTx, "Show Scene Entries")
        : SC_LOCTEXT(HideEntriesTx, "Hide Scene Entries"));
    Proxy->Modify();

    bool bChanged = false;
    for (const FGuid& Id : EntryIds)
    {
        FScenecapsEditorEntryView V;
        if (!Proxy->GetEntryView(Id, V) || V.bLocked) continue;
        if (V.bVisible != bVisible)
        {
            Proxy->SetEntryVisible(Id, bVisible);
            bChanged = true;
            if (auto T = ToolkitPtr.Pin()) T->UpdatePreviewActorVisibility(Id, bVisible);
        }
    }
    if (bChanged) Proxy->MarkDirty();
    RefreshTree();
}

FReply SScenecapsEditorOutliner::OnSelectAllClicked()
{
    if (!TreeView.IsValid()) return FReply::Handled();
    TreeView->ClearSelection();
    for (const auto& N : AllNodes)
        if (N.IsValid() && !IsEntryLocked(N->EntryId)) TreeView->SetItemSelection(N, true);
    return FReply::Handled();
}

FReply SScenecapsEditorOutliner::OnDeleteSelectedClicked()
{
    const TArray<FGuid> Sel = GetSelectedEntryIds();
    if (Sel.Num() == 0) return FReply::Handled();
    TSet<FGuid> ToRemove;
    for (const FGuid& Id : Sel) CollectEntryAndDescendants(Id, ToRemove);
    DeleteEntries(ToRemove);
    return FReply::Handled();
}

FReply SScenecapsEditorOutliner::OnHideSelectedClicked() { SetEntriesVisible(GetSelectedEntryIds(), false); return FReply::Handled(); }
FReply SScenecapsEditorOutliner::OnShowSelectedClicked() { SetEntriesVisible(GetSelectedEntryIds(), true); return FReply::Handled(); }

FReply SScenecapsEditorOutliner::OnMoveSelectedToRootClicked()
{
    auto T = ToolkitPtr.Pin();
    auto Proxy = GetProxy(ToolkitPtr);
    if (!T.IsValid() || !Proxy.IsValid()) return FReply::Handled();
    const TArray<FGuid> Sel = GetSelectedEntryIds();
    if (Sel.Num() == 0) return FReply::Handled();
    T->ReparentEntries(Sel, Proxy->GetRootId());
    return FReply::Handled();
}

FReply SScenecapsEditorOutliner::OnToggleVisibility(TSharedPtr<FScenecapsOutlinerNode> Item)
{
    auto Proxy = GetProxy(ToolkitPtr);
    if (!Item.IsValid() || !Proxy.IsValid() || IsEntryLocked(Item->EntryId)) return FReply::Handled();
    FScenecapsEditorEntryView V;
    if (!Proxy->GetEntryView(Item->EntryId, V)) return FReply::Handled();

    const FScopedTransaction Transaction(SC_LOCTEXT(ToggleEntryVisibilityTx, "Toggle Scene Entry Visibility"));
    Proxy->Modify();
    Proxy->SetEntryVisible(Item->EntryId, !V.bVisible);
    Proxy->MarkDirty();
    if (auto T = ToolkitPtr.Pin()) T->UpdatePreviewActorVisibility(Item->EntryId, !V.bVisible);
    RefreshTree();
    return FReply::Handled();
}

FReply SScenecapsEditorOutliner::OnToggleLock(TSharedPtr<FScenecapsOutlinerNode> Item)
{
    auto Proxy = GetProxy(ToolkitPtr);
    if (!Item.IsValid() || !Proxy.IsValid()) return FReply::Handled();
    FScenecapsEditorEntryView V;
    if (!Proxy->GetEntryView(Item->EntryId, V)) return FReply::Handled();

    const FScopedTransaction Transaction(SC_LOCTEXT(ToggleEntryLockTx, "Toggle Scene Entry Lock"));
    Proxy->Modify();
    Proxy->SetEntryLocked(Item->EntryId, !V.bLocked);
    Proxy->MarkDirty();
    RefreshTree();
    return FReply::Handled();
}

FReply SScenecapsEditorOutliner::OnDeleteEntry(TSharedPtr<FScenecapsOutlinerNode> Item)
{
    if (!Item.IsValid() || IsEntryLocked(Item->EntryId)) return FReply::Handled();
    TSet<FGuid> ToRemove;
    CollectEntryAndDescendants(Item->EntryId, ToRemove);
    DeleteEntries(ToRemove);
    return FReply::Handled();
}

// ---------- Drag & drop ----------

FReply SScenecapsEditorOutliner::OnRowDragDetected(
    const FGeometry&, const FPointerEvent&, TSharedRef<FScenecapsOutlinerNode> Item)
{
    auto Proxy = GetProxy(ToolkitPtr);
    if (!Proxy.IsValid()) return FReply::Handled();

    TArray<FGuid> ToDrag;
    const TArray<FGuid> Sel = GetSelectedEntryIds();
    if (Sel.Contains(Item->EntryId)) ToDrag = Sel;
    else if (!IsEntryLocked(Item->EntryId)) ToDrag.Add(Item->EntryId);

    TSet<FGuid> DragSet(ToDrag);
    ToDrag.RemoveAll([&](const FGuid& Id)
        {
            FScenecapsEditorEntryView V;
            if (!Proxy->GetEntryView(Id, V)) return true;
            FGuid Cur = V.ParentId;
            int32 Guard = 0;
            while (Cur.IsValid() && Guard++ <= Proxy->GetEntryCount())
            {
                if (DragSet.Contains(Cur)) return true;
                if (Cur == Proxy->GetRootId()) break;
                FScenecapsEditorEntryView PV;
                if (!Proxy->GetEntryView(Cur, PV)) break;
                Cur = PV.ParentId;
            }
            return false;
        });

    if (ToDrag.Num() == 0) return FReply::Handled();
    return FReply::Handled().BeginDragDrop(FScenecapsOutlinerDragDropOp::New(ToDrag));
}

TOptional<EItemDropZone> SScenecapsEditorOutliner::OnCanAcceptDrop(
    const FDragDropEvent& DragDropEvent, EItemDropZone, TSharedPtr<FScenecapsOutlinerNode> TargetItem)
{
    if (!TargetItem.IsValid() || IsEntryLocked(TargetItem->EntryId)) return TOptional<EItemDropZone>();
    auto Op = DragDropEvent.GetOperationAs<FScenecapsOutlinerDragDropOp>();
    if (!Op.IsValid() || Op->DraggedEntryIds.Contains(TargetItem->EntryId)) return TOptional<EItemDropZone>();
    for (const FGuid& DraggedId : Op->DraggedEntryIds)
        if (IsDescendantOf(TargetItem->EntryId, DraggedId)) return TOptional<EItemDropZone>();
    return TOptional<EItemDropZone>(EItemDropZone::OntoItem);
}

FReply SScenecapsEditorOutliner::OnAcceptDrop(
    const FDragDropEvent& DragDropEvent, EItemDropZone, TSharedPtr<FScenecapsOutlinerNode> TargetItem)
{
    if (!TargetItem.IsValid() || IsEntryLocked(TargetItem->EntryId)) return FReply::Handled();
    auto Op = DragDropEvent.GetOperationAs<FScenecapsOutlinerDragDropOp>();
    if (!Op.IsValid()) return FReply::Handled();
    if (auto T = ToolkitPtr.Pin()) T->ReparentEntries(Op->DraggedEntryIds, TargetItem->EntryId);
    return FReply::Handled();
}

// ---------- Naming ----------

void SScenecapsEditorOutliner::OnNameCommitted(
    const FText& NewText, ETextCommit::Type CommitInfo, TSharedPtr<FScenecapsOutlinerNode> Item)
{
    if (CommitInfo != ETextCommit::OnEnter && CommitInfo != ETextCommit::OnUserMovedFocus) return;
    auto Proxy = GetProxy(ToolkitPtr);
    if (!Item.IsValid() || !Proxy.IsValid() || IsEntryLocked(Item->EntryId)) return;

    FScenecapsEditorEntryView V;
    if (!Proxy->GetEntryView(Item->EntryId, V)) return;
    const FString NewName = NewText.ToString();
    if (V.DisplayName == NewName) return;

    const FScopedTransaction Transaction(SC_LOCTEXT(RenameEntryTx, "Rename Scene Entry"));
    Proxy->Modify();
    Proxy->SetEntryDisplayName(Item->EntryId, NewName);
    Proxy->MarkDirty();
}

// ---------- Keyboard ----------

FReply SScenecapsEditorOutliner::OnKeyDown(const FGeometry&, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    const bool bCtrl = InKeyEvent.IsControlDown();
    if (Key == EKeys::Delete) return OnDeleteSelectedClicked();
    if (bCtrl && Key == EKeys::D) return OnDuplicateSelected();
    if (bCtrl && Key == EKeys::C) return OnCopySelected();
    if (bCtrl && Key == EKeys::V) return OnPasteEntries();
    return FReply::Unhandled();
}

// ---------- Copy / Paste / Duplicate ----------

FReply SScenecapsEditorOutliner::OnCopySelected()
{
    auto Proxy = GetProxy(ToolkitPtr);
    if (!Proxy.IsValid()) return FReply::Handled();
    const TArray<FGuid> Sel = GetSelectedEntryIds();
    if (Sel.Num() == 0) return FReply::Handled();

    TSet<FGuid> ToCopy;
    for (const FGuid& Id : Sel) CollectEntryAndDescendants(Id, ToCopy);
    Proxy->CopyEntriesToClipboard(ToCopy);
    return FReply::Handled();
}

FReply SScenecapsEditorOutliner::OnPasteEntries()
{
    auto Proxy = GetProxy(ToolkitPtr);
    auto T = ToolkitPtr.Pin();
    if (!Proxy.IsValid() || !T.IsValid() || !Proxy->CanPasteFromClipboard()) return FReply::Handled();

    FGuid TargetParent = Proxy->GetRootId();
    const TArray<FGuid> Sel = GetSelectedEntryIds();
    if (Sel.Num() == 1) TargetParent = Sel[0];

    const FScopedTransaction Transaction(SC_LOCTEXT(PasteEntriesTx, "Paste Scene Entries"));
    Proxy->Modify();
    Proxy->PasteFromClipboard(TargetParent);
    Proxy->MarkDirty();

    T->RebuildPreviewScene();
    T->RefreshOutliner();
    return FReply::Handled();
}

FReply SScenecapsEditorOutliner::OnDuplicateSelected()
{
    auto T = ToolkitPtr.Pin();
    auto Proxy = GetProxy(ToolkitPtr);
    if (!T.IsValid() || !Proxy.IsValid()) return FReply::Handled();

    const TArray<FGuid> Sel = GetSelectedEntryIds();
    if (Sel.Num() == 0) return FReply::Handled();

    const FScopedTransaction Transaction(SC_LOCTEXT(DuplicateEntriesTx, "Duplicate Scene Entries"));
    Proxy->Modify();

    // Collect all selected entries and their descendants
    TSet<FGuid> ToDuplicate;
    for (const FGuid& Id : Sel) CollectEntryAndDescendants(Id, ToDuplicate);

    // Duplicate entries by copying to clipboard and pasting at the same parent level
    // Save current clipboard state
    TArray<FScenecapsEntry> SavedClipboard;
    if (ScenecapsClipboard::HasData())
    {
        SavedClipboard = ScenecapsClipboard::Get();
    }

    // Copy selected entries to clipboard
    Proxy->CopyEntriesToClipboard(ToDuplicate);

    // Paste each root entry to its original parent
    TSet<FGuid> SelSet(Sel);
    for (const FGuid& Id : Sel)
    {
        FScenecapsEditorEntryView V;
        if (!Proxy->GetEntryView(Id, V)) continue;
        if (!SelSet.Contains(V.ParentId))
        {
            Proxy->PasteFromClipboard(V.ParentId);
        }
    }

    // Restore clipboard
    ScenecapsClipboard::Clear();
    if (SavedClipboard.Num() > 0)
    {
        ScenecapsClipboard::Set(MoveTemp(SavedClipboard));
    }
    Proxy->MarkDirty();

    T->RebuildPreviewScene();
    T->RefreshOutliner();
    return FReply::Handled();
}

FReply SScenecapsEditorOutliner::OnExpandAllClicked()
{
    if (TreeView.IsValid()) for (const auto& N : AllNodes) if (N.IsValid()) TreeView->SetItemExpansion(N, true);
    return FReply::Handled();
}
FReply SScenecapsEditorOutliner::OnCollapseAllClicked()
{
    if (TreeView.IsValid()) for (const auto& N : AllNodes) if (N.IsValid()) TreeView->SetItemExpansion(N, false);
    return FReply::Handled();
}

FReply SScenecapsEditorOutliner::OnFocusSelected()
{
    const TArray<FGuid> Sel = GetSelectedEntryIds();
    if (Sel.Num() != 1) return FReply::Handled();
    if (auto T = ToolkitPtr.Pin()) T->SelectPreviewActor(Sel[0]);
    return FReply::Handled();
}

// ---------- Lookup ----------

FScenecapsEntry* SScenecapsEditorOutliner::FindEntry(const FGuid&) const { return nullptr; }  // 不再直接暴露

FText SScenecapsEditorOutliner::GetEntryDisplayName(const FGuid& EntryId) const
{
    if (auto Proxy = GetProxy(ToolkitPtr))
    {
        FScenecapsEditorEntryView V;
        if (Proxy->GetEntryView(EntryId, V))
        {
            if (!V.DisplayName.IsEmpty()) return FText::FromString(V.DisplayName);
            const FString AssetName = V.IconAsset.IsNull() ? V.IconClass.GetAssetName() : V.IconAsset.GetAssetName();
            if (!AssetName.IsEmpty()) return FText::FromString(AssetName);
        }
    }
    return FText::FromString(TEXT("<Invalid>"));
}

bool SScenecapsEditorOutliner::IsEntryVisible(const FGuid& EntryId) const
{
    if (auto Proxy = GetProxy(ToolkitPtr))
    {
        FScenecapsEditorEntryView V;
        if (Proxy->GetEntryView(EntryId, V)) return V.bVisible;
    }
    return false;
}

bool SScenecapsEditorOutliner::IsEntryLocked(const FGuid& EntryId) const
{
    if (auto Proxy = GetProxy(ToolkitPtr))
    {
        FScenecapsEditorEntryView V;
        if (Proxy->GetEntryView(EntryId, V)) return V.bLocked;
    }
    return false;
}

TSharedPtr<FScenecapsOutlinerNode> SScenecapsEditorOutliner::FindNode(const FGuid& EntryId) const
{
    for (const auto& N : AllNodes)
        if (N.IsValid() && N->EntryId == EntryId) return N;
    return nullptr;
}

bool SScenecapsEditorOutliner::IsDescendantOf(
    const FGuid& PossibleDescendant, const FGuid& PossibleAncestor) const
{
    auto Proxy = GetProxy(ToolkitPtr);
    if (!Proxy.IsValid()) return false;

    FGuid Cur = PossibleDescendant;
    int32 Guard = 0;
    while (Cur.IsValid() && Guard++ < Proxy->GetEntryCount() + 1)
    {
        if (Cur == PossibleAncestor) return true;
        FScenecapsEditorEntryView V;
        if (!Proxy->GetEntryView(Cur, V)) break;
        Cur = V.ParentId;
    }
    return false;
}
