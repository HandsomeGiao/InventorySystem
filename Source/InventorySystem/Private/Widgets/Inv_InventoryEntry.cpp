#include "Widgets/Inv_InventoryEntry.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Items/Data/Inv_VirtualItemData.h"
#include "Items/Fragments/VirtualItem/Inv_VirtualItemFragment.h"
#include "InventorySystem.h"
#include "Subsystem/Inv_VirtualItemDataSubsystem.h"
#include "Widgets/Inv_InvDragDrop.h"

void UInv_InventoryEntry::SetInfo(const FInv_RealItemData &RealItemData)
{
    // 打印新ID和旧ID
    UE_LOG(LogInventorySystem, Display,
        TEXT("UInv_InventoryEntry::SetInfo: Setting info for item '%s' (Previous Item ID: '%s')"),
        *RealItemData.RealItemId.ToString(),
        *RealItemData.RealItemId.ToString());
    // 更新当前物品 ID
    CurrentItemData = RealItemData;

    // 获取虚拟数据
    const FInv_VirtualItemData *VirtualData = RealItemData.GetVirtualItemData();
    if (!VirtualData)
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_InventoryEntry::SetInfo: Invalid virtual item data for item '%s'"),
            *RealItemData.RealItemId.ToString());
        ClearEntry();
        return;
    }

    // 更新图标
    UTexture2D *Icon = nullptr;
    if (const FInv_ImageFragment *ImageFragment = VirtualData->GetFragmentOfType<FInv_ImageFragment>())
    {
        Icon = ImageFragment->GetIcon();
    }
    UpdateIcon(Icon);

    // 更新数量
    UpdateCount();

    UE_LOG(LogInventorySystem, Display,
        TEXT("UInv_InventoryEntry::SetInfo: Updated entry with item '%s' (Count: %d)"),
        *VirtualData->ItemTag.ToString(), RealItemData.StackCount);
}

void UInv_InventoryEntry::ClearEntry()
{
    CurrentItemData = FInv_RealItemData();
    // 清空图标
    UpdateIcon(nullptr);

    // 清空数量
    UpdateCount();

    UE_LOG(LogInventorySystem, Display, TEXT("UInv_InventoryEntry::ClearEntry: Entry cleared"));
}

FReply UInv_InventoryEntry::NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent)
{
    // 我们只关心左键拖拽
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && GetCurrentItemId().IsValid())
    {
        // FReply::Handled() 表示我们处理了这个事件。
        // .DetectDrag() 告诉Slate："请开始监视这个控件，如果鼠标移动了，就触发 OnDragDetected。"
        // 我们将 this 作为 "Drag Recipient"，即拖拽事件的响应者。
        return FReply::Handled().DetectDrag(this->TakeWidget(), EKeys::LeftMouseButton);
    }
    else if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && GetCurrentItemId().IsValid())
    {
        // 右键点击事件处理
        OnItemOptions.Broadcast(WidgetIndex);
        UE_LOG(LogInventorySystem, Display,
            TEXT("UInv_InventoryEntry::NativeOnMouseButtonDown: Right click on WidgetIndex:%d"), WidgetIndex);
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

void UInv_InventoryEntry::NativeOnDragDetected(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent,
    UDragDropOperation *&OutOperation)
{
    Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

    if (UInv_InvDragDrop *DragDropOp = NewObject<UInv_InvDragDrop>())
    {
        DragDropOp->SourceItemId = CurrentItemData.RealItemId;
        DragDropOp->SourceWidgetIndex = WidgetIndex;
        DragDropOp->Payload = this;
        DragDropOp->DefaultDragVisual = this;
        DragDropOp->Pivot = EDragPivot::CenterCenter;
        OutOperation = DragDropOp;
        UE_LOG(LogInventorySystem, Display,
            TEXT("UInv_InventoryEntry::NativeOnDragDetected: Created UInv_InvDragDrop operation,Index:%d, item '%s'"),
            WidgetIndex,
            *CurrentItemData.RealItemId.ToString()
            );
    }
    else
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_InventoryEntry::NativeOnDragDetected: Failed to create UInv_InvDragDrop operation"));
    }
}

bool UInv_InventoryEntry::NativeOnDrop(const FGeometry &InGeometry, const FDragDropEvent &InDragDropEvent,
    UDragDropOperation *InOperation)
{
    if (UInv_InvDragDrop *InvDragOp = Cast<UInv_InvDragDrop>(InOperation))
    {
        UInv_InventoryEntry *SourceEntry = Cast<UInv_InventoryEntry>(InvDragOp->Payload);

        // 确保源格子有效且有物品
        if (!SourceEntry->GetCurrentItemData().RealItemId.IsValid())
        {
            UE_LOG(LogInventorySystem, Warning,
                TEXT("UInv_InventoryEntry::NativeOnDrop: Source entry has no valid item to drop"));
            return false;
        }

        // 这里可以处理物品交换逻辑
        UE_LOG(LogInventorySystem, Display,
            TEXT("UInv_InventoryEntry::NativeOnDrop: From WidgetIndex:%d , To WidgetIndex:%d "),
            InvDragOp->SourceWidgetIndex, WidgetIndex);

        // 具体的交换逻辑需要在拥有这个Widget的Inventory Widget中实现
        if (CurrentItemData.RealItemId.IsValid())
            OnItemDrop.Broadcast(InvDragOp->SourceItemId, CurrentItemData.RealItemId);
        else
        {
            //这里是将一个物品移动到空格子上,不需要服务器参与计算,只需要更改视觉效果即可
            SetInfo(SourceEntry->GetCurrentItemData());
            SourceEntry->ClearEntry();
        }
        return true; // 接受放下
    }
    return false;
}

void UInv_InventoryEntry::UpdateIcon(UTexture2D *Icon)
{
    if (!ItemIcon)
    {
        return;
    }

    if (Icon)
    {
        // 设置图标
        ItemIcon->SetBrushFromTexture(Icon);
        ItemIcon->SetOpacity(1.f);
    }
    else
    {
        // 隐藏图标
        ItemIcon->SetOpacity(0.f);
    }
}

void UInv_InventoryEntry::UpdateCount()
{
    //传入空RealItemData则隐藏数量
    if (!ItemCount)
    {
        return;
    }
    const FInv_VirtualItemData *VirtualData = CurrentItemData.GetVirtualItemData();
    if (VirtualData && VirtualData->GetFragmentOfType<FInv_MaxStackFragment>())
    {
        // 显示数量
        ItemCount->SetText(FText::AsNumber(CurrentItemData.StackCount));
    }
    else
    {
        // 隐藏数量（单个物品或空格子不显示数量）
        ItemCount->SetText(FText::GetEmpty());
    }
}