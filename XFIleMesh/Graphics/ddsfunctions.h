#pragma once

//
//prototypes 
//
HRESULT CreateTextureFromDdsFile(ID3D11DeviceContext *pContext, TCHAR *pFilename, ID3D11Texture2D **ppTexture, DWORD *pSrcWidth, DWORD *pSrcHeight);
HRESULT CreateD2D1BitmapFromDdsFile(ID2D1RenderTarget *pRenderTarget,TCHAR *pFilename, ID2D1Bitmap **ppBitmap);

#define _DDS_SUPPORTED_
