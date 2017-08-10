//
//  class CTextureLoader
//
//  Desc : This program loads images via 
//		WIC ( Windows Imaging Component ) and 
//		creates 2D Textures
//
#include "stdafx.h"
#include <stdio.h>
#include <D3D11.h>
#include <D2D1.h>
#include <ddraw.h>
#include <wincodec.h>
#include "TextureLoader.h"
#include "ddsfunctions.h"
#include "tgafunctions.h"

#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o) if (o){ (o)->Release(); o = NULL; }
#define SAFE_DELETE(o)  if (o){ delete (o); o = NULL; }
#define SAFE_DELETE_ARRAY(o) if (o){ delete [] (o); o = NULL; }


CTextureLoader *CTextureLoader::m_pInstance = NULL;

CTextureLoader::CTextureLoader(void)
{
	HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_pFactory)
        );
}


CTextureLoader::~CTextureLoader(void)
{
	if (m_pFactory != NULL){
		m_pFactory->Release();
		m_pFactory = NULL;
	}
	//SAFE_RELEASE(m_pFactory);
}

void CTextureLoader::Initialize(){
	if (m_pInstance != NULL)
		return;
	m_pInstance = new CTextureLoader();
}

CTextureLoader *CTextureLoader::GetInstance(){
	if (m_pInstance == NULL){
		Initialize();
	}
	return m_pInstance;
}

void CTextureLoader::Destroy(){

	if (m_pInstance != NULL){
		delete m_pInstance;
		m_pInstance = NULL;
	}
}

//
//  function CopyBitmapSourceToTexture2D
//    @param :
//      pContext : Device's context
//	    pBitmapSource   : BitmapSource to copy
//      pTexture : The Texture to copy to
//      stride   : Texture's vertical stride
//      size     : Texture's size
//
HRESULT CopyBitmapSourceToTexture2D(ID3D11DeviceContext *pContext, IWICBitmapSource *pBitmapSource, ID3D11Texture2D *pTexture, UINT stride, UINT size){
	D3D11_MAPPED_SUBRESOURCE hMappedResource;
	HRESULT hr;
	hr = pContext->Map( 
		pTexture,
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&hMappedResource );
	if (SUCCEEDED(hr)){
		// ここで書き込む
		BYTE* pBits = (BYTE*)hMappedResource.pData;
		hr = pBitmapSource->CopyPixels(NULL,stride, size, pBits);
		pContext->Unmap(pTexture,0);
	}
	return hr;
}

HRESULT StretchCopyBitmapSourceToTexture2D(ID3D11DeviceContext *pContext, IWICBitmapSource *pBitmapSource, ID3D11Texture2D *pTexture, UINT stride, UINT size, FillMode mode){
	D3D11_MAPPED_SUBRESOURCE hMappedResource;
	HRESULT hr;
	BYTE	*pBuffer = NULL;
	if (mode == FillMode::None){
		return CopyBitmapSourceToTexture2D(pContext,pBitmapSource,pTexture,stride,size);
	}
	D3D11_TEXTURE2D_DESC td;
	pTexture->GetDesc(&td);
	
	//  Copy with linear filter
	DWORD sourceStride;
	DWORD sourceSize;
	UINT  sourceWidth;
	UINT  sourceHeight;
	pBitmapSource->GetSize(&sourceWidth,&sourceHeight);

	if (td.Width == sourceWidth && td.Height == sourceHeight){
		return CopyBitmapSourceToTexture2D(pContext,pBitmapSource,pTexture,stride,size);
	}

	sourceStride = sourceWidth * sizeof(DWORD);
	sourceSize = sourceStride * sourceHeight;
	pBuffer = (BYTE*)new DWORD[(sourceSize + 3) >> 2];
	
	hr = pBitmapSource->CopyPixels(NULL,sourceStride, sourceSize, (BYTE*)pBuffer);


	if (FAILED(hr))
		goto ERROR_EXIT;
	hr = pContext->Map( 
		pTexture,
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&hMappedResource );
	if (SUCCEEDED(hr)){
		BYTE *pBits = (BYTE*)hMappedResource.pData, *pBits2;
		FLOAT sx, sy;
		INT iSx1, iSx2;
		INT iSy1, iSy2;
		DWORD	tx, ty;
		FLOAT u0,u1,t[4];
		FLOAT r,g,b,a;
		DWORD pixel[4];
		DWORD pixel1;
		FLOAT rTexWidth, rTexHeight;
		rTexWidth = 1.0f / (FLOAT)td.Width;
		rTexHeight = 1.0f / (FLOAT)td.Height;
		for (ty = 0; ty < td.Height ; ++ty){
			pBits2 = pBits;
			sy = (FLOAT)ty * sourceHeight * rTexHeight;
			iSy1 = (INT)sy;
			iSy2 = (iSy1 + 1)%sourceHeight;
			u1 = sy - (FLOAT)iSy1;
			for (tx = 0; tx < td.Width ; ++tx){
				sx = (FLOAT)tx * sourceWidth * rTexWidth;
				iSx1 = (INT)sx;
				iSx2 = (iSx1 + 1)%sourceWidth;
				u0 = sx - (FLOAT)iSx1;
				pixel[0] = *(DWORD*)(pBuffer+(sourceStride*iSy1+4*iSx1));
				pixel[1] = *(DWORD*)(pBuffer+(sourceStride*iSy1+4*iSx2));
				pixel[2] = *(DWORD*)(pBuffer+(sourceStride*iSy2+4*iSx1));
				pixel[3] = *(DWORD*)(pBuffer+(sourceStride*iSy2+4*iSx2));
				t[0] = (1-u0)*(1-u1);
				t[1] = u0*(1-u1);
				t[2] = (1-u0)*u1;
				t[3] = u0*u1;
				r = g = b = a = 0.0f;
				for (int i = 0; i < _countof(t) ; ++i){
					a += ((pixel[i]>>24)&0xff) * t[i];
					r += ((pixel[i]>>16)&0xff) * t[i];
					g += ((pixel[i]>>8)&0xff) * t[i];
					b += ((pixel[i])&0xff) * t[i];
				}

				pixel1  = (((int)a&0xff)<<24);
				pixel1 += (((int)r&0xff)<<16);
				pixel1 += (((int)g&0xff)<<8);
				pixel1 += ( (int)b&0xff);
				*(DWORD*)pBits2 = pixel1;

				pBits2 += sizeof(DWORD);
			}
			pBits += hMappedResource.RowPitch;
		}
		pContext->Unmap(pTexture,0);
	}
ERROR_EXIT:
	SAFE_DELETE(pBuffer);
	return hr;
}

//
//  method: CreateTextureFromFile
//    @param :
//      pContext  : (in)Device's context
//		pFilename : (in)pathname of the image file
//	    ppTexture : (out) Load result
//      pSrcWidth : (out) width of the source image  ( != texture width )
//      pSrcHight : (out)  height of the source image ( != texture height )
//
//   @return
//      S_OK : succeeded
//
HRESULT CTextureLoader::CreateTextureFromFile(ID3D11DeviceContext *pContext, TCHAR *pFilename, ID3D11Texture2D **ppTexture, DWORD *pSrcWidth, DWORD *pSrcHeight, FillMode dwFillmode){
	IWICBitmapDecoder *pDecoder = NULL;
	IWICBitmapFrameDecode *pFrame = NULL;
	WICPixelFormatGUID pixelFormat;
	ID3D11Texture2D *pOutput = NULL;
	ID3D11Device *pDevice = NULL;
	//IWICImagingFactory *pFactory;
	HRESULT hr;

	//  Check if TGA
	int len = (INT)_tcslen(pFilename);
	if (len > 3){
		TCHAR *pTale = pFilename;
		pTale = pFilename + (len - 4);
#ifdef _TARGA_SUPPORTED_
		if (0 == _tcsicmp(pTale,_T(".tga"))){
			return CreateTextureFromTgaFile(pContext,pFilename,ppTexture,pSrcWidth,pSrcHeight,dwFillmode);
		}
#endif
#ifdef _DDS_SUPPORTED_
		if (0 == _tcsicmp(pTale,_T(".dds"))){
			//  create texture from dds file
			return CreateTextureFromDdsFile(pContext,pFilename,ppTexture,pSrcWidth,pSrcHeight);
		}
#endif
	}

	//  create texture using WIC
	CTextureLoader *pInstance = GetInstance();


	hr = pInstance->m_pFactory->CreateDecoderFromFilename(pFilename,0,
		GENERIC_READ,WICDecodeMetadataCacheOnDemand,&pDecoder);
	if (FAILED(hr))
		goto ERROR_EXIT;
	hr = pDecoder->GetFrame(0,&pFrame);
	if (FAILED(hr))
		goto ERROR_EXIT;

	UINT width, height;
	pFrame->GetSize(&width, &height);

	//  テクスチャの生成
    D3D11_TEXTURE2D_DESC td;
	UINT texWidth = 4, texHeight = 4;
	while(texWidth < width)
		texWidth <<= 1;
	while(texHeight < height)
		texHeight <<= 1;
	
    ZeroMemory( &td, sizeof( D3D11_TEXTURE2D_DESC ) );
    td.Width                = texWidth;
    td.Height               = texHeight;
    td.MipLevels            = 1;
    td.ArraySize            = 1;
    td.Format               = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count     = 1;	//	MULTI SAMPLE COUNT
    td.SampleDesc.Quality   = 0;	//	MULtI SAMPLE QUALITY
    td.Usage                = D3D11_USAGE_DYNAMIC;	//  Make it writeable
    td.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
    td.MiscFlags            = 0;

	pContext->GetDevice(&pDevice);
	hr = pDevice->CreateTexture2D(&td,NULL,&pOutput);
	if (FAILED(hr))
		goto ERROR_EXIT;

	//  読み込んだ画像のピクセルフォーマットを取得
	hr = pFrame->GetPixelFormat(&pixelFormat);
	if (FAILED(hr))
		goto ERROR_EXIT;
	
	if (pixelFormat != GUID_WICPixelFormat32bppBGRA){
		IWICFormatConverter *pFC;
		hr = m_pInstance->m_pFactory->CreateFormatConverter(&pFC);
		if (FAILED(hr))
			goto ERROR_EXIT;
		
		//  画像変換の用意
		hr = pFC->Initialize(pFrame,GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
		if (FAILED(hr)){
			SAFE_RELEASE(pFC);
			goto ERROR_EXIT;
		}
		//  テクスチャに流し込み
		StretchCopyBitmapSourceToTexture2D(pContext,pFC,pOutput, 4*texWidth, 4 * texWidth* texHeight,dwFillmode );
		SAFE_RELEASE(pFC);
	}else{
		//	変換不要な時
		StretchCopyBitmapSourceToTexture2D(pContext,pFrame,pOutput, 4*texWidth, 4 * texWidth* texHeight,dwFillmode );
	}
ERROR_EXIT:
	SAFE_RELEASE(pDevice);
	if (FAILED(hr)){
		SAFE_RELEASE(pOutput);
	}else{
		*ppTexture = pOutput;
		if (dwFillmode == FillMode::None){
			*pSrcWidth = width;
			*pSrcHeight = height;
		}else{
			*pSrcWidth  = texWidth;
			*pSrcHeight = texHeight;
		}
	}
	SAFE_RELEASE(pFrame);
	SAFE_RELEASE(pDecoder)
	return hr;
}

#ifdef __DIRECT2D__
//
//  method: CreateD2D1BitmapFromFile
//    @param :
//      pRenderTarget  : (in)Direct2D's RenderTarget
//		pFilename : (in)pathname of the image file
//	    ppBitmap  : (out) Load result
//
//   @return
//      S_OK : succeeded
//
HRESULT CTextureLoader::CreateD2D1BitmapFromFile(ID2D1RenderTarget *pRenderTarget,TCHAR *pFilename, ID2D1Bitmap **ppBitmap){
	HRESULT hr;
	IWICBitmapDecoder *pDecoder = NULL;
	IWICBitmapFrameDecode *pFrame = NULL;
	WICPixelFormatGUID pixelFormat;
	ID2D1Bitmap *pOutput = NULL;
	IWICFormatConverter *pFormatConverter = NULL;
	//  create texture using WIC
	CTextureLoader *pInstance = GetInstance();

	hr = pInstance->m_pFactory->CreateDecoderFromFilename(pFilename,0,
		GENERIC_READ,WICDecodeMetadataCacheOnDemand,&pDecoder);
	if (FAILED(hr))
		goto ERROR_EXIT;
	hr = pDecoder->GetFrame(0,&pFrame);
	if (FAILED(hr))
		goto ERROR_EXIT;


	UINT width, height;
	pFrame->GetSize(&width, &height);

	//  Get the pixel format of the image
	hr = pFrame->GetPixelFormat(&pixelFormat);
	if (FAILED(hr))
		goto ERROR_EXIT;
	
	if (pixelFormat != GUID_WICPixelFormat32bppBGRA){

		hr = m_pInstance->m_pFactory->CreateFormatConverter(&pFormatConverter);
		if (FAILED(hr))
			goto ERROR_EXIT;
		
		//  convert the pixel format
		hr = pFormatConverter->Initialize(pFrame,GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
		if (FAILED(hr)){
			SAFE_RELEASE(pFormatConverter);
			goto ERROR_EXIT;
		}
		//  create D2DBitmap
        hr = pRenderTarget->CreateBitmapFromWicBitmap(
            pFormatConverter,
            NULL,
            &pOutput
            );

	}else{
		//  create D2DBitmap
        hr = pRenderTarget->CreateBitmapFromWicBitmap(
            pFrame,
            NULL,
            &pOutput
            );
	}

ERROR_EXIT:
	if (FAILED(hr)){
		SAFE_RELEASE(pOutput);
	}else{
		*ppBitmap = pOutput;
	}
	SAFE_RELEASE(pFormatConverter);
	SAFE_RELEASE(pFrame);
	SAFE_RELEASE(pDecoder)
	return hr;
}
#endif
