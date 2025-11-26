#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/FastArray/Inv_ItemFastArray.h"
#include "GameplayTagContainer.h"
#include "Inv_InventoryBase.generated.h"

class UInv_InventoryWidgetBase;
/**
 * 库存组件基类
 *
 * 核心功能：
 * - 管理物品存储（基于 FastArray，支持网络同步）
 * - 提供物品增删改查的基础接口
 * - 监听物品变化事件
 *
 * 设计原则：
 * - 作为基类，只提供核心功能，具体逻辑由子类实现
 * - 服务端权威：所有修改操作必须在服务端执行
 * - 事件驱动：通过虚函数响应物品变化
 *
 * 使用方式：
 * - 重写虚函数实现具体的业务逻辑
 */
UCLASS(ClassGroup = (InventorySystem), meta = (BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UInv_InventoryBase : public UActorComponent
{
    GENERATED_BODY()

public:
    // ========== 构造和初始化 ==========

    UInv_InventoryBase();

    // ========== UActorComponent 接口 ==========

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    void CreateInvWidget();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ========== 查询接口（客户端和服务端都可用）==========

    /**
     * 通过 ID 查找物品
     * @param ItemId 物品的唯一 ID
     * @return 物品数据的 const 指针，未找到返回 nullptr
     */
    const FInv_RealItemData *FindItemById(const FGuid &ItemId) const;

    /**
     * 检查是否包含指定物品
     * @param ItemId 物品的唯一 ID
     * @return 如果包含返回 true
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System")
    bool ContainsItem(const FGuid &ItemId) const;

    /**
     * 获取物品数量
     * @return 当前库存中的物品数量
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System")
    int32 GetItemCount() const;

    /**
     * 检查库存是否为空
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System")
    bool IsEmpty() const;

    /**
     * 获取所有物品 ID
     * @return 所有物品 ID 的数组
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System")
    TArray<FGuid> GetAllItemIds() const;

    // ========== 修改接口（仅服务端）==========
    /**
     * 尝试添加物品实例（服务端执行）
     *
     * @param ItemData 要添加的物品数据
     * @return 是否成功添加
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System")
    virtual bool TryAddItemInstanceIndividually(const FInv_RealItemData &ItemData);

    /**
     * 尝试部分添加物品实例（服务端执行）
     *
     * @param RealItemData 要添加的物品数据
     * @return 实际添加的数量
     */
    int32 TryAddItemInstanceByStacking(const FInv_RealItemData &RealItemData);

    /**
     * 尝试移除物品（服务端执行）
     *
     * @param ItemId 要移除的物品 ID
     * @return 是否成功移除
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System")
    virtual bool TryRemoveItem(const FGuid &ItemId);

    /**
     * 更新物品数据（仅服务端）
     *
     * @param ItemId 要更新的物品 ID
     * @param NewItemData 新的物品数据
     * @return 是否成功更新
     *
     * 注意：
     * - 仅在服务端有效
     * - NewItemData.RealItemId 必须与 ItemId 一致
     * - 用于服务端修改物品属性（如耐久度、新鲜度等）
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System")
    virtual bool UpdateItem(const FGuid &ItemId, const FInv_RealItemData &NewItemData);

    /**
     * 清空所有物品（服务端执行）
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System")
    virtual void ClearAllItems();

    // ========== 拾取物品接口 ==========

    /**
     * 尝试拾取一个带有 ItemComponent 的 Actor（服务端执行）
     *
     * @param ItemActor 要拾取的物品Actor（必须包含 UInv_ItemComponent）
     * @return 是否成功拾取（至少拾取了一部分）
     *
     * 注意：
     * - 仅在服务端有效
     * - 会自动验证物品是否可以被拾取
     * - 支持部分拾取（背包满时可能只拾取一部分）
     * - 成功后会触发 ItemComponent 的 OnPickedUp 事件
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System")
    virtual bool TryPickupItem(AActor *ItemActor);

    /**
    * 辅助函数：检测视野中的可拾取物品
    *
    * 从指定位置和方向执行射线检测，查找带有 Inv_ItemComponent 的 Actor
    *
    * @param StartLocation 射线起点（通常是相机位置）
    * @param Direction 射线方向（通常是相机朝向）
    * @param TraceDistance 射线检测距离（默认使用 PickupRange）
    * @return 检测到的可拾取物品 Actor，如果没有则返回 nullptr
    *
    * 使用示例（在 Character 蓝图中）：
    * - 获取相机位置和朝向
    * - 调用此函数获取目标物品
    * - 如果有物品，显示拾取提示UI
    * - 按下拾取键时，调用 ServerPickupItem
    */
    // TODO:将这种检测方法放到这里不应该,这种检测方法应该只针对于玩家的仓库类型.
    UFUNCTION(BlueprintCallable, Category = "Inventory System")
    AActor *DetectPickableItemInView(
        const FVector &StartLocation, const FVector &Direction, float TraceDistance = -1.0f) const;

    // ========== RPC 接口（客户端调用，服务端执行）==========

    /**
     * RPC：请求拾取物品
     *
     * @param ItemActor 要拾取的物品Actor
     *
     * 客户端调用此函数请求服务端拾取物品
     * 服务端会验证距离、权限、容量等条件
     *
     * 示例：
     * ```cpp
     * // 客户端代码
     * void AMyCharacter::OnPickupButtonPressed(AActor* ItemActor)
     * {
     *     UMyInventory* Inventory = FindComponentByClass<UMyInventory>();
     *     Inventory->ServerPickupItem(ItemActor);  // 自动在服务端执行
     * }
     * ```
     */
    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Inventory System")
    void ServerPickupItem(AActor *ItemActor);

    // ========== 委托定义 ==========

    /**
     * 物品添加委托（可在蓝图中绑定）
     */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAddedDelegate, const FInv_RealItemData &, ItemData);

    /**
     * 物品变化委托（可在蓝图中绑定）
     */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemChangedDelegate, const FInv_RealItemData &, ItemData);

    /**
     * 物品移除委托（可在蓝图中绑定）
     */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemovedDelegate, FGuid, ItemId);

    /**
     *  可选修饰性事件,仅用于添加额外的特效
     */
    UPROPERTY(BlueprintAssignable, Category = "Inventory System|Events")
    FOnItemAddedDelegate OnItemAddedEvent;

    /**
     *  可选修饰性事件,仅用于添加额外的特效
     */
    UPROPERTY(BlueprintAssignable, Category = "Inventory System|Events")
    FOnItemChangedDelegate OnItemChangedEvent;

    /**
     *  可选修饰性事件,仅用于添加额外的特效
     */
    UPROPERTY(BlueprintAssignable, Category = "Inventory System|Events")
    FOnItemRemovedDelegate OnItemRemovedEvent;

    // ========== 查询接口 ==========

    /**
     * 检查是否达到容量上限
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System")
    bool IsFull() const { return MaxSlots > 0 && GetItemCount() >= MaxSlots; }

    /**
     * 获取剩余槽位数
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System")
    int32 GetRemainingSlots() const { return MaxSlots > 0 ? MaxSlots - GetItemCount() : INT32_MAX; }

    /**
     * 获取最大槽位数
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System")
    int32 GetMaxSlots() const { return MaxSlots; }

protected:
    // ========== 配置属性 ==========

    /**
     * 最大物品槽位数
     * 0 表示无限制
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System",
        meta = (ClampMin = "0", UIMin = "0", UIMax = "200"))
    int32 MaxSlots{40};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System")
    int32 PerRowCount{5};

    // ========== 事件响应（可被子类重写）==========

    /**
     * 当物品被添加时调用
     *
     * @param ItemData 新添加的物品数据
     *
     * 注意：
     * - 服务端和客户端都会调用
     * - 用于响应物品变化，执行UI更新等操作
     * - 子类可重写以实现自定义逻辑
     * Dedicated server不会响应此类事件
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Inventory System")
    void OnItemAdded(const FInv_RealItemData &ItemData);

    /**
     * 当物品数据变化时调用
     *
     * @param ItemData 变化后的物品数据
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Inventory System")
    void OnItemChanged(const FInv_RealItemData &ItemData);

    /**
     * 当物品被移除时调用
     *
     * @param ItemId 被移除物品的 ID
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Inventory System")
    void OnItemRemoved(FGuid ItemId);

    // ========== 验证接口（可被子类重写）==========

    /**
     * 验证是否可以添加物品
     *
     * @param ItemData 要添加的物品数据
     * @return 如果可以添加返回 true
     *
     * 子类可重写以实现自定义验证逻辑：
     * - 容量限制
     * - 物品类型限制
     * - 等等
     */
    virtual bool CanAddItemEntirely(const FInv_RealItemData &ItemData) const;

    /**
     * Blueprint 可重写版本的添加物品验证
     *
     * @param ItemData 要添加的物品数据
     * @return 如果可以添加返回 true
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Inventory System")
    bool CanAddItemEntirely_BP(const FInv_RealItemData &ItemData) const;

    /**
     * 允许存放的物品类型标签（空表示允许所有类型）
     * 例如：["Item.Equipment", "Item.Consumable"]
     */
    UPROPERTY(EditAnywhere, Category = "Inventory System|UI")
    bool bUseItemTypeFilter{false};
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category ="Inventory System|UI")
    FGameplayTagContainer AllowedItemTypes;

    /**
     * 验证是否可以移除物品
     *
     * @param ItemId 要移除的物品 ID
     * @return 如果可以移除返回 true
     */
    virtual bool CanRemoveItem(const FGuid &ItemId) const;

    UFUNCTION(BlueprintNativeEvent, Category = "Inventory System")
    bool CanRemoveItem_BP(const FGuid &ItemId) const;

    // ========== 内部辅助方法 ==========

    /**
     * 绑定 FastArray 委托
     * 在 BeginPlay 时调用
     */
    virtual void BindFastArrayDelegates();

    /**
     * 解绑 FastArray 委托
     * 在 EndPlay 时调用
     */
    virtual void UnbindFastArrayDelegates();

    /**
     * 获取虚拟物品数据子系统
     * @return 子系统指针，未找到返回 nullptr
     */
    class UInv_VirtualItemDataSubsystem *GetVirtualItemDataSubsystem() const;

    // ========== 配置属性 ==========

    /**
     * 拾取距离限制（单位：厘米）
     * 防止玩家拾取过远的物品
     * 0 表示无限制
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System|UI",
        meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0"))
    float PickupRange{300.0f};

private:
    // ========== FastArray 委托句柄 ==========

    FDelegateHandle OnItemAddedHandle;
    FDelegateHandle OnItemChangedHandle;
    FDelegateHandle OnItemRemovedHandle;

    // ========== FastArray 数据 ==========

    /**
     * 物品列表（网络同步）
     * 存储该库存中的所有物品
     */
    UPROPERTY(Replicated)
    FInv_ItemList ItemList;

    // ========== 内部辅助方法 ==========

    /**
     * 检查物品类型是否允许添加
     */
    bool IsItemTypeAllowed(const FInv_RealItemData &ItemData) const;

    // ========= 关联UI窗口
    UPROPERTY(EditAnywhere, Category = "Inventory System|UI")
    TSubclassOf<UInv_InventoryWidgetBase> InventoryWidgetClass;

    TWeakObjectPtr<UInv_InventoryWidgetBase> InventoryWidgetInstance;

    UFUNCTION(Server,Reliable)
    void ServerOnItemDroppedFunc(FGuid SourceItemId, FGuid TargetItemId);

    UFUNCTION(Server,Reliable)
    void ServerOnItemSplitFunc(FGuid ItemId,int32 SplitCount);
};