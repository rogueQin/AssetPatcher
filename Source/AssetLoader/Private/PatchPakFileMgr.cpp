// Fill out your copyright notice in the Description page of Project Settings.

#include "PatchPakFileMgr.h"

#include "AssetDataType.h"
#include "AssetPatchConfig.h"
#include "IPlatformFilePak.h"
#include "PatchVersionMgr.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetStringLibrary.h"

#define LOCTEXT_NAMESPACE "FAssetLoaderModule"

UPatchPakFileMgr* UPatchPakFileMgr::Instance = nullptr;

UPatchPakFileMgr* UPatchPakFileMgr::Get()
{
	return Instance;
}

UPatchPakFileMgr* UPatchPakFileMgr::Create(UObject* Outer)
{
	if (nullptr == Instance)
	{
		Instance = NewObject<UPatchPakFileMgr>(Outer, StaticClass());
		Instance->AddToRoot();
		Instance->Init();
	}
	return Instance;
}

void UPatchPakFileMgr::Destory()
{
	if (nullptr != Instance)
	{
		Instance->SwitchToDefaultPlatformFile();
		Instance->RemoveFromRoot();
		Instance->MarkAsGarbage();
	}
	Instance = nullptr;
}

UPatchPakFileMgr::~UPatchPakFileMgr()
{
	UE_LOG(LogTemp,Display, TEXT("UPatchPakFileMgr::~UPatchPakFileMgr"));
}

void UPatchPakFileMgr::Init()
{
	DefaultPlatformFile = &FPlatformFileManager::Get().GetPlatformFile();
	
	PatchPlatformFile = MakeShareable(new FPakPlatformFile());
	PatchPlatformFile->Initialize(DefaultPlatformFile, TEXT("PatchPlatformFile"));
	FPlatformFileManager::Get().SetPlatformFile(*PatchPlatformFile.Get());
#if WITH_COREUOBJECT
	FPlatformFileManager::Get().InitializeNewAsyncIO();
#endif
	FPlatformFileManager::Get().SetPlatformFile(*DefaultPlatformFile);
}

bool UPatchPakFileMgr::LoadPackageRes(const FString& PakFilePath)
{
	FPlatformFileManager::Get().SetPlatformFile(*PatchPlatformFile.Get());
	FString PakLoadURL = FPaths::ProjectDir() + "DLC/" + PakFilePath;
	
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*PakLoadURL))
	{
		FPlatformFileManager::Get().SetPlatformFile(*DefaultPlatformFile);
		return false;
	}
	
	TRefCountPtr PakFile = new FPakFile(PatchPlatformFile.Get(), *PakLoadURL, false);
	FString MountPoint = PakFile->GetMountPoint();
	int32 Pos = MountPoint.Find("Content/");
	MountPoint = FPaths::ProjectDir() + MountPoint.RightChop(Pos);
	PakFile->SetMountPoint(*MountPoint);

	if (!PatchPlatformFile->Mount(*PakLoadURL, 4, *MountPoint))
	{
		FPlatformFileManager::Get().SetPlatformFile(*DefaultPlatformFile);
		return false;
	}

	TArray<FString> PakFileList;
	PakFile->FindPrunedFilesAtPath(PakFileList, *PakFile->GetMountPoint(), true, false, false);
	
	if (!PakFileList.IsEmpty())
	{
		for (FString& FileName : PakFileList)
		{
			if (!FileName.EndsWith(TEXT(".uasset")) || !FPaths::GetBaseFilename(FileName).StartsWith(TEXT("BP_")))
				continue;
		
			FString LoadFileName = FileName;
			Pos = LoadFileName.Find("Content/");
			LoadFileName = LoadFileName.RightChop(Pos);
			LoadFileName.ReplaceInline(TEXT("Content/"), TEXT("/Game/"));
			LoadFileName.ReplaceInline(TEXT("uasset"), *FPaths::GetBaseFilename(FileName));
			LoadFileName = LoadFileName + TEXT("_C'");
			LoadFileName = UKismetStringLibrary::Concat_StrStr(TEXT("/Script/Engine.Blueprint'"), LoadFileName);
			
			UClass* Class = LoadClass<ACharacter>(nullptr, *LoadFileName);
			if (nullptr != Class)
				GetWorld()->SpawnActor<ACharacter>(Class, FVector(10, 0, 100), FRotator::ZeroRotator);
		}
	}

	PatchPlatformFile->Unmount(*PakLoadURL);
	FPlatformFileManager::Get().SetPlatformFile(*DefaultPlatformFile);
	return true;
}

void UPatchPakFileMgr::SwitchToPatchPlatformFile()
{
	FPlatformFileManager::Get().SetPlatformFile(*PatchPlatformFile.Get());
}

bool UPatchPakFileMgr::MountPak(const FString& PakName)
{
	if (MountedPakList.Contains(PakName))
		return true;
	
	FString PakLoadURL = FPaths::ProjectDir() + AssetPatchConfig::PatchPath + PakName + AssetPatchConfig::PakSuffix;
	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, *PakLoadURL);
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*PakLoadURL))
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchPakFileMgr::MountPak File not Exisits!"));
		return false;
	}
	TRefCountPtr PakFile = new FPakFile(PatchPlatformFile.Get(), *PakLoadURL, false);
	FString MountPoint = PakFile->GetMountPoint();
	int32 Pos = MountPoint.Find("Content/");
	MountPoint = FPaths::ProjectDir() + MountPoint.RightChop(Pos);
	PakFile->SetMountPoint(*MountPoint);

	GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, *MountPoint);
	if (!PatchPlatformFile->Mount(*PakLoadURL, 4, *MountPoint))
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("UPatchPakFileMgr::MountPak Mount Error!"));
		return false;
	}
	MountedPakList.Add(PakName);
	return true;
}

void UPatchPakFileMgr::UnMountPak(const FString& PakName)
{
	if (!MountedPakList.Contains(PakName)) return;
	
	FString PakLoadURL = FPaths::ProjectDir() + AssetPatchConfig::PatchPath + PakName + AssetPatchConfig::PakSuffix;
	PatchPlatformFile->Unmount(*PakLoadURL);
	MountedPakList.Remove(PakName);
}

void UPatchPakFileMgr::ClearMount()
{
	for (FString PakName : MountedPakList)
	{
		FString PakLoadURL = FPaths::ProjectDir() + AssetPatchConfig::PatchPath + PakName + AssetPatchConfig::PakSuffix;
		PatchPlatformFile->Unmount(*PakLoadURL);
	}
	MountedPakList.Empty();
}

void UPatchPakFileMgr::SwitchToDefaultPlatformFile()
{
	FPlatformFileManager::Get().SetPlatformFile(*DefaultPlatformFile);
}

void UPatchPakFileMgr::RecodeVersionPak()
{
	TArray<FPatchPakBrief> PakList = UPatchVersionMgr::Get()->GetPakList();
	TSet<FString> NewVersionPak;
	for (FPatchPakBrief Pak : PakList)
	{
		NewVersionPak.Add(Pak.PakName);
	}
	SwitchToPatchPlatformFile();
	for (auto VersionedPak : VersionPak)
	{
		if (!NewVersionPak.Contains(VersionedPak))
		{
			UnMountPak(VersionedPak);
			FString UselessPakFile = FPaths::ProjectDir() + AssetPatchConfig::PatchPath + VersionedPak + AssetPatchConfig::PakSuffix;
			FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*UselessPakFile);
		}
	}
	SwitchToDefaultPlatformFile();
	VersionPak.Empty();
	VersionPak.Append(NewVersionPak);
}

#undef LOCTEXT_NAMESPACE
