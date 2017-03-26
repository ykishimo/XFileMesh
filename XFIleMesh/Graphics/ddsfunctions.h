#pragma once

//
//prototypes 
//
HRESULT CreateTextureFromDdsFile(ID3D11DeviceContext *pContext, TCHAR *pFilename, ID3D11Texture2D **ppTexture, DWORD *pSrcWidth, DWORD *pSrcHeight);

#define _DDS_SUPPORTED_
