// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM()
enum EAssetLoaderCode
{
	Complete = 0,
	InProcess = 1,
	FileComplete = 2,
	VerionError = -1,
	FileRequestError = -2,
	FileFolderError = -3,
	FileSaveError = -4,
};

/**
 * 
 */
class ASSETLOADER_API AssetPatchConfig
{
public:
	AssetPatchConfig();
	~AssetPatchConfig();

public:
	inline static const FString ServerURL = "http://172.16.0.203:8080/";
	inline static const FString PatchPath = "DLC/";
	inline static const FString ManifestFileName = "manifest.json";
	inline static const FString VersionFileName = "ver";
	inline static const FString PakSuffix = ".pak";
};
