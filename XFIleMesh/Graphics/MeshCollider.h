#pragma once
#include "IMesh.h"
#include "SimpleMesh.h"

struct frameiterator;
struct COLLISIONVERTEX;
struct COLLISIONINFO;
class CMeshCollider : public virtual IMeshCollider,  public virtual ISimpleMesh
{
public:
	CMeshCollider(TCHAR *pFilename);
	virtual ~CMeshCollider(void);

	virtual BOOL	ProbeTheWallSinkDepth(DirectX::XMFLOAT3 *pVec, FLOAT fRadius, DirectX::XMFLOAT3 *pVecNormal, FLOAT *pDepth) override;
	virtual BOOL	ProbeTheWallSinkDepthWithMotion(DirectX::XMFLOAT3 *pVec, FLOAT fRadius, DirectX::XMFLOAT3 *pVecNormal, FLOAT *pDepth) override;
	virtual BOOL	CheckCollisionWithSegment(DirectX::XMFLOAT3 *pVec1, DirectX::XMFLOAT3 *pVec2, FLOAT fRadius) override;
	virtual BOOL	CheckCollisionWithSegment(DirectX::XMFLOAT3 *pVec1, DirectX::XMFLOAT3 *pVec2, FLOAT fErrorCapacity, DirectX::XMFLOAT3 *pVecOut) override;

	virtual BOOL	ProbeTheGroundAltitude(DirectX::XMFLOAT3 *pVec, DirectX::XMFLOAT3 *pBoxMin, DirectX::XMFLOAT3 *pBoxMax, DirectX::XMFLOAT3 *pVecNormal, FLOAT *pAlt, FLOAT *pDist) override;
	virtual BOOL	ProbeTheGroundAltitudeOneSide(DirectX::XMFLOAT3 *pVec, DirectX::XMFLOAT3 *pBoxMin, DirectX::XMFLOAT3 *pBoxMax, DirectX::XMFLOAT3 *pVecNormal, FLOAT *pAlt, FLOAT *pDist) override;
	virtual BOOL	ProbeTheGroundAltitudeVerticallyNearest(DirectX::XMFLOAT3 *pVec, DirectX::XMFLOAT3 *pBoxMin, DirectX::XMFLOAT3 *pBoxMax, DirectX::XMFLOAT3 *pVecNormal, FLOAT *pAlt, FLOAT *pDist)  override;

	//  Inclusion of CSimpleMeshRenderer

	virtual HRESULT RestoreDeviceObjects(ID3D11DeviceContext *pContext) override;
	virtual HRESULT ReleaseDeviceObjects() override;

	virtual BOOL	IsPrepared();
	virtual void	Render(ID3D11DeviceContext *pContext);
	virtual void	SetWorldMatrix(DirectX::XMMATRIX *pMatWorld);
	virtual void	SetViewMatrix(DirectX::XMMATRIX  *pMatView);
	virtual void	SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection);

	virtual void	SetLightDir(DirectX::XMFLOAT3 *pVecDir);
	virtual void	SetLightDiffuse(DirectX::XMFLOAT3 *pVecDiffuse);
	virtual void	SetLightAmbient(DirectX::XMFLOAT3 *pVecAmbient);

	virtual MeshFrame	*GetRootFrame();

	//  keep this 16-byte aligned for XMMATRIX
	inline void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	inline void operator delete(void *p){
		return _mm_free(p);
	}
protected:
	CSimpleMeshRenderer m_smrRenderer;	//  has-a inclusion to avoid multiplex inheritance
	void DeleteObjects();
	void EnflatFrames(MeshFrame *pFrame, frameiterator* pFrames);


	BOOL ProbeTheWallSinkDepthBase(
		DirectX::XMFLOAT3 *pPosition,FLOAT fRadius, DirectX::XMFLOAT3 *pVecNormal,
		FLOAT *pDepth);

	void GetBoundingAABB(DirectX::XMVECTOR *pVecMin, DirectX::XMVECTOR *pVecMax);
	void GetBoundingAABBFloat3(DirectX::XMFLOAT3 *pVecMin, DirectX::XMFLOAT3 *pVecMax);

	void PrepareBoundingAABB();

	DWORD m_dwNumFrames;
	MeshFrame **m_ppMeshFrames;

	DirectX::XMMATRIX m_matWorld;
	DirectX::XMFLOAT3 m_vecMin;
	DirectX::XMFLOAT3 m_vecMax;
};


