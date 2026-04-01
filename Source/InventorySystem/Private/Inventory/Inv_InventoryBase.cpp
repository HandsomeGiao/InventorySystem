#include "Inventory/Inv_InventoryBase.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InventorySystem.h"
#include "Items/Component/Inv_ItemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/Inv_InventoryWidgetOwnerSubsystem.h"
#include "Subsystem/Inv_VirtualItemDataSubsystem.h"
#include "Widgets/Inv_InventoryWidgetBase.h"

UInv_InventoryBase::UInv_InventoryBase() : ItemList(this)
{
	// 启用网络复制
	SetIsReplicatedByDefault(true);

	// 默认每帧不需要 Tick
	PrimaryComponentTick.bCanEverTick = false;
}

void UInv_InventoryBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 复制 ItemList（FastArray 自动处理增量同步）
	DOREPLIFETIME(UInv_InventoryBase, ItemList);
}

UInv_InventoryWidgetBase* UInv_InventoryBase::CreateInventoryWidget(APlayerController* OwningPlayer,
                                                                    bool bAddToViewport)
{
	return CreateInventoryWidgetByType(ResolveInventoryWidgetType(OwningPlayer), OwningPlayer, bAddToViewport);
}

void UInv_InventoryBase::BindInventoryWidget(UInv_InventoryWidgetBase* InInventoryWidget)
{
	BindInventoryWidgetByType(ResolveInventoryWidgetType(InInventoryWidget ? InInventoryWidget->GetOwningPlayer() : nullptr),
	                          InInventoryWidget);
}

void UInv_InventoryBase::UnbindInventoryWidget()
{
	UnbindInventoryWidgetByType(ResolveInventoryWidgetType());
}

void UInv_InventoryBase::DestroyInventoryWidget()
{
	DestroyInventoryWidgetByType(ResolveInventoryWidgetType());
}

void UInv_InventoryBase::RefreshInventoryWidget()
{
	RefreshInventoryWidgetByType(ResolveInventoryWidgetType());
}

UInv_InventoryWidgetBase* UInv_InventoryBase::GetInventoryWidget() const
{
	return GetInventoryWidgetByType(ResolveInventoryWidgetType());
}

UInv_InventoryWidgetBase* UInv_InventoryBase::CreateInventoryWidgetByType(EInv_InventoryWidgetType WidgetType,
                                                                          APlayerController* OwningPlayer,
                                                                          bool bAddToViewport)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	if (WidgetType == EInv_InventoryWidgetType::None)
	{
		WidgetType = DefaultInventoryWidgetType;
	}

	if (UInv_InventoryWidgetBase* ExistingWidget = GetInventoryWidgetByType(WidgetType))
	{
		RefreshInventoryWidgetByType(WidgetType);

		if (bAddToViewport && !ExistingWidget->IsInViewport())
		{
			ExistingWidget->AddToViewport();
		}

		return ExistingWidget;
	}

	const TSubclassOf<UInv_InventoryWidgetBase> WidgetClass = ResolveInventoryWidgetClass(WidgetType);
	if (!IsValid(WidgetClass))
	{
		UE_LOG(LogInventorySystem, Error,
		       TEXT("UInv_InventoryBase::CreateInventoryWidgetByType: InventoryWidgetClass is invalid"));
		return nullptr;
	}

	OwningPlayer = ResolveInventoryWidgetOwningPlayer(OwningPlayer);
	if (!IsValid(OwningPlayer))
	{
		UE_LOG(LogInventorySystem, Warning,
		       TEXT("UInv_InventoryBase::CreateInventoryWidgetByType: Failed to get OwningPlayer"));
		return nullptr;
	}

	UInv_InventoryWidgetBase* NewWidget = CreateWidget<UInv_InventoryWidgetBase>(OwningPlayer, WidgetClass);
	if (!IsValid(NewWidget))
	{
		UE_LOG(LogInventorySystem, Error,
		       TEXT("UInv_InventoryBase::CreateInventoryWidgetByType: Failed to create InventoryWidgetInstance"));
		return nullptr;
	}

	if (bAddToViewport)
	{
		NewWidget->AddToViewport();
	}

	BindInventoryWidgetByType(WidgetType, NewWidget);

	return NewWidget;
}

void UInv_InventoryBase::BindInventoryWidgetByType(EInv_InventoryWidgetType WidgetType,
                                                   UInv_InventoryWidgetBase* InInventoryWidget)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (WidgetType == EInv_InventoryWidgetType::None)
	{
		WidgetType = DefaultInventoryWidgetType;
	}

	if (!IsValid(InInventoryWidget))
	{
		UE_LOG(LogInventorySystem, Warning,
		       TEXT("UInv_InventoryBase::BindInventoryWidgetByType: InInventoryWidget is invalid"));
		return;
	}

	if (GetInventoryWidgetByType(WidgetType) == InInventoryWidget)
	{
		RefreshInventoryWidgetByType(WidgetType);
		return;
	}

	UInv_InventoryWidgetOwnerSubsystem* WidgetOwnerSubsystem =
		ResolveInventoryWidgetOwnerSubsystem(InInventoryWidget->GetOwningPlayer());
	if (WidgetOwnerSubsystem)
	{
		UInv_InventoryBase* ActiveInventory = WidgetOwnerSubsystem->FindInventoryWidgetOwner(WidgetType);
		if (IsValid(ActiveInventory) && ActiveInventory != this)
		{
			ActiveInventory->DestroyInventoryWidgetByType(WidgetType);
		}
	}

	for (auto It = InventoryWidgetInstances.CreateIterator(); It; ++It)
	{
		if (It.Value() == InInventoryWidget && It.Key() != WidgetType)
		{
			InInventoryWidget->OnItemSplit.RemoveDynamic(this, &ThisClass::ServerOnItemSplitFunc);
			InInventoryWidget->SetInventory(nullptr);
			InInventoryWidget->SetInventoryWidgetType(EInv_InventoryWidgetType::None);
			It.RemoveCurrent();
			break;
		}
	}

	UnbindInventoryWidgetByType(WidgetType);

	InventoryWidgetInstances.FindOrAdd(WidgetType) = InInventoryWidget;
	InInventoryWidget->SetInventory(this);
	InInventoryWidget->SetInventoryWidgetType(WidgetType);
	InInventoryWidget->InitializeInventory(MaxSlots, PerRowCount);
	InInventoryWidget->OnItemSplit.AddDynamic(this, &ThisClass::ServerOnItemSplitFunc);
	if (WidgetOwnerSubsystem)
	{
		WidgetOwnerSubsystem->RegisterInventoryWidgetOwner(WidgetType, this);
	}

	RefreshInventoryWidgetByType(WidgetType);
}

void UInv_InventoryBase::UnbindInventoryWidgetByType(EInv_InventoryWidgetType WidgetType)
{
	if (WidgetType == EInv_InventoryWidgetType::None)
	{
		WidgetType = DefaultInventoryWidgetType;
	}

	UInv_InventoryWidgetBase* InventoryWidgetInstance = GetInventoryWidgetByType(WidgetType);
	if (!IsValid(InventoryWidgetInstance))
	{
		return;
	}

	InventoryWidgetInstance->OnItemSplit.RemoveDynamic(this, &ThisClass::ServerOnItemSplitFunc);
	UInv_InventoryWidgetOwnerSubsystem* WidgetOwnerSubsystem =
		ResolveInventoryWidgetOwnerSubsystem(InventoryWidgetInstance->GetOwningPlayer());
	InventoryWidgetInstance->SetInventory(nullptr);
	InventoryWidgetInstance->SetInventoryWidgetType(EInv_InventoryWidgetType::None);
	InventoryWidgetInstances.Remove(WidgetType);
	if (WidgetOwnerSubsystem)
	{
		WidgetOwnerSubsystem->UnregisterInventoryWidgetOwner(WidgetType, this);
	}
}

void UInv_InventoryBase::DestroyInventoryWidgetByType(EInv_InventoryWidgetType WidgetType)
{
	UInv_InventoryWidgetBase* WidgetToDestroy = GetInventoryWidgetByType(WidgetType);
	if (!IsValid(WidgetToDestroy))
	{
		return;
	}

	UnbindInventoryWidgetByType(WidgetType);
	WidgetToDestroy->RemoveFromParent();
}

void UInv_InventoryBase::RefreshInventoryWidgetByType(EInv_InventoryWidgetType WidgetType)
{
	UInv_InventoryWidgetBase* InventoryWidgetInstance = GetInventoryWidgetByType(WidgetType);
	if (!IsValid(InventoryWidgetInstance))
	{
		return;
	}

	InventoryWidgetInstance->UpdateInventory(GetAllItems());
}

void UInv_InventoryBase::RefreshAllInventoryWidgets()
{
	const TArray<FInv_RealItemData> ItemDataArray = GetAllItems();
	for (const TPair<EInv_InventoryWidgetType, TObjectPtr<UInv_InventoryWidgetBase>>& WidgetPair :
	     InventoryWidgetInstances)
	{
		if (IsValid(WidgetPair.Value))
		{
			WidgetPair.Value->UpdateInventory(ItemDataArray);
		}
	}
}

UInv_InventoryWidgetBase* UInv_InventoryBase::GetInventoryWidgetByType(EInv_InventoryWidgetType WidgetType) const
{
	if (WidgetType == EInv_InventoryWidgetType::None)
	{
		WidgetType = DefaultInventoryWidgetType;
	}

	if (const TObjectPtr<UInv_InventoryWidgetBase>* FoundWidget = InventoryWidgetInstances.Find(WidgetType))
	{
		return FoundWidget->Get();
	}

	return nullptr;
}

EInv_InventoryWidgetType UInv_InventoryBase::ResolveInventoryWidgetType(APlayerController* ViewingPlayer) const
{
	switch (InventoryOwnerType)
	{
	case EInv_InventoryOwnerType::Player:
		{
			ViewingPlayer = ResolveInventoryWidgetOwningPlayer(ViewingPlayer);
			const APawn* ViewingPawn = ViewingPlayer ? ViewingPlayer->GetPawn() : nullptr;
			return ViewingPawn && ViewingPawn == GetOwner()
				       ? EInv_InventoryWidgetType::PlayerSelf
				       : EInv_InventoryWidgetType::PlayerOther;
		}

	case EInv_InventoryOwnerType::Container:
		return EInv_InventoryWidgetType::Container;

	case EInv_InventoryOwnerType::Other:
	default:
		return DefaultInventoryWidgetType == EInv_InventoryWidgetType::None
			       ? EInv_InventoryWidgetType::Other
			       : DefaultInventoryWidgetType;
	}
}

void UInv_InventoryBase::RequestMoveItem(UInv_InventoryBase* SourceInventory, const FGuid& SourceItemId,
                                         UInv_InventoryBase* TargetInventory, int32 TargetSlotIndex)
{
	if (!IsValid(SourceInventory) || !IsValid(TargetInventory))
	{
		UE_LOG(LogInventorySystem, Warning,
		       TEXT("UInv_InventoryBase::RequestMoveItem: SourceInventory or TargetInventory is invalid"));
		return;
	}

	if (GetOwner()->HasAuthority())
	{
		TryMoveItemBetweenInventories(SourceInventory, SourceItemId, TargetInventory, TargetSlotIndex);
		return;
	}

	ServerRequestMoveItem(SourceInventory->GetOwner(), SourceItemId, TargetInventory->GetOwner(), TargetSlotIndex);
}

bool UInv_InventoryBase::CanSendServerCommand() const
{
	if (!GetOwner())
	{
		return false;
	}

	if (GetOwner()->HasAuthority() || GetNetMode() == NM_Standalone)
	{
		return true;
	}

	return GetOwner()->GetLocalRole() == ROLE_AutonomousProxy;
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

	if (bAutoCreateInventoryWidget)
	{
		const EInv_InventoryWidgetType WidgetType = ResolveInventoryWidgetType();
		if (InventoryOwnerType != EInv_InventoryOwnerType::Player || WidgetType == EInv_InventoryWidgetType::PlayerSelf)
		{
			CreateInventoryWidgetByType(WidgetType);
		}
	}
}

void UInv_InventoryBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 解绑委托
	UnbindFastArrayDelegates();

	TArray<EInv_InventoryWidgetType> WidgetTypes;
	InventoryWidgetInstances.GetKeys(WidgetTypes);
	for (const EInv_InventoryWidgetType WidgetType : WidgetTypes)
	{
		DestroyInventoryWidgetByType(WidgetType);
	}

	Super::EndPlay(EndPlayReason);
}

// ========== 查询接口实现 ==========

const FInv_RealItemData* UInv_InventoryBase::FindItemById(const FGuid& ItemId) const
{
	return ItemList.FindItem(ItemId);
}

bool UInv_InventoryBase::ContainsItem(const FGuid& ItemId) const
{
	return ItemList.ContainRealItem(ItemId);
}

int32 UInv_InventoryBase::GetItemsCount() const
{
	return ItemList.Num();
}

bool UInv_InventoryBase::IsInventoryEmpty() const
{
	return ItemList.IsEmpty();
}

TArray<FGuid> UInv_InventoryBase::GetAllItemIds() const
{
	return ItemList.GetAllItemIds();
}

TArray<FInv_RealItemData> UInv_InventoryBase::GetAllItems() const
{
	return ItemList.GetAllItems();
}

// ========== 修改接口实现 ==========

bool UInv_InventoryBase::TryAddItemInstanceIndividually(const FInv_RealItemData& ItemData)
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

	const int32 EmptySlotIndex = FindFirstEmptySlotIndex();
	if (EmptySlotIndex == INDEX_NONE)
	{
		UE_LOG(LogInventorySystem, Warning,
		       TEXT("UInv_InventoryBase::TryAddItemInstance: No empty slot available"));
		return false;
	}

	FInv_RealItemData NewItemData = ItemData;
	NewItemData.SlotIndex = EmptySlotIndex;

	// 添加到 FastArray
	const bool bSuccess = ItemList.TryAddNewItem(NewItemData);

	if (bSuccess)
	{
		UE_LOG(LogInventorySystem, Display,
		       TEXT("UInv_InventoryBase::TryAddItemInstance: Successfully added item '%s' to SlotIndex %d"),
		       *ItemData.RealItemId.ToString(), EmptySlotIndex);
	}

	return bSuccess;
}

int32 UInv_InventoryBase::TryAddItemInstanceByStacking(const FInv_RealItemData& RealItemData)
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

bool UInv_InventoryBase::TryRemoveItem(const FGuid& ItemId)
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

bool UInv_InventoryBase::UpdateItem(const FGuid& ItemId, const FInv_RealItemData& NewItemData)
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

	const FInv_RealItemData* OldItemData = ItemList.FindItem(ItemId);
	if (!OldItemData)
	{
		UE_LOG(LogInventorySystem, Warning, TEXT("UInv_InventoryBase::UpdateItem: Failed to find item '%s'"),
		       *ItemId.ToString());
		return false;
	}

	FInv_RealItemData FinalItemData = NewItemData;
	if (FinalItemData.SlotIndex == INDEX_NONE)
	{
		FinalItemData.SlotIndex = OldItemData->SlotIndex;
	}

	// 执行更新
	const bool bSuccess = ItemList.UpdateItem(ItemId, FinalItemData);

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

/**
 * 尝试拾取一个带有 ItemComponent 的 Actor（服务端执行）
 *
 * @param ItemActor 要拾取的物品Actor（必须包含 UInv_ItemComponent）
 * @return 是否成功拾取（至少拾取了一部分）
 *
 * 注意：
 * - 仅在服务端有效
 * - 会自动验证物品是否可以被拾取
 * - 支持部分拾取（背包满时可能只拾取一部分）
 * - 成功后会触发 ItemComponent 的 OnPickedUp 事件
 */
bool UInv_InventoryBase::TryPickupItem(AActor* ItemActor)
{
	// 仅在服务端执行
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogInventorySystem, Warning, TEXT("UInv_InventoryBase::TryPickupItem: Can only execute on server"));
		return false;
	}

	// 验证 Actor 有效性
	if (!IsValid(ItemActor))
	{
		UE_LOG(LogInventorySystem, Warning, TEXT("UInv_InventoryBase::TryPickupItem: Invalid ItemActor"));
		return false;
	}

	// 获取 ItemComponent
	UInv_ItemComponent* ItemComp = ItemActor->FindComponentByClass<UInv_ItemComponent>();
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
	FInv_RealItemData& ItemData = ItemComp->GetItemDataMutable();

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

	// 尝试不使用堆叠添加物品
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

	//只要捡起了一部分的物品,就会调用这个函数
	if (bSuccess)
	{
		ItemComp->OnPickedUp(GetOwner());
	}

	return bSuccess;
}

// ========== RPC 实现 ==========

void UInv_InventoryBase::ServerPickupItem_Implementation(AActor* ItemActor)
{
	TryPickupItem(ItemActor);
}

void UInv_InventoryBase::ServerRequestMoveItem_Implementation(AActor* SourceInventoryOwner, FGuid SourceItemId,
                                                              AActor* TargetInventoryOwner, int32 TargetSlotIndex)
{
	UInv_InventoryBase* SourceInventory = SourceInventoryOwner
		                                      ? SourceInventoryOwner->FindComponentByClass<UInv_InventoryBase>()
		                                      : nullptr;
	UInv_InventoryBase* TargetInventory = TargetInventoryOwner
		                                      ? TargetInventoryOwner->FindComponentByClass<UInv_InventoryBase>()
		                                      : nullptr;

	TryMoveItemBetweenInventories(SourceInventory, SourceItemId, TargetInventory, TargetSlotIndex);
}

// ========== 事件响应默认实现 ==========

void UInv_InventoryBase::OnItemAdded(const FInv_RealItemData& ItemData)
{
	if (IsInventoryFull())
	{
		UE_LOG(LogInventorySystem, Display,
		       TEXT("UInv_InventoryBase::OnItemAdded: Inventory is full after adding item '%s'"),
		       *ItemData.RealItemId.ToString());
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		OnItemAddedCustomEvent.Broadcast(ItemData);
	}

	for (const TPair<EInv_InventoryWidgetType, TObjectPtr<UInv_InventoryWidgetBase>>& WidgetPair :
	     InventoryWidgetInstances)
	{
		if (IsValid(WidgetPair.Value))
		{
			WidgetPair.Value->AddItem(ItemData);
		}
	}

	// 基类默认实现：仅记录日志
	UE_LOG(LogInventorySystem, Log, TEXT("UInv_InventoryBase::OnItemAdded: Item '%s' added (Role: %s)"),
	       *ItemData.RealItemId.ToString(),
	       GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));
}

void UInv_InventoryBase::OnItemChanged(const FInv_RealItemData& ItemData)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		OnItemChangedCustomEvent.Broadcast(ItemData);
	}

	for (const TPair<EInv_InventoryWidgetType, TObjectPtr<UInv_InventoryWidgetBase>>& WidgetPair :
	     InventoryWidgetInstances)
	{
		if (IsValid(WidgetPair.Value))
		{
			WidgetPair.Value->UpdateItem(ItemData);
		}
	}

	// 基类默认实现：仅记录日志
	UE_LOG(LogInventorySystem, Display, TEXT("UInv_InventoryBase::OnItemChanged: Item '%s' changed (Role: %s)"),
	       *ItemData.RealItemId.ToString(),
	       GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));
}

void UInv_InventoryBase::OnItemRemoved(FGuid ItemId)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		OnItemRemovedCustomEvent.Broadcast(ItemId);
	}

	for (const TPair<EInv_InventoryWidgetType, TObjectPtr<UInv_InventoryWidgetBase>>& WidgetPair :
	     InventoryWidgetInstances)
	{
		if (IsValid(WidgetPair.Value))
		{
			WidgetPair.Value->RemoveItem(ItemId);
		}
	}

	// 基类默认实现：仅记录日志
	UE_LOG(LogInventorySystem, Display, TEXT("UInv_InventoryBase::OnItemRemoved: Item '%s' removed (Role: %s)"),
	       *ItemId.ToString(),
	       GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));
}

// ========== 验证接口默认实现 ==========

bool UInv_InventoryBase::CanAddItemEntirely(const FInv_RealItemData& ItemData) const
{
	if (IsInventoryFull())
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

	return true;
}

bool UInv_InventoryBase::CanRemoveItem(const FGuid& ItemId) const
{
	// 基类默认实现：只要物品存在就允许移除
	return ItemList.ContainRealItem(ItemId);
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

APlayerController* UInv_InventoryBase::ResolveInventoryWidgetOwningPlayer(APlayerController* OwningPlayer) const
{
	if (IsValid(OwningPlayer))
	{
		return OwningPlayer;
	}

	return GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
}

UInv_InventoryWidgetOwnerSubsystem* UInv_InventoryBase::ResolveInventoryWidgetOwnerSubsystem(
	APlayerController* OwningPlayer) const
{
	OwningPlayer = ResolveInventoryWidgetOwningPlayer(OwningPlayer);
	if (!IsValid(OwningPlayer))
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = OwningPlayer->GetLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UInv_InventoryWidgetOwnerSubsystem>() : nullptr;
}

TSubclassOf<UInv_InventoryWidgetBase> UInv_InventoryBase::ResolveInventoryWidgetClass(
	EInv_InventoryWidgetType InWidgetType) const
{
	for (const auto& [WidgetType, WidgetClass] : InventoryWidgetConfigs)
	{
		if (WidgetType == InWidgetType && IsValid(WidgetClass))
		{
			return WidgetClass;
		}
	}

	return InventoryWidgetClass;
}

int32 UInv_InventoryBase::FindFirstEmptySlotIndex() const
{
	for (int32 SlotIndex = 0; SlotIndex < MaxSlots; ++SlotIndex)
	{
		if (!ItemList.IsSlotOccupied(SlotIndex))
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}

bool UInv_InventoryBase::IsValidInventorySlotIndex(int32 SlotIndex) const
{
	return SlotIndex >= 0 && SlotIndex < MaxSlots;
}

const FInv_RealItemData* UInv_InventoryBase::FindItemBySlotIndex(int32 SlotIndex) const
{
	return ItemList.FindItemBySlotIndex(SlotIndex);
}

bool UInv_InventoryBase::CanAcceptItemAtSlot(const FInv_RealItemData& ItemData, int32 SlotIndex,
                                             const FGuid& IgnoredItemId) const
{
	if (!IsValidInventorySlotIndex(SlotIndex))
	{
		return false;
	}

	if (bUseItemTypeFilter && !IsItemTypeAllowed(ItemData))
	{
		return false;
	}

	const FInv_RealItemData* OccupiedItem = FindItemBySlotIndex(SlotIndex);
	if (!OccupiedItem)
	{
		return true;
	}

	return IgnoredItemId.IsValid() && OccupiedItem->RealItemId == IgnoredItemId;
}

int32 UInv_InventoryBase::GetStackTransferCount(const FInv_RealItemData& SourceItemData,
                                                const FInv_RealItemData& TargetItemData) const
{
	if (SourceItemData.VirtualItemDataHandle != TargetItemData.VirtualItemDataHandle)
	{
		return 0;
	}

	const FInv_VirtualItemData* VirtualItemData = SourceItemData.GetVirtualItemData();
	const FInv_MaxStackFragment* StackFragment = VirtualItemData
		                                             ? VirtualItemData->GetFragmentOfType<FInv_MaxStackFragment>()
		                                             : nullptr;
	if (!StackFragment)
	{
		return 0;
	}

	return FMath::Min(SourceItemData.StackCount, StackFragment->MaxStackCount - TargetItemData.StackCount);
}

bool UInv_InventoryBase::TryMoveItemBetweenInventories(UInv_InventoryBase* SourceInventory, const FGuid& SourceItemId,
                                                       UInv_InventoryBase* TargetInventory, int32 TargetSlotIndex)
{
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogInventorySystem, Warning,
		       TEXT("UInv_InventoryBase::TryMoveItemBetweenInventories: Can only execute on server"));
		return false;
	}

	if (!IsValid(SourceInventory) || !IsValid(TargetInventory))
	{
		UE_LOG(LogInventorySystem, Warning,
		       TEXT("UInv_InventoryBase::TryMoveItemBetweenInventories: SourceInventory or TargetInventory is invalid"
		       ));
		return false;
	}

	if (!TargetInventory->IsValidInventorySlotIndex(TargetSlotIndex))
	{
		UE_LOG(LogInventorySystem, Warning,
		       TEXT("UInv_InventoryBase::TryMoveItemBetweenInventories: Invalid TargetSlotIndex %d"),
		       TargetSlotIndex);
		return false;
	}

	const FInv_RealItemData* SourceItemData = SourceInventory->FindItemById(SourceItemId);
	if (!SourceItemData)
	{
		UE_LOG(LogInventorySystem, Warning,
		       TEXT("UInv_InventoryBase::TryMoveItemBetweenInventories: Failed to find source item '%s'"),
		       *SourceItemId.ToString());
		return false;
	}

	if (SourceInventory == TargetInventory && SourceItemData->SlotIndex == TargetSlotIndex)
	{
		return false;
	}

	const FInv_RealItemData* TargetItemData = TargetInventory->FindItemBySlotIndex(TargetSlotIndex);
	if (!TargetItemData)
	{
		return TryTransferItemToEmptySlot(SourceInventory, SourceItemId, TargetInventory, TargetSlotIndex);
	}

	if (TargetItemData->RealItemId == SourceItemId)
	{
		return false;
	}

	if (GetStackTransferCount(*SourceItemData, *TargetItemData) > 0)
	{
		return TryStackItems(SourceInventory, SourceItemId, TargetInventory, TargetItemData->RealItemId);
	}

	return TrySwapItems(SourceInventory, SourceItemId, TargetInventory, TargetItemData->RealItemId);
}

bool UInv_InventoryBase::TryTransferItemToEmptySlot(UInv_InventoryBase* SourceInventory, const FGuid& SourceItemId,
                                                    UInv_InventoryBase* TargetInventory, int32 TargetSlotIndex)
{
	const FInv_RealItemData* SourceItemData = SourceInventory->FindItemById(SourceItemId);
	if (!SourceItemData)
	{
		return false;
	}

	if (!TargetInventory->CanAcceptItemAtSlot(*SourceItemData, TargetSlotIndex))
	{
		return false;
	}

	if (SourceInventory == TargetInventory)
	{
		FInv_RealItemData MovedItemData = *SourceItemData;
		MovedItemData.SlotIndex = TargetSlotIndex;
		return SourceInventory->UpdateItem(SourceItemId, MovedItemData);
	}

	FInv_RealItemData MovedItemData = *SourceItemData;
	MovedItemData.SlotIndex = TargetSlotIndex;

	if (!TargetInventory->ItemList.TryAddNewItem(MovedItemData))
	{
		return false;
	}

	if (!SourceInventory->TryRemoveItem(SourceItemId))
	{
		TargetInventory->ItemList.RemoveItem(SourceItemId);
		return false;
	}

	return true;
}

bool UInv_InventoryBase::TryStackItems(UInv_InventoryBase* SourceInventory, const FGuid& SourceItemId,
                                       UInv_InventoryBase* TargetInventory, const FGuid& TargetItemId)
{
	const FInv_RealItemData* SourceItemData = SourceInventory->FindItemById(SourceItemId);
	const FInv_RealItemData* TargetItemData = TargetInventory->FindItemById(TargetItemId);
	if (!SourceItemData || !TargetItemData)
	{
		return false;
	}

	const int32 StackTransferCount = GetStackTransferCount(*SourceItemData, *TargetItemData);
	if (StackTransferCount <= 0)
	{
		return false;
	}

	const FInv_RealItemData OldTargetItemData = *TargetItemData;
	FInv_RealItemData NewTargetItemData = *TargetItemData;
	NewTargetItemData.StackCount += StackTransferCount;

	if (!TargetInventory->UpdateItem(TargetItemId, NewTargetItemData))
	{
		return false;
	}

	FInv_RealItemData NewSourceItemData = *SourceItemData;
	NewSourceItemData.StackCount -= StackTransferCount;

	if (NewSourceItemData.StackCount <= 0)
	{
		if (!SourceInventory->TryRemoveItem(SourceItemId))
		{
			TargetInventory->UpdateItem(TargetItemId, OldTargetItemData);
			return false;
		}

		return true;
	}

	if (!SourceInventory->UpdateItem(SourceItemId, NewSourceItemData))
	{
		TargetInventory->UpdateItem(TargetItemId, OldTargetItemData);
		return false;
	}

	return true;
}

bool UInv_InventoryBase::TrySwapItems(UInv_InventoryBase* SourceInventory, const FGuid& SourceItemId,
                                      UInv_InventoryBase* TargetInventory, const FGuid& TargetItemId)
{
	const FInv_RealItemData* SourceItemData = SourceInventory->FindItemById(SourceItemId);
	const FInv_RealItemData* TargetItemData = TargetInventory->FindItemById(TargetItemId);
	if (!SourceItemData || !TargetItemData)
	{
		return false;
	}

	if (!SourceInventory->CanAcceptItemAtSlot(*TargetItemData, SourceItemData->SlotIndex, SourceItemId))
	{
		return false;
	}

	if (!TargetInventory->CanAcceptItemAtSlot(*SourceItemData, TargetItemData->SlotIndex, TargetItemId))
	{
		return false;
	}

	const FInv_RealItemData OldSourceItemData = *SourceItemData;
	const FInv_RealItemData OldTargetItemData = *TargetItemData;

	FInv_RealItemData NewSourceItemData = *SourceItemData;
	FInv_RealItemData NewTargetItemData = *TargetItemData;
	NewSourceItemData.SlotIndex = OldTargetItemData.SlotIndex;
	NewTargetItemData.SlotIndex = OldSourceItemData.SlotIndex;

	if (!TargetInventory->UpdateItem(TargetItemId, NewTargetItemData))
	{
		return false;
	}

	if (!SourceInventory->UpdateItem(SourceItemId, NewSourceItemData))
	{
		TargetInventory->UpdateItem(TargetItemId, OldTargetItemData);
		return false;
	}

	return true;
}

bool UInv_InventoryBase::IsItemTypeAllowed(const FInv_RealItemData& ItemData) const
{
	if (!bUseItemTypeFilter)
	{
		return true;
	}

	// 获取物品的类型标签
	const FInv_VirtualItemData* VirtualData = ItemData.GetVirtualItemData();
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
	if (IsInventoryFull())
	{
		UE_LOG(LogInventorySystem, Display,
		       TEXT("UInv_InventoryBase::OnItemSplitFunc: Inventory is full, cannot split item '%s'"),
		       *ItemId.ToString());
		return;
	}

	const int32 EmptySlotIndex = FindFirstEmptySlotIndex();
	if (EmptySlotIndex == INDEX_NONE)
	{
		UE_LOG(LogInventorySystem, Display,
		       TEXT("UInv_InventoryBase::OnItemSplitFunc: No empty slot for item '%s'"),
		       *ItemId.ToString());
		return;
	}

	if (!ItemList.TrySplitItem(ItemId, SplitCount, EmptySlotIndex))
	{
		UE_LOG(LogInventorySystem, Warning,
		       TEXT("UInv_InventoryBase::OnItemSplitFunc: Failed to split item '%s' with count %d"),
		       *ItemId.ToString(), SplitCount);
	}
}
