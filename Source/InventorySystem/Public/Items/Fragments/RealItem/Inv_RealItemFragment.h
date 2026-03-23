#pragma once

#include "CoreMinimal.h"
#include "Inv_RealItemFragment.generated.h"

// TODO:添加RealItemFragment

USTRUCT(BlueprintType)
struct INVENTORYSYSTEM_API FInv_RealItemFragment
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct INVENTORYSYSTEM_API FInv_TestValueFragment : public FInv_RealItemFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float TestValue{10.f};
};