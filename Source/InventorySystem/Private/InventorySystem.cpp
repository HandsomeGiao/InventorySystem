// Copyright Epic Games, Inc. All Rights Reserved.

#include "InventorySystem.h"

// 定义 InventorySystem 日志通道
DEFINE_LOG_CATEGORY(LogInventorySystem);

#define LOCTEXT_NAMESPACE "FInventorySystemModule"

void FInventorySystemModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FInventorySystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FInventorySystemModule, InventorySystem)