// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Inv_ItemOptionsWidget.h"

#include "Components/Button.h"
#include "Components/Slider.h"

void UInv_ItemOptionsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);

    SplitBtn->OnClicked.AddDynamic(this, &ThisClass::OnSplitBtnClicked);

    SplitSlider->SetStepSize(1.f);
    SplitSlider->MouseUsesStep = true;
}

void UInv_ItemOptionsWidget::SetItemInfo(const FGuid &InItemId, int32 InItemStackCount)
{
    ItemId = InItemId, ItemStackCount = InItemStackCount;

    if (ItemStackCount <= 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("UInv_ItemOptionsWidget::SetItemInfo: ItemStackCount <= 1, cannot split."));
        SplitSlider->SetMinValue(0.f);
        SplitSlider->SetMaxValue(0.f);
        SplitSlider->SetValue(0.f);

        OnSplitInfoChanged.Broadcast(false);
    }
    else
    {
        SplitSlider->SetMinValue(1.f);
        SplitSlider->SetMaxValue(InItemStackCount - 1);
        SplitSlider->SetValue(1.f);

        OnSplitInfoChanged.Broadcast(true);
    }
}

void UInv_ItemOptionsWidget::OnSplitBtnClicked()
{
    OnItemSplit.Broadcast(ItemId, static_cast<int32>(SplitSlider->GetValue()));
    RemoveFromParent();
}