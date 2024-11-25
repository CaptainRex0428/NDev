// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RenderTarget2Texture2D.generated.h"

/**
 * 
 */
UCLASS()
class NDEV_API URenderTarget2Texture2D : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(Blueprintcallable, category = "UDev", meta = (Keywords = "RT2DefaultTexture"))
	static  UTexture2D* DSRT2DefaultTexture(UTextureRenderTarget2D* InRenderTarget, FName InName, enum TextureCompressionSettings CompressionSettings, enum TextureMipGenSettings MipSettings);

	UFUNCTION(Blueprintcallable, category = "UDev", meta = (Keywords = "RT2Texture"))
	static UTexture2D* CreateTexture2DWithPixelData(int32 Width, int32 Height, TArray<FColor> PixelData, TextureCompressionSettings CompressionSettings = TC_Default, bool sRGB = true, TextureMipGenSettings MipMapSettings = TMGS_FromTextureGroup);

	UFUNCTION(Blueprintcallable, category = "UDev", meta = (Keywords = "RT2Texture"))
	static UTexture2D* CreateTexture2DWithColor(int32 Width, int32 Height, TextureCompressionSettings CompressionSettings = TC_Default, bool sRGB = true, TextureMipGenSettings MipMapSettings = TMGS_FromTextureGroup, FColor FillColor = FColor(255, 0, 122, 255));

	UFUNCTION(Blueprintcallable, category = "UDev", meta = (Keywords = "RT2Texture"))
	static bool ReadRenderTargetData(UTextureRenderTarget2D* InRenderTarget, TArray<FColor>& OutPixelData);

	UFUNCTION(Blueprintcallable, category = "UDev", meta = (Keywords = "RT2Texture"))
	static bool ReadRenderTargetData2Texture(UTextureRenderTarget2D* InRenderTarget, UTexture2D* Texture);

	UFUNCTION(Blueprintcallable, category = "UDev", meta = (Keywords = "RT2Texture"))
	static UTexture2D* CompressTextureFormat(UTexture2D* Texture);


private:
	static void GenerateMipMap(UTexture2D* Texture2D);

	static bool ReallocMipMap(FTexture2DMipMap* MipMap, int64 size);

	static EPixelFormat ChoosePixelFormat(TextureCompressionSettings CompressionSettings, bool bSRGB);
};
