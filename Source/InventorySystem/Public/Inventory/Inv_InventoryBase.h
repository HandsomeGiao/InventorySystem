#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/FastArray/Inv_ItemFastArray.h"
#include "GameplayTagContainer.h"
#include "Widgets/Inv_InventoryWidgetTypes.h"
#include "Inv_InventoryBase.generated.h"

class APlayerController;
class UInv_InventoryWidgetBase;
class UInv_InventoryWidgetOwnerSubsystem;

UCLASS(ClassGroup = (InventorySystem), meta = (BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UInv_InventoryBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UInv_InventoryBase();

	// ========== 查询接口（客户端和服务端都可用）==========

	const FInv_RealItemData* FindItemById(const FGuid& ItemId) const;
	bool ContainsItem(const FGuid& ItemId) const;
	int32 GetItemsCount() const;
	bool IsInventoryEmpty() const;
	TArray<FGuid> GetAllItemIds() const;
	TArray<FInv_RealItemData> GetAllItems() const;
	bool IsInventoryFull() const { return MaxSlots > 0 && GetItemsCount() >= MaxSlots; }
	int32 GetRemainingSlots() const { return MaxSlots - GetItemsCount(); }
	int32 GetMaxSlots() const { return MaxSlots; }

	// ========== UI接口 ==========

	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	UInv_InventoryWidgetBase* CreateInventoryWidget(APlayerController* OwningPlayer = nullptr,
	                                                bool bAddToViewport = true);

	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void BindInventoryWidget(UInv_InventoryWidgetBase* InInventoryWidget);

	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void UnbindInventoryWidget();

	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void DestroyInventoryWidget();

	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void RefreshInventoryWidget();

	UFUNCTION(BlueprintPure, Category = "Inventory System|UI")
	UInv_InventoryWidgetBase* GetInventoryWidget() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	UInv_InventoryWidgetBase* CreateInventoryWidgetByType(EInv_InventoryWidgetType WidgetType,
	                                                      APlayerController* OwningPlayer = nullptr,
	                                                      bool bAddToViewport = true);

	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void BindInventoryWidgetByType(EInv_InventoryWidgetType WidgetType, UInv_InventoryWidgetBase* InInventoryWidget);

	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void UnbindInventoryWidgetByType(EInv_InventoryWidgetType WidgetType);

	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void DestroyInventoryWidgetByType(EInv_InventoryWidgetType WidgetType);

	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void RefreshInventoryWidgetByType(EInv_InventoryWidgetType WidgetType);

	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void RefreshAllInventoryWidgets();

	UFUNCTION(BlueprintPure, Category = "Inventory System|UI")
	UInv_InventoryWidgetBase* GetInventoryWidgetByType(EInv_InventoryWidgetType WidgetType) const;

	UFUNCTION(BlueprintPure, Category = "Inventory System")
	EInv_InventoryOwnerType GetInventoryOwnerType() const { return InventoryOwnerType; }

	UFUNCTION(BlueprintCallable, Category = "Inventory System")
	void SetInventoryOwnerType(EInv_InventoryOwnerType InInventoryOwnerType) { InventoryOwnerType = InInventoryOwnerType; }

	UFUNCTION(BlueprintPure, Category = "Inventory System|UI")
	EInv_InventoryWidgetType ResolveInventoryWidgetType(APlayerController* ViewingPlayer = nullptr) const;

	void RequestMoveItem(UInv_InventoryBase* SourceInventory, const FGuid& SourceItemId,
	                     UInv_InventoryBase* TargetInventory, int32 TargetSlotIndex);
	bool CanSendServerCommand() const;

	// ========== 修改接口（仅服务端）==========

	virtual bool TryAddItemInstanceIndividually(const FInv_RealItemData& ItemData);
	int32 TryAddItemInstanceByStacking(const FInv_RealItemData& RealItemData);
	virtual bool TryRemoveItem(const FGuid& ItemId);
	virtual bool UpdateItem(const FGuid& ItemId, const FInv_RealItemData& NewItemData);
	virtual void ClearAllItems();
	virtual bool TryPickupItem(AActor* ItemActor);

	// ========== 修改接口(客户端服务器均可) ==========

	UFUNCTION(Server, Reliable, Category = "Inventory System")
	void ServerPickupItem(AActor* ItemActor);

	// ========== 委托定义 ==========

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAddedDelegate, const FInv_RealItemData&, ItemData);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemChangedDelegate, const FInv_RealItemData&, ItemData);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemovedDelegate, FGuid, ItemId);

	/**
	 *  可选修饰性事件,在DS上不会被调用
	 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory System|Events")
	FOnItemAddedDelegate OnItemAddedCustomEvent;
	UPROPERTY(BlueprintAssignable, Category = "Inventory System|Events")
	FOnItemChangedDelegate OnItemChangedCustomEvent;
	UPROPERTY(BlueprintAssignable, Category = "Inventory System|Events")
	FOnItemRemovedDelegate OnItemRemovedCustomEvent;

protected:
	// ========== override func =============

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ========== 配置属性 ==========

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System",
		meta = (ClampMin = "0", UIMin = "0", UIMax = "200"))
	int32 MaxSlots{40};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System")
	int32 PerRowCount{5};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System")
	EInv_InventoryOwnerType InventoryOwnerType{EInv_InventoryOwnerType::Other};

	/**
	 * 允许存放的物品类型标签（空表示允许所有类型）
	 * 例如：["Item.Equipment", "Item.Consumable"]
	 */
	UPROPERTY(EditAnywhere, Category = "Inventory System|UI")
	bool bUseItemTypeFilter{false};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category ="Inventory System|UI")
	FGameplayTagContainer AllowedItemTypes;

	// Inventory窗口类
	UPROPERTY(EditAnywhere, Category = "Inventory System|UI")
	TSubclassOf<UInv_InventoryWidgetBase> InventoryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System|UI")
	EInv_InventoryWidgetType DefaultInventoryWidgetType{EInv_InventoryWidgetType::PlayerSelf};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System|UI")
	TArray<FInv_InventoryWidgetConfig> InventoryWidgetConfigs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System|UI")
	bool bAutoCreateInventoryWidget{true};

	// ========== 验证接口（可被子类重写）==========

	// 能否将该物品不做堆叠地添加
	virtual bool CanAddItemEntirely(const FInv_RealItemData& ItemData) const;
	virtual bool CanRemoveItem(const FGuid& ItemId) const;

	// ========== 内部辅助方法 ==========

	virtual void BindFastArrayDelegates();
	virtual void UnbindFastArrayDelegates();

private:
	// ========= 私有变量 ============

	FDelegateHandle OnItemAddedHandle;
	FDelegateHandle OnItemChangedHandle;
	FDelegateHandle OnItemRemovedHandle;

	UPROPERTY(Replicated)
	FInv_ItemList ItemList;

	UPROPERTY(Transient)
	TMap<EInv_InventoryWidgetType, TObjectPtr<UInv_InventoryWidgetBase>> InventoryWidgetInstances;

	// ========== 内部辅助方法 ==========

	APlayerController* ResolveInventoryWidgetOwningPlayer(APlayerController* OwningPlayer) const;
	UInv_InventoryWidgetOwnerSubsystem* ResolveInventoryWidgetOwnerSubsystem(APlayerController* OwningPlayer) const;
	TSubclassOf<UInv_InventoryWidgetBase> ResolveInventoryWidgetClass(EInv_InventoryWidgetType InWidgetType) const;
	int32 FindFirstEmptySlotIndex() const;
	bool IsValidInventorySlotIndex(int32 SlotIndex) const;
	const FInv_RealItemData* FindItemBySlotIndex(int32 SlotIndex) const;
	bool CanAcceptItemAtSlot(const FInv_RealItemData& ItemData, int32 SlotIndex,
	                         const FGuid& IgnoredItemId = FGuid()) const;
	int32 GetStackTransferCount(const FInv_RealItemData& SourceItemData, const FInv_RealItemData& TargetItemData) const;
	bool TryMoveItemBetweenInventories(UInv_InventoryBase* SourceInventory, const FGuid& SourceItemId,
	                                   UInv_InventoryBase* TargetInventory, int32 TargetSlotIndex);
	bool TryTransferItemToEmptySlot(UInv_InventoryBase* SourceInventory, const FGuid& SourceItemId,
	                                UInv_InventoryBase* TargetInventory, int32 TargetSlotIndex);
	bool TryStackItems(UInv_InventoryBase* SourceInventory, const FGuid& SourceItemId,
	                   UInv_InventoryBase* TargetInventory, const FGuid& TargetItemId);
	bool TrySwapItems(UInv_InventoryBase* SourceInventory, const FGuid& SourceItemId,
	                  UInv_InventoryBase* TargetInventory, const FGuid& TargetItemId);
	bool IsItemTypeAllowed(const FInv_RealItemData& ItemData) const;

	UFUNCTION()
	void OnItemAdded(const FInv_RealItemData& ItemData);
	UFUNCTION()
	void OnItemChanged(const FInv_RealItemData& ItemData);
	UFUNCTION()
	void OnItemRemoved(FGuid ItemId);

	UFUNCTION(Server, Reliable)
	void ServerRequestMoveItem(AActor* SourceInventoryOwner, FGuid SourceItemId, AActor* TargetInventoryOwner,
	                           int32 TargetSlotIndex);
	UFUNCTION(Server, Reliable)
	void ServerOnItemSplitFunc(FGuid ItemId, int32 SplitCount);
};
