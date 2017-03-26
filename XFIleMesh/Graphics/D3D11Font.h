#pragma once

#include "DeviceDependentObject.h"

#define D3DFONT_BOLD        0x0001
#define D3DFONT_ITALIC      0x0002
#define	D3DFONT_SIZE_IN_PIXELS	0x1000

//  Flags used in rendering (not used now)
#define D3DFONT_FILTERED    0x0004

class ID3D11Font : public IDeviceDependentObject{
public:
	static ID3D11Font *CreateAnkFont(ID3D11DeviceContext *pContext);
	virtual ~ID3D11Font()=0;
	virtual void	Render(ID3D11DeviceContext *pContext) = 0;
	virtual void DrawAnkText(ID3D11DeviceContext *pContext,const TCHAR *pString, DirectX::XMFLOAT4 color ,FLOAT x, FLOAT y) = 0;
	virtual void DrawChar(ID3D11DeviceContext *pContext, char ankCode, DirectX::XMFLOAT4 color, FLOAT *px, FLOAT *py) = 0;
};

