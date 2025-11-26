#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Items/Data/Inv_RealItemData.h"
#include "Inv_ItemFastArray.generated.h"

// 前向声明
struct FInv_ItemList;

/**
 * FastArray 单个物品元素
 *
 * 设计要点：
 * - 存储完整的 FInv_RealItemData（网络同步必需）
 * - 使用 RealItemId（GUID）作为唯一标识
 * - 支持高效的网络增量同步
 */
USTRUCT(BlueprintType)
struct INVENTORYSYSTEM_API FInv_ItemFastArrayItem : public FFastArraySerializerItem
{
    GENERATED_BODY()

    FInv_ItemFastArrayItem()
    {
    }

    explicit FInv_ItemFastArrayItem(const FInv_RealItemData &InItemData) : RealItemData(InItemData)
    {
    }

    // ========== 数据成员 ==========

    /**
     * 完整的物品实例数据
     * 必须存储在这里以支持网络同步
     */
    UPROPERTY()
    FInv_RealItemData RealItemData;

    // ========== 辅助方法 ==========

    /**
     * 获取物品的唯一 ID
     */
    FORCEINLINE FGuid GetItemId() const { return RealItemData.RealItemId; }

    /**
     * 获取物品数据的 const 引用（避免拷贝）
     */
    FORCEINLINE const FInv_RealItemData &GetItemData() const { return RealItemData; }

    /**
     * 获取物品数据的可修改引用（服务端使用）
     */
    FORCEINLINE FInv_RealItemData &GetItemDataMutable() { return RealItemData; }
};

/**
 * FastArray 物品列表
 */
USTRUCT(BlueprintType)
struct INVENTORYSYSTEM_API FInv_ItemList : public FFastArraySerializer
{
    GENERATED_BODY()

    FInv_ItemList() : OwnerComponent(nullptr)
    {
    }

    explicit FInv_ItemList(UActorComponent *InComp) : OwnerComponent(InComp)
    {
    }

    // ========== 数据成员 ==========

    /**
     * 物品数组
     * FastArray 核心，自动处理增量同步
     */
    UPROPERTY()
    TArray<FInv_ItemFastArrayItem> Items;

    /**
     * 拥有此列表的组件
     * 用于访问外部上下文（如获取所有者、判断网络角色等）
     */
    TWeakObjectPtr<UActorComponent> OwnerComponent;

    // ==========数组变更回调
    void PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize);

    // ========== 委托定义 ==========

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemAdded, const FInv_RealItemData & /*ItemData*/);
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemChanged, const FInv_RealItemData & /*ItemData*/);
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, FGuid /*ItemId*/);

    /**
     * 委托实例
     */
    FOnItemAdded OnItemAdded;
    FOnItemChanged OnItemChanged;
    FOnItemRemoved OnItemRemoved;

    // ========== FastArray 核心接口 ==========

    /**
     * FastArray 序列化函数
     * 必须实现此方法以支持网络同步
     */
    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FInv_ItemFastArrayItem, FInv_ItemList>(Items, DeltaParms,
            *this);
    }

    // ========== 物品管理 API ==========

    /**
     * 添加物品到列表（仅服务端）
     * @param ItemData 要添加的物品数据
     * @return 是否成功添加
     */
    bool TryAddNewItem(const FInv_RealItemData &ItemData);

    /**
     *  尝试堆叠物品到已有物品上
     *  @param NewRealItem 要堆叠的物品数据
     *  @return 成功堆叠的数量
     */
    int32 TryStackItem(const FInv_RealItemData &NewRealItem);

    /**
     * 尝试将物品拖拽到另外一个物品上进行合并
     */
    void TryDropItem(FGuid SourceItemId, FGuid TargetItemId);

    /**
     *  尝试拆分物品
     */
    bool TrySplitItem(FGuid ItemId, int32 SplitCount);

    /**
     * 移除物品（仅服务端）
     * @param ItemId 要移除的物品 ID
     * @return 是否成功移除
     */
    bool RemoveItem(const FGuid &ItemId);

    /**
     * 移除指定索引的物品（仅服务端，内部使用）
     *
     * 警告：不要在外部调用！FastArray 索引不稳定。
     * 请使用 RemoveItem(const FGuid&) 代替。
     *
     * @param Index 物品在数组中的索引
     * @return 是否成功移除
     */
    bool RemoveItemAt(int32 Index);

    /**
     * 更新物品数据（仅服务端）
     * @param ItemId 要更新的物品 ID
     * @param NewItemData 新的物品数据
     * @return 是否成功更新
     */
    bool UpdateItem(const FGuid &ItemId, const FInv_RealItemData &NewItemData);

    /**
     * 清空所有物品（仅服务端）
     */
    void ClearAllItems();

    // ========== 查询 API（客户端和服务端都可用）==========

    /**
     * 通过 ID 查找物品（推荐）
     * @param ItemId 物品的唯一 ID
     * @return 物品数据的 const 指针，未找到返回 nullptr
     * 注意：线性搜索 O(n)，如需高频查询请在外部维护映射
     */
    const FInv_RealItemData *FindItem(const FGuid &ItemId) const;

    /**
     * 通过 ID 查找物品（可修改版本，仅服务端使用）
     * @param ItemId 物品的唯一 ID
     * @return 物品数据的指针，未找到返回 nullptr
     */
    FInv_RealItemData *FindItemMutable(const FGuid &ItemId);

    /**
     * 获取所有物品的 ID 列表
     * @return 所有物品 ID 的数组
     */
    TArray<FGuid> GetAllItemIds() const;

    /**
     * 遍历所有物品（使用回调）
     * @param Callback 对每个物品调用的回调函数
     */
    void ForEachItem(TFunctionRef<void(const FInv_RealItemData &)> Callback) const;

    /**
     * 通过索引获取物品（内部使用，不推荐外部调用）
     *
     * 警告：FastArray 索引不稳定！仅用于内部遍历。
     * 外部请使用 FindItem(FGuid) 或 ForEachItem()。
     *
     * @param Index 数组索引
     * @return 物品数据的 const 引用
     */
    const FInv_RealItemData &GetItemByIndex(int32 Index) const { return Items[Index].RealItemData; }

    /**
     * 通过索引获取物品（可修改版本，内部使用）
     *
     * 警告：仅服务端内部使用，不推荐外部调用！
     *
     * @param Index 数组索引
     * @return 物品数据的引用
     */
    FInv_RealItemData &GetItemByIndexMutable(int32 Index) { return Items[Index].RealItemData; }

    /**
     * 获取物品数量
     */
    int32 Num() const { return Items.Num(); }

    /**
     * 检查列表是否为空
     */
    bool IsEmpty() const { return Items.IsEmpty(); }

    /**
     * 检查是否包含指定 ID 的物品
     */
    bool ContainRealItem(const FGuid &ItemId) const;

    // ========== 内部辅助方法 ==========

    /**
     * 获取指定物品的索引（内部使用）
     *
     * 警告：不推荐外部调用！索引会随着元素增删而变化。
     *
     * @return 索引，未找到返回 INDEX_NONE
     */
    int32 IndexOfItem(const FGuid &ItemId) const;

    /**
     * 检查是否为服务端
     */
    bool IsServer() const;

};

// ========== FastArray 序列化支持 ==========

template <>
struct TStructOpsTypeTraits<FInv_ItemList> : public TStructOpsTypeTraitsBase2<FInv_ItemList>
{
    enum
    {
        WithNetDeltaSerializer = true,
    };
};