//
//	@file SkinnedMesh.h
//	@brief an declaration of class CSkinnedMesh
//
#pragma once
#include "IMesh.h"

struct MeshFrame;
struct MeshContainer;
struct FrameRendererData;
struct AnimationSet;
class  IAnimator;

//
//	@class CSkinnedMesh
//	@note an implementation of ISkinnedMesh
//
class CSkinnedMesh : public ISkinnedMesh
{
public:
	CSkinnedMesh(TCHAR *pFilename);
	virtual ~CSkinnedMesh(void);
	virtual BOOL	IsPrepared() override;
	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext) override;
	virtual HRESULT ReleaseDeviceObjects() override;
	
	virtual void	Render(ID3D11DeviceContext *pContext) override;
	virtual void	SetWorldMatrix(DirectX::XMMATRIX *pMatWorld) override;
	virtual void	SetViewMatrix(DirectX::XMMATRIX  *pMatView) override;
	virtual void	SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection) override;

	virtual void	SetLightDir(DirectX::XMFLOAT3 *pVecDir) override;
	virtual void	SetLightDiffuse(DirectX::XMFLOAT3 *pVecDiffuse) override;
	virtual void	SetLightAmbient(DirectX::XMFLOAT3 *pVecAmbient) override;

	virtual void	Update(DOUBLE timeElapsed,DirectX::XMMATRIX *pMatWorld = NULL) override;
	virtual void    AdjustPose(DirectX::XMMATRIX *pMatWorld = NULL) override;

	static void CompileShaderCodes();	//	
	static void RemoveShaderCodes();	//	

	MeshFrame	*GetRootFrame() override{
		return m_pFrameRoot;
	}

	IAnimator *GetAnimator() override{
		return	m_pAnimator;
	}


	//  keep this 16-byte aligned for XMMATRIX
	inline void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	inline void operator delete(void *p){
		return _mm_free(p);
	}
private:
	void InitFields();
	virtual HRESULT RestoreClassObjects();
	virtual HRESULT ReleaseClassObjects();
	virtual HRESULT RestoreInstanceObjects();
	virtual HRESULT ReleaseInstanceObjects();
protected:
	MeshFrame *m_pFrameRoot;
	AnimationSet **m_ppAnimationSets;
	IAnimator    *m_pAnimator;
	DWORD		 m_dwNumAnimations;
	ID3D11DeviceContext	*m_pDeviceContext;
	ID3D11Device        *m_pDevice;

	static ID3D11VertexShader	*m_pVertexShader;
	static ID3D11PixelShader	*m_pPixelShader;
	static ID3D11PixelShader	*m_pNoTexPixelShader;
	static ID3D11InputLayout	*m_pInputLayout;
	static ID3D11Buffer			*m_pConstantBuffer;
	static INT					m_iShaderReferenceCount;

	static BYTE  *m_pVertexShaderCode;
	static DWORD m_dwVertexShaderCodeSize;
	static BYTE  *m_pPixelShaderCode;
	static DWORD m_dwPixelShaderCodeSize;
	static BYTE  *m_pNoTexPixelShaderCode;
	static DWORD m_dwNoTexPixelShaderCodeSize;

	ID3D11Buffer		*m_pVertexBuffer;
	ID3D11SamplerState	*m_pTextureSamplerState;

	DWORD				m_dwFVF;
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

