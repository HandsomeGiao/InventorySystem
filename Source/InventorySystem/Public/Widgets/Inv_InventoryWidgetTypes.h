#pragma once

#include "CoreMinimal.h"
#include "Inv_InventoryWidgetTypes.generated.h"

class UInv_InventoryWidgetBase;

UENUM(BlueprintType)
enum class EInv_InventoryWidgetType : uint8
{
	None UMETA(Hidden),
	PlayerSelf UMETA(DisplayName = "Player Self"),
	PlayerOther UMETA(DisplayName = "Player Other"),
	Container UMETA(DisplayName = "Container"),
	Other UMETA(DisplayName = "Other")
};

UENUM(BlueprintType)
enum class EInv_InventoryOwnerType : uint8
{
	Player UMETA(DisplayName = "Player"),
	Container UMETA(DisplayName = "Container"),
	Other UMETA(DisplayName = "Other")
};

USTRUCT(BlueprintType)
struct INVENTORYSYSTEM_API FInv_InventoryWidgetConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System|UI")
	EInv_InventoryWidgetType WidgetType{EInv_InventoryWidgetType::PlayerSelf};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory System|UI")
	TSubclassOf<UInv_InventoryWidgetBase> WidgetClass;
};
