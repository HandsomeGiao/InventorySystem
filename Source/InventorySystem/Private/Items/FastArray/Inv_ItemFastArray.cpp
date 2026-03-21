#include "Items/FastArray/Inv_ItemFastArray.h"

#include "InventorySystem.h"
#include "Items/Fragments/VirtualItem/Inv_VirtualItemFragment.h"

// ========== FInv_ItemList 实现 ==========

void FInv_ItemList::PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize)
{
    for (const int32 Index : RemovedIndices)
    {
        if (Items.IsValidIndex(Index))
        {
            const FGuid RemovedItemId = Items[Index].GetItemId();
            UE_LOG(LogInventorySystem, Verbose,
                TEXT("FInv_ItemList::PreReplicatedRemove: Item '%s' at index %d removed via replication"),
                *RemovedItemId.ToString(), Index);

            // 广播移除事件
            OnItemRemoved.Broadcast(RemovedItemId);
        }
    }
}

void FInv_ItemList::PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize)
{
    for (const int32 Index : AddedIndices)
    {
        if (Items.IsValidIndex(Index))
        {
            const FInv_RealItemData &AddedItemData = Items[Index].RealItemData;
            UE_LOG(LogInventorySystem, Verbose,
                TEXT("FInv_ItemList::PostReplicatedAdd: Item '%s' at index %d added via replication"),
                *AddedItemData.RealItemId.ToString(), Index);

            // 广播添加事件
            OnItemAdded.Broadcast(AddedItemData);
        }
    }
}

void FInv_ItemList::PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize)
{
    for (const int32 Index : ChangedIndices)
    {
        if (Items.IsValidIndex(Index))
        {
            const FInv_RealItemData &ChangedItemData = Items[Index].RealItemData;
            UE_LOG(LogInventorySystem, Verbose,
                TEXT("FInv_ItemList::PostReplicatedChange: Item '%s' at index %d changed via replication"),
                *ChangedItemData.RealItemId.ToString(), Index);

            // 广播变化事件
            OnItemChanged.Broadcast(ChangedItemData);
        }
    }
}

bool FInv_ItemList::TryAddNewItem(const FInv_RealItemData &ItemData)
{
    if (!IsServer())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::AddItem: Only server can add items!"));
        return false;
    }

    // 检查是否已存在相同 ID 的物品
    if (ContainRealItem(ItemData.RealItemId))
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::AddItem: Item with ID '%s' already exists!"),
            *ItemData.RealItemId.ToString());
        return false;
    }

    // 添加到数组
    FInv_ItemFastArrayItem &NewItem = Items.AddDefaulted_GetRef();
    NewItem.RealItemData = ItemData;

    // 标记为已修改（触发网络同步）
    MarkItemDirty(NewItem);

    UE_LOG(LogInventorySystem, Display,
        TEXT("FInv_ItemList::AddItem: Added item Successfully '%s' (GUID: %s), StackCount: %d"),
        *ItemData.GetVirtualItemData()->ItemTag.ToString(), *ItemData.RealItemId.ToString(), ItemData.StackCount);

    // broadcast event on client which is also a server
    if (OwnerComponent.IsValid())
    {
        ENetMode Nm = OwnerComponent->GetNetMode();
        if (Nm == NM_ListenServer || Nm == NM_Standalone)
            OnItemAdded.Broadcast(ItemData);
    }

    return true;
}

int32 FInv_ItemList::TryStackItem(const FInv_RealItemData &NewRealItem)
{
    if (!IsServer())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::TryStackItem: Only server can stack items!"));
        return 0;
    }

    // 检查是否已存在相同 ID 的物品
    if (ContainRealItem(NewRealItem.RealItemId))
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::AddItem: Item with ID '%s' already exists!"),
            *NewRealItem.RealItemId.ToString());
        return 0;
    }

    int32 NeedToFill = NewRealItem.StackCount;
    const FInv_StackFragment *StackFragment = NewRealItem.GetVirtualItemData()->GetFragmentOfType<FInv_StackFragment>();
    // 无法堆叠
    if (!StackFragment)
        return 0;
    const int32 MaxStack = StackFragment->MaxStackCount;

    for (FInv_ItemFastArrayItem &ItemEntry : Items)
    {
        if (NeedToFill <= 0)
            break;
        if (ItemEntry.RealItemData.VirtualItemDataHandle != NewRealItem.VirtualItemDataHandle)
            continue;
        // 执行堆叠
        const int32 SurplusStackCount = MaxStack - ItemEntry.RealItemData.StackCount;
        if (SurplusStackCount <= 0)
            continue;
        const int32 TrueStackCount = FMath::Min(SurplusStackCount, NeedToFill);
        ItemEntry.RealItemData.StackCount += TrueStackCount;
        NeedToFill -= TrueStackCount;

        // 标记为已修改
        MarkItemDirty(ItemEntry);

        UE_LOG(LogInventorySystem, Display,
            TEXT("FInv_ItemList::TryStackItem: Stacked item ID: '%s' (New StackCount: %d)"),
            *ItemEntry.RealItemData.RealItemId.ToString(), ItemEntry.RealItemData.StackCount);

        // broadcast event on client which is also a server
        if (OwnerComponent.IsValid())
        {
            ENetMode Nm = OwnerComponent->GetNetMode();
            if (Nm == NM_ListenServer || Nm == NM_Standalone)
                OnItemChanged.Broadcast(ItemEntry.RealItemData);
        }
    }
    return NewRealItem.StackCount - NeedToFill;
}

void FInv_ItemList::TryDropItem(FGuid SourceItemId, FGuid TargetItemId)
{
    if (!IsServer())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::TryDropItem: Only server can drop items!"));
        return;
    }

    const int32 SourceIndex = IndexOfItem(SourceItemId);
    const int32 TargetIndex = IndexOfItem(TargetItemId);

    if (SourceItemId == TargetItemId)
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("FInv_ItemList::TryDropItem: Source item '%s' and target item are the same, no action taken."),
            *SourceItemId.ToString());
        return;
    }

    if (SourceIndex == INDEX_NONE || TargetIndex == INDEX_NONE)
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("FInv_ItemList::TryDropItem: Source item '%s' or target item '%s' not found!"),
            *SourceItemId.ToString(), *TargetItemId.ToString());
        return;
    }

    FInv_RealItemData &SourceItemData = Items[SourceIndex].RealItemData;
    FInv_RealItemData &TargetItemData = Items[TargetIndex].RealItemData;
    if (SourceItemData.VirtualItemDataHandle != TargetItemData.VirtualItemDataHandle)
    {
        UE_LOG(LogInventorySystem, Display,
            TEXT("FInv_ItemList::TryDropItem: Cannot drop item '%s' onto different item '%s'"),
            *SourceItemId.ToString(), *TargetItemId.ToString());
        return;
    }

    // 相同的物品种类,检测能否堆叠
    const FInv_VirtualItemData *VirtualItemData = Items[SourceIndex].RealItemData.GetVirtualItemData();
    const FInv_StackFragment *StackFragment = VirtualItemData
                                                  ? VirtualItemData->GetFragmentOfType<FInv_StackFragment>()
                                                  : nullptr;
    const int32 MaxStack = StackFragment ? StackFragment->MaxStackCount : 1;
    const int32 CanStackCount = FMath::Min(SourceItemData.StackCount, MaxStack - TargetItemData.StackCount);
    // 打印上述变量
    UE_LOG(LogInventorySystem, Display,
        TEXT(
            "FInv_ItemList::TryDropItem: Dropping item '%s' onto item '%s'. MaxStack: %d, SourceStack: %d, TargetStack: %d, CanStack: %d"
        ),
        *SourceItemId.ToString(), *TargetItemId.ToString(),
        MaxStack, SourceItemData.StackCount, TargetItemData.StackCount, CanStackCount);
    if (CanStackCount <= 0)
        return;
    SourceItemData.StackCount -= CanStackCount;
    // 这里需要先更新TargetIndex,因为SourceIndex可能会删除元素,使得TargetIndex失效
    TargetItemData.StackCount += CanStackCount;
    MarkItemDirty(Items[TargetIndex]);
    if (SourceItemData.StackCount <= 0)
        RemoveItemAt(SourceIndex);
    else
        MarkItemDirty(Items[SourceIndex]);

    // 在ListenServer或者StandAlone模式下,需要主动广播消息
    if (OwnerComponent.IsValid())
    {
        ENetMode Nm = OwnerComponent->GetNetMode();
        if (Nm == NM_ListenServer || Nm == NM_Standalone)
        {
            if (SourceItemData.StackCount > 0)
                OnItemChanged.Broadcast(SourceItemData);
            OnItemChanged.Broadcast(TargetItemData);
        }
    }
}

bool FInv_ItemList::TrySplitItem(FGuid ItemId, int32 SplitCount)
{
    if (!IsServer())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::TrySplitItem: Only server can split items!"));
        return false;
    }
    const int32 OldItemIndex = IndexOfItem(ItemId);
    if (OldItemIndex == INDEX_NONE)
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("FInv_ItemList::TrySplitItem: Item '%s' not found!"), *ItemId.ToString());
        return false;
    }

    FInv_ItemFastArrayItem &OldItemEntry = Items[OldItemIndex];
    FInv_RealItemData &OldItemData = OldItemEntry.RealItemData;

    if (SplitCount <= 0 || SplitCount >= OldItemData.StackCount)
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("FInv_ItemList::TrySplitItem: Invalid SplitCount %d for item '%s' with StackCount %d"),
            SplitCount, *ItemId.ToString(), OldItemData.StackCount);
        return false;
    }

    OldItemData.StackCount -= SplitCount;
    MarkItemDirty(OldItemEntry);

    // tips: 检查是否还有空余额外是在InventoryBase中进行的,这里直接添加创建并添加新的物品
    FInv_ItemFastArrayItem &NewItemEntry = Items.AddDefaulted_GetRef();
    NewItemEntry.RealItemData = OldItemData; // 复制原有数据
    NewItemEntry.RealItemData.StackCount = SplitCount; // 设置新物品的堆叠数量
    NewItemEntry.RealItemData.RealItemId = FGuid::NewGuid();
    MarkItemDirty(NewItemEntry);

    if (OwnerComponent.IsValid())
    {
        ENetMode Nm = OwnerComponent->GetNetMode();
        if (Nm == NM_ListenServer || Nm == NM_Standalone)
        {
            OnItemAdded.Broadcast(NewItemEntry.RealItemData);
            OnItemChanged.Broadcast(OldItemData);
        }
    }

    return true;
}

bool FInv_ItemList::RemoveItem(const FGuid &ItemId)
{
    if (!IsServer())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::RemoveItem: Only server can remove items!"));
        return false;
    }

    const int32 Index = IndexOfItem(ItemId);
    if (Index == INDEX_NONE)
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::RemoveItem: Item '%s' not found!"),
            *ItemId.ToString());
        return false;
    }

    return RemoveItemAt(Index);
}

bool FInv_ItemList::RemoveItemAt(int32 Index)
{
    if (!IsServer())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::RemoveItemAt: Only server can remove items!"));
        return false;
    }

    if (!Items.IsValidIndex(Index))
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::RemoveItemAt: Invalid index %d"), Index);
        return false;
    }

    const FGuid ItemId = Items[Index].GetItemId();

    // broadcast event on client which is also a server
    if (OwnerComponent.IsValid())
    {
        ENetMode Nm = OwnerComponent->GetNetMode();
        if (Nm == NM_ListenServer || Nm == NM_Standalone)
            OnItemRemoved.Broadcast(ItemId);
    }

    // 从数组移除
    Items.RemoveAt(Index);

    // 标记数组已修改
    MarkArrayDirty();

    UE_LOG(LogInventorySystem, Log, TEXT("FInv_ItemList::RemoveItemAt: Removed item '%s' at index %d"),
        *ItemId.ToString(),
        Index);

    return true;
}

bool FInv_ItemList::UpdateItem(const FGuid &ItemId, const FInv_RealItemData &NewItemData)
{
    if (!IsServer())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::UpdateItem: Only server can update items!"));
        return false;
    }

    // 验证新数据的 ID 与要更新的 ID 一致
    if (NewItemData.RealItemId != ItemId)
    {
        UE_LOG(LogInventorySystem, Error,
            TEXT("FInv_ItemList::UpdateItem: New item data ID '%s' doesn't match target ID '%s'!"),
            *NewItemData.RealItemId.ToString(), *ItemId.ToString());
        return false;
    }

    const int32 Index = IndexOfItem(ItemId);
    if (Index == INDEX_NONE)
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::UpdateItem: Item '%s' not found!"),
            *ItemId.ToString());
        return false;
    }

    // 更新数据
    Items[Index].RealItemData = NewItemData;

    // 标记为已修改
    MarkItemDirty(Items[Index]);

    UE_LOG(LogInventorySystem, Log, TEXT("FInv_ItemList::UpdateItem: Updated item '%s'"), *ItemId.ToString());

    // broadcast event on client which is also a server
    if (OwnerComponent.IsValid())
    {
        ENetMode Nm = OwnerComponent->GetNetMode();
        if (Nm == NM_ListenServer || Nm == NM_Standalone)
            OnItemChanged.Broadcast(NewItemData);
    }

    return true;
}

void FInv_ItemList::ClearAllItems()
{
    if (!IsServer())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::ClearAllItems: Only server can clear items!"));
        return;
    }

    // 在服务端（ListenServer 或 Standalone）广播所有移除委托
    if (OwnerComponent.IsValid())
    {
        ENetMode Nm = OwnerComponent->GetNetMode();
        if (Nm == NM_ListenServer || Nm == NM_Standalone)
        {
            for (const FInv_ItemFastArrayItem &Item : Items)
            {
                OnItemRemoved.Broadcast(Item.GetItemId());
            }
        }
    }

    Items.Empty();

    MarkArrayDirty();

    UE_LOG(LogInventorySystem, Log, TEXT("FInv_ItemList::ClearAllItems: Cleared all items"));
}

const FInv_RealItemData *FInv_ItemList::FindItem(const FGuid &ItemId) const
{
    // 线性搜索
    for (const FInv_ItemFastArrayItem &Item : Items)
    {
        if (Item.GetItemId() == ItemId)
        {
            return &Item.RealItemData;
        }
    }
    return nullptr;
}

FInv_RealItemData *FInv_ItemList::FindItemMutable(const FGuid &ItemId)
{
    // 线性搜索
    for (FInv_ItemFastArrayItem &Item : Items)
    {
        if (Item.GetItemId() == ItemId)
        {
            return &Item.RealItemData;
        }
    }
    return nullptr;
}

bool FInv_ItemList::ContainRealItem(const FGuid &ItemId) const
{
    return FindItem(ItemId) != nullptr;
}

TArray<FGuid> FInv_ItemList::GetAllItemIds() const
{
    TArray<FGuid> ItemIds;
    ItemIds.Reserve(Items.Num());

    for (const FInv_ItemFastArrayItem &Item : Items)
    {
        ItemIds.Add(Item.GetItemId());
    }

    return ItemIds;
}

void FInv_ItemList::ForEachItem(TFunctionRef<void(const FInv_RealItemData &)> Callback) const
{
    for (const FInv_ItemFastArrayItem &Item : Items)
    {
        Callback(Item.RealItemData);
    }
}

int32 FInv_ItemList::IndexOfItem(const FGuid &ItemId) const
{
    // 线性搜索
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (Items[i].GetItemId() == ItemId)
        {
            return i;
        }
    }

    return INDEX_NONE;
}

bool FInv_ItemList::IsServer() const
{
    if (!OwnerComponent.IsValid())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("FInv_ItemList::IsServer: OwnerComponent is null!"));
        return false;
    }

    const AActor *Owner = OwnerComponent->GetOwner();
    if (!Owner)
    {
        return false;
    }

    return Owner->HasAuthority();
}