// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetDataType.generated.h"

USTRUCT()
struct FPatchPakBrief
{
	GENERATED_BODY()

	FString PakName;
	int64 PakSize;
};

USTRUCT()
struct FPatchAssetInfo
{
	GENERATED_BODY()
	FString AssetPackage;
	FString AssetName;
	FString AssetType;
	FString AssetPak;
	TArray<FName> Dependencies;
};

USTRUCT()
struct FPatchPakInfo
{
	GENERATED_BODY()

	FString HashName;
	FString HashFile;
	FString PatchPath;
	FString PakConfigPath;
	int64 PakSize;
	TArray<FString> FileList;
	TSet<FString> AssetList;
};



