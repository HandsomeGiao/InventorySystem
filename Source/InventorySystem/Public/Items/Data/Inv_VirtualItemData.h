#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Items/Fragments/VirtualItem/Inv_VirtualItemFragment.h"
#include "StructUtils/InstancedStruct.h"
#include "Inv_VirtualItemData.generated.h"

// Forward declaration
struct FInv_BaseRealItemFragment;

/**
 * 代表游戏世界内的某一种物品（虚拟/抽象的物品定义）
 * 例如：代表"香蕉"这种水果，"铁剑"这种武器
 * 存储在 DataTable 中，所有同类物品实例共享这份数据
 */
USTRUCT(BlueprintType)
struct INVENTORYSYSTEM_API FInv_VirtualItemData : public FTableRowBase
{
    GENERATED_BODY()

    // 物品的 GameplayTag 标识（用于快速查找和分类）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Definition")
    FGameplayTag ItemTag;

    // 静态、共享的物品数据（图标、网格大小、最大堆叠数等）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Definition")
    TArray<TInstancedStruct<FInv_VirtualItemFragment>> ItemFragments;

    // 实例Fragment模板：定义每个实例需要哪些动态属性及其默认值
    // 例如：武器需要耐久度Fragment（最大耐久度=100），食物需要新鲜度Fragment等
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Instance Templates" )
    TArray<TInstancedStruct<FInv_BaseRealItemFragment>> InstanceFragmentTemplates;

    // find fragment
    // 找到第一个符合类型的Fragment
    template <typename T> requires std::derived_from<T, FInv_VirtualItemFragment>
    const T *GetFragmentOfType() const;
    // 找到第一个符合类型的Fragment(可变)
    template <typename T> requires std::derived_from<T, FInv_VirtualItemFragment>
    T *GetFragmentOfTypeMutable();
};

template <typename T> requires std::derived_from<T, FInv_VirtualItemFragment>
const T *FInv_VirtualItemData::GetFragmentOfType() const
{
    for (const TInstancedStruct<FInv_VirtualItemFragment> &Fragment : ItemFragments)
    {
        if (const T *FragmentPtr = Fragment.GetPtr<T>()) { return FragmentPtr; }
    }
    return nullptr;
}

template <typename T> requires std::derived_from<T, FInv_VirtualItemFragment>
T *FInv_VirtualItemData::GetFragmentOfTypeMutable()
{
    for (TInstancedStruct<FInv_VirtualItemFragment> &Fragment : ItemFragments)
    {
        if (T *FragmentPtr = Fragment.GetMutablePtr<T>()) { return FragmentPtr; }
    }
    return nullptr;
}