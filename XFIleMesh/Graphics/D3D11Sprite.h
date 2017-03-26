#pragma once

#include "DeviceDependentObject.h"

//  Sprite インタフェース
class ID3D11Sprite : public IDeviceDependentObject
{
public:
//	static ID3D11Sprite *CreateSprite();
	static ID3D11Sprite *CreateInstance();
	virtual ~ID3D11Sprite() = 0;  //  pure virtual destructor
	virtual void	SetTexture(INT no, TCHAR *pFilename)=0;
	virtual void    Render(ID3D11DeviceContext *pContext, FLOAT x, FLOAT y, FLOAT w, FLOAT h, INT no)=0;
	virtual void    Render(ID3D11DeviceContext *pContext, FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT tx, FLOAT ty, FLOAT tw, FLOAT th, INT no)=0;
	virtual void	SetDiffuse(FLOAT r, FLOAT g, FLOAT b, FLOAT a)=0;
	virtual INT		GetNumTextures()=0;
};
