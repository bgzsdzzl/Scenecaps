// Plugins/Scenecaps/Source/ScenecapsEditor/Public/ScenecapsEditorOutliner.h

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Input/SComboBox.h"
#include "ScenecapsTypes.h"

class UScenecapsAsset;
class FScenecapsEditorToolkit;
class SSearchBox;

class FScenecapsOutlinerDragDropOp : public FDragDropOperation
{
public:
    DRAG_DROP_OPERATOR_TYPE(FScenecapsOutlinerDragDropOp, FDragDropOperation)

        TArray<FGuid> DraggedEntryIds;

    static TSharedRef<FScenecapsOutlinerDragDropOp> New(const TArray<FGuid>& InEntryIds)
    {
        TSharedRef<FScenecapsOutlinerDragDropOp> Op = MakeShareable(new FScenecapsOutlinerDragDropOp());
        Op->DraggedEntryIds = InEntryIds;
        Op->Construct();
        return Op;
    }

    virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;
};

class FScenecapsOutlinerNode : public TSharedFromThis<FScenecapsOutlinerNode>
{
public:
    explicit FScenecapsOutlinerNode(const FGuid& InEntryId) : EntryId(InEntryId) {}

    FGuid EntryId;
    TArray<TSharedPtr<FScenecapsOutlinerNode>> Children;
};

class SScenecapsEditorOutliner : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SScenecapsEditorOutliner) {}
        SLATE_ARGUMENT(TWeakPtr<FScenecapsEditorToolkit>, Toolkit)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void RefreshTree();
    void SelectEntry(const FGuid& EntryId);
    TSharedPtr<SWidget> OnRowContextMenuOpening();

    virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

    class SRowContextMenuWrapper : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SRowContextMenuWrapper) {}
            SLATE_ARGUMENT(TWeakPtr<SScenecapsEditorOutliner>, Outliner)
            SLATE_DEFAULT_SLOT(FArguments, Content)
        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs);
        void SetWrapperContent(TSharedRef<SWidget> InContent) { ChildSlot[InContent]; }
        virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

    private:
        TWeakPtr<SScenecapsEditorOutliner> Outliner;
    };

private:
    // ----- Variant UI -----
    void RefreshVariantOptions();
    TSharedRef<SWidget> OnGenerateVariantWidget(TSharedPtr<FName> Item);
    FText GetVariantComboLabel() const;
    void OnVariantSelected(TSharedPtr<FName> NewItem, ESelectInfo::Type);

    // ----- Variant 操作 -----
    void AddSelectedToVariant(FName VariantName);
    void RemoveSelectedFromVariant(FName VariantName);

    // ----- 其余 -----
    TSharedRef<ITableRow> OnGenerateRow(
        TSharedPtr<FScenecapsOutlinerNode> Item,
        const TSharedRef<STableViewBase>& OwnerTable);

    void OnGetChildren(TSharedPtr<FScenecapsOutlinerNode> Item, TArray<TSharedPtr<FScenecapsOutlinerNode>>& OutChildren);
    void OnSelectionChanged(TSharedPtr<FScenecapsOutlinerNode> Item, ESelectInfo::Type SelectInfo);

    FReply OnToggleVisibility(TSharedPtr<FScenecapsOutlinerNode> Item);
    FReply OnToggleLock(TSharedPtr<FScenecapsOutlinerNode> Item);
    FReply OnDeleteEntry(TSharedPtr<FScenecapsOutlinerNode> Item);

    FReply OnRowDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, TSharedRef<FScenecapsOutlinerNode> Item);
    TOptional<EItemDropZone> OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FScenecapsOutlinerNode> TargetItem);
    FReply OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FScenecapsOutlinerNode> TargetItem);

    void OnNameCommitted(const FText& NewText, ETextCommit::Type CommitInfo, TSharedPtr<FScenecapsOutlinerNode> Item);

    FReply OnCopySelected();
    FReply OnPasteEntries();
    FReply OnDuplicateSelected();
    FReply OnExpandAllClicked();
    FReply OnCollapseAllClicked();
    FReply OnFocusSelected();
    FReply OnSelectAllClicked();
    FReply OnDeleteSelectedClicked();
    FReply OnHideSelectedClicked();
    FReply OnShowSelectedClicked();
    FReply OnMoveSelectedToRootClicked();

    TArray<FGuid> GetSelectedEntryIds() const;
    void CollectEntryAndDescendants(const FGuid& EntryId, TSet<FGuid>& OutSet) const;
    void DeleteEntries(const TSet<FGuid>& EntryIds);
    void SetEntriesVisible(const TArray<FGuid>& EntryIds, bool bVisible);

    void OnFilterChanged(const FText& NewText);
    TSet<FGuid> ComputeVisibleSet() const;

    const FSlateBrush* GetEntryIcon(const FGuid& EntryId) const;
    FText GetEntryDisplayName(const FGuid& EntryId) const;
    bool IsEntryVisible(const FGuid& EntryId) const;
    bool IsEntryLocked(const FGuid& EntryId) const;

    TSharedPtr<FScenecapsOutlinerNode> FindNode(const FGuid& EntryId) const;
    bool IsDescendantOf(const FGuid& PossibleDescendant, const FGuid& PossibleAncestor) const;

private:
    TWeakPtr<FScenecapsEditorToolkit> ToolkitPtr;

    TSharedPtr<SSearchBox> SearchBoxWidget;
    TSharedPtr<STreeView<TSharedPtr<FScenecapsOutlinerNode>>> TreeView;
    TArray<TSharedPtr<FScenecapsOutlinerNode>> RootNodes;
    TArray<TSharedPtr<FScenecapsOutlinerNode>> AllNodes;

    // ★ 变体下拉框
    TSharedPtr<SComboBox<TSharedPtr<FName>>> VariantComboBox;
    TArray<TSharedPtr<FName>> VariantOptions;

    FString FilterText;
    bool bSuppressFocusOnSelection = false;
};