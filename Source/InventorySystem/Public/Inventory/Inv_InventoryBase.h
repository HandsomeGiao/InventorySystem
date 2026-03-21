#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/FastArray/Inv_ItemFastArray.h"
#include "GameplayTagContainer.h"
#include "Inv_InventoryBase.generated.h"

class UInv_InventoryWidgetBase;

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
	bool IsInventoryFull() const { return MaxSlots > 0 && GetItemsCount() >= MaxSlots; }
	int32 GetRemainingSlots() const { return MaxSlots - GetItemsCount(); }
	int32 GetMaxSlots() const { return MaxSlots; }

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

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAddedDelegate, const FInv_RealItemData &, ItemData);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemChangedDelegate, const FInv_RealItemData &, ItemData);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System|UI",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0"))
	float PickupRange{300.0f};

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

	TWeakObjectPtr<UInv_InventoryWidgetBase> InventoryWidgetInstance;

	// ========== 内部辅助方法 ==========

	void CreateInvWidget();
	bool IsItemTypeAllowed(const FInv_RealItemData& ItemData) const;

	UFUNCTION()
	void OnItemAdded(const FInv_RealItemData& ItemData);
	UFUNCTION()
	void OnItemChanged(const FInv_RealItemData& ItemData);
	UFUNCTION()
	void OnItemRemoved(FGuid ItemId);

	UFUNCTION(Server, Reliable)
	void ServerOnItemDroppedFunc(FGuid SourceItemId, FGuid TargetItemId);
	UFUNCTION(Server, Reliable)
	void ServerOnItemSplitFunc(FGuid ItemId, int32 SplitCount);

};