#pragma once
#include "DeviceDependentObject.h"

class IAnimator;
struct MeshFrame;

//  Simple mesh
class ISimpleMesh : public IDeviceDependentObject
{
public:
	static ISimpleMesh *CreateInstance(TCHAR *pFilename);
	virtual ~ISimpleMesh(void) = 0;

	virtual BOOL	IsPrepared() = 0;
	virtual void	Render(ID3D11DeviceContext *pContext) = 0;
	virtual void	SetWorldMatrix(DirectX::XMMATRIX *pMatWorld) = 0;
	virtual void	SetViewMatrix(DirectX::XMMATRIX  *pMatView) = 0;
	virtual void	SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection) = 0;

	virtual void	SetLightDir(DirectX::XMFLOAT3 *pVecDir) = 0;
	virtual void	SetLightDiffuse(DirectX::XMFLOAT3 *pVecDiffuse) = 0;
	virtual void	SetLightAmbient(DirectX::XMFLOAT3 *pVecAmbient) = 0;

	virtual MeshFrame	*GetRootFrame() = 0;
};

class IAnimated
{
public:
	virtual void	Update(DOUBLE timeElapsed,DirectX::XMMATRIX *pMatWorld = NULL) = 0;
	virtual void    AdjustPose(DirectX::XMMATRIX *pMatWorld = NULL) = 0;

	virtual IAnimator *GetAnimator() = 0;
};

//  Animated mesh
class IAnimatedMesh : public virtual ISimpleMesh, public virtual IAnimated
{
public:
	static IAnimatedMesh *CreateInstance(TCHAR *pFilename);
	virtual ~IAnimatedMesh(void) = 0;
#if 0
	virtual BOOL	IsPrepared() = 0;
	virtual void	Render(ID3D11DeviceContext *pContext) = 0;
	virtual void	SetWorldMatrix(DirectX::XMMATRIX *pMatWorld) = 0;
	virtual void	SetViewMatrix(DirectX::XMMATRIX  *pMatView) = 0;
	virtual void	SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection) = 0;

	virtual void	SetLightDir(DirectX::XMFLOAT3 *pVecDir) = 0;
	virtual void	SetLightDiffuse(DirectX::XMFLOAT3 *pVecDiffuse) = 0;
	virtual void	SetLightAmbient(DirectX::XMFLOAT3 *pVecAmbient) = 0;

	virtual void	Update(DOUBLE timeElapsed,DirectX::XMMATRIX *pMatWorld = NULL) = 0;
	virtual void    AdjustPose(DirectX::XMMATRIX *pMatWorld = NULL) = 0;

	virtual IAnimator *GetAnimator() = 0;
#endif
};

//  Mesh collider (wall)
//
class IMeshCollider : public virtual ISimpleMesh{
public:
	virtual ~IMeshCollider() = 0;
	static IMeshCollider *CreateInstance(TCHAR *pFilename);
	virtual BOOL	ProbeTheWallSinkDepth(DirectX::XMFLOAT3 *pVec, FLOAT fRadius, DirectX::XMFLOAT3 *pVecNormal, FLOAT *pDepth) = 0;
	virtual BOOL	ProbeTheWallSinkDepthWithMotion(DirectX::XMFLOAT3 *pVec, FLOAT fRadius, DirectX::XMFLOAT3 *pVecNormal, FLOAT *pDepth) = 0;
	virtual BOOL	CheckCollisionWithSegment(DirectX::XMFLOAT3 *pVec1, DirectX::XMFLOAT3 *pVec2, FLOAT fRadius) = 0;
	virtual BOOL	CheckCollisionWithSegment(DirectX::XMFLOAT3 *pVec1, DirectX::XMFLOAT3 *pVec2, FLOAT fErrorCapacity, DirectX::XMFLOAT3 *pVecOut) = 0;

	virtual BOOL	ProbeTheGroundAltitude(DirectX::XMFLOAT3 *pVec, DirectX::XMFLOAT3 *pBoxMin, DirectX::XMFLOAT3 *pBoxMax, DirectX::XMFLOAT3 *pVecNormal, FLOAT *pAlt, FLOAT *pDist) = 0;
	virtual BOOL	ProbeTheGroundAltitudeOneSide(DirectX::XMFLOAT3 *pVec, DirectX::XMFLOAT3 *pBoxMin, DirectX::XMFLOAT3 *pBoxMax, DirectX::XMFLOAT3 *pVecNormal, FLOAT *pAlt, FLOAT *pDist) = 0;
	virtual BOOL	ProbeTheGroundAltitudeVerticallyNearest(DirectX::XMFLOAT3 *pVec, DirectX::XMFLOAT3 *pBoxMin, DirectX::XMFLOAT3 *pBoxMax, DirectX::XMFLOAT3 *pVecNormal, FLOAT *pAlt, FLOAT *pDist) = 0;
};

//  Skinned mesh
class ISkinnedMesh : public IAnimatedMesh
{
public:
	static ISkinnedMesh *CreateInstance(TCHAR *pFilename);
	virtual ~ISkinnedMesh(void) = 0;
	virtual BOOL	IsPrepared() = 0;
#if 1	
	virtual void	Render(ID3D11DeviceContext *pContext) = 0;
	virtual void	SetWorldMatrix(DirectX::XMMATRIX *pMatWorld) = 0;
	virtual void	SetViewMatrix(DirectX::XMMATRIX  *pMatView) = 0;
	virtual void	SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection) = 0;

	virtual void	SetLightDir(DirectX::XMFLOAT3 *pVecDir) = 0;
	virtual void	SetLightDiffuse(DirectX::XMFLOAT3 *pVecDiffuse) = 0;
	virtual void	SetLightAmbient(DirectX::XMFLOAT3 *pVecAmbient) = 0;

	virtual void	Update(DOUBLE timeElapsed,DirectX::XMMATRIX *pMatWorld = NULL) = 0;
	virtual void    AdjustPose(DirectX::XMMATRIX *pMatWorld = NULL) = 0;

	virtual MeshFrame	*GetRootFrame() = 0;

	virtual IAnimator *GetAnimator() = 0;
#endif

};


//  Bone renderer
class IBoneRenderer : public IDeviceDependentObject
{
public:
	static IBoneRenderer *CreateInstance(MeshFrame *pFrameRoot);
	virtual ~IBoneRenderer(void) = 0;
	
	virtual void	Render(ID3D11DeviceContext *pContext) = 0;
	virtual void	SetWorldMatrix(DirectX::XMMATRIX *pMatWorld) = 0;
	virtual void	SetViewMatrix(DirectX::XMMATRIX  *pMatView) = 0;
	virtual void	SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection) = 0;

	virtual MeshFrame	*GetRootFrame() = 0;
};


//
//	class Animator
//
struct AnimationSet;

class IAnimator{
public:
//	static IAnimator *CreateAnimator(MeshFrame *pRoot);
	static IAnimator *CreateInstance(MeshFrame *pRoot);
	virtual ~IAnimator() = 0;
	virtual HRESULT	AddAnimationSets(AnimationSet **ppAnimationSet, INT num) = 0;
	virtual void	SetTrackAnimation(INT trackNo, INT animNo, DOUBLE weight, BOOL loop) = 0;
	virtual void	SetTrackWeight(INT trackNo, DOUBLE weight) = 0;
	virtual void	SetTrackSpeed(INT trackNo, DOUBLE speed) = 0;
	virtual DOUBLE	SetTrackTime(INT trackNo, DOUBLE time) = 0;
	virtual void	AdvanceTime(DOUBLE time) = 0;
	virtual void	AdjustAnimation() = 0;	//	Finalize animation and apply to mesh
	virtual INT		GetNumAnimations() = 0;
	virtual BOOL	GetTrackDuration(INT trackNo, DOUBLE *pDuration) = 0;
	virtual BOOL	GetAnimationDuration(INT animNo, DOUBLE *pDuration, DOUBLE speed = 1.0) = 0;
	virtual void	SetLastFrameLength(INT bSameAsLastFrame) = 0;
};


//
//	class MeshFrameTracker
//
class IMeshFrameTracker{
public:
	static IMeshFrameTracker *CreateInstance(MeshFrame *pRoot, CHAR *pName);	//  never accept unicode
	virtual ~IMeshFrameTracker() = 0;
	virtual void GetTransform(DirectX::XMMATRIX *pMatrix) = 0;
};
