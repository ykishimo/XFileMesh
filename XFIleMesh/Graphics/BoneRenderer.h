#pragma once

#include "IMesh.h"

struct MeshFrame;

class CBoneRenderer : public IBoneRenderer
{
public:
	CBoneRenderer(MeshFrame *pFrameRoot);
	virtual ~CBoneRenderer(void);

	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext);
	virtual HRESULT ReleaseDeviceObjects();

	virtual void	SetWorldMatrix(DirectX::XMMATRIX *pMatWorld);
	virtual void	SetViewMatrix(DirectX::XMMATRIX  *pMatView);
	virtual void	SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection);

	virtual void Render(ID3D11DeviceContext *pContext);
	
	virtual MeshFrame	*GetRootFrame() override{
		return m_pFrameRoot;
	}

	//  keep this 16-byte aligned for XMMATRIX
	inline void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	inline void operator delete(void *p){
		return _mm_free(p);
	}
	static void CompileShaderCodes();	//	
	static void RemoveShaderCodes();	//	

protected:
	void RenderSub(MeshFrame *pFrame, DirectX::XMMATRIX *pParent);
	void InitFields();
	//HRESULT PrepareShaders();

	virtual HRESULT RestoreClassObjects();
	virtual HRESULT ReleaseClassObjects();
	virtual HRESULT RestoreInstanceObjects();
	virtual HRESULT ReleaseInstanceObjects();

	MeshFrame *m_pFrameRoot;

	ID3D11DeviceContext	*m_pDeviceContext;
	ID3D11Device        *m_pDevice;

	ID3D11Buffer		*m_pVertexBuffer;

	DirectX::XMFLOAT3	m_pVertexSysMem[2];
	DirectX::XMMATRIX	m_matProjection;
	DirectX::XMMATRIX	m_matView;
	DirectX::XMMATRIX	m_matWorld;

	//  static fields
	static ID3D11VertexShader	*m_pVertexShader;
	static ID3D11PixelShader	*m_pPixelShader;
	static ID3D11InputLayout	*m_pInputLayout;
	static ID3D11Buffer		    *m_pConstantBuffer;

	static BYTE  *m_pVertexShaderCode;
	static DWORD m_dwVertexShaderCodeSize;
	static BYTE  *m_pPixelShaderCode;
	static DWORD m_dwPixelShaderCodeSize;
	static INT	 m_iShaderReferenceCount;
};

