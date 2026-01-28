// Copyright Stash. All Rights Reserved.
// Stash Pay Unreal Engine SDK - Module Interface

#pragma once

#include "Modules/ModuleManager.h"

/**
 * Stash Pay Module
 * 
 * Main module for the Stash Pay checkout integration plugin.
 * Handles platform initialization and provides support checking.
 */
class FMobileNativeCodeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Initialize platform-specific components */
	void Initialization();

	/** Check if the current platform supports Stash Pay */
	static bool IsSupported();
};
