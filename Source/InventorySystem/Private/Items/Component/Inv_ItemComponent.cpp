#include "Items/Component/Inv_ItemComponent.h"
#include "Items/Data/Inv_VirtualItemData.h"
#include "Items/Fragments/VirtualItem/Inv_VirtualItemFragment.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "InventorySystem.h"

UInv_ItemComponent::UInv_ItemComponent()
{
    // 启用网络复制
    SetIsReplicatedByDefault(true);

    // 默认配置
    bDestroyOnPickup = true;
    DestroyDelay = 0.1f;
    LifeSpan = 0.0f;
    bIsPickedUp = false;

    // 默认不需要 Tick
    PrimaryComponentTick.bCanEverTick = false;
}

void UInv_ItemComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Super::OnComponentDestroyed(bDestroyingHierarchy);

    // 被销毁时,如果DestroyTimerHandle还没有被激活,也就是说该Actor是因为另外的原因销毁的,那么此时应该手动清空该定时器
    if (GetWorld() && DestroyTimerHandle.IsValid())
        GetWorld()->GetTimerManager().ClearTimer(DestroyTimerHandle);
}

void UInv_ItemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 复制物品数据和拾取状态
    DOREPLIFETIME(UInv_ItemComponent, RealItemData);
    DOREPLIFETIME(UInv_ItemComponent, bIsPickedUp);
}

void UInv_ItemComponent::BeginPlay()
{
    Super::BeginPlay();

    // 在服务端为每个实例重新生成唯一的 RealItemId
    // 这是必要的，因为从蓝图实例化时，RealItemData 会直接复制 CDO 的值
    // 而不会重新调用结构体的构造函数
    if (GetOwner()->HasAuthority())
    {
        RealItemData.RealItemId = FGuid::NewGuid();
        UE_LOG(LogInventorySystem, Verbose,
            TEXT("UInv_ItemComponent::BeginPlay: Generated new RealItemId: %s for actor '%s'"),
            *RealItemData.RealItemId.ToString(), *GetOwner()->GetName());
    }

    // 如果设置了生命周期，启动销毁计时器
    if (LifeSpan > 0.0f && GetOwner()->HasAuthority())
    {
        FTimerHandle LifeSpanTimerHandle;
        GetWorld()->GetTimerManager().SetTimer(
            LifeSpanTimerHandle,
            [this]() {
                if (GetOwner() && !bIsPickedUp)
                {
                    UE_LOG(LogInventorySystem, Log,
                        TEXT("UInv_ItemComponent: Item lifespan expired, destroying actor '%s'"),
                        *GetOwner()->GetName());
                    GetOwner()->Destroy();
                }
            },
            LifeSpan,
            false
            );
    }

    UE_LOG(LogInventorySystem, Verbose,
        TEXT("UInv_ItemComponent::BeginPlay: Item component initialized on %s"),
        GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));
}

// ========== 物品数据访问 ==========

void UInv_ItemComponent::SetItemData(const FInv_RealItemData &NewItemData)
{
    // 仅在服务端执行
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_ItemComponent::SetItemData: Can only set item data on server! Owner: %s"),
            *GetOwner()->GetName());
        return;
    }

    RealItemData = NewItemData;

    // 触发数据变化事件
    OnItemDataChanged();

    UE_LOG(LogInventorySystem, Log, TEXT("UInv_ItemComponent::SetItemData: Item data set (ItemId: %s, StackCount: %d)"),
        *RealItemData.RealItemId.ToString(), RealItemData.StackCount);
}

const FInv_VirtualItemData *UInv_ItemComponent::GetVirtualItemData() const { return RealItemData.GetVirtualItemData(); }

FText UInv_ItemComponent::GetItemDisplayName() const
{
    const FInv_VirtualItemData *VirtualData = GetVirtualItemData();
    if (!VirtualData)
    {
        return FText::FromString(TEXT("Unknown Item"));
    }

    // 从Fragment中获取显示名称
    if (const FInv_DisplayFragment *DisplayFragment = VirtualData->GetFragmentOfType<FInv_DisplayFragment>())
    {
        return DisplayFragment->DisplayName;
    }

    // 回退：使用Tag作为名称
    return FText::FromString(VirtualData->ItemTag.ToString());
}

UTexture2D *UInv_ItemComponent::GetItemIcon() const
{
    const FInv_VirtualItemData *VirtualData = GetVirtualItemData();
    if (!VirtualData)
    {
        return nullptr;
    }

    // 从Fragment中获取图标
    if (const FInv_ImageFragment *ImageFragment = VirtualData->GetFragmentOfType<FInv_ImageFragment>())
    {
        return ImageFragment->GetIcon();
    }

    return nullptr;
}

// ========== 拾取逻辑 ==========

bool UInv_ItemComponent::CanBePickedUp_Implementation() const
{
    // 基本条件：未被拾取、物品数据有效
    if (bIsPickedUp)
    {
        return false;
    }

    if (!RealItemData.RealItemId.IsValid() && RealItemData.StackCount <= 0)
    {
        return false;
    }

    if (!GetVirtualItemData())
    {
        return false;
    }

    return true;
}

void UInv_ItemComponent::OnPickedUp(AActor *Picker)
{
    // 仅在服务端执行
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_ItemComponent::OnPickedUp: Can only execute on server!"));
        return;
    }

    // 验证拾取者
    if (!Picker)
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_ItemComponent::OnPickedUp: Invalid picker!"));
        return;
    }

    UE_LOG(LogInventorySystem, Display,
        TEXT("UInv_ItemComponent::OnPickedUp:'%s' was picked up by '%s' (remaining: %d)"),
        *GetOwner()->GetName(), *Picker->GetName(), RealItemData.StackCount);

    // 如果全部被拾取
    if (0 >= RealItemData.StackCount)
    {
        // 标记为已拾取
        MarkAsPickedUp();

        // 如果需要销毁
        if (bDestroyOnPickup)
        {
            DestroyAfterDelay();
        }
    }
    else
    {
        UE_LOG(LogInventorySystem, Display, TEXT("UInv_ItemComponent::OnPickedUp: Partial pickup, %d items remaining"),
            RealItemData.StackCount);
    }
    // 触发数据变化事件
    if (GetOwner()->GetNetMode() == NM_ListenServer || GetOwner()->GetNetMode() == NM_Standalone)
        OnItemDataChanged();
}

void UInv_ItemComponent::MarkAsPickedUp()
{
    // 仅在服务端执行
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("UInv_ItemComponent::MarkAsPickedUp: Can only execute on server!"));
        return;
    }

    bIsPickedUp = true;

    UE_LOG(LogInventorySystem, Verbose,
        TEXT("UInv_ItemComponent::MarkAsPickedUp: Item marked as picked up"));
}

// ========== 事件回调 ==========

void UInv_ItemComponent::OnItemDataChanged_Implementation()
{
    // 基类默认实现：仅记录日志
    UE_LOG(LogInventorySystem, Verbose,
        TEXT("UInv_ItemComponent::OnItemDataChanged: Item data changed (Role: %s)"),
        GetOwner()->HasAuthority() ? TEXT("Server") : TEXT("Client"));

    // 子类或蓝图可重写此方法来更新视觉效果
    // 例如：更新网格体、材质、粒子效果等
}

void UInv_ItemComponent::OnItemDestroyed_Implementation()
{
    // 基类默认实现：仅记录日志
    UE_LOG(LogInventorySystem, Log,
        TEXT("UInv_ItemComponent::OnItemDestroyed: Item about to be destroyed"));

    // 子类或蓝图可重写此方法
    // 例如：播放销毁音效、粒子效果等
}

// ========== 复制回调 ==========

void UInv_ItemComponent::OnRep_ItemData()
{
    // 客户端收到物品数据更新
    OnItemDataChanged();

    UE_LOG(LogInventorySystem, Verbose,
        TEXT("UInv_ItemComponent::OnRep_ItemData: Item data replicated to client"));
}

// ========== 内部方法 ==========

void UInv_ItemComponent::DestroyAfterDelay()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    // 触发销毁事件
    OnItemDestroyed();

    if (DestroyDelay > 0.0f)
    {
        // 延迟销毁
        GetWorld()->GetTimerManager().SetTimer(
            DestroyTimerHandle,
            [this]() {
                if (GetOwner())
                {
                    UE_LOG(LogInventorySystem, Verbose,
                        TEXT("UInv_ItemComponent::DestroyAfterDelay: Destroying actor '%s'"),
                        *GetOwner()->GetName());
                    GetOwner()->Destroy();
                }
            },
            DestroyDelay,
            false
            );
    }
    else
    {
        // 立即销毁
        UE_LOG(LogInventorySystem, Verbose,
            TEXT("UInv_ItemComponent::DestroyAfterDelay: Destroying actor '%s' immediately"),
            *GetOwner()->GetName());
        GetOwner()->Destroy();
    }
}