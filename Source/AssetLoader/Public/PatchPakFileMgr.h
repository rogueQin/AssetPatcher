// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PatchPakFileMgr.generated.h"

/**
 * 
 */
UCLASS()
class ASSETLOADER_API UPatchPakFileMgr : public UObject
{
	GENERATED_BODY()
public:
	static UPatchPakFileMgr* Get();
	static UPatchPakFileMgr* Create(UObject* Outer);
	void Init();
	static void Destory();
	~UPatchPakFileMgr();
	
private:
	TSharedPtr<class FPakPlatformFile> PatchPlatformFile;
	class IPlatformFile* DefaultPlatformFile;
	static UPatchPakFileMgr* Instance;

	TSet<FString> MountedPakList;
	TSet<FString> VersionPak;
public:
	bool LoadPackageRes(const FString& PakFilePath);

	void SwitchToPatchPlatformFile();
	bool MountPak(const FString& PakName);
	void UnMountPak(const FString& PakName);
	void ClearMount();
	void SwitchToDefaultPlatformFile();
	void RecodeVersionPak();
};
