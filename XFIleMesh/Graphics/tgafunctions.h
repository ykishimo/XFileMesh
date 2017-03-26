#pragma once

//  Prototypes
HRESULT CreateTextureFromTgaFile(ID3D11DeviceContext *pContext, TCHAR *pFilename, ID3D11Texture2D **ppTexture, DWORD *pSrcWidth, DWORD *pSrcHeight, FillMode dwFillMode = FillMode::None);

#define _TARGA_SUPPORTED_
