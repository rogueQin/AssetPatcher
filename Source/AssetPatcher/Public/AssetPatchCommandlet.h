// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "AssetDataType.h"
#include "AssetPatchCommandlet.generated.h"

/**
 * 
 */
UCLASS()
class ASSETPATCHER_API UAssetPatchCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UAssetPatchCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	void InitPatchContent();
	void CreatePakInfo();
	void CreateInfoList(const FString& ParentDir, TArray<FPatchPakInfo>& PakList);
	void CreatePakConfigs(TArray<FPatchPakInfo>& PakList);
	void CreatePaks(TArray<FPatchPakInfo>& PakList);
	void CreateManifest();
	void RecodePakManifest(TArray<FPatchPakInfo>& PakList);
	void RecodeDependenceManifest();

private:
	FString PatchGamePath;
	FString PatchAssetPath;
	FString PatchCookPath;
	FString PatchPakPath;
	TArray<FPatchPakInfo> PatchPakList;
	TArray<FPatchAssetInfo> PatchAssetList;
	TMap<FString, FString> Asset2Pak;
	TSharedPtr<FJsonObject> Manifest;
	
	int32 Ver = 0;
	TSet<FString> VersionedPak;
};
