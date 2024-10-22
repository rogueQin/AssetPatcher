// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetPatchCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
DEFINE_LOG_CATEGORY_STATIC(UAssetPatch, Log, All)


#define PATCH_PATH TEXT("-PatchPath=")

UAssetPatchCommandlet::UAssetPatchCommandlet()
{
	LogToConsole = true;
}

int32 UAssetPatchCommandlet::Main(const FString& Params)
{
	int32 result = Super::Main(Params);
	UE_LOG(UAssetPatch, Display, TEXT("UAssetPatchCommandlet::Main result:%d"), result);
	if (!FParse::Value(*Params, *FString(PATCH_PATH).ToLower(), PatchAssetPath))
	{
		UE_LOG(UAssetPatch, Error, TEXT("AssetPatch PatchPath is null!"));
		return -1;
	}
	UE_LOG(UAssetPatch, Display, TEXT("AssetPatch PatchPath:%s"), *PatchAssetPath);
	
	InitPatchContent();
	CreatePakInfo();
	
	return 0;
}

void UAssetPatchCommandlet::InitPatchContent()
{
	UE_LOG(UAssetPatch, Display, TEXT("UAssetPatchCommandlet::InitPatchContent"));
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString SavePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
	PatchPakPath = SavePath + "DLC/Pak/";

	if (!FPaths::DirectoryExists(PatchPakPath))
	{
		PlatformFile.CreateDirectoryTree(*PatchPakPath);
	}

	FString ReleasePath = PatchPakPath + "Release/";
	if (!FPaths::DirectoryExists(ReleasePath))
	{
		PlatformFile.CreateDirectoryTree(*ReleasePath);
	}
	
	FString ManifestFilePath = PatchPakPath + "Release/manifest.json";
	if (FPaths::FileExists(ManifestFilePath))
	{
		FString ManifestStr;
		FFileHelper::LoadFileToString(ManifestStr, *(ManifestFilePath));
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ManifestStr);
		FJsonSerializer::Deserialize(JsonReader, JsonObject);
		Ver = JsonObject->GetIntegerField("Ver");
		auto PakList = JsonObject->GetArrayField("PakList");
		for (auto Pak : PakList)
		{
			VersionedPak.Add(Pak->AsObject()->GetStringField("PakName"));
		}
	}
	Ver++;
	
	FString VersionDir = PatchPakPath + FString::FromInt(Ver) + "/Release/" + FString::FromInt(Ver) + "/";
	PlatformFile.CreateDirectoryTree(*VersionDir);
	
	Manifest = MakeShared<FJsonObject>();
	Manifest->SetNumberField("Ver", Ver);
	
	PatchCookPath = SavePath + "DLC/Cooked/" + FApp::GetProjectName() + "/Content/";
	PatchGamePath = "/Game/";
}

void UAssetPatchCommandlet::CreatePakInfo()
{
	UE_LOG(UAssetPatch, Display, TEXT("UAssetPatchCommandlet::CreatePakInfo"));
	FString CookedAssetPath = PatchCookPath + PatchAssetPath;
	PatchPakList.Empty();
	CreateInfoList(CookedAssetPath, PatchPakList);
	CreatePakConfigs(PatchPakList);
	CreatePaks(PatchPakList);
	RecodePakManifest(PatchPakList);
	RecodeDependenceManifest();
	CreateManifest();
}

void UAssetPatchCommandlet::CreateInfoList(const FString& ParentDir, TArray<FPatchPakInfo>& PakList)
{
	UE_LOG(UAssetPatch, Display, TEXT("UAssetPatchCommandlet::CreateInfoList ParentDir:%s"), *ParentDir);
	TArray<FString> DirectoryList;
	FPatchPakInfo PakInfo;
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*ParentDir, [&DirectoryList, &PakInfo](const TCHAR* FileOrDirName, bool IsDir)->bool {
		if (IsDir)
			DirectoryList.Add(FileOrDirName);
		else
			PakInfo.FileList.Add(FileOrDirName);
		
		return true;
	});
	if (!PakInfo.FileList.IsEmpty())
	{
		UE_LOG(UAssetPatch, Display, TEXT("PatchDir:%s"), *ParentDir);
		PakInfo.PatchPath = ParentDir;
		PakList.Add(PakInfo);
		for (auto FileName : PakInfo.FileList)
		{
			UE_LOG(UAssetPatch, Display, TEXT("PatchFile:%s"), *FileName);
		}
	}
	for (auto Dir : DirectoryList)
	{
		CreateInfoList(Dir, PakList);
	}
}

void UAssetPatchCommandlet::CreatePakConfigs(TArray<FPatchPakInfo>& PakList)
{
	UE_LOG(UAssetPatch, Display, TEXT("UAssetPatchCommandlet::CreatePakConfigs"));
	FString MountTag = FApp::GetProjectName();
	MountTag = MountTag + "/Content/";
	
	for (FPatchPakInfo& PakInfo : PakList)
	{
		PakInfo.HashName = FMD5::HashAnsiString(*PakInfo.PatchPath);
		PakInfo.PakConfigPath = PatchPakPath + FString::FromInt(Ver) + "/MetaDetail/" + PakInfo.HashName + ".txt";
		FString PakContent = "";
		for (FString FilePath : PakInfo.FileList)
		{
			int32 Pos = FilePath.Find(MountTag);
			FString MountPath = "../../../" + FilePath.RightChop(Pos);
			PakContent.Append("\"" + FilePath + "\" \"" + MountPath + "\"  -compress\n");
		}
		FFileHelper::SaveStringToFile(PakContent, *PakInfo.PakConfigPath);
	}
}

void UAssetPatchCommandlet::CreatePaks(TArray<FPatchPakInfo>& PakList)
{
	UE_LOG(UAssetPatch, Display, TEXT("UAssetPatchCommandlet::CreatePaks"));
#if PLATFORM_MAC
	FString UnrealPak = FPaths::ConvertRelativePathToFull(FPaths::EngineDir()) + TEXT("Binaries/Mac/UnrealPak");
#elif PLATFORM_WINDOWS
	FString UnrealPak = FPaths::ConvertRelativePathToFull(FPaths::EngineDir()) + TEXT("Binaries/Win64/UnrealPak.exe");
#endif
	
	

	for (FPatchPakInfo& PakInfo : PakList)
	{
		FString PakNameHashPath = PatchPakPath + PakInfo.HashName + ".pak";
		// -compressed
		FString Params = FString::Printf(TEXT("\"%s\" -create=\"%s\""), *PakNameHashPath, *PakInfo.PakConfigPath);
		FProcHandle UnrealPakProcHandle = FPlatformProcess::CreateProc(*UnrealPak, *Params, true, true, true, nullptr, 0, nullptr, nullptr);

		if(UnrealPakProcHandle.IsValid())
		{
			FPlatformProcess::WaitForProc(UnrealPakProcHandle);
		}
		int32 ProcReturnCode = -1;
		FPlatformProcess::GetProcReturnCode(UnrealPakProcHandle, &ProcReturnCode);
		UE_LOG(UAssetPatch, Display, TEXT("FPakModuleModule::CreatePaks : %s"), *PakInfo.HashName);
		
		PakInfo.PakSize = FPlatformFileManager::Get().GetPlatformFile().FileSize(*PakNameHashPath);
		PakInfo.HashFile = LexToString(FMD5Hash::HashFile(*PakNameHashPath));
		if (!VersionedPak.Contains(PakInfo.HashFile))
		{
			FString ArchivePakPath = PatchPakPath + FString::FromInt(Ver) + "/Release/" + PakInfo.HashFile + ".pak";
			FPlatformFileManager::Get().GetPlatformFile().CopyFile(*ArchivePakPath, *PakNameHashPath);
		}

		FString ReleasePakPath = PatchPakPath + "Release/" + PakInfo.HashFile + ".pak";
		if (!FPlatformFileManager::Get().GetPlatformFile().MoveFile(*ReleasePakPath, *PakNameHashPath))
		{
			FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*PakNameHashPath);
		}
	}
}

void UAssetPatchCommandlet::CreateManifest()
{
	TArray<TSharedPtr<FJsonValue>> PakListJson;
	for (FPatchPakInfo& PakInfo : PatchPakList)
	{
		TSharedPtr<FJsonObject> PakJsonObj = MakeShared<FJsonObject>();
		PakJsonObj->SetStringField("PakName", PakInfo.HashFile);
		PakJsonObj->SetNumberField("PakSize", PakInfo.PakSize);
		// PakInfo.HashFile
		// PakListJson.Add(MakeShared<FJsonValueString>(PakInfo.HashFile));
		PakListJson.Add(MakeShared<FJsonValueObject>(PakJsonObj));
	}
	Manifest->SetArrayField("PakList", PakListJson);

	TArray<TSharedPtr<FJsonValue>> AssetListJson;
	for (FPatchAssetInfo& PakInfo : PatchAssetList)
	{
		TSharedPtr<FJsonObject> AssetJsonObj = MakeShared<FJsonObject>();
		AssetJsonObj->SetStringField("AssetPackage", PakInfo.AssetPackage);
		AssetJsonObj->SetStringField("AssetName", PakInfo.AssetName);
		AssetJsonObj->SetStringField("AssetType", PakInfo.AssetType);
		AssetJsonObj->SetStringField("AssetPak", PakInfo.AssetPak);

		TArray<TSharedPtr<FJsonValue>> DependenciesJson;
		for (FName Dependency : PakInfo.Dependencies)
		{
			DependenciesJson.Add(MakeShared<FJsonValueString>(Dependency.ToString()));
		}
		AssetJsonObj->SetArrayField("Dependencies", DependenciesJson);
		
		AssetListJson.Add(MakeShared<FJsonValueObject>(AssetJsonObj));
	}
	Manifest->SetArrayField("AssetList", AssetListJson);

	FString ManifestJsonStr;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&ManifestJsonStr);
	FJsonSerializer::Serialize(Manifest.ToSharedRef(), JsonWriter);
	FString ManifestFilePath = PatchPakPath + "manifest.json";
	FFileHelper::SaveStringToFile(ManifestJsonStr, *ManifestFilePath);
	
	FString BackPath = PatchPakPath + FString::FromInt(Ver) + "/Release/" + FString::FromInt(Ver) + "/manifest.json";
	FPlatformFileManager::Get().GetPlatformFile().CopyFile(*BackPath, *ManifestFilePath);

	FString ReleasePath = PatchPakPath + "Release/" +  + "manifest.json";
	FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*ReleasePath);
	FPlatformFileManager::Get().GetPlatformFile().MoveFile(*ReleasePath, *ManifestFilePath);

	FString VersionStr = FString::FromInt(Ver);
	FString VersionPath = PatchPakPath + FString::FromInt(Ver) + "/ver";
	FFileHelper::SaveStringToFile(VersionStr, *VersionPath);
}

void UAssetPatchCommandlet::RecodePakManifest(TArray<FPatchPakInfo>& PakList)
{
	UE_LOG(UAssetPatch, Display, TEXT("UAssetPatchCommandlet::RecodePakManifest"));
	FString MountTag = FApp::GetProjectName();
	MountTag = MountTag + "/Content/";
	
	for (FPatchPakInfo& PakInfo : PakList)
	{
		for (FString FilePath : PakInfo.FileList)
		{
			FString AssetGamePath = FPaths::GetBaseFilename(FilePath, false);
			int32 Pos = AssetGamePath.Find(MountTag) + MountTag.Len();
			AssetGamePath = "/Game/" + AssetGamePath.RightChop(Pos);
			PakInfo.AssetList.Add(AssetGamePath);
		}

		for (FString AssetFile : PakInfo.AssetList)
		{
			Asset2Pak.Add(AssetFile, PakInfo.HashFile);
		}
	}
}

void UAssetPatchCommandlet::RecodeDependenceManifest()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	
	FString ScanPath = PatchGamePath + PatchAssetPath;;
	UE_LOG(LogTemp, Display, TEXT("UAssetPatchCommandlet::RecodeDependenceManifest Folder:%s"), *ScanPath);
	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByPath(FName(*ScanPath), AssetList, true);
	UE_LOG(LogTemp, Display, TEXT("UAssetPatchCommandlet::RecodeDependenceManifest AssetDataCount:%d"), AssetList.Num());
	for (FAssetData& AssetData : AssetList)
	{
		UE_LOG(LogTemp, Display, TEXT("UAssetPatchCommandlet::RecodeDependenceManifest : PackageNamer:%s"), *AssetData.PackageName.ToString());
		FPatchAssetInfo AssetInfo;
		AssetInfo.AssetPackage = AssetData.PackageName.ToString();
		AssetInfo.AssetName = AssetData.AssetName.ToString();
		AssetInfo.AssetType = AssetData.AssetClassPath.ToString();
		AssetInfo.AssetPak = Asset2Pak.FindRef(AssetData.PackageName.ToString());
		AssetRegistry.GetDependencies(AssetData.PackageName, AssetInfo.Dependencies);
		PatchAssetList.Add(AssetInfo);
	}
}
