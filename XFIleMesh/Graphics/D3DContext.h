//
// @file D3DContext.h
// @brief declaration of the interface ID3DContext
//
#pragma once

class ID3DContext
{
public:
	virtual ~ID3DContext(void)=0;

	//  Interfaces
	virtual ID3D11Device *GetDevice() = 0;
	virtual ID3D11DeviceContext *GetDeviceContext() = 0;
	virtual IDXGISwapChain *GetSwapChain() = 0;
	virtual ID3D11RenderTargetView *GetRenderTargetView() = 0;
	virtual ID3D11DepthStencilView *GetDepthStencilView() = 0;
	virtual ID3D11Texture2D *GetBackBuffer() = 0;
	virtual ID3D11ShaderResourceView	*GetRenderTargetShaderResourceView() = 0;
	virtual ID3D11Texture2D				*GetpDepthStencilTexture() = 0;
	virtual ID3D11ShaderResourceView	*GetDepthStencilShaderResourceView() = 0;
	virtual const TCHAR *GetDriverTypeText() = 0;
	virtual const TCHAR *GetFeatureLevelText() = 0;
	virtual void  GetCurrentViewport(D3D11_VIEWPORT *pViewport) = 0;

	virtual void ClearRenderTargetView(float ClearColor[4]) = 0;
	virtual void ClearDepthStencilView(UINT ClearFlags, FLOAT Depth, UINT8 Stencil) = 0;
	virtual HRESULT Present(UINT SyncInterval, UINT Flags) = 0;

	virtual void SetDepthEnable(BOOL bDepth, BOOL bWrite) = 0;
};

HRESULT CreateD3DContext(ID3DContext **ppContext, HWND hWnd, BOOL bWindowed=true);

//  Macros
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#undef SAFE_RELEASE

#define	SAFE_DELETE(o)	{  if (o){ delete (o);  o = NULL; }  }
#define	SAFE_DELETE_ARRAY(o)	{  if (o){ delete [] (o);  o = NULL; }  }
#define SAFE_RELEASE(o) if(o){  (o)->Release(); o = NULL; }

//  remove // below to compile and save precompiled shaders.
//#define COMPILE_AND_SAVE_SHADER

#ifdef COMPILE_AND_SAVE_SHADER
#include <d3dcompiler.h>
HRESULT CompileShaderFromMemory(BYTE* pCode, DWORD dwSize, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob **ppBlobOut);
extern void SaveShaderCode(BYTE *pData, DWORD dwSize, TCHAR *pFilename, char *pFuncName);
#endif
