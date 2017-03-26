#pragma once

class ID3D11PointSprite;
class IBurnUp{
public:
	static IBurnUp*Create(DirectX::XMFLOAT3 *pVecPos);
	virtual ~IBurnUp(void)=0;
    virtual BOOL Update(FLOAT timeElapsed) = 0;
    virtual void Render(ID3D11DeviceContext *pContext, ID3D11PointSprite *pSprite) = 0;
};
