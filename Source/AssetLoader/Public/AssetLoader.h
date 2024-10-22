#pragma once

#include "CoreMinimal.h"
#include "PatchDownloader.h"
#include "PatchPakFileMgr.h"
#include "PatchVersionMgr.h"
#include "Engine/StreamableManager.h"
#include "Modules/ModuleManager.h"


class ASSETLOADER_API FAssetLoaderModule : public IModuleInterface
{
public:
    FAssetLoaderModule();
    ~FAssetLoaderModule();
    
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    static FAssetLoaderModule& Get();

    void InitModule(UObject* Outer);
    void DestoryModule();
    
    void VersionCheck();
    FVersionCheckDelegate& OnVersionChecked();

    void GetPatchInfo(const TArray<FString>& AssetList, TMap<FString, int64>& OutList);
    void DownloadPatch(const TArray<FString>& AssetList);
    FPatchDownloadDelegate& OnDownLoadProcess();

    void MountPatchAsset(const FString& AssetName);
    
    template< class T > 
    inline T* PatchLoadObject(UObject* Outer, const FString& AssetName)
    {
        UPatchPakFileMgr::Get()->SwitchToPatchPlatformFile();

        MountPatchAsset(AssetName);
        T* obj = LoadObject<T>(Outer, *AssetName);
        
        UPatchPakFileMgr::Get()->SwitchToDefaultPlatformFile();
        return obj;
    }

    template< class T > 
    inline UClass* PatchLoadClass(UObject* Outer, const FString& AssetName)
    {
        UPatchPakFileMgr::Get()->SwitchToPatchPlatformFile();
        
        MountPatchAsset(AssetName);
        FString ClassAsset = AssetName.Left(AssetName.Len() - 1) + "_C'";
        GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("AssetLoader::PatchLoadClass"));
        GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, ClassAsset);
        // FStreamableManager AssetLoader;
        // UClass* Class = AssetLoader.LoadSynchronous<UClass>(*ClassAsset);
        UClass* Class = LoadClass<T>(Outer, *ClassAsset);
        if (Class == nullptr)
        {
            GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, TEXT("Class Is null!"));
        }
        UPatchPakFileMgr::Get()->SwitchToDefaultPlatformFile();
        return Class;
    }

};
