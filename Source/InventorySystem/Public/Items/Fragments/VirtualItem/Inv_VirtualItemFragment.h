#pragma once

#include "CoreMinimal.h"
#include "Inv_VirtualItemFragment.generated.h"

USTRUCT(NotBlueprintType)
struct INVENTORYSYSTEM_API FInv_VirtualItemFragment
{
	GENERATED_BODY()

	//只要基类析构函数为virtual，则派生类析构函数自动变为virtual
	virtual ~FInv_VirtualItemFragment() = default;
};

USTRUCT(BlueprintType)
struct INVENTORYSYSTEM_API FInv_GridFragment : public FInv_VirtualItemFragment
{
	GENERATED_BODY()

	FIntPoint GetGridSize() const { return GridSize; }
	void SetGridSize(const FIntPoint& Size) { GridSize = Size; }
	float GetGridPadding() const { return GridPadding; }
	void SetGridPadding(float Padding) { GridPadding = Padding; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "InventorySystem")
	FIntPoint GridSize{1, 1};

	UPROPERTY(EditDefaultsOnly, Category = "InventorySystem", meta = (ClampMin = "0.0"))
	float GridPadding{0.f};
};

USTRUCT(BlueprintType)
struct INVENTORYSYSTEM_API FInv_ImageFragment : public FInv_VirtualItemFragment
{
	GENERATED_BODY()

	UTexture2D* GetIcon() const { return Icon; }
	bool HasIcon() const { return Icon != nullptr; }
	FVector2D GetIconDimensions() const { return IconDimensions; }

private:
	UPROPERTY(EditDefaultsOnly, Category = "InventorySystem")
	TObjectPtr<UTexture2D> Icon{nullptr};

	UPROPERTY(EditDefaultsOnly, Category = "InventorySystem")
	FVector2D IconDimensions{44.f, 44.f};
};

USTRUCT(BlueprintType)
struct INVENTORYSYSTEM_API FInv_MaxStackFragment : public FInv_VirtualItemFragment
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category="InventorySystem", meta = (ClampMin = "1"))
	int32 MaxStackCount{1};
};

USTRUCT()
struct INVENTORYSYSTEM_API FInv_ActorFragment : public FInv_VirtualItemFragment
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "InventorySystem")
	TSoftClassPtr<AActor> ActorClass;
};

USTRUCT()
struct INVENTORYSYSTEM_API FInv_WeightFragment : public FInv_VirtualItemFragment
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "InventorySystem", meta = (ClampMin = "0.0"))
	float UnitWeight{0.0f};
};

USTRUCT()
struct INVENTORYSYSTEM_API FInv_DisplayFragment : public FInv_VirtualItemFragment
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "InventorySystem")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, Category = "InventorySystem")
	FText Description;
};