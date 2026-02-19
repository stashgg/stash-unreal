// Copyright Stash. All Rights Reserved.
// Stash Unreal Engine SDK - Module Interface

#pragma once

#include "Modules/ModuleManager.h"

// Custom log category for Stash plugin
DECLARE_LOG_CATEGORY_EXTERN(LogStash, Log, All);

/**
 * Stash Module
 *
 * Main module for the Stash Native integration plugin (card, modal, browser).
 * Handles platform initialization and provides support checking.
 */
class FStashModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Initialize platform-specific components */
	void Initialization();

	/** Check if the current platform supports Stash Native */
	static bool IsSupported();
};
