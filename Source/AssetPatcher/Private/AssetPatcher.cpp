// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetPatcher.h"

#include "AssetCommands.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ToolMenus.h"
// #include "AssetRegistry/AssetRegistryModule.h"
// #include "Engine/AssetManager.h"
// #include "Engine/AssetManagerSettings.h"
// #include "Settings/ProjectPackagingSettings.h"

#define LOCTEXT_NAMESPACE "FAssetPatcherModule"

class FAssetRegistryModule;

void FAssetPatcherModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	IModuleInterface::StartupModule();

	FAssetCommands::Register();
	PluginCommands = MakeShared<FUICommandList>();
	PluginCommands->MapAction(FAssetCommands::Get().AssetPatcher, FExecuteAction::CreateRaw(this, &FAssetPatcherModule::CreateAssets), FCanExecuteAction());
	PluginCommands->MapAction(FAssetCommands::Get().CookAssets, FExecuteAction::CreateRaw(this, &FAssetPatcherModule::CookAssets), FCanExecuteAction());
	PluginCommands->MapAction(FAssetCommands::Get().PakAssets, FExecuteAction::CreateRaw(this, &FAssetPatcherModule::PatchAssets), FCanExecuteAction());
	
	CreateExtendMenu();
}

void FAssetPatcherModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	PluginCommands->UnmapAction(FAssetCommands::Get().AssetPatcher);
	FAssetCommands::Unregister();
	
	IModuleInterface::ShutdownModule();
}

void FAssetPatcherModule::CreateExtendMenu()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.FolderContextMenu");
	FToolMenuSection* Section = Menu->FindSection("PathContextBulkOperations");
	if (Section)
	{
		FToolMenuEntry CookEntry = FToolMenuEntry::InitMenuEntry(FAssetCommands::Get().CookAssets, FText::FromString("Cook Asset"), FText::FromString("Cook assets!"));
		CookEntry.Name = FName("CookAsset");
		CookEntry.SetCommandList(PluginCommands);
		Section->AddEntry(CookEntry);
		
		FToolMenuEntry PakEntry = FToolMenuEntry::InitMenuEntry(FAssetCommands::Get().PakAssets, FText::FromString("Pak Asset"), FText::FromString("creat paks!"));
		PakEntry.Name = FName("PakAsset");
		PakEntry.SetCommandList(PluginCommands);
		Section->AddEntry(PakEntry);
		
		FToolMenuEntry PatchEntry = FToolMenuEntry::InitMenuEntry(FAssetCommands::Get().AssetPatcher, FText::FromString("Patch Asset"), FText::FromString("Cook assets and creat pak!"));
		PatchEntry.Name = FName("PatchAsset");
		PatchEntry.SetCommandList(PluginCommands);
		Section->AddEntry(PatchEntry);
	}
}

void FAssetPatcherModule::CreateAssets()
{
	CookAssets();
	PatchAssets();
}

void FAssetPatcherModule::CookAssets()
{
#if PLATFORM_MAC
	FString UnrealExe = FPaths::ConvertRelativePathToFull(FPaths::EngineDir()) + TEXT("Binaries/Mac/UnrealEditor");
	FString Platform = "IOS";
#elif PLATFORM_WINDOWS
	FString UnrealExe = FPaths::ConvertRelativePathToFull(FPaths::EngineDir()) + TEXT("Binaries/Win64/UnrealEditor-Cmd.exe");
	FString Platform = "Windows";
#endif

	FString ProjectFilePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), FApp::GetProjectName())) + ".uproject";
	FString DLCCookedDir = FPaths::ProjectSavedDir() + "DLC/Cooked/";
	FString DLCPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()) + "DLC";


	// UAssetManager::Get().GetSettings().DirectoriesToExclude
	// auto PackaSetting = GetDefault<UProjectPackagingSettings>();
	// PackaSetting->DirectoriesToNeverCook
	
	// -COOKDIR=
	// -DLCNAME=%s
	// -NoDefaultMaps
	FString Params = FString::Printf(TEXT("%s -run=Cook -TargetPlatform=%s -OutputDir=%s -COOKDIR=%s -NoDefaultMaps -unattended -UTF8Output"), *ProjectFilePath, *Platform, *DLCCookedDir, *DLCPath);
	FProcHandle UnrealEditorCmdProcHandle = FPlatformProcess::CreateProc(*UnrealExe, *Params, true, true, true, nullptr, 0, nullptr, nullptr);
	UE_LOG(LogTemp, Display, TEXT("FAssetPatcherModule::CookAssets : Create UnrealEditorCmdProcHandle!!"));
	if(UnrealEditorCmdProcHandle.IsValid())
	{
		UE_LOG(LogTemp, Display, TEXT("FAssetPatcherModule::CookAssets : UnrealEditorCmdProcHandle.IsValid!!"));
		FPlatformProcess::WaitForProc(UnrealEditorCmdProcHandle);
	}
	int32 ProcReturnCode = -1;
	FPlatformProcess::GetProcReturnCode(UnrealEditorCmdProcHandle, &ProcReturnCode);

	UE_LOG(LogTemp, Display, TEXT("FAssetPatcherModule::CookAssets : End!!"));
}

void FAssetPatcherModule::PatchAssets()
{
#if PLATFORM_MAC
	FString UnrealExe = FPaths::ConvertRelativePathToFull(FPaths::EngineDir()) + TEXT("Binaries/Mac/UnrealEditor");
#elif PLATFORM_WINDOWS
	FString UnrealExe = FPaths::ConvertRelativePathToFull(FPaths::EngineDir()) + TEXT("Binaries/Win64/UnrealEditor-Cmd.exe");
#endif
	
	FString ProjectFilePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), FApp::GetProjectName())) + ".uproject";
	
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FString> Folders;
	ContentBrowserModule.Get().GetSelectedFolders(Folders);
	for (FString Folder : Folders)
	{
		int32 pos = Folder.Find("/Game/");
		FString PatchPath = Folder.RightChop(pos + 6);
		FString Params = FString::Printf(TEXT("%s -run=AssetPatch -PatchPath=%s"), *ProjectFilePath, *PatchPath);
		FProcHandle UnrealEditorCmdProcHandle = FPlatformProcess::CreateProc(*UnrealExe, *Params, true, true, true, nullptr, 0, nullptr, nullptr);
		if(UnrealEditorCmdProcHandle.IsValid())
		{
			FPlatformProcess::WaitForProc(UnrealEditorCmdProcHandle);
		}
		int32 ProcReturnCode = -1;
		FPlatformProcess::GetProcReturnCode(UnrealEditorCmdProcHandle, &ProcReturnCode);
		UE_LOG(LogTemp, Display, TEXT("FAssetPatcherModule::PatchAssets : End!!"));
	}
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAssetPatcherModule, AssetPatcher)