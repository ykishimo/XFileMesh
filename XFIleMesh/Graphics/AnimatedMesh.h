#pragma once
#include "IMesh.h"
#include "SimpleMesh.h"

struct AnimationSet;
class  IAnimator;

class CAnimatedMesh : public IAnimatedMesh
{
public:
	CAnimatedMesh(TCHAR *pFilename);
	virtual ~CAnimatedMesh(void);
	IAnimator *GetAnimator() override{
		return	m_pAnimator;
	}

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

	virtual void	Update(DOUBLE timeElapsed,DirectX::XMMATRIX *pMatWorld = NULL) override;
	virtual void    AdjustPose(DirectX::XMMATRIX *pMatWorld = NULL) override;

	//  keep this 16-byte aligned for XMMATRIX
	inline void *operator new(size_t size){
		return _mm_malloc(size,16);
	}
	inline void operator delete(void *p){
		return _mm_free(p);
	}
protected:
	CSimpleMeshRenderer m_pRenderer;	//  has-a inclusion to avoid multiplex inheritance
	void DeleteObjects();
	AnimationSet **m_ppAnimationSets;
	IAnimator    *m_pAnimator;
	DWORD		 m_dwNumAnimations;
};

