#include "stdafx.h"
#include <D3D11.h>
#include <DirectXMath.h>

#include "IMesh.h"
#include "SimpleMesh.h"
#include "AnimatedMesh.h"
#include "SkinnedMesh.h"
#include "BoneRenderer.h"
#include "Animator.h"
#include "MeshFrameTracker.h"

ISimpleMesh *ISimpleMesh::CreateInstance(TCHAR *pFilename){
	ISimpleMesh *pResult = new CSimpleMesh(pFilename);
	if (pResult && pResult->GetRootFrame() == NULL){
		delete pResult;
		pResult = NULL;
	}
	return pResult;
}

ISimpleMesh::~ISimpleMesh(void)
{
}

IAnimatedMesh *IAnimatedMesh::CreateInstance(TCHAR *pFilename){
	CAnimatedMesh *pResult = new CAnimatedMesh(pFilename);
	if (pResult && pResult->GetRootFrame() == NULL){
		delete pResult;
		pResult = NULL;
	}
	return pResult;
}

IAnimatedMesh::~IAnimatedMesh(void)
{
}


ISkinnedMesh *ISkinnedMesh::CreateInstance(TCHAR *pFilename){
	ISkinnedMesh *pResult = new CSkinnedMesh(pFilename);
	if (pResult && pResult->GetRootFrame() == NULL){
		delete pResult;
		pResult = NULL;
	}
	return pResult;
}

ISkinnedMesh::~ISkinnedMesh(void)
{
}

IBoneRenderer *IBoneRenderer::CreateInstance(MeshFrame *pFrameRoot){
	return new CBoneRenderer(pFrameRoot);
}

IBoneRenderer::~IBoneRenderer(){
}

//  Factory method
IAnimator *IAnimator::CreateInstance(MeshFrame *pRoot){
	return new CAnimator(pRoot);
}

//  entity of pure virtual destructor
IAnimator::~IAnimator(){}


//  Factory
IMeshFrameTracker *IMeshFrameTracker::CreateInstance(MeshFrame *pRoot, CHAR *pName){
	MeshFrame *pFrame = CMeshFrameTracker::FindFrame(pRoot,pName);
	if (pFrame == NULL)
		return NULL;
	return new CMeshFrameTracker(pFrame);
}

//  entity of pure virtual destructor
IMeshFrameTracker::~IMeshFrameTracker(){}