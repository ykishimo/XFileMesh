#pragma once

//  Prototypes
HRESULT CreateTextureFromTgaFile(ID3D11DeviceContext *pContext, TCHAR *pFilename, ID3D11Texture2D **ppTexture, DWORD *pSrcWidth, DWORD *pSrcHeight, FillMode dwFillMode = FillMode::None);
HRESULT CreateD2D1BitmapFromTgaFile(ID2D1RenderTarget *pRenderTarget,TCHAR *pFilename, ID2D1Bitmap **ppBitmap);

#define _TARGA_SUPPORTED_
