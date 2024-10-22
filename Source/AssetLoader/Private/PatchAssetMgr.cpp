// Fill out your copyright notice in the Description page of Project Settings.


#include "PatchAssetMgr.h"

#include "AssetDataType.h"
#include "PatchVersionMgr.h"

#define LOCTEXT_NAMESPACE "FAssetLoaderModule"

UPatchAssetMgr* UPatchAssetMgr::Instance = nullptr;

UPatchAssetMgr* UPatchAssetMgr::Get()
{
	return Instance;
}

UPatchAssetMgr* UPatchAssetMgr::Create(UObject* Outer)
{
	if (Instance == nullptr)
	{
		Instance = NewObject<UPatchAssetMgr>(Outer, UPatchAssetMgr::StaticClass());
		Instance->AddToRoot();
		Instance->Init();
	}
	return Instance;
}

void UPatchAssetMgr::Init()
{
}

void UPatchAssetMgr::Destory()
{
	if (nullptr != Instance)
	{
		Instance->RemoveFromRoot();
		Instance->MarkAsGarbage();
	}
	Instance = nullptr;
}

void UPatchAssetMgr::GetPakByPackage(const FString& AssetPackage, TSet<FString>& OutPaks, TSet<FString>& OutAssets)
{
	// GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("FAssetLoaderModule::GetAssetPaks"));
	// GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, *AssetPackage);
	const FPatchAssetInfo* AssetData = UPatchVersionMgr::Get()->GetAssetList().FindByPredicate([AssetPackage](FPatchAssetInfo& AssetInfo)->bool
	{
		return AssetInfo.AssetPackage == AssetPackage;
	});
	if (nullptr != AssetData && !OutAssets.Contains(AssetData->AssetPackage))
	{
		OutAssets.Add(AssetData->AssetPackage);
		OutPaks.Add(AssetData->AssetPak);
		AssetData->Dependencies;
		for (FName DependAsset : AssetData->Dependencies)
		{
			GetPakByPackage(DependAsset.ToString(), OutPaks, OutAssets);
		}
	}
}

void UPatchAssetMgr::GetPakByPackages(const TArray<FString>& AssetsPackage, TSet<FString>& OutPaks, TSet<FString>& OutAssets)
{
	for (FString AssetPackage : AssetsPackage)
	{
		GetPakByPackage(AssetPackage, OutPaks, OutAssets);
	}
}

void UPatchAssetMgr::GetPakByAsset(const FString& AssetName, TSet<FString>& OutPaks, TSet<FString>& OutAssets)
{
	const FPatchAssetInfo* AssetData = UPatchVersionMgr::Get()->GetAssetList().FindByPredicate([AssetName](FPatchAssetInfo& AssetInfo)->bool
	{
		return AssetName.Equals(AssetInfo.AssetType + "'" + AssetInfo.AssetPackage + "." + AssetInfo.AssetName + "'");
	});
	if (nullptr != AssetData && !OutAssets.Contains(AssetData->AssetPackage))
	{
		OutAssets.Add(AssetData->AssetPackage);
		OutPaks.Add(AssetData->AssetPak);
		AssetData->Dependencies;
		for (FName DependAsset : AssetData->Dependencies)
		{
			GetPakByPackage(DependAsset.ToString(), OutPaks, OutAssets);
		}
	}
}

void UPatchAssetMgr::GetPakByAssets(const TArray<FString>& AssetsName, TSet<FString>& OutPaks, TSet<FString>& OutAssets)
{
	for (FString AssetName : AssetsName)
	{
		GetPakByAsset(AssetName, OutPaks, OutAssets);
	}
}

#undef LOCTEXT_NAMESPACE
