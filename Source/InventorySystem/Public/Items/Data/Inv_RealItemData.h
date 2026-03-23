#pragma once

#include "Inv_VirtualItemData.h"
#include "../Fragments/RealItem/Inv_RealItemFragment.h"
#include "StructUtils/InstancedStruct.h"
#include "Inv_RealItemData.generated.h"

/**
 * 代表游戏世界中具体存在的物品实例
 * 例如：5个香蕉、一把耐久度80%的铁剑、一个+3附魔的盾牌
 */
USTRUCT(BlueprintType)
struct INVENTORYSYSTEM_API FInv_RealItemData
{
	GENERATED_BODY()

	// 指向DataTable中的物品定义（共享数据）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InventorySystem | Item Instance")
	FDataTableRowHandle VirtualItemDataHandle;

	// 唯一实例ID
	UPROPERTY(BlueprintReadOnly, Category = "InventorySystem | Item Instance")
	FGuid RealItemId;

	// 堆叠数量,按理来说这应该抽象为一个Fragment,但是一开始我并没有这样去做,导致积重难返.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InventorySystem | Item Instance", meta = (ClampMin = "1"))
	int32 StackCount{1};

	// 实例特定的动态数据（耐久度、新鲜度、附魔等）,
	// TODO:这里我们设置为无法编辑,也就是说RealItemFragment只能由
	// virtual item data决定.但是有时候会需要编辑特定的RealItemFragment,以后再完善.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InventorySystem | Instance Data")
	TArray<TInstancedStruct<FInv_RealItemFragment>> InstanceFragments;

	// ========== 便捷访问方法 ==========

	/**
	 * 获取物品定义数据（共享的静态数据）
	 */
	const FInv_VirtualItemData* GetVirtualItemData() const
	{
		return VirtualItemDataHandle.GetRow<FInv_VirtualItemData>(TEXT("GetItemData"));
	}

	/**
	 * 获取实例Fragment（只读）
	 */
	template <typename T> requires std::derived_from<T, FInv_RealItemFragment>
	const T* GetInstanceFragment() const;

	/**
	 * 获取实例Fragment（可写）
	 */
	template <typename T> requires std::derived_from<T, FInv_RealItemFragment>
	T* GetInstanceFragmentMutable();

	/**
	 * 添加实例Fragment
	 */
	template <typename T> requires std::derived_from<T, FInv_RealItemFragment>
	T& AddInstanceFragment();

	/**
	 * 检查是否有某个Fragment
	 */
	template <typename T> requires std::derived_from<T, FInv_RealItemFragment>
	bool HasInstanceFragment() const;
};

// ========== 模板实现 ==========

template <typename T> requires std::derived_from<T, FInv_RealItemFragment>
const T* FInv_RealItemData::GetInstanceFragment() const
{
	for (const TInstancedStruct<FInv_RealItemFragment>& Fragment : InstanceFragments)
	{
		if (const T* FragmentPtr = Fragment.GetPtr<T>())
		{
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FInv_RealItemFragment>
T* FInv_RealItemData::GetInstanceFragmentMutable()
{
	for (TInstancedStruct<FInv_RealItemFragment>& Fragment : InstanceFragments)
	{
		if (T* FragmentPtr = Fragment.GetMutablePtr<T>())
		{
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FInv_RealItemFragment>
T& FInv_RealItemData::AddInstanceFragment()
{
	TInstancedStruct<FInv_RealItemFragment>& NewFragment = InstanceFragments.AddDefaulted_GetRef();
	NewFragment.InitializeAs<T>();
	return *NewFragment.GetMutablePtr<T>();
}

template <typename T> requires std::derived_from<T, FInv_RealItemFragment>
bool FInv_RealItemData::HasInstanceFragment() const
{
	return GetInstanceFragment<T>() != nullptr;
}