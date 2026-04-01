// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Items/Fragments/RealItem/Inv_RealItemFragment.h"
#include "StructUtils/InstancedStruct.h"
#include "Inv_BpFuncLib.generated.h"

struct FInv_VirtualItemFragment;

/**
 * handful of Blueprint utility functions for Inventory System
 */
UCLASS()
class INVENTORYSYSTEM_API UInv_BpFuncLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Inventory System|Utils")
	static int32 GetRealItemFragmentByClass(const TArray<TInstancedStruct<FInv_RealItemFragment>>& Fragments,
	                                        const UScriptStruct* FragmentClass);

	UFUNCTION(BlueprintCallable, Category="Inventory System|Utils")
	static int32 GetVirtualFragmentByClass(const TArray<FInstancedStruct>& Fragments,
	                                       const UScriptStruct* FragmentClass);
};