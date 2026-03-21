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

	// ========== 物品数据访问 ==========

	const FInv_RealItemData& GetItemData() const { return RealItemData; }
	FInv_RealItemData& GetItemDataMutable() { return RealItemData; }
	const FInv_VirtualItemData* GetVirtualItemData() const;
	bool CanBePickedUp() const;

	// ========== 数据修改 ===========

	void SetItemData(const FInv_RealItemData& NewItemData);
	void OnPickedUp(AActor* Picker);
	void MarkAsPickedUp();

protected:
	// ========== override func =======

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	// ========== 配置属性 ==========

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System|Lifecycle")
	bool bDestroyOnPickup{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System|Lifecycle",
		meta = (ClampMin = "0.0", EditCondition = "bDestroyOnPickup"))
	float DestroyDelay{0.1f};

	UPROPERTY(ReplicatedUsing = OnRep_ItemData, EditAnywhere, BlueprintReadWrite, Category = "Inventory System|Item",
		meta = (AllowPrivateAccess = "true"), Replicated)
	FInv_RealItemData RealItemData;

	// ========== 事件回调 ==========

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemDataChangedCustomEvent);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemDestroyCustomEvent);

	FOnItemDataChangedCustomEvent OnItemDataChangedCustomEvent;
	FOnItemDestroyCustomEvent OnItemDestroyCustomEvent;

private:
	// ========== 私有数据 ==========

	UPROPERTY(Replicated)
	bool bIsPickedUp{false};

	FTimerHandle DestroyTimerHandle;

	// ========== 内部函数 ==========

	UFUNCTION()
	void OnRep_ItemData();
	void DestroyAfterDelay();
};