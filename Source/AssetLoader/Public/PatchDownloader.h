// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Interfaces/IHttpRequest.h"
#include "PatchDownloader.generated.h"

// ErrorCode, PakName, DownloadedSize
DECLARE_DELEGATE_ThreeParams(FPatchDownloadDelegate, int32, FString, int64)

struct FPatchPakBrief;
/**
 * 
 */
UCLASS()
class ASSETLOADER_API UPatchDownloader : public UObject
{
	GENERATED_BODY()
public:
	static UPatchDownloader* Get();
	static UPatchDownloader* Create(UObject* Outer);
	void Init();
	static void Destory();

	// UPatchDownloader();
	~UPatchDownloader();
	
	// UFUNCTION(BlueprintCallable)
	// void DownLoadFile(const FString& FileName);
	void OnFileDownLoadProgress(FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived);
	void OnFileDownLoadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool Connected);
	
private:
	static UPatchDownloader* Instance;
	const FString ResourcePath = FPaths::ProjectSavedDir();
	FString FileSavePath;

	TMap<FString, FString> RequestList;
	FPatchDownloadDelegate DownLoadDelegate;

	TQueue<FString> CacheList;
public:
	void GetPakDownLoadInfo(const FString& PakName, FPatchPakBrief& OutBrief);
	void GetPaksDownLoadInfo(const TArray<FString>& PakList, TArray<FPatchPakBrief>& OutBriefList);

	FPatchDownloadDelegate& GetDownLoadDelegate(); 
	void DownloadPak(const FString& FileName);
	
	void AddToQuene(const FString& FileName);
	void DownloadOneByOne();
};
