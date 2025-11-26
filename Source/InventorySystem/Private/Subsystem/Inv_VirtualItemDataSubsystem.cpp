#include "Subsystem/Inv_VirtualItemDataSubsystem.h"

#include "InventorySystem.h"
#include "Engine/Engine.h"

void UInv_VirtualItemDataSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogInventorySystem, Log, TEXT("Inv_VirtualItemDataSubsystem: Initialized"));

    // 验证 DataTable 是否已设置
    if (!VirtualItemDataTable)
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT(
                "Inv_VirtualItemDataSubsystem: VirtualItemDataTable is not set! Please set it in the Blueprint child class."
            ));
        return;
    }

    // 验证 DataTable 的行结构是否正确
    if (VirtualItemDataTable->GetRowStruct() != FInv_VirtualItemData::StaticStruct())
    {
        UE_LOG(LogInventorySystem, Error,
            TEXT("Inv_VirtualItemDataSubsystem: DataTable row struct is not FInv_VirtualItemData! Got: %s"),
            *VirtualItemDataTable->GetRowStruct()->GetName());
        return;
    }

    // 构建映射
    BuildTagToRowNameMap();

    UE_LOG(LogInventorySystem, Log, TEXT("Inv_VirtualItemDataSubsystem: Initialized with %d items."),
        TagToRowNameMap.Num());
}

void UInv_VirtualItemDataSubsystem::Deinitialize()
{
    TagToRowNameMap.Empty();
    RowNameToTagMap.Empty();

    UE_LOG(LogInventorySystem, Log, TEXT("Inv_VirtualItemDataSubsystem: Deinitialized"));

    Super::Deinitialize();
}

const FInv_VirtualItemData *UInv_VirtualItemDataSubsystem::GetItemDataByTag(const FGameplayTag &ItemTag) const
{
    if (!VirtualItemDataTable)
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("Inv_VirtualItemDataSubsystem: DataTable is not set!"));
        return nullptr;
    }

    if (!ItemTag.IsValid())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("Inv_VirtualItemDataSubsystem: Invalid ItemTag"));
        return nullptr;
    }

    // 通过 Tag 查找 RowName
    const FName *RowNamePtr = TagToRowNameMap.Find(ItemTag);
    if (!RowNamePtr)
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("Inv_VirtualItemDataSubsystem: Item with tag '%s' not found"),
            *ItemTag.ToString());
        return nullptr;
    }

    // 从 DataTable 获取数据
    return VirtualItemDataTable->FindRow<FInv_VirtualItemData>(*RowNamePtr, TEXT("GetItemDataByTag"));
}

const FInv_VirtualItemData *UInv_VirtualItemDataSubsystem::GetItemDataByRowName(const FName &RowName) const
{
    if (!VirtualItemDataTable)
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("Inv_VirtualItemDataSubsystem: DataTable is not set!"));
        return nullptr;
    }

    if (RowName.IsNone())
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("Inv_VirtualItemDataSubsystem: Invalid RowName"));
        return nullptr;
    }

    return VirtualItemDataTable->FindRow<FInv_VirtualItemData>(RowName, TEXT("GetItemDataByRowName"));
}

FName UInv_VirtualItemDataSubsystem::GetRowNameByTag(const FGameplayTag &ItemTag) const
{
    if (!ItemTag.IsValid())
    {
        return NAME_None;
    }

    const FName *RowNamePtr = TagToRowNameMap.Find(ItemTag);
    return RowNamePtr ? *RowNamePtr : NAME_None;
}

FGameplayTag UInv_VirtualItemDataSubsystem::GetTagByRowName(const FName &RowName) const
{
    if (RowName.IsNone())
    {
        return FGameplayTag();
    }

    const FGameplayTag *TagPtr = RowNameToTagMap.Find(RowName);
    return TagPtr ? *TagPtr : FGameplayTag();
}

FDataTableRowHandle UInv_VirtualItemDataSubsystem::CreateRowHandleByTag(const FGameplayTag &ItemTag) const
{
    FDataTableRowHandle Handle;

    if (!VirtualItemDataTable)
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("Inv_VirtualItemDataSubsystem: Cannot create RowHandle, DataTable is not set!"));
        return Handle;
    }

    FName RowName = GetRowNameByTag(ItemTag);
    if (RowName.IsNone())
    {
        UE_LOG(LogInventorySystem, Warning,
            TEXT("Inv_VirtualItemDataSubsystem: Cannot create RowHandle, item with tag '%s' not found"),
            *ItemTag.ToString());
        return Handle;
    }

    Handle.DataTable = VirtualItemDataTable;
    Handle.RowName = RowName;

    return Handle;
}

bool UInv_VirtualItemDataSubsystem::HasItemWithTag(const FGameplayTag &ItemTag) const
{
    if (!ItemTag.IsValid())
    {
        return false;
    }

    return TagToRowNameMap.Contains(ItemTag);
}

TArray<FGameplayTag> UInv_VirtualItemDataSubsystem::GetAllItemTags() const
{
    TArray<FGameplayTag> Tags;
    TagToRowNameMap.GetKeys(Tags);
    return Tags;
}

void UInv_VirtualItemDataSubsystem::BuildTagToRowNameMap()
{
    TagToRowNameMap.Empty();
    RowNameToTagMap.Empty();

    if (!VirtualItemDataTable)
    {
        return;
    }

    // 获取 DataTable 的所有行名
    TArray<FName> RowNames = VirtualItemDataTable->GetRowNames();

    int32 RegisteredCount = 0;
    int32 SkippedCount = 0;

    for (const FName &RowName : RowNames)
    {
        // 获取行数据
        const FInv_VirtualItemData *ItemData = VirtualItemDataTable->FindRow<FInv_VirtualItemData>(
            RowName,
            TEXT("BuildTagToRowNameMap")
            );

        if (!ItemData)
        {
            UE_LOG(LogInventorySystem, Warning, TEXT("Inv_VirtualItemDataSubsystem: Failed to get data for row '%s'"),
                *RowName.ToString());
            SkippedCount++;
            continue;
        }

        // 检查 ItemTag 是否有效
        if (!ItemData->ItemTag.IsValid())
        {
            UE_LOG(LogInventorySystem, Warning, TEXT("Inv_VirtualItemDataSubsystem: Row '%s' has invalid ItemTag, skipping"),
                *RowName.ToString());
            SkippedCount++;
            continue;
        }

        // 检查是否有重复的 Tag
        if (TagToRowNameMap.Contains(ItemData->ItemTag))
        {
            UE_LOG(LogInventorySystem, Error,
                TEXT(
                    "Inv_VirtualItemDataSubsystem: Duplicate ItemTag '%s' found in rows '%s' and '%s'! Skipping second occurrence."
                ),
                *ItemData->ItemTag.ToString(),
                *TagToRowNameMap[ItemData->ItemTag].ToString(),
                *RowName.ToString());
            SkippedCount++;
            continue;
        }

        // 添加到映射表
        TagToRowNameMap.Add(ItemData->ItemTag, RowName);
        RowNameToTagMap.Add(RowName, ItemData->ItemTag);
        RegisteredCount++;
    }

    UE_LOG(LogInventorySystem, Log,
        TEXT("Inv_VirtualItemDataSubsystem: Built tag mapping. Registered: %d, Skipped: %d, Total Rows: %d"),
        RegisteredCount, SkippedCount, RowNames.Num());
}

UInv_VirtualItemDataSubsystem *UInv_VirtualItemDataSubsystem::Get(const UObject *WorldContext)
{
    if (!WorldContext)
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("Inv_VirtualItemDataSubsystem::Get: Invalid WorldContext"));
        return nullptr;
    }

    UWorld *World = WorldContext->GetWorld();
    if (!World)
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("Inv_VirtualItemDataSubsystem::Get: Failed to get World from WorldContext"));
        return nullptr;
    }

    UGameInstance *GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("Inv_VirtualItemDataSubsystem::Get: Failed to get GameInstance"));
        return nullptr;
    }

    return GameInstance->GetSubsystem<UInv_VirtualItemDataSubsystem>();
}