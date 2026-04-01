#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Widgets/Inv_InventoryWidgetTypes.h"
#include "Inv_InventoryWidgetOwnerSubsystem.generated.h"

class UInv_InventoryBase;

UCLASS()
class INVENTORYSYSTEM_API UInv_InventoryWidgetOwnerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UInv_InventoryBase* FindInventoryWidgetOwner(EInv_InventoryWidgetType WidgetType) const;
	void RegisterInventoryWidgetOwner(EInv_InventoryWidgetType WidgetType, UInv_InventoryBase* Inventory);
	void UnregisterInventoryWidgetOwner(EInv_InventoryWidgetType WidgetType, UInv_InventoryBase* Inventory);

private:
	TMap<EInv_InventoryWidgetType, TWeakObjectPtr<UInv_InventoryBase>> InventoryWidgetOwnersByType;
};
