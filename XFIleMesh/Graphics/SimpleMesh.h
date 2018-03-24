#pragma once
#include "IMesh.h"
struct MeshFrame;
struct MeshContainer;
struct FrameRendererData;

__declspec(align(16)) class CSimpleMeshRenderer : virtual public ISimpleMesh
{
public:
	CSimpleMeshRenderer();
	virtual ~CSimpleMeshRenderer(void);
	virtual BOOL	IsPrepared();
	virtual HRESULT AdjustVertexFormat();
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext);
	virtual HRESULT ReleaseDeviceObjects();
	
	virtual void	Render(ID3D11DeviceContext *pContext);
	virtual void	SetWorldMatrix(DirectX::XMMATRIX *pMatWorld);
	virtual void	SetViewMatrix(DirectX::XMMATRIX  *pMatView);
	virtual void	SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection);

	virtual void	SetLightDir(DirectX::XMFLOAT3 *pVecDir);
	virtual void	SetLightDiffuse(DirectX::XMFLOAT3 *pVecDiffuse);
	virtual void	SetLightAmbient(DirectX::XMFLOAT3 *pVecAmbient);

	static void CompileShaderCodes();	//	
	static void RemoveShaderCodes();	//	

	virtual MeshFrame	*GetRootFrame() override{
		return m_pFrameRoot;
	}

	virtual MeshFrame *ReplaceRootFrame(MeshFrame *pFrame);

protected:
	void InitFields();
	virtual HRESULT RestoreClassObjects();
	virtual HRESULT ReleaseClassObjects();
	virtual HRESULT RestoreInstanceObjects();
	virtual HRESULT ReleaseInstanceObjects();

	virtual HRESULT CreateFrameRendererDatas();
	virtual HRESULT DeleteFrameRendererDatas();

	virtual void ReleaseFrameRendererClients(FrameRendererData *pFrame);
protected:
	MeshFrame *m_pFrameRoot;

	ID3D11DeviceContext	*m_pDeviceContext;
	ID3D11Device        *m_pDevice;

	static ID3D11VertexShader	*m_pVertexShader;
	static ID3D11PixelShader	*m_pPixelShader;
	static ID3D11PixelShader	*m_pNoTexPixelShader;
	static ID3D11InputLayout	*m_pInputLayout;
	static ID3D11Buffer			*m_pConstantBuffer;
	static INT                  m_iShaderReferenceCount;

	static BYTE  *m_pVertexShaderCode;
	static DWORD m_dwVertexShaderCodeSize;
	static BYTE  *m_pPixelShaderCode;
	static DWORD m_dwPixelShaderCodeSize;
	static BYTE  *m_pNoTexPixelShaderCode;
	static DWORD m_dwNoTexPixelShaderCodeSize;

	ID3D11Buffer		*m_pVertexBuffer;
	ID3D11SamplerState	*m_pTextureSamplerState;

	DirectX::XMMATRIX	m_matProjection;
	DirectX::XMMATRIX	m_matView;
	DirectX::XMMATRIX	m_matWorld;
	FrameRendererData	**m_ppFrameRendererDatas;
	DWORD				m_dwNumFrameRendererDatas;

	DirectX::XMFLOAT3	m_vecLightDir;
	DirectX::XMFLOAT3	m_vecLightDiffuse;
	DirectX::XMFLOAT3	m_vecLightAmbient;

	BOOL				m_bPrepared;
};

__declspec(align(16)) class CSimpleMesh : public virtual CSimpleMeshRenderer
{
public:
	CSimpleMesh(TCHAR *pFilename);
	virtual ~CSimpleMesh(void);

	//  keep this 16-byte aligned for XMMATRIX
	inline void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	inline void operator delete(void *p){
		return _mm_free(p);
	}
protected:
};

struct FrameRendererData{
	MeshFrame		*pFrame;
	MeshContainer	*pMeshContainer;
	DirectX::XMFLOAT3	m_vecObjCenter;
	ID3D11Buffer	*m_pVertexBuffer;
	ID3D11Buffer	*m_pIndexBuffer;
};

//
//	Ordering Table
//
__declspec(align(16)) struct OrderingTableItem{
	DirectX::XMVECTOR	m_vecPolygonCenter;	//	not transformed
	FLOAT	m_fZ;	//	transformed z
	int	m_iIndex;
	int m_iMatID;
	void *operator new(size_t size) {
		return _mm_malloc(size, 16);
	}
	void *operator new[](size_t size) {
		return _mm_malloc(size, 16);
	}
		void operator delete(void *p) {
		return _mm_free(p);
	}
};

struct FrameRendererDataOT : public FrameRendererData{
	OrderingTableItem *m_pItem;
	int m_iNumItem;
};

__declspec(align(16)) class CSimpleOTMeshRenderer : virtual public CSimpleMeshRenderer
{
public:
	CSimpleOTMeshRenderer();
	virtual ~CSimpleOTMeshRenderer(void);

	virtual void	Render(ID3D11DeviceContext *pContext);

protected:

	virtual HRESULT CreateFrameRendererDatas();
	virtual void ReleaseFrameRendererClients(FrameRendererData *pFrame) override;
	virtual void ZSortInViewSpace();

	//  keep this 16-byte aligned for XMMATRIX
	inline void *operator new(size_t size) {
		return _mm_malloc(size, 16);
	}
	inline void operator delete(void *p) {
		return _mm_free(p);
	}
};

__declspec(align(16)) class CSimpleOTMesh : public virtual CSimpleOTMeshRenderer
{
public:
	CSimpleOTMesh(TCHAR *pFilename);
	virtual ~CSimpleOTMesh(void);

	//  keep this 16-byte aligned for XMMATRIX
	inline void *operator new(size_t size) {
		return _mm_malloc(size, 16);
	}
	inline void operator delete(void *p) {
		return _mm_free(p);
	}
protected:
};


