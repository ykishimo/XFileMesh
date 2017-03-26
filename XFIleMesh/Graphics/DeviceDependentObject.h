//
//	Uniformed interface of directx's device dependent objects
//
#pragma once

struct ID3D11DeviceContext;

class IDeviceDependentObject{
public:
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext) = 0;
	virtual HRESULT ReleaseDeviceObjects() = 0;
};
