#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/Data/Inv_RealItemData.h"
#include "Inv_InventoryEntry.generated.h"

class UInv_ItemOptionsWidget;
// 前向声明
class UImage;
class UTextBlock;

/**
 * 库存物品格子 Widget
 */
UCLASS(Blueprintable, BlueprintType)
class INVENTORYSYSTEM_API UInv_InventoryEntry : public UUserWidget
{
    GENERATED_BODY()

public:
    // ========== 公共接口 ==========

    /**
     * 设置物品信息并更新 UI
     * @param RealItemData 物品数据
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
    void SetInfo(const FInv_RealItemData &RealItemData);

    const FInv_RealItemData& GetCurrentItemData() const { return  CurrentItemData; }

    /**
     * 清空格子（不显示任何物品）
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System|UI")
    void ClearEntry();

    /**
     * 获取当前显示的物品 ID（如果有）
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System|UI")
    FGuid GetCurrentItemId() const { return CurrentItemData.RealItemId; }

    /**
     * 检查格子是否为空
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System|UI")
    bool IsEmpty() const { return !CurrentItemData.RealItemId.IsValid(); }

    // ========== 拖拽功能 ==========
    virtual FReply NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) override;
    virtual void NativeOnDragDetected(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent,
        UDragDropOperation *&OutOperation) override;
    virtual bool NativeOnDrop(const FGeometry &InGeometry, const FDragDropEvent &InDragDropEvent,
        UDragDropOperation *InOperation) override;

    // 声明拖拽时的多播事件
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemDrop, FGuid, SourceItemId,FGuid, TargetItemId);
    FOnItemDrop OnItemDrop;

    // ========= 物品选项事件 ==========
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemOptions, int32, WidgetIndex);
    FOnItemOptions OnItemOptions;

    void SetWidgetIndex(int32 InIndex) { WidgetIndex = InIndex; }
protected:
    // ========== UI 组件（蓝图绑定）==========

    /**
     * 物品图标
     * 在蓝图中添加名为 "ItemIcon" 的 Image 组件
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage *ItemIcon;

    /**
     * 物品数量文本
     * 在蓝图中添加名为 "ItemCount" 的 TextBlock 组件
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock *ItemCount;

    int32 WidgetIndex;

private:
    // ========== 内部数据 ==========

    /**
     * 当前显示的物品 
     * 用于追踪当前显示的物品
     */
    FInv_RealItemData CurrentItemData;

    // ========== 内部辅助方法 ==========

    /**
     * 更新图标显示
     * @param Icon 要显示的图标纹理
     */
    void UpdateIcon(UTexture2D *Icon);

    /**
     * 更新数量文本
     */
    void UpdateCount();
};