//
//	DDS file format module
//
//	Cube map is not supported now
//
#include "stdafx.h"
#include <stdio.h>
#include <D3D11.h>
#include <D2D1.h>
#include <ddraw.h>
#include <wincodec.h>

//
//	macros
//
#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o) if (o){ (o)->Release(); o = NULL; }
#define SAFE_DELETE(o)  if (o){ delete (o); o = NULL; }
#define SAFE_DELETE_ARRAY(o) if (o){ delete [] (o); o = NULL; }

//
//  prototypes.
//
DXGI_FORMAT	GetCompressedPixelFormatFromDdsHeader(DDSURFACEDESC2 *pddsc);
HRESULT CreateTextureFromDdsRgbImage(ID3D11DeviceContext *pContext, DDSURFACEDESC2 *pddsc, VOID *pBuffer, ID3D11Texture2D **ppTexture);
HRESULT CreateTextureFromDdsLinearImage(ID3D11DeviceContext *pContext, DDSURFACEDESC2 *pddsc, DXGI_FORMAT fmt, VOID *pBuffer, DWORD dwLinearSize, ID3D11Texture2D **ppTexture);

HRESULT CreateBitmapFromDdsLinearImage(ID2D1RenderTarget *pRenderTarget,DDSURFACEDESC2 *pddsc, DXGI_FORMAT fmt, VOID *pBuffer, DWORD dwLinearSize, ID2D1Bitmap **ppBitmap);
HRESULT CreateBitmapFromDdsRgbImage(ID2D1RenderTarget *pRenderTarget, DDSURFACEDESC2 *pddsc, VOID *pBuffer, ID2D1Bitmap **ppBitmap);


//
//	function CreateTextureFromDdsFile
//    @param :
//      pContext  : (in)Device's context
//		pFilename : (in)pathname of the image file
//	    ppTexture : (out) Load result
//      pSrcWidth : (out) width of the source image  ( != texture width )
//      pSrcHight : (out)  height of the source image ( != texture height )
//
HRESULT CreateTextureFromDdsFile(ID3D11DeviceContext *pContext, TCHAR *pFilename, ID3D11Texture2D **ppTexture, DWORD *pSrcWidth, DWORD *pSrcHeight){
	FILE *fp = NULL;
	errno_t err;
	unsigned char header[256];
	DWORD	*pBuffer = NULL;
	ID3D11Texture2D *pOutput = NULL;
	HRESULT	hr = E_FAIL;
	int idlength;
	DDSURFACEDESC2	*pddsc = NULL;
	DWORD		dwLinearSize = 0L;
	DXGI_FORMAT	texFormat = DXGI_FORMAT_UNKNOWN;
	err = _tfopen_s(&fp,pFilename,_T("rb"));

	if (fp== NULL)
		goto ERROR_EXIT;

	//  read the header;
	fread_s(header,sizeof(header),4,1,fp);
	if (*(DWORD*)header != 0x20534444L)	//  check FOURCC "DDS "
		goto ERROR_EXIT;

	fread_s(header,sizeof(header),1,1,fp);
	idlength = ((int)header[0]) & 255;
	if (idlength != 0x7c)
		goto ERROR_EXIT;
	fread_s(header+1,sizeof(header)-1,idlength-1,1,fp);

	pddsc = (DDSURFACEDESC2*)header;

	if (pddsc->dwFlags & DDSD_PITCH){
		dwLinearSize = (DWORD)pddsc->lPitch;
		dwLinearSize *= pddsc->dwHeight;
		pBuffer = new DWORD[(dwLinearSize+3) >> 2];
		fread((void*)pBuffer,1,dwLinearSize, fp);
		hr = CreateTextureFromDdsRgbImage(pContext,pddsc,pBuffer,&pOutput);
		if (SUCCEEDED(hr)){
			*pSrcWidth = pddsc->dwWidth;
			*pSrcHeight = pddsc->dwHeight;
			*ppTexture = pOutput;
			pOutput = NULL;
		}
	}else if (pddsc->dwFlags & DDSD_LINEARSIZE){
		dwLinearSize = (DWORD)pddsc->dwLinearSize;
		pBuffer = new DWORD[(dwLinearSize+3) >> 2];
		fread((void*)pBuffer,1,dwLinearSize, fp);
		texFormat = GetCompressedPixelFormatFromDdsHeader(pddsc);
		if (texFormat == DXGI_FORMAT_UNKNOWN){
			hr = CreateTextureFromDdsRgbImage(pContext,pddsc,pBuffer,&pOutput);
			if (SUCCEEDED(hr)){
				*pSrcWidth = pddsc->dwWidth;
				*pSrcHeight = pddsc->dwHeight;
				*ppTexture = pOutput;
				pOutput = NULL;
			}
			goto ERROR_EXIT;
		}else{
			hr = CreateTextureFromDdsLinearImage(pContext,pddsc,texFormat,pBuffer,dwLinearSize,&pOutput);

			SAFE_DELETE_ARRAY(pBuffer);
			if (FAILED(hr))
				goto ERROR_EXIT;

			*pSrcWidth = pddsc->dwWidth;
			*pSrcHeight = pddsc->dwHeight;
			*ppTexture = pOutput;
			pOutput = NULL;
		}
	}else if ((pddsc->dwFlags & DDSD_WIDTH) && (pddsc->dwFlags & DDSD_HEIGHT)){
		//  no pitch no linear size
		texFormat = GetCompressedPixelFormatFromDdsHeader(pddsc);
		if (texFormat != DXGI_FORMAT_UNKNOWN){

			//	compressed format
			switch(texFormat){
			case	DXGI_FORMAT_BC1_UNORM:
				dwLinearSize = pddsc->dwWidth * pddsc->dwHeight;
				dwLinearSize = (dwLinearSize+1) >> 1;
				break;
			case	DXGI_FORMAT_BC2_UNORM:
				dwLinearSize = pddsc->dwWidth * pddsc->dwHeight;
				break;
			case	DXGI_FORMAT_BC3_UNORM:
				dwLinearSize = pddsc->dwWidth * pddsc->dwHeight;
				break;
			default:
				goto ERROR_EXIT;
			}
			pBuffer = new DWORD[(dwLinearSize+3) >> 2];
			fread((void*)pBuffer,1,dwLinearSize, fp);

			hr = CreateTextureFromDdsLinearImage(pContext,pddsc,texFormat,pBuffer,dwLinearSize,&pOutput);

			SAFE_DELETE_ARRAY(pBuffer);
			if (FAILED(hr))
				goto ERROR_EXIT;

			*pSrcWidth = pddsc->dwWidth;
			*pSrcHeight = pddsc->dwHeight;
			*ppTexture = pOutput;
			pOutput = NULL;

		}else{

			//  uncompressed format

			DWORD pixelSize = pddsc->ddpfPixelFormat.dwRGBBitCount;
			pixelSize = (pixelSize + 7) >> 3;	//	byte size.

			dwLinearSize = pixelSize * pddsc->dwWidth * pddsc->dwHeight;
			pBuffer = new DWORD[(dwLinearSize+3) >> 2];
			fread((void*)pBuffer,1,dwLinearSize, fp);
			hr = CreateTextureFromDdsRgbImage(pContext,pddsc,pBuffer,&pOutput);
			if (SUCCEEDED(hr)){
				*pSrcWidth = pddsc->dwWidth;
				*pSrcHeight = pddsc->dwHeight;
				*ppTexture = pOutput;
				pOutput = NULL;
			}
		}
		
	}else
		goto ERROR_EXIT;	//  can't calculate the size


ERROR_EXIT:
	if (fp != NULL)
		fclose(fp);
	
	SAFE_RELEASE(pOutput);
	SAFE_DELETE_ARRAY(pBuffer);

	return hr;
}
//
//	function CreateD2D1BitmapFromDdsFile
//    @param :
//      pRenderTarget  : (in)Direct2D's Render Target
//		pFilename : (in)pathname of the image file
//	    ppTexture : (out) Load result
//      pSrcWidth : (out) width of the source image  ( != texture width )
//      pSrcHight : (out)  height of the source image ( != texture height )
//
HRESULT CreateD2D1BitmapFromDdsFile(ID2D1RenderTarget *pRenderTarget,TCHAR *pFilename, ID2D1Bitmap **ppBitmap){
	FILE *fp = NULL;
	errno_t err;
	unsigned char header[256];
	DWORD	*pBuffer = NULL;
	ID2D1Bitmap *pOutput = NULL;
	HRESULT	hr = E_FAIL;
	int idlength;
	DDSURFACEDESC2	*pddsc = NULL;
	DWORD		dwLinearSize = 0L;
	DXGI_FORMAT	texFormat = DXGI_FORMAT_UNKNOWN;

	err = _tfopen_s(&fp,pFilename,_T("rb"));

	if (fp== NULL)
		goto ERROR_EXIT;

	//  read the header;
	fread_s(header,sizeof(header),4,1,fp);
	if (*(DWORD*)header != 0x20534444L)	//  check FOURCC "DDS "
		goto ERROR_EXIT;

	fread_s(header,sizeof(header),1,1,fp);
	idlength = ((int)header[0]) & 255;
	if (idlength != 0x7c)
		goto ERROR_EXIT;
	fread_s(header+1,sizeof(header)-1,idlength-1,1,fp);

	pddsc = (DDSURFACEDESC2*)header;

	if (pddsc->dwFlags & DDSD_PITCH){
		dwLinearSize = (DWORD)pddsc->lPitch;
		dwLinearSize *= pddsc->dwHeight;
		pBuffer = new DWORD[(dwLinearSize+3) >> 2];
		fread((void*)pBuffer,1,dwLinearSize, fp);
		hr = CreateBitmapFromDdsRgbImage(pRenderTarget,pddsc,pBuffer,&pOutput);
		if (SUCCEEDED(hr)){
			*ppBitmap = pOutput;
			pOutput = NULL;
		}
	}else if (pddsc->dwFlags & DDSD_LINEARSIZE){
		dwLinearSize = (DWORD)pddsc->dwLinearSize;
		pBuffer = new DWORD[(dwLinearSize+3) >> 2];
		fread((void*)pBuffer,1,dwLinearSize, fp);
		texFormat = GetCompressedPixelFormatFromDdsHeader(pddsc);
		if (texFormat == DXGI_FORMAT_UNKNOWN){
			hr = CreateBitmapFromDdsRgbImage(pRenderTarget,pddsc,pBuffer,&pOutput);
			if (SUCCEEDED(hr)){
				*ppBitmap = pOutput;
				pOutput = NULL;
			}
			goto ERROR_EXIT;
		}else{
			hr= CreateBitmapFromDdsLinearImage(pRenderTarget,pddsc, texFormat,pBuffer, dwLinearSize, &pOutput);
			SAFE_DELETE_ARRAY(pBuffer);
			if (FAILED(hr))
				goto ERROR_EXIT;

			*ppBitmap = pOutput;
			pOutput = NULL;
		}
	}else if ((pddsc->dwFlags & DDSD_WIDTH) && (pddsc->dwFlags & DDSD_HEIGHT)){
		//  no pitch no linear size
		texFormat = GetCompressedPixelFormatFromDdsHeader(pddsc);
		if (texFormat != DXGI_FORMAT_UNKNOWN){

			//	compressed format
			switch(texFormat){
			case	DXGI_FORMAT_BC1_UNORM:
				dwLinearSize = pddsc->dwWidth * pddsc->dwHeight;
				dwLinearSize = (dwLinearSize+1) >> 1;
				break;
			case	DXGI_FORMAT_BC2_UNORM:
				dwLinearSize = pddsc->dwWidth * pddsc->dwHeight;
				break;
			case	DXGI_FORMAT_BC3_UNORM:
				dwLinearSize = pddsc->dwWidth * pddsc->dwHeight;
				break;
			default:
				goto ERROR_EXIT;
			}
			pBuffer = new DWORD[(dwLinearSize+3) >> 2];
			fread((void*)pBuffer,1,dwLinearSize, fp);
			hr= CreateBitmapFromDdsLinearImage(pRenderTarget,pddsc, texFormat,pBuffer, dwLinearSize, &pOutput);

			SAFE_DELETE_ARRAY(pBuffer);
			if (FAILED(hr))
				goto ERROR_EXIT;

			*ppBitmap = pOutput;
			pOutput = NULL;

		}else{

			//  uncompressed format

			DWORD pixelSize = pddsc->ddpfPixelFormat.dwRGBBitCount;
			pixelSize = (pixelSize + 7) >> 3;	//	byte size.

			dwLinearSize = pixelSize * pddsc->dwWidth * pddsc->dwHeight;
			pBuffer = new DWORD[(dwLinearSize+3) >> 2];
			fread((void*)pBuffer,1,dwLinearSize, fp);
			hr = CreateBitmapFromDdsRgbImage(pRenderTarget,pddsc,pBuffer,&pOutput);
			if (SUCCEEDED(hr)){
				*ppBitmap = pOutput;
				pOutput = NULL;
			}
		}
		
	}else
		goto ERROR_EXIT;	//  can't calculate the size

ERROR_EXIT:
	if (fp != NULL)
		fclose(fp);

	SAFE_RELEASE(pOutput);
	SAFE_DELETE_ARRAY(pBuffer);
	return hr;
}


//
//	Create texture from DDS linear image
//	@param
//		pContext : (in) Device's context
//		pddsc	 : (in) pointer to the direct draw surface
//		fmt      : (in) pixel format
//      pBuffer  : (in) image buffer
//      dwLinearSize : (in) image data size
//      ppTexture : (out) result texture.
//
HRESULT CreateTextureFromDdsLinearImage(ID3D11DeviceContext *pContext, DDSURFACEDESC2 *pddsc, DXGI_FORMAT fmt, VOID *pBuffer, DWORD dwLinearSize, ID3D11Texture2D **ppTexture){
	HRESULT  hr = E_FAIL;
	D3D11_TEXTURE2D_DESC     td;
	D3D11_MAPPED_SUBRESOURCE hMappedResource;
	ID3D11Device     *pDevice = NULL;
	ID3D11Texture2D  *pOutput = NULL;

	ZeroMemory( &td, sizeof( D3D11_TEXTURE2D_DESC ) );
	td.Width                = pddsc->dwWidth;
	td.Height               = pddsc->dwHeight;
	td.MipLevels            = (pddsc->dwFlags&DDSD_MIPMAPCOUNT)?pddsc->dwMipMapCount:1;
	td.ArraySize            = 1;
	td.Format               = fmt;
	td.SampleDesc.Count     = 1;	//	MULTI SAMPLE COUNT
	td.SampleDesc.Quality   = 0;	//	MULtI SAMPLE QUALITY
	td.Usage                = D3D11_USAGE_DYNAMIC;	//  Make it writeable
	td.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags       = D3D11_CPU_ACCESS_WRITE;
	td.MiscFlags            = 0;		

	pContext->GetDevice(&pDevice);

	hr = pDevice->CreateTexture2D(&td,NULL,&pOutput);

	if (SUCCEEDED(hr)){
		hr = pContext->Map( 
			pOutput,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&hMappedResource );
		if (SUCCEEDED(hr)){
			memcpy(hMappedResource.pData,pBuffer,dwLinearSize);

			pContext->Unmap(pOutput,0);		
			*ppTexture = pOutput;
			pOutput = NULL;
		}
	}
	SAFE_RELEASE(pOutput);
	SAFE_RELEASE(pDevice);
	return hr;
}


//
//	Create bitmap from DDS linear image
//	@param
//		pContext : (in) Device's context
//		pddsc	 : (in) pointer to the direct draw surface
//		fmt      : (in) pixel format
//      pBuffer  : (in) image buffer
//      dwLinearSize : (in) image data size
//      ppBitmap     : (out) result bitmap
//
HRESULT CreateBitmapFromDdsLinearImage(ID2D1RenderTarget *pRenderTarget,DDSURFACEDESC2 *pddsc, DXGI_FORMAT fmt, VOID *pBuffer, DWORD dwLinearSize, ID2D1Bitmap **ppBitmap){
	HRESULT  hr = E_FAIL;
	ID2D1Bitmap *pOutput = NULL;
	D2D1_SIZE_U bitmapSize;
	D2D1_BITMAP_PROPERTIES bitmapProperties;
	BYTE *pDest = NULL;
	
	bitmapSize.width = pddsc->dwWidth;
	bitmapSize.height = pddsc->dwHeight;
	bitmapProperties.dpiX = 96.0f;
	bitmapProperties.dpiY = 96.0f;
	bitmapProperties.pixelFormat.format = fmt;
	bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_STRAIGHT;

	hr = pRenderTarget->CreateBitmap(bitmapSize,pBuffer,0,&bitmapProperties,&pOutput);
	if (SUCCEEDED(hr)){
		*ppBitmap = pOutput;
	}
	SAFE_RELEASE(pOutput);
	return hr;
}



//
//  Get compressed pixel format from DDS Header
//	@param
//		pddsc : Direct Draw Surface Desc
//	@return
//		DXGI_FORMAT
//	Note.
//		if the format is uncompressed format returns DXGI_FORMAT_UNKNOWN
//
DXGI_FORMAT	GetCompressedPixelFormatFromDdsHeader(DDSURFACEDESC2 *pddsc){
	DXGI_FORMAT	format = DXGI_FORMAT_UNKNOWN;

	if (pddsc->ddpfPixelFormat.dwFlags & DDPF_FOURCC){
		switch(pddsc->ddpfPixelFormat.dwFourCC){
		case	FOURCC_DXT1:
			format = DXGI_FORMAT_BC1_UNORM;
			break;
		case	FOURCC_DXT2:
		case	FOURCC_DXT3:
			format = DXGI_FORMAT_BC2_UNORM;
			break;
		case	FOURCC_DXT4:
		case	FOURCC_DXT5:
			format = DXGI_FORMAT_BC3_UNORM;
			break;
		}
	}
	return format;
}

//
//  function CreateTextureFromDdsRgbImage
//	@param
//      pContext  : (in)Device's context
//		pddsc     : (in)direct draw surface desc
//		pBuffer   : (in)image data
//		ppTexture : (out) Load result
//
//	Note:
//		Create texture and put image with uncompressed format.
//		converting pixel formats to DXGI_FORMAT_B8G8R8A8_UNORM
//
HRESULT CreateTextureFromDdsRgbImage(ID3D11DeviceContext *pContext, DDSURFACEDESC2 *pddsc, VOID *pBuffer, ID3D11Texture2D **ppTexture){
	D3D11_TEXTURE2D_DESC	td;
	DWORD texWidth = 4;
	DWORD texHeight = 4;
	HRESULT hr = E_FAIL;
	ID3D11Device *pDevice = NULL;
	ID3D11Texture2D *pOutput = NULL;
	D3D11_MAPPED_SUBRESOURCE hMappedResource;

	if (0 == (pddsc->ddpfPixelFormat.dwFlags & DDPF_RGB)){
		goto ERROR_EXIT;
	}
	while(texWidth < pddsc->dwWidth)
		texWidth <<= 1;

	while(texHeight < pddsc->dwHeight)
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
	pDevice->CreateTexture2D(&td,NULL,&pOutput);

	INT bitCount = (INT)pddsc->ddpfPixelFormat.dwRGBBitCount;
	hr = pContext->Map( 
		pOutput,
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&hMappedResource );
	if (FAILED(hr))
		goto ERROR_EXIT;
	hr = E_FAIL;

	int r, g, b, a;
	DWORD srcPitch;
	DWORD srcWidth  = pddsc->dwWidth;
	DWORD srcHeight = pddsc->dwHeight;
	DWORD x, y;
	if (pddsc->dwFlags & DDSD_PITCH){
		srcPitch = (DWORD)pddsc->lPitch;
	}else{
		DWORD pixelByte = (bitCount+7)>>3;
		srcPitch = (DWORD)pddsc->dwWidth * ((bitCount+7)>>3);
	}
	BYTE *pDest = (BYTE*)hMappedResource.pData;
	BYTE *pByte, *pSrc;

	switch(bitCount){
	case	32:
		//  A8R8G8B8  Just copy pixels
		if (   (pddsc->ddpfPixelFormat.dwRBitMask == 0x00ff0000L)
			&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x0000ff00L)
			&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x000000ffL)
			&& (pddsc->ddpfPixelFormat.dwRGBAlphaBitMask == 0xff000000L)){
			//	A8R8G8B8
			for (y = 0; y < srcHeight ; ++y){
				pByte = pDest + (y * hMappedResource.RowPitch);
				pSrc = (BYTE*)pBuffer + (y * srcPitch);
				for (x = 0; x < srcWidth ; ++x){
					*(DWORD*)pByte = *(DWORD*)pSrc;
					pSrc  += sizeof(DWORD);
					pByte += sizeof(DWORD);
				}
				while(x < texWidth){
					*(DWORD*)pByte = 0L;
					pByte += sizeof(DWORD);
					++x;
				}
			}
			while(y < texHeight){
				pByte = pDest + (y * hMappedResource.RowPitch);
				for (x = 0; x < texWidth ; ++x){
					*(DWORD*)(pByte+sizeof(DWORD)*x) = 0L;
				}
				++y;
			}
			hr = S_OK;
		}else if (   (pddsc->ddpfPixelFormat.dwRBitMask == 0x000000ffL)
			&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x0000ff00L)
			&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x00ff0000L)
			&& (pddsc->ddpfPixelFormat.dwRGBAlphaBitMask == 0xff000000L)){
			//	A8B8G8R8 -> A8R8G8B8
			for (y = 0; y < srcHeight ; ++y){
				pByte = pDest + (y * hMappedResource.RowPitch);
				pSrc = (BYTE*)pBuffer + (y * srcPitch);
				for (x = 0; x < srcWidth ; ++x){
					r = 0xff & (int)*pSrc++;
					g = 0xff & (int)*pSrc++;
					b = 0xff & (int)*pSrc++;
					a = 0xff & (int)*pSrc++;
					*(DWORD*)pByte = (a << 24)+(r << 16)+(g << 8) + b;
					pByte += sizeof(DWORD);
				}
				while(x < texWidth){
					*(DWORD*)pByte = 0L;
					pByte += sizeof(DWORD);
					++x;
				}
			}
			while(y < texHeight){
				pByte = pDest + (y * hMappedResource.RowPitch);
				for (x = 0; x < texWidth ; ++x){
					*(DWORD*)(pByte+sizeof(DWORD)*x) = 0L;
				}
				++y;
			}
			hr = S_OK;
		}
		break;
	case	24:
		if (	(pddsc->ddpfPixelFormat.dwRBitMask == 0xff0000L)
			&&	(pddsc->ddpfPixelFormat.dwGBitMask == 0x00ff00L)
			&&	(pddsc->ddpfPixelFormat.dwBBitMask == 0x0000ffL)
		){
			//	R8G8B8 -> A8R8G8B8
			for (y = 0; y < srcHeight ; ++y){
				pByte = pDest + (y * hMappedResource.RowPitch);
				pSrc = (BYTE*)pBuffer + (y * srcPitch);
				for (x = 0; x < srcWidth ; ++x){
					b = 0xff & (int)*pSrc++;
					g = 0xff & (int)*pSrc++;
					r = 0xff & (int)*pSrc++;
					a = 255;
					*(DWORD*)pByte = (a << 24)+(r << 16)+(g << 8) + b;
					pByte += sizeof(DWORD);
				}
				while(x < texWidth){
					*(DWORD*)pByte = 0L;
					pByte += sizeof(DWORD);
					++x;
				}
			}
			while(y < texHeight){
				pByte = pDest + (y * hMappedResource.RowPitch);
				for (x = 0; x < texWidth ; ++x){
					*(DWORD*)(pByte+sizeof(DWORD)*x) = 0L;
				}
				++y;
			}
			hr = S_OK;
		}
		break;
	case	16:
		if (	(pddsc->ddpfPixelFormat.dwRBitMask == 0x7c00L)
			&&	(pddsc->ddpfPixelFormat.dwGBitMask == 0x03e0L)
			&&	(pddsc->ddpfPixelFormat.dwBBitMask == 0x001fL)
			&&	(pddsc->ddpfPixelFormat.dwRGBAlphaBitMask == 0x8000L)
		){
			//	A1R5G5B5 -> A8R8G8B8
			USHORT usPixel;
			for (y = 0; y < srcHeight ; ++y){
				pByte = pDest + (y * hMappedResource.RowPitch);
				pSrc = (BYTE*)pBuffer + (y * srcPitch);
				for (x = 0; x < srcWidth ; ++x){
					usPixel = *(USHORT*)pSrc;
					r = (((usPixel >> 10)&0x1f)*255)/31;
					g = (((usPixel >> 5) & 0x1f) * 255) / 31;
					b = ((usPixel & 0x1f) * 255) / 31;
					a = ((usPixel >> 15)&1) * 255;
					
					*(DWORD*)pByte = (a << 24)+(r << 16)+(g << 8) + b;
					pSrc  += sizeof(USHORT);
					pByte += sizeof(DWORD);
				}
				while(x < texWidth){
					*(DWORD*)pByte = 0L;
					pByte += sizeof(DWORD);
					++x;
				}
			}
			while(y < texHeight){
				pByte = pDest + (y * hMappedResource.RowPitch);
				for (x = 0; x < texWidth ; ++x){
					*(DWORD*)(pByte+sizeof(DWORD)*x) = 0L;
				}
				++y;
			}
			hr = S_OK;
		}else if (	(pddsc->ddpfPixelFormat.dwRBitMask == 0xf800L)
			&&	(pddsc->ddpfPixelFormat.dwGBitMask == 0x07e0L)
			&&	(pddsc->ddpfPixelFormat.dwBBitMask == 0x001fL)
		){
			//	R5G6B5 -> A8R8G8B8
			USHORT usPixel;
			for (y = 0; y < srcHeight ; ++y){
				pByte = pDest + (y * hMappedResource.RowPitch);
				pSrc = (BYTE*)pBuffer + (y * srcPitch);
				for (x = 0; x < srcWidth ; ++x){
					usPixel = *(USHORT*)pSrc;
					r = (((usPixel >> 11)&0x1f)*255)/31;
					g = (((usPixel >> 5) & 0x3f) * 255) / 63;
					b = ((usPixel & 0x1f) * 255) / 31;
					a = 255;
					
					*(DWORD*)pByte = (a << 24)+(r << 16)+(g << 8) + b;
					pSrc  += sizeof(USHORT);
					pByte += sizeof(DWORD);
				}
				while(x < texWidth){
					*(DWORD*)pByte = 0L;
					pByte += sizeof(DWORD);
					++x;
				}
			}
			while(y < texHeight){
				pByte = pDest + (y * hMappedResource.RowPitch);
				for (x = 0; x < texWidth ; ++x){
					*(DWORD*)(pByte+sizeof(DWORD)*x) = 0L;
				}
				++y;
			}
			hr = S_OK;
		}
	}
	pContext->Unmap(pOutput,0);
	if (SUCCEEDED(hr)){
		*ppTexture = pOutput;
		pOutput = NULL;
	}
ERROR_EXIT:
	SAFE_RELEASE(pOutput);
	SAFE_RELEASE(pDevice);
	return hr;
}

//
//  function CreateBitmapFromDdsRgbImage
//	@param
//      pContext  : (in)Device's context
//		pddsc     : (in)direct draw surface desc
//		pBuffer   : (in)image data
//		ppBitmap  : (out) Load result
//
//	Note:
//		Create bitmap and put image with uncompressed format.
//		converting pixel formats to DXGI_FORMAT_B8G8R8A8_UNORM
//
HRESULT CreateBitmapFromDdsRgbImage(ID2D1RenderTarget *pRenderTarget, DDSURFACEDESC2 *pddsc, VOID *pBuffer, ID2D1Bitmap **ppBitmap){
	HRESULT hr = E_FAIL;
	ID2D1Bitmap *pOutput = NULL;
	D2D1_SIZE_U bitmapSize;
	D2D1_BITMAP_PROPERTIES bitmapProperties;
	BYTE *pDest = NULL;
	if (0 == (pddsc->ddpfPixelFormat.dwFlags & DDPF_RGB)){
		goto ERROR_EXIT;
	}

	bitmapSize.width = pddsc->dwWidth;
	bitmapSize.height = pddsc->dwHeight;
	bitmapProperties.dpiX = 96.0f;
	bitmapProperties.dpiY = 96.0f;
	bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_STRAIGHT;
	
	INT bitCount = (INT)pddsc->ddpfPixelFormat.dwRGBBitCount;

	if (FAILED(hr))
		goto ERROR_EXIT;
	hr = E_FAIL;

	int r, g, b, a;
	DWORD srcPitch;
	DWORD srcWidth  = pddsc->dwWidth;
	DWORD srcHeight = pddsc->dwHeight;
	DWORD x, y;
	if (pddsc->dwFlags & DDSD_PITCH){
		srcPitch = (DWORD)pddsc->lPitch;
	}else{
		DWORD pixelByte = (bitCount+7)>>3;
		srcPitch = (DWORD)pddsc->dwWidth * ((bitCount+7)>>3);
	}
	pDest = (BYTE*)new DWORD[srcWidth*srcHeight];
	BYTE *pByte, *pSrc;
	DWORD destPitch = srcWidth * sizeof(DWORD);

	switch(bitCount){
	case	32:
		//  A8R8G8B8  Just copy pixels
		if (   (pddsc->ddpfPixelFormat.dwRBitMask == 0x00ff0000L)
			&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x0000ff00L)
			&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x000000ffL)
			&& (pddsc->ddpfPixelFormat.dwRGBAlphaBitMask == 0xff000000L)){
			//	A8R8G8B8
			hr = pRenderTarget->CreateBitmap(bitmapSize,pBuffer,srcPitch,&bitmapProperties,&pOutput);
		}else if (   (pddsc->ddpfPixelFormat.dwRBitMask == 0x000000ffL)
			&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x0000ff00L)
			&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x00ff0000L)
			&& (pddsc->ddpfPixelFormat.dwRGBAlphaBitMask == 0xff000000L)){
			//	A8B8G8R8 -> A8R8G8B8
			for (y = 0; y < srcHeight ; ++y){
				pByte = pDest + (y * destPitch);
				pSrc = (BYTE*)pBuffer + (y * srcPitch);
				for (x = 0; x < srcWidth ; ++x){
					r = 0xff & (int)*pSrc++;
					g = 0xff & (int)*pSrc++;
					b = 0xff & (int)*pSrc++;
					a = 0xff & (int)*pSrc++;
					*(DWORD*)pByte = (a << 24)+(r << 16)+(g << 8) + b;
					pByte += sizeof(DWORD);
				}
			}
			hr = pRenderTarget->CreateBitmap(bitmapSize,pDest,destPitch,&bitmapProperties,&pOutput);
		}
		break;
	case	24:
		if (	(pddsc->ddpfPixelFormat.dwRBitMask == 0xff0000L)
			&&	(pddsc->ddpfPixelFormat.dwGBitMask == 0x00ff00L)
			&&	(pddsc->ddpfPixelFormat.dwBBitMask == 0x0000ffL)
		){
			//	R8G8B8 -> A8R8G8B8
			for (y = 0; y < srcHeight ; ++y){
				pByte = pDest + (y * destPitch);
				pSrc = (BYTE*)pBuffer + (y * srcPitch);
				for (x = 0; x < srcWidth ; ++x){
					b = 0xff & (int)*pSrc++;
					g = 0xff & (int)*pSrc++;
					r = 0xff & (int)*pSrc++;
					a = 255;
					*(DWORD*)pByte = (a << 24)+(r << 16)+(g << 8) + b;
					pByte += sizeof(DWORD);
				}
			}
			hr = pRenderTarget->CreateBitmap(bitmapSize,pDest,destPitch,&bitmapProperties,&pOutput);
		}
		break;
	case	16:
		if (	(pddsc->ddpfPixelFormat.dwRBitMask == 0x7c00L)
			&&	(pddsc->ddpfPixelFormat.dwGBitMask == 0x03e0L)
			&&	(pddsc->ddpfPixelFormat.dwBBitMask == 0x001fL)
			&&	(pddsc->ddpfPixelFormat.dwRGBAlphaBitMask == 0x8000L)
		){
			//	A1R5G5B5 -> A8R8G8B8
			USHORT usPixel;
			for (y = 0; y < srcHeight ; ++y){
				pByte = pDest + (y * destPitch);
				pSrc = (BYTE*)pBuffer + (y * srcPitch);
				for (x = 0; x < srcWidth ; ++x){
					usPixel = *(USHORT*)pSrc;
					r = (((usPixel >> 10)&0x1f)*255)/31;
					g = (((usPixel >> 5) & 0x1f) * 255) / 31;
					b = ((usPixel & 0x1f) * 255) / 31;
					a = ((usPixel >> 15)&1) * 255;
					
					*(DWORD*)pByte = (a << 24)+(r << 16)+(g << 8) + b;
					pSrc  += sizeof(USHORT);
					pByte += sizeof(DWORD);
				}
			}
			hr = pRenderTarget->CreateBitmap(bitmapSize,pDest,destPitch,&bitmapProperties,&pOutput);
		}else if (	(pddsc->ddpfPixelFormat.dwRBitMask == 0xf800L)
			&&	(pddsc->ddpfPixelFormat.dwGBitMask == 0x07e0L)
			&&	(pddsc->ddpfPixelFormat.dwBBitMask == 0x001fL)
		){
			//	R5G6B5 -> A8R8G8B8
			USHORT usPixel;
			for (y = 0; y < srcHeight ; ++y){
				pByte = pDest + (y * destPitch);
				pSrc = (BYTE*)pBuffer + (y * srcPitch);
				for (x = 0; x < srcWidth ; ++x){
					usPixel = *(USHORT*)pSrc;
					r = (((usPixel >> 11)&0x1f)*255)/31;
					g = (((usPixel >> 5) & 0x3f) * 255) / 63;
					b = ((usPixel & 0x1f) * 255) / 31;
					a = 255;
					
					*(DWORD*)pByte = (a << 24)+(r << 16)+(g << 8) + b;
					pSrc  += sizeof(USHORT);
					pByte += sizeof(DWORD);
				}
			}
			hr = pRenderTarget->CreateBitmap(bitmapSize,pDest,destPitch,&bitmapProperties,&pOutput);
		}
	}
	if (SUCCEEDED(hr)){
		*ppBitmap = pOutput;
		pOutput = NULL;
	}
ERROR_EXIT:
	SAFE_DELETE_ARRAY(pDest);
	SAFE_RELEASE(pOutput);
	return hr;
}

