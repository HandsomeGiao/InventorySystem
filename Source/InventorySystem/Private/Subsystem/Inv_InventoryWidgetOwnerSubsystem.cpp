#include "Subsystem/Inv_InventoryWidgetOwnerSubsystem.h"

#include "Inventory/Inv_InventoryBase.h"

void UInv_InventoryWidgetOwnerSubsystem::Deinitialize()
{
	InventoryWidgetOwnersByType.Empty();

	Super::Deinitialize();
}

UInv_InventoryBase* UInv_InventoryWidgetOwnerSubsystem::FindInventoryWidgetOwner(
	EInv_InventoryWidgetType WidgetType) const
{
	if (const TWeakObjectPtr<UInv_InventoryBase>* FoundInventory = InventoryWidgetOwnersByType.Find(WidgetType))
	{
		return FoundInventory->Get();
	}

	return nullptr;
}

void UInv_InventoryWidgetOwnerSubsystem::RegisterInventoryWidgetOwner(EInv_InventoryWidgetType WidgetType,
                                                                      UInv_InventoryBase* Inventory)
{
	if (WidgetType == EInv_InventoryWidgetType::None || !IsValid(Inventory))
	{
		return;
	}

	InventoryWidgetOwnersByType.FindOrAdd(WidgetType) = Inventory;
}

void UInv_InventoryWidgetOwnerSubsystem::UnregisterInventoryWidgetOwner(EInv_InventoryWidgetType WidgetType,
                                                                        UInv_InventoryBase* Inventory)
{
	if (WidgetType == EInv_InventoryWidgetType::None)
	{
		return;
	}

	const TWeakObjectPtr<UInv_InventoryBase>* FoundInventory = InventoryWidgetOwnersByType.Find(WidgetType);
	if (!FoundInventory)
	{
		return;
	}

	if (!IsValid(Inventory) || FoundInventory->Get() == Inventory)
	{
		InventoryWidgetOwnersByType.Remove(WidgetType);
	}
}
