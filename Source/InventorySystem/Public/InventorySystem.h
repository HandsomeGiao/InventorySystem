// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// 声明 InventorySystem 日志通道
DECLARE_LOG_CATEGORY_EXTERN(LogInventorySystem, Log, All);

class FInventorySystemModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
