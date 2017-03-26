#include "stdafx.h"
#include <crtdbg.h>
#include <stdio.h>
#include <D3D11.h>
#include <directxmath.h>
#include <D2D1.h>
#include <list>
#include "XFileParser.h"

#include "Mesh.h"
#include "Animation.h"
#include "SimpleMesh.h"
#include "Animator.h"
#include "AnimatedMesh.h"
#include "D3D11TextureDB.h"

#include "D3DContext.h"


//
//  ctor for simple mesh renderer
//
CAnimatedMesh::CAnimatedMesh(TCHAR *pFilename)
{
	//  Initialize optional fields
	m_ppAnimationSets = NULL;
	m_pAnimator = NULL;
	m_dwNumAnimations = 0;

	//m_pRenderer = NULL;
	//m_pRenderer = new CSimpleMeshRenderer();

	MeshFrame *pFrame = NULL;
	if (SUCCEEDED( CXFileParser::CreateMesh(pFilename,&pFrame,&m_ppAnimationSets,&m_dwNumAnimations))){

		//  Create animator
		m_pAnimator = IAnimator::CreateInstance(pFrame);
		m_pAnimator->AddAnimationSets(m_ppAnimationSets,m_dwNumAnimations);

		m_pAnimator->SetTrackAnimation(0,0,1.0,true);

		MeshFrame *pOld = m_pRenderer.ReplaceRootFrame(pFrame);
		SAFE_DELETE(pOld)
	}
	if (FAILED(m_pRenderer.AdjustVertexFormat())){
		DeleteObjects();
	}
}
CAnimatedMesh::~CAnimatedMesh(void)
{
	DeleteObjects();
}

void CAnimatedMesh::DeleteObjects(){
	if (m_ppAnimationSets!=NULL){
		//  Ignore animations
		for (DWORD i = 0; i < m_dwNumAnimations ; ++i){
			SAFE_DELETE(m_ppAnimationSets[i]);
		}
		delete m_ppAnimationSets;
		m_ppAnimationSets = NULL;
	}
	SAFE_DELETE(m_pAnimator);
}
//
//	Update animations
//
void	CAnimatedMesh::Update(DOUBLE timeElapsed, DirectX::XMMATRIX *pMatWorld){
	m_pAnimator->AdvanceTime(timeElapsed);
	m_pAnimator->AdjustAnimation();
	AdjustFrameMatrices(m_pRenderer.GetRootFrame(),pMatWorld);
}

void	CAnimatedMesh::AdjustPose(DirectX::XMMATRIX *pMatWorld){
	AdjustFrameMatrices(m_pRenderer.GetRootFrame(),pMatWorld);
}

//  Inclusion to Mesh Renderer
#if 1
BOOL	CAnimatedMesh::IsPrepared(){
	return m_pRenderer.IsPrepared();
}
void	CAnimatedMesh::Render(ID3D11DeviceContext *pContext){
	m_pRenderer.Render(pContext);
}
void	CAnimatedMesh::SetWorldMatrix(DirectX::XMMATRIX *pMatWorld){
	m_pRenderer.SetWorldMatrix(pMatWorld);
}
void	CAnimatedMesh::SetViewMatrix(DirectX::XMMATRIX  *pMatView){
	m_pRenderer.SetViewMatrix(pMatView);
}
void	CAnimatedMesh::SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection){
	m_pRenderer.SetProjectionMatrix(pMatProjection);
}

void	CAnimatedMesh::SetLightDir(DirectX::XMFLOAT3 *pVecDir){
	m_pRenderer.SetLightDir(pVecDir);
}
void	CAnimatedMesh::SetLightDiffuse(DirectX::XMFLOAT3 *pVecDiffuse){
	m_pRenderer.SetLightDiffuse(pVecDiffuse);
}
void	CAnimatedMesh::SetLightAmbient(DirectX::XMFLOAT3 *pVecAmbient){
	m_pRenderer.SetLightAmbient(pVecAmbient);
}

MeshFrame	*CAnimatedMesh::GetRootFrame(){
	return	m_pRenderer.GetRootFrame();
}

HRESULT CAnimatedMesh::RestoreDeviceObjects(ID3D11DeviceContext *pContext){
	return m_pRenderer.RestoreDeviceObjects(pContext);
}

HRESULT CAnimatedMesh::ReleaseDeviceObjects(){
	return m_pRenderer.ReleaseDeviceObjects();
}
#endif

