#include "Widgets/Inv_InventoryEntry.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Items/Data/Inv_VirtualItemData.h"
#include "Items/Fragments/VirtualItem/Inv_VirtualItemFragment.h"
#include "InventorySystem.h"
#include "Widgets/Inv_InventoryWidgetBase.h"
#include "Widgets/Inv_InvDragDrop.h"

void UInv_InventoryEntry::SetInfo(const FInv_RealItemData& RealItemData)
{
	// 打印新ID和旧ID
	UE_LOG(LogInventorySystem, Display,
	       TEXT("UInv_InventoryEntry::SetInfo: Setting info for item '%s' (Previous Item ID: '%s')"),
	       *RealItemData.RealItemId.ToString(),
	       *CurrentItemData.RealItemId.ToString());
	// 更新当前物品 ID
	CurrentItemData = RealItemData;

	// 获取虚拟数据
	const FInv_VirtualItemData* VirtualData = RealItemData.GetVirtualItemData();
	if (!VirtualData)
	{
		UE_LOG(LogInventorySystem, Warning,
		       TEXT("UInv_InventoryEntry::SetInfo: Invalid virtual item data for item '%s'"),
		       *RealItemData.RealItemId.ToString());
		ClearEntry();
		return;
	}

	// 更新图标
	UTexture2D* Icon = nullptr;
	if (const FInv_ImageFragment* ImageFragment = VirtualData->GetFragmentOfType<FInv_ImageFragment>())
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

FReply UInv_InventoryEntry::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && GetCurrentItemId().IsValid())
	{
		return FReply::Handled().DetectDrag(this->TakeWidget(), EKeys::LeftMouseButton);
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && GetCurrentItemId().IsValid())
	{
		// 右键点击事件处理
		OnItemOptions.Broadcast(WidgetIndex);
		UE_LOG(LogInventorySystem, Display,
		       TEXT("UInv_InventoryEntry::NativeOnMouseButtonDown: Right click on WidgetIndex:%d"), WidgetIndex);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInv_InventoryEntry::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
                                               UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!InventoryWidget.IsValid() || !IsValid(InventoryWidget->GetInventory()))
	{
		UE_LOG(LogInventorySystem, Warning,
		       TEXT("UInv_InventoryEntry::NativeOnDragDetected: InventoryWidget or BoundInventory is invalid"));
		return;
	}

	if (UInv_InvDragDrop* DragDropOp = NewObject<UInv_InvDragDrop>())
	{
		DragDropOp->SourceInventory = InventoryWidget->GetInventory();
		DragDropOp->SourceItemId = CurrentItemData.RealItemId;
		DragDropOp->SourceSlotIndex = WidgetIndex;
		DragDropOp->Payload = this;
		DragDropOp->DefaultDragVisual = this;
		DragDropOp->Pivot = EDragPivot::CenterCenter;
		OutOperation = DragDropOp;
		UE_LOG(LogInventorySystem, Display,
		       TEXT("UInv_InventoryEntry::NativeOnDragDetected: Created drag operation, SlotIndex:%d, item '%s'"),
		       WidgetIndex,
		       *CurrentItemData.RealItemId.ToString());
	}
	else
	{
		UE_LOG(LogInventorySystem, Warning,
		       TEXT("UInv_InventoryEntry::NativeOnDragDetected: Failed to create UInv_InvDragDrop operation"));
	}
}

bool UInv_InventoryEntry::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                       UDragDropOperation* InOperation)
{
	if (UInv_InvDragDrop* InvDragOp = Cast<UInv_InvDragDrop>(InOperation))
	{
		if (!InventoryWidget.IsValid())
		{
			return false;
		}

		if (!IsValid(InvDragOp->SourceInventory))
		{
			UE_LOG(LogInventorySystem, Warning,
			       TEXT("UInv_InventoryEntry::NativeOnDrop: SourceInventory is invalid"));
			return false;
		}

		if (InvDragOp->SourceInventory == InventoryWidget->GetInventory() &&
			InvDragOp->SourceSlotIndex == WidgetIndex)
		{
			return false;
		}

		InventoryWidget->RequestItemDrop(InvDragOp->SourceInventory, InvDragOp->SourceItemId, WidgetIndex);

		UE_LOG(LogInventorySystem, Display,
		       TEXT("UInv_InventoryEntry::NativeOnDrop: SourceItem '%s' dropped onto SlotIndex:%d"),
		       *InvDragOp->SourceItemId.ToString(), WidgetIndex);

		return true;
	}
	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UInv_InventoryEntry::UpdateIcon(UTexture2D* Icon)
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
	const FInv_VirtualItemData* VirtualData = CurrentItemData.GetVirtualItemData();
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
