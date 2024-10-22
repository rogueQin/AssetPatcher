// Fill out your copyright notice in the Description page of Project Settings.

#include "PatchDownloader.h"

#include "AssetDataType.h"
#include "AssetPatchConfig.h"
#include "HttpModule.h"
#include "PatchVersionMgr.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "FAssetLoaderModule"

UPatchDownloader* UPatchDownloader::Instance = nullptr;

UPatchDownloader* UPatchDownloader::Get()
{
	return Instance;
}

UPatchDownloader* UPatchDownloader::Create(UObject* Outer)
{
	if (nullptr == Instance)
	{
		Instance = NewObject<UPatchDownloader>(Outer, UPatchDownloader::StaticClass());
		Instance->AddToRoot();
		Instance->Init();
	}
	return Instance;
}

void UPatchDownloader::Init()
{
}

void UPatchDownloader::Destory()
{
	if (nullptr != Instance)
	{
		Instance->RemoveFromRoot();
		Instance->MarkAsGarbage();
	}
	Instance = nullptr;
}

// UPatchDownloader::UPatchDownloader()
// {
// 	UE_LOG(LogTemp, Display, TEXT("UPatchDownloader::UPatchDownloader"));
// }

UPatchDownloader::~UPatchDownloader()
{
	UE_LOG(LogTemp, Display, TEXT("UPatchDownloader::~UPatchDownloader"))
}

// void UPatchDownloader::DownLoadFile(const FString& FileName)
// {
// 	FString SavePath = FPaths::ProjectDir() + AssetPatchConfig::PatchPath + FileName;
// 	FString DownLoadURL = AssetPatchConfig::ServerURL + FileName;
// 	RequestList.Add(DownLoadURL, SavePath);
// 	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
// 	// int64 NewFileOffset = FMath::Min<int64>(fileOffset + XRW_GDownloadPacketSize, FileTotalSize) - 1;
// 	// FString HeaderValue = FString::Printf(TEXT("bytes=%lld-%lld"), fileOffset, NewFileOffset);
// 	// HttpRequest->SetHeader("Range", HeaderValue);
// 	HttpRequest->SetVerb("GET");
// 	HttpRequest->SetURL(DownLoadURL);
// 	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPatchDownloader::OnFileDownLoadComplete);
// 	HttpRequest->OnRequestProgress().BindUObject(this, &UPatchDownloader::OnFileDownLoadProgress);
// 	HttpRequest->ProcessRequest();
// }

void UPatchDownloader::OnFileDownLoadProgress(FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
{
	const FHttpResponsePtr Response = Request->GetResponse();
	if (Response.IsValid())
	{
		// const int64 FullSize = Response->GetContentLength();
		// UE_LOG(LogTemp, Display, TEXT("AGameResourceState::OnFileDownLoadProgress"));
		FString FileName = RequestList.FindRef(Request->GetURL());
		DownLoadDelegate.ExecuteIfBound(EAssetLoaderCode::InProcess, FileName, BytesReceived);
	}
}

void UPatchDownloader::OnFileDownLoadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool Connected)
{
	Request->OnRequestProgress().Unbind();
	Request->OnProcessRequestComplete().Unbind();

	FString PakName = RequestList.FindRef(Request->GetURL());
	if (Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()) && Connected)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchDownloader::OnFileDownLoadComplete Success!"));
		FString SavePath = FPaths::ProjectDir() + AssetPatchConfig::PatchPath + PakName + AssetPatchConfig::PakSuffix;
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		FString Path, Extension;
		FPaths::Split(SavePath, Path, PakName, Extension);
		if (!PlatformFile.DirectoryExists(*Path) && !PlatformFile.CreateDirectoryTree(*Path))
		{
			GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchDownloader::OnFileDownLoadComplete FileFolderError!"));
			DownLoadDelegate.ExecuteIfBound(EAssetLoaderCode::FileFolderError, PakName, 0);
			return;	
		}
		
		IFileHandle* FileHandle = PlatformFile.OpenWrite(*SavePath);
		if (!FileHandle)
		{
			GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchDownloader::OnFileDownLoadComplete FileSaveError!"));
			DownLoadDelegate.ExecuteIfBound(EAssetLoaderCode::FileSaveError, PakName, 0);
			return;
		}

		FileHandle->Write(Response->GetContent().GetData(), Response->GetContentLength());
		delete FileHandle;

		RequestList.Remove(Request->GetURL());
		DownLoadDelegate.ExecuteIfBound(EAssetLoaderCode::FileComplete, PakName, Response->GetContentLength());
		DownloadOneByOne();
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchDownloader::OnFileDownLoadComplete Error!"));
		DownLoadDelegate.ExecuteIfBound(EAssetLoaderCode::FileRequestError, PakName, 0);
	}
}

void UPatchDownloader::GetPakDownLoadInfo(const FString& PakName, FPatchPakBrief& OutBrief)
{
	FString DownLoadURL = AssetPatchConfig::ServerURL + PakName + AssetPatchConfig::PakSuffix;
	const FPatchPakBrief* PakBrief = UPatchVersionMgr::Get()->GetPakList().FindByPredicate([PakName](FPatchPakBrief PakBrief)->bool
	{
		return PakBrief.PakName == PakName;
	});
	OutBrief.PakName = PakBrief->PakName;
	OutBrief.PakSize = PakBrief->PakSize;
}

void UPatchDownloader::GetPaksDownLoadInfo(const TArray<FString>& PakList, TArray<FPatchPakBrief>& OutBriefList)
{
	for (FString PakName : PakList)
	{
		FPatchPakBrief PakBrief;
		GetPakDownLoadInfo(PakName, PakBrief);
		OutBriefList.Add(PakBrief);
	}
}

FPatchDownloadDelegate& UPatchDownloader::GetDownLoadDelegate()
{
	return DownLoadDelegate;
}

void UPatchDownloader::DownloadPak(const FString& FileName)
{
	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchDownloader::DownloadPak"));
	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, *FileName);
	FString DownLoadURL = AssetPatchConfig::ServerURL + FileName + AssetPatchConfig::PakSuffix;
	RequestList.Add(DownLoadURL, FileName);
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	// int64 NewFileOffset = FMath::Min<int64>(fileOffset + XRW_GDownloadPacketSize, FileTotalSize) - 1;
	// FString HeaderValue = FString::Printf(TEXT("bytes=%lld-%lld"), fileOffset, NewFileOffset);
	// HttpRequest->SetHeader("Range", HeaderValue);
	HttpRequest->SetVerb("GET");
	HttpRequest->SetURL(DownLoadURL);
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UPatchDownloader::OnFileDownLoadComplete);
	HttpRequest->OnRequestProgress().BindUObject(this, &UPatchDownloader::OnFileDownLoadProgress);
	HttpRequest->ProcessRequest();
}

void UPatchDownloader::AddToQuene(const FString& FileName)
{
	CacheList.Enqueue(FileName);
}

void UPatchDownloader::DownloadOneByOne()
{
	// GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchDownloader::DownloadOneByOne"));
	if (RequestList.IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchDownloader::DownloadOneByOne RequestList is Empty!"));
		if (!CacheList.IsEmpty())
		{
			FString FileName;
			CacheList.Dequeue(FileName);
			DownloadPak(FileName);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchDownloader::DownloadOneByOne CacheList is Empty!"));
			DownLoadDelegate.ExecuteIfBound(EAssetLoaderCode::Complete, "", 0);
		}
	}
}

#undef LOCTEXT_NAMESPACE
