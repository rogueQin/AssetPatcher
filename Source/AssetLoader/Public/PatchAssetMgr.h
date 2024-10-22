// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PatchAssetMgr.generated.h"

/**
 * 
 */
UCLASS()
class ASSETLOADER_API UPatchAssetMgr : public UObject
{
	GENERATED_BODY()
public:
	static UPatchAssetMgr* Get();
	static UPatchAssetMgr* Create(UObject* Outer);
	void Init();
	static void Destory();
	// static 

private:
	static UPatchAssetMgr* Instance;

public:
	void GetPakByPackage(const FString& AssetPackage, TSet<FString>& OutPaks, TSet<FString>& OutAssets);
	void GetPakByPackages(const TArray<FString>& AssetsPackage, TSet<FString>& OutPaks, TSet<FString>& OutAssets);
	void GetPakByAsset(const FString& AssetName, TSet<FString>& OutPaks, TSet<FString>& OutAssets);
	void GetPakByAssets(const TArray<FString>& AssetsName, TSet<FString>& OutPaks, TSet<FString>& OutAssets);
};
