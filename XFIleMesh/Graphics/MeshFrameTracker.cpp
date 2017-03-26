#include "stdafx.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include "MeshFrameTracker.h"
#include "Mesh.h"

CMeshFrameTracker::CMeshFrameTracker(MeshFrame *pFrame)
{
	m_pFrame = pFrame;
}


CMeshFrameTracker::~CMeshFrameTracker(void)
{
}

void CMeshFrameTracker::GetTransform(DirectX::XMMATRIX *pMatrix){
	*pMatrix = m_pFrame->CombinedMatrix;
}

MeshFrame *CMeshFrameTracker::FindFrame(MeshFrame *pFrame, CHAR *pName){
	MeshFrame *pResult = NULL;

	if (pFrame == NULL || pName == NULL)
		return NULL;

	if(pFrame->pName !=NULL){
		if(0 == strcmp(pFrame->pName,pName)){
			return pFrame;
		}
	}
	if (pFrame->pFrameSibling){
		pResult = CMeshFrameTracker::FindFrame(pFrame->pFrameSibling,pName);
		if (pResult)
			return pResult;
	}
		if (pFrame->pFrameFirstChild){
		pResult = CMeshFrameTracker::FindFrame(pFrame->pFrameFirstChild,pName);
		if (pResult)
			return pResult;
	}
	return pResult;
}
