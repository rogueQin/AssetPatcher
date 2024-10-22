// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

class FAssetCommands : public TCommands<FAssetCommands>
{
public:
	FAssetCommands();

	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> AssetPatcher;
	TSharedPtr<FUICommandInfo> CookAssets;
	TSharedPtr<FUICommandInfo> PakAssets;
};
