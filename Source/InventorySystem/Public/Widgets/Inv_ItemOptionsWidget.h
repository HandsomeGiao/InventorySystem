#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inv_ItemOptionsWidget.generated.h"

class UButton;
class USlider;
/**
 * 右键物品时,显示的选项窗口
 */
UCLASS()
class INVENTORYSYSTEM_API UInv_ItemOptionsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSplitInfoChanged, bool, bCanShow);

    UPROPERTY(BlueprintAssignable, Category = "Inventory System|Events")
    FOnSplitInfoChanged OnSplitInfoChanged;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemSplit, FGuid, ItemId, int32, SplitCount);

    FOnItemSplit OnItemSplit;

    virtual void NativeConstruct() override;

    void SetItemInfo(const FGuid &InItemId, int32 InItemStackCount);

    UPROPERTY(meta = (BindWidget))
    USlider *SplitSlider;
    UPROPERTY(meta = (BindWidget))
    UButton *SplitBtn;

private:
    FGuid ItemId;
    int32 ItemStackCount;

    UFUNCTION()
    void OnSplitBtnClicked();
};