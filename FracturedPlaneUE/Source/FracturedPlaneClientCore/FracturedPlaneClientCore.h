// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FRACTUREDPLANECLIENTCORE_API UFracturedPlaneClientCore : public IModuleInterface
{
public:

	/**
	 * Called right after the module DLL has been loaded and the module object has been created
	 * Load dependent modules here, and they will be guaranteed to be available during ShutdownModule. ie:
	 *
	 * FModuleManager::Get().LoadModuleChecked(TEXT("HTTP"));
	 */
	virtual void StartupModule()
	{

	}

	/**
	 * Called after the module has been reloaded
	 */
	virtual void PostLoadCallback()
	{
	}

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 * During normal shutdown, this is called in reverse order that modules finish StartupModule().
	 * This means that, as long as a module references dependent modules in it's StartupModule(), it
	 * can safely reference those dependencies in ShutdownModule() as well.
	 */
	virtual void ShutdownModule()
	{
	}

	/**
	 * Returns true if this module hosts gameplay code
	 *
	 * @return True for "gameplay modules", or false for engine code modules, plugins, etc.
	 */
	virtual bool IsGameModule() const
	{
		return true;
	}
};

