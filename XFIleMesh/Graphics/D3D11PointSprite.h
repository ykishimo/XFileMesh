//
// @file D3D11PointSprite.h
// @brief declaration of ID3D11PointSprite
//
#pragma once

#include "DeviceDependentObject.h"

//  Inteface of Point Sprite
class ID3D11PointSprite : public IDeviceDependentObject
{
public:
	static ID3D11PointSprite *CreateInstance();
	virtual ~ID3D11PointSprite()=0;  //  純粋仮想デストラクタ
	virtual void SetTexture(INT no, TCHAR *pFilename) = 0;
	virtual void Render(ID3D11DeviceContext *pContext, DirectX::XMFLOAT3 *position, FLOAT psize, DirectX::XMFLOAT4 *pcolor, INT texNo = 0) = 0;
	virtual void Flush(ID3D11DeviceContext *pContext) = 0;
	virtual INT	GetNumTextures() = 0;

	virtual void	SetWorldMatrix(DirectX::XMMATRIX *pMatWorld) = 0;
	virtual void	SetViewMatrix(DirectX::XMMATRIX  *pMatView) = 0;
	virtual void	SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection) = 0;

};

