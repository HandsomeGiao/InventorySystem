#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/Data/Inv_RealItemData.h"
#include "Inv_ItemComponent.generated.h"

/**
 * 物品组件 - 标识一个Actor是可拾取的物品，标识了物品的基本行为和属性
 */
UCLASS(ClassGroup = (InventorySystem), meta = (BlueprintSpawnableComponent), Blueprintable)
class INVENTORYSYSTEM_API UInv_ItemComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // ========== 构造和初始化 ==========

    UInv_ItemComponent();
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

    // ========== UActorComponent 接口 ==========

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    virtual void BeginPlay() override;

    // ========== 物品数据访问 ==========

    const FInv_RealItemData &GetItemData() const { return RealItemData; }
    FInv_RealItemData &GetItemDataMutable() { return RealItemData; }
    
    void SetItemData(const FInv_RealItemData &NewItemData);
    
    const FInv_VirtualItemData *GetVirtualItemData() const;

    UFUNCTION(BlueprintPure, Category = "Inventory System|Item")
    FText GetItemDisplayName() const;

    UFUNCTION(BlueprintPure, Category = "Inventory System|Item")
    UTexture2D *GetItemIcon() const;

    // ========== 拾取逻辑 ==========

    UFUNCTION(BlueprintNativeEvent, Category = "Inventory System|Item")
    bool CanBePickedUp() const;

    /**
     * 当物品被拾取时调用（服务端）
     * @param Picker 拾取者
     */
    void OnPickedUp(AActor *Picker);

    /**
     * 标记物品已被拾取（服务端）
     * 这会阻止其他玩家拾取，并延迟销毁Actor
     */
    UFUNCTION(BlueprintCallable, Category = "Inventory System|Item")
    void MarkAsPickedUp();

    /**
     * 检查物品是否已被拾取
     */
    UFUNCTION(BlueprintPure, Category = "Inventory System|Item")
    bool IsPickedUp() const { return bIsPickedUp; }

    // ========== 生命周期配置 ==========

    /**
     * 是否在被拾取后自动销毁Actor
     * 默认为 true
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System|Lifecycle")
    bool bDestroyOnPickup{true};

    /**
     * 拾取后延迟销毁的时间（秒）
     * 用于播放拾取动画/音效
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System|Lifecycle",
        meta = (ClampMin = "0.0", EditCondition = "bDestroyOnPickup"))
    float DestroyDelay{0.1f};

    /**
     * 物品在世界中的存活时间（秒）
     * 0 表示永久存在
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System|Lifecycle",
        meta = (ClampMin = "0.0"))
    float LifeSpan{0.0f};

protected:
    // ========== 事件回调 ==========

    /**
     * 当物品数据被设置时调用
     * 可在此更新视觉效果（如网格体、材质等）
     * 该函数是装饰性的,不会在专用服务器上被调用!
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Inventory System|Item")
    void OnItemDataChanged();

    /**
     * 当物品即将被销毁时调用
     * 在客户端和服务器都会被调用
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Inventory System|Item")
    void OnItemDestroyed();

private:
    // ========== 核心数据 ==========

    /**
     * 物品实例数据
     * 包含物品类型、堆叠数量、实例属性等
     */
    UPROPERTY(ReplicatedUsing = OnRep_ItemData, EditAnywhere, BlueprintReadWrite, Category = "Inventory System|Item",
        meta = (AllowPrivateAccess = "true"), Replicated)
    FInv_RealItemData RealItemData;

    /**
     * 物品是否已被拾取（防止重复拾取）
     */
    UPROPERTY(Replicated)
    bool bIsPickedUp{false};

    // ========== 复制回调 ==========

    UFUNCTION()
    void OnRep_ItemData();

    // ========== 内部方法 ==========

    /**
     * 延迟销毁Actor
     */
    void DestroyAfterDelay();

    /**
     * 计时器句柄
     */
    FTimerHandle DestroyTimerHandle;
};