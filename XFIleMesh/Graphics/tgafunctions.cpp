//
//	TrueVision Targa format module
//
#include "stdafx.h"
#include <stdio.h>
#include <D3D11.h>
#include <D2D1.h>
#include <ddraw.h>
#include <wincodec.h>
#include "TextureLoader.h"
#include "tgafunctions.h"

//
//  macros
//
#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o) if (o){ (o)->Release(); o = NULL; }
#define SAFE_DELETE(o)  if (o){ delete (o); o = NULL; }
#define SAFE_DELETE_ARRAY(o) if (o){ delete [] (o); o = NULL; }

//
//  prototypes
//
HRESULT	DecodeTgaHeader(BYTE *pHeader, int *pPixelBits, BOOL *pRLE, BOOL *pBW, DWORD *pSrcWidth, DWORD *pSrcHeight, BYTE *pFlags);
HRESULT ConvertPixelsFromTgaFile(FILE *fp,DWORD *pBuffer, DWORD numPixels, BOOL bRLE, BOOL bBW, INT numBitsPerPixel);

HRESULT CopyTgaSourceToTexture2D(ID3D11DeviceContext *pContext, ID3D11Texture2D *pTexture, DWORD *pSource, UINT width, UINT height, UINT pitch, BYTE flags, FillMode fillmode);
HRESULT StretchCopyTgaSourceToTexture2D(ID3D11DeviceContext *pContext, ID3D11Texture2D *pTexture, DWORD *pSource, UINT width, UINT height, UINT pitch, BYTE flags);

//
//	function CreateTextureFromTgaFile
//    @param :
//      pContext  : (in)Device's context
//		pFilename : (in)pathname of the image file
//	    ppTexture : (out) Load result
//      pSrcWidth : (out) width of the source image  ( != texture width )
//      pSrcHight : (out)  height of the source image ( != texture height )
//		dwFillmode : (in)fillmode None/linear
//
HRESULT CreateTextureFromTgaFile(ID3D11DeviceContext *pContext, TCHAR *pFilename, ID3D11Texture2D **ppTexture, DWORD *pSrcWidth, DWORD *pSrcHeight, FillMode dwFillmode){
	FILE *fp = NULL;
	errno_t err;
	unsigned char header[256];
	DWORD srcwidth,srcheight;
	int pixelbits, row_pitch;
	BOOL	bRLE = false, bBW = false;
	DWORD	*pBuffer = NULL;
	int		num_elements = 0;
	ID3D11Device *pDevice = NULL;
	ID3D11Texture2D *pOutput = NULL;
	HRESULT	hr = E_FAIL;
	int idlength;
	BYTE src_descriptor;

	err = _tfopen_s(&fp,pFilename,_T("rb"));

	if (fp== NULL)
		goto ERROR_EXIT;

	//  read the header;
	fread_s(header,sizeof(header),1,1,fp);
	idlength = ((int)header[0]) & 255;
	if (idlength == 0)
		idlength = 18;
	fread_s(header+1,sizeof(header)-1,idlength-1,1,fp);

	hr = DecodeTgaHeader(header,&pixelbits,&bRLE,&bBW,&srcwidth,&srcheight, &src_descriptor);
	if (FAILED(hr)){
		goto ERROR_EXIT;
	}

	num_elements = srcwidth * srcheight;
	pBuffer = new DWORD[num_elements];
	row_pitch = srcwidth * sizeof(DWORD);

	//  convert pixels
	hr = ConvertPixelsFromTgaFile(fp,pBuffer,num_elements,bRLE,bBW,pixelbits);
	if (FAILED(hr))
		goto ERROR_EXIT;

	//  create an texture
    D3D11_TEXTURE2D_DESC td;
	UINT texWidth = 4, texHeight = 4;
	while(texWidth < srcwidth)
		texWidth <<= 1;
	while(texHeight < srcheight)
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

	SAFE_RELEASE(pDevice);

	//  copy bits
	hr = CopyTgaSourceToTexture2D(pContext,pOutput,pBuffer,srcwidth,srcheight,row_pitch, src_descriptor,dwFillmode);
	if (FAILED(hr))
		goto ERROR_EXIT;
	if (dwFillmode == FillMode::None){
		*pSrcWidth = srcwidth;
		*pSrcHeight = srcheight;
	}else{
		*pSrcWidth = texWidth;
		*pSrcHeight = texHeight;
	}
	*ppTexture = pOutput;
	pOutput = NULL;

ERROR_EXIT:
	if (fp != NULL)
		fclose(fp);

	SAFE_RELEASE(pOutput);
	SAFE_RELEASE(pDevice);
	SAFE_DELETE_ARRAY(pBuffer);
	
	return hr;
}

//
//  function CopyTgaSourceToTexture2D
//    @param :
//      pContext : Device's context
//      pTexture : The Texture to copy to
//	    pSource  : Source to copy
//      width    : width of source image
//      height   : height of source image
//      flags    : tga flags
//      fillmode : fillmode None / Linear
//
HRESULT CopyTgaSourceToTexture2D(ID3D11DeviceContext *pContext, ID3D11Texture2D *pTexture, DWORD *pSource, UINT width, UINT height, UINT pitch, BYTE flags, FillMode fillmode){
	HRESULT hr = S_OK;
	D3D11_TEXTURE2D_DESC td;
	pTexture->GetDesc(&td);
	if (fillmode != FillMode::None && (width != td.Width || height != td.Height)){
		hr = StretchCopyTgaSourceToTexture2D(pContext,pTexture,pSource,width,height,pitch,flags);
	}else{
		D3D11_MAPPED_SUBRESOURCE hMappedResource;
		D3D11_TEXTURE2D_DESC td;
		BYTE *pRawSource = (BYTE*)pSource;
		pTexture->GetDesc(&td);

		hr = pContext->Map( 
			pTexture,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&hMappedResource );
		if (SUCCEEDED(hr)){
			//  fill pixels to texture
			BYTE* pBits = (BYTE*)hMappedResource.pData;

			UINT	x, y;
			INT sourceX=0, sourceY=0;
			int diffX=1, diffY=1;
			INT	x2;
			if (flags&0x10){
				diffX = -1;
				sourceX = width-1;
			}
			if (0 == (flags&0x20)){
				sourceY = height - 1;
				diffY = -1;
			}

			for(y=0; y < height; y++ )
			{
				DWORD* pDst32 = (DWORD*)(pBits+(y * hMappedResource.RowPitch));
				x2 = sourceX;
				for(x = 0; x < width; x++ )
				{
					*pDst32++ = *((DWORD*)(pRawSource+(x2*sizeof(DWORD))+(sourceY * pitch)));
					x2 += diffX;
				}
				while(x < td.Width){
					*pDst32++ = 0L;
					++x;
				}
				sourceY += diffY;
			}
			while( y < td.Height){
				DWORD* pDst32 = (DWORD*)(pBits+(y * hMappedResource.RowPitch));
				for(x = 0; x < td.Width; x++ )
				{
					*pDst32++ = 0L;
				}
				++y;
			}
			pContext->Unmap(pTexture,0);
		}
	}
	return hr;
}
//
//  function StretchCopyTgaSourceToTexture2D
//    Note: fill image with linear filter
//    @param :
//      pContext : Device's context
//      pTexture : The Texture to copy to
//	    pSource  : Source to copy
//      width    : width of source image
//      height   : height of source image
//      flags    : tga flags
//
HRESULT StretchCopyTgaSourceToTexture2D(ID3D11DeviceContext *pContext, ID3D11Texture2D *pTexture, DWORD *pSource, UINT width, UINT height, UINT pitch, BYTE flags){
	D3D11_MAPPED_SUBRESOURCE hMappedResource;
	HRESULT hr;
    D3D11_TEXTURE2D_DESC td;
	BYTE *pRawSource = (BYTE*)pSource;

	pTexture->GetDesc(&td);
	DWORD sourceSize;
	sourceSize = pitch * height;

	hr = pContext->Map( 
		pTexture,
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&hMappedResource );
	if (SUCCEEDED(hr)){
		//	fill pixels to texture
		BYTE* pBits = (BYTE*)hMappedResource.pData;
		BYTE* pBits2;
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

		FLOAT offsetX=0, offsetY=0;
		FLOAT diffX=1, diffY=1;
		if (flags&0x10){
			diffX = -1;
			offsetX = (FLOAT)(width-1);
		}
		if (0 == (flags&0x20)){
			offsetY = FLOAT(height - 1);
			diffY = -1;
		}

		for (ty = 0; ty < td.Height ; ++ty){
			pBits2 = pBits;
			sy = (FLOAT)ty * height * rTexHeight;
			sy = sy * diffY + offsetY;
			iSy1 = max(0,(INT)sy);
			iSy2 = (iSy1 + 1)%height;
			u1 = sy - (FLOAT)iSy1;
			for (tx = 0; tx < td.Width ; ++tx){
				sx = (FLOAT)tx * width * rTexWidth;
				sx = sx * diffX + offsetX;
				iSx1 = max(0,(INT)sx);
				iSx2 = (iSx1 + 1)%width;
				u0 = sx - (FLOAT)iSx1;
				//  linear filter
				pixel[0] = *(DWORD*)(pRawSource+(pitch*iSy1+4*iSx1));
				pixel[1] = *(DWORD*)(pRawSource+(pitch*iSy1+4*iSx2));
				pixel[2] = *(DWORD*)(pRawSource+(pitch*iSy2+4*iSx1));
				pixel[3] = *(DWORD*)(pRawSource+(pitch*iSy2+4*iSx2));
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
	return hr;
}

HRESULT CreateD2D1BitmapFromTgaFile(ID2D1RenderTarget *pRenderTarget,TCHAR *pFilename, ID2D1Bitmap **ppBitmap){
	FILE *fp = NULL;
	errno_t err;
	unsigned char header[256];
	DWORD srcwidth,srcheight;
	int pixelbits, row_pitch;
	BOOL	bRLE = false, bBW = false;
	DWORD	*pBuffer = NULL;
	int		num_elements = 0;
	ID2D1Bitmap *pOutput = NULL;
	HRESULT	hr = E_FAIL;
	int idlength;
	BYTE src_descriptor;

	err = _tfopen_s(&fp,pFilename,_T("rb"));

	if (fp== NULL)
		goto ERROR_EXIT;

	//  read the header;
	fread_s(header,sizeof(header),1,1,fp);
	idlength = ((int)header[0]) & 255;
	if (idlength == 0)
		idlength = 18;
	fread_s(header+1,sizeof(header)-1,idlength-1,1,fp);

	hr = DecodeTgaHeader(header,&pixelbits,&bRLE,&bBW,&srcwidth,&srcheight, &src_descriptor);
	if (FAILED(hr)){
		goto ERROR_EXIT;
	}

	num_elements = srcwidth * srcheight;
	pBuffer = new DWORD[num_elements];
	row_pitch = srcwidth * sizeof(DWORD);

	//  convert pixels
	hr = ConvertPixelsFromTgaFile(fp,pBuffer,num_elements,bRLE,bBW,pixelbits);
	if (FAILED(hr))
		goto ERROR_EXIT;
	D2D1_SIZE_U bitmapSize;
	D2D1_BITMAP_PROPERTIES bitmapProperties;

	//	flip x or y
	BYTE flag = src_descriptor ^ 0x20;
	if (flag & 0x30){
		DWORD *pBuffer2 = new DWORD[num_elements];
		DWORD *pdwSource;
		DWORD *pdwDest;

		int diffX = 1, diffY = 1;
		int sourceX = 0, sourceY = 0;
		if (src_descriptor&0x10){
			diffX = -1;						//  flip horizontally
			sourceX = srcwidth-1;
		}
		if (0 == (src_descriptor&0x20)){
			sourceY = srcheight - 1;		//  flip upside down
			diffY = -1;
		}
		for (int y = 0; y < (int)srcheight ; ++y){
			pdwSource = (DWORD*)(((BYTE*)pBuffer) + sourceY * row_pitch);
			pdwSource += sourceX;
			pdwDest   = (DWORD*)(((BYTE*)pBuffer2)+y*row_pitch);
			for (int x = 0; x < (int)srcwidth ; ++x){
				*pdwDest++ = *pdwSource;
				pdwSource += diffX;
			}
			sourceY += diffY;
		}
		delete [] pBuffer;
		pBuffer = pBuffer2;
		pBuffer2 = NULL;
	}
	bitmapSize.width = srcwidth;
	bitmapSize.height = srcheight;
	bitmapProperties.dpiX = 96.0f;
	bitmapProperties.dpiY = 96.0f;
	bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	hr = pRenderTarget->CreateBitmap(bitmapSize,pBuffer,row_pitch,&bitmapProperties,&pOutput);

	*ppBitmap = pOutput;
	pOutput = NULL;

ERROR_EXIT:
	if (fp != NULL)
		fclose(fp);

	SAFE_RELEASE(pOutput);
	SAFE_DELETE_ARRAY(pBuffer);
	
	return hr;
}

//
//  Decode TGA header
//	@param:
//		pHeader : (in)17 byte header
//		pPixelBits : (out)bits per pixel
//		pRLE	: (out) is run length encoding
//		pBW     : (out) is black and white
//      pSrcWidth  : (out) width of source image.
//      pSrcHeight : (out) height of source image.
//
HRESULT	DecodeTgaHeader(BYTE *pHeader, int *pPixelBits, BOOL *pRLE, BOOL *pBW, DWORD *pSrcWidth, DWORD *pSrcHeight,BYTE *pFlags){

	BOOL	bRLE = false, bBW = false;
	DWORD srcwidth,srcheight;
	int pixelbits;

	switch(pHeader[2] & 0xff){
	case	0:
	case	1:
	case	9:
		return E_FAIL;
	case	2:
		break;
	case	3:
		bBW = true;
		break;
	case	11:
		bBW = true;
	case	10:
		bRLE = true;
		break;
	}
	if (pHeader[1] != 0){
		//  we don't support color maps
		return E_FAIL;
	}

	//  picture size
	srcwidth  = ((DWORD)(*(unsigned short*)(pHeader+12)))&0xffffL;
	srcheight = ((DWORD)(*(unsigned short*)(pHeader+14)))&0xffffL;

	pixelbits = ((int)(pHeader[16])) & 0xff;
	*pRLE = bRLE;
	*pBW = bBW;
	*pSrcWidth = srcwidth;
	*pSrcHeight = srcheight;
	*pPixelBits = pixelbits;
	*pFlags = pHeader[17];

	return S_OK;
}

//
//  Convert pixels from TGA file
//	@param:
//		fp : FILE pointer
//		pBuffer : (in-out) target buffer
//      numPixels : (in) number of pixels
//      bRLE      : (in) iss run length encoding active
//      bBW       : (in) is black and white
//      numBitsPerPixel : (in) number of bits per pixel
//		flags     : (in) tga's descriptor
//
HRESULT ConvertPixelsFromTgaFile(FILE *fp,DWORD *pBuffer, DWORD numPixels, BOOL bRLE, BOOL bBW, INT numBitsPerPixel){
	DWORD	current = 0;
	int r, g, b, a;
	if (!bRLE){  //  without run length encoding
		DWORD current = 0;
		if (bBW){
			BYTE pixel;
			if (numBitsPerPixel != 8)
				return E_FAIL;
			while(current < numPixels){
				if (1!= fread_s((void*)&pixel,1,1,1,fp))
					return E_FAIL;
				r = pixel & 255;
				b = g = r;
				a = 255;
				pBuffer[current++] = (a << 24) + (r << 16) + (g << 8) + b;
			}
		}else{
			switch(numBitsPerPixel){
			case	8:
				return E_FAIL;
			case	16:
				while(current < numPixels){
					USHORT	pixel;
					if (1!= fread_s((void*)&pixel,2,2,1,fp))
						return E_FAIL;
					r = (((pixel >> 10) & 0x1f)*255)/31;
					g = (((pixel >> 5 ) & 0x1f)*255)/31;
					b = ((pixel & 0x1f) * 255) / 31;
					a = ((pixel >> 15 ) & 0x1) * 255;
					pBuffer[current++] = (a << 24) + (r << 16) + (g << 8) + b;
				}
				break;
			case	24:
				while(current < numPixels){
					DWORD pixel = 0;
					if (1!= fread_s((void*)&pixel,4,3,1,fp))
						return E_FAIL;
					r = (pixel >> 16) & 255;
					g = (pixel >> 8) & 255;
					b = pixel & 255;
					a = 255;
					pBuffer[current++] = (a << 24) + (r << 16) + (g << 8) + b;
				}
				break;
			case	32:
				while(current < numPixels){
					DWORD pixel;
					if (1!= fread_s((void*)&pixel,4,4,1,fp))
						return E_FAIL;
					r = (pixel >> 16) & 255;
					g = (pixel >> 8) & 255;
					b = pixel & 255;
					a = (pixel >> 24) & 255;
					pBuffer[current++] = (a << 24) + (r << 16) + (g << 8) + b;
				}
				break;
			}
		}
	}else{	//  with run length encodings.
		BYTE	byte;
		int repeat;
		DWORD	targetPixel;
		if (bBW){
			BYTE pixel;
			if (numBitsPerPixel != 8)
				return E_FAIL;
			while(current < numPixels){
				if (1!= fread_s((void*)&byte,1,1,1,fp))
					return E_FAIL;
				repeat = byte & 0x7f;
				++repeat;
				if (byte & 0x80){
					if (1!= fread_s((void*)&pixel,1,1,1,fp))
						return E_FAIL;
					r = pixel & 255;
					b = g = r;
					a = 255;
					targetPixel = (a << 24) + (r << 16) + (g << 8) + b;
					for (int i = 0; i < repeat && current < numPixels; ++i){
						pBuffer[current++] = targetPixel;
					}
				}else{
					for (int i = 0; i < repeat && current < numPixels; ++i){
						if (1!= fread_s((void*)&pixel,1,1,1,fp))
							return E_FAIL;
						r = pixel & 255;
						b = g = r;
						a = 255;
						pBuffer[current++] = (a << 24) + (r << 16) + (g << 8) + b;
					}
				}
			}
		}else{
			switch(numBitsPerPixel){
			case	8:
				return E_FAIL;
			case	16:
				while(current < numPixels){
					USHORT	pixel;
					if (1!= fread_s((void*)&byte,1,1,1,fp))
						return E_FAIL;
					repeat = byte & 0x7f;
					++repeat;
					if (byte & 0x80){
						if (1!= fread_s((void*)&pixel,2,2,1,fp))
							return E_FAIL;
						r = (((pixel >> 10) & 0x1f)*255)/31;
						g = (((pixel >> 5 ) & 0x1f)*255)/31;
						b = ((pixel & 0x1f) * 255) / 31;
						a = ((pixel >> 15 ) & 0x1) * 255;
						targetPixel = (a << 24) + (r << 16) + (g << 8) + b;
						for (int i = 0; i < repeat && current < numPixels; ++i){
							pBuffer[current++] = targetPixel;
						}
					}else{
						for (int i = 0; i < repeat && current < numPixels; ++i){
							if (1!= fread_s((void*)&pixel,2,2,1,fp))
								return E_FAIL;
							r = (((pixel >> 10) & 0x1f)*255)/31;
							g = (((pixel >> 5 ) & 0x1f)*255)/31;
							b = ((pixel & 0x1f) * 255) / 31;
							a = ((pixel >> 15 ) & 0x1) * 255;
							pBuffer[current++] = (a << 24) + (r << 16) + (g << 8) + b;
						}
					}
				}
				break;
			case	24:
				while(current < numPixels){
					DWORD pixel = 0;
					if (1!= fread_s((void*)&byte,1,1,1,fp))
						return E_FAIL;
					repeat = byte & 0x7f;
					++repeat;
					if (byte & 0x80){
						if (1!= fread_s((void*)&pixel,4,3,1,fp))
							return E_FAIL;
						r = (pixel >> 16) & 255;
						g = (pixel >> 8) & 255;
						b = pixel & 255;
						a = 255;
						targetPixel = (a << 24) + (r << 16) + (g << 8) + b;
						for (int i = 0; i < repeat && current < numPixels; ++i){
							pBuffer[current++] = targetPixel;
						}
					}else{
						for (int i = 0; i < repeat && current < numPixels; ++i){
							if (1!= fread_s((void*)&pixel,4,3,1,fp))
								return E_FAIL;
							r = (pixel >> 16) & 255;
							g = (pixel >> 8) & 255;
							b = pixel & 255;
							a = 255;
							pBuffer[current++] = (a << 24) + (r << 16) + (g << 8) + b;
						}
					}
				}
				break;
			case	32:
				while(current < numPixels){
					DWORD pixel;
					if (1!= fread_s((void*)&byte,1,1,1,fp))
						return E_FAIL;
					repeat = byte & 0x7f;
					++repeat;
					if (byte & 0x80){
						if (1!= fread_s((void*)&pixel,4,4,1,fp))
							return E_FAIL;
						r = (pixel >> 16) & 255;
						g = (pixel >> 8) & 255;
						b = pixel & 255;
						a = (pixel >> 24) & 255;
						targetPixel = (a << 24) + (r << 16) + (g << 8) + b;
						for (int i = 0; i < repeat && current < numPixels; ++i){
							pBuffer[current++] = targetPixel;
						}
					}else{
						for (int i = 0; i < repeat && current < numPixels; ++i){
							if (1!= fread_s((void*)&pixel,4,4,1,fp))
								return E_FAIL;
							r = (pixel >> 16) & 255;
							g = (pixel >> 8) & 255;
							b = pixel & 255;
							a = (pixel >> 24) & 255;
							pBuffer[current++] = (a << 24) + (r << 16) + (g << 8) + b;
						}
					}
				}
				break;
			}
		}
	}
	return S_OK;
}



