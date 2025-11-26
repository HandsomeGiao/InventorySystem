#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Items/Data/Inv_VirtualItemData.h"
#include "Inv_VirtualItemDataSubsystem.generated.h"

/**
 * 虚拟物品数据子系统
 *
 * 功能：
 * 1. 管理游戏中所有物品的定义数据（存储在 DataTable 中）
 * 2. 提供 GameplayTag 到 RowName 的快速映射
 * 3. 提供便捷的查询接口
 *
 * 使用方式：
 * - 创建此类的蓝图子类
 * - 在蓝图中设置 VirtualItemDataTable
 * - 通过 GameplayTag 查询物品定义
 *
 * 注意：
 * - 本类为 Abstract，需要在蓝图中创建子类使用
 * - DataTable 在初始化时设置，运行时不可修改
 * TODO:在使用了此类的派生类蓝图时,可能需要手动加载蓝图才能生效.
 */
UCLASS(Abstract)
class INVENTORYSYSTEM_API UInv_VirtualItemDataSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ========== Subsystem 生命周期 ==========

    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    // ========== 查询接口 ==========

    /**
     * 通过 GameplayTag 获取物品定义数据
     * @param ItemTag 物品的 GameplayTag
     * @return 物品定义数据指针，如果未找到返回 nullptr
     */
    const FInv_VirtualItemData *GetItemDataByTag(const FGameplayTag &ItemTag) const;

    /**
     * 通过 RowName 获取物品定义数据
     * @param RowName DataTable 中的行名
     * @return 物品定义数据指针，如果未找到返回 nullptr
     */
    const FInv_VirtualItemData *GetItemDataByRowName(const FName &RowName) const;

    /**
     * 通过 GameplayTag 获取对应的 RowName
     * @param ItemTag 物品的 GameplayTag
     * @return DataTable 中的行名，如果未找到返回 NAME_None
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System")
    FName GetRowNameByTag(const FGameplayTag &ItemTag) const;

    /**
     * 通过 RowName 获取对应的 GameplayTag
     * @param RowName DataTable 中的行名
     * @return 物品的 GameplayTag，如果未找到返回空 Tag
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System")
    FGameplayTag GetTagByRowName(const FName &RowName) const;

    /**
     * 创建物品实例的 DataTableRowHandle（通过 GameplayTag）
     * @param ItemTag 物品的 GameplayTag
     * @return DataTableRowHandle，可用于创建 FInv_RealItemData
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System")
    FDataTableRowHandle CreateRowHandleByTag(const FGameplayTag &ItemTag) const;

    /**
     * 检查是否存在指定 Tag 的物品定义
     * @param ItemTag 物品的 GameplayTag
     * @return 如果存在返回 true
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System")
    bool HasItemWithTag(const FGameplayTag &ItemTag) const;

    /**
     * 获取所有已注册的物品 Tag 列表
     * @return 所有物品的 GameplayTag 数组
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System")
    TArray<FGameplayTag> GetAllItemTags() const;

protected:
    /**
     * 从 DataTable 构建 GameplayTag 到 RowName 的映射
     * 在 Initialize 时自动调用
     */
    void BuildTagToRowNameMap();

    // ========== 数据成员 ==========

    /**
     * 物品定义数据表
     * 在蓝图子类中设置，初始化时自动构建映射
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory System")
    TObjectPtr<UDataTable> VirtualItemDataTable;

    /**
     * GameplayTag 到 RowName 的映射
     * 在 Initialize() 时自动构建，运行时只读
     */
    TMap<FGameplayTag, FName> TagToRowNameMap;

    /**
     * RowName 到 GameplayTag 的反向映射
     * 在 Initialize() 时自动构建，运行时只读
     */
    TMap<FName, FGameplayTag> RowNameToTagMap;

public:
    // ========== 静态辅助方法 ==========

    /**
     * 获取子系统实例（便捷访问）
     * @param WorldContext 世界上下文对象
     * @return 子系统实例指针，如果未找到返回 nullptr
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System", meta = (WorldContext = "WorldContext"))
    static UInv_VirtualItemDataSubsystem *Get(const UObject *WorldContext);
};