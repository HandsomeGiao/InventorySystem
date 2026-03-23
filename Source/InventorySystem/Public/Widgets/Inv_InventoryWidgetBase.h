#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/Data/Inv_RealItemData.h"
#include "Inv_InventoryWidgetBase.generated.h"

class UInv_ItemOptionsWidget;
class UInv_InventoryEntry;
class UPanelWidget;
class UUniformGridPanel;

/**
 * 玩家库存窗口 Widget
 *
 * 功能：
 * - 显示玩家的库存物品
 * - 管理多个 UInv_InventoryEntry 格子
 */
UCLASS(Blueprintable, BlueprintType)
class INVENTORYSYSTEM_API UInv_InventoryWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	// ========== 修改接口 ==========

	// 初始化
	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void InitializeInventory(int32 InTotalSlots, int32 InColumnsPerRow);
	//物品修改
	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void AddItem(const FInv_RealItemData& ItemData);
	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void UpdateItem(const FInv_RealItemData& ItemData);
	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void RemoveItem(const FGuid& ItemId);
	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void UpdateInventory(const TArray<FInv_RealItemData>& ItemDataArray);
	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void UpdateSlot(const FInv_RealItemData& ItemData);
	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void ClearSlot(int32 SlotIndex);
	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	void ClearAllSlots();

	// ========== 查询接口 ==========

	UFUNCTION(BlueprintPure, Category = "Inventory System|UI")
	int32 GetSlotCount() const { return InventoryEntries.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
	int32 FindSlotIdxByItemId(const FGuid& ItemId) const;

	bool IsValidSlotIndex(int32 SlotIndex) const;

	// ========= 事件广播 ==========
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemDrop, FGuid, SourceItemId, FGuid, TargetItemId);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemSplit, FGuid, ItemId, int32, SplitCount);

	FOnItemDrop OnItemDrop;
	FOnItemSplit OnItemSplit;

protected:
	// ========= 辅助函数 ==============

	UFUNCTION()
	void OnItemDroppedFunc(FGuid SourceItemId, FGuid TargetItemId);
	UFUNCTION()
	void OnItemOptionsFunc(int32 WidgetIndex);
	UFUNCTION()
	void OnItemSplitFunc(FGuid ItemId, int32 SplitCount);

	UInv_InventoryEntry* CreateEntryWidget();

	// ========== 配置参数 ==========

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory System|UI")
	TSubclassOf<UInv_InventoryEntry> EntryWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory System|UI")
	TSubclassOf<UInv_ItemOptionsWidget> ItemOptionsWidgetClass;

	// ========== 私有变量 ============

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UUniformGridPanel* InventoryGrid;

	int32 ColumnsPerRow;
	int32 TotalSlots;
	TWeakObjectPtr<UInv_ItemOptionsWidget> ItemOptionsWidgetInstance;

	UPROPERTY()
	TArray<UInv_InventoryEntry*> InventoryEntries;
};