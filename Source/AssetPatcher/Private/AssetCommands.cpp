// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetCommands.h"

#define LOCTEXT_NAMESPACE "FAssetPatcherModule"

FAssetCommands::FAssetCommands():TCommands<FAssetCommands>("PakExtendCommands",
	NSLOCTEXT("Extend", "Editor", "Commands"),
	NAME_None,
	FName("FAssetStyle"))
{
}

void FAssetCommands::RegisterCommands()
{
	UI_COMMAND(AssetPatcher, "AssetPatcher", "Patch DLC Asset!", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(CookAssets, "CookAssets", "Cook DLC Asset!", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(PakAssets, "PakAssets", "Pak DLC Asset!", EUserInterfaceActionType::Button, FInputChord());
}


#undef LOCTEXT_NAMESPACE
