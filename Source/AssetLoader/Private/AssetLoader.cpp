#include "AssetLoader.h"

#include "AssetDataType.h"
#include "AssetPatchConfig.h"
#include "PatchAssetMgr.h"
#include "PatchDownloader.h"
#include "PatchPakFileMgr.h"
#include "PatchVersionMgr.h"

#define LOCTEXT_NAMESPACE "FAssetLoaderModule"

FAssetLoaderModule::FAssetLoaderModule()
{
	UE_LOG(LogTemp, Display, TEXT("FAssetLoaderModule::FAssetLoaderModule"));
}

FAssetLoaderModule::~FAssetLoaderModule()
{
	UE_LOG(LogTemp, Display, TEXT("FAssetLoaderModule::~FAssetLoaderModule"));
}

void FAssetLoaderModule::StartupModule()
{
}

void FAssetLoaderModule::ShutdownModule()
{
}

FAssetLoaderModule& FAssetLoaderModule::Get()
{
	// FAssetLoaderModule Module = FModuleManager::GetModuleChecked<FAssetLoaderModule>(FName("AssetLoader"));

	return FModuleManager::LoadModuleChecked<FAssetLoaderModule>(FName("AssetLoader"));
}

void FAssetLoaderModule::InitModule(UObject* Outer)
{
	UPatchPakFileMgr::Create(Outer);
	UPatchVersionMgr::Create(Outer);
	UPatchDownloader::Create(Outer);
	UPatchAssetMgr::Create(Outer);
}

void FAssetLoaderModule::DestoryModule()
{
	UPatchPakFileMgr::Destory();
	UPatchVersionMgr::Destory();
	UPatchDownloader::Destory();
	UPatchAssetMgr::Destory();
}

void FAssetLoaderModule::VersionCheck()
{
	UPatchVersionMgr::Get()->GetServerVersion();
}

FVersionCheckDelegate& FAssetLoaderModule::OnVersionChecked()
{
	return UPatchVersionMgr::Get()->GetVersionDelegate();
}

void FAssetLoaderModule::GetPatchInfo(const TArray<FString>& AssetList, TMap<FString, int64>& OutList)
{
	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("FAssetLoaderModule::GetPatchInfo"));
	TSet<FString> PatchPakList;
	TSet<FString> PatchAssetList;
	UPatchAssetMgr::Get()->GetPakByAssets(AssetList, PatchPakList, PatchAssetList);
	
	TArray<FPatchPakBrief> BriefList;
	UPatchDownloader::Get()->GetPaksDownLoadInfo(PatchPakList.Array(), BriefList);
	
	for (FPatchPakBrief& BriefInfo : BriefList)
	{
		FString PakPath = FPaths::ProjectDir() + AssetPatchConfig::PatchPath + BriefInfo.PakName + AssetPatchConfig::PakSuffix;
		if (!FPaths::FileExists(PakPath))
		{
			GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, *PakPath);
			OutList.Add(BriefInfo.PakName, BriefInfo.PakSize);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Green, *PakPath);
		}
	}
}

void FAssetLoaderModule::DownloadPatch(const TArray<FString>& AssetList)
{
	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("FAssetLoaderModule::DownloadPatch"));
	TMap<FString, int64> DownloadPakPreview;
	GetPatchInfo(AssetList, DownloadPakPreview);
	for (auto PakPreview : DownloadPakPreview)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, *PakPreview.Key);
		UPatchDownloader::Get()->AddToQuene(PakPreview.Key);
		// UPatchDownloader::Get()->DownloadPak(PakPreview.Key);
	}
	UPatchDownloader::Get()->DownloadOneByOne();
}

FPatchDownloadDelegate& FAssetLoaderModule::OnDownLoadProcess()
{
	return UPatchDownloader::Get()->GetDownLoadDelegate();
}

void FAssetLoaderModule::MountPatchAsset(const FString& AssetName)
{
	TSet<FString> OutPaks;
	TSet<FString> PatchAssetList;
	UPatchAssetMgr::Get()->GetPakByAsset(AssetName, OutPaks, PatchAssetList);
	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("FAssetLoaderModule::MountPatchAsset"));
	for (FString PakName : OutPaks)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, *PakName);
		UPatchPakFileMgr::Get()->MountPak(PakName);
	}
}

// template< class T > 
// T* FAssetLoaderModule::PatchLoadObject(UObject* Outer, const FString& AssetName)
// {
// 	TSet<FString> OutPaks;
// 	UPatchAssetMgr::Get()->GetAssetPaks(AssetName, OutPaks);
//
// 	UPatchPakFileMgr::Get()->SwitchToPatchPlatformFile();
// 	for (FString PakName : OutPaks)
// 	{
// 		UPatchPakFileMgr::Get()->MountPak(PakName);
// 	}
// 	
// 	T* obj = LoadObject<T>(Outer, AssetName);
// 	UPatchPakFileMgr::Get()->SwitchToDefaultPlatformFile();
// 	
// 	return obj;
// }

// template <class T>
// UClass* FAssetLoaderModule::PatchLoadClass(UObject* Outer, const FString& AssetName)
// {
// 	TSet<FString> OutPaks;
// 	UPatchAssetMgr::Get()->GetAssetPaks(AssetName, OutPaks);
// 	UPatchPakFileMgr::Get()->SwitchToPatchPlatformFile();
// 	for (FString PakName : OutPaks)
// 	{
// 		UPatchPakFileMgr::Get()->MountPak(PakName);
// 	}
// 	
// 	FString ClassAsset = AssetName.Left(AssetName.Len() - 1) + "_C'";
// 	UClass* Class = LoadObject<T>(Outer, ClassAsset);
//
// 	UPatchPakFileMgr::Get()->SwitchToDefaultPlatformFile();
// 	
// 	return Class;
// }

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAssetLoaderModule, AssetLoader)
