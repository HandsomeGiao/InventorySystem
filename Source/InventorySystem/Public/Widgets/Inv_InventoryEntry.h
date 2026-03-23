#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/Data/Inv_RealItemData.h"
#include "Inv_InventoryEntry.generated.h"

class UInv_ItemOptionsWidget;
class UImage;
class UTextBlock;

UCLASS(Blueprintable, BlueprintType)
class INVENTORYSYSTEM_API UInv_InventoryEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	// ========== 外部查询接口 ==========

	const FInv_RealItemData& GetCurrentItemData() const { return CurrentItemData; }
	FGuid GetCurrentItemId() const { return CurrentItemData.RealItemId; }
	bool IsEmpty() const { return !CurrentItemData.RealItemId.IsValid(); }

	// ========== 外部修改接口 ==========
	void SetInfo(const FInv_RealItemData& RealItemData);
	void ClearEntry();
	void SetWidgetIndex(int32 InIndex) { WidgetIndex = InIndex; }

	// ========== 多播 ============
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemDrop, FGuid, SourceItemId, FGuid, TargetItemId);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemOptions, int32, WidgetIndex);

	FOnItemDrop OnItemDrop;
	FOnItemOptions OnItemOptions;

protected:
	// ========== override func =============

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	                                  UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	                          UDragDropOperation* InOperation) override;

	// ========== UI 组件（蓝图绑定）==========

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* ItemIcon;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* ItemCount;

private:
	// ========== 内部数据 ==========

	int32 WidgetIndex;
	FInv_RealItemData CurrentItemData;

	// ========== 内部辅助方法 ==========
	void UpdateIcon(UTexture2D* Icon);
	void UpdateCount();
};