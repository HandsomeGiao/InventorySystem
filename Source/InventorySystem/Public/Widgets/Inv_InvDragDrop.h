#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Inv_InvDragDrop.generated.h"

class UInv_InventoryBase;

/**
 * 用于物品拖拽操作的 DragDropOperation 子类
 */
UCLASS()
class INVENTORYSYSTEM_API UInv_InvDragDrop : public UDragDropOperation
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TObjectPtr<UInv_InventoryBase> SourceInventory;

    // 被拖拽的物品数据
    FGuid SourceItemId;
    int32 SourceSlotIndex;
};
