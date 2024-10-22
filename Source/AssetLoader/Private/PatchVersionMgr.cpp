// Fill out your copyright notice in the Description page of Project Settings.


#include "PatchVersionMgr.h"

#include "AssetDataType.h"
#include "AssetPatchConfig.h"
#include "HttpModule.h"
#include "PatchDownloader.h"
#include "PatchPakFileMgr.h"
#include "Interfaces/IHttpResponse.h"


#define LOCTEXT_NAMESPACE "FAssetLoaderModule"

UPatchVersionMgr* UPatchVersionMgr::Instance = nullptr;

UPatchVersionMgr* UPatchVersionMgr::Get()
{
	return Instance;
}

UPatchVersionMgr* UPatchVersionMgr::Create(UObject* Outer)
{
	if (nullptr == Instance)
	{
		Instance = NewObject<UPatchVersionMgr>(Outer, UPatchVersionMgr::StaticClass());
		Instance->AddToRoot();
		Instance->Init();
	}
	return Instance;
}

void UPatchVersionMgr::Init()
{
	GetNetworkPermission();
	GetLocalVersion();
}

void UPatchVersionMgr::Destory()
{
	if (nullptr != Instance)
	{
		Instance->RemoveFromRoot();
		Instance->MarkAsGarbage();
	}
	Instance = nullptr;
}

void UPatchVersionMgr::GetNetworkPermission()
{
	FString DownLoadURL = AssetPatchConfig::ServerURL + AssetPatchConfig::VersionFileName;
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetURL(DownLoadURL);
	HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool Connected){});
	HttpRequest->ProcessRequest();
}

void UPatchVersionMgr::GetLocalVersion()
{
	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchVersionMgr::GetLocalVersion!"));
	
	PakList.Empty();
	AssetList.Empty();
	
	FString ManifestFilePath = FPaths::ProjectDir() + AssetPatchConfig::PatchPath + AssetPatchConfig::ManifestFileName;
	if (FPaths::FileExists(ManifestFilePath))
	{
		
		FString ManifestStr;
		FFileHelper::LoadFileToString(ManifestStr, *(ManifestFilePath));
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ManifestStr);
		FJsonSerializer::Deserialize(JsonReader, JsonObject);
		LocalVersion = JsonObject->GetIntegerField("Ver");
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchVersionMgr::ManifestFilePath PakList!"));
		TArray<TSharedPtr<FJsonValue>> PakListData = JsonObject->GetArrayField("PakList");
		for (TSharedPtr<FJsonValue> PakJson : PakListData)
		{
			TSharedPtr<FJsonObject> PakObject = PakJson->AsObject();
			FPatchPakBrief PakBrief;
			PakBrief.PakName = PakObject->GetStringField("PakName");
			PakBrief.PakSize = PakObject->GetNumberField("PakSize");
			PakList.Add(PakBrief);
			// GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, *PakBrief.PakName);
		}
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchVersionMgr::ManifestFilePath AssetList!"));
		TArray<TSharedPtr<FJsonValue>> AssetListData = JsonObject->GetArrayField("AssetList");
		for (TSharedPtr<FJsonValue> AssetJson : AssetListData)
		{
			FPatchAssetInfo AssetInfo;
			TSharedPtr<FJsonObject> AssetObject = AssetJson->AsObject();
			AssetInfo.AssetPackage = AssetObject->GetStringField("AssetPackage");
			AssetInfo.AssetName = AssetObject->GetStringField("AssetName");
			AssetInfo.AssetType = AssetObject->GetStringField("AssetType");
			AssetInfo.AssetPak = AssetObject->GetStringField("AssetPak");
			TArray<TSharedPtr<FJsonValue>> DependList = AssetObject->GetArrayField("Dependencies");
			for (auto DependJson : DependList)
			{
				AssetInfo.Dependencies.Add(FName(*DependJson->AsString()));
			}
			AssetList.Add(AssetInfo);
			// GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, *AssetInfo.AssetPackage);
		}
	}

	UPatchPakFileMgr::Get()->RecodeVersionPak();
}

void UPatchVersionMgr::GetServerVersion()
{
	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchVersionMgr::GetServerVersion"));

	FString DownLoadURL = AssetPatchConfig::ServerURL + AssetPatchConfig::VersionFileName;
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetURL(DownLoadURL);
	HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool Connected){
		Request->OnProcessRequestComplete().Unbind();
		if (Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()) && Connected)
		{
			FString VerJsonStr = Response->GetContentAsString();
			ServerVersion = FCString::Atoi(*VerJsonStr);
			if (LocalVersion != ServerVersion)
			{
				DownLoadManifest();
			}
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchVersionMgr::GetServerVersion Version Check Complete!"));
				GetVersionDelegate().ExecuteIfBound(EAssetLoaderCode::Complete);
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchVersionMgr::GetServerVersion Request Error!"));
			GetVersionDelegate().ExecuteIfBound(EAssetLoaderCode::VerionError);
		}
	});
	HttpRequest->ProcessRequest();
}

void UPatchVersionMgr::DownLoadManifest()
{
	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchVersionMgr::DownLoadManifest!"));
	FString DownLoadURL = AssetPatchConfig::ServerURL + FString::FromInt(ServerVersion) + "/" + AssetPatchConfig::ManifestFileName;
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetURL(DownLoadURL);
	HttpRequest->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool Connected){
		Request->OnProcessRequestComplete().Unbind();
		if (Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()) && Connected)
		{
			GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchVersionMgr::DownLoadManifest Request Complete!"));
			FString SavePath = FPaths::ProjectDir() + AssetPatchConfig::PatchPath + AssetPatchConfig::ManifestFileName;
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			FString Path, FileName, Extension;
			FPaths::Split(SavePath, Path, FileName, Extension);
			if (!PlatformFile.DirectoryExists(*Path) && !PlatformFile.CreateDirectoryTree(*Path))
			{
				GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchVersionMgr::DownLoadManifest Request FileFolderError!"));
				GetVersionDelegate().ExecuteIfBound(EAssetLoaderCode::FileFolderError);
				return;
			}

			PlatformFile.DeleteFile(*SavePath);
			IFileHandle* FileHandle = PlatformFile.OpenWrite(*SavePath);
			if (!FileHandle)
			{
				GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchVersionMgr::DownLoadManifest Request FileSaveError!"));
				GetVersionDelegate().ExecuteIfBound(EAssetLoaderCode::FileSaveError);
				return;
			}
			FileHandle->Write(Response->GetContent().GetData(), Response->GetContentLength());
			delete FileHandle;

			GetLocalVersion();
			GetVersionDelegate().ExecuteIfBound(EAssetLoaderCode::Complete);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchVersionMgr::DownLoadManifest Request Error!"));
			GetVersionDelegate().ExecuteIfBound(EAssetLoaderCode::FileRequestError);
		}
	});
	HttpRequest->ProcessRequest();
}

FVersionCheckDelegate& UPatchVersionMgr::GetVersionDelegate()
{
	return VersionResult;
}

const TArray<FPatchPakBrief>& UPatchVersionMgr::GetPakList()
{
	return PakList;
}

const TArray<FPatchAssetInfo>& UPatchVersionMgr::GetAssetList()
{
	return AssetList;
}

#undef LOCTEXT_NAMESPACE
