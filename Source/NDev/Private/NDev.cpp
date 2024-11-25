// Copyright Epic Games, Inc. All Rights Reserved.

#include "NDev.h"

#define LOCTEXT_NAMESPACE "FNDevModule"

void FNDevModule::StartupModule()
{
#if  UE_GAME
	FName  StrName("TextureFormatDXT");
	IModuleInterface* TextureModule = FModuleManager::Get().GetModule(StrName);
	if (TextureModule == nullptr)
	{
		UE_LOG(RT2DefaultTextureLog, Warning, TEXT("TextureFormatDXT not loaded, starting load it"));
		FModuleManager::Get().LoadModule(StrName);
	}

	TextureModule = FModuleManager::Get().GetModule(StrName);
	if (TextureModule == nullptr)
	{
		UE_LOG(RT2DefaultTextureLog, Error, TEXT("TextureFormatDXT load Faild"));
	}
#endif
}

void FNDevModule::ShutdownModule()
{
#if  UE_GAME
	FName  StrName("TextureFormatDXT");
	if (!FModuleManager::Get().UnloadModule(StrName))
	{
		UE_LOG(RT2DefaultTextureLog, Error, TEXT("TextureFormatDXT unload Faild"));
	}
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNDevModule, NDev)