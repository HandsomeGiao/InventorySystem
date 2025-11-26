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
    // ========== 公共接口 ==========

    UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
    void AddItem(const FInv_RealItemData &ItemData);
    UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
    void UpdateItem(const FInv_RealItemData &ItemData);
    UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
    void RemoveItem(const FGuid &ItemId);

    /**
     * 初始化库存窗口
     * 创建指定数量的物品格子
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
    void InitializeInventory(int32 InTotalSlots, int32 InColumnsPerRow);

    /**
     * 更新整个库存显示
     * @param ItemDataArray 物品数据数组
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
    void UpdateInventory(const TArray<FInv_RealItemData> &ItemDataArray);

    /**
     * 更新单个格子
     * @param ItemData 物品数据
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
    void UpdateSlot(const FInv_RealItemData &ItemData);

    /**
     * 清空单个格子
     * @param SlotIndex 格子索引
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
    void ClearSlot(int32 SlotIndex);

    /**
     * 清空所有格子
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
    void ClearAllSlots();

    /**
     * 获取格子总数
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System|UI")
    int32 GetSlotCount() const { return InventoryEntries.Num(); }

    /**
     * 通过物品 ID 查找格子索引
     * @param ItemId 物品 ID
     * @return 格子索引，未找到返回 INDEX_NONE
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
    int32 FindSlotByItemId(const FGuid &ItemId) const;

    // ========= 物品拖拽事件 ==========
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemDrop, FGuid, SourceItemId, FGuid, TargetItemId);
    FOnItemDrop OnItemDrop;

    // ========= 物品选项事件 ==========
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemSplit, FGuid, ItemId, int32, SplitCount);
    FOnItemSplit OnItemSplit;

protected:
    UFUNCTION()
    void OnItemDroppedFunc(FGuid SourceItemId, FGuid TargetItemId);
    UFUNCTION()
    void OnItemOptionsFunc(int32 WidgetIndex);
    UFUNCTION()
    void OnItemSplitFunc(FGuid ItemId,int32 SplitCount);

    // ========== 配置参数 ==========

    /**
     * 物品格子的 Widget 类
     * 必须是 UInv_InventoryEntry 或其子类
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory System|UI")
    TSubclassOf<UInv_InventoryEntry> EntryWidgetClass;

    /**
     * 容器面板（用于放置格子）
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UUniformGridPanel *InventoryGrid;

    /**
     * 每行的列数（用于网格布局）
     */
    int32 ColumnsPerRow;

    /**
     * 总格子数
     */
    int32 TotalSlots;

    /**
     * 选项窗口类
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory System|UI")
    TSubclassOf<UInv_ItemOptionsWidget> ItemOptionsWidgetClass;
    TWeakObjectPtr<UInv_ItemOptionsWidget> ItemOptionsWidgetInstance;

private:
    // ========== 内部数据 ==========

    /**
     * 所有物品格子的引用
     */
    UPROPERTY()
    TArray<UInv_InventoryEntry *> InventoryEntries;

    // ========== 内部辅助方法 ==========

    /**
     * 创建单个物品格子
     */
    UInv_InventoryEntry *CreateEntryWidget();

    /**
     * 验证格子索引是否有效
     */
    bool IsValidSlotIndex(int32 SlotIndex) const;
};