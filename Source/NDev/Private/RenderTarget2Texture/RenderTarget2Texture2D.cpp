
#include "NDev.h"
#include "RenderTarget2Texture/RenderTarget2Texture2D.h"
#include "NDevMessage.h"

#include "UnrealClient.h"
#include "ImageCore.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/ITextureFormat.h"
#include "Interfaces/ITextureFormatModule.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureCompressorModule.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "IAssetViewport.h"
#endif

#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

#if WITH_EDITOR || UE_GAME 
#include "Developer/TextureCompressor/Public/TextureCompressorModule.h"
#endif //#if WITH_EDITOR || UE_GAME 

bool GEnableDebugRT2DfaultTexture = false;
static FAutoConsoleVariableRef  CVarDisableSceneCapture(
	TEXT("RT2DfaultTexture.EnableDebug"),
	GEnableDebugRT2DfaultTexture,
	TEXT("1 enable save files to ProjectDir/Saved/RT2Default folder, 0 disable\n"),
	ECVF_RenderThreadSafe | ECVF_Scalability
);

void SaveFileToDisk(FString InName, const void* InImageData, int32 InImageDataSize, int32 InWidth, int32 InHeight, EImageFormat  InImageFormat, ERGBFormat InERGBFormat)
{
#if UE_EDITOR
	if (!GEnableDebugRT2DfaultTexture)
	{
		return;
	}
	static int32 CaptureCount = 0;
	static FString 	SavePath = FPaths::Combine(FPaths::ProjectDir(), FString("Saved"), FString("RT2Default"));

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*SavePath))
	{
		PlatformFile.CreateDirectoryTree(*SavePath);
	}
	FString FileName;
	if (InImageFormat == EImageFormat::JPEG)
	{
		FileName = FString::Printf(TEXT("%s_%d_%d_%d.jpg"), *InName, InWidth, InHeight, CaptureCount);
	}
	else if (InImageFormat == EImageFormat::DDS)
	{
		FileName = FString::Printf(TEXT("%s_%d_%d_%d.dds"), *InName, InWidth, InHeight, CaptureCount);
	}

	FString CaptureFilePath = FPaths::Combine(SavePath, FileName);
	IImageWrapperModule* ImageWrapperModule = FModuleManager::GetModulePtr<IImageWrapperModule>(FName("ImageWrapper"));
	if (ImageWrapperModule != nullptr)
	{
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule->CreateImageWrapper(InImageFormat);

		if (InImageFormat == EImageFormat::JPEG)
		{

			ImageWrapper->SetRaw(InImageData, InImageDataSize, InWidth, InHeight, InERGBFormat, 8);

			const TArray64<uint8>& JPGData = ImageWrapper->GetCompressed(0);
			FFileHelper::SaveArrayToFile(JPGData, *CaptureFilePath);
		}
		else
		{
			ImageWrapper->SetRaw(InImageData, InImageDataSize, InWidth, InHeight, InERGBFormat, 8);
			const TArray64<uint8>& JPGData = ImageWrapper->GetCompressed(0);
			FFileHelper::SaveArrayToFile(JPGData, *CaptureFilePath);
		}
	}
	CaptureCount++;

#endif
}

UTexture2D* URenderTarget2Texture2D::DSRT2DefaultTexture(UTextureRenderTarget2D* InRenderTarget, FName InName, enum TextureCompressionSettings CompressionSettings, enum TextureMipGenSettings MipSettings)
{

#if WITH_EDITOR || UE_GAME 
	// 目前只处理RGBA模式的
	MsgLog("DSRT2DefaultTexture start.");
	MsgLog("DSRT2DefaultTexture 1.");
	int32 Width = InRenderTarget->SizeX;
	int32 Height = InRenderTarget->SizeY;

	TArray<FColor> SurfData;
	FTextureRenderTargetResource* RenderTarget = InRenderTarget->GameThread_GetRenderTargetResource();
	RenderTarget->ReadPixels(SurfData);


	SaveFileToDisk(InName.ToString(), SurfData.GetData(), SurfData.Num() * 4, Width, Height, EImageFormat::JPEG, ERGBFormat::RGBA);

	static const FName TextureFormatModuleName("TextureFormatDXT");
	ITextureFormat* TextureModule = FModuleManager::GetModuleChecked<ITextureFormatModule>(TextureFormatModuleName).GetTextureFormat();


	FImage  Image(Width, Height, 1, ERawImageFormat::BGRA8, InRenderTarget->SRGB ? EGammaSpace::sRGB : EGammaSpace::Linear);

	TArrayView64<FColor> ImageData = Image.AsBGRA8();
	//SurfData is rgbA8, Img is bgra8, so exchange it
	for (int32 h = 0; h < Height; h++)
	{
		for (int32 w = 0; w < Width; w++)
		{
			int32 idx = Width * h + w;
			ImageData[idx] = FColor(SurfData[idx].B, SurfData[idx].G, SurfData[idx].R, SurfData[idx].A);
		}
	}

	SaveFileToDisk(InName.ToString(), ImageData.GetData(), ImageData.Num() * 4, Width, Height, EImageFormat::DDS, ERGBFormat::BGRA);

	FTextureBuildSettings  TextureBuildSettings;
	ETextureRenderTargetFormat RTFormat = InRenderTarget->RenderTargetFormat;
	bool hasAlpha = (
		RTFormat == RTF_RGBA8 ||
		RTFormat == RTF_RGBA8_SRGB ||
		RTFormat == RTF_RGBA16f ||
		RTFormat == RTF_RGBA32f
		) ? true : false;

	{
		TextureBuildSettings.bSRGB = InRenderTarget->IsSRGB();
		FName TextureFormatName = NAME_None;

		// Supported texture format names.
		static FName NameDXT1(TEXT("DXT1"));
		static FName NameDXT3(TEXT("DXT3"));
		static FName NameDXT5(TEXT("DXT5"));
		static FName NameAutoDXT(TEXT("AutoDXT"));
		static FName NameDXT5n(TEXT("DXT5n"));
		static FName NameBC4(TEXT("BC4"));
		static FName NameBC5(TEXT("BC5"));

		//EPixelFormat pixelFormat = GetPixelFormatFromRenderTargetFormat();
		switch (CompressionSettings)
		{
		case TC_Default:
			TextureFormatName = hasAlpha ? NameDXT5 : NameDXT1;
			break;
		default:
			TextureFormatName = NameBC5;
			break;
		}
		TextureBuildSettings.TextureFormatName = TextureFormatName;
	}

	//start just need init but not really used params
	FIntVector3 InMip0Dimensions = FIntVector3(0, 0, 0);
	int32 InMip0NumSlicesNoDepth = 0;
	int32 InMipIndex = 0;
	int32 InMipCount = 1;
	FStringView DebugTexturePathName;
	//end just need init but not really used params

	bool bImageHasAlphaChannel = hasAlpha;
	FCompressedImage2D OutCompressedImage;

	TextureModule->CompressImage(Image, TextureBuildSettings, InMip0Dimensions, InMip0NumSlicesNoDepth, InMipIndex, InMipCount, DebugTexturePathName, bImageHasAlphaChannel, OutCompressedImage);

	//FImageView ImageView(OutCompressedImage.RawData.GetData(), Width, Height, ERawImageFormat::BGRA8);
	FImageView ImageView(Image);

	//debug functions, now the FName is None
	static int Count = 0;
	FString	FileName = FString::Printf(TEXT("RT2DefaultTexture_%s_%d_%d_%d.dds"), *InName.ToString(), Width, Height, Count++);
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, hasAlpha ? EPixelFormat::PF_DXT5 : EPixelFormat::PF_DXT1, *FileName, OutCompressedImage.RawData);
	Texture->SRGB = InRenderTarget->SRGB;
	Texture->NeverStream = true;
#else
	MsgLog("this should only be called on editor or game, not on server");
	return nullptr;
#endif
	MsgLog("DSRT2DefaultTexture 11111.");
	return  Texture;
}

bool URenderTarget2Texture2D::ReadRenderTargetData(UTextureRenderTarget2D* InRenderTarget, TArray<FColor>& OutPixelData)
{
	if (!InRenderTarget)
	{
		MsgLog(L"Invalid RenderTarget input.");
		return false;
	}

	FTextureRenderTargetResource* RenderTargetResource = InRenderTarget->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource)
	{
		MsgLog(L"Failed to get RenderTarget resource.");
		return false;
	}

	int32 Width = InRenderTarget->SizeX;
	int32 Height = InRenderTarget->SizeY;

	// 从 RenderTarget 读取像素数据
	TArray<FColor> RawPixelData;
	RawPixelData.AddUninitialized(Width * Height);

	RenderTargetResource->ReadPixels(RawPixelData);

	// 转换为 BGRA 格式
	OutPixelData.Empty();
	OutPixelData.AddUninitialized(Width * Height);

	for (int32 i = 0; i < RawPixelData.Num(); ++i)
	{
		const FColor& SrcColor = RawPixelData[i];
		OutPixelData[i] = FColor(SrcColor.R, SrcColor.G, SrcColor.B, SrcColor.A);
	}

	return true;
}

bool URenderTarget2Texture2D::ReadRenderTargetData2Texture(UTextureRenderTarget2D* InRenderTarget, UTexture2D* Texture)
{
	TArray<FColor> OutPixelData;

	int64 sourceX = Texture->GetSizeX();
	int64 sourceY = Texture->GetSizeY();

	if (!ReadRenderTargetData(InRenderTarget, OutPixelData) || OutPixelData.Num() == 0)
	{
		MsgLog(L"Read render target failed.");

		return false;
	};

	MsgLog(FString::Printf(L"PixelData count : %d", OutPixelData.Num()));

	if (!Texture)
	{
		MsgLog(L"Texture Input Error.");

		return false;
	}

	if ((Texture->GetSizeX() != 1 && Texture->GetSizeX() % 2 != 0) || (Texture->GetSizeY() != 1 && Texture->GetSizeY() % 2 != 0))
	{
		MsgLog("Texture input size error, not in 2^n format.");

		return false;
	}

	if ((InRenderTarget->SizeX != 1 && InRenderTarget->SizeX % 2 != 0) || (InRenderTarget->SizeY != 1 && InRenderTarget->SizeY % 2 != 0))
	{
		MsgLog("Render target input size error, not in 2^n format.");

		return false;
	}

	FTexturePlatformData* PlatformData = Texture->GetPlatformData();

	if (Texture->GetPlatformData()->PixelFormat != PF_B8G8R8A8)
	{
		Texture->GetPlatformData()->PixelFormat = PF_B8G8R8A8;
	}

	// 清理原始Mip

	PlatformData->Mips.Empty();

	Texture->UpdateResource();

	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	PlatformData->Mips.Add(Mip);

	Mip->SizeX = sourceX;
	Mip->SizeY = sourceY;
	ReallocMipMap(Mip, Mip->SizeX * Mip->SizeY * sizeof(FColor));


	// 确保纹理设置为可写
	void* TextureData = Mip->BulkData.Lock(LOCK_READ_WRITE);

	// 填充像素数据
	FColor* ColorData = static_cast<FColor*>(TextureData);

	float SpanX = static_cast<float>(InRenderTarget->SizeX) / Texture->GetSizeX();
	float SpanY = static_cast<float>(InRenderTarget->SizeY) / Texture->GetSizeY();

	for (int32 Y = 0; Y < Mip->SizeY; ++Y)
	{
		for (int32 X = 0; X < Mip->SizeX; ++X)
		{
			float SampleX = SpanX * X;
			float SampleY = SpanY * Y;

			int64 FloorX = FMath::Clamp(FMath::FloorToInt(SampleX), 0, InRenderTarget->SizeX - 1);
			int64 FloorY = FMath::Clamp(FMath::FloorToInt(SampleY), 0, InRenderTarget->SizeY - 1);

			int64 PixelIndex = FloorX + FloorY * InRenderTarget->SizeX;

			if (!OutPixelData.IsValidIndex(PixelIndex))
			{
				MsgLog(FString::Printf(TEXT("PixelIndex out of bounds: %d, Max: %d"), PixelIndex, OutPixelData.Num()));
				continue;
			}

			ColorData[X + Y * Mip->SizeX] = OutPixelData[FloorX + FloorY * InRenderTarget->SizeX];
		}
	}

	// 解锁并更新纹理数据
	Mip->BulkData.Unlock();

	Texture->UpdateResource();

	// 手动生成 MipMap
	GenerateMipMap(Texture);

	return true;
}

UTexture2D* URenderTarget2Texture2D::CreateTexture2DWithPixelData(int32 Width, int32 Height, TArray<FColor> PixelData, TextureCompressionSettings CompressionSettings, bool sRGB, TextureMipGenSettings MipMapSettings)
{
	if (PixelData.Num() != Width * Height)
	{
		MsgLog(FString::Printf(L"Invalid pixel data: Expected %d pixels, but got %d.", Width * Height, PixelData.Num()));
		return nullptr;
	}

	// 创建一个新的 UTexture2D 对象
	UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);

	if (!NewTexture)
	{
		return nullptr;
	}

	NewTexture->MipGenSettings = MipMapSettings;
	NewTexture->CompressionSettings = CompressionSettings;
	NewTexture->SRGB = sRGB;
	NewTexture->NeverStream = true; // 确保纹理不会被流式加载

	// 获取纹理的 Mip 数据
	FTexture2DMipMap& Mip = NewTexture->GetPlatformData()->Mips[0];
	// 确保纹理设置为可写
	void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);


	// 填充像素数据
	FColor* ColorData = static_cast<FColor*>(TextureData);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			// 从 OutPixelData 中读取颜色值
			ColorData[X + Y * Width] = PixelData[X + Y * Width];
		}
	}

	// 解锁并更新纹理数据
	Mip.BulkData.Unlock();

	NewTexture->UpdateResource();

	// 手动生成 MipMap
	GenerateMipMap(NewTexture);

	return NewTexture;
}

UTexture2D* URenderTarget2Texture2D::CreateTexture2DWithColor(int32 Width, int32 Height, TextureCompressionSettings CompressionSettings, bool sRGB, TextureMipGenSettings MipMapSettings, FColor FillColor)
{
	// 创建一个新的 UTexture2D 对象
	UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);

	if (!NewTexture)
	{
		return nullptr;
	}

	// 确保纹理设置为可写
	NewTexture->MipGenSettings = MipMapSettings;
	NewTexture->CompressionSettings = CompressionSettings;
	NewTexture->SRGB = sRGB;
	NewTexture->NeverStream = true; // 确保纹理不会被流式加载

	// 获取纹理的 Mip 数据
	FTexture2DMipMap& Mip = NewTexture->GetPlatformData()->Mips[0];
	ReallocMipMap(&Mip, Mip.SizeX * Mip.SizeY * sizeof(FColor));

	void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);

	// 填充像素数据
	FColor* ColorData = static_cast<FColor*>(TextureData);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			// 从 OutPixelData 中读取颜色值
			ColorData[X + Y * Width] = FillColor;
		}
	}

	// 解锁并更新纹理数据
	Mip.BulkData.Unlock();

	NewTexture->UpdateResource();

	// 手动生成 MipMap
	GenerateMipMap(NewTexture);


	return NewTexture;
}

void URenderTarget2Texture2D::GenerateMipMap(UTexture2D* Texture)
{
	if (!Texture || !Texture->GetPlatformData())
	{
		return;
	}

	// 获取平台数据
	FTexturePlatformData* PlatformData = Texture->GetPlatformData();

	// 获取第一层 MipMap
	FTexture2DMipMap SourceMip = PlatformData->Mips[0];
	int32 SourceWidth = SourceMip.SizeX;
	int32 SourceHeight = SourceMip.SizeY;

	if ((SourceWidth != 1 && SourceWidth % 2 != 0) || (SourceHeight != 1 && SourceHeight % 2 != 0))
	{
		return;
	}

	// 锁定原始像素数据
	void* SourceData = SourceMip.BulkData.Lock(LOCK_READ_WRITE);
	FColor* SourceColorData = static_cast<FColor*>(SourceData);

	// 创建多级细化级别
	int32 MipCount = FMath::FloorToInt(FMath::Log2(static_cast<float>(FMath::Min(SourceWidth, SourceHeight)))) + 1;

	for (int32 MipLevel = 1; MipLevel < MipCount; ++MipLevel)
	{
		int32 MipWidth = FMath::Max(1, SourceWidth >> MipLevel);
		int32 MipHeight = FMath::Max(1, SourceHeight >> MipLevel);

		// 创建新的 MipMap
		FTexture2DMipMap* NewMip = new FTexture2DMipMap();
		PlatformData->Mips.Add(NewMip);

		NewMip->SizeX = MipWidth;
		NewMip->SizeY = MipHeight;

		// 分配像素数据
		if (!ReallocMipMap(NewMip, MipWidth * MipHeight * sizeof(FColor)))
		{
			MsgLog(FString::Printf(L"MipData realloc error at level %d!", MipLevel), ELogVerbosity::Error);
			continue;
		};

		// 重新读取MipData
		FColor* MipData = static_cast<FColor*>(NewMip->BulkData.Lock(LOCK_READ_WRITE));

		if (!MipData)
		{
			MsgLog(FString::Printf(L"MipData is null at MipLevel %d!", MipLevel),ELogVerbosity::Error);
			NewMip->BulkData.Unlock();
			continue;
		}

		// 生成 MipMap 数据（简单的平均值降采样）
		for (int32 Y = 0; Y < MipHeight; ++Y)
		{
			for (int32 X = 0; X < MipWidth; ++X)
			{
				// 当前 MipMap 的像素对应上一层 2x2 块的起始位置
				int32 SourceX = X * (1 << MipLevel);
				int32 SourceY = Y * (1 << MipLevel);

				// 使用高精度类型累加颜色分量
				int32 SumR = 0, SumG = 0, SumB = 0, SumA = 0;
				int32 PixelCount = 0;

				for (int32 OffsetY = 0; OffsetY < (1 << MipLevel); ++OffsetY)
				{
					for (int32 OffsetX = 0; OffsetX < (1 << MipLevel); ++OffsetX)
					{
						int32 SampleX = SourceX + OffsetX;
						int32 SampleY = SourceY + OffsetY;

						if (SampleX < SourceWidth && SampleY < SourceHeight)
						{
							FColor SampleColor = SourceColorData[SampleX + SampleY * SourceWidth];

							SumR += SampleColor.R;
							SumG += SampleColor.G;
							SumB += SampleColor.B;
							SumA += SampleColor.A;

							PixelCount++;
						}
					}
				}

				// 平均值赋值给当前 MipMap
				if (PixelCount > 0)
				{
					MipData[X + Y * MipWidth] = FColor(
						static_cast<uint8>(SumR / PixelCount),
						static_cast<uint8>(SumG / PixelCount),
						static_cast<uint8>(SumB / PixelCount),
						static_cast<uint8>(SumA / PixelCount)
					);
				}
				else
				{
					MipData[X + Y * MipWidth] = FColor(0, 0, 0, 0); // 默认值
				}
			}
		}

		// 解锁 MipMap 数据
		NewMip->BulkData.Unlock();
	}

	// 解锁原始数据
	SourceMip.BulkData.Unlock();

	// 更新纹理资源
	Texture->UpdateResource();
}

bool URenderTarget2Texture2D::ReallocMipMap(FTexture2DMipMap* MipMap, int64 size)
{
	if (!MipMap)
	{
		MsgLog(L"MipMap input error.");
	}
	MipMap->BulkData.Lock(LOCK_READ_WRITE);
	MipMap->BulkData.Realloc(size);
	MipMap->BulkData.Unlock();

	return true;
}



EPixelFormat URenderTarget2Texture2D::ChoosePixelFormat(TextureCompressionSettings CompressionSettings, bool bSRGB)
{

	switch (CompressionSettings)
	{
	case TC_Default:
		return PF_DXT1;

	case TC_Normalmap:
		return PF_BC5;

	case TC_Grayscale:
		return PF_G8;

	case TC_HDR:
		return PF_FloatRGBA;

	default:
		return PF_B8G8R8A8;
	}

}

UTexture2D* URenderTarget2Texture2D::CompressTextureFormat(UTexture2D* Texture)
{
	if (!Texture)
	{
		MsgLog("Invalid texture provided for compression.");
		return Texture;
	}

#if WITH_EDITOR || UE_GAME 
	// 确保只在编辑器或游戏中运行
	static const FName TextureFormatModuleName("TextureFormatDXT");
	ITextureFormat* TextureModule = FModuleManager::GetModuleChecked<ITextureFormatModule>(TextureFormatModuleName).GetTextureFormat();

	if (!TextureModule)
	{
		MsgLog("Failed to load TextureFormatDXT module.");
		return Texture;
	}

	EPixelFormat SourceFormat = Texture->GetPixelFormat();


	if (SourceFormat != PF_B8G8R8A8 && SourceFormat != PF_R8G8B8A8)
	{
		MsgLog(L"Only handle unzipped texture in PF_B8G8R8A8 or PF_R8G8B8A8 mode.");
		return Texture;
	}

	if (Texture->CompressionSettings == TC_VectorDisplacementmap)
	{
		MsgLog(L"CompressionSettings no need to compress.");
		return Texture;
	}

	// 准备纹理压缩设置
	FTextureBuildSettings TextureBuildSettings;

	// 判断是否有 Alpha 通道，并选择目标压缩格式
	bool bHasAlpha = (SourceFormat == PF_B8G8R8A8 || SourceFormat == PF_R8G8B8A8 || SourceFormat == PF_FloatRGBA || SourceFormat == PF_A8);

	// 获取纹理平台数据
	FTexturePlatformData* PlatformData = Texture->GetPlatformData();

	EPixelFormat TargetPixelFormat = PF_Unknown;

	if (!PlatformData || PlatformData->Mips.Num() == 0)
	{
		MsgLog(L"Invalid platform data for texture.");
		return Texture;
	}

	{
		TextureBuildSettings.bSRGB = Texture->SRGB; // 保持纹理的 sRGB 设置

		FName TextureFormatName = NAME_None;

		static FName NameDXT1(TEXT("DXT1"));
		static FName NameDXT3(TEXT("DXT3"));
		static FName NameDXT5(TEXT("DXT5"));
		static FName NameAutoDXT(TEXT("AutoDXT"));
		static FName NameDXT5n(TEXT("DXT5n"));
		static FName NameBC4(TEXT("BC4"));
		static FName NameBC5(TEXT("BC5"));

		switch (Texture->CompressionSettings)
		{
		case TC_Normalmap:
			TextureFormatName = NameBC5;
			TargetPixelFormat = PF_BC5;
			break;
		case TC_Default:
		case TC_Masks:
		default:
			TextureFormatName = bHasAlpha ? NameDXT5 : NameDXT1;
			TargetPixelFormat = bHasAlpha ? PF_DXT5 : PF_DXT1;
			break;
		}

		PlatformData->PixelFormat = TargetPixelFormat;

		TextureBuildSettings.TextureFormatName = TextureFormatName;
	}

	for (int32 MipIndex = 0; MipIndex < PlatformData->Mips.Num(); ++MipIndex)
	{
		FImage MipImage;
		FTexture2DMipMap& Mip = PlatformData->Mips[MipIndex];

		// 构造 Mip 对应的 FImage 数据
		void* MipData = Mip.BulkData.Lock(LOCK_READ_ONLY);
		MipImage.Init(Mip.SizeX, Mip.SizeY, 1, ERawImageFormat::BGRA8, Texture->SRGB ? EGammaSpace::sRGB : EGammaSpace::Linear);
		FMemory::Memcpy(MipImage.AsBGRA8().GetData(), MipData, Mip.BulkData.GetBulkDataSize());
		Mip.BulkData.Unlock();

		if (Mip.SizeX < 4 || Mip.SizeY < 4)
		{
			MsgLog(FString::Printf(TEXT("Preserving raw data for small MipMap (MipIndex: %d, Size: %dx%d)"), MipIndex, Mip.SizeX, Mip.SizeY));

			// 保留原始数据
			Mip.BulkData.Lock(LOCK_READ_WRITE);
			void* BulkDataPtr = Mip.BulkData.Realloc(Mip.BulkData.GetBulkDataSize());
			if (BulkDataPtr)
			{
				FMemory::Memcpy(BulkDataPtr, MipImage.AsBGRA8().GetData(), Mip.BulkData.GetBulkDataSize());
			}
			Mip.BulkData.Unlock();

			continue;
		}

		FCompressedImage2D OutCompressedImage;

		// 压缩当前 Mip 层级
		if (!TextureModule->CompressImage(MipImage, TextureBuildSettings, FIntVector3(Mip.SizeX, Mip.SizeY, 1), 0, MipIndex, PlatformData->Mips.Num(), TEXT("DebugPath"), bHasAlpha, OutCompressedImage))
		{
			MsgLog(L"Failed to compress Mip %d");
			continue;
		}

		Mip.BulkData.Lock(LOCK_READ_WRITE);
		void* BulkDataPtr = Mip.BulkData.Realloc(OutCompressedImage.RawData.Num());
		if (!BulkDataPtr)
		{
			Mip.BulkData.Unlock();
			continue;
		}

		FMemory::Memcpy(BulkDataPtr, OutCompressedImage.RawData.GetData(), OutCompressedImage.RawData.Num());
		Mip.BulkData.Unlock();
	}

	// 更新纹理资源
	Texture->UpdateResource();

	return Texture;
#else
	MsgLog(FString::Prinf(L"This function (%s) should only be called in the editor or game, not on the server.",__FUNCTION__));
	return Texture;
#endif
}
