#include "Inventory/Inv_InventoryBase.h"
#include "Engine/World.h"
#include "InventorySystem.h"
#include "Items/Component/Inv_ItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/Inv_VirtualItemDataSubsystem.h"
#include "Widgets/Inv_InventoryWidgetBase.h"

UInv_InventoryBase::UInv_InventoryBase() : ItemList(this)
{
    // 启用网络复制
    SetIsReplicatedByDefault(true);

    // 默认每帧不需要 Tick
    PrimaryComponentTick.bCanEverTick = false;
}

void UInv_InventoryBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 复制 ItemList（FastArray 自动处理增量同步）
    DOREPLIFETIME(UInv_InventoryBase, ItemList);
}

void UInv_InventoryBase::CreateInvWidget()
{
    // 创建UI窗口,需要注意,只会在客户端创建UI窗口
    if (GetNetMode() != NM_DedicatedServer && IsValid(InventoryWidgetClass))
    {
        InventoryWidgetInstance = CreateWidget<UInv_InventoryWidgetBase>(GetWorld()->GetFirstPlayerController(),
            InventoryWidgetClass);
        if (InventoryWidgetInstance.IsValid())
        {
            InventoryWidgetInstance->AddToViewport();
            InventoryWidgetInstance->InitializeInventory(MaxSlots, PerRowCount);
            InventoryWidgetInstance->OnItemDrop.AddDynamic(this, &ThisClass::ServerOnItemDroppedFunc);
            InventoryWidgetInstance->OnItemSplit.AddDynamic(this, &ThisClass::ServerOnItemSplitFunc);
        }
        else
        {
            UE_LOG(LogInventorySystem, Error,
                TEXT("UInv_InventoryBase::BeginPlay: Failed to create InventoryWidgetInstance"));
        }
    }
}

void UInv_InventoryBase::BeginPlay()
{
    Super::BeginPlay();

    // 设置 FastArray 的 OwnerComponent
    ItemList.OwnerComponent = this;

    // 绑定 FastArray 委托
    BindFastArrayDelegates();

    UE_LOG(LogInventorySystem, Log, TEXT("UInv_InventoryBase::BeginPlay: Inventory initialized on %s"),
        GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));

    CreateInvWidget();
}

void UInv_InventoryBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 解绑委托
    UnbindFastArrayDelegates();

    Super::EndPlay(EndPlayReason);
}

// ========== 查询接口实现 ==========

const FInv_RealItemData *UInv_InventoryBase::FindItemById(const FGuid &ItemId) const
{
    return ItemList.FindItem(ItemId);
}

bool UInv_InventoryBase::ContainsItem(const FGuid &ItemId) const
{
    return ItemList.ContainRealItem(ItemId);
}

int32 UInv_InventoryBase::GetItemCount() const
{
    return ItemList.Num();
}

bool UInv_InventoryBase::IsEmpty() const
{
    return ItemList.IsEmpty();
}

TArray<FGuid> UInv_InventoryBase::GetAllItemIds() const
{
    return ItemList.GetAllItemIds();
}

// ========== 修改接口实现 ==========

bool UInv_InventoryBase::TryAddItemInstanceIndividually(const FInv_RealItemData &ItemData)
{
    // 仅在服务端执行
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_InventoryBase::TryAddItemInstance: Can only add items on server! Owner: %s"),
            *GetOwner()->GetName());
        return false;
    }

    // 验证是否可以完全添加
    if (!CanAddItemEntirely(ItemData))
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("UInv_InventoryBase::TryAddItemInstance: CanAddItem check failed"));
        return false;
    }

    // 添加到 FastArray
    const bool bSuccess = ItemList.TryAddNewItem(ItemData);

    if (bSuccess)
    {
        UE_LOG(LogInventorySystem, Display,
            TEXT("UInv_InventoryBase::TryAddItemInstance: Successfully added item '%s'"),
            *ItemData.RealItemId.ToString());
    }

    return bSuccess;
}

int32 UInv_InventoryBase::TryAddItemInstanceByStacking(const FInv_RealItemData &RealItemData)
{
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_InventoryBase::TryAddItemInstancePartially: Can only add items on server! Owner: %s"),
            *GetOwner()->GetName());
        return 0;
    }

    return ItemList.TryStackItem(RealItemData);
}

bool UInv_InventoryBase::TryRemoveItem(const FGuid &ItemId)
{
    // 仅在服务端执行
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_InventoryBase::TryRemoveItem: Can only remove items on server! Owner: %s"),
            *GetOwner()->GetName());
        return false;
    }

    // 验证是否可以移除
    if (!CanRemoveItem(ItemId))
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("UInv_InventoryBase::TryRemoveItem: CanRemoveItem check failed"));
        return false;
    }

    // 从 FastArray 移除
    const bool bSuccess = ItemList.RemoveItem(ItemId);

    if (bSuccess)
    {
        UE_LOG(LogInventorySystem, Log, TEXT("UInv_InventoryBase::TryRemoveItem: Successfully removed item '%s'"),
            *ItemId.ToString());
    }
    else
    {
        UE_LOG(LogInventorySystem, Error, TEXT("UInv_InventoryBase::TryRemoveItem: Failed to remove item '%s'"),
            *ItemId.ToString());
    }

    return bSuccess;
}

void UInv_InventoryBase::ClearAllItems()
{
    // 仅在服务端执行
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_InventoryBase::ClearAllItems: Can only clear items on server! Owner: %s"),
            *GetOwner()->GetName());
        return;
    }

    ItemList.ClearAllItems();

    UE_LOG(LogInventorySystem, Log, TEXT("UInv_InventoryBase::ClearAllItems: Cleared all items"));
}

bool UInv_InventoryBase::UpdateItem(const FGuid &ItemId, const FInv_RealItemData &NewItemData)
{
    // 仅在服务端执行
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(LogInventorySystem, Error, TEXT("UInv_InventoryBase::UpdateItem: Can only execute on server! Owner: %s"),
            *GetOwner()->GetName());
        return false;
    }

    // 验证 ItemId 是否有效
    if (!ItemId.IsValid())
    {
        UE_LOG(LogInventorySystem, Error, TEXT("UInv_InventoryBase::UpdateItem: Invalid ItemId (GUID is zero)"));
        return false;
    }

    // 验证 NewItemData.RealItemId 是否有效
    if (!NewItemData.RealItemId.IsValid())
    {
        UE_LOG(LogInventorySystem, Error, TEXT("UInv_InventoryBase::UpdateItem: Invalid RealItemId in NewItemData"));
        return false;
    }

    // 验证 ID 一致性
    if (NewItemData.RealItemId != ItemId)
    {
        UE_LOG(LogInventorySystem, Error,
            TEXT("UInv_InventoryBase::UpdateItem: Item ID mismatch! Requested: %s, Data: %s"), *ItemId.ToString(),
            *NewItemData.RealItemId.ToString());
        return false;
    }

    // 执行更新
    const bool bSuccess = ItemList.UpdateItem(ItemId, NewItemData);

    if (bSuccess)
    {
        UE_LOG(LogInventorySystem, Log, TEXT("UInv_InventoryBase::UpdateItem: Successfully updated item '%s'"),
            *ItemId.ToString());
    }
    else
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("UInv_InventoryBase::UpdateItem: Failed to update item '%s'"),
            *ItemId.ToString());
    }

    return bSuccess;
}

// ========== 拾取物品实现 ==========

bool UInv_InventoryBase::TryPickupItem(AActor *ItemActor)
{
    // 仅在服务端执行
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("UInv_InventoryBase::TryPickupItem: Can only execute on server"));
        return false;
    }

    // 验证 Actor 有效性
    if (!ItemActor || !ItemActor->IsValidLowLevel())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("UInv_InventoryBase::TryPickupItem: Invalid ItemActor"));
        return false;
    }

    // 获取 ItemComponent
    UInv_ItemComponent *ItemComp = ItemActor->FindComponentByClass<UInv_ItemComponent>();
    if (!ItemComp)
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("UInv_InventoryBase::TryPickupItem: ItemActor has no ItemComponent"));
        return false;
    }

    // 检查 ItemComponent 是否允许拾取
    if (!ItemComp->CanBePickedUp())
    {
        UE_LOG(LogInventorySystem, Display,
            TEXT("UInv_InventoryBase::TryPickupItem: ItemComponent.CanBePickedUp() returned false"));
        return false;
    }

    // 获取物品数据
    FInv_RealItemData &ItemData = ItemComp->GetItemDataMutable();

    // pick up at least one of them ?
    bool bSuccess = false;

    // 尝试部分添加物品
    // 对于ItemComp的所有数据更改均放在本函数中,所以以下的TryXXX函数不对传入的ItemData进行更改.
    const int32 TotalStackCount = TryAddItemInstanceByStacking(ItemData);
    if (TotalStackCount > 0)
    {
        bSuccess = true;
        ItemData.StackCount -= TotalStackCount;
        UE_LOG(LogInventorySystem, Display,
            TEXT("UInv_InventoryBase::TryPickupItem Partially: Picked up %d items from '%s', %d remaining"),
            TotalStackCount,
            *ItemActor->GetName(), ItemData.StackCount);
    }

    // 尝试不适用堆叠添加物品
    if (ItemData.StackCount > 0)
    {
        if (TryAddItemInstanceIndividually(ItemData))
        {
            UE_LOG(LogInventorySystem, Display,
                TEXT("UInv_InventoryBase::TryPickupItem Entirelly: Successfully picked up %d items from '%s'"),
                ItemData.StackCount,
                *ItemActor->GetName());

            ItemData.StackCount = 0;
            bSuccess = true;
        }
        else
        {
            UE_LOG(LogInventorySystem, Log,
                TEXT("UInv_InventoryBase::TryPickupItem: Could not pick up '%s' Individually, %d remaining"),
                *ItemActor->GetName(), ItemData.StackCount);
        }
    }

    if (bSuccess)
        ItemComp->OnPickedUp(GetOwner());

    return bSuccess;
}

AActor *UInv_InventoryBase::DetectPickableItemInView(const FVector &StartLocation, const FVector &Direction,
    float TraceDistance) const
{
    // 如果检测距离仍然无效，返回空
    if (TraceDistance <= 0.0f)
    {
        return nullptr;
    }

    // 计算射线终点
    const FVector EndLocation = StartLocation + (Direction.GetSafeNormal() * TraceDistance);

    // 设置射线检测参数
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner()); // 忽略自己
    QueryParams.bTraceComplex = false; // 简单碰撞检测即可
    QueryParams.bReturnPhysicalMaterial = false;

    // 执行射线检测
    const UWorld *World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const bool bHit = World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation,
        ECC_Visibility, // 使用可见性通道
        QueryParams);

    // 如果没有击中任何东西，返回空
    if (!bHit || !HitResult.GetActor())
    {
        return nullptr;
    }

    AActor *HitActor = HitResult.GetActor();

    // 检查该 Actor 是否有 ItemComponent
    UInv_ItemComponent *ItemComponent = HitActor->FindComponentByClass<UInv_ItemComponent>();
    if (!ItemComponent)
    {
        return nullptr;
    }

    // 检查是否可以被拾取
    if (!ItemComponent->CanBePickedUp())
    {
        return nullptr;
    }

    UE_LOG(LogInventorySystem, Verbose,
        TEXT("UInv_SlottedInventoryComponent::DetectPickableItemInView: Found pickable item '%s' at distance %.1f"),
        *HitActor->GetName(), HitResult.Distance);

    return HitActor;
}

// ========== RPC 实现 ==========

void UInv_InventoryBase::ServerPickupItem_Implementation(AActor *ItemActor)
{
    TryPickupItem(ItemActor);
}

bool UInv_InventoryBase::ServerPickupItem_Validate(AActor *ItemActor)
{
    // 基本验证：Actor 必须有效
    if (!ItemActor)
    {
        UE_LOG(LogInventorySystem, Error, TEXT("UInv_InventoryBase::ServerPickupItem_Validate: Invalid ItemActor"));
        return false;
    }

    // 必须包含 ItemComponent
    UInv_ItemComponent *ItemComp = ItemActor->FindComponentByClass<UInv_ItemComponent>();
    if (!ItemComp)
    {
        UE_LOG(LogInventorySystem, Error,
            TEXT("UInv_InventoryBase::ServerPickupItem_Validate: ItemActor has no ItemComponent"));
        return false;
    }

    return true;
}

// ========== 事件响应默认实现 ==========

void UInv_InventoryBase::OnItemAdded_Implementation(const FInv_RealItemData &ItemData)
{
    if (IsFull())
        UE_LOG(LogInventorySystem, Display,
        TEXT("UInv_InventoryBase::OnItemAdded: Inventory is full after adding item '%s'"),
        *ItemData.RealItemId.ToString());
    // 广播委托事件（蓝图和C++都可以订阅）
    OnItemAddedEvent.Broadcast(ItemData);

    if (InventoryWidgetInstance.IsValid())
        InventoryWidgetInstance->AddItem(ItemData);

    // 基类默认实现：仅记录日志
    UE_LOG(LogInventorySystem, Log, TEXT("UInv_InventoryBase::OnItemAdded: Item '%s' added (Role: %s)"),
        *ItemData.RealItemId.ToString(),
        GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));
}

void UInv_InventoryBase::OnItemChanged_Implementation(const FInv_RealItemData &ItemData)
{
    // 广播委托事件（蓝图和C++都可以订阅）
    OnItemChangedEvent.Broadcast(ItemData);

    if (InventoryWidgetInstance.IsValid())
        InventoryWidgetInstance->UpdateItem(ItemData);

    // 基类默认实现：仅记录日志
    UE_LOG(LogInventorySystem, Display, TEXT("UInv_InventoryBase::OnItemChanged: Item '%s' changed (Role: %s)"),
        *ItemData.RealItemId.ToString(),
        GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));
}

void UInv_InventoryBase::OnItemRemoved_Implementation(FGuid ItemId)
{
    // 广播委托事件（蓝图和C++都可以订阅）
    OnItemRemovedEvent.Broadcast(ItemId);

    if (InventoryWidgetInstance.IsValid())
        InventoryWidgetInstance->RemoveItem(ItemId);

    // 基类默认实现：仅记录日志
    UE_LOG(LogInventorySystem, Display, TEXT("UInv_InventoryBase::OnItemRemoved: Item '%s' removed (Role: %s)"),
        *ItemId.ToString(),
        GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));
}

// ========== 验证接口默认实现 ==========

bool UInv_InventoryBase::CanAddItemEntirely(const FInv_RealItemData &ItemData) const
{
    // 基类默认实现：总是允许添加
    // 子类可重写以实现容量限制等逻辑
    if (IsFull())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("UInv_SlottedInventoryComponent::CanAddItem: Inventory is full"));
        return false;
    }

    if (bUseItemTypeFilter && !IsItemTypeAllowed(ItemData))
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_InventoryBase::CanAddItemEntirely: Item type '%s' not allowed in this inventory"),
            *ItemData.GetVirtualItemData()->ItemTag.ToString());
        return false;
    }

    return CanAddItemEntirely_BP(ItemData);
}

bool UInv_InventoryBase::CanAddItemEntirely_BP_Implementation(const FInv_RealItemData &ItemData) const
{
    return true;
}

bool UInv_InventoryBase::CanRemoveItem(const FGuid &ItemId) const
{
    // 基类默认实现：只要物品存在就允许移除
    return ItemList.ContainRealItem(ItemId) && CanRemoveItem_BP(ItemId);
}

bool UInv_InventoryBase::CanRemoveItem_BP_Implementation(const FGuid &ItemId) const
{
    return true;
}

// ========== 内部辅助方法实现 ==========

void UInv_InventoryBase::BindFastArrayDelegates()
{
    // 绑定 FastArray 委托到本地方法
    OnItemAddedHandle = ItemList.OnItemAdded.AddUObject(this, &UInv_InventoryBase::OnItemAdded);
    OnItemChangedHandle = ItemList.OnItemChanged.AddUObject(this, &UInv_InventoryBase::OnItemChanged);
    OnItemRemovedHandle = ItemList.OnItemRemoved.AddUObject(this, &UInv_InventoryBase::OnItemRemoved);

    UE_LOG(LogInventorySystem, Display, TEXT("UInv_InventoryBase::BindFastArrayDelegates: Delegates bound"));
}

void UInv_InventoryBase::UnbindFastArrayDelegates()
{
    // 解绑委托
    if (OnItemAddedHandle.IsValid())
    {
        ItemList.OnItemAdded.Remove(OnItemAddedHandle);
        OnItemAddedHandle.Reset();
    }

    if (OnItemChangedHandle.IsValid())
    {
        ItemList.OnItemChanged.Remove(OnItemChangedHandle);
        OnItemChangedHandle.Reset();
    }

    if (OnItemRemovedHandle.IsValid())
    {
        ItemList.OnItemRemoved.Remove(OnItemRemovedHandle);
        OnItemRemovedHandle.Reset();
    }

    UE_LOG(LogInventorySystem, Display, TEXT("UInv_InventoryBase::UnbindFastArrayDelegates: Delegates unbound"));
}

UInv_VirtualItemDataSubsystem *UInv_InventoryBase::GetVirtualItemDataSubsystem() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    UGameInstance *GameInstance = GetWorld()->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UInv_VirtualItemDataSubsystem>();
}

bool UInv_InventoryBase::IsItemTypeAllowed(const FInv_RealItemData &ItemData) const
{
    if (!bUseItemTypeFilter)
        return true;

    // 获取物品的类型标签
    const FInv_VirtualItemData *VirtualData = ItemData.GetVirtualItemData();
    if (!VirtualData)
    {
        return false;
    }

    // 检查物品标签是否匹配任何允许的类型
    return VirtualData->ItemTag.MatchesAny(AllowedItemTypes);
}

void UInv_InventoryBase::ServerOnItemSplitFunc_Implementation(FGuid ItemId, int32 SplitCount)
{
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_InventoryBase::OnItemSplitFunc: Can only execute on server! Owner: %s"),
            *GetOwner()->GetName());
        return;
    }

    //背包已经满了,无法再分割物品
    if (IsFull())
    {
        UE_LOG(LogInventorySystem, Display,
            TEXT("UInv_InventoryBase::OnItemSplitFunc: Inventory is full, cannot split item '%s'"),
            *ItemId.ToString());
        return;
    }

    if (!ItemList.TrySplitItem(ItemId, SplitCount))
        UE_LOG(LogInventorySystem, Warning,
        TEXT("UInv_InventoryBase::OnItemSplitFunc: Failed to split item '%s' with count %d"),
        *ItemId.ToString(), SplitCount);
}

void UInv_InventoryBase::ServerOnItemDroppedFunc_Implementation(FGuid SourceItemId, FGuid TargetItemId)
{
    // TODO:这里是只要发生了物品拖拽,就会发送RPC,也许可以通过条件检查进行优化,减少不必要的RPC发送
    ItemList.TryDropItem(SourceItemId, TargetItemId);
}