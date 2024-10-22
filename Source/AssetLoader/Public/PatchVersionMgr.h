// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PatchVersionMgr.generated.h"

struct FPatchAssetInfo;
struct FPatchPakBrief;
DECLARE_DELEGATE_OneParam(FVersionCheckDelegate, int32)

/**
 * 
 */
UCLASS()
class ASSETLOADER_API UPatchVersionMgr : public UObject
{
	GENERATED_BODY()
public:
	static UPatchVersionMgr* Get();
	static UPatchVersionMgr* Create(UObject* Outer);
	void Init();
	static void Destory();

	// virtual void BeginDestroy() override;
	void GetNetworkPermission();
	void GetLocalVersion();
	void GetServerVersion();
	void DownLoadManifest();
	FVersionCheckDelegate& GetVersionDelegate();
private:
	static UPatchVersionMgr* Instance;
	
	int32 LocalVersion = 0;
	int32 ServerVersion = 0;

	FVersionCheckDelegate VersionResult;

	TArray<FPatchPakBrief> PakList;
	TArray<FPatchAssetInfo> AssetList;
	// TSet<FString> VersionPak;
public:
	const TArray<FPatchPakBrief>& GetPakList();
	const TArray<FPatchAssetInfo>& GetAssetList();
};
