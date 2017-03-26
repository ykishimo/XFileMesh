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

	//m_smrRenderer = NULL;
	//m_smrRenderer = new CSimpleMeshRenderer();

	MeshFrame *pFrame = NULL;
	if (SUCCEEDED( CXFileParser::CreateMesh(pFilename,&pFrame,&m_ppAnimationSets,&m_dwNumAnimations))){

		//  Create animator
		m_pAnimator = IAnimator::CreateInstance(pFrame);
		m_pAnimator->AddAnimationSets(m_ppAnimationSets,m_dwNumAnimations);

		m_pAnimator->SetTrackAnimation(0,0,1.0,true);

		MeshFrame *pOld = m_smrRenderer.ReplaceRootFrame(pFrame);
		SAFE_DELETE(pOld)
	}
	if (FAILED(m_smrRenderer.AdjustVertexFormat())){
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
	AdjustFrameMatrices(m_smrRenderer.GetRootFrame(),pMatWorld);
}

void	CAnimatedMesh::AdjustPose(DirectX::XMMATRIX *pMatWorld){
	AdjustFrameMatrices(m_smrRenderer.GetRootFrame(),pMatWorld);
}

//  Inclusion to Mesh Renderer
#if 1
BOOL	CAnimatedMesh::IsPrepared(){
	return m_smrRenderer.IsPrepared();
}
void	CAnimatedMesh::Render(ID3D11DeviceContext *pContext){
	m_smrRenderer.Render(pContext);
}
void	CAnimatedMesh::SetWorldMatrix(DirectX::XMMATRIX *pMatWorld){
	m_smrRenderer.SetWorldMatrix(pMatWorld);
}
void	CAnimatedMesh::SetViewMatrix(DirectX::XMMATRIX  *pMatView){
	m_smrRenderer.SetViewMatrix(pMatView);
}
void	CAnimatedMesh::SetProjectionMatrix(DirectX::XMMATRIX *pMatProjection){
	m_smrRenderer.SetProjectionMatrix(pMatProjection);
}

void	CAnimatedMesh::SetLightDir(DirectX::XMFLOAT3 *pVecDir){
	m_smrRenderer.SetLightDir(pVecDir);
}
void	CAnimatedMesh::SetLightDiffuse(DirectX::XMFLOAT3 *pVecDiffuse){
	m_smrRenderer.SetLightDiffuse(pVecDiffuse);
}
void	CAnimatedMesh::SetLightAmbient(DirectX::XMFLOAT3 *pVecAmbient){
	m_smrRenderer.SetLightAmbient(pVecAmbient);
}

MeshFrame	*CAnimatedMesh::GetRootFrame(){
	return	m_smrRenderer.GetRootFrame();
}

HRESULT CAnimatedMesh::RestoreDeviceObjects(ID3D11DeviceContext *pContext){
	return m_smrRenderer.RestoreDeviceObjects(pContext);
}

HRESULT CAnimatedMesh::ReleaseDeviceObjects(){
	return m_smrRenderer.ReleaseDeviceObjects();
}
#endif

