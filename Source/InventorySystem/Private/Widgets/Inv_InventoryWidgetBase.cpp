#include "Widgets/Inv_InventoryWidgetBase.h"
#include "Widgets/Inv_InventoryEntry.h"
#include "Components/UniformGridPanel.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/Inv_InventoryBase.h"
#include "InventorySystem.h"
#include "Widgets/Inv_ItemOptionsWidget.h"

void UInv_InventoryWidgetBase::AddItem(const FInv_RealItemData &ItemData)
{
    UpdateSlot(ItemData);
}

void UInv_InventoryWidgetBase::UpdateItem(const FInv_RealItemData &ItemData)
{
    UpdateSlot(ItemData);
}

void UInv_InventoryWidgetBase::RemoveItem(const FGuid &ItemId)
{
    const int32 SlotIndex = FindSlotIdxByItemId(ItemId);
    if (SlotIndex == INDEX_NONE)
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_PlayerInventoryWidget::RemoveItem: Item '%s' not found in any slot"),
            *ItemId.ToString());
        return;
    }

    ClearSlot(SlotIndex);

    UE_LOG(LogInventorySystem, Verbose,
        TEXT("UInv_PlayerInventoryWidget::RemoveItem: Removed item '%s' from slot %d"),
        *ItemId.ToString(), SlotIndex);
}

void UInv_InventoryWidgetBase::InitializeInventory(int32 InTotalSlots, int32 InColumnsPerRow)
{
    if (!InventoryGrid)
    {
        UE_LOG(LogInventorySystem, Error,
            TEXT("UInv_PlayerInventoryWidget::InitializeInventory: InventoryGrid is null!"));
        return;
    }

    if (InTotalSlots <= 0)
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_PlayerInventoryWidget::InitializeInventory: Invalid SlotCount %d"), InTotalSlots);
        return;
    }

    // save info
    TotalSlots = InTotalSlots;
    ColumnsPerRow = InColumnsPerRow;

    // 保存列数配置
    if (ColumnsPerRow <= 0)
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_PlayerInventoryWidget::InitializeInventory: Invalid ColumnsPerRow %d, defaulting to 1"),
            ColumnsPerRow);
        ColumnsPerRow = 1;
    }

    // 清空现有格子
    InventoryGrid->ClearChildren();
    InventoryEntries.Empty();
    ItemIdToSlotIndex.Empty();

    // 创建格子
    for (int32 i = 0; i < TotalSlots; ++i)
    {
        UInv_InventoryEntry *Entry = CreateEntryWidget();
        if (!Entry)
        {
            UE_LOG(LogInventorySystem, Error,
                TEXT("UInv_PlayerInventoryWidget::InitializeInventory: Failed to create entry widget at index %d"), i);
            continue;
        }

        Entry->SetWidgetIndex(i);
        Entry->SetInventoryWidget(this);
        InventoryEntries.Add(Entry);
        Entry->OnItemOptions.AddDynamic(this, &UInv_InventoryWidgetBase::OnItemOptionsFunc);

        // 添加到网格（如果是 UniformGridPanel）
        const int32 Row = i / ColumnsPerRow;
        const int32 Column = i % ColumnsPerRow;
        InventoryGrid->AddChildToUniformGrid(Entry, Row, Column);
    }

    UE_LOG(LogInventorySystem, Log,
        TEXT("UInv_PlayerInventoryWidget::InitializeInventory: Created %d inventory slots (%d columns per row)"),
        TotalSlots, ColumnsPerRow);
}

void UInv_InventoryWidgetBase::SetInventory(UInv_InventoryBase* InInventory)
{
    BoundInventory = InInventory;
}

void UInv_InventoryWidgetBase::RequestItemDrop(UInv_InventoryBase* SourceInventory, const FGuid& SourceItemId,
                                               int32 TargetSlotIndex)
{
    UInv_InventoryBase* TargetInventory = BoundInventory.Get();
    if (!IsValid(SourceInventory) || !IsValid(TargetInventory))
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_InventoryWidgetBase::RequestItemDrop: SourceInventory or TargetInventory is invalid"));
        return;
    }

    UInv_InventoryBase* RequestInventory = nullptr;
    if (SourceInventory->CanSendServerCommand())
    {
        RequestInventory = SourceInventory;
    }
    else if (TargetInventory->CanSendServerCommand())
    {
        RequestInventory = TargetInventory;
    }

    if (!IsValid(RequestInventory))
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_InventoryWidgetBase::RequestItemDrop: Failed to find a valid inventory to send server RPC"));
        return;
    }

    RequestInventory->RequestMoveItem(SourceInventory, SourceItemId, TargetInventory, TargetSlotIndex);
}

void UInv_InventoryWidgetBase::UpdateInventory(const TArray<FInv_RealItemData> &ItemDataArray)
{
    // 先清空所有格子
    ClearAllSlots();

    // 更新每个有物品的格子
    for (const FInv_RealItemData &ItemData : ItemDataArray)
    {
        AddItem(ItemData);
    }

    UE_LOG(LogInventorySystem, Verbose,
        TEXT("UInv_PlayerInventoryWidget::UpdateInventory: Updated inventory with %d items"), ItemDataArray.Num());
}

void UInv_InventoryWidgetBase::UpdateSlot(const FInv_RealItemData &ItemData)
{
    const int32 SlotIndex = ItemData.SlotIndex;
    if (!IsValidSlotIndex(SlotIndex))
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_PlayerInventoryWidget::UpdateSlot: Invalid SlotIndex %d for item '%s'"),
            SlotIndex, *ItemData.RealItemId.ToString());
        return;
    }

    const int32 ExistingSlotIndex = FindSlotIdxByItemId(ItemData.RealItemId);
    if (ExistingSlotIndex != INDEX_NONE && ExistingSlotIndex != SlotIndex)
    {
        ClearSlot(ExistingSlotIndex);
    }

    const FGuid PreviousItemId = InventoryEntries[SlotIndex]->GetCurrentItemId();
    if (PreviousItemId.IsValid() && PreviousItemId != ItemData.RealItemId)
    {
        ItemIdToSlotIndex.Remove(PreviousItemId);
    }

    InventoryEntries[SlotIndex]->SetInfo(ItemData);
    ItemIdToSlotIndex.FindOrAdd(ItemData.RealItemId) = SlotIndex;

    UE_LOG(LogInventorySystem, Verbose,
        TEXT("UInv_PlayerInventoryWidget::UpdateSlot: Updated slot %d with item '%s'"),
        SlotIndex, *ItemData.RealItemId.ToString());
}

void UInv_InventoryWidgetBase::ClearSlot(int32 SlotIndex)
{
    if (!IsValidSlotIndex(SlotIndex))
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_PlayerInventoryWidget::ClearSlot: Invalid slot index %d"), SlotIndex);
        return;
    }

    const FGuid ItemId = InventoryEntries[SlotIndex]->GetCurrentItemId();
    if (ItemId.IsValid())
    {
        ItemIdToSlotIndex.Remove(ItemId);
    }

    InventoryEntries[SlotIndex]->ClearEntry();

    UE_LOG(LogInventorySystem, Verbose, TEXT("UInv_PlayerInventoryWidget::ClearSlot: Cleared slot %d"), SlotIndex);
}

void UInv_InventoryWidgetBase::ClearAllSlots()
{
    ItemIdToSlotIndex.Empty();

    for (UInv_InventoryEntry *Entry : InventoryEntries)
    {
        if (Entry)
        {
            Entry->ClearEntry();
        }
    }

    UE_LOG(LogInventorySystem, Verbose, TEXT("UInv_PlayerInventoryWidget::ClearAllSlots: Cleared all slots"));
}

int32 UInv_InventoryWidgetBase::FindSlotIdxByItemId(const FGuid &ItemId) const
{
    if (!ItemId.IsValid())
    {
        return INDEX_NONE;
    }

    if (const int32 *SlotIndex = ItemIdToSlotIndex.Find(ItemId))
    {
        return IsValidSlotIndex(*SlotIndex) ? *SlotIndex : INDEX_NONE;
    }

    for (int32 i = 0; i < InventoryEntries.Num(); ++i)
    {
        if (InventoryEntries[i] && InventoryEntries[i]->GetCurrentItemId() == ItemId)
        {
            return i;
        }
    }

    return INDEX_NONE;
}

void UInv_InventoryWidgetBase::OnItemOptionsFunc(int32 WidgetIndex)
{
    if (!InventoryEntries.IsValidIndex(WidgetIndex) || !InventoryEntries[WidgetIndex] ||
        !InventoryEntries[WidgetIndex]->GetCurrentItemId().IsValid())
    {
        return;
    }

    if (ItemOptionsWidgetInstance.IsValid())
    {
        ItemOptionsWidgetInstance->RemoveFromParent();
    }

    // 创建一个Inv_ItemOptionsWidget,使其显示在鼠标位置
    ItemOptionsWidgetInstance = CreateWidget<UInv_ItemOptionsWidget>(this, ItemOptionsWidgetClass);
    if (ItemOptionsWidgetInstance.IsValid())
    {
        ItemOptionsWidgetInstance->AddToViewport();

        //得到鼠标位置
        FVector2D MousePosition(FVector2D::Zero());
        if (APlayerController *PC = GetWorld()->GetFirstPlayerController())
        {
            PC->GetMousePosition(MousePosition.X, MousePosition.Y);
        }
        else
        {
            UE_LOG(LogInventorySystem, Warning,
                TEXT("UInv_InventoryWidgetBase::OnItemOptionsFunc: Failed to get PlayerController for mouse position"));
        }

        // 需要注意的是,如果这里的mouse pisition超出屏幕范围,可能导致显示异常,需要进行调整
        MousePosition.X = FMath::Clamp(MousePosition.X, 0.f, GEngine->GameViewport->Viewport->GetSizeXY().X - 300.f);
        MousePosition.Y = FMath::Clamp(MousePosition.Y, 0.f, GEngine->GameViewport->Viewport->GetSizeXY().Y - 300.f);
        ItemOptionsWidgetInstance->SetPositionInViewport(MousePosition); // 可以根据需要调整位置
        ItemOptionsWidgetInstance->SetItemInfo(InventoryEntries[WidgetIndex]->GetCurrentItemId(),
            InventoryEntries[WidgetIndex]->GetCurrentItemData().StackCount);
        ItemOptionsWidgetInstance->OnItemSplit.AddDynamic(this, &ThisClass::OnItemSplitFunc);
    }
}

void UInv_InventoryWidgetBase::OnItemSplitFunc(FGuid ItemId, int32 SplitCount)
{
    OnItemSplit.Broadcast(ItemId, SplitCount);
}

UInv_InventoryEntry *UInv_InventoryWidgetBase::CreateEntryWidget()
{
    // 检查是否设置了 Widget 类
    if (!EntryWidgetClass)
    {
        UE_LOG(LogInventorySystem, Error,
            TEXT(
                "UInv_PlayerInventoryWidget::CreateEntryWidget: EntryWidgetClass is not set! Please set it in Blueprint."
            ));
        return nullptr;
    }

    // 创建 Widget 实例
    UInv_InventoryEntry *Entry = CreateWidget<UInv_InventoryEntry>(this, EntryWidgetClass);
    if (!Entry)
    {
        UE_LOG(LogInventorySystem, Error,
            TEXT("UInv_PlayerInventoryWidget::CreateEntryWidget: Failed to create widget of class '%s'"),
            *EntryWidgetClass->GetName());
        return nullptr;
    }

    return Entry;
}

bool UInv_InventoryWidgetBase::IsValidSlotIndex(int32 SlotIndex) const
{
    return InventoryEntries.IsValidIndex(SlotIndex) && InventoryEntries[SlotIndex] != nullptr;
}
