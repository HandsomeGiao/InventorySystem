// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/Inv_BpFuncLib.h"

#include "Items/Fragments/VirtualItem/Inv_VirtualItemFragment.h"

int32 UInv_BpFuncLib::GetRealItemFragmentByClass(
    const TArray<TInstancedStruct<FInv_RealItemFragment>> &Fragments,
    const UScriptStruct *FragmentClass)
{
    if (!FragmentClass)
        return INDEX_NONE;

    if (!FragmentClass->IsChildOf(FInv_RealItemFragment::StaticStruct()))
    {
        UE_LOG(LogTemp, Warning,
            TEXT(
                "UInv_BpFuncLib::GetRealItemFragmentByClass: Provided FragmentClass is not a child of FInv_BaseRealItemFragment"
            ));
        return INDEX_NONE;
    }

    for (int32 Index = 0; Index < Fragments.Num(); ++Index)
    {
        if (Fragments[Index].GetScriptStruct()->IsChildOf(FragmentClass))
            return Index;
    }

    return INDEX_NONE; // 循环结束，未找到 
}

int32 UInv_BpFuncLib::GetVirtualFragmentByClass(const TArray<TInstancedStruct<FInv_VirtualItemFragment>> &Fragments,
    const UScriptStruct *FragmentClass)
{
    if (!FragmentClass)
        return INDEX_NONE;

    if (!FragmentClass->IsChildOf(FInv_VirtualItemFragment::StaticStruct()))
    {
        UE_LOG(LogTemp, Warning,
            TEXT(
                "UInv_BpFuncLib::GetVirtualFragmentByClass: Provided FragmentClass is not a child of FInv_VirtualItemFragment"
            ));
        return INDEX_NONE;
    }

    for (int32 Index = 0; Index < Fragments.Num(); ++Index)
    {
        if (Fragments[Index].GetScriptStruct()->IsChildOf(FragmentClass))
            return Index;
    }
    return INDEX_NONE; // 循环结束，未找到
}